#include <windows.h>
#include <mmsystem.h>
#include <dos.h>				; voor MK_FP
#include "spectrum.h"

#define maxchunktime 200

LONG emulacttime[spdavbuflen];
LONG emultime[spdavbuflen];
int spdavbufptr;
LONG TimeOfLastSample;
LONG TimeOfFirstSilentSample;
HGLOBAL hSpecMem;
BYTE FAR *SpecMem;
HGLOBAL hLogBuf=0;
BYTE *LogBuffer;
HGLOBAL houtbuf;
WORD unpack_outcount;

int InitEmulator(void)        // executed only once
{
	int i;
	init_memory();
	outbuflen=1000;      // 1000 outs per 20ms usually enough
	houtbuf=GlobalAlloc(GPTR,4*outbuflen);
	if (!houtbuf) {fatalerror(FatalMemAlloc);return(1);}
	outbufptr=(DWORD far*)GlobalLock(houtbuf);
	videopage7=0;    // do not read screen from page 7
//	page7locked=0;	  // in memory.c
	Reset();
	copper=0;         // is set before calling anyway, but bbsts
	state.num50hz=0;
	spdavbufptr=0;
	TimeOfLastSample=0;
	TimeOfFirstSilentSample=0;
	soundsilent=1;
	soundsilentq=1;
	for (i=0;i<spdavbuflen;i++) {
		emulacttime[i]=state.curtime;
		emultime[i]=state.num50hz;
	}
	for (i=0;i<5;i++) {
		z80header.kbdasc[i]='1'+i;
		z80header.kbdmap[i]=3+(0x100<<i);
	}
	TranslateJoystickSetting();
	setz80time(0);
	state.curtime = GetCurrentTime();
	ResetAY();
	return(0);
}

void Reset(void)
{
	reset=0xff;       // Signal to emulator core to execute reset
	anyinter=0xff;
	memstate.diskifpaged=FALSE;
	state.multifacepaged=FALSE;
	state.currahpaged=FALSE;
	pagerom();
	switch (hmode) {
	case hm_48k:
	case hm_48kif1:
	case hm_48kmgt:
		break;
	case hm_128k:
	case hm_128kif1:
	case hm_128kmgt:
		state.hstate &= 0x1f;
		out7ffd128(0);
		pageram0();
		break;
	case hm_samram:
		state.hstate=0;
		pageram0();
		break;
	}
}


void Nmi(void)
{
	nmi=0xff;
	anyinter=0xff;
	switchon_multiface();
	if (hmode==hm_samram) {
		state.hstate=0;
		pagerom();
		pageram();
	}
}

void InitLogging(void)
{
	if (hLogBuf) {
		GlobalUnlock(hLogBuf);
		GlobalFree(hLogBuf);
	}
	hLogBuf=GlobalAlloc(GPTR,5*LOGBUFLEN);
	if (!hLogBuf) {fatalerror(FatalMemAlloc);return;}
	LogBuffer=(BYTE*)GlobalLock(hLogBuf);
	logbufptr=LogBuffer;
	logbuflen=5*LOGBUFLEN;
	logging=1;
	installsettings();
	state.logging = 1;
}

void FlushLogBuf(void)
{
	int hFile;
	if (!hLogBuf) return;
	hFile=OpenReadWrite(gszLogFile);
	if (hFile==-1) {
		hFile=OpenCreate(gszLogFile);
		if (hFile==-1) {
			logging=0;
			state.logging=1;
			installsettings();
			if (hLogBuf) {
				GlobalUnlock(hLogBuf);
				GlobalFree(hLogBuf);
			}
			hLogBuf=0;
         notify(LogCantWrite);
		}
	}
	if (hFile!=-1) {
		_llseek(hFile,0,2);     // Move to end of file
      _lwrite(hFile,LogBuffer,5*LOGBUFLEN-logbuflen);    // total minus left
      _lclose(hFile);
      logbufptr=LogBuffer;
      logbuflen=5*LOGBUFLEN;
   }
}

void QuitLogging(void)
{
	FlushLogBuf();
   GlobalUnlock(hLogBuf);
   GlobalFree(hLogBuf);
	hLogBuf=0;
	logging=0;
	installsettings();
	state.logging = 0;
}

void InstallSettings(void)    // Reflects changes in state.* in emulator vars
{
	if (state.coppering) {
		copper=1;
		if (!hVidCopperBuf) {
			hVidCopperBuf=GlobalAlloc(GPTR,12288);
			if (hVidCopperBuf) {
				vidbufbase=(BYTE*)GlobalLock(hVidCopperBuf);
			} else {
				copper=0;
			}
		}
	} else {
		copper=0;
		GlobalUnlock(hVidCopperBuf);
		GlobalFree(hVidCopperBuf);
		hVidCopperBuf=NULL;
	}
	installsettings();      // initialise internal emulator variables & buffers
}




int pack(BYTE* target,BYTE *source,WORD size)
{
   register BYTE cur=*source;
	register int count=1;
   int outptr=0;
   int inptr=1;
   do {
		if ((inptr!=size)&&(source[inptr]==cur)&&(count<255)) {
			inptr++;
			count++;
      } else {
			if ((count>4)||((count>1)&&(cur==0xED))) {
            target[outptr++]=0xED;
            target[outptr++]=0xED;
				target[outptr++]=count;
            target[outptr++]=cur;
			} else {
				while (count--) target[outptr++]=cur;
				if (cur==0xED) {
					// this implies count was 1
					if (inptr!=size) {
						// do not include byte != 0xED following single 0xED in block
						target[outptr++] = source[inptr++];
					}
				}
			}
			if (inptr!=size) {
				cur=source[inptr++];
				count=1;
			} else count=0;
		}
	} while ((count)&&(outptr<size));
	if (outptr>=size)
		for (outptr=0;outptr<size;outptr++) target[outptr]=source[outptr];
	return (outptr);
}


int unpack(BYTE *inp,BYTE *outp,WORD sizeout, WORD sizein)
{
    unsigned int incount=0,outcount=0;
	 unsigned int i;
	 char j;
    if (sizein==sizeout) do {
       *(outp++)=*(inp++);
		 incount++;
       outcount++;
		} while (incount<sizein);
	 else do {
		 if ((inp[0]==0xED)&&(inp[1]==0xED)) {
            i=inp[2];
				j=inp[3];
            inp+=4;
				incount+=4;
				outcount+=i;
				for (;i!=0;i--) *(outp++)=j;
		 } else {
			*(outp++)=*(inp++);
			incount++;
			outcount++;
		 }
	 } while ((outcount<sizeout)&&(incount<sizein));
	 if ((outcount!=sizeout)||(sizein!=incount)) incount=0;
	 unpack_outcount=outcount;    // hmmm, but handy, see loaddat.c
	 return (incount);
}

#pragma option -a1
// byte align
int LoadSNAFile(HFILE handle)
{
	struct {
		BYTE i;
		WORD hla,dea,bca;
		BYTE fa,aa;
		WORD hl,de,bc,iy,ix;
		BYTE iff2,r,f,a;
		WORD sp;
		BYTE imode,border;
	} snaheader;
	long len;
	int i;

	Reset();
	len = _llseek(handle,0,2);
	if (len == 49179L)
		select_hmode(hm_48k);
	else if (len == 131103L)
		select_hmode(hm_128k);
	else if (len == 147487L)
		select_hmode(hm_128k);
	else return (LoadSNABadFile);
	if (len > 49179L) {
		_llseek(handle,49179L,0);
		_lread(handle,&z80header.pc,2);
		_lread(handle,&z80header.hstate,1);
		state.hstate &= 0x1f;
		out7ffd128(z80header.hstate);
	}
	_llseek(handle,0,0);
	pagerom();
	pageram0();
	if (_lread(handle,&snaheader,27)!=27) return(LoadSNABadFile);
	border = min(snaheader.border,7);
	z80header.rrbit7=snaheader.r;
	rreg=snaheader.r+1;
	z80header.i=snaheader.i;
	z80header.flg=snaheader.imode;
	z80header.fa=((snaheader.f)<<8) + snaheader.a;
	z80header.faa=((snaheader.fa)<<8) + snaheader.aa;
	z80header.iff=!!(snaheader.iff2&4);
	z80header.iff2=!!(snaheader.iff2&4);
	z80header.sp=snaheader.sp;
	z80header.hl=snaheader.hl;
	z80header.hla=snaheader.hla;
	z80header.de=snaheader.de;
	z80header.dea=snaheader.dea;
	z80header.bc=snaheader.bc;
	z80header.bca=snaheader.bca;
	z80header.ix=snaheader.ix;
	z80header.iy=snaheader.iy;
//   setz80time(0L);            // arbitrary, in fact		// not yet implemented
	if (_lread(handle,&SpecMem[16384],49152L)!=(WORD)49152L)
		return(LoadSNABadFile);
	if (len == 49179L) {
		z80header.pc=(SpecMem[z80header.sp]+(SpecMem[z80header.sp+1]<<8));
		if (z80header.sp+1<=16385) {
			SpecMem[z80header.sp]=0;
			SpecMem[z80header.sp+1]=0;
		}
		z80header.sp+=2;
	} else {
		_llseek(handle,4,1);
		for (i=0;i<7;i++) {
			if ( (i != 2) && (i != 5) && (i != (z80header.hstate&7)) ) {
				mempage(3,i+3);
				if (_lread(handle,&SpecMem[49152L],16384) != 16384)
					return(LoadSNABadFile);
			}
		}
	}
	memstate.diskifpaged=FALSE;
	state.multifacepaged=FALSE;
	state.currahpaged=FALSE;
	pageram();
	reset=0;    // reset reset flag
	anyinter=0;
	z80header.loram=0;
	z80header.hiram=0;
	return(0);
}
#pragma option -a.


int LoadZ80File(HFILE handle)
{
	unsigned int i;
	HGLOBAL hTemp;
	unsigned char* Temp;
	struct {
		WORD length;
		BYTE page;
	} block;
	Reset();    // To make sure Spectrum resets when s'thing's wrong
	if (_lread(handle,&z80header,30)!=30) return(LoadZ80Error);
	border = rr_borclr(z80header.rrbit7);
	if (flg_is2(z80header.flg))
		issue2=0;
	else
		issue2=1;
	ToggleMenu(hWndMain,CM_ISSUE2,&issue2);
	currentjoystick=5*(((BYTE)z80header.flg)>>6);
	SetJoystick(currentjoystick);
	z80header.flg = flg_im(z80header.flg);
	rreg = z80header.r+1;
	// +1 because here R val is used before increment, in Z80 it is used after increment
	if (z80header.rrbit7==0xFF) z80header.rrbit7=1;
	if (z80header.pc!=0) {  /* prior to v2.0 */
		 z80header.hmode=hm_48k;
		 select_hmode(z80header.hmode);
		 pageram0();
		 if (!rr_compressed(z80header.rrbit7)) {/* old.. */
			if (_lread(handle,&SpecMem[16384],49152L)!=(WORD)49152L)
				return(LoadZ80BadFile);
		 } else {   /* not-so-old */
			HGLOBAL hMem;
			BYTE *tempbuf;
			hMem=GlobalAlloc(GPTR,(UINT)50000L);
			if (!hMem) return(LoadZ80NoMem);
			tempbuf=(BYTE*)GlobalLock(hMem);
			i=_lread(handle,tempbuf,(UINT)49152L);
			i=(unpack(tempbuf,(BYTE*)&SpecMem[16384],(UINT)49152L,i-4)!=i-4);
			GlobalUnlock(hMem);
			GlobalFree(hMem);
			if (i) return(LoadZ80BadFile);
		 }
	} else {
		DWORD time;
		if (_lread(handle,&z80header.length,2)!=2) return(LoadZ80Error);
		if ((z80header.length <= 54) && ((z80header.length!=23)&&(z80header.length!=54)))
			return(LoadZ80UnkVer);
		i = z80header.length;
		if (z80header.length > 54) {
			i = 54;
//			notify(MsgUnknownLength);
		}
		if (_lread(handle,&z80header.pc2,i) != i ) return(LoadZ80Error);
		_llseek(handle,z80header.length - i,1);
		// Now convert data in header to internal values
		if (z80header.hmode >= 7) {
			return(ErrorHardware);
		}
		z80header.pc=z80header.pc2;
		if ((z80header.length==23)&&(z80header.hmode>=3)) z80header.hmode++;
		if (z80header.length>=54) TranslateJoystickSetting();
		time = ((z80header.tstatehi+1)&3)*17472L + (17472L-z80header.tstatelo);
//		setz80time(time);		// not yet.  difficult to implement.

		state.currahemulated=!!(z80header.flg2 & flg2_uspeech);
		state.ayemu48k=!!(z80header.flg2 & flg2_ayemu48k);
		memstate.diskifpaged=FALSE;
		//	state.multifacepaged=z80header.multipaged;
		state.multifacepaged=FALSE;
		state.currahpaged=FALSE;

		if (z80header.hmode>hm_128kmgt) return(LoadZ80UnsupMode);
		select_hmode(z80header.hmode);

		hTemp=GlobalAlloc(GPTR,16384);
		if (!hTemp) return(LoadZ80NoMem);
		Temp=(BYTE*)GlobalLock(hTemp);

		if (_lread(handle,&block,3)!=3) {
			errz80:
			GlobalUnlock(hTemp);
			GlobalFree(hTemp);
			return(LoadZ80BadFile);
		}
		mempage(1,0);
		mempage(2,0);
		mempage(3,0);     // select rom in all pages
		do {
			if (_lread(handle,Temp,block.length)!=block.length) goto errz80;
			if ((block.page<2)||(block.page>10)) {
				reset_errz80:
				pageram0();
				pagerom();
				goto errz80;
			}
			mempage(1,block.page);
			if (unpack(Temp,&SpecMem[16384],16384,block.length)!=block.length)
				goto reset_errz80;
			i=_lread(handle,&block,3);
		} while ((i==3)&&(block.length));
		GlobalUnlock(hTemp);
		GlobalFree(hTemp);
	}
	mempage(1,0);
	z80header.rrbit7<<=7;
	state.hstate &= 0x1f;
	out7ffd128(z80header.hstate);
	fffdstate=z80header.fffd;
	for (i=0;i<16;i++) soundregs[i]=z80header.sregs[i];
	if ((hmode==hm_48kif1)||(hmode==hm_128kif1)||(hmode==hm_samram))
		memstate.diskifpaged=z80header.if1paged;
	if ((hmode==hm_48kmgt)||(hmode==hm_128kmgt))
		memstate.diskifpaged=z80header.mgtpaged;
	ResetAY();
	InitAY();
	z80header.loram=0;
	z80header.hiram=0;
	pageram0();
	pagerom();
	reset=0;    // reset reset flag
	anyinter=0;
//	if (hHardwareDialog) SendMessage(hHardwareDialog,WM_INITDIALOG,0,0);
	return(0);
}



int SaveZ80File(HFILE hFile)
{
	unsigned int i;
	HGLOBAL hTemp;
	unsigned char* Temp;
	BOOL savepage[11];
	long time;
	struct {
		 WORD length;
		 BYTE page;
	} block;
	z80header.pc2=z80header.pc;
	z80header.pc=0;
	z80header.length=54;
	z80header.hmode=hmode;
	z80header.flg2=3;          // LDIR emulation on, R emulation on
	if (state.currahemulated) z80header.flg2 |= flg2_uspeech;
	if (state.ayemu48k) z80header.flg2 |= flg2_ayemu48k;
	time=getcurrenttime();     // This also updates z80header.rr
	z80header.tstatelo=(tframe/4) - time % (tframe/4);
	z80header.tstatehi=(3+ (time / (tframe/4)))%4;
	z80header.if1paged = memstate.diskifpaged;
	z80header.hstate = state.hstate;
	z80header.specflg=0;
	z80header.loram=0xFF*(!z80header.loram);
	z80header.hiram=0xFF*(!z80header.hiram);
	z80header.rrbit7=(z80header.rrbit7&128?0x81:0)+(border<<1);
	z80header.r--;
	z80header.flg=z80header.flg | (4*!!issue2) | (64*currentjoystick) |32;
	z80header.fffd = fffdstate;
	for (i=0;i<16;i++) z80header.sregs[i]=soundregs[i];
	i=_lwrite(hFile,&z80header,32+54);
	z80header.flg=flg_im(z80header.flg);
	z80header.pc=z80header.pc2;
	z80header.r++;
	z80header.loram = !z80header.loram;
	z80header.hiram = !z80header.hiram;
	if (i!=(32+54)) goto wzf_error;
	for (i=3;i<11;i++) savepage[i]=TRUE;
	// 128k modes: save page 3-10
	// samram: save page 4-8
	// 48k modes: save pages 4,5 and 8
	if (hmode<hm_128k) {				// samram or 48K modes
		savepage[3]=savepage[9]=savepage[10]=FALSE;
		if (hmode != hm_samram)
			savepage[6]=savepage[7]=FALSE;
	}
	hTemp=GlobalAlloc(GMEM_MOVEABLE,16384+16);
	if (!hTemp) goto wzf_error;
	Temp=(unsigned char*)GlobalLock(hTemp);
	mempage(1,0);
	mempage(2,0);
	mempage(3,0);
	for (i=3;i<11;i++) {
		if (savepage[i]) {
			block.page=i;
			mempage(1,i);
			block.length=pack(Temp,&SpecMem[16384],16384);
			if (_lwrite(hFile,&block,3)!=3) goto wzf_error_mem;
			if (_lwrite(hFile,Temp,block.length)!=block.length) {
				wzf_error_mem:
				mempage(1,0);
				pageram0();
				pagerom();
				wzf_error:
				_lclose(hFile);
				return (SaveZ80Error);
			}
		}
	}
	mempage(1,0);
	pageram0();
	pagerom();
	GlobalUnlock(hTemp);
	GlobalFree(hTemp);
	if (_lclose(hFile)) goto wzf_error;
	return (0);
}


BOOL CALLBACK DiHaltDialProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	switch (wMess) {
	case WM_INITDIALOG:
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_COMMAND:
		switch (wPar) {
		case DIH_CONT:
		case DIH_EICONT:
		case DIH_PAUSE:
			EndDialog(hDlg,(int)wPar);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		EndDialog(hDlg,-1);
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


void ClearAvBuf(void)
{
	int i=spdavbufptr-1,j;
	if (i<0) i+=spdavbuflen;
	for (j=0;j<spdavbuflen;j++) {
		emultime[j]=emultime[i];
		emulacttime[j]=emulacttime[i];
	}
}



void executepiece(void)
// if howlong==0, call was from main loop, and code pays heed to state.paused.
// if howlong!=0, call was from debugger, and code emulates approx howlong T's, or until
//  debug event.
{
	WORD num20ms;
	int showscr;
	char RunFast;
	char tijdel[50];
	static int memshrink=0;
	static char CoreBusy=FALSE;
	static char chunkcount=0;
	static long lasttime=0;
	LONG thisblock,curt=GetCurrentTime();
	FARPROC lpfnDialProc;
//   MSG keymsg;
	int i;
	char maxblock=FALSE;
	int keytailptr = depresstail;
	int totnum20ms;

	RunFast = state.fastest || (TapeMaxLoadSpeed && PlayingSomething());
	if (state.paused||CoreBusy||(BlocksInUse>=numbufs-bufmargin)) {
		state.curtime=curt;
		return;
	}
	if (QNTime && (curt-QNTime>4000)) SendMessage(hQNDlg,WM_CLOSE,12345,0L);
	chunkcount++;
	if (curt-lasttime>150) {
		// means there's a WM_TIMER pending, and we're not yielding.  Shouldn't
		// happen for too long.  Be relaxed about it when emulating as fast as
		// possible, or when saving.
		if (curt-lasttime>((RunFast||(feearmicmask==8))?1500L:350L)) {
			lasttime=state.curtime=curt;
			return;
		}
	} else {
		lasttime=curt;
	}
	if ((!soundsilent) && sound &&
		 (!RunFast) && (!state.truepitch) && (hWaveOut) && (!Played5Minutes)) {
		// Let time be dictated by sample player
		MMTIME time;
		WORD bytesinqueue;
		int goal;
		if (feearmicmask==8)
			goal=numbufs-bufmargin;		// if saving, then almost full buffer is our goal
		else
			goal=2;
		time.wType=TIME_BYTES;
		waveOutGetPosition(hWaveOut,&time,sizeof(time));
		bytesinqueue=sent-time.u.cb;
		if (bytesinqueue>=(goal+1)*SamplesPerBlock) {    // NOTE: not BytesPerBlock,
			state.curtime=curt;                    // we're computing words, sending bytes
			return;
		}
		if (bytesinqueue>=goal*SamplesPerBlock) {
			state.curtime += 1000L/state.speed;		// relax, give 10 ms
		}
		if (bytesinqueue<(goal-1)*SamplesPerBlock) {
			state.curtime -= 2000L/state.speed;    // steal 20 ms
		}
	}
	CoreBusy=TRUE;    // prevent double entry; would happen with dlg boxes
	if ((RunFast)||(msinqueue>2000L)) {
		// if >2s loaded, or fastest, or saving, then emulate big chunk
		num20ms=state.maxnoscreenrefresh;
		maxblock=TRUE;
	} else {
		thisblock = curt-state.curtime;
		// number of msecs to emulate
		if (thisblock<0) num20ms=0; else {
			if (thisblock>1000) thisblock=1000;
			num20ms = (WORD)(state.speed*(thisblock+(1000L/state.speed))/2000L);
			if (num20ms>state.maxnoscreenrefresh) {
				num20ms=state.maxnoscreenrefresh;
				maxblock=TRUE;
				state.curtime = curt;      // move on to <now>; we were slow
			} else
				state.curtime += (num20ms*2000L)/state.speed;
		}
	}
	emulacttime[spdavbufptr]=curt;
	emultime[spdavbufptr]=state.num50hz;
	i=spdavbufptr+1;
	if (i>=spdavbuflen) i=0;
	if ((chunkcount&7)==0) {
		int newspd=((emultime[spdavbufptr]-emultime[i])*2000L)/
			max(emulacttime[spdavbufptr]-emulacttime[i],10);
		if (newspd!=state.actspeed) {
			state.actspeed=newspd;
			if (hSpeedDialog)
				PostMessage(hSpeedDialog,WM_USER+1,newspd,0L);
		}
	}
	spdavbufptr=i;
	showscr=min(num20ms,2);
	if ((state.vidstate==0)||RunFast||(feearmicmask==8))
		if (chunkcount&1) showscr=0;     // chuncky
	if (state.vidstate==2) showscr=-1;     // smooth
	if (gifRecording) showscr=-1;			// recording a movie, then show all frames
	novideograb=!((showscr==-1)||(num20ms==showscr));
	totnum20ms=num20ms;
	FlushKeyBuf(keytailptr,num20ms,0);     // Flush first key(s)
	while (num20ms) {
		switch(i=emulate(SpecMem)) {
		case msg_trip:			  // Emulator tripped over breakpoint or other trap
			if (hDebugDialog) {
				PostMessage(hDebugDialog,WM_TRAP,i,0);
				SetPauseState(1);
				num20ms=0;
				maxblock=FALSE;
			}
			break;
		case msg_dihalt:       // Emulator encountered HALT while IFF=0
			if (hDebugDialog) {
				PostMessage(hDebugDialog,WM_TRAP,i,0);
				SetPauseState(1);
				num20ms=0;
				maxblock=FALSE;
			} else {
				lpfnDialProc=MyMakeProcInstance(DiHaltDialProc,ghInstance);
				i=DialogBox(ghInstance,"DIHALTDIALOG",hWndMain,lpfnDialProc);
				MyFreeProcInstance(lpfnDialProc);
				if (i==DIH_EICONT) {
					z80header.iff=0xFF;
					z80header.iff2=0xFF;
					z80header.pc++;        // step over HALT instruction
				}
				if (i==DIH_PAUSE) {
					SetPauseState(1);
					num20ms=0;
					maxblock=FALSE;
				}
				if (i==DIH_CONT) {
					z80header.pc++;
				}
			}
			break;
		case msg_logbuffull: // log buffer is filled up
			FlushLogBuf();    // this writes buffer and resets pointers
			break;
		case msg_twrap:      // global counter wrapped
//			currah_50hz();
//         while (PeekMessage(&keymsg,
//                         hWndMain,WM_KEYFIRST,WM_KEYLAST,
//                         PM_NOYIELD|PM_REMOVE))
//            TranslateKbd(keymsg.wParam,keymsg.lParam,keymsg.message);
//         while (PeekMessage(&keymsg,
//                            hWndMain,WM_KEYFIRST,WM_KEYLAST,
//                            PM_NOYIELD|PM_REMOVE))
//            DispatchMessage(&keymsg);
			tglobal=tframe;   // another 20 ms
			if (hDebugDialog) {
				SendMessage(hDebugDialog,WM_TWRAP,0,0);
				if (state.paused) {
					num20ms=0;
					maxblock=FALSE;
				}
			}
			break;
		case msg_50hz:
		case msg_sampleblk50hz:
//				{
//					static int count=0;
//					count=(count+1)%50;
//					if (!count) {
//						sprintf(tijdel,"%lu edges\n%lu outs",DBGnumloops/50,DBGnumouts/50);
//						WRITEDEBUGMSG(tijdel,1);
//						DBGnumloops=DBGnumouts=0;
//					}
//				}
			num20ms--;
			state.num50hz++;
			FlushKeyBuf(keytailptr,totnum20ms,totnum20ms-num20ms);
			HandleWarajevoMode();
			if (miractive && (miridle>2)) flushmirblock();
			currah_50hz();
			novideograb=!((showscr==-1)||(num20ms==showscr));
			if ((GetCurrentTime()-curt)>maxchunktime) {
				state.maxnoscreenrefresh=
					max(7,min(2*(totnum20ms-num20ms),state.maxnoscreenrefresh-1));
				num20ms=0;
			}
			if (i==msg_50hz) break;
		case msg_sampleblk:  // sample block is ready to be played
			if (SpeedChanged) {
				SpeedChanged=FALSE;
				if (!state.truepitch) {
					AllocBuffers();
					soundnextblock = (soundbufptr/BytesPerBlock);
					break;
				}
			}
//			{
//				static int numsoundblks=0;
//				numsoundblks++;
//				sprintf (tijdel,"Num blks %u.  s.s.q %u.  s.n.b. %u.  InUse %u",
//					numsoundblks, soundsilentq, soundnextblock, BlocksInUse);
//				WRITEDEBUGMSG(tijdel,3);
//			}
			if (soundsilentq) {
				if (!TimeOfFirstSilentSample)
					TimeOfFirstSilentSample=soundlastthi;
			} else {
				TimeOfFirstSilentSample=0;
			}
			// Do not immediately play 1st block, as it is likely
			//  that 2nd block will not be computed in time
			if ( (BlocksInUse==0)&&
				  (((soundnextblock+1)%numbufs)==(soundbufptr/BytesPerBlock))&&
				  ((!soundsilentq)||
					(soundlastthi-TimeOfFirstSilentSample<(ShutUpDelay/20)))&&
				  (!RunFast)&&
				  (!state.truepitch))
						break;
			// Now take blocks out of queue, compute time-of-first-sample,
			//  process blocks (convert word samples to bytes, add currah
			//  speech), and play and/or record them.
			i=soundnextblock;
			while (i != (soundbufptr/BytesPerBlock)) {
				buftimehi[i] = soundlastthi;
				buftimelo[i] = soundlasttlo -
					(soundbufptr/2) * (long)TStatesPerSample +
					i*TStatesPerBlock;
				if (soundbufptr < i*BytesPerBlock)
					buftimelo[i] -= numbufs*TStatesPerBlock;
				PostProcess(i);
				i=(i+1)%numbufs;
			}
			// save blocks:
			if (state.record) {
				if (TimeOfLastSample) {
					WriteSilenceBlock(soundlastthi-TimeOfLastSample);
					TimeOfLastSample=0;
				}
				i=soundnextblock;
				while (i != (soundbufptr/BytesPerBlock)) {
					SaveBlock(i);
					i=(i+1)%numbufs;
				}
			}
			// Now see if we can stop playing sound samples:
			if ((soundsilentq)&&
				 (soundlastthi-TimeOfFirstSilentSample>=(ShutUpDelay/20))) {
					soundsilent=TRUE;
					i=(soundbufptr/BytesPerBlock+numbufs-1)%numbufs;
					SilenceBlock(i);     // Go to 0 level (smooth ramp corresp. to 10 Hz)
					TimeOfLastSample=soundlastthi;
					TimeOfFirstSilentSample=0;
			}
			// Finally, really play blocks.  In fastest and truepitch mode, discard
			// blocks that cannot be played.
			if (RunFast || state.truepitch) {
				int firstnotplayed=-1;
				while (soundnextblock != (soundbufptr/BytesPerBlock)) {
					if (BlocksInUse<numbufs-bufmargin) PlayBlock(soundnextblock);
					else if (firstnotplayed==-1) firstnotplayed=soundnextblock;
					soundnextblock=(soundnextblock+1)%numbufs;
				}
				if (firstnotplayed!=-1) {
					// Copy first 32 samples, necess. when recording
					DWORD sbpold,sbpnew;
					sbpold = (soundbufptr / BytesPerBlock)*BytesPerBlock;
					sbpnew = firstnotplayed*BytesPerBlock;
					for (i=0;i<32;i++)
						wavebuf[0].lpData[sbpnew+i] =
							wavebuf[0].lpData[sbpold+i];
					soundcurblk=firstnotplayed;
					soundnextblock=firstnotplayed;
					soundbufptr=sbpnew+(soundbufptr % BytesPerBlock);
				}
			} else {
				while (soundnextblock != (soundbufptr/BytesPerBlock)) {
					if (BlocksInUse>=numbufs-bufmargin) {
						num20ms=0;   // stop immediately
						maxblock=FALSE;
					} else {
						PlayBlock(soundnextblock);
					}
					soundnextblock=(soundnextblock+1)%numbufs;
				}
			}
			// Make sure more sound blocks are computed when saving, and sound is
			// being produced:
			if ((feearmicmask==8)&&(!soundsilent)&&(BlocksInUse<numbufs/2)) {
				num20ms=max(num20ms,6);
			}
			break;
		case msg_halfscreen: // it is time to show the screen
			// Make sure page 7 is locked; should be superfluous
			if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked||useVz80d))
				UpdateVideo(page7fp);
			else
				UpdateVideo(SpecMem+16384);
			ShowScreen(TRUE);
			if (gifRecording) GifFrame();
			goto checkshrinkoutbuf;
		case msg_grabok:
			UpdateVideoCopper(vidbufbase);
			ShowScreen(TRUE);
			if (gifRecording) GifFrame();
		checkshrinkoutbuf:
			outbufoffset=0;
			if (memshrink) memshrink--; else {
				if (outbuflen>1000) {
					GlobalUnlock(houtbuf);
					outbuflen-=500;
					houtbuf=GlobalReAlloc(houtbuf,outbuflen*4,GMEM_MOVEABLE);
					if (!houtbuf) {fatalerror(FatalMemAlloc);break;}
					outbufptr=(DWORD far*)GlobalLock(houtbuf);
					memshrink=MemShrinkTime;
				}
			}
			break;
		case msg_outbufextend:
			GlobalUnlock(houtbuf);
			outbuflen+=500;
			houtbuf=GlobalReAlloc(houtbuf,outbuflen*4,GMEM_MOVEABLE);
			if (!houtbuf) {fatalerror(FatalMemAlloc);break;}
			outbufptr=(DWORD far*)GlobalLock(houtbuf);
			memshrink=MemShrinkTime;
			break;
		case msg_inbufempty:
			if (InPlaying) {				// sample file
				readinbuffer(-1);
				break;
			}
			if (TzxInPlaying) {			// TZX file
				readtzxfile(-1);
				break;
			}
			if (!LoadGetBlock()) num20ms=0;  // (Loading:) No sound ready -> wait for it
			break;
		case msg_emulin:
			// here debug trap code must be inserted
			if ((inaddress&0xff)==31) {         // Kempston joystick port
				inresult=~KeyMap[8];             // Never float bus (JSW)
				isresult=1;
			} else if ((inaddress&4)==0) {      // ZX Printer
				if (zxprinter) {
					inresult=(zxpcount<0?zxpcount++,1:0x81);
					isresult=1;
				}
			} else if ((inaddress&0xff)==0xbf) {   // switch on multiface
				inresult=0x7f | (state.hstate<<4);  // bit 3 of z80header.hstate
				isresult=1;
				switchon_multiface();
			} else if ((inaddress&0xff)==0x3f) {
				switchoff_multiface();              // & float bus
			} else if (inaddress==0xfffd) {
				if (hmode>=hm_128k) {
					 inresult=infffd128();
					 isresult=1;
				}
			} else if ((inaddress & 0x18) != 0x18) {
				if ((hmode == hm_48kif1) ||
					 (hmode == hm_128kif1) ||
					 (hmode == hm_samram)) {
					inresult = in_if1(inaddress);
					isresult = 1;
				}
			}
			break;
		case msg_emulout:
			// here debug trap code must be inserted
			if ((outaddress&4)==0) {            // ZX Printer
				if (outvalue&4) {                // Motor off?
					if (zxpmotor) {
						zxpmotor=0;
						if (zxpcount>0) {
							while (zxpcount<256) zxpline[zxpcount++]=0;
							if (zxprinter) printzxline(zxpline);
						}
						endline();
						zxpcount=-4;
					}
				} else {                         // Motor on?
					zxpmotor=1;
					if (zxpcount>=0)
						zxpline[zxpcount++]=outvalue&0x80;
					if (zxpcount==256) {
						zxpcount=-4;
						if (zxprinter) printzxline(zxpline);
					}
				}
			} else if ((outaddress&0xFF)==31) {
				if (hmode==hm_samram)
					out31samram(outvalue);
			} else if ((outaddress|0x3ffd)==0x7ffd) {
					 out7ffd128(outvalue);
			} else if ((outaddress|0x3ffd)==0xbffd) {
					 AYout(outvalue);					// See INOUT.ASM
			} else if ((outaddress|0x3ffd)==0xfffd) {
					 outfffd128(outvalue);
			} else if ((outaddress & 0x18) != 0x18) {
					 out_if1(outaddress,outvalue);
			} else if ((outaddress & 0xFF) == 0xdf) {
					 if (state.specdrumemu)
						SpecDRUMout(outvalue);
			}
			break;
		case msg_edfb:
			if (z80header.pc<0x4000)
				ldobhla();
			else
				edfb();
			break;
		case msg_loadtrap:         LoadTrap();          break;
		case msg_savetrap:         SaveTrap();          break;
		case msg_getwarmodedata:   GetWarModeData();    break;
		case msg_mirbuffull:       flushmirblock();     break;
		case msg_edfe:             ldhla();             break;
		case msg_edfa:             ldobahl();           break;
		case msg_edf9:             edf9();              break;
		case msg_rst08:            rst08();             break;
		case msg_ret0700:          ret0700();           break;
		case msg_memspecial:			memspecial();			break;
		case msg_rst38:   			rst38();					break;
		}
	}
	CoreBusy=FALSE;
	if (hPlaySample) PostMessage(hPlaySample,PSF_SHOWPOSITION,0,0);
	if (maxblock) {
		// Adjust max # of 20ms cycles emulated per 18 Hz irpt to account
		// for very slow and very fast computers.
		curt = GetCurrentTime()-curt;
		if ((curt<50)&&(state.maxnoscreenrefresh<100)) state.maxnoscreenrefresh++;
		if ((curt>maxchunktime)&&(state.maxnoscreenrefresh>7)) state.maxnoscreenrefresh--;
	}
}



