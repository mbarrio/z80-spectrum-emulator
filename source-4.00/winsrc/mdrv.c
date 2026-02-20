#include <windows.h>
#include <stdio.h>
#include "spectrum.h"

// tedoen: als rs232 input wijzer veranderd wordt, getal inlezen.

// first some RS232 variables

WORD	rs232indata;		// contains rs232 bits to be sent
WORD	rs232outdata;		// contains rs232 bits collected
BYTE	dtr=0;				// signals 'go send rs232 byte' to Spectrum
char	rsoutfile[128];
char	rsinfile[128];
long	rsoutpos;
long	rsinpos;
long	rsinlen;
BOOL	rsincrlf;			// crlf -> cr conversion on input
BOOL	rsoutcrlf;			// cr -> crlf conversion on output
BOOL	rsoutkeyw;			// keyword conversion on output

// microdrive variables

char		cart1[128];			// name of cartridge 1 file
char		cart2[128];
char		readwrite1;
char		readwrite2;

char		*mdrvfile;
BOOL		header;				// header/datablock toggle
int		blocknum;
int		gapcounter;			// counter to toggle gap & wait
BOOL		gap;					// if TRUE, current block is a gap
BOOL		writeprotect;		// TRUE if cartridge is write protected
BOOL		commsdat;
BOOL		commsclk;
BOOL		readwrite;
BOOL		erase;
BOOL		cts;
BYTE		oldcontrol;
BYTE		motors;
BOOL		bControlFlushes;	// if TRUE, an out to mdrv_control flushes mdrv buf
BOOL		byteswritten;		// if TRUE, buffer must be written to disk
BOOL		bytesread;		 	// if TRUE, new buffer must be read from disk
int		mdrvpointer;		// 0-based for writing, 1-based for reading
BYTE		mdrvbuf[0x220];

void mdrvflushbuf(void);
void newmotor(void);
int  getrsbyte(void);
void sendrsbyte(BYTE);


BYTE in_if1(BYTE addr)
{
	BYTE x;
	int i;
	switch (addr & 0x18) {
		case 0x10:
			if (rs232indata <= 1) {
				rs232indata = 0;
				if (cts != 0) {
					i = getrsbyte();
					if (i != -1) {
						rs232indata = 0x800F | ((~i)<<4);		// marker, 3 stop, 4 start
					}
				}
			}
			x = (rs232indata & 1)<<7;
			rs232indata>>=1;
			return x;
		case 0x08:
			if (!mdrvfile) {
				return (dtr ? 14 : 6);
			}
			gapcounter++;
			if (gap && (gapcounter<256))
				x=4;
			else
				x=(gapcounter>>2)&4;
			x |= (gapcounter>>2)&2;
			if (!writeprotect)
				x |= 1;
			if (dtr)
				x |= 8;
			return x;
		default:				// case 00
			if (!mdrvfile) {
				// if1 crash
				return 0;
			}
			x=0xFF;							// rubbish
			if (!mdrvpointer) return x;
			if (!readwrite) return x;
			if (!erase) return x;
			bytesread = TRUE;
			bControlFlushes = TRUE;
			if (gap) return x;
			x = mdrvbuf[mdrvpointer-1];
			mdrvpointer += (mdrvpointer < 0x21f);
			return x;
	}
}

void out_if1(BYTE addr, BYTE val)
{
	BOOL cc;
	switch (addr & 0x18) {
		case 0x10:
			if (!commsdat)		// network
				return;
			rs232outdata = (rs232outdata >> 1) | ((val&1)<<8);
			if (rs232outdata & 1) {
				sendrsbyte(~rs232outdata >> 1);
				rs232outdata = 0;
			}
			return;
		case 0x08:
			if (((oldcontrol ^ val)&0x0F) || bControlFlushes) {
				mdrvflushbuf();
				bControlFlushes = FALSE;
			}
			oldcontrol = val;
			cc = commsclk;
			commsclk = (val & 2);
			commsdat = (val & 1);
			readwrite = (val & 4);
			erase = (val & 8);
			cts = (val & 16);
			rs232outdata = 0;
			if ((commsclk == 0) && cc) {		// downgoing edge
				motors = (motors<<1) | commsdat;
				newmotor();
				mdrvflushbuf();
			}
			return;
		default:		// case 0x00
			if (!mdrvfile) {
				// if1 crash
				return;
			}
			if (!byteswritten) {
				byteswritten = TRUE;
				mdrvpointer = 0;
			}
			if (readwrite)			// i.e. reading
				return;
			if (erase)				// i.e. not erasing
				return;
			mdrvbuf[mdrvpointer] = val;
			mdrvpointer += (mdrvpointer < 0x21f);
			return;
	}
}

void mdrvflushbuf(void)
{
	HANDLE mdrvh;
	int i,j;
	if (!mdrvfile) {
		gap=TRUE;
		gapcounter=0;
		mdrvpointer = 1;
		return;
	}
	if (readwrite) {		// reading
		if (!bytesread && (mdrvpointer==1)) return;		// still valid data in buffer
		// increase block number
		if (bytesread) {
			header = !header;
			if (header) {
				blocknum++;
				if (blocknum >= 254)
					blocknum=0;
			}
		}
		mdrvpointer = 1;
		bytesread = FALSE;
		mdrvh = _lopen(mdrvfile,READ);
		if (!mdrvh) {
			mdrvfile = NULL;
			notify(MDRFileError);
			gap = TRUE;
			gapcounter=0;
			return;
		}
		_llseek(mdrvh,543L*(long)blocknum + (header?0:15),0);
		if (header)
			i=15;
		else
			i=528;
		if (_lread(mdrvh,mdrvbuf,i) != i) {
			_lclose(mdrvh);
			notify(MDRFileError);
			gap = TRUE;
			mdrvfile = NULL;
			return;
		}
		_lclose(mdrvh);
		j=0;
		for (i=0;i<14;i++)
			j = (j+(BYTE)mdrvbuf[i]) % 255;
		gap = j-(BYTE)mdrvbuf[14];
		gapcounter=0;
	} else {
		if (!mdrvpointer) return;		// nothing to write
		if (mdrvpointer > 0x200)
			header=FALSE;
		mdrvpointer = 0;
		gap = FALSE;
		if (writeprotect) return;
		if (!byteswritten) return;
		byteswritten = FALSE;
		mdrvh = _lopen(mdrvfile,READ_WRITE);
		if (!mdrvh) {
			mdrvfile = NULL;
			notify(MDRFileWriteError);
			return;
		}
		_llseek(mdrvh,543L*(long)blocknum + (header?0:15),0);
		if (header)
			i=15;
		else
			i=528;
		if (_lwrite(mdrvh,mdrvbuf+12,i) != i) {
			_lclose(mdrvh);
			mdrvfile = NULL;
			notify(MDRFileWriteError);
			return;
		}
		_llseek(mdrvh,137923L,0);
		_lclose(mdrvh);
		header = !header;
		if (header) {
			blocknum++;
			if (blocknum >= 254)
				blocknum=0;
		}
	}
}


void newmotor(void)
{
	int i;
	BYTE m=motors;
	if (m==0xFF) {
		mdrvfile = NULL;
		return;
	}
	i = 0;
	while (m & 1) {
		i++;
		m = (m>>1)|0x80;
	}
	if (m != 0xFE) {		// more than 1 motor turned on
		mdrvfile = NULL;
		return;
	}
	bytesread = TRUE;		// to force loading of new sector
	switch (i) {
		case 0:
			mdrvfile = cart1;
			writeprotect = readwrite1;
			break;
		case 1:
			mdrvfile = cart2;
			writeprotect = readwrite2;
			break;
		default:
			mdrvfile = NULL;
	}
	if (mdrvfile && (!mdrvfile[0])) {
		mdrvfile = NULL;
		return;
	}
}





BOOL CALLBACK MdrvDialogProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	HANDLE h;
	int i;
	char wp;
	char name[128];
	switch (wMess) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,MDR_FILENAME1,cart1[0]?cart1:"-none-");
		SetDlgItemText(hDlg,MDR_FILENAME2,cart2[0]?cart2:"-none-");
		SetDlgItemText(hDlg,MDR_OPEN1,cart1[0]?"Close":"Open");
		SetDlgItemText(hDlg,MDR_OPEN2,cart2[0]?"Close":"Open");
		CheckDlgButton(hDlg,MDR_READONLY1,cart1[0]&&readwrite1);
		CheckDlgButton(hDlg,MDR_READONLY2,cart2[0]&&readwrite2);
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		hMdrvDialog=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnMdrvDialog);
		break;
	case WM_USER+1:		// open .mdr file through main menu
		wPar = MDR_OPEN1;
		for (i=0;((char*)lPar)[i];i++)
			name[i] = ((char*)lPar)[i];
		goto do_open;
	case WM_COMMAND:
		switch (wPar) {
		case MDR_OPEN1:
			if (cart1[0]) {
				cart1[0]=0;
				SendMessage(hDlg,WM_INITDIALOG,0,0);
				return TRUE;
			}
			goto openmdrfile;
		case MDR_OPEN2:
			if (cart2[0]) {
				cart2[0]=0;
				SendMessage(hDlg,WM_INITDIALOG,0,0);
				return TRUE;
			}
			openmdrfile:
			name[0]=0;
			i = FileDlg(4,name,TRUE,hDlg);
			if (i==-1) return TRUE;		// cancel
			do_open:
			h = _lopen(name,READ);
			if (h==HFILE_ERROR) {
				// make new one
				h = _lcreat(name,0);
				if (h==HFILE_ERROR) {
					mdrwrerr:
					notify(MDRFileCreateError);
					return TRUE;
				}
				_llseek(h,137922L,0);
				wp=0;
				if (_lwrite(h,&wp,1) != 1) {
					_lclose(h);
					goto mdrwrerr;
				}
				if (_lclose(h)) goto mdrwrerr;
				h = _lopen(name,READ);
				if (h==HFILE_ERROR) goto mdrwrerr;
				quicknotify("UNFORMATTED cartridge file created.");
			}
			_llseek(h,137922L,0);
			if (_lread(h,&wp,1) != 1) {
				notify(MDRFileError);
				return TRUE;
			}
			switch (wPar) {
				case MDR_OPEN1:
					for (i=0;i<128;i++) cart1[i]=name[i];
					readwrite1 = wp;
					break;
				case MDR_OPEN2:
					for (i=0;i<128;i++) cart2[i]=name[i];
					readwrite2 = wp;
					break;
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case MDR_READONLY1:
		case MDR_READONLY2:
			i = IsDlgButtonChecked(hDlg,wPar);
			if (wPar == MDR_READONLY1) {
				if (!cart1[0]) return TRUE;
				h = _lopen(cart1,READ_WRITE);
			} else {
				if (!cart2[0]) return TRUE;
				h = _lopen(cart2,READ_WRITE);
			}
			if (!h) {
				mdrwperr:
				notify(MDRFileWriteError);
				_lclose(h);
				SendMessage(hDlg,WM_INITDIALOG,0,0);
				return TRUE;
			}
			_llseek(h,137922L,0);
			if (_lwrite(h,&i,1) != 1)
				goto mdrwperr;
			_lclose(h);
			if (wPar == MDR_READONLY1)
				readwrite1 = i;
			else
				readwrite2 = i;
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		}
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


BOOL CALLBACK RsDialogProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	HANDLE h;
	int i;
	BOOL b;
	WORD w;
	char name[128];
	switch (wMess) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg,RS_INCRLF,rsincrlf);
		if (rsinfile[0]) {
			SetDlgItemText(hDlg,RS_INFILE,rsinfile);
			SetDlgItemText(hDlg,RS_INOPEN,"Close");
			SendMessage(hDlg,WM_USER+1,0,0);				// rsinlen
			SendMessage(hDlg,WM_USER+2,0,0);				// rsinpos;
		} else {
			SetDlgItemText(hDlg,RS_INFILE,"-none-");
			SetDlgItemText(hDlg,RS_INOPEN,"Open");
			SetDlgItemText(hDlg,RS_INBYTES,"0");
			SetDlgItemText(hDlg,RS_INMAXBYTES,"bytes (of 0)");
		}
		CheckDlgButton(hDlg,RS_OUTCRLF,rsoutcrlf);
		CheckDlgButton(hDlg,RS_OUTKEYW,rsoutkeyw);
		if (rsoutfile[0]) {
			SetDlgItemText(hDlg,RS_OUTFILE,rsoutfile);
			SetDlgItemText(hDlg,RS_OUTOPEN,"Close");
			SendMessage(hDlg,WM_USER+3,0,0);				// rsoutpos;
		} else {
			SetDlgItemText(hDlg,RS_OUTFILE,"-none-");
			SetDlgItemText(hDlg,RS_OUTOPEN,"Open");
			SetDlgItemText(hDlg,RS_OUTBYTES,"0 bytes");
		}
		break;
	case WM_USER+1:
		sprintf(name,"bytes (of %lu)",rsinlen);
		SetDlgItemText(hDlg,RS_INMAXBYTES,name);
		return TRUE;
	case WM_USER+2:
		sprintf(name,"%lu",rsinpos);
		SetDlgItemText(hDlg,RS_INBYTES,name);
		return TRUE;
	case WM_USER+3:
		sprintf(name,"%lu bytes",rsoutpos);
		SetDlgItemText(hDlg,RS_OUTBYTES,name);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		hRsDialog=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnRsDialog);
		break;
	case WM_USER+4:		// open .mdr file through main menu
		for (i=0;((char*)lPar)[i];i++)
			name[i] = ((char*)lPar)[i];
		goto rs_open;
	case WM_COMMAND:
		switch (wPar) {
		case RS_INOPEN:
			if (rsinfile[0]) {
				rsinfile[0]=0;
				SendMessage(hDlg,WM_INITDIALOG,0,0);
				return TRUE;
			}
			name[0]=0;
			i = FileDlg(5,name,TRUE,hDlg);
			if (i==-1) return TRUE;	// cancel
			rs_open:
			h = _lopen(name,READ);
			if (!h) {
				notify(RSInErr);
				return TRUE;
			}
			rsinlen = _llseek(h,0,2);
			rsinpos = 0;
			for (i=0;name[i];i++)
				rsinfile[i]=name[i];
			_lclose(h);
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case RS_OUTOPEN:
			if (rsoutfile[0]) {
				rsoutfile[0]=0;
				SendMessage(hDlg,WM_INITDIALOG,0,0);
				dtr = FALSE;
				return TRUE;
			}
			name[0]=0;
			i = FileDlg(5,name,FALSE,hDlg);
			if (i==-1) return TRUE;	// cancel
			h = _lcreat(name,0);
			if (!h) {
				notify(RSOutErr);
				return TRUE;
			}
			rsoutpos = 0;
			for (i=0;name[i];i++)
				rsoutfile[i]=name[i];
			_lclose(h);
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			dtr = TRUE;
			return TRUE;
		case RS_INCRLF:
			rsincrlf = IsDlgButtonChecked(hDlg,wPar);
			return TRUE;
		case RS_OUTCRLF:
			rsoutcrlf = IsDlgButtonChecked(hDlg,wPar);
			return TRUE;
		case RS_OUTKEYW:
			rsoutkeyw = IsDlgButtonChecked(hDlg,wPar);
			return TRUE;
		case RS_INBYTES:
			if (HIWORD(lPar)==EN_CHANGE) {
				w = GetDlgItemInt(hDlg, RS_INBYTES, &b, 0);
				if (b) rsinpos=min(w,rsinlen);
			}
			return TRUE;
		}
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


int getrsbyte()			// returns byte to send to RS input chan; -1 if none available
{
	HANDLE h;
	static int crlftrans=0;
	BOOL cont=FALSE;
	BYTE b;
	if (!rsinfile[0])
		return -1;
	if (rsinpos == rsinlen)
		return -1;
	h = _lopen(rsinfile,READ);
	if (!h) {
		notify (RSInErr);
		return -1;
	}
	_llseek(h,rsinpos,0);
	do {
		if (rsinpos == rsinlen)
			return -1;
		rsinpos++;
		if (_lread(h,&b,1) != 1) {
			notify (RSInErr);
			_lclose(h);
			rsinfile[0]=0;
			return -1;
		}
		if (rsincrlf) {
			if ((b==0x0D)||(b==0x0A)) {
				if (crlftrans>b)
					cont=TRUE;			// let first 0d/0a go as 0d, but skip an 0a after 0d
				crlftrans = b;
				b = 0x0D;
			} else {
				crlftrans = 0;
				cont = FALSE;
			}
		}
	} while (cont);
	_lclose(h);
	if (hRsDialog) SendMessage(hRsDialog,WM_USER+2,0,0);
	return b;
}

void sendrsbyte(BYTE b)
{
	HANDLE h;
	static BOOL senttoken=FALSE;
	char string[20],*keyw;
	int len=1;
	if (!rsoutfile[0])
		return;
	h = _lopen(rsoutfile,WRITE);
	if (!h) {
		notify (RSOutErr);
		dtr = FALSE;
		return;
	}
	// now do some translation
	string[0]=b;
	if (rsoutcrlf && (b==0x0d)) {
		string[1]=0x0a;
		len=2;
	}
	if (rsoutkeyw && (b>164)) {
		int i=0;
		if (senttoken || (b<202))
			len=0;
		else
			string[0]=' ';
		keyw = GlobalLock(hKeyword);
		senttoken = (b>167);					// true if trailing space needed
		while (b != 165)
			for (b--;~keyw[i++]&0x80;) {}
		do {
			string[len++]=keyw[i]&0x7F;
		} while (~keyw[i++]&0x80);
		if (((string[len-1]>='A') || (string[len-1]=='$')) && senttoken)
			string[len++]=' ';
		senttoken=TRUE;
		GlobalUnlock(hKeyword);
	} else
		senttoken = FALSE;
	// then send it away
	rsoutpos = _llseek(h,0,2)+len;
	if (_lwrite(h,string,len) != len) {
		_lclose(h);
		notify(RSOutErr);
		rsoutfile[0]=0;
		dtr = FALSE;
	} else {
		if (hRsDialog) SendMessage(hRsDialog,WM_USER+3,0,0);
	}
	_lclose(h);
	return;
}

