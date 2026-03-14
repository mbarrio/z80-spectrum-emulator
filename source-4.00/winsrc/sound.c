#include <windows.h>
#include <mmsystem.h>   // for wave mapper functions
#include <mem.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "spectrum.h"

#define framesperblock 5      // to make 1 block > 2048 bytes; seems to be necess.
#define saveframesperblock 15	// safe saving

#define bcInBufLen 5000
#define MinWindowSize 5   		// min and max lengths of window over which digital
#define MaxWindowSize 31		//  filter seeks extrema in sample data
#define FCutOff 3333				// max correctly handled freq divided by 1.5
// 2731 = 4096/1.5
#define FFollow 400				// averager follows this frequency

#define FLoKnob 500
#define FHiKnob 4000

#define fmargin 0.85
#define alfa 0.09
#define amp 0x300
#define sampsiz 2					// size of one sample in bytes
#define oslen 16              // in samples, not in bytes (i.e. tailslen, sound.asm)
#define osres 64					// time resolution of samples
#define numvols 15				// number of different volume levels
#define producesourcefile 0	// must be 0

#define inbufferlen 4096
#define innumbuffers 4

typedef int WAVE;             // must be signed, and sizeof(WAVE)==sampsiz

HWAVEOUT hWaveOut;
WORD SampleRate;
WAVEHDR *wavebuf;
HGLOBAL hWavebuf=NULL;
HGLOBAL hWaveBfr=NULL;
HGLOBAL hInBuf=NULL;
// WORD TStatesPerSample;     // Defined in CORE.ASM
LONG TStatesPerBlock;
WORD SamplesPerBlock;
// LONG BytesPerBlock;     // Defined in CORE.ASM
long buftimelo[numbufs];
DWORD buftimehi[numbufs];
char  SpeedChanged;
DWORD ShutUpDelay;
LONG sent;
LONG received;
int BlocksInUse;
long  Curintperbit;     // used to store intperbit over tails
int kernellen;				// length of window for digital filtering
long alpha;					// used in filter for computing running average
const BYTE vocheader[]={"Creative Voice File\032\032\000\012\001\051\021"};

int	iSoundKnobs[7]={7,7,7,7,7,7,7};
long	alfalo,alfahi;
long	mulo,muhi;

long  InFilePos;
long  InInBlock;
long  InTimePos;
long  InTotalLength;
BOOL  InPlaying;
BYTE  InLastValue;
PCMWAVEFORMAT outpwf;
BOOL  SoundPossible;
int	oversampkernel[8];

void SilenceSound(void);

void initsound(void)
{
	float *wave;
	int i;
	soundsilent=1;
	BlocksInUse=0;
	SampleRate=22050;
	outpwf.wf.wFormatTag=WAVE_FORMAT_PCM;
	outpwf.wf.nChannels=1;
	outpwf.wf.nSamplesPerSec=SampleRate;
	outpwf.wf.nAvgBytesPerSec=SampleRate;
	outpwf.wf.nBlockAlign=1;
	outpwf.wBitsPerSample=8;
	hWaveOut=NULL;
	i=waveOutOpen(&hWaveOut,WAVE_MAPPER,(LPWAVEFORMAT)&outpwf,hWndMain,NULL,CALLBACK_WINDOW);
	SoundPossible=TRUE;
	if (i) {
		WriteInfoString("Could not open wave device at 22050 Hz, trying 11025 Hz");
		SampleRate=11025;
		outpwf.wf.nSamplesPerSec=SampleRate;
		outpwf.wf.nAvgBytesPerSec=SampleRate;
		i=waveOutOpen(&hWaveOut,WAVE_MAPPER,(LPWAVEFORMAT)&outpwf,hWndMain,NULL,CALLBACK_WINDOW);
	}
	if (i) {
		WriteInfoString("Could not open wave device at 11025 Hz; no sound.");
		state.sound=0;
		EnableMenuItem(GetMenu(hWndMain),CM_SOUND,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		SoundPossible=FALSE;
		SampleRate=22050;    // for sound recording
	}
	waveOutClose(hWaveOut);
	hWaveOut=NULL;
	// Initialise hi/lo averaging variables
	alfahi = (long)(65536.0*(1.0-exp(-(float)FHiKnob/(float)SampleRate)));
	alfalo = (long)(65536.0*(1.0-exp(-(float)FLoKnob/(float)SampleRate)));
	mulo = muhi = 0;;
	#if producesourcefile == 1
	{
		HGLOBAL hMem,hMem2;
		WAVE *WaveTable;
		FILE *fout;
		// Rest necessary anyway for sound recording
		hMem=GlobalAlloc(GMEM_MOVEABLE,1024*sizeof(float));
		hMem2=GlobalAlloc(GMEM_MOVEABLE,numvols*oslen*osres*sizeof(WAVE));
		wave=(float*)GlobalLock(hMem);
		WaveTable=(WAVE*)GlobalLock(hMem2);
		for (i=0;i<oslen*osres;i++) {
			if (i==oslen*osres/2)
				wave[i]=1;
			else
				wave[i]=exp(-alfa*alfa*M_PI*(i-oslen*osres/2)*(i-oslen*osres/2)/(osres*osres))*
					sin(fmargin*M_PI*(i-oslen*osres/2)/osres)/(fmargin*M_PI*(i-oslen*osres/2)/osres);
		}
		for (i=1;i<oslen*osres;i++) {
			wave[i]+=wave[i-1];           // Euler-integration.
		}
		for (j=0;j<numvols;j++) {
			for (i=0;i<oslen*osres;i++) {
				WaveTable[i+oslen*osres*j]=
					 (WAVE)((j+1)*amp*wave[osres-1-i/oslen+osres*(i%oslen)]/wave[oslen*osres-1]+0.5);
			}
		}
		for (i=0;i<oslen*osres;i++)			// adjust for quick DWORD adding method in asm
			if ((sampsiz*(i+1))&3)
				if (WaveTable[i]<0)
					WaveTable[i+1]--;
		fout=fopen("c:\\bc4\\spectrum\\asm\\wavetbl.asm","w+");
		fprintf(fout,"INOUT segment dword use16 public 'CODE'\n\npublic _WaveTable\n\n_WaveTable:\n");
		for (j=0;j<numvols*oslen*osres;j+=oslen*osres) {
			for (i=0;i<oslen*osres;i+=4/sampsiz) {
				if (i % (32/sampsiz))
					fprintf(fout,",%09lxh",*((DWORD*)&(WaveTable[i+j])));
				else
					fprintf(fout,"\n    dd %09lxh",*((DWORD*)&(WaveTable[i+j])));
			}
			fprintf(fout,"\n");
		}
		fprintf(fout,"\nINOUT ends\n\nend");
		fclose(fout);
		GlobalUnlock(hMem);
		GlobalFree(hMem);
		GlobalUnlock(hMem2);
		GlobalFree(hMem2);
	}
	#endif
	hWaveBfr=NULL;
	AdjustSoundKnobs();
}

void SilenceSound()     // called when there's nothing to play anymore
{
	int i;
	if (hWaveOut) {
		waveOutReset(hWaveOut);
		waveOutClose(hWaveOut);
		for (i=0;i<numbufs;i++) {
			waveOutUnprepareHeader(hWaveOut,&wavebuf[i],sizeof(WAVEHDR));
		}
	}
	hWaveOut=NULL;
}

void QuitSound()        // called when quitting the emulator entirely
{
	SilenceSound();
	GlobalUnlock(hWaveBfr);
	GlobalFree(hWaveBfr);
	GlobalUnlock(hWavebuf);
	GlobalFree(hWavebuf);
}


void AllocBuffers(void)
{
	int i;
	if (!hWavebuf) {
		hWavebuf=GlobalAlloc(GMEM_MOVEABLE,numbufs*sizeof(WAVEHDR));
		wavebuf=(WAVEHDR*)GlobalLock(hWavebuf);
	}
	if (hWaveBfr) {
		if (hWaveOut) {
			waveOutReset(hWaveOut);
			for (i=0;i<numbufs;i++) {
				waveOutUnprepareHeader(hWaveOut,&wavebuf[i],sizeof(WAVEHDR));
			}
		}
		GlobalUnlock(hWaveBfr);
		GlobalFree(hWaveBfr);
		hWaveBfr=NULL;
	}
	TStatesPerSample=((state.truepitch?100:state.speed)*(3500000L/16))/
						  ((SampleRate*100L)/16);      // 15 <= .. <= 3175
	TStatesPerBlock=(feearmicmask==8?saveframesperblock:framesperblock)*
								(3500000L/50); // 2.8 10^5
	SamplesPerBlock=(TStatesPerBlock/TStatesPerSample)&(-4L);
	if (((1000L*SamplesPerBlock)/SampleRate)<20) {  // length blk in ms
		SamplesPerBlock=((20L*SampleRate)/1000);
	}
	if (SamplesPerBlock<2304) {
		SamplesPerBlock=2304;
	}
	if (((1000L*SamplesPerBlock)/SampleRate)>320) {
		SamplesPerBlock=((320L*SampleRate)/1000);				// max 16 frames
	}
	if ((LONG)SamplesPerBlock*sampsiz*numbufs>0xFF80L) {
		SamplesPerBlock=0xFF80/(sampsiz*numbufs);
	}
	SamplesPerBlock &= (-4L);
	TStatesPerBlock=(LONG)SamplesPerBlock*TStatesPerSample;
	BytesPerBlock=sampsiz*(LONG)SamplesPerBlock;
	if (sound) {
		hWaveBfr=GlobalAlloc(GMEM_MOVEABLE,(LONG)BytesPerBlock*numbufs+sampsiz*oslen*2);
		wavebuf[0].lpData=GlobalLock(hWaveBfr);
		for (i=1;i<numbufs;i++)
			wavebuf[i].lpData=wavebuf[0].lpData+i*(LONG)BytesPerBlock;
		for (i=0;i<numbufs;i++) {
			// Word samples are reduced to Bytes before they're sent to device,
			//  so no BytesPerBlock here:
			wavebuf[i].dwBufferLength=SamplesPerBlock;
			wavebuf[i].dwFlags=NULL;
		}
		soundbufbase=(BYTE FAR*)wavebuf[0].lpData;
	}
	BlocksInUse=0;
	soundbuflen=(LONG)BytesPerBlock*numbufs;
	#if sampsiz==1
	soundlastval=(128-(amp/2))*0x1010101L;
	#else
	soundlastval=(0x8000-(amp/2))*0x10001L;
	#endif
	felastout=0;
	soundnextblock=0;
	sent=0;
	soundnooutyet=1;
	soundsilent=1;    // first OUT will reset all relevant 'internal' vars
	soundbufptr=0;    // this is referenced when processing msg_sampleblk
	soundnextblock=0; // this is referenced when executing the first OUT
}


void ResetAY(void)
{
	if (hmode<=hm_128k) {
		AYampa=AYampb=AYampc=10;		// this makes EAR output OFF to land on 0x80
		soundlastamp = 10+feamp;
		soundlastval = (soundlastamp*0x300L+0x80L)*0x10001L;
	}
	getcurrenttime();						// this updates soundtime(hi/lo)
	AYtimehi = soundtimehi;
	AYtimelo = soundtimelo;
}

void InitAY(void)
{
	BYTE i;
	BYTE b=fffdstate;
	for (i=0;i<16;i++) {
		fffdstate=i;
		AYout(soundregs[i]);
	}
	fffdstate=b;
}

void FlushRecordBuf(void)
{
	HFILE handle;
	HGLOBAL temphand=hSampleBuf;
	if (hSampleBuf) {
		hSampleBuf=NULL;
		handle=OpenReadWrite(gszSampleFile);
		if (handle==HFILE_ERROR) {
			frberror:
			state.record=FALSE;
			DestroyWindow(hRecordDialog); // also updates state.sound
			notify(SaveSampleError);
		} else {
			_llseek(handle,0,2);       // go to EOF
			if (savesampletype==LZF_VOC) {
				char hdr[6];
				hdr[0]=1;                              // sound data
				*(WORD*)&(hdr[1])=SampleBufLen+2;      // length
				hdr[3]=0;                              // high byte of length
				hdr[4]=256-(1000000L+(SampleRate/2))/SampleRate;   // s. rate
				hdr[5]=0;                              // compression type
				_lwrite(handle,hdr,6);
			}
			if (_lwrite(handle,GlobalLock(temphand),SampleBufLen)!=SampleBufLen) {
				_lclose(handle);
				goto frberror;
			}
			_lclose(handle);
		}
		GlobalUnlock(temphand);
		GlobalFree(temphand);
		SampleBufLen=0;
	}
}


void WriteSampleHeader(HFILE handle)
{
	switch (savesampletype) {
   case LZF_WAV:
		break;
	case LZF_RAW:
		break;
	case LZF_VOC:
		_lwrite(handle,vocheader,0x1a);
		break;
	}
}

void WriteSampleTrailer(void)
{
	HFILE handle;
	int localzero=0;

	if (savesampletype==LZF_RAW) return;
	if (savesampletype==LZF_WAV) return;
	handle=OpenReadWrite(gszSampleFile);
	if (handle==HFILE_ERROR) {
		state.record=FALSE;
		DestroyWindow(hRecordDialog);
		notify(SaveSampleError);
		return;
	} else {
		_llseek(handle,0,2);             // go to EOF
		_lwrite(handle,&localzero,1);    // VOC end marker: byte 00
   }
	_lclose(handle);
}


void WriteSilenceBlock(long time)
{
	HFILE handle;
	if (state.record && !state.recpaused) {
		FlushRecordBuf();
		if (!state.record) return;
		handle=OpenReadWrite(gszSampleFile);
		if (handle==HFILE_ERROR) {
			state.record=FALSE;
			DestroyWindow(hRecordDialog);
			notify(SaveSampleError);
			return;
		}
		_llseek(handle,0,2);
		if (savesampletype==LZF_VOC) {
			char sil[]={3,3,0,0,0,0,0xd3};
			*(WORD*)(&sil[4])=(time>148?0xFF00:441*time);
			if (time>0) {
				_lwrite(handle,sil,7);
				TotalRecorded += *(WORD*)(&sil[4]);
			}
		} else {
			WORD len;
			HGLOBAL hmem;
			BYTE *stilte;
			if (time<=0) goto endsil;
			len = (time*(SampleRate/50)>0xFF00?0xFF00:time*(SampleRate/50));
			hmem=GlobalAlloc(GMEM_MOVEABLE,len);
			if (!hmem) goto endsil;
			stilte=GlobalLock(hmem);
			memset(stilte,128,len);
			_lwrite(handle,stilte,len);
			TotalRecorded += len;
			GlobalUnlock(hmem);
			GlobalFree(hmem);
		}
		endsil:
		_lclose(handle);
	}
}


void PostProcess(int num)
{
	LONG buftlo,bufthi;
	LONG gamma1,gamma2,gamma3;
	LONG alfalo1,alfahi1;
	LONG sig;
	WORD *inbuf;
	BYTE *outbuf;
	int i;
	bufthi = buftimehi[num];
	buftlo = buftimelo[num];
	currah_speak((WORD*)wavebuf[num].lpData,bufthi,buftlo,SamplesPerBlock);
	outbuf = (BYTE*)inbuf = (WORD*)wavebuf[num].lpData;
	alfalo1 = 65536L-alfalo;
	alfahi1 = 65536L-alfahi;
	gamma1 = (32768L * iSoundKnobs[6] * iSoundKnobs[5])/(15*10);
	gamma2 = (256*( iSoundKnobs[4] - 7 ) * iSoundKnobs[6] )/(15*10);
	gamma3 = (256*( 7 - iSoundKnobs[5] ) * iSoundKnobs[6] )/(15*10);
	for (i=0;i<SamplesPerBlock;i++) {
//		addsig +=
		sig = ((long)inbuf[i]-32768L);
//		if ((i&3)==0) {
//			mulo = ((alfalo1/256)*mulo+alfalo*(addsig/4))/256;
//			addsig=0;
//		}

//		mulo = (alfalo1*mulo)/65536L + alfalo*sig;
		mulo = muladd32(alfalo1,mulo,alfalo,sig);
//		mulo = (alfalo1/256)*(mulo/256) + alfalo*sig;
//		muhi = (((alfahi1/256)*muhi) + alfahi*sig)/256;
		muhi = muladd32(alfahi1,muhi,alfahi,sig);
//		muhi = (alfahi1/256)*(muhi/256) + alfahi*sig;
		outbuf[i] = 128+(gamma1*sig + gamma2*(mulo/256) + gamma3*(muhi/256))/(32768L*256L);
	}
}


void SaveBlock(int num)
{
	// Note that here SamplesPerBlock is used instead of BytesPerBlock;
	//  we're not writing word samples, but reduced (byte) samples
	HGLOBAL hnew;
	char far* sbuffer;
	if (state.record && !state.recpaused) {
		if (SampleBufLen<maxsamplebuflen)
			hnew=GlobalReAlloc(hSampleBuf,SampleBufLen+SamplesPerBlock,NULL);
		else
			hnew=NULL;
		if (!hnew) {
			FlushRecordBuf();
			if (!state.record) return;
			hnew=GlobalAlloc(GMEM_MOVEABLE,SamplesPerBlock);
			if (!hnew) {
				state.record=FALSE;
				DestroyWindow(hRecordDialog);
				notify(MemSampleError);
				return;           // bad error
			}
		}
		hSampleBuf=hnew;
		sbuffer=GlobalLock(hSampleBuf);
		_fmemcpy(sbuffer+(WORD)SampleBufLen,wavebuf[num].lpData,SamplesPerBlock);
		SampleBufLen+=SamplesPerBlock;
		GlobalUnlock(hSampleBuf);
		TotalRecorded+=SamplesPerBlock;
		SendMessage(hRecordDialog,WM_USER+1,0,0L);
	}
}


void PlayBlock(int num)
{
	if (state.sound && SoundPossible) {
		if (!hWaveOut) {
			int i;
			i=waveOutOpen(&hWaveOut,WAVE_MAPPER,(LPWAVEFORMAT)&outpwf,hWndMain,NULL,
				CALLBACK_WINDOW);
			if (i) return;
			sent=0;
			BlocksInUse=0;
			for (i=0;i<numbufs;i++) wavebuf[num].dwFlags=0;
		}
		if (wavebuf[num].dwFlags)
			{fatalerror(Fatal2);return;}
		if (waveOutPrepareHeader(hWaveOut,&wavebuf[num],sizeof(WAVEHDR)))
			return;
		if (waveOutWrite(hWaveOut,&wavebuf[num],sizeof(WAVEHDR)))
			return;
		BlocksInUse++;
		if (BlocksInUse>numbufs-3) BlockDone();
//   if (BlocksInUse) fatalerror(Fatal1);
		sent+=SamplesPerBlock;
	}
}


void SilenceBlock(int num)
// This silences a block in byte format; it does not need to be PostProcess'ed
// Assumes sampsiz==2
{
	WORD curval = 0x2080+amp*soundlastamp;
	int diff=abs((int)(curval-0x8080))/256;


	int i,k=1,j=0;
	for (i=0;i<diff;i++) {
		k=(k*13)%257;
		j=max(j,i+(k>>5));
		((BYTE*)(wavebuf[num].lpData))[i]=
			(((long)i*0x8080+(long)(diff-i)*curval)/diff)>>8;
	}
	memset(&(wavebuf[num].lpData[diff]),128,SamplesPerBlock-diff);
}



void BlockDone(void)
{
	int i;
	if (!hWaveOut) return;
	for (i=0;i<numbufs;i++) {
		if (wavebuf[i].dwFlags & WHDR_DONE) {
			waveOutUnprepareHeader(hWaveOut,&wavebuf[i],sizeof(WAVEHDR));
			wavebuf[i].dwFlags = NULL;
			if (BlocksInUse) BlocksInUse--;
			received++;
		}
	}
	// If wave device isn't playing, close it
	if ((BlocksInUse==0)&&(soundsilent)) SilenceSound();
}

void setsamplerate(WORD tperbit)
{
	// This routine sets internal vars (mult. factors) for filtering
	// routine, sets intperbit which is used in the calculations, and
	// sets Curintperbit, which stores the value of intperbit over ''tails''.
	// The ''2*'' below is because internally every sample is oversampled
	// twice
	long SampleRate = 1750000L/tperbit;
	long result=0;
	float x;
	int i;
	intperbit=Curintperbit=tperbit;

	// now initialise oversampkernel, for oversampling the sound card
	// or .VOC file input
	result=0;
	for (i=0;i<8;i++) {
		x=(i-3.5)*M_PI;
		result+=256*exp(-0.015*x*x)*(sin(x)/x);
	}
	for (i=0;i<8;i++) {
		x=(i-3.5)*M_PI;
		oversampkernel[i]=(65536.0*exp(-0.015*x*x)*(sin(x)/x))/result;
	}

	 kernellen = ((FCutOff/2)+2*SampleRate) / FCutOff;
	 kernellen = max(min(kernellen,MaxWindowSize),MinWindowSize);
	 kernellen |= 1;

	alpha = 256/(1+SampleRate/FFollow);
}

void initinning()
{
	inning=0;
	if (!hInBuf) hInBuf=GlobalAlloc(GMEM_MOVEABLE,bcInBufLen+16);   // See inout.asm for
	if (!hInBuf) return;                               				 // reason for 16
	inbufbase=(DWORD FAR*)GlobalLock(hInBuf);
	inbuflen=bcInBufLen*8;
	setsamplerate(1750000L/SampleRate);
}

void resetinning()
{
	inning=0;
	if (hInBuf) {
		GlobalUnlock(hInBuf);
		GlobalFree(hInBuf);
	}
	inbufbase=NULL;
	hInBuf=NULL;
}

BYTE filter(BYTE input)
// filters sound input by oversampling twice using oversampkernel, then
//  applies a simple digital filter.  Returns two bits, bit 0 is first
// Requires kernellen and oversampkernel to be init'ed (setsamplerate)
{
	static BYTE oversampbuf[8];
	static BYTE signal[MaxWindowSize];
	static BYTE curstate=0;
	static BYTE curextr=0x80;
	static long average=0;
	BYTE maxb,minb,b;
	BYTE filt=0;
	long result=0;
	int i;

   return (input>0x80)*3;

	average += alpha * (((long)input-0x80) - ((average+0x80)>>8) );

	maxb=minb=oversampbuf[4];
	for (i=0;i<kernellen-1;i++) {
		 if ((b=signal[i]=signal[i+1])>maxb) maxb=b;
		 if (b<minb) minb=b;
	}
	signal[i]=oversampbuf[4];
//	if (signal[kernellen/2]==maxb) curstate=0;
//	if (signal[kernellen/2]==minb) curstate=3;
	if ((signal[kernellen/2]==maxb)&&curstate)
		if (maxb>(3*(average>>8)+curextr+0x180)/4) {curstate=0;curextr=maxb;}
	if ((signal[kernellen/2]==minb)&&(!curstate))
		if (minb<(3*(average>>8)+curextr+0x180)/4) {curstate=3;curextr=minb;}
	filt = curstate & 1;
	for (i=0;i<7;i++) result+=
		 (long)oversampkernel[i]*(oversampbuf[i]=oversampbuf[i+1]);
	result=min(0xFF00,max(0,result+oversampkernel[7]*(oversampbuf[7]=input)));
	maxb=minb=result/256;
	for (i=0;i<kernellen-1;i++) {
		if ((b=signal[i]=signal[i+1])>maxb) maxb=b;
		if (b<minb) minb=b;
	}
	signal[i]=result/256;
//	if (signal[kernellen/2]==maxb) curstate=0;
//	if (signal[kernellen/2]==minb) curstate=3;
	if ((signal[kernellen/2]==maxb)&&curstate)
		if (maxb>(3*(average>>8)+curextr+0x180)/4) {curstate=0;curextr=maxb;}
	if ((signal[kernellen/2]==minb)&&(!curstate))
		if (minb<(3*(average>>8)+curextr+0x180)/4) {curstate=3;curextr=minb;}
	return (filt | (curstate&2));
}

void readinbuffer(skip)
long skip;
{
	// Loads part of sample file, filters it, and stores a two-times oversampled
	//  copy of the contents in the sample bit buffer, to be processed by the
	//  emulator.
	// skip is time to skip in ms from curpos in sample file to now.  If ==0
	//  time to skip is time from last sample in buffer to now.
	DWORD tskip,sskip,skipnow;
	BYTE b;
	HFILE hInFile;
	static BYTE htype;
	DWORD len;

	getcurrenttime();       // soundtimelo/hi are set now
	if (skip!=-1)           // -1 means buffer empty, <>-1 go to t='skip'
		tskip=skip*3500;
	else
		tskip=(soundtimehi-intbasehi)*tframe+soundtimelo-intbaselo;
	hInFile=OpenRead(gszInFile);
	if (hInFile==HFILE_ERROR) {
		goto psf_error;
	}
	if ((loadsampletype==LZF_RAW)&&(InInBlock==0)) {
		InInBlock = _llseek(hInFile,0,2)-InFilePos;
		htype = 1;
	}
	sskip=tskip/(2*intperbit);
	if (sskip) InLastValue=128;
	skipnow = min(sskip,InInBlock);
	InInBlock-=skipnow;
	InFilePos+=skipnow;
	tskip-=skipnow*2*intperbit;
	InTimePos+=(skipnow*2*intperbit)/3500;
	while (InInBlock==0) {
		_llseek(hInFile,InFilePos,0);
		if (_lread(hInFile,&htype,1)!=1) {
			if (loadsampletype==LZF_RAW) goto psf_eof;
			psf_error:
			inning=0;
			InPlaying=FALSE;
			_lclose(hInFile);
			if (hPlaySample) PostMessage(hPlaySample,PSF_RESET,0,0);
			notify(SampleFileError);
			return;
		}
		if (htype==0) {      // EOF
			psf_eof:
			inning=0;
			if (skip==-1) {
				InPlaying=FALSE;
				_lclose(hInFile);
				if (hPlaySample) PostMessage(hPlaySample,PSF_RESET,1,0);
				message(SampleFileEnd);
			}
			return;
		}
		len=0;
		if (_lread(hInFile,&len,3)!=3) {
			goto psf_error;
		}
		InFilePos+=4;
		switch (htype) {
		case 1:              // sound data.  Compression type is ignored
			InInBlock=len-2;
			len=0;
			_lread(hInFile,&len,1);
			setsamplerate(((256-len)*175L)/100L);
			InFilePos+=2;
			break;
		case 2:              // sound continue
			InInBlock=len;
			setsamplerate(Curintperbit);
			break;
		case 3:
			if (len!=3) {
				goto psf_error;
			}
			InFilePos+=len;
			_lread(hInFile,&len,2);
			InInBlock=len;
			len=0;
			_lread(hInFile,&len,1);
			intperbit=(((256-len)*175L)/100L)*InInBlock;
			InInBlock=1;
			// intperbit = (1/2) * time of silence block
			break;
		default:
			InFilePos+=len;
			break;
		}
		sskip=tskip/(2*intperbit);
		if (sskip) InLastValue=128;
		skipnow = min(sskip,InInBlock);
		InInBlock-=skipnow;
		if (htype!=3) InFilePos+=skipnow;
		tskip-=skipnow*2*intperbit;
		InTimePos+=(skipnow*2*intperbit)/3500;
		if (skip!=-1) YieldIfYeNeed();
	}
	if (htype==3) {
		InInBlock=0;
		inbuflen=2;       // 2 bits
		inbufbase[0]=0;   // set bits
	} else {
		DWORD toread = min(bcInBufLen*4,InInBlock);
		HGLOBAL htemp;
		BYTE FAR *temp;
		int i;
		register int j;
		if ((toread & 15)&&(toread>15)) toread &= 0xfff0L;
		_llseek(hInFile,InFilePos,0);
		htemp = GlobalAlloc(GMEM_MOVEABLE,toread);
		if (!htemp) {
			inning=0;
			InPlaying=FALSE;
			_lclose(hInFile);
			if (hPlaySample) PostMessage(hPlaySample,PSF_RESET,0,0);
			notify (FatalMemAlloc);
			return;
		}
		temp=(BYTE FAR*)GlobalLock(htemp);
		if (_lread(hInFile,temp,toread)!=toread) {
			GlobalUnlock(htemp);
			GlobalFree(htemp);
			goto psf_error;
		}
		InInBlock-=toread;
		InFilePos+=toread;
		if (toread&15) {
			// Subsequent blocks will reset intperbit and other vars to value
			// stored in Curintperbit
			intperbit = Curintperbit/16;
			for (i=0;i<toread;i++) {
				b=filter(temp[i]);
				inbufbase[i]=(b&1) ? 0xffffL : 0;
				inbufbase[i]+=(b&2) ? 0xffff0000L : 0;
			}
			inbuflen=32*toread;     // # of bits
			InTimePos+=(intperbit*inbuflen)/3500;
		} else {
			register DWORD data;
			for (i=0;i<toread;i+=16) {
				data=0;
				for (j=0;j<16;j++) {
					data|=(DWORD)((DWORD)filter(temp[i+j])<<(2*j));
				}
				inbufbase[i/16]=data;
			}
			inbuflen=2*toread;      // # of bits
			InTimePos+=(intperbit*inbuflen)/3500;      // = time at e.o. blk
		}
		GlobalUnlock(htemp);
		GlobalFree(htemp);
	}
	// now advance 'time corresp. to first sample in buffer' variables.
	// tskip is left-over T states (may be large when in midst of silence blk
	// New 'start of buffer time' is current time minus tskip
	intbaselo = soundtimelo - tskip;
	intbasehi = soundtimehi;
	if (intbaselo<0) {
		intbasehi -= ((-intbaselo)/tframe)+1;
		intbaselo = tframe - ((-intbaselo)%tframe);
		// Ugly.  Why isn't there a %p that satisfies 0<=%p<p always?
	}
	_lclose(hInFile);
}


BYTE	tzx_curblock;		// current block type; 0=none
// long TapeInFilePos
// long TapeInBlockPos
// char *gszPlayTapFile
#pragma option -a1
// byte align:
struct {
	WORD pilot;		// pilot pulse len
	WORD sync1;		// sync1 pulse len
	WORD sync2;		// sync2 pulse len
	WORD zero;		// zero bit pulse len
	WORD one;		// one bit pulse len
	WORD pilotlen;	// # pilot pulses
	BYTE blb;		// # bits in last byte
	WORD pause;		// pause after data in ms
	DWORD datlen;	// (3-byte) length of data
}	tzx_datahdr;
#pragma option -a.
char	tzx_out;			// current output level
BYTE	tzx_outtype;	// 0=pure tone 1=pilot 2,3=sync1,2 4=data 5=pause 6=pulse 7=dirrec
WORD	tzx_numpulses;	// pulses in pure tone or pulse block
long	tzx_bytesleft;	// bytes left in data block or direct recording block
HGLOBAL 	tzx_selectdata=NULL;	// data for user selection menu
HWND		tzx_selectwindow=NULL;
WORD		tzx_selectblock;		// user selection


void readtzxfile(skip)
long skip;
{
	// Reads part of TZX file, and puts result in sample bit buffer.
	// skip is ignored.
	HFILE hInFile;
	long timediff;
	DWORD d;
	WORD w1;
	long l;
	int i;
	int b0,b1,p0,q0,p1,q1;
	long bitpos;
	long bytepos,bytesread;
	HGLOBAL htemp;
	BYTE FAR *temp;

	// Algorithm:
	// 1. update time base of sound bit buffer
	// 2. read portion of TZX file, and store in sound bit buffer
	// 3. repeat until time of last sample in sound bit buffer is > current time

	hInFile=OpenRead(gszPlayTapFile);
	inbuflen = 0;		// INOUT.ASM has updates intbase already, so don't do it below
	do {
		intbaselo += intperbit * inbuflen;
		intbasehi += (intbaselo / tframe);
		intbaselo %= tframe;
		inbuflen = 0;

		if (TapeInFilePos==-1) {		// first seek current block if necessary
			WindTape(hInFile);
			tzx_curblock=0;
			tzx_out = 0;			// set current output level to 'low'
		} else {
			_llseek(hInFile,TapeInFilePos,0);
		}
		if (!tzx_curblock) {		// no current block; load block type byte
			if (hPlayTapDialog)
				PostMessage(hPlayTapDialog,WM_USER+1,0,0);	// show progress
			if (_lread(hInFile,&tzx_curblock,1)!=1) {
				// end of TZX file
				_lclose(hInFile);
				SendMessage(hPlayTapDialog,WM_COMMAND,PT_MOVE,0);	// stop tape!
//				message(TZXFileEnd);
				return;
			}
			if (TapeInFilePos==0) {	// first block?
				tzx_out = 0;		// set current output level to 'low'
			}
			TapeInFilePos++;
			switch (tzx_curblock) {
			case 0x10:		// ordinary block
				tzx_datahdr.pilot = 2168;
				tzx_datahdr.sync1 = 667;
				tzx_datahdr.sync2 = 735;
				tzx_datahdr.zero = 855;
				tzx_datahdr.one = 1710;
				tzx_datahdr.pilotlen = 8064;		// short is 3220
				tzx_datahdr.blb = 8;
				tzx_datahdr.pause = 1000;
				tzx_datahdr.datlen = 0;
				if (_lread(hInFile,&(tzx_datahdr.pause),5) != 5) {
					tzx_err:
					inning=0;
					TzxInPlaying = 0;
					_lclose(hInFile);
					message(TZXFileError);
					return;
				}
				TapeInFilePos+=4;
				if (tzx_datahdr.datlen & 0x800000L) {	// not header but data
					tzx_datahdr.pilotlen = 3220;
				}
				tzx_bytesleft = tzx_datahdr.datlen & 0xFFFFL;
				tzx_outtype = 1;
				break;
			case 0x11:		// turbo block
				tzx_datahdr.datlen = 0;
				if (_lread(hInFile,&(tzx_datahdr.pilot),18) != 18)
					goto tzx_err;
				TapeInFilePos+=18;
				tzx_bytesleft = tzx_datahdr.datlen;
				tzx_outtype = 1;
				break;
			case 0x12:		// pure tone
				if (_lread(hInFile,&(tzx_datahdr.pilot),4) != 4)
					goto tzx_err;
				TapeInFilePos+=4;
				tzx_datahdr.pilotlen = tzx_datahdr.sync1;
				tzx_outtype = 0;
				tzx_bytesleft = 0;
				break;
			case 0x13:		// pulse train
				tzx_numpulses=0;
				if (_lread(hInFile,&tzx_numpulses,1) != 1)
					goto tzx_err;
				TapeInFilePos += 1;
				tzx_bytesleft = 2*tzx_numpulses;
				tzx_outtype = 6;
				break;
			case 0x14:		// pure data
				tzx_datahdr.datlen = 0;
				if (_lread(hInFile,&(tzx_datahdr.one),10) != 10)
					goto tzx_err;
				tzx_datahdr.zero = tzx_datahdr.one;		// shift down because of pilotlen
				tzx_datahdr.one = tzx_datahdr.pilotlen;
				TapeInFilePos+=10;
				tzx_bytesleft = tzx_datahdr.datlen;
				tzx_outtype = 4;
				break;
			case 0x15:		// direct recording
				tzx_datahdr.datlen = 0;
				if (_lread(hInFile,&(tzx_datahdr.one),5) != 5)
					goto tzx_err;
				if (_lread(hInFile,&(tzx_datahdr.datlen),3) != 3)
					goto tzx_err;
				TapeInFilePos += 8;
				tzx_datahdr.pause = tzx_datahdr.pilotlen;
				// 'one' contains the sample rate, in T's per bit.
				tzx_bytesleft = tzx_datahdr.datlen;
				tzx_outtype = 7;
				break;
			case 0x20:		// pause
				if (_lread(hInFile,&(tzx_datahdr.pause),2) != 2)
					goto tzx_err;
				TapeInFilePos += 2;
				tzx_bytesleft = 0;
//				if (tzx_datahdr.pause) {
//					tzx_outtype = 5;
//				} else {
//					tzx_curblock = 0;
				tzx_outtype = 5;
				if (!tzx_datahdr.pause) {
					TapeInBlockPos++;
					SendMessage(hPlayTapDialog,WM_USER+1,0,0);			// update bar
					SendMessage(hPlayTapDialog,WM_COMMAND,PT_MOVE,0);	// pause, indefinite
					TapeInBlockPos--;
				}
				break;
			case 0x23:		// relative jump
				if (_lread(hInFile,&i,2) != 2)
					goto tzx_err;
				TapeInFilePos = -1;
				TapeInBlockPos += i-1;		// -1 compensates ++ below as _curblock=0
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x24:		// FOR loop
				if (_lread(hInFile,&TzxForCounter,2) != 2)
					goto tzx_err;
				TapeInFilePos += 2;
				TzxForPos = TapeInFilePos;
				TzxForBlk = TapeInBlockPos + 1;
				TzxForLooping = 1;
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x25:		// NEXT
				if (TzxForLooping) {
					TzxForCounter--;
					if (TzxForCounter) {
						TapeInFilePos = TzxForPos;
						TapeInBlockPos = TzxForBlk - 1;
					} else {
						TzxForLooping=0;
					}
				}
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x26:		// CALL sequence
				TzxCallPos = TapeInFilePos;
				TzxCallBlk = TapeInBlockPos;
				TzxReturnAddress = 1;
				// continue into RETURN code
			case 0x27:		// RETURN from CALL
				if (!TzxReturnAddress) {		// no CALL to RETURN to
					TapeInFilePos = _llseek(hInFile,0,2);		// go to end
				} else {
					_llseek(hInFile,TzxCallPos+1,0);				// go to CALL blk
					_lread(hInFile,&w1,2);
					if (w1 < TzxReturnAddress) {					// no more calls
						TapeInFilePos = TzxCallPos+2*w1+3;
						TapeInBlockPos = TzxCallBlk+1;
						TzxReturnAddress = 0;
					} else {
						_llseek(hInFile, TzxCallPos+2*TzxReturnAddress+1, 0);
						_lread(hInFile,&TapeInBlockPos,2);
						TapeInFilePos = -1;
						TzxReturnAddress++;
					}
				}
				tzx_curblock = 0;
				tzx_bytesleft = 0;
				break;
			case 0x28:		// 'select' block
				tzx_bytesleft = 0;
				tzx_outtype = 5;						// pause
				timediff = ((long)(intbasehi - soundtimehi))*(long)tframe +
								(long)intbaselo - (long)soundtimelo;
				tzx_datahdr.pause = 1+(timediff/3500);	// # ms
				TapeInFilePos = -1;
				TapeInBlockPos--;						// return to this blk if more pause necessary
				SetPauseState(1);						// stop emulator
				if (tzx_selectwindow) {				// already selecting
					if (tzx_selectblock != 0xFFFF) {		// user selected something
						TapeInBlockPos = tzx_selectblock-1;	// compensates ++ at bottom
						tzx_curblock = 0;
						SetPauseState(0);
						SendMessage(tzx_selectwindow,WM_CLOSE,0,0);
					}
					break;
				}
				_lread(hInFile,&w1,2);				// length
				w1=0;                            // throw away
				_lread(hInFile,&w1,1);				// # selections
				{
					USERSELBLOCK *blk;
					int k;
					tzx_selectdata=GlobalAlloc(GPTR,(w1+1)*sizeof(USERSELBLOCK));
					if (!tzx_selectdata) {
						notify(FatalUserSelMemAlloc);
						SetPauseState(0);
						break;
					}
					blk = (USERSELBLOCK*)GlobalLock(tzx_selectdata);
					k=0;
					while (k<w1) {
						_lread(hInFile,&i,2);		// rel offset
						blk[k].blockpos = TapeInBlockPos + (long)i + 1;		// compensates -- before
						i=0;
						_lread(hInFile,&i,1);		//  length of txt
						_lread(hInFile,blk[k].title,i);
						blk[k].title[i]=0;
						k++;
					}
					blk[w1].blockpos = 0xFFFF;
					GlobalUnlock(tzx_selectdata);
					tzx_selectwindow = MyCreateDialogParam(ghInstance,"TZXSELECT",hWndMain,TzxSelectDialProc,WIN_TSB);
					tzx_selectblock = 0xFFFF;
				}
				break;
			case 0x2a:
				TapeInFilePos += 4;		// skip length dword (contains 0)
				tzx_bytesleft = 0;
				if (hmode <= hm_48kmgt) {
					tzx_outtype = 5;
					TapeInBlockPos++;
					SendMessage(hPlayTapDialog,WM_USER+1,0,0);			// update bar
					SendMessage(hPlayTapDialog,WM_COMMAND,PT_MOVE,0);	// pause, indefinite
					TapeInBlockPos--;
				} else {
					tzx_curblock = 0;
				}
				break;
			default:			// all other blocks are skipped
				TapeInFilePos--;							// back up to type byte
				_llseek(hInFile,TapeInFilePos,0);		// set file pos
				TapeInFilePos += BlockLen(hInFile);	// skip current block
				tzx_curblock = 0;						// set 'no current block, load new'
				tzx_bytesleft = 0;
				break;
			}
		}
		// If (tzx_curblock), translate current block into bit buffer.
		// If not, just continue into next
		if (tzx_curblock) {
			switch (tzx_outtype) {
			case 0:	// pure tone
			case 1:	// pilot tone
				if (tzx_out)
					d = 0x55555555L;	// lsb played first, high
				else
					d = 0xAAAAAAAAL;		// lsb played first, low
				// compute # of bits to play now
				inbuflen = min((long)tzx_datahdr.pilotlen, (long)bcInBufLen*8L);
				// set sample rate
				intperbit = tzx_datahdr.pilot;
				// compute new tzx_out
				if (inbuflen & 1)
					tzx_out = !tzx_out;
				// compute # of pulses left, new block if none
				tzx_datahdr.pilotlen -= inbuflen;
				if (!tzx_datahdr.pilotlen) {
					if (tzx_outtype==0) {	// pure tone
						tzx_curblock = 0;
					} else {						// pilot tone
						tzx_outtype++;
					}
				}
				// actually fill the sound bit buffer
				l = ((inbuflen+31)/32);
				for (i=0;i<l;i++)
					inbufbase[i] = d;
				break;
			case 2:	// sync1
			case 3:	// sync2
				inbufbase[0] = tzx_out;
				tzx_out = !tzx_out;
				if (tzx_outtype==2)
					intperbit = tzx_datahdr.sync1;
				else
					intperbit = tzx_datahdr.sync2;
				inbuflen = 1;
				tzx_outtype++;
				break;
			case 4:	// data
				// First compute basic time step dt, and integers such that
				//  p*dt and q*dt are approximations to lengths of bit 0 and 1 (b0, b1)
				// Constraints: (p+q)*dt = (b0+b1),  |b0-p*dt|<58 T, |b1-q*dt|<58 T
				//  (best compare time remains same, absolute difference approx. 1 loop
				//   through standard sample routine)
				// Algorithm: sort of Euclidian GCD
				b0 = tzx_datahdr.zero;
				b1 = tzx_datahdr.one;
				intperbit = b0+b1;
				p0 = 1; q0 = 0;
				p1 = 0; q1 = 1;
				#define swap(a,b) a^=b^=a^=b
				#define sgn(a) (a<0?-1:1)
				#define mydiv(a,b) (sgn(a)*sgn(b)*(abs(a)+abs(b)/2)/abs(b))
				while (abs(b0)>=58 * (abs(p0)+abs(q0))) {
					i = mydiv(b1,b0);
					b1 -= i*b0;
					q1 -= i*q0;
					p1 -= i*p0;
					swap(b0,b1);
					swap(p0,p1);
					swap(q0,q1);
				}
				intperbit /= (abs(p0)+abs(q0));
				p0 = abs(p0);
				q0 = abs(q0);
				swap(p0,q0);
				// Now p0 and q0 are multipliers for bit 0 and 1 respectively
				for (i=0;i<bcInBufLen/4;i++)
					inbufbase[i]=0;
				// Compute how many bytes to load.  Make sure bit buffer is large enough
				bytesread = min(tzx_bytesleft, ((long)bcInBufLen*8)/(2*max(p0,q0)*8));
				htemp = GlobalAlloc(GMEM_MOVEABLE,bytesread);
				if (htemp==NULL) {
					_lclose(hInFile);
					fatalerror(FatalMemAlloc);
					inning=0;
					return;
				}
				temp = GlobalLock(htemp);
				if (_lread(hInFile, temp, bytesread) != (WORD)bytesread)
					goto tzx_err;
				// translate bytes into sample bits
				bitpos=0;
				tzx_bytesleft -= bytesread;
				TapeInFilePos += bytesread;
				#define setsampbit(i) inbufbase[(i)/32] |= ((DWORD)1<<((i)&31));
				for (bytepos=0;bytepos<bytesread-1;bytepos++) {
					for (i=7;i>=0;i--) {
						if (temp[bytepos] & (1<<i))
							p1 = q0;		// bit 1
						else
							p1 = p0;		// bit 0
						if (!tzx_out) bitpos+=p1;
						for (q1=0;q1<p1;q1++) {
							setsampbit(bitpos);
							bitpos++;
						}
						if (tzx_out) bitpos+=p1;
					}
				}
				for (i=7;i>=(tzx_bytesleft?0:8-tzx_datahdr.blb);i--) {
					if (temp[bytepos] & (1<<i))
						p1 = q0;		// bit 1
					else
						p1 = p0;		// bit 0
					if (!tzx_out) bitpos+=p1;
					for (q1=0;q1<p1;q1++) {
						setsampbit(bitpos);
						bitpos++;
					}
					if (tzx_out) bitpos+=p1;
				}
				GlobalUnlock(htemp);
				GlobalFree(htemp);
				if (!tzx_bytesleft) {
					tzx_outtype++;
					// If TZX file ends with zero pause, then state of EAR port is
					// undefined and final edge may not be produced, according to
					// TZX specification.  However some TZX files depend on the EAR
					// going low (ghould'n'ghosts, game over 2, winter games).  So
					// here's a patch.  It is not foolproof.
					i=0;
					if (!_lread(hInFile, &i, 1) || (i==0x22)) {
						// no more blocks, or 'group end' block
						if (!tzx_datahdr.pause)
							tzx_datahdr.pause = 1;		// add 1ms pause
					}
					// Next fix:
					// If last pulse is 'low', and there is some pause, add a final
					// short 'high' pulse to make sure final edge is present.  This
					// is NOT according to TZX specification, but some .TZX files need
					// it.
					if (tzx_out && tzx_datahdr.pause) {
						for (i=0;i<p0+q0;i++) {
							setsampbit(bitpos);
							bitpos++;
						}
					}
				}
				inbuflen = bitpos;
				break;
			case 5:	// pause
				if (tzx_datahdr.pause)
					tzx_out=0;	 				// low level
				l = min(bcInBufLen/4, 1+tzx_datahdr.pause/32);
				for (i=0;i<l;i++)
					inbufbase[i]=0;
				intperbit = tframe/20;		// 1 ms
				inbuflen = min((long)bcInBufLen*8, (long)tzx_datahdr.pause);
				tzx_curblock=0;
				break;
			case 6:	// pulse
				do {
					if (_lread(hInFile,&(tzx_datahdr.pilot),2) != 2)
						goto tzx_err;
					TapeInFilePos += 2;
					tzx_bytesleft -= 2;
					inbufbase[0] = tzx_out;
					tzx_out = !tzx_out;
					intperbit = tzx_datahdr.pilot;
					inbuflen = 1;
					if (!tzx_bytesleft)			// no more pulses? Then stop.
						tzx_curblock=0;
				} while ( (tzx_curblock && !intperbit) );		// Necessary for levia48.tzx
				if (!intperbit) {
					intperbit = 1;
					inbuflen = 0;
				}
				break;
			case 7:	// direct recording, then pause
				bytesread = min(bcInBufLen, tzx_bytesleft);
				tzx_bytesleft -= bytesread;
				TapeInFilePos += bytesread;
				if (_lread(hInFile,inbufbase,bytesread) != bytesread)
					goto tzx_err;
				intperbit = tzx_datahdr.one;		// see init code above
				if (tzx_bytesleft)
					inbuflen = 8*bytesread;
				else {
					inbuflen = 8*bytesread - 8 + tzx_datahdr.blb;
					tzx_out = !!(((char*)inbufbase)[bytesread-1] & (0x80 >> (tzx_datahdr.blb-1)));
					/* bugfix here... Noted by Woitek Wasilewski */
					tzx_outtype = 5;					// pause
				}
				for (i=0;i<=bytesread/2;i++) {
					w1 = ((WORD*)inbufbase)[i];
					_asm {
						mov cx,16
						mov dx,w1
					reverse_bits:
						shl dx,1
						rcr ax,1
						loop reverse_bits
						xchg ah,al
						mov w1,ax
					}
					((WORD*)inbufbase)[i] = w1;
				}
				break;
			}
		}
		if (!tzx_curblock) {		// i.e. block finished
			TapeInBlockPos++;                         	// advance block counter
		}
		// compute start-of-buffer - now
		timediff = ((long)(intbasehi - soundtimehi))*(long)tframe +
						(long)intbaselo - (long)soundtimelo;
		// add buffer, i.e. end-of-buffer - now
		timediff += intperbit * inbuflen;
	} while (timediff <= 0);
	_lclose(hInFile);
}


void initTZXinning(void)
{
	initinning();
	iimode = !!iimode;
	getcurrenttime();
	intbaselo = soundtimelo;
	intbasehi = soundtimehi;
	intperbit = 1000;
	inbuflen = 0;
	inning = 1;
}


//////////////////////////////////////////////////////////////////////////////
// Below is the code for real-time loading via AD converter (i.e. SoundBlaster)
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
	HGLOBAL  nextinqueue;      // handle of next block
	LONG     numbits;          // # of bits in current block
	DWORD    data[];           // DWORD because of optimizations in assembly
} INBLOCK;

HWND     hLoadDlg;
FARPROC  lpfnLoadDlgProc;
WORD     InSampleRate;
HWAVEIN  hWaveIn;
WAVEHDR  *inwavehdr;
HGLOBAL  hinwavehdr;
HGLOBAL  hinwavebuf;
HGLOBAL  inqueuehead;      // handle of start of input mem block queue
HGLOBAL  inqueuetail;      // handle of end of input mem block queue
HGLOBAL  incurbuf;         // handle of block currently fed to emulator
BOOL     loading;
BOOL     loadpossible;
int      inbufsused;
int      insendptr,ingetptr;
LONG     msinqueue;
PCMWAVEFORMAT inpwf;
int      statuscounter;

void initload()
{
	int i;

	hLoadDlg=NULL;
	lpfnLoadDlgProc=NULL;
	inqueuehead=inqueuetail=incurbuf=NULL;
	loading=FALSE;
	msinqueue=0;
	InSampleRate=22050;
	inpwf.wf.wFormatTag=WAVE_FORMAT_PCM;
	inpwf.wf.nChannels=1;
	inpwf.wf.nSamplesPerSec=InSampleRate;
	inpwf.wf.nAvgBytesPerSec=InSampleRate;
	inpwf.wf.nBlockAlign=1;
	inpwf.wBitsPerSample=8;
	hWaveIn=NULL;
	i=waveInOpen(&hWaveIn,WAVE_MAPPER,(LPWAVEFORMAT)&inpwf,hWndMain,NULL,CALLBACK_WINDOW);
	if (i) {
		InSampleRate=11025;
		inpwf.wf.nSamplesPerSec=InSampleRate;
		inpwf.wf.nAvgBytesPerSec=InSampleRate;
		i=waveInOpen(&hWaveIn,WAVE_MAPPER,(LPWAVEFORMAT)&inpwf,hWndMain,NULL,CALLBACK_WINDOW);
	}
	if (i) {
		loadpossible=FALSE;
		return;
	}
	loadpossible=TRUE;
	waveInClose(hWaveIn);
}

BOOL CALLBACK LoadDlgProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		statuscounter=0;
		break;
	case WM_CLOSE:
		hrd=RepaintData(hDlg);
		DestroyWindow(hDlg);
		return 0;
	case WM_SETFOCUS:
		SetActiveWindow(hWndMain);
		return TRUE;
	case WM_DESTROY:
		hLoadDlg=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnLoadDlgProc);
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}

void SendStatusString(char *str)
{
	char s[80];
	if (!hLoadDlg) return;
	sprintf(s,"(%d:) %s",++statuscounter,str);
	SetDlgItemText(hLoadDlg,LOAD_STATUS,s);
}

void InitLoading()
// may only be called if not 'InPlaying', and certainly not 'inning'
{
	resetinning();
	inning=loading;
	inbuflen=0;
	setsamplerate(1750000L/InSampleRate);
	iimode=!!iimode;
	getcurrenttime();
	intbaselo=soundtimelo;
	intbasehi=soundtimehi;
}

void LoadButton()
{
	int i;

	if (!loadpossible) {
		MessageBeep(-1);
		return;
	}
    message(SharewareMsg);
    return;
}

void FreeLoadMemChain()
{
	HGLOBAL hcurrent=inqueuehead,hnext;
	INBLOCK *current;
	while (hcurrent) {
		current=(INBLOCK*)GlobalLock(hcurrent);
		hnext=current->nextinqueue;
		GlobalUnlock(hcurrent);
		GlobalFree(hcurrent);
		hcurrent=hnext;
	}
	inqueuehead=NULL;
	inqueuetail=NULL;
}

void PollLoading()
{
	int i,j;
	BOOL gotblocks;

	if (!loading) return;
	gotblocks=FALSE;
	while (inwavehdr[ingetptr].dwFlags & WHDR_DONE) {
		INBLOCK *inblock;
		HGLOBAL hinblock;
		hinblock=GlobalAlloc(GMEM_MOVEABLE,
			sizeof(INBLOCK)+(inwavehdr[ingetptr].dwBytesRecorded)/4+17);
			// 17=1+16; 1 is to counter rounding down of dwBy.., see inout.asm for 16
		if (hinblock==NULL) {
			pollmem:
			SendStatusString("Out of memory");
		} else {
			register DWORD data;
			inblock=(INBLOCK*)GlobalLock(hinblock);
			if (inblock==NULL) goto pollmem;
			inblock->nextinqueue=NULL;
			inblock->numbits=2*inwavehdr[ingetptr].dwBytesRecorded;
			msinqueue += (inblock->numbits*500L)/InSampleRate;
			for (i=0;i<inwavehdr[ingetptr].dwBytesRecorded;i+=16) {
				data=0;
				for (j=0;j<16;j++) {
					data|=(DWORD)((DWORD)filter(inwavehdr[ingetptr].lpData[i+j])<<(2*j));
				}
				inblock->data[i/16]=data;
			}
			GlobalUnlock(hinblock);
			if (inqueuehead==NULL) {
				inqueuehead=hinblock;
				inqueuetail=hinblock;
			} else {
				inblock=(INBLOCK*)GlobalLock(inqueuetail);
				if (inblock==NULL) {
					GlobalFree(hinblock);
					goto pollmem;
				}
				inblock->nextinqueue=hinblock;
				GlobalUnlock(inqueuetail);
				inqueuetail=hinblock;
			}
		}
		waveInUnprepareHeader(hWaveIn,&inwavehdr[ingetptr],sizeof(WAVEHDR));
		inwavehdr[ingetptr].dwFlags = NULL;
		ingetptr=(ingetptr+1)%innumbuffers;
		gotblocks=TRUE;
	}
	if (gotblocks) {
		if (hLoadDlg) {
			char str[40];
			sprintf(str,"%d.%02d s",(int)(msinqueue/1000),(int)((msinqueue/10)%100));
			SetDlgItemText(hLoadDlg,LOAD_TIME,str);
		}
	}
	if ((inwavehdr[ingetptr].dwFlags==NULL)&&(msinqueue!=0)) {
		SendStatusString("Missed piece");
	}
	while (inwavehdr[insendptr].dwFlags == NULL) {
		waveInPrepareHeader(hWaveIn,&inwavehdr[insendptr],sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn,&inwavehdr[insendptr],sizeof(WAVEHDR));
		insendptr=(insendptr+1)%innumbuffers;
	}
}

BOOL LoadGetBlock()
// returns FALSE if no blocks ready
{
	INBLOCK *inb;
	if (incurbuf) {
		GlobalUnlock(incurbuf);
		GlobalFree(incurbuf);
		incurbuf=NULL;
		inbuflen=0;
	}
	if (!loading) return TRUE;       // continue
	if (!inning) return TRUE;        // strange, very strange
	if (!inqueuehead) return FALSE;  // wait
	incurbuf=inqueuehead;
	inb=(INBLOCK*)GlobalLock(incurbuf);
	inbufbase=inb->data;
	inbuflen=inb->numbits;
	inqueuehead=inb->nextinqueue;
	msinqueue-=(inb->numbits*500L)/InSampleRate;
	return TRUE;
}



BOOL AdjustSoundKnobs(void)
{
	int totvol;
	fevolume = iSoundKnobs[0];
	AYvolume = 16*((iSoundKnobs[1]*18)/16);		// 15->16, 7->7
	specdrumshift = 2 + 6*(15-iSoundKnobs[2])/15;
	totvol = fevolume + (45*AYvolume)/256 + (0xff >> specdrumshift);
	if (totvol>63) {
		iSoundKnobs[0] -= !!iSoundKnobs[0];
		iSoundKnobs[1] -= !!iSoundKnobs[1];
		iSoundKnobs[2] -= !!iSoundKnobs[2];
		return TRUE;
	}
	return FALSE;
}


BOOL CALLBACK SOProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	int i,pos;
	char name[128];
	char *fn;
	switch (wMess) {
	case WM_INITDIALOG:
		for (i=SO_EAR;i<=SO_VOL;i++) {
			SetScrollRange( GetDlgItem(hDlg,i),SB_CTL,0,15,FALSE);
			SetScrollPos( GetDlgItem(hDlg,i),SB_CTL,15-iSoundKnobs[i-SO_EAR],TRUE);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		hSODialog=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnSOProc);
		break;
//	case WM_COMMAND:
//		switch (wPar) {
//		SendMessage(hDlg,WM_INITDIALOG,0,0);
//		return TRUE;
	case WM_VSCROLL:
		i = GetDlgCtrlID((HWND)HIWORD(lPar))-SO_EAR;
		if ((i<0)||(i>6)) i=0;
		switch (wPar) {
		case SB_THUMBPOSITION:
			iSoundKnobs[i]=15-LOWORD(lPar);
			break;
		case SB_LINEUP:
		case SB_PAGEUP:
			iSoundKnobs[i] += (iSoundKnobs[i]<15);
			break;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			iSoundKnobs[i] -= (iSoundKnobs[i]>0);
			break;
		}
		pos = iSoundKnobs[i];
		while (AdjustSoundKnobs()) {
			// adjust knobs & reset current knob until volume is in limits
			iSoundKnobs[i] = pos;
		}
		for (i=0;i<7;i++) {
			SetScrollPos(GetDlgItem(hDlg,i+SO_EAR), SB_CTL, 15-iSoundKnobs[i],TRUE);
		}
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}




