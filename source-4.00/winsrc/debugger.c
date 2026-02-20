#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "spectrum.h"
#include "helpmap.h"
#include "tokens.h"

// tedoen:
// - labels in disassembly (betere cursorbewegingen) (pgup/pgdn vooral)
// - commando label (welke refs, of welke definitie)
// - help file aanpassen, asm erin! (dump, asm, asmfile)

#pragma option -a1		// BYTE align

// Do not modify following structure -- matches structure defined in DEBUG.ASM:
// Trap types are:
// (1) PC==value
// (2) *(DWORD*)PC & (DWORD)mask == (DWORD) value		(i.e. trigger on certain opcodes)
// (3) *(WORD*)addr & (WORD)mask =?= (WORD) value     (i.e. memory change, for example)
// (>=4) operand & (WORD)mask =?= (WORD) value        (i.e. register in certain range for ex.)
// Operand can be AF, BC, ..., IX, IY, PC, SP, IR
// " =?= " is one of ==, !=, < and >
typedef struct {
	BYTE typ;
#define typ_end 		0
#define typ_breakpt 	1
#define typ_opcode	2
#define typ_memory	3
#define typ_af			4
#define typ_bc			5
#define typ_de			6
#define typ_hl			7
#define typ_afa		8
#define typ_bca		9
#define typ_dea		10
#define typ_hla		11
#define typ_ix			12
#define typ_iy			13
#define typ_pc			14
#define typ_sp			15
#define typ_ir			16
	BYTE flg;
#define flg_end	0x00	// these are not used for type==0
#define flg_and 	0x01
#define flg_temp  0x02	// temporary; deleted as soon as it's activated (execute instruction)
#define flg_dis	0x80	// (disabled; only used internally)
#define typ_equal 0x00	// these are not used for type==0 thru 2
#define typ_uneq	0x40
#define typ_less	0x80
#define typ_more	0xC0
	WORD handler;		// address of handler for type -- don't touch
	WORD mask;			// AND mask.  PC for type=1.
	WORD addr;			// mem addr for type=3. 2nd AND mask for type=2. Void for rest
	WORD val;			// value to which to compare.  Void for type=1
	WORD handler2;		// address of comparison handler.  2nd value for type=2. Void for type=1
} TRAP;

extern TRAP		*traplist;		// in DEBUG.ASM
TRAP				*inttraplist;	// internal trap list, including disabled ones
HGLOBAL			hTrapList=0,hIntTrapList=0;

const char reg_trans_table[]={1,0,2,3,13,14,4,5,22,21,15,16,17,18,19,20,25,26,23,24,6,7,8,9,11,10};

#pragma option -a.				// back to word/dword alignment

#define code_ysize 13
#define stack_ysize 19
#define data_ysize 5
#define data_xsize 38

#define data_data 0				// display data, hex and char
#define data_break 1				// display breakpoints

void InitTraplist(void);
char GetPar(void);
BOOL Interpret(char *);
BOOL ParseBreakpoint(void);
int  FindBreak(WORD);
void ClearTrapList(int);
void BreakPointString(char *, TRAP *);
void CopyTraplist(void);
char GetNPar(void);
int  assemble(BOOL);

HANDLE 	hDebugDialog;
HANDLE	hDebugInputWindow;	// WinMain routes ENTER keypresses for this window to hDebugDialog
FARPROC 	lpfnDebugProc;

WORD		wViewAddr,wViewEndAddr;	// Top and bottom address in code window (*End* set by UpdCode)
BOOL     bViewSkipLabel;         // Whether or not to display label on line #1, if any
BOOL     bViewLastLabel;         // TRUE if last line was a label
int		iUPPos;						// User ptr position number (set by UpdCode)
WORD		wUserPtr;					// Address of user pointer (usually == PC)
BOOL		bDisplayPC;					// TRUE means: copy PC to wUserPtr
BOOL		bDisplayType;				// TRUE if text, FALSE if disassembly (default)
WORD		wDataAddr;					// Address of data shown in data window
int		iDataType;					// What to display in data window?
BOOL		TimeTrap=FALSE;		   // if TRUE, then we should halt emulator at next update
int		iRecentDbgTrap=-1;		// number of trap that triggered & should be disp'd
BOOL     bAsmLine;               // TRUE if user input; FALSE if file input;
FILE     *asmfile;

int		CharX;					// size of character
int		CharY;
char		str[128];
HFONT		hFont;

#define _donotdefinewUserPtragain

#include "label.h"

#include "lexer.c"

#include "assemb.h"

void DebugWindow()
{
	if (hDebugDialog) return;
	lpfnDebugProc = MyMakeProcInstance(DbgDialProc,ghInstance);
	hDebugDialog = MyCreateDialogParam(ghInstance,"DEBUGWINDOW",hWndMain,lpfnDebugProc,CM_DEBUG);
}

void PrStr(HDC hDc, int x, int y, char *s)
{
	TextOut(hDc, x*CharX, y*CharY, s, strlen(s));
}

#define PrV(x,y,form,val) sprintf(str,form,val);PrStr(hdc,x,y,str)
#define PrS(x,y,s) PrStr(hdc,x,y,s)

void WriteAsmInfoString(char *str)
{
	char *p;
	int size;
	if (!hAsmInfoMemory) {
		hAsmInfoMemory=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 10);
	}
	p = GlobalLock(hAsmInfoMemory);
	size = strlen(p);
	GlobalUnlock(hAsmInfoMemory);
	hAsmInfoMemory = GlobalReAlloc(hAsmInfoMemory,size + strlen(str) + 10,GMEM_MOVEABLE);
	p = GlobalLock(hAsmInfoMemory);
	sprintf(p+size,"%s\r\n",str);
	GlobalUnlock(hAsmInfoMemory);
	if (hAsmInfoBox)
		SendMessage(hAsmInfoBox,WM_INITDIALOG,0,0);
}

BOOL CALLBACK AsmInfoBoxProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	char *p;
	switch (wMess) {
	case WM_INITDIALOG:
		if (hAsmInfoMemory) p=GlobalLock(hAsmInfoMemory); else p=&"";
		SendDlgItemMessage(hDlg,ASM_EDIT,WM_SETTEXT,0,(DWORD)p);
		PostMessage(GetDlgItem(hDlg,ASM_EDIT),EM_SETSEL,-1,0);
		if (hAsmInfoMemory)
			GlobalUnlock(hAsmInfoMemory);
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return FALSE;
	case WM_COMMAND:
		if (wPar==ASM_CLOSE)
			SendMessage(hDlg,WM_CLOSE,0,0);
		if (wPar==ASM_CLEAR) {
			if (hAsmInfoMemory) GlobalFree(hAsmInfoMemory);
			hAsmInfoMemory=NULL;
			SendMessage(hDlg,WM_INITDIALOG,0,0);
		}
		return TRUE;
	case WM_DESTROY:
		hAsmInfoBox=NULL;
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


void Advance1(void)
{
	if ((!bViewSkipLabel) && (FindAddrLabel(wViewAddr))) {
		/* First line is a label, not skipped, so skip */
		bViewSkipLabel = TRUE;
		return;
	}
	/* First line is the real text */
	wViewAddr += Disassemble(wViewAddr,bDisplayType,NULL);
	bViewSkipLabel = FALSE;
}


void MovePointer(WORD msg)
{
	WORD list[13];
	long curadr;
	int i=0;
	int back=1;
	WORD len;
	BOOL bskip;

	switch (msg) {
	case VK_DOWN:
		wUserPtr += Disassemble(wUserPtr,bDisplayType,NULL);
		if (wUserPtr == wViewEndAddr) {
			/* if new line starts with label, we scroll down two lines */
			if (FindAddrLabel(wUserPtr))
				Advance1();
			Advance1();
		}
		return;
	case VK_NEXT:
		wUserPtr = wViewAddr = wViewEndAddr;
		bViewSkipLabel = TRUE;
		for (i=0;i<(iUPPos>13?0:iUPPos);i++)
			Advance1();		/* wViewAddr */
		wUserPtr = wViewAddr;
		wViewAddr = wViewEndAddr;
		wViewEndAddr = wUserPtr;
		bViewSkipLabel = bViewLastLabel;;
		return;
	case VK_PRIOR:
		back=13;
	case VK_UP:
		curadr = ((long)wUserPtr) - 26*8;		// should be multiple of 8, at least 26*8
		bskip = FALSE;
		while (curadr < (long)wUserPtr) {
			if ((!bskip)&&(curadr == wViewAddr)&&(bViewSkipLabel==bskip)&&(back==13)) {
				wViewAddr = list[i];
				if (list[(i+12)%13]==list[i]) /* second line of label/text pair */
					bViewSkipLabel = TRUE;
				else
					bViewSkipLabel = FALSE;
			}
			list[i] = curadr;
			i++;
			i%=13;
			if ((!bskip)&&(FindAddrLabel(curadr)))
				bskip = TRUE;
			else {
				bskip = FALSE;
				curadr += (long)Disassemble(curadr,bDisplayType,NULL);
			}
		}
		wUserPtr = list[(i+13-back)%13];
		wViewEndAddr = wUserPtr;
	}
}

void Hilite(HDC hdc)
{
		SetTextColor(hdc,GetSysColor(COLOR_HIGHLIGHTTEXT));
		SetBkColor(hdc,GetSysColor(COLOR_HIGHLIGHT));
}

void Lolite(HDC hdc)
{
	SetBkColor(hdc,GetSysColor(MyBackgroundColor));
	SetTextColor(hdc,GetSysColor(MyForegroundColor));
}

void PrVX(HDC hdc, int x,int y,char *str)
{
	static char old[19][7];
	if (strncmp(str,old[y]+x,strlen(str))) {
		Hilite(hdc);
	}
	strncpy(old[y]+x,str,strlen(str));
	PrStr(hdc,x,y,str);
	Lolite(hdc);
}

void UpdRegs(HWND hWnd)
{
	HDC hdc=GetDC(hWnd);
	#define PrVx(x,y,form,val) {sprintf(str,form,val); PrVX(hdc,x,y,str);}
	Lolite(hdc);
	SelectObject(hdc,hFont);
	if (z80header.fa & 0x0100) PrVx(0,0,"C",0) else PrVx(0,0,".",0);
	if (z80header.fa & 0x4000) PrVx(1,0,"Z",0) else PrVx(1,0,".",0);
	if (z80header.fa & 0x8000) PrVx(2,0,"P",0) else PrVx(2,0,"M",0);
	if (z80header.fa & 0x0400) PrVx(4,0,"PE",0) else PrVx(4,0,"PO",0);
	PrS(0,1,"AF");PrVx(3,1,"%02x",z80header.fa & 0xFF); PrVx(5,1,"%02x",z80header.fa>>8);
	PrS(0,2,"BC");PrVx(3,2,"%04x",z80header.bc);
	PrS(0,3,"DE");PrVx(3,3,"%04x",z80header.de);
	PrS(0,4,"HL");PrVx(3,4,"%04x",z80header.hl);
	PrS(0,5,"AF'");PrVx(3,5,"%02x",z80header.faa & 0xFF); PrVx(5,5,"%02x",z80header.faa>>8);
	PrS(0,6,"BC'");PrVx(3,6,"%04x",z80header.bca);
	PrS(0,7,"DE'");PrVx(3,7,"%04x",z80header.dea);
	PrS(0,8,"HL'");PrVx(3,8,"%04x",z80header.hla);
	PrS(0,9,"IX");PrVx(3,9,"%04x",z80header.ix);
	PrS(0,10,"IY");PrVx(3,10,"%04x",z80header.iy);
	PrS(0,11,"PC");PrVx(3,11,"%04x",z80header.pc);
	PrS(0,12,"SP");PrVx(3,12,"%04x",z80header.sp);
	PrS(0,13,"IR");PrVx(3,13,"%02x",z80header.i);
						PrVx(5,13,"%02x",z80header.r&0x80 + (((unsigned char)(z80header.r-2))&0x7f));
	// time, R and bit 7 of R are updated in main window procedure
	PrS(0,14,"IM"); PrVx(3,14,"%1x",flg_im(z80header.flg));
	if (z80header.iff) PrVx(4,14,",EI",0) else PrVx(4,14,",DI",0);
//	if (z80header.iff2) PrS(4,12,"EI");
	PrS(0,15,"F");PrVx(2,15,"%05lx",soundtimehi);
	PrS(0,16,"T");PrVx(2,16,"%05lx",soundtimelo);
	if (hmode >= hm_128k) {
		PrVx(0,17,"R52%d ",state.hstate & 7);
		PrVx(5,17,"S%d",5+((state.hstate & 8)>>2));
	} else
		PrS(0,17,"       ");
	PrVx(0,18,"R:%s",romstring());
	ReleaseDC(hWnd,hdc);
}

void DisassembleToFile(WORD from, WORD to, FILE *f)
{
	char text[81];
	WORD len;
	while (from<=to) {
		len = Disassemble(from,bDisplayType,text);
		fprintf(f,"%s\n",text);
		from += len;
	}
}

void UpdCode(HWND hWnd)
{
	HDC hdc=GetDC(hWnd);
	WORD addr,len;
	int i;
	char text[code_ysize][81];
	WORD c;
	BOOL bUPShown=FALSE;

	Lolite(hdc);
	SelectObject(hdc,hFont);

	if (bDisplayPC) {
		wUserPtr = z80header.pc;
		bDisplayPC = FALSE;
	}
	if ((wViewEndAddr - wViewAddr > code_ysize*(bDisplayType?8:4))||
		 (wViewAddr>wUserPtr)||
		 (wViewEndAddr<wUserPtr)) {
		wViewAddr = wUserPtr;
		bViewSkipLabel = FALSE;
	}
	while (!bUPShown) {
		addr = wViewAddr;
		for (i=0;i<code_ysize;i++) {
			text[i][0]=text[i][1]=0;
			len = Disassemble(addr,bDisplayType,text[i]+2);
			if ((!i) && (!len) && (bViewSkipLabel)) {
				len = Disassemble(addr,bDisplayType,text[i]+2);
			}
			if ((addr == wUserPtr)&&(len)) {
				bUPShown=TRUE;
				text[i][6]='*';
				iUPPos=i;
			}
			if (len) {	// don't hilite labels
				if (addr == z80header.pc) {
					// hilite line
					text[i][0]=1;
					text[i][1]=1;
				}
				if (FindBreak(addr) != -1) {
					// this line has breakpoint; toggle hilite of addr
					text[i][0] ^= 1;
				}
			}
			addr += len;
		}
		bViewLastLabel = (len==0);
		wViewEndAddr = addr;
		if (!bUPShown) {
			wViewAddr = wUserPtr;
			bViewSkipLabel = FALSE;
		}
	}
	for (i=0;i<code_ysize;i++) {
		if (text[i][0])
			Hilite(hdc);
		if (text[i][2]==' ') { /* label */
			text[i][42]=' ';
			text[i][43]=0;
			PrStr(hdc,0,i,text[i]+2);
		} else {
			c=*(WORD*)(text[i]+6);
			text[i][6]=' ';
			text[i][7]=0;
			PrStr(hdc,0,i,text[i]+2);
			*(WORD*)(text[i]+6)=c;
			if (text[i][0] != text[i][1]) {
				if (text[i][1]) Hilite(hdc); else Lolite(hdc);
			}
			PrStr(hdc,4,i,text[i]+6);
			if (text[i][1])
				Lolite(hdc);
		}
	}
	ReleaseDC(hWnd,hdc);
}

void UpdStack(HWND hWnd)
{
	HDC hdc=GetDC(hWnd);
	int i;
	Lolite(hdc);
	SelectObject(hdc,hFont);

	for (i=0;i<stack_ysize;i++) {
		PrV(0,i,"%04x", z80header.sp+2*i);
		PrV(5,i,"%04x", (WORD)specpeek(z80header.sp+2*i) +
							 ((WORD)specpeek(z80header.sp+2*i+1)<<8) );
	}

	ReleaseDC(hWnd,hdc);
}

void UpdData(HWND hWnd)
{
	HDC hdc=GetDC(hWnd);
	int i,j;
	int num,inum;
	WORD w0;
	char text[256];
	char *s;
	char disabled;

	Lolite(hdc);
	SelectObject(hdc,hFont);

	switch (iDataType) {
	case data_data:
		w0 = wDataAddr;
		for (i=0;i<data_ysize;i++) {
			w0 += Disassemble(w0, TRUE, text);
			PrStr(hdc,0,i,text);
		}
		break;
	case data_break:
		j=0;
		num=inum=0;
		disabled=FALSE;
		if (hIntTrapList) {
			i=0;
			text[0]=0;
			s=text;
			while (inttraplist[i].typ != typ_end) {
				// first print number and 'disabled' or 'guilty' signs
				if (!text[0]) {
					if (inttraplist[i].flg & flg_dis) {
						sprintf(s,"!");
						disabled = TRUE;
					} else {
						disabled = FALSE;
						if (inttraplist[i].flg & flg_temp) {
							sprintf(s,"~");
						} else {
							sprintf(s," ");
						}
					}
					inum++;
					s++;
					sprintf(s,"%d:",inum);					// 1-based
					s += strlen(s);
				}
				// Update 'actual trap table' counter, set 'guilty' sign if opportune
				if (!disabled) {
					if (num == iRecentDbgTrap) {
						text[0]='*';
						iRecentDbgTrap = -1;
					}
					num++;
				}
				// Then add actual breakpoint expression
				BreakPointString(s, &inttraplist[i]);
				s+=strlen(s);
				// Finally, either add an & or print the expression
				if (inttraplist[i].flg & flg_and) {
					sprintf(s," & ");
					s+=3;
				} else {
					while (strlen(text)<data_xsize)
						sprintf(text+strlen(text),"    ");
					text[data_xsize]=0;							// chop to fit in window
					if (j<data_ysize)
						PrStr(hdc,0,j,text);
					j++;
					text[0]=0;
					s=text;
				}
				i++;
			}
		}
		for (i=0;i<data_xsize;i++)
			text[i]=' ';
		text[i]=0;
		while (j<data_ysize)
			PrStr(hdc,0,j++,text);
	}


	ReleaseDC(hWnd,hdc);
}

BOOL CALLBACK DbgDialProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	static BOOL updatemsg=FALSE;
	char inputstring[128];
	RECT rect;
	HBRUSH hBrush;
	HDC hDc;
	PAINTSTRUCT FAR *lpps;
	int i;

	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		SetWindowText(hDlg,"WinZ80 Debugger");
		hDebugInputWindow = GetDlgItem(hDlg,DBG_INPUT);
		CharX = LOWORD(GetDialogBaseUnits());
		CharY = HIWORD(GetDialogBaseUnits());
//		hFont = CreateFont(CharY,0,0,0,FW_NORMAL,0,0,0,OEM_CHARSET,OUT_DEFAULT_PRECIS,
//			CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE, "Courier");
		hFont = CreateFont(-(CharY*3)/4,0,0,0,0,0,0,0,0,0,0,DEFAULT_QUALITY,FIXED_PITCH,"Courier");
//		hFont = GetStockObject(ANSI_FIXED_FONT);
		SendMessage(hDlg,WM_UPDATE,0,0);
		updatemsg=TRUE;
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnDebugProc);
		hDebugDialog=NULL;
		DeleteObject(hFont);			// Don't delete stock objects
		break;
	case WM_ERASEBKGND:
		GetUpdateRect(hDlg,&rect,FALSE);
		hBrush=CreateSolidBrush(GetSysColor(MyBackgroundColor));
		hDc=GetDC(hDlg);
		FillRect(hDc,&rect,hBrush);
		ReleaseDC(hDlg,hDc);
		DeleteObject(hBrush);
		return TRUE;
	case WM_PAINT:
		BeginPaint(hDlg,lpps);
		updatemsg=TRUE;
		SendMessage(hDlg,WM_UPDATE,0,0);
		EndPaint(hDlg,lpps);
		return 0;
	case WM_UPDATE:
		getcurrenttime();		// Updates z80header.r and soundtime(hi/lo)
		z80header.r = (z80header.r & 0x7f) | (z80header.rrbit7 & 0x80);
		UpdRegs(GetDlgItem(hDlg,DBG_REGS));
		UpdCode(GetDlgItem(hDlg,DBG_CODE));
		UpdData(GetDlgItem(hDlg,DBG_DATA));
		UpdStack(GetDlgItem(hDlg,DBG_STACK));
		updatemsg=FALSE;
		return TRUE;
	case WM_USER+1:
		SetDlgItemText(hDlg,DBG_MSG,lPar);
		return TRUE;
	case WM_TWRAP:
		// 'Automatic' refresh.  Reset user pointer, and display type to disassembly
		bDisplayType = FALSE;
		bDisplayPC = TRUE;			// Signal to copy wUserPtr to z80header.pc
		if (!updatemsg) {
			updatemsg=TRUE;
			PostMessage(hDlg,WM_UPDATE,0,0);
		}
		if (TimeTrap) {							// we were exec'ing a time interval.
			TimeTrap=FALSE;						//  Interval done, so now stop.
			SetPauseState(1);
			SetDlgItemText(hDlg,DBG_MSG,"Timer triggered");
		} else {
			SetDlgItemText(hDlg,DBG_MSG,"");
		}
		return TRUE;
	case WM_TRAP:
		switch (wPar) {
		case msg_dihalt:
			SetDlgItemText(hDlg,DBG_MSG,"DI/HALT exception");
			break;
		case msg_trip:
			iRecentDbgTrap = trapoffset / sizeof(TRAP);
			// iRe..ap is 0-based, indexed into external trap list.  Cvtd by UpdData
			if (traplist[iRecentDbgTrap].flg & flg_temp) {
				sprintf (str,"Breakpoint for EXECUTE triggered");
				ClearTrapList(0);		// remove all temporary breakpoints
				TimeTrap = FALSE;		// also reset possible timer trap
			} else {
				sprintf (str,"User breakpoint triggered");
				TimeTrap = FALSE;		// reset timer trap, in case of "run n" or "singlestep"
			}
			SetDlgItemText(hDlg,DBG_MSG,str);
			break;
		}
		SetPauseState(TRUE);
		bDisplayType = FALSE;
		bDisplayPC = TRUE;			// Signal to copy wUserPtr to z80header.pc
		if (!updatemsg) {
			updatemsg=TRUE;
			PostMessage(hDlg,WM_UPDATE,0,0);
		}
		break;
	case WM_CHAR:
		switch (wPar) {
		case VK_RETURN:
			GetDlgItemText(hDlg,DBG_INPUT,inputstring,127);
			i=Interpret(inputstring);
			goto interpretit;
		case VK_EXECUTE:			 // (mis)used internally; not a virtual key.
			i=Interpret((char*)lPar);
			interpretit:
			if (!i) {				// result was OK: print intp'ed result hilited
				SendMessage(hDebugInputWindow,EM_SETSEL,0,MAKELONG(0,0xFFFF));
				SendMessage(hDebugInputWindow,EM_REPLACESEL,0,(DWORD)scan_str);
				SendMessage(hDebugInputWindow,EM_SETSEL,0,MAKELONG(0,0xFFFF));
			} else {
				// -n means: due to ASM, was correct, don't beep
				SendMessage(hDebugInputWindow,EM_SETSEL,0,MAKELONG((i<0?-i:i),0xFFFF));
				if (i>0) MessageBeep(-1);
			}
			if (!updatemsg) {
				updatemsg=TRUE;
				PostMessage(hDlg,WM_UPDATE,0,0);
			}
			return TRUE;
		case VK_UP:
		case VK_DOWN:
		case VK_PRIOR:
		case VK_NEXT:
			MovePointer(wPar);
			if (!updatemsg) {
				updatemsg=TRUE;
				PostMessage(hDlg,WM_UPDATE,0,0);
			}
			return TRUE;
		case VK_F1:
			SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"help");
			break;
		case VK_F2:
			i = FindBreak(wUserPtr);
			if (i==-1)
				SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"break pc=$");
			else {
				sprintf(inputstring,"clear %i",i+1);
				SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)inputstring);
			}
			break;
		case VK_F3:
			if (iDataType == data_data)
				SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"display");
			else
				SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"display hl");
			break;
		case VK_F4:
			SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"execute $");
			break;
		case VK_F5:
		case VK_F6:
		case VK_F10:
			SendMessage(hWndMain,WM_CHAR,wPar,lPar);
			break;
		case VK_F7:
			SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"singlestep");
			break;
		case VK_F8:
			SendMessage(hDlg,WM_CHAR,VK_EXECUTE,(long)"execute");
			break;
		case VK_F9:
			SendMessage(hDlg,WM_CHAR,VK_EXECUTE,state.paused ? (long)"run" : (long)"pause");
			break;
		}
		return FALSE;
	case WM_COMMAND:
		switch (wPar) {
		case DM_HELP:
			WinHelp(hWndMain,gszHelpFile,HELP_CONTEXT,cdebugc);
			return TRUE;
		case DM_QUIT:
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		case DM_ASSEMBLER:
			if (!hAsmInfoBox) {
				hAsmInfoBox = MyCreateDialogParam(ghInstance,"AsmInfo",hWndMain,AsmInfoBoxProc,WIN_AIB);
			}
		case DBG_INPUT:
			return TRUE;
		case DBG_DATA:
		case DBG_REGS:
		case DBG_STACK:
			return TRUE;
		}
		return FALSE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


// In the final version we need a procedure that translates breakpoint
//  expressions (like "(HL==#4000 AND DE>#8000 AND PC=#A345) or (PC==#1303)" )
//  together with 'local' breakpoints, to a traplist.  Following two functions
//  are a blunt first try

void CopyTraplist()
// copies internal traplist to actual one
{
	char disabled = FALSE;
	TRAP *t=inttraplist;
	int i=0;

	if (!hIntTrapList) return;
	if (hTrapList) {
		GlobalUnlock(hTrapList);
		GlobalFree(hTrapList);
	}
	hTrapList = GlobalAlloc(GMEM_MOVEABLE,sizeof(TRAP));
	if (!hTrapList) return;
	while (t->typ != typ_end) {
		if (!(t->flg & flg_and))
			disabled = (t->flg & flg_dis);
		if (!disabled) {
			GlobalReAlloc(hTrapList,(i+2)*sizeof(TRAP),GMEM_MOVEABLE);
			traplist=(TRAP*)GlobalLock(hTrapList);
			traplist[i] = *t;
			traplist[i].flg &= (~flg_dis);
			GlobalUnlock(hTrapList);
			i++;
		}
		t++;
	}
	traplist = (TRAP*)GlobalLock(hTrapList);
	traplist[i].typ = typ_end;
}


void InitTraplist()
{
	CopyTraplist();			// copies internal trap list to debugger trap list
	install_traplist();		// installs handlers; updates value of 'debugging'
	installsettings();		// tells emulator whether we have to debug or not
}


void ClearTrapList(int num)
{
	if (num==-1) {
		if (hIntTrapList) {
			GlobalUnlock(hIntTrapList);
			GlobalFree(hIntTrapList);
		}
		hIntTrapList = GlobalAlloc(GMEM_MOVEABLE,sizeof(TRAP));
		inttraplist = (TRAP*)GlobalLock(hIntTrapList);
		if (!inttraplist) return;
		inttraplist[0].typ = typ_end;
	} else {
		int i=1;
		TRAP *t=inttraplist;
		TRAP *out=inttraplist;
		BOOL copying=TRUE;
		// if num==0, copy all but temporary breaks.  Otherwise copy all but num.
		while (t->typ != typ_end) {
			if (i==num)
				copying=FALSE;
			if (copying) {
				if (num || (!(t->flg & flg_temp))) {
					*out=*t;
					out++;
				}
			}
			if (!(t->flg & flg_and)) {
				i++;
				copying = TRUE;
			}
			t++;
		}
		*out=*t;								// copy typ_end record
	}
	InitTraplist();
}

int FindBreak(WORD w)
{
// returns index of simple breakpoint to address w, or -1 if none found.
	int i=0;
	if (!hIntTrapList)
		ClearTrapList(-1);
	while (inttraplist[i].typ != typ_end) {
		if ((inttraplist[i].typ == typ_breakpt) &&
			 (inttraplist[i].flg == flg_end) &&
			 (inttraplist[i].mask == w))
			 return i;
		i++;
	}
	return -1;
}

int SetBreak(WORD w)
// adds a breakpoint to the current trap list
{
	int i;
	if (!hIntTrapList)
		ClearTrapList(-1);
	i=0;
	while (inttraplist[i].typ != typ_end)
		i++;
	GlobalUnlock(hIntTrapList);
	GlobalReAlloc(hIntTrapList,(i+2)*sizeof(TRAP),GMEM_MOVEABLE);
	inttraplist=(TRAP*)GlobalLock(hIntTrapList);
	if (!inttraplist) return -1;
	inttraplist[i+1].typ = typ_end;
	inttraplist[i].typ = typ_breakpt;
	inttraplist[i].flg = flg_end;
	inttraplist[i].mask = w;			// this is a 'mask' for all types other than breakpoint
	return i;
}

BOOL ParseBreakpoint()
// Reads tokens from input string and adds corresponding breakpoint(s) to
// InitTrapList.  Returns TRUE if succesful.  There need to be at least 1
// breakpoint
{
	int start0,start,i,tok,temptype,plusone;
	TRAP *t;
	DWORD arg1;

	if (!hIntTrapList)
		ClearTrapList(-1);
	for (start=0; inttraplist[start].typ != typ_end; start++);
	start0 = start;
	GlobalUnlock(hIntTrapList);
	GlobalReAlloc(hIntTrapList,(start+32)*sizeof(TRAP),GMEM_MOVEABLE);
	inttraplist=(TRAP*)GlobalLock(hIntTrapList);
	if (!inttraplist) return FALSE;
	i = start;
	do {
		t = inttraplist+i;
		scan_type = 0;
		t->flg = flg_end;										// no more exprs, by default
		tok = yylex();
		switch (tok) {
		case register_tok:									// breakpoint, or reg compare
			t->typ = typ_af+scan_reg;						// 0=f,1=a,&c instead of 0=af &c
			arg1 = 0xff;
			break;
		case regpair_tok:										// reg compare
			t->typ = typ_af+scan_reg;
			arg1 = 0xffffL;
			break;
		case br_open_tok:										// opcode, or mem compare
			temptype = scan_type;
			scan_type = 0;
			tok = yylex();
			switch (tok) {
			case regpair_tok:									// (pc), i.e. opcode
				if (scan_reg != 20 /* pc */ )
					goto pbp_exit;
				t->typ = typ_opcode;
				arg1 = 0xffffffffL;
				break;
			case value_tok:
				if (scan_type == 1 /* byte type */ )
					goto pbp_exit;
				t->typ = typ_memory;
				t->addr = scan_word;
				arg1 = 0xffffL;
				break;
			default:
				goto pbp_exit;
			}
			tok = yylex();
			if (tok != br_close_tok)
				goto pbp_exit;
			scan_type = temptype;
			break;
		default:
			goto pbp_exit;
		}
		tok = yylex();
		if (tok == and_tok) {
			tok = yylex();
			if (tok==value_tok)
				arg1 &= scan_word;
			else if (tok==dvalue_tok)
				arg1 &= scan_dword;
			else
				goto pbp_exit;
			tok = yylex();
		}
		if (tok != compare_tok)
			goto pbp_exit;
		if (scan_type == 1)
			arg1 &= 0xff;
		if (scan_type == 2)
			arg1 &= 0xffffL;
		if (GetPar()) goto pbp_exit;
		if (scan_type == 1)
			arg1 &= 0xff;
		if (scan_type == 2)
			arg1 &= 0xffffL;
		scan_word &= arg1;
		// Now the expression has been parsed.  Next: enter stuff in data structure
		plusone=1;
		if (t->typ == typ_opcode) {
			t->mask = arg1;
			t->addr = (arg1 >> 16);
			t->val = scan_dword;
			t->handler2 = (scan_dword >> 16);
		} else {
			if ((t->typ>=typ_af) && ((t->typ - typ_af)&1)) {		// 8-bit hi reg
				t->typ--;
				arg1 <<= 8;
				scan_word <<= 8;
				plusone=0x100;				// to make displayed equation logical
			}
			t->mask = arg1;
			t->val = scan_word;
		}
		// Convert to valid typ_xx values
		if (t->typ >= typ_af)
			t->typ = typ_af + (t->typ - typ_af)/2;
		// Include
		switch (scan_comp) {
		case 0:  t->typ |= typ_equal; break;
		case 1:	t->typ |= typ_uneq;  break;
		case 4:	t->val-=plusone;
		case 2:	t->typ |= typ_less;  break;
		case 5:	t->val+=plusone;
		case 3:  t->typ |= typ_more;  break;
		}
		// Now convert "PC=value" to breakpoint instead of ordinary reg compare
		if ((t->typ == typ_pc) && (t->mask == 0xffff)) {
			t->typ = typ_breakpt;
			t->mask = t->val;
		}
		// Finished
		t->flg |= flg_and;
		i++;
		inttraplist[i].typ = typ_end;
		tok = yylex();
	} while (tok == and_tok);
	inttraplist[i-1].flg &= ~flg_and;		// reset last AND bit
	if (tok == eof_tok)
		start = i;									// signal success
pbp_exit:
	inttraplist[start].typ = typ_end;
	GlobalUnlock(hIntTrapList);
	GlobalReAlloc(hIntTrapList,(start+1)*sizeof(TRAP),GMEM_MOVEABLE);
	inttraplist=(TRAP*)GlobalLock(hIntTrapList);
	InitTraplist();
	return (start0 != start);					// TRUE if something was added
}



void DisableBreakpoint(char what, int num)
// if what then enable else not.
{
	int i=1;
	TRAP *t=inttraplist;
	if (!hIntTrapList)
		return;
	while ((i != num)&&(t->typ != typ_end)) {
		if (!(t->flg & flg_and))
			i++;
		t++;
	}
	if ((t==inttraplist)&&(t->typ == typ_end)) return;
	do {
		if (t->typ != typ_end) {
			if (what)
				t->flg &= ~flg_dis;
			else
				t->flg |= flg_dis;
			t++;
		}
	} while (((t-1)->flg & flg_and)&&(t->typ != typ_end));
	InitTraplist();
}

void BreakPointString(char *s, TRAP *t)
{
	const char compstr[]="=\0x<>\0<=\0>=";
	const char regpstr[]="af\0xbc\0xde\0xhl\0xaf'\0bc'\0de'\0hl'\0ix\0xiy\0xpc\0xsp\0xir";
	const char reg8hstr[]="a\0xxb\0xxd\0xxh\0xxa'\0xb'\0xd'\0xh'\0xixh\0iyh\0pch\0sph\0i";
	const char reg8lstr[]="f\0xxc\0xxe\0xxl\0xxf'\0xc'\0xe'\0xl'\0xixl\0iyl\0pcl\0spl\0r";
	DWORD d1,d2;
	WORD w1,w2;
	BYTE b1;
	const char *str;

	b1 = (t->typ)&0x3f;
	if (b1 > 16) {
		sprintf(s,"-error-");
		return;
	}
	switch (b1) {
	case typ_end:
		sprintf(s,"-end-");
		break;
	case typ_breakpt:
		sprintf(s,"pc=%04x",t->mask);
		break;
	case typ_opcode:
		d1 = (((DWORD)t->addr)<<16) + t->mask;
		d2 = (((DWORD)t->handler2)<<16) + t->val;
		switch (d1) {
		case 0xFF:
			sprintf(s,"[pc]=%02x",(WORD)d2);
			break;
		case 0xFFFFL:
			sprintf(s,"[pc]=%04x",(WORD)d2);
			break;
		case 0xFFFFFFL:
			sprintf(s,"[pc]=%06lx",d2);
			break;
		case 0xFFFFFFFFL:
			sprintf(s,"[pc]=%08lx",d2);
			break;
		default:
			sprintf(s,"[pc]&%08lx=%08lx",d1,d2);
			break;
		}
		break;
	case typ_memory:
		#define oper (compstr+3*((t->typ)>>6))
		switch (t->mask) {
		case 0xFF:
			sprintf(s,"[%04x]%s%02x",t->addr,oper,t->val);
			break;
		case 0xFFFFL:
			sprintf(s,"[%04x]%s%04x",t->addr,oper,t->val);
			break;
		default:
			sprintf(s,"[%04x]&%04x%s%04x",t->addr,t->mask,oper,t->val);
			break;
		}
		break;
	default:
		b1-=typ_af;
		w1 = t->mask;
		w2 = t->val;
		str = regpstr;
		if (w1==0xFF)
			str = reg8lstr;
		if (w1==0xFF00L) {
			str = reg8hstr;
			w1 = 0xFF;
			w2 >>= 8;
		}
		switch (w1) {
		case 0xFF:
			sprintf(s,"%s%s%02x",str+4*b1,oper,w2);
			break;
		case 0xFFFFL:
			sprintf(s,"%s%s%04x",str+4*b1,oper,w2);
			break;
		default:
			sprintf(s,"%s&%04x%s%04x",str+4*b1,w1,oper,w2);
			break;
		}
		break;
	}
}



void SetPauseState(BOOL pause)
{
	if (state.paused != pause) {
		state.paused = pause;
		if (hCtrlDialog) {
			SendMessage(hCtrlDialog,WM_INITDIALOG,0,0);
		}
		UpdateWinZ80Caption();
	}
}


// spultsies voor de assembler

void warningerror(char *w,char *s)
{
	// tempcurpc is set to CUR_PC (==wUserPtr) at beginning of assembly
	// of each instruction.  Var is def'd in asm.y
	char str[256];
	if (bAsmLine) {
		sprintf(str,"%04x %s %s",tempcurpc,w,s);
	} else {
		sprintf(str,"%04x [%u] %s %s",tempcurpc,zzlineno,w,s);
	}
	WriteAsmInfoString(str);
	if (bAsmLine) {
		SendMessage(hDebugDialog,WM_USER+1,0,(long)str);
	}
}

void error(char *s)
{
	warningerror("Error:",s);
}

void warning(char *s)
{
	warningerror("Warning:",s);
}

void zzerror(char *s)
{
	error(s);
}

char GetNPar()
{
	int tok;
	tok = yylex();
	switch (tok) {
		case value_tok:
			return 0;
		case eof_tok:
			return -1;
	}
	return 1;
}


char GetPar()
// returns 0 if succesful.  Value retrieved is in scan_word.  Type, if known,
// is in scan_type (0=none, 1=byte, 2=word)
// returns 1 if syntax error; returns -1 if no parameter found
{
	int tok;
	char t;

	tok = yylex();
	switch (tok) {
		case value_tok:
		case dvalue_tok:
			return 0;
		case br_open_tok:
			t = scan_type;									// store output type
			scan_type = 0;
			if (GetPar()) return 1;
			if (scan_type == 1) return 1;				// byte-values are not addresses
			scan_word = specpeek(scan_word) + ((WORD)specpeek(scan_word+1)<<8);
			scan_type = t;									// reset output type
			tok = yylex();
			return (tok != br_close_tok);
		case regpair_tok:
			if (scan_type == 1) return 1;				// "byte hl" makes no sense
			scan_word = z80header.a[reg_trans_table[scan_reg]] +
							(((WORD)z80header.a[reg_trans_table[scan_reg+1]])<<8);
			scan_type = 2;
			return 0;
		case register_tok:
			if (scan_type == 2) return 1;
			scan_word = (BYTE)z80header.a[reg_trans_table[scan_reg]];
			scan_type = 1;
			return 0;
		case eof_tok:
			return -1;
		default:
			return 1;
	}
}


int Interpret(char *string)
{
	 // Interprets user input using yylex() from lexer.c.
	 static YY_BUFFER_STATE yybuf=NULL;
	 // zzbuf is used in case asm_tok below
	 YY_BUFFER_STATE zzbuf;
	 int token,t2;

	 int i,j;
	 WORD par;
	 WORD w1,w2;
	 BYTE b1;
	 char taper;
	 char c1,c2;
	 char label[100];

	 #define klaar {int dummy=yylex(); if (dummy != eof_tok) return scan_index;}

	 if (yybuf)
		yy_delete_buffer(yybuf);
	 yybuf = yy_scan_string((const char *)string);    // initialise & alloc buffer
	 scan_type = 0;
	 scan_index = 0;
	 scan_str[0] = 0;
	 token = yylex();                                // get first token
	 switch (token) {
		  case pause_tok:
				klaar;
				SetPauseState(1);
				break;
		  case run_tok:
				// run [nn], run for nn T states, or indefinetely if no time given
				i = GetNPar();
				klaar;
				if (i) {
					 SetPauseState(0);
				} else {
					 TimeTrap = TRUE;
					 SetTGlobal(scan_dword);
					 SetPauseState(0);
				}
				break;
		  case break_tok:
				if (!ParseBreakpoint())
					return scan_index;
				break;
		  case clear_tok:
				i = GetNPar();
				if (i==-1) {	// no parameter
					ClearTrapList(-1);
				} else {
					klaar;
					ClearTrapList(scan_word);
				}
				break;
		  case singlestep_tok:
				klaar;
				SetTGlobal(1);             // execute single instruction
				TimeTrap = TRUE;     		// stop emulator at next update,
				SetPauseState(0);       	// and go
				break;
		  case execute_tok:
				// Execute next instruction -- if it's a call then follow it through.
				// If an argument is given, execute till address == argument.
				i = GetPar();
				klaar;
				if (i!=-1) {
					i = SetBreak(scan_word);
					if (i==-1) return scan_index;
					inttraplist[i].flg |= flg_temp;
					InitTraplist();
				} else {
					b1 = specpeek(z80header.pc);
					if ((b1==0xDD)||(b1==0xFD))
						 b1 = specpeek(z80header.pc+1);
					i = (int)(char)specpeek(z80header.pc+1);
					w1 = (BYTE)i + (specpeek(z80header.pc+2)<<8);
					if ((b1==0xC3)||(b1==0x18)||(b1==0xE9)||
						  ((b1>=0x10)&&(b1<=0x38)&&((b1&7)==0)&&(i>0))||
						  ((b1>=0xC0)&&((b1&7)==2)&&(w1>z80header.pc))||
						  (b1==0xC9)||
						  ((b1>=0xC0)&&((b1&7)==0))) {
						 // simply do single step with JP nn, JR nn, JP HL/IX/IY,
						 // and JP cc forward, JR cc forward or DJNZ forward,
						 // RET or RET cc
						 SetTGlobal(1);
						 TimeTrap = TRUE;
					} else {
						 i = Disassemble(z80header.pc,FALSE,NULL);
						 i = SetBreak(z80header.pc + i);
						 if (i==-1) return scan_index;
						 inttraplist[i].flg |= flg_temp;
						 InitTraplist();
					}
				}
				SetPauseState(0);
				break;
		  case quit_tok:
				klaar;
				PostMessage(hDebugDialog,WM_COMMAND,DM_QUIT,0);
				break;
		  case nmi_tok:
				klaar;
				Nmi();
				break;
		  case reset_tok:
				klaar;
				Reset();
				break;
//		  case trace_tok:
//				klaar;
//				if (!state.logging) {
//					 InitLogging();
//				} else {
//					 QuitLogging();
//				}
//				break;
		  case type_tok:
				klaar;
				bDisplayType = !bDisplayType;
				break;
		  case display_tok:
				i = GetPar();
				if (i == -1) {		// no parameter
					iDataType = data_break;
				} else if (i == 0) {
					 klaar;
					 iDataType = data_data;
					 wDataAddr = scan_word;
				} else
					 return scan_index;
				break;
		  case view_tok:
				if (GetPar())
					return scan_index;
				klaar;
				wUserPtr = scan_word;
				break;
		  case help_tok:
				klaar;
				PostMessage(hDebugDialog, WM_COMMAND, DM_HELP, 0);
				break;
		  case disable_tok:				// tokens ON and OFF
		  case enable_tok:
				i = GetNPar();
				if (i) return scan_index;
				klaar;
				 DisableBreakpoint(token == enable_tok, scan_word);
				// num of trap is 1-based, indexed into internal list
				break;
		  case di_tok:
		  case ei_tok:
				klaar;
				z80header.iff = (token == ei_tok);
				break;
		  case im0_tok:
				klaar;
				z80header.flg = 0;
				break;
		  case im1_tok:
				klaar;
				z80header.flg = 1;
				break;
		  case im2_tok:
				klaar;
				z80header.flg = 2;
				break;
		  case ld_tok:
				token = yylex();
				switch (token){
				case register_tok:
					i = 1;		// byte;
					break;
				case regpair_tok:
					i = 2;		// word;
					break;
				case br_open_tok:
					i = scan_type;
					if (GetPar()) return scan_index;
					par = scan_word;
					if (yylex() != br_close_tok) return scan_index;
					break;
				default:
					return scan_index;
				}
				if (yylex() != comma_tok) return scan_index;
				if (token == br_open_tok) {				 // poke (sequence of) bytes/words
					if (!i) i=1;								 // default to bytes
					scan_type = i;								 // to disallow reg(pair)s
					j = GetPar();
					while (j==0) {
						specpoke(par++, scan_word & 0xff);
						if (i==2) {
							specpoke(par++, scan_word >> 8);
						}
						scan_type = i;								 // to disallow reg(pair)s
						j = GetPar();
					}
					if (j==1) return scan_index;
				} else {											 // set register (pair)
					j = scan_reg;									 // GetPar may overwrite scan_reg
					if (GetPar()) return scan_index;
					klaar;
					if ((scan_type)&&(scan_type != i)) return scan_index;
					if ((i==1)&&(scan_word & 0xff00L)) return scan_index;
					z80header.a[reg_trans_table[j]] = scan_word & 0xff;
					if (i==2)
						z80header.a[reg_trans_table[j+1]] = (scan_word >> 8);
					// copy r reg value in header to relevant run-time vars
					*((BYTE *)&rreg) = z80header.r;
					z80header.rrbit7 = z80header.r;
				}
				break;
		  case asm_tok:
				bAsmLine=TRUE;
				zzbuf = zz_scan_string(string + scan_index);
				i=assemble(FALSE);
				zz_delete_buffer(zzbuf);
				if (i) return scan_index; else return -scan_index;
				break;
		  case asmfile_tok:
				bAsmLine=TRUE;
				while (string[scan_index]==' ') scan_index++;
				asmfile = fopen(string + scan_index, "r");
				if (asmfile == NULL) {
					sprintf(str,"Could not open file '%s'",string+scan_index);
					error(str);
					break;
				}
				bAsmLine=FALSE;
				zzlineno=1;
				strupr(string+scan_index);
				if (strstr(string+scan_index,".SYM")) {
					sprintf(str,"Loading TAPER format label file '%s':",string+scan_index);
					taper=1;
				} else {
					sprintf(str,"Assembling file '%s':",string+scan_index);
					taper=0;
				}
				WriteAsmInfoString(str);
				if (!taper) {
					zzbuf = zz_create_buffer(asmfile,64);
					zz_switch_to_buffer(zzbuf);
					i=assemble(TRUE);
					zz_delete_buffer(zzbuf);
				} else {
					char err;
					while (!feof(asmfile)) {
						err=0;
						w1=0xffff;
						fscanf(asmfile,"%s",&label);
						if (label[0]=='$')
							sscanf(label+1,"%x",&w1);
						else
							sscanf(label,"%u",&w1);
						fscanf(asmfile,"%s%c",&label,&c1);
						if (c1 != '\n') {
							sprintf(str,"Error (line %d, label '%s'): Extra characters on line",zzlineno,label);
							WriteAsmInfoString(str);
							err=1;
							while (c1 != '\n') fscanf(asmfile,"%c",&c1);
						}
						sprintf(str,"%s: EQUA #%04x",label,w1);
						zzbuf = zz_scan_string(str);
						assemble(FALSE);
						zz_delete_buffer(zzbuf);
						zzlineno++;
					}
				}
				bAsmLine=TRUE;
				fclose(asmfile);
				scan_str[0]=0;
				scan_index=0;
				return 0;
		  case savelabels_tok:
				while (string[scan_index]==' ') scan_index++;
				asmfile = fopen(string + scan_index, "w");
				if (asmfile == NULL) {
					sprintf(str,"Could not open file '%s' for writing",string+scan_index);
					bAsmLine=TRUE;
					error(str);
					break;
				}
				strupr(string + scan_index);
				WriteLabelsToFile(asmfile,!!strstr(string + scan_index,".SYM"));
				scan_str[0]=0;
				fclose(asmfile);
				return 0;
		  case dump_tok:
				if (GetPar()) return scan_index;
				w1 = scan_word;
				token = yylex();
				if (token != comma_tok) return scan_index;
				if (GetPar()) return scan_index;
				w2 = scan_word;
				if (w1>=w2) return scan_index;
				token = yylex();
				if (token != comma_tok) return scan_index;
				while (string[scan_index]==' ') scan_index++;
				asmfile = fopen(string + scan_index, "w");
				if (asmfile == NULL) {
					sprintf(str,"Could not open file '%s' for writing",string+scan_index);
					bAsmLine=TRUE;
					error(str);
					break;
				}
				DisassembleToFile(w1,w2,asmfile);
//				strcat(scan_str,string+scan_index);
				scan_str[0]=0;
				fclose(asmfile);
				return 0;
		  default:
				return scan_index;
	}
	return 0;
}

int assemble(BOOL prepare)
{
	int i;
	int errors=0;
	if (prepare) PrepareForAssembly();
	do {
		tempcurpc = wUserPtr;		// temporary solution
		i=zzparse();
		if (i || asm_err) {
			errors++;
		}
	} while ((errors<10)&&(!asm_eof));
	if (errors>=10)
		error("Too many errors - stop.");
	if (prepare) WarnForUndefinedLabels();
	return (i || errors);
}


void LoadLabels()
{
	FILE *lblfile = fopen(gszLabelFile, "r");
	YY_BUFFER_STATE zzbuf;
	if (lblfile == NULL) {
		sprintf(str,"Could not open file '%s' for standard labels -- check WinZ80.ini entry LabelFile.",gszLabelFile);
		return;
	}
	bAsmLine=FALSE;
	sprintf(str,"Loading label file '%s':",gszLabelFile);
	WriteAsmInfoString(str);
	zzbuf = zz_create_buffer(lblfile,64);
	zz_switch_to_buffer(zzbuf);
	zzlineno=1;
	assemble(FALSE);
	zz_delete_buffer(zzbuf);
	fclose(lblfile);
}


