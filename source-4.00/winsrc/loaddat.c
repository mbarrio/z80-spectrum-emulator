#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "spectrum.h"

char  datfile[128];
char  datdir[128];
char  DatLoadTitle[128];
char  datext[10];

void edfb()
{
   int i,j,k;
   WORD start,len;
   int level;
   HFILE handle;
	FARPROC lpfnDialProc;

	z80header.pc+=2;
	z80header.fa^=0x100;    // CCF; signal 'error loading .DAT file'
	level=((WORD)z80header.fa & 0xFF);
	sprintf(datext,"%d.DAT",level);
	strcpy(datfile,gszZ80File);
	for (i=0;datfile[i];i++) {}
	for (j=i; (j>=0)&&(datfile[j]!='\\')&&(datfile[j]!=':') ;j--) {}
	j++;
	for (k=0;(k<(12-strlen(datext)))&& datfile[j+k] && (datfile[j+k]!='.'); k++) {}
	datfile[j+k]=0;
	strcat(datfile,datext);
	handle=OpenRead(datfile);
	if (handle==HFILE_ERROR) {
		BOOL SltLoad=FALSE;
		// now try to load from SLT block in .Z80 file
		// first try to open .SLT file
		strcpy(datfile,gszZ80File);
		handle=OpenRead(datfile);
		if (handle==HFILE_ERROR) {
			strcpy(datfile,gszZ80File);
			AddFile(datfile,"/.SLT");
			handle=OpenRead(datfile);
		}
		if (handle==HFILE_ERROR) {
			strcpy(datfile,gszZ80File);
			AddFile(datfile,"/.Z80");
			handle=OpenRead(datfile);
		}
      if (handle!=HFILE_ERROR) {
        // we succeeded in opening it
        _llseek(handle,6,0);
        _lread(handle,&len,2);       // PC
        if (!len) {
          // it's a >= v2.01 file
          _llseek(handle,30,0);
          _lread(handle,&len,2);
          _llseek(handle,len+32,0);     // start of pages
          do {
                if (_lread(handle,&len,2)!=2) len=0;
                _llseek(handle,len+1,1);    // skip page no and page
          } while (len);
          _lread(handle,&len,2);     // read first two letters of SLT id.
          if (len == ('S'+('L'<<8))) {
            // we did find the level data
            long curpos=0,levpos=-1,levsiz;
            struct {
                WORD type;
                WORD levno;
                DWORD length;
            } hdr;
            HGLOBAL hdata;
            BYTE *datafile;
            _llseek(handle,1,1);    // skip 'T' of "SLT" id.
            do {
                i=_lread(handle,&hdr,8);
                if ((hdr.type==1)&&(hdr.levno==level)) {
                    levpos=curpos;
                    levsiz=hdr.length;
                }
                curpos+=hdr.length;
            } while ((i==8)&&(hdr.type));
            if ((i==8)&&(levpos!=-1)) {
                // table read OK and level found
                _llseek(handle,levpos,1);
                hdata=GlobalAlloc(GHND,levsiz);
                datafile=GlobalLock(hdata);
                _lread(handle,datafile,levsiz);
                if (z80header.hl>=16384) {
                   // it does not decompress into rom
                   unpack(datafile,
                          (BYTE*)&SpecMem[z80header.hl],
                          -z80header.hl,
                          levsiz);
                   z80header.fa^=0x100;      // CCF again, loaded .DAT OK
                   sprintf(DatLoadTitle,
                    "Loaded level %d from %s, at address #%04x (length: #%x bytes)",
                    level,datfile,z80header.hl,unpack_outcount);
                   quicknotify(DatLoadTitle);
                   SltLoad=TRUE;
                }
                GlobalUnlock(hdata);
                GlobalFree(hdata);
            }
          }
        }
        _lclose(handle);
      }
      if (SltLoad) return;
      strcpy(datdir,gszZ80FileDir);
      sprintf(DatLoadTitle,"Loading level %d data:",level);
		lpfnDialProc=MyMakeProcInstance(LoadZ80DialProc,ghInstance);
		i=DialogBoxParam(ghInstance,"LOADZ80FILE",hWndMain,lpfnDialProc,dat_load);
		MyFreeProcInstance(lpfnDialProc);
		videobuf->updatevisibility=TRUE;
		if (i==of_error) return;
		handle=OpenRead(datfile);
		if (handle==HFILE_ERROR) return;
	}
	start=z80header.hl;
	if (start < 0x4000) return;
	len=_lread(handle,&SpecMem[start],0x10000L-(LONG)start);
	z80header.fa^=0x100;    // CCF again, signal 'loaded .DAT OK'
	_lclose(handle);
   sprintf(DatLoadTitle,
           "Loaded level %d (from .DAT file) at address #%04x (length: #%x bytes)",
           level,start,len);
   quicknotify(DatLoadTitle);
}


