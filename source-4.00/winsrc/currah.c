#include <windows.h>
#include <stdio.h>
#include "spectrum.h"

#define cursampbuflen 100	// # of allophones that can be stored
#define numsampbufs	 7		// 1 more than # of allophone sample buffers needed
#define maxallosamplen 8192	// max length of single allophone sample

typedef struct {
	char	allobuf;
	int	simple;
	DWORD	offset;
	DWORD	relrestart;
	DWORD pause;
	DWORD	length;
	DWORD playlength;
} allodata;

LONG	cur_currlength;		// length of current allophone being played in T's (spec i/o level)
char	cur_cursample;			// last allophone number
int	cur_spointer;			// pointer to start of allophone buffer
int	cur_s2pointer;			// pointer to end of allophone buffer
LONG	cur_startlo[cursampbuflen];	// starting time
DWORD	cur_starthi[cursampbuflen];	// starting time (hi)
char	cur_sample[cursampbuflen];		// allophone number (0-127, bit 6 = higher pitch)

HGLOBAL	allosampbuf[numsampbufs];
allodata allos[128];


// tedoen:
//  samples moeten gespeeld!
//  lengtes uitgezocht

void currah_init(void)
// sound must be initialised before calling currah_init, because sample rate
// must be known.
{
		FILE *fin;
		char nmbufs=0;
		char dummybuf[100];
		long curbuflen=0;
		long datastart;
		long blocklen;
		HGLOBAL hsampbuf;
		BYTE *sampbuf,*sampbuf2;
		int i;
		WORD j;

		if ((SampleRate!=11025)&&(SampleRate!=22050)) {
			fatalerror(fatalcurrahsamplerate);
			return;
		}
		fin = fopen(gszCurrahFile,"rb");
		if (fin==NULL)
			fatalerror(fatalcurrahfile);
		allosampbuf[0]=NULL;
		fscanf(fin,"%[^\032]",dummybuf);
		fscanf(fin,"%c",dummybuf);
		for (i=0;i<128;i++) {
			if ((i & 0x3F) < 5) {
				switch (i & 0x3F) {
				case 0:	allos[i].pause=221;  break;
				case 1:	allos[i].pause=441;	break;
				case 2:	allos[i].pause=662;	break;
				case 3:	allos[i].pause=1103;	break;
				case 4:	allos[i].pause=2205;	break;
				}
				allos[i].playlength = allos[i].pause;
				allos[i].length = 0;
				allos[i].relrestart = 0;
				allos[i].offset = 0;
			} else {
				if (fscanf(fin,"%D %D %D %D %D %d\n",
							&(allos[i].offset),
							&(allos[i].relrestart),
							&(allos[i].length),
							&(allos[i].pause),
							&(allos[i].playlength),
							&(allos[i].simple)
						  )!=6) {
					fclose(fin);
					fatalerror(fatalcurrahfile);
					return;
				}
			}
		}
		datastart = ftell(fin);
		hsampbuf = GlobalAlloc(GMEM_MOVEABLE,maxallosamplen);
		if (hsampbuf==NULL) {
			fclose(fin);
			fatalerror(fatalmemory);
			return;
		}
		sampbuf = (BYTE*)GlobalLock(hsampbuf);

		for (i=5;i<128;i++) {
			if (i==64) i=64+5;
			fseek(fin,datastart+allos[i].offset,SEEK_SET);
			if (fread(sampbuf,1,(int)allos[i].length,fin) != (int)allos[i].length) {
				fclose(fin);
				fatalerror(fatalcurrahfile);
				return;
			}
			if (SampleRate==11025) {
				allos[i].relrestart = (allos[i].length>>1) -
					((allos[i].length-allos[i].relrestart)>>1);
				allos[i].length >>= 1;
				allos[i].playlength >>= 1;
				allos[i].pause >>= 1;
			}
			blocklen = allos[i].length;
			if ((curbuflen+blocklen >= 0x10000L)||(nmbufs==0)) {
				nmbufs++;
				allosampbuf[nmbufs++] = GlobalAlloc(GMEM_MOVEABLE,blocklen);
				allosampbuf[nmbufs] = NULL;
				if (allosampbuf[nmbufs-1]==NULL) {
					fclose(fin);
					fatalerror(fatalmemory);
					return;
				}
				curbuflen = 0;
			} else {
				if (GlobalReAlloc(allosampbuf[nmbufs-1],curbuflen+blocklen,0)==NULL) {
					fclose(fin);
					fatalerror(fatalmemory);
				};
			}
			allos[i].allobuf = nmbufs-1;
			sampbuf2 = (BYTE*)GlobalLock(allosampbuf[nmbufs-1]);
			if (SampleRate==11025) {
				for (j=0;j<blocklen;j+=2)
					sampbuf2[(int)curbuflen+(j/2)] = (sampbuf[j]-0x77);
			} else {
				for (j=0;j<blocklen;j++)
					sampbuf2[(int)curbuflen+j] = (sampbuf[j]-0x77);
			}
			allos[i].offset = curbuflen;
			curbuflen += blocklen;
			GlobalUnlock(allosampbuf[nmbufs-1]);
		}
		GlobalUnlock(hsampbuf);
		GlobalFree(hsampbuf);
		fclose(fin);
}


void currah_end(void)
{
	int i=0;
	while (allosampbuf[i]) {
		GlobalUnlock(allosampbuf[i]);
		GlobalFree(allosampbuf[i]);
		i++;
	}
}

void currah_speak(WORD *sample, DWORD stimehi, long stimelo, WORD len)
// Currah speech is added to buffer 'sample' of length 'len', starting at time 'stime*'
{
	long timebase;
	long sampstart,sampend,tcurr,tnext,tnew;
	long samppos,allopos,skiplen,allolen,alloplay;
	char cur_next;
	char allo;
	char done_here;
	char cur_alloplay;
	BYTE *allosample;
	int i;
	int volume;

	// no currah, no go
	if (!state.currahemulated) return;
	// no allos in buffer, then return
	if (cur_spointer == cur_s2pointer) return;
	// compute time hi base, for local use
	timebase = (long)min(stimehi,stimehi+(stimelo/(long)tframe)-1);
	timebase = (long)min(timebase,cur_starthi[cur_spointer]);
	// compute sample start and end
	sampstart = (stimehi-timebase)*tframe + stimelo;
	sampend = sampstart + TStatesPerSample*(long)len;
	do {
		done_here = FALSE;
		// compute start time of next allo
		cur_next = (cur_spointer+1)%cursampbuflen;
		if (cur_next == cur_s2pointer)
			tnext = sampend;
		else
			tnext = (cur_starthi[cur_next]-timebase)*tframe + cur_startlo[cur_next];
		allo = cur_sample[cur_spointer];						// allophone number
		cur_alloplay = ((allo&0x3F)>4);
		// if current sample is playing, put it in sample buffer up to tnext
		if (cur_alloplay) {
			tcurr = (cur_starthi[cur_spointer]-timebase)*tframe + cur_startlo[cur_spointer];
			allosample = (BYTE*)GlobalLock(allosampbuf[allos[allo].allobuf]);
			samppos = (tcurr - sampstart)/(long)TStatesPerSample;	// position into sample buffer
			allolen = allos[allo].length;							// length of allophone sample
			alloplay = allos[allo].playlength;					// minimum length (>allolen)
			allopos = -allos[allo].pause;							// position into allophone sample (normally 0)
			allolen -= allopos;										// add pause to 'sample' length
			if (samppos >= len)
				done_here = TRUE;
			while ((samppos < len) &&
					 (allolen) &&
					 ((alloplay>0) || (samppos < (tnext-sampstart)/(long)TStatesPerSample))) {
				// first skip part before start of sample
				while (samppos<0) {
					skiplen = min(allolen,-samppos);
					allolen -= skiplen;
					alloplay -= skiplen;
					allopos += skiplen;
					samppos += skiplen;
					if (allolen==0) {
						allolen = allos[allo].relrestart;
						if (allolen==0) {
							// no continuation
							cur_alloplay = FALSE;
							allolen = -samppos;
						} else {
							allopos = allos[allo].length - allolen;
						}
					}
				}
				// if no continuation, then skip to start of next allophone
				if (!cur_alloplay) {
					tnew = tnext;
				} else {
					// truncate allolen so that sample fits into buffer
					if (samppos + allolen > len) {
						// if we truncate, then allo continues into next sample. so done
						// allolen might become negative here
						allolen = len-samppos;
						done_here = TRUE;
					}
					// now actually add sample to buffer
					// first skip 'pause' part if necessary
					if (allopos<0) {
						i=min(-allopos,allolen);
						allopos += i;
						samppos += i;
						alloplay -= i;
						allolen -= i;
					}
					volume = iSoundKnobs[3]*17;
					for (i=0;i<allolen;i++) {
						sample[(WORD)(samppos++)] +=
							((char)(allosample[(WORD)allos[allo].offset + (WORD)(allopos++)]))*volume;
					}
					alloplay -= allolen;
					allolen = allos[allo].relrestart;
					allopos = allos[allo].length - allolen;
					tnew = sampstart + samppos*(long)TStatesPerSample;
				}
			}
			GlobalUnlock(allosampbuf[allos[allo].allobuf]);
		} else {
			// (not cur_alloplay)
			tnew = tnext;
		}
		// if still, or not yet, playing at end of sample, then done
		if (done_here) return;
		// now skip allo's until next starts later than tnew
		// keep a pointer to the last actual allo (not delay) before allo with t>tnew
		// Otro idea: skip until real allo found, disregard time altogether
//		cur_nextallo = (cur_spointer+1)%cursampbuflen;
//		found_allo = FALSE;
//		do {
			cur_spointer = (cur_spointer+1)%cursampbuflen;		// next one, or empty
			if (cur_spointer == cur_s2pointer) {
				tcurr = tnew+1;
				tnext = tnew+1;
			} else {
				tcurr = (cur_starthi[cur_spointer]-timebase)*tframe + cur_startlo[cur_spointer];
//				if ((cur_sample[cur_spointer] & 0x3F)>4) {
//					cur_nextallo = cur_spointer;
//					found_allo = TRUE;
//				}
				cur_next = (cur_spointer+1)%cursampbuflen;
				if (cur_next == cur_s2pointer)
					tnext = tnew+1;
				else
					tnext = (cur_starthi[cur_next]-timebase)*tframe + cur_startlo[cur_next];
			}
//		} while ((tnext<=tnew)&&(!found_allo));
		// now set all times for allos between cur_nextallo and cur_spointer, inclusive,
		// to tnew, then set current sample to the one pointed to by cur_nextallo
//		i=cur_nextallo;
		i=cur_spointer;
//		do {
			cur_starthi[i] = timebase + (tnew/tframe);
			cur_startlo[i] = tnew % tframe;
//			i = (i+1)%cursampbuflen;
//		} while (i != (cur_spointer+1)%cursampbuflen);
//		// note: above sets time of empty slot (if cur_sp..==cur_s2p..) but this is no problem
//		cur_spointer = cur_nextallo;
		allo = cur_sample[cur_spointer];
	} while (cur_spointer != cur_s2pointer);
	return;
}



void currah_reset(void)
{
	cur_cursample = 0;
	cur_spointer = cur_s2pointer = 0;
}

void memspecial(void)
{
	int i;
// int j;
//	char msg[100];
	// it is only possible to get here if currah is emulated, so no check is
	//  done.
	if (specialaddr & 0x1000) {
		if (specialaddr & 0x2000) {
			if (!currah_50hz()) {			// if previous sample is played
				cur_cursample = (specialdata & 0x3f) + ((specialaddr&1)<<6);
				// if high pitched simple allophone is played for a second time,
				// do not store it in the buffer but continue playing the previous one
				i = (cur_s2pointer + cursampbuflen - 1)%cursampbuflen;
				if (!((cur_cursample & 0x40) && (allos[cur_cursample].simple) &&
						(cur_s2pointer != cur_spointer) && (cur_sample[i]==cur_cursample))) {
					getcurrenttime();
					cur_startlo[cur_s2pointer] = soundtimelo;
					cur_starthi[cur_s2pointer] = soundtimehi;
					cur_sample[cur_s2pointer] = cur_cursample;
					cur_s2pointer=(cur_s2pointer+1)%cursampbuflen;
					cur_currlength = allos[cur_cursample].playlength * TStatesPerSample;
				} else {
					cur_currlength += allos[cur_cursample].playlength * TStatesPerSample;
				}
				currah_setbusybit();
			}
		} else {
			if ((cur_cursample)&&(!(specialdata & 0x3f))) {		// reset
				cur_cursample = 0;
				getcurrenttime();
				cur_startlo[cur_s2pointer] = soundtimelo;
				cur_starthi[cur_s2pointer] = soundtimehi;
				cur_sample[cur_s2pointer] = cur_cursample;
				cur_s2pointer=(cur_s2pointer+1)%cursampbuflen;
			}
		}


//		i=cur_s2pointer-1;
//		j=0;
//		msg[0]=0;
//		do {
//			j++;
//			i = (i+cursampbuflen)%cursampbuflen;
//			sprintf(msg+strlen(msg)," %u",cur_sample[i]);
//			i--;
//		} while (j<16);
//		WRITEDEBUGMSG(msg,2);
	}
}

BOOL currah_50hz(void)
// returns TRUE if currah chip is busy
{
	long timeplayed;
	DWORD timebase;
	int last = (cur_s2pointer + cursampbuflen - 1)%cursampbuflen;
	if (cur_spointer == cur_s2pointer) {
		// nothing pending, not busy
		pokepage(12,0x1000,0);
		return FALSE;
	}
	getcurrenttime();
	timebase = min((DWORD)soundtimehi,cur_starthi[last]);
	timeplayed =
		((DWORD)soundtimehi - timebase)*tframe + soundtimelo -
		((cur_starthi[last] - timebase)*tframe + cur_startlo[last]);
	if (timeplayed > cur_currlength) {
		// current allo has had enough time
		pokepage(12,0x1000,0);
		// advance pointer here if sound is off, i.e. speak routine is not called
		if (!state.sound)
			cur_spointer = (cur_spointer+1) % cursampbuflen;
		return FALSE;
	} else {
		pokepage(12,0x1000,1);
		enable_soundblocks();		// makes sure (silent) sound blocks are generated
		return TRUE;
	}
}

void currah_setbusybit(void)
{
	currah_50hz();
}

void rst38(void)
{
	if (state.currahemulated)
		toggle_currah();
}
