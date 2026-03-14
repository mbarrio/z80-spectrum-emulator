#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "spectrum.h"

#define warmodebuflen 1024

typedef struct
{     								// array with pointers into tzx file
	WORD listno;					// corresponding entry in menu list
	long pos;						// position, in bytes, of block
} TZXBLK;

TZXBLK* TzxArray=NULL;
HGLOBAL hTzxArray;
int   TapeInPlaying=0;			// TRUE if playing .TAP file in normal or warajevo mode
int	TzxInPlaying=0;			// TRUE if playing .TZX file.  Not both are TRUE at same time
char	TapeType=0;					// 0 is TAP, 1 means TZX.  If any of above is TRUE, this is valid
long  TapeInFilePos=0;			// For both .TAP and .TZX; -1 means don't know, figure out using:
int   TapeInBlockPos=0;			// For both .TAP and .TZX
int	TapeInListBlocks=0;
int   TapeInTotalBlocks=0;		// For both .TAP and .TZX
char	TzxForLooping=0;			// TRUE if executing FOR loop
long	TzxForPos=0;				// File pos of first blk after FOR
WORD	TzxForBlk=0;				// Block no of &c
WORD  TzxForCounter=0;			// # loops to be executed, including current one
long	TzxCallPos=0;				// File pos of call sequence blk currently being executed
WORD	TzxCallBlk=0;				// Block no of &c
WORD	TzxReturnAddress=0;		// 0=not calling; 1=executing first subroutine now &c
int   TapeInWarajevo=0;			// TRUE if playing .TAP in Warajevo mode
char	TzxVerbose=0;				// TRUE if tzx files are listed verbosely
char	TapeMaxLoadSpeed=0;
int   TapeOutMirror=0;
char  MirTempName[144]="";
LONG  MirFilePos;
HGLOBAL hWarBuffer=NULL;
HGLOBAL hMirBuffer=NULL;
BYTE	gbBlocktype;				// global output of BlockLen
BYTE  *MirBuffer;

void resetmirroring(void);
BOOL DeleteBlock(HFILE, int);
BOOL MoveBlock(HFILE, int, int);

long BlockLen(HFILE tapefile)
// Returns length of block without length word for .TAP files
// Returns length of block including type byte, for .TZX files
// Returns -2 in error
{
	long len=0;
	long lenofs=0,lenlen=0,lenadd=0;
	long pos=_llseek(tapefile,0,1);		// move 0 bytes from HERE, i.e. get pos
	if (TapeType) { 							// i.e. TZX
		if (_lread(tapefile,&gbBlocktype,1)!=1) {
			len=-2;								// signal 'error' or EOF
			_llseek(tapefile,pos,0);
			return(len);
		}
		switch (gbBlocktype) {
		case 0x10:	lenofs=2;	lenlen=2;	lenadd=4;	break;
		case 0x11:	lenofs=0xf;	lenlen=3;	lenadd=0x12;break;
		case 0x12:									lenadd=4;	break;
		case 0x13:	lenofs=0;	lenlen=1;	lenadd=1;	break;	// times 2
		case 0x14:	lenofs=7;	lenlen=3;	lenadd=0xa;	break;
		case 0x15:	lenofs=5;	lenlen=3;	lenadd=8;	break;
		case 0x20:									lenadd=2;	break;
		case 0x21:	lenofs=0;	lenlen=1;	lenadd=1;	break;
		case 0x22:									lenadd=0;	break;
		case 0x23:									lenadd=2;	break;
		case 0x24:                          lenadd=2;	break;
		case 0x25:									lenadd=0;	break;
		case 0x26:	lenofs=0;   lenlen=2;   lenadd=2;	break;	// times 2
		case 0x27:									lenadd=0;	break;
		case 0x28:	lenofs=0;	lenlen=2;	lenadd=2;	break;
		case 0x30:	lenofs=0;	lenlen=1;	lenadd=1;	break;
		case 0x31:	lenofs=1;	lenlen=1;	lenadd=2;	break;
		case 0x32:	lenofs=0;	lenlen=2;	lenadd=2;	break;
		case 0x33:  lenofs=0;	lenlen=1;	lenadd=1;	break;	// times 3
		case 0x34:									lenadd=8;	break;
		case 0x35:	lenofs=0x10;lenlen=4;	lenadd=0x14;break;
		case 0x40:	lenofs=1;	lenlen=3;	lenadd=4;	break;
		case 0x5a:									lenadd=9;	break;
		default:		lenofs=0;	lenlen=4;	lenadd=4;	break;
		}
		_llseek(tapefile,lenofs,1);
		if (lenlen) {
			if (_lread(tapefile,&len,lenlen)!=lenlen) {
				len=-2;
				_llseek(tapefile,pos,0);
				return(len);
			}
		}
		if (gbBlocktype==0x13) len*=2;
		if (gbBlocktype==0x33) len*=3;
		if (gbBlocktype==0x26) len*=2;
		len += lenadd + 1;			// +1 to account for type byte
	} else {
		// .TAP
		if (_lread(tapefile,&len,2)!=2)
			len=-2;
	}
	_llseek(tapefile,pos,0);
	return(len);
}

void SkipBlock(HFILE tapefile)
{
	long len;
	len=BlockLen(tapefile);
	if (TapeType)
		_llseek(tapefile,len,1);		// tzx
	else
		_llseek(tapefile,len+2,1);		// tap
}

void WindTape(HFILE tapefile)
{
	int i;
	if (TapeInFilePos!=-1) {
		_llseek(tapefile,TapeInFilePos,0);
		return;
	}
	if (TapeType) {
		if (TapeInBlockPos >= TapeInTotalBlocks) {
			_llseek(tapefile,0,2);		// goto end
			return;
		}
		TapeInFilePos = TzxArray[TapeInBlockPos].pos;
		_llseek(tapefile,TapeInFilePos,0);
		return;
	}
	i=TapeInFilePos=0;
	_llseek(tapefile,TapeInFilePos,0);
	while (i<TapeInBlockPos) {
		TapeInFilePos += BlockLen(tapefile);
		if (!TapeType)
			TapeInFilePos += 2;		// length word for .TAP files
		_llseek(tapefile,TapeInFilePos,0);
		i++;
	}
}

BOOL CheckTapefile(HFILE tapefile)
{
	int dummy;
	_llseek(tapefile,0,0);
	while (BlockLen(tapefile)!=-2) {
		SkipBlock(tapefile);
		_llseek(tapefile,-1,1);
		if (_lread(tapefile,&dummy,1)!=1) return FALSE;
	}
	return TRUE;
}

BOOL RepairTapefile(HFILE tapefile)
{
	WORD dummy;
	long eofpos=_llseek(tapefile,0,2);
	long curpos=_llseek(tapefile,0,0);
	while (curpos+BlockLen(tapefile)+(TapeType?0:2)<eofpos-1) {
		curpos+=BlockLen(tapefile)+(TapeType?0:2);
		SkipBlock(tapefile);
	}
	if (curpos==eofpos) return(TRUE);
	if (TapeType) {
		// TZX: truncate
		return(_lwrite(tapefile,&dummy,0)==0);
	}
	if (curpos==eofpos-1) {
		return(_lwrite(tapefile,&dummy,0)==0);
	}
	dummy=eofpos-curpos-2;
	if (dummy==0) {
		return(_lwrite(tapefile,&dummy,0)==0);
	}
	return(_lwrite(tapefile,&dummy,2)==2);
}

void InterpretHeader(BYTE *data, char *str)
{
	BYTE b;
	WORD w;
	int i;

	b=data[0];
	for (i=0;i<10;i++) data[i]=data[i+1];
	data[10]=0;
	for (i=9;i&&(data[i]==' ');i--) data[i]=0;
	w=*((WORD*)&(data[13]));
	switch (b) {
	case 0:
		if (w<0x8000L)
			sprintf(str,"Program: %s LINE %u",data,w);
		else
			sprintf(str,"Program: %s",data);
		break;
	case 1:
		sprintf(str,"Number array: %s DATA %c()",data,64+(data[14]&31));
		break;
	case 2:
		sprintf(str,"Char array: %s DATA %c$()",data,64+(data[14]&31));
		break;
	case 3:
		sprintf(str,"Bytes: %s CODE %u,%u",data,w,*((WORD*)&(data[11])));
		break;
	default:
		sprintf(str,"Nonsensical header! ('name': %s)",data);
		break;
	}
}

HGLOBAL BlockString(HFILE tapefile)
{
	HGLOBAL mem,mem2;
	char far* str;
	BYTE* data;
	BYTE b;
	int i;
	long len,blen;
	long pos;

	mem=GlobalAlloc(GHND,300);
	mem2=GlobalAlloc(GHND,4096);
	str=GlobalLock(mem);
	data=(BYTE*)GlobalLock(mem2);
	len=BlockLen(tapefile);
	if (len==-2) {
		str[0]=0;
		goto exitbs2;
	}
	if (TapeType) {
		// .TZX block
		// Get absolute position
		pos = _llseek(tapefile,0,1);
		// Read a bit of data
		_lread(tapefile,data,0x80);
		_llseek(tapefile,pos,0);
		// ghBlocktype is set by BlockLen above
		switch (gbBlocktype) {
		case 0x10:	sprintf(str,"10h ");
						if ((data[5]&0x80)||((WORD)len > 19+0x07)) {
							sprintf(str,"10h Normal data (%lu)",len-0x07);
						} else {
							InterpretHeader(data+6,str+4);
						}
						break;
		case 0x11:	sprintf(str,"11h Turbosaved data (%lu)",len-0x13); break;
		case 0x12:	sprintf(str,"12h Pure tone (%u edges)",*(WORD*)(data+3)); break;
		case 0x13:	sprintf(str,"13h Pulse list (%lu edges)",(len/2-1)); break;
		case 0x14:	sprintf(str,"14h Turbosaved pure data (%lu)",len-0xb); break;
		case 0x15:	sprintf(str,"15h Direct recording (%lu bits)",8*(len-9)); break;
		case 0x20:	if (*(WORD*)(data+1))
							sprintf(str,"20h Pause (%u ms)",*(WORD*)(data+1));
						else
							sprintf(str,"20h Pause (indefinite)");
						break;
		case 0x21:	data[ data[1] + 2 ]=0;
						sprintf(str,"21h Group: %.40s",data+2); break;
		case 0x22:	sprintf(str,"22h Group end"); break;
		case 0x23:	sprintf(str,"23h Jump (%d blocks)",*(int*)(data+1)); break;
		case 0x24:	sprintf(str,"24h FOR A=1 TO %d",*(WORD*)(data+1)); break;
		case 0x25:	sprintf(str,"25h NEXT A"); break;
		case 0x26:	sprintf(str,"26h CALL sequence (%u)",*(WORD*)(data+1)); break;
		case 0x27:	sprintf(str,"27h RETURN from call"); break;
		case 0x28:	sprintf(str,"28h Selection menu (%u entries)",(WORD)data[3]); break;
		case 0x2a:	sprintf(str,"2ah Stop tape in 48K mode"); break;
		case 0x30:	data[ data[1] + 2 ]=0;
						sprintf(str,"30h Text: %.40s",data+2); break;
		case 0x31:	data[ data[2] + 3 ]=0;
						sprintf(str,"31h Pause msg: %.40s",data+3); break;
		case 0x32:	sprintf(str,"32h Archive info"); break;
		case 0x33:	sprintf(str,"33h Hardware type"); break;
		case 0x34:	sprintf(str,"34h Emulation info"); break;
		case 0x35:	sprintf(str,"35h Custom info (%.16s)",data+1); break;
		case 0x40:	sprintf(str,"40h Snapshot (type %u)",(WORD)data[1]); break;
		case 0x5a:	sprintf(str,"5ah ZXTape v%u.%02u identifier",(WORD)data[8],(WORD)data[9]); break;
		default:		sprintf(str,"%02Xh Unknown v1.12 block type",(WORD)data[0]); break;
		}
		if (!TzxVerbose) {
			for (i=4; str[i-1]; i++) str[i-4]=str[i];		// Throw away hex no in front
		}
	} else {
		// .TAP block
		if (len==0) {
			sprintf(str,"  Empty block");
			goto exitbs2;
		}
		_lread(tapefile,data,pos=3);
		if (len==1) {
			sprintf(str,"  Checksum byte (#%02X)",data[2]);
			goto exitbs;
		}
		if (data[2]&0x80) {
			if (data[2]==0xff)
				sprintf(str,"  Data (%u)",(WORD)len-2);
			else
				sprintf(str,"  Data (%lu; A=#%02X)",len-2,data[2]);
			goto exitbs;
		}
		if ((data[2]!=0)||(len!=19)) {
			sprintf(str,"Header (%lu bytes; A=#%02X)",len-2,data[2]);
			goto exitbs;
		}
		_lread(tapefile,data,17);
		pos=20;
		InterpretHeader(data,str);
		exitbs:
		_llseek(tapefile,-pos+2,1);
		pos=len+2;
		b=0;
		while (len!=0) {
			blen=min(4096,len);
			_lread(tapefile,data,blen);
			for (i=0;i<blen;i++) b^=data[i];
			len-=blen;
		}
		_llseek(tapefile,-pos,1);
		if (b) {
			sprintf(data,"%s  [checksum]",str);
			sprintf(str,"%s",data);
		}
	}
	exitbs2:
	GlobalUnlock(mem);
	GlobalUnlock(mem2);
	GlobalFree(mem2);
	return(mem);
}


WORD BlockToList(WORD block)
{
	WORD w;
	if (!TapeType) return(block);				// .tap files
	if (block >= TapeInTotalBlocks)
		return(TapeInListBlocks);
	TzxArray=(TZXBLK*)GlobalLock(hTzxArray);
	w=TzxArray[block].listno;
	GlobalUnlock(hTzxArray);
	return w;
}

WORD ListToBlock(WORD list)
{
	WORD w;
	if (!TapeType) return(list);				// .tap files
	if (list >= TapeInListBlocks)
		return(TapeInTotalBlocks);
	w=0;
	TzxArray = (TZXBLK*)GlobalLock(hTzxArray);
	while (TzxArray[w].listno != list) w++;
	GlobalUnlock(hTzxArray);
	return (w);
}

BOOL CALLBACK TzxSelectDialProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	int i,j;
	static HGLOBAL hrd;
	char str[80];
	USERSELBLOCK *data;

	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		if (!tzx_selectdata) {
			SendMessage(hDlg,WM_CLOSE,0,0);		// no data, then do nothing
		}
		data = (USERSELBLOCK*)GlobalLock(tzx_selectdata);
		for (i=0;data[i].blockpos != 0xFFFFL;i++) {
			for (j=0;data[i].title[j];j++)
				if ((data[i].title[j]==0x0d)||(data[i].title[j]==0x0a))
					data[i].title[j]=0;
			sprintf(str,"(%d:) %.30s",i+1,data[i].title);
			SendDlgItemMessage(hDlg,TZXS_LISTBOX,LB_ADDSTRING,0,(long)&str);
		}
		SendDlgItemMessage(hDlg,TZXS_LISTBOX,LB_SETCURSEL,0,0L);
		GlobalUnlock(tzx_selectdata);
		break;
	case WM_CLOSE:
		hrd=RepaintData(hDlg);
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		PostMessage(hWndMain,IK_FREELPFN,hrd,NULL);
		tzx_selectwindow=NULL;
		GlobalFree(tzx_selectdata);
		break;
	case WM_COMMAND:
		switch (wPar) {
		case TZXS_LISTBOX:
			if (HIWORD(lPar)!=LBN_DBLCLK) {
				return TRUE;
			}
		case TZXS_OK:
			tzx_selectblock = SendDlgItemMessage(hDlg,TZXS_LISTBOX,LB_GETCURSEL,0,0L);
			data = (USERSELBLOCK*)GlobalLock(tzx_selectdata);
			tzx_selectblock = data[tzx_selectblock].blockpos;
			if (tzx_selectblock != 0xFFFF)
				SetPauseState(0);			// this causes tzx code to destroy window
			GlobalUnlock(tzx_selectdata);
			break;
	  }
	  return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}



BOOL CALLBACK PlayTapProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{

	HFILE hfile;
	HGLOBAL hglob;
	static HGLOBAL hrd;
	static int moving;
	char text1[100],text2[100];
	int i;
	BOOL b;

	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		CheckDlgButton(hDlg,PT_WARAJEVO,TapeInWarajevo);
		CheckDlgButton(hDlg,PT_VERBOSE,TzxVerbose);
		CheckDlgButton(hDlg,PT_MAXSPEED,TapeMaxLoadSpeed);
		SendDlgItemMessage(hDlg,PT_LISTBOX,LB_RESETCONTENT,0,0L);
		moving=-1;
		if (hTzxArray) GlobalFree(hTzxArray);
		hTzxArray = GlobalAlloc(GMEM_MOVEABLE, sizeof(TZXBLK) );
		TapeInTotalBlocks = 0;
		TapeInListBlocks = 0;
		if (gszPlayTapFile[0]) {
			SetDlgItemText(hDlg,PT_FILENAME,gszPlayTapFile);
			hfile=_lopen(gszPlayTapFile,READ);
			b=TRUE;
			while (BlockLen(hfile)!=-2) {
				TzxArray = (TZXBLK*)GlobalLock(hTzxArray);
				TzxArray[TapeInTotalBlocks].listno = TapeInListBlocks;
				if (!b) TzxArray[TapeInTotalBlocks].listno--;
				TzxArray[TapeInTotalBlocks].pos = _llseek(hfile,0,1);
				GlobalUnlock(hTzxArray);
				TapeInTotalBlocks++;
				hTzxArray = GlobalReAlloc(hTzxArray, sizeof(TZXBLK)*(TapeInTotalBlocks+1),GMEM_MOVEABLE);
				hglob=BlockString(hfile);		// this also sets gbBlockType
				if (b) {								// not in .tzx group
					SendDlgItemMessage(hDlg,PT_LISTBOX,LB_ADDSTRING,0,(long)GlobalLock(hglob));
					TapeInListBlocks++;
				}
				if (TapeType && !TzxVerbose) {	// tzx
					if (gbBlocktype == 0x21)	b=FALSE;
					if (gbBlocktype == 0x22)	b=TRUE;
				}
				GlobalUnlock(hglob);
				GlobalFree(hglob);
				SkipBlock(hfile);
			}
			SendDlgItemMessage(hDlg,PT_LISTBOX,LB_ADDSTRING,0,(long)"  --<end-of-file>--");
			_lclose(hfile);
		} else {
			SetDlgItemText(hDlg,PT_FILENAME,"<none>");
			SendDlgItemMessage(hDlg,PT_LISTBOX,LB_ADDSTRING,0,(long)"<nothing>");
			TapeInPlaying=0;
			TzxInPlaying=0;
			TapeInBlockPos=-1;
			TapeInTotalBlocks=0;
			TapeInListBlocks=0;
		}
		if (TzxInPlaying) {
			ShowWindow(GetDlgItem(hDlg,PT_DELETE),SW_HIDE);
			if (inning)
				SetDlgItemText(hDlg,PT_MOVE,"Pause");
			else
				SetDlgItemText(hDlg,PT_MOVE,"Play");
		} else {
			ShowWindow(GetDlgItem(hDlg,PT_DELETE),SW_SHOW);
			SetDlgItemText(hDlg,PT_MOVE,"Move");
		}
		if (TapeInBlockPos>=TapeInTotalBlocks) TapeInBlockPos=0;
		SendDlgItemMessage(hDlg,PT_LISTBOX,LB_SETCURSEL,BlockToList(TapeInBlockPos),0L);
		break;
	case WM_CLOSE:
		hrd=RepaintData(hDlg);
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		if (TzxInPlaying) {
			TzxInPlaying = FALSE;
			inning=0;
		}
		if (TapeInPlaying) {
			TapeInPlaying = FALSE;
		}
		gszPlayTapFile[0]=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnPlayTapProc);
		hPlayTapDialog=NULL;
		break;
	case WM_USER+1:      // Update current position bar
		SendDlgItemMessage(hDlg,PT_LISTBOX,LB_SETCURSEL,BlockToList(TapeInBlockPos),0L);
		return TRUE;
	case WM_COMMAND:
		switch (wPar) {
		case PT_CANCEL:
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		case PT_LISTBOX:
			if (HIWORD(lPar)==LBN_SELCHANGE) {
				ResetWarajevo();
				TapeInBlockPos=ListToBlock(SendDlgItemMessage(hDlg,PT_LISTBOX,LB_GETCURSEL,0,0L));
				TapeInFilePos=-1;
				TzxForLooping=0;		// cancel any loop that's currently being exec'd
				TzxReturnAddress=0;	// cancel any CALL that's &c
			}
			if ((HIWORD(lPar)!=LBN_DBLCLK)||(moving==-1)) return TRUE;
			// otherwise, continue into PT_MOVE:
		case PT_MOVE:
			if (TapeType) {
				// don't move in .TZX files, button is 'Play' now
				if (TzxInPlaying) {
					if (inning) {	// do pause
//						// get position from list box, as TapeInBlockPos may have been
//						//  advanced early (last portion of block is being played)
//						TapeInBlockPos=ListToBlock(SendDlgItemMessage(hDlg,PT_LISTBOX,LB_GETCURSEL,0,0L));
						TapeInFilePos=-1;
						inning = 0;
					} else {
						initTZXinning();
					}
					SendMessage(hDlg,WM_INITDIALOG,0,0L);
				} else
					MessageBeep(-1);
				return TRUE;
			}
			if ((moving==-1)&&(TapeInPlaying)&&(TapeInBlockPos<TapeInTotalBlocks)) {
				moving=TapeInBlockPos;
				SendDlgItemMessage(hDlg,PT_LISTBOX,LB_GETTEXT,moving,(DWORD)text1);
				sprintf(text2,"MOVING: %s to...",text1);
				SendDlgItemMessage(hDlg,PT_LISTBOX,LB_DELETESTRING,moving,0);
				SendDlgItemMessage(hDlg,PT_LISTBOX,LB_INSERTSTRING,moving,(DWORD)text2);
				SendDlgItemMessage(hDlg,PT_LISTBOX,LB_SETCURSEL,TapeInBlockPos,0);
				return TRUE;
			}
			if (moving==-1) {
				MessageBeep(-1);
				return TRUE;
			}
			if ((moving==TapeInBlockPos)||(moving==TapeInBlockPos-1)) {
				SendMessage(hDlg,WM_INITDIALOG,0,0L);
				return TRUE;
			}
			if (!TapeInPlaying) {
				moverr:
				MessageBeep(-1);
				SendMessage(hDlg,WM_INITDIALOG,0,0L);
				return TRUE;
			}
			hfile=_lopen(gszPlayTapFile,READ_WRITE);
			if (hfile==HFILE_ERROR) goto moverr;
			if (!MoveBlock(hfile,moving,TapeInBlockPos)) {
				_lclose(hfile);
				goto moverr;
			}
			_lclose(hfile);
			TapeInBlockPos=0;
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
			return TRUE;
		case PT_DELETE:
			if (TapeType) {
				// Don't allow deleting of blocks in .TZX files
				MessageBeep(-1);
				return TRUE;
			}
			if ((!TapeInPlaying)
				  ||(TapeInBlockPos<0)||(TapeInBlockPos>=TapeInTotalBlocks)) {
					MessageBeep(-1);
					return TRUE;
			}
			SendDlgItemMessage(hDlg,PT_LISTBOX,LB_GETTEXT,TapeInBlockPos,(DWORD)text1);
			sprintf(text2,"About to delete: %s",text1);
			i=MessageBox(hDlg,text2,"About to DELETE a block!",
						MB_ICONEXCLAMATION|MB_OKCANCEL);
			if (i!=IDOK) return TRUE;
			hfile=_lopen(gszPlayTapFile,READ_WRITE);
			if (hfile==HFILE_ERROR) {
				MessageBeep(-1);
				return TRUE;
			}
			if (!DeleteBlock(hfile,TapeInBlockPos)) {
				MessageBeep(-1);
				return TRUE;
			}
			_lclose(hfile);
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
			return TRUE;
		case PT_WARAJEVO:
			if (TapeType) {		// don't do anything in TZX mode
				MessageBeep(-1);
				return TRUE;
			}
			TapeInWarajevo=!TapeInWarajevo;
			CheckDlgButton(hDlg,PT_WARAJEVO,TapeInWarajevo);
			ResetWarajevo();
			return TRUE;
		case PT_VERBOSE:
			if (!TapeType) {
				MessageBeep(-1);
				return TRUE;
			}
			TzxVerbose=!TzxVerbose;
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
			return TRUE;
		case PT_MAXSPEED:
			TapeMaxLoadSpeed = !TapeMaxLoadSpeed;
			CheckDlgButton(hDlg,PT_MAXSPEED,TapeMaxLoadSpeed);
			return TRUE;
		case PT_OPEN:
			gszPlayTapFile[0]=0;
			TapeInPlaying=0;
			ResetWarajevo();
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
/* begin add/modify jts 4/8/97 */
			if ( lPar != CM_OPENGLOBALFILE )
			{
				i=FileDlg(2, gszPlayTapFile, TRUE, hDlg);
			} else {
				strcpy(gszPlayTapFile, gszDefaultFile);
				i=0;
			}
/* end add/modify jts 4/8/97 */
			videobuf->updatevisibility=TRUE;
			if (i==-1) {
				gszPlayTapFile[0]=0;
				return TRUE;
			}
			TapeType = extensionsub;		// 0=.TAP, 1=.TZX
			hfile=_lopen(gszPlayTapFile,READ);
			i=CheckTapefile(hfile);
			_lclose(hfile);
			if (!i) {
				if (MessageBox(hDlg,
									"Last block is incomplete!  Shall I repair the .TAP/.TZX file?",
									"Error in .TAP/.TZX file",
									MB_ICONEXCLAMATION|MB_OKCANCEL)==IDOK) {
					hfile=_lopen(gszPlayTapFile,READ_WRITE);
					RepairTapefile(hfile);
					_lclose(hfile);
				} else {
					gszPlayTapFile[0]=0;
					notify(BadTapFile);
					return TRUE;
				}
			}
			if (TapeType)
				TzxInPlaying=TRUE;
			else
				TapeInPlaying=TRUE;
			TapeInFilePos=-1;
			TapeInBlockPos=0;
			ResetWarajevo();
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
			return TRUE;
	  }
	  return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}



BOOL CALLBACK RecTapProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	HFILE hfile;
	HGLOBAL hglob;
	static HGLOBAL hrd;
	static int moving;
	static int cursel,totblks;
	char text1[100],text2[100];
	int i;

	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		CheckDlgButton(hDlg,RT_MIRROR,TapeOutMirror);
		SendDlgItemMessage(hDlg,RT_LISTBOX,LB_RESETCONTENT,0,0L);
		moving=-1;
		cursel=lPar-1;
		if (gszRecTapFile[0]) {
			SetDlgItemText(hDlg,RT_FILENAME,gszRecTapFile);
			hfile=_lopen(gszRecTapFile,READ);
			while (BlockLen(hfile)!=-2) {
				hglob=BlockString(hfile);
				SendDlgItemMessage(hDlg,RT_LISTBOX,LB_ADDSTRING,0,(long)GlobalLock(hglob));
				GlobalUnlock(hglob);
				GlobalFree(hglob);
				SkipBlock(hfile);
			}
			SendDlgItemMessage(hDlg,RT_LISTBOX,LB_ADDSTRING,0,(long)"  --<end-of-file>--");
			_lclose(hfile);
		} else {
			SetDlgItemText(hDlg,RT_FILENAME,"<none>");
			SendDlgItemMessage(hDlg,RT_LISTBOX,LB_ADDSTRING,0,(long)"<nothing>");
		}
		totblks=SendDlgItemMessage(hDlg,RT_LISTBOX,LB_GETCOUNT,0,0L)-1;
		if (cursel==-2) cursel=totblks-1;
		SendDlgItemMessage(hDlg,RT_LISTBOX,LB_SETCURSEL,cursel,0L);
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnRecTapProc);
		hRecTapDialog=NULL;
		break;
	case WM_COMMAND:
		switch (wPar) {
		case RT_CANCEL:
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		case RT_LISTBOX:
			if (HIWORD(lPar)==LBN_SELCHANGE) {
				cursel=SendDlgItemMessage(hDlg,RT_LISTBOX,LB_GETCURSEL,0,0L);
			}
			if ((HIWORD(lPar)!=LBN_DBLCLK)||(moving==-1)) return TRUE;
			// otherwise, continue into RT_MOVE:
		case RT_MOVE:
			if ((moving==-1)&&(cursel!=-1)&&(cursel<totblks)) {
				moving=cursel;
				SendDlgItemMessage(hDlg,RT_LISTBOX,LB_GETTEXT,moving,(DWORD)text1);
				sprintf(text2,"MOVING: %s to...",text1);
				SendDlgItemMessage(hDlg,RT_LISTBOX,LB_DELETESTRING,moving,0);
				SendDlgItemMessage(hDlg,RT_LISTBOX,LB_INSERTSTRING,moving,(DWORD)text2);
				SendDlgItemMessage(hDlg,RT_LISTBOX,LB_SETCURSEL,cursel,0);
				return TRUE;
			}
			if (moving==-1) {
				MessageBeep(-1);
				return TRUE;
			}
			if ((moving==cursel)||(moving==cursel-1)) {
				SendMessage(hDlg,WM_INITDIALOG,0,cursel+1);
				return TRUE;
			}
			hfile=_lopen(gszRecTapFile,READ_WRITE);
			if (hfile==HFILE_ERROR) {
				moverr:
				MessageBeep(-1);
				SendMessage(hDlg,WM_INITDIALOG,0,cursel+1);
				return TRUE;
			}
			if (!MoveBlock(hfile,moving,cursel)) {
				_lclose(hfile);
				goto moverr;
			}
			_lclose(hfile);
			SendMessage(hDlg,WM_INITDIALOG,0,cursel+1);
			return TRUE;
		case RT_DELETE:
			if ((!gszRecTapFile[0])||(cursel<0)||(cursel>=totblks)) {
					MessageBeep(-1);
					return TRUE;
			}
			SendDlgItemMessage(hDlg,RT_LISTBOX,LB_GETTEXT,cursel,(DWORD)text1);
			sprintf(text2,"About to delete: %s",text1);
			i=MessageBox(hDlg,text2,"About to DELETE a block!",
						MB_ICONEXCLAMATION|MB_OKCANCEL);
			if (i!=IDOK) return TRUE;
			hfile=_lopen(gszRecTapFile,READ_WRITE);
			if (hfile==HFILE_ERROR) {
				MessageBeep(-1);
				return TRUE;
			}
			if (!DeleteBlock(hfile,cursel)) {
				MessageBeep(-1);
				return TRUE;
			}
			_lclose(hfile);
			SendMessage(hDlg,WM_INITDIALOG,0,cursel+1);
			return TRUE;
		case RT_MIRROR:
			TapeOutMirror=!TapeOutMirror;
			resetmirroring();
			CheckDlgButton(hDlg,RT_MIRROR,TapeOutMirror);
			return TRUE;
		case RT_OPEN:
			gszRecTapFile[0]=0;
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
/* begin add/modified jts 4/8/97 */
			i = FileDlg(2, gszRecTapFile, FALSE, hDlg);
			// Puts -1, FE_OVERWRITE or FE_APPEND in iFileDlgFlg
/* end add/modified jts 4/8/97 */
			videobuf->updatevisibility=TRUE;
			if (i==-1) {
				gszRecTapFile[0]=0;
				return TRUE;
			}
			if (extensionsub) {
				// We cannot write .TZX files
				gszRecTapFile[0]=0;
				notify(NoWriteTZXFile);
				return TRUE;
			}
			hfile=_lopen(gszRecTapFile,READ);
			if ( (hfile == HFILE_ERROR) ||
				  ( (hfile != HFILE_ERROR)&&(iFileDlgFlg==FE_OVERWRITE) ) ) {
				// Ah.  It was a new file, or existing file need overwriting
				if (hfile != HFILE_ERROR) _lclose(hfile);
				hfile = _lcreat(gszRecTapFile,0);
				if (hfile == HFILE_ERROR) {
					gszRecTapFile[0]=0;
					notify(TapCreateErr);
					return TRUE;
				}
			}
			i=CheckTapefile(hfile);
			_lclose(hfile);
			if (!i) {
				gszRecTapFile[0]=0;
				notify(BadTapFile);
				return TRUE;
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0L);
			resetmirroring();    // to activate mirroring when a file's been opened
			return TRUE;
	  }
	  return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}



void LoadTrap(void)
{
	HFILE hfile;
	HGLOBAL hmem;
	BYTE far* blk;
	BYTE b,chksum;
	long len;
	int blklen,blkpos;
	if ((!TapeInPlaying)||TapeInWarajevo) {
		// emulate RET NZ instruction; given that ZF=1
		emulretnz:
		z80header.pc++;
		rreg-=(0x50000L-1);
		return;
	}
	if (SpacePressed()) {
		z80header.fa=0x007F;          // ZF=0, CF=0
		return;                       // Simply emulate RET NZ again
	}
	hfile=_lopen(gszPlayTapFile,READ);
	WindTape(hfile);
	len=BlockLen(hfile);
	if (len==-2) {
		TapeInFilePos=TapeInBlockPos=0;
		WindTape(hfile);
		len=BlockLen(hfile);
		if (len==-2) {
			TapeInPlaying=0;
			_lclose(hfile);
			goto emulretnz;
		}
	}
	TapeInFilePos+=len+2;
	_llseek(hfile,2,1);
	TapeInBlockPos++;
	if (hPlayTapDialog) SendMessage(hPlayTapDialog,WM_USER+1,0,0L);
	blklen=blkpos=0;
	hmem=GlobalAlloc(GHND,4096);
	blk=(BYTE*)GlobalLock(hmem);
	chksum=0;
	do {
		if (blkpos==blklen) {
			blkpos=0;
			blklen=min(4096,len);
			len-=blklen;
			if (_lread(hfile,blk,blklen)!=blklen) {
				TapeInPlaying=0;
				notify(ErrReadingTap);
				goto tapeerr;
			}
		}
		if (blklen==0) goto tapeerr;
		b=blk[blkpos++];
		chksum^=b;
		if (z80header.de==0) {           // DE=0, all bytes have been loaded
			z80header.hl = MAKEWORD(b,chksum);  // H=chksum, L=last byte loaded
			z80header.pc = 0x5DF;         // continue with: LD A,H/CP 1/RET
			goto freemem;
		}
		if (z80header.faa & 0x4000) {    // ZF=1, normal bytes
			if (z80header.faa & 0x0100) { // CF=1, normal loading
				pokebyte(SpecMem,z80header.ix,b);
				z80header.ix++;
			} else {                      // CF=0, verify
				b^=SpecMem[z80header.ix];
				if (b) goto tapeerr;
				z80header.ix++;
			}
		} else {                         // ZF=0, flag byte
			z80header.faa ^= b;
			if (z80header.faa & 0xFF) goto tapeerr;
			z80header.faa |= 0x4000;      // Set ZF
			z80header.de++;
		}
		z80header.de--;
	} while (1);
	tapeerr:
	z80header.bc=0x0006;                // B=0 (timer overflow)
	z80header.fa=0x4000;                // CF=0, ZF=1 (timer overflow)
	z80header.pc=0x5E2;                 // -> RET
	freemem:
	_lclose(hfile);
	GlobalUnlock(hmem);
	GlobalFree(hmem);
	return;
}


void SaveTrap(void)
{
	HFILE hfile;
	BYTE chksum;
	WORD len;
	LONG start;
	if (!gszRecTapFile[0]) {
		rreg-=(0x40000L-1);
		z80header.pc++;
		return;
	}
	if (SpacePressed()) {
		z80header.fa=0x007F;          // ZF=0, CF=0
		z80header.pc=0x053E;          // -> RET
		return;
	}
	z80header.fa=0x0100;             // ZF=0, CF=1
	z80header.bc=0x000E;
	z80header.hl=0xFF00;
	z80header.pc=0x053E;             // -> RET
	z80header.de--;
	z80header.ix++;                  // Undo fixup for flag saving
	hfile=_lopen(gszRecTapFile,READ_WRITE);
	if (hfile==HFILE_ERROR) {
		hfile=_lcreat(gszRecTapFile,0);
		if (hfile==HFILE_ERROR) goto serr2;
	}
	start=_llseek(hfile,0L,2);
	if (z80header.de>=0xFF00) {
		len=1;
		_lwrite(hfile,&len,2);
		_lwrite(hfile,&(z80header.faa),1);
		_lclose(hfile);
		return;
	}
	len=z80header.de+2;
	_lwrite(hfile,&len,2);
	_lwrite(hfile,&(z80header.faa),1);
	len-=2;
	if ((-z80header.ix)>=len) {
		if (_lwrite(hfile,SpecMem+z80header.ix,len)!=len) {
			serr:
			_llseek(hfile,start,0);
			_lwrite(hfile,NULL,0);
			_lclose(hfile);
			serr2:
			gszRecTapFile[0]=0;
			if (hRecTapDialog) PostMessage(hRecTapDialog,WM_INITDIALOG,0,0);
			notify(SaveTapErr);
			return;
		}
	} else {
		_lwrite(hfile,SpecMem+z80header.ix,-z80header.ix);
		_lwrite(hfile,SpecMem,z80header.ix+len);
	}
	chksum=(z80header.faa)&0xFF;
	for (;len;) chksum^=SpecMem[--len+z80header.ix];
	_lwrite(hfile,&chksum,1);
	_lclose(hfile);
	if (hRecTapDialog) SendMessage(hRecTapDialog,WM_INITDIALOG,0,-1L);
	z80header.ix+=z80header.de+1;
	z80header.de=0xFFFF;
}



void HandleWarajevoMode()
{
	if (!TapeInWarajevo) {
		waractive=0;
		warstopped=0;
		return;
	}
	if (warstopped) {    // A block has been loaded (partly)
		if (waractive) return;     // Be sure to include final edge
		warstopped=0;
		if (!TapeInPlaying) {
			waractive=0;
			return;
		}
		waractive=1;      // Set up variables for next block
		warbytesleft=0;   // No valid bytes left in buffer
		warleader=768;    // Start with (at least) 768x2 leader edges
		warsignal=1;
		warsiglenleft=1750000L;    // but first include half a second silence
		TapeInBlockPos++;
		if (hPlayTapDialog) SendMessage(hPlayTapDialog,WM_USER+1,0,0);
		TapeInFilePos=-1;
	}
}

void ResetWarajevo()
{
	if (!TapeInWarajevo) {
		waractive=0;
		warstopped=0;
      return;
   }
   waractive=TapeInPlaying;
	warstopped=0;
	warbytesleft=0;   // No valid bytes left in buffer
   warleader=768;    // Start with (at least) 768x2 leader edges
   warsignal=1;
	warsiglenleft=1750000L;
}

void GetWarModeData(void)
{
	HFILE hfile;
	long len;
	if (warstopped) {       // Final edge has been delivered; now stop completely
		waractive=0;
		return;
	}
	if (!hWarBuffer) {
		hWarBuffer=GlobalAlloc(GHND,warmodebuflen);
	} else {
		GlobalUnlock(hWarBuffer);
	}
	warbuffer=GlobalLock(hWarBuffer);
	hfile=_lopen(gszPlayTapFile,READ);
	WindTape(hfile);
	len=BlockLen(hfile);
	if (len==-2) {
		TapeInFilePos=TapeInBlockPos=0;
		if (hPlayTapDialog) SendMessage(hPlayTapDialog,WM_USER+1,0,0);
		WindTape(hfile);
		len=BlockLen(hfile);
		if (len==-2) {
			TapeInPlaying=0;
			_lclose(hfile);
			ResetWarajevo();
			return;
		}
	}
	if (len<=warbytecounter) {
		finaledge:
		warstopped=1;              // stopped && active: delivering final edge
		wardata=0x3fc0;            // terminate with 0 bit
		_lclose(hfile);
		return;
	}
	len = min(len-warbytecounter,warmodebuflen);
	warbytesleft=len;
	if (len==0) goto finaledge;
	_llseek(hfile,2+warbytecounter,1);
	if (_lread(hfile,warbuffer,len)!=len) {
		TapeInPlaying=0;
		notify(ErrReadingTap);
		_lclose(hfile);
		ResetWarajevo();
		return;
	}
	_lclose(hfile);
	wardata=0xff + (warbuffer[0]<<8);
	warbuffer++;
	warbytecounter++;
	warbytesleft--;
	return;
}

BOOL DeleteBlock(HFILE hfile, int num)
{
	int bp=TapeInBlockPos;
   long readpos,writepos,chunklen;
   HGLOBAL hmem;
	BYTE *data;

   hmem=GlobalAlloc(GHND,4096);
   if (!hmem) return(FALSE);
	data=GlobalLock(hmem);
   TapeInBlockPos=num;
	TapeInFilePos=-1;
	WindTape(hfile);
   if (BlockLen(hfile)==-2) {
		delerr:
		TapeInBlockPos=bp;
      TapeInFilePos=-1;
      GlobalUnlock(hmem);
		GlobalFree(hmem);
		return(FALSE);
	}
	writepos=_llseek(hfile,0,1);
   SkipBlock(hfile);
   readpos=_llseek(hfile,0,1);
	while (chunklen=_lread(hfile,data,4096)) {
      _llseek(hfile,writepos,0);
		if (_lwrite(hfile,data,chunklen)!=chunklen) goto delerr;
		readpos+=chunklen;
		writepos+=chunklen;
      _llseek(hfile,readpos,0);
	}
   _llseek(hfile,writepos,0);
   _lwrite(hfile,data,0);
   TapeInBlockPos=bp;
   TapeInFilePos=-1;
   GlobalUnlock(hmem);
   GlobalFree(hmem);
   return(TRUE);
}


BOOL CopyBlock(HFILE hFrom, char *szTo)
{
	HFILE hfile;
	HGLOBAL hmem;
   long chunklen,restlen;
	BYTE *data;

	restlen=BlockLen(hFrom)+2;
   if (restlen==0) return FALSE;
   hfile=_lopen(szTo,READ_WRITE);
   if (hfile==HFILE_ERROR) {
      hfile=_lcreat(szTo,0);
		if (hfile==HFILE_ERROR) return FALSE;
   }
	_llseek(hfile,0,2);
	hmem=GlobalAlloc(GHND,4096);
   if (!hmem) {
      _lclose(hfile);
		return(FALSE);
	}
	data=GlobalLock(hmem);
	do {
      chunklen=min(4096,restlen);
      restlen-=chunklen;
		if (_lread(hFrom,data,chunklen)!=chunklen) {
         merr:
         GlobalUnlock(hmem);
			GlobalFree(hmem);
         _lclose(hfile);
         return FALSE;
		}
      if (chunklen)
			if (_lwrite(hfile,data,chunklen)!=chunklen) goto merr;
   } while (restlen);
	_lclose(hfile);
	GlobalUnlock(hmem);
   GlobalFree(hmem);
	return TRUE;
}


BOOL MoveBlock(HFILE hfile, int from, int to)
// 'to' is position without 'from' erased
{
	int bp=TapeInBlockPos;
   long readpos,writepos,readpos2,blklen,restlen,chunklen;
   HGLOBAL hmem;
   BYTE *data;

   hmem=GlobalAlloc(GHND,4096);
	if (!hmem) return(FALSE);
	data=GlobalLock(hmem);
   TapeInBlockPos=from;
	TapeInFilePos=-1;
	WindTape(hfile);
   blklen=BlockLen(hfile)+2;
   readpos2=_llseek(hfile,0,1);        //position of 'from' block
   TapeInBlockPos=to;
   TapeInFilePos=-1;
	WindTape(hfile);
   restlen=_llseek(hfile,0,1);         //current position (start of 'to' blk)
   readpos=_llseek(hfile,0,2);         //length of file
   writepos=readpos+blklen;
	restlen=readpos-restlen;            //now length of everything after 'to'
   do {
      chunklen=min(4096,restlen);
		restlen-=chunklen;
		readpos-=chunklen;
      writepos-=chunklen;
      _llseek(hfile,readpos,0);
		if (_lread(hfile,data,chunklen)!=chunklen) {
         merr:
         GlobalUnlock(hmem);
			GlobalFree(hmem);
         return FALSE;
      }
		_llseek(hfile,writepos,0);
      if (chunklen)
			if (_lwrite(hfile,data,chunklen)!=chunklen) goto merr;
   } while (restlen);
   writepos=readpos;
	if (from>to) {
		readpos2+=blklen;
		from++;
   }
	if (bp>=to) bp++;
	do {
		chunklen=min(4096,blklen);
      blklen-=chunklen;
      _llseek(hfile,readpos2,0);
		if (_lread(hfile,data,chunklen)!=chunklen) goto merr;
      _llseek(hfile,writepos,0);
      if (chunklen)
			if (_lwrite(hfile,data,chunklen)!=chunklen) goto merr;
      readpos2+=chunklen;
      writepos+=chunklen;
	} while (blklen);
	GlobalUnlock(hmem);
	GlobalFree(hmem);
	TapeInBlockPos=bp;
	return DeleteBlock(hfile,from);
}

void resetmirroring()
{
	if (hMirBuffer) {
		GlobalUnlock(hMirBuffer);
		GlobalFree(hMirBuffer);
		hMirBuffer=NULL;
	}
	if (MirTempName[0]) {
		OpenDelete(MirTempName);
	}
	if ((!TapeOutMirror)||(!gszRecTapFile[0])) {
		miractive=FALSE;
		return;
	}
	miractive=TRUE;
	mirinned=0;
	mirbyte=0;
	mircurbit=7;
	mirbitcount=0L;
	mirbuflen=0;
	miridle=0;
}

void flushmirblock()
// called from emulator.c, either at 20ms frame end when miridle>1, or because
// of a mirbuffull message.  In first case, when no data is pending, a direct
// return is made.  Otherwise, the buffer is allocated or flushed.
{
	HFILE hfile;
	if (!miractive) return;
	if ((mirbitcount==0L)&&(miridle>1)) {
      miridle=0;
		return;
   }
   if (!hMirBuffer) {
		hMirBuffer=GlobalAlloc(GHND,4096);
		MirBuffer=GlobalLock(hMirBuffer);
	}
   if (mirbitcount==0L) {
      mirbuflen=4096;
      mirbufptr=MirBuffer;
      if (MirTempName[0]==0)
			GetTempFileName(0,"spec",0,MirTempName);
		MirFilePos=0;
      return;
	}
   // save block
   if (MirFilePos==0) {
      _lclose(_lcreat(MirTempName,0));
      MirFilePos=2;
   }
   hfile=_lopen(MirTempName,READ_WRITE);
   if (hfile==HFILE_ERROR) {
		resetmirroring();
		miractive=0;
		notify(ErrWritingMirTap);
      return;
   }
	_llseek(hfile,MirFilePos,0);
	_lwrite(hfile,MirBuffer,4096-mirbuflen);
   MirFilePos+=4096-mirbuflen;
   if (miridle>1) {
      // end block
		MirFilePos-=2;
		_lwrite(hfile,NULL,0);
      _llseek(hfile,0,0);
      _lwrite(hfile,&MirFilePos,2);
      _llseek(hfile,0,0);
		if (gszRecTapFile[0]==0) {
         apperr:
         _lclose(hfile);
         miractive=0;
         resetmirroring();
         notify(ErrAppMirTap);
			return;
		}
      if (!CopyBlock(hfile,gszRecTapFile)) goto apperr;
		_lclose(hfile);
      resetmirroring();
      if (hRecTapDialog)
         SendMessage(hRecTapDialog,WM_INITDIALOG,0,-1L);
      return;
   }
	_lclose(hfile);
   mirbufptr=MirBuffer;
   mirbuflen=4096;
}



BOOL PlayingSomething(void)
// Returns TRUE if playing back TZX or sample file, or playing back .TAP
// file in warajevo mode.  Used to find out whether to speed up execution
{
	if (InPlaying && inning) return TRUE;		// sample file
	if (TzxInPlaying && inning) return TRUE;
	if (waractive && (warleader<512)) return TRUE;	// wavajevo, 1st 256 edges delivered
	return FALSE;
}

