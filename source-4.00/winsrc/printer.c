#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "spectrum.h"

char zxprinter;
char multicolumn;
char szPrinter[128];
char *szDevice;
char *szDriver;
char *szOutput;
HDC hPrinter;
HBITMAP hPrinterBitmap;
struct {
  BITMAPINFOHEADER header;
  RGBQUAD colors[2];
  char bitmap[32];
} bmi;
int printerypos;
int printerysize;
int printerdotwidth;
int printerdotheight;
int printerleftmargin;
int printercolumn;
int printercolskip;
int printerpagecount;
int printerhorz;
int printerhscale;
int printervscale;
BOOL printerabort;
int zxpcount,zxpmotor;
char zxpline[256];

void initprinter(void)
{
   GetProfileString("Windows","device","",szPrinter,128);
   szDevice=strtok(szPrinter,",");
   szDriver=strtok(NULL,",");
   szOutput=strtok(NULL,",");
   hPrinter=NULL;
   zxpcount=-4;
   zxpmotor=0;
   ActivatePrinterMenu();
}


void ActivatePrinterMenu(void)
{
   char temp[100];
   if (hPrinter) {
      sprintf(temp,"Print pages (%d)",printerpagecount);
      ModifyMenu(GetMenu(hWndMain),CM_FORMFEED,
         MF_BYCOMMAND|MF_ENABLED|MF_STRING,CM_FORMFEED,temp);
   } else {
      ModifyMenu(GetMenu(hWndMain),CM_FORMFEED,
         MF_BYCOMMAND|MF_DISABLED|MF_GRAYED|MF_STRING,CM_FORMFEED,"Print pages");
   }
}


HDC printerDC(void)
{
   DOCINFO di;

   if (hPrinter) return hPrinter;
   hPrinter=CreateDC(szDriver,szDevice,szOutput,NULL);
   if (hPrinter==NULL) goto printererr;
   bmi.header.biSize = sizeof(BITMAPINFOHEADER);
   bmi.header.biWidth = 256;
   bmi.header.biHeight = 1;
   bmi.header.biPlanes = 1;
   bmi.header.biBitCount = 1;
   bmi.header.biCompression = BI_RGB;
   bmi.header.biSizeImage = 32;
   bmi.header.biClrUsed = 0;
   bmi.header.biClrImportant = 0;
   bmi.colors[0].rgbBlue = 255;
   bmi.colors[0].rgbRed = 255;
   bmi.colors[0].rgbGreen = 255;
   bmi.colors[0].rgbReserved = 0;
   bmi.colors[1].rgbBlue = 0;
   bmi.colors[1].rgbRed = 0;
   bmi.colors[1].rgbGreen = 0;
   bmi.colors[1].rgbReserved = 0;
   hPrinterBitmap = CreateDIBitmap(hPrinter,(LPBITMAPINFO)&bmi,0,NULL,(BITMAPINFO FAR*)NULL,0);
   if (!hPrinterBitmap) goto printererr;
   di.cbSize=sizeof(DOCINFO);
   di.lpszDocName="Spectrum emulator: ZX-printer";
   di.lpszOutput=NULL;
   printerpagecount=1;
   printerdotwidth=max(
      (GetDeviceCaps(hPrinter,LOGPIXELSX)*(LONG)printerhscale)/(3200L),1);
   printerdotheight=max(
      (GetDeviceCaps(hPrinter,LOGPIXELSY)*(LONG)printervscale)/(3200L),1);
   printerysize=GetDeviceCaps(hPrinter,VERTRES);
   printerhorz=GetDeviceCaps(hPrinter,HORZRES)-printerleftmargin;
   printercolskip=printerhorz;
   if (multicolumn) {
      int cols;
      cols = printerhorz/(256*printerdotwidth);
      if (cols>1) {
         printercolskip = 256*printerdotwidth +
            (printerhorz - cols*256*printerdotwidth)/(cols-1);
      }
   }
   // 3200 = 256 pixels * 100 (scale factor) / 8 inches (width A4 paper)
   if (StartDoc(hPrinter,&di)>0) {
      StartPage(hPrinter);
      printerypos=0;
      printercolumn=0;
      printerpagecount=1;
      ActivatePrinterMenu();
      return hPrinter;
   } else {
      printererr:
      ToggleMenu(hWndMain,CM_ZXPRINTER,&zxprinter);
      if (hPrinter) DeleteDC(hPrinter);
      hPrinter=NULL;
      notify(PrinterError);
      return hPrinter;
   }
}

BOOL CALLBACK AbortDlg(HWND hDlg, WORD wMsg, WORD wPar, LONG lPar)
{
	if (wMsg==WM_CLOSE) {
		DestroyWindow(hDlg);
		return 0;
	}
   if (wMsg==WM_COMMAND) {
      printerabort=TRUE;
      return (TRUE);
   }
   return FALSE;
}

BOOL AbortProc(HDC hdc, int error)
{
   MSG msg;
   while (!printerabort && PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
      DispatchMessage(&msg);
   }
   return (!printerabort);
}

void quitprinter(void)
{
   FARPROC lpfnAbortProc,lpfnDlgProc;
   HWND hAbortDlg;

   if (!hPrinter) return;
	EndPage(hPrinter);
   printerabort=FALSE;
	lpfnDlgProc=MyMakeProcInstance((FARPROC)AbortDlg,ghInstance);
	hAbortDlg=CreateDialog(ghInstance,"PrintAbort",hWndMain,lpfnDlgProc);
	lpfnAbortProc=MakeProcInstance((FARPROC)AbortProc,ghInstance);
	SetAbortProc(ghInstance,lpfnAbortProc);
	EndDoc(hPrinter);
	DeleteDC(hPrinter);
	DestroyWindow(hAbortDlg);
	MyFreeProcInstance(lpfnDlgProc);
	FreeProcInstance(lpfnAbortProc);
	hPrinter=NULL;
   ActivatePrinterMenu();
   return;
}

void printzxline(char *line)
{
   int i;
   printerDC();
   if (printercolumn>=printerhorz) {
      EndPage(hPrinter);
      StartPage(hPrinter);
		printercolumn=0;
      printerpagecount++;
      ActivatePrinterMenu();
   }
   if (!hPrinter) return;
   for (i=0;i<256;i++) {
      if ((i&7)==0) bmi.bitmap[i/8]=0;
      if (line[i]) bmi.bitmap[i/8]|=(0x80>>(i&7));
   }
   StretchDIBits(hPrinter,
      printerleftmargin+printercolumn,
      printerypos,
      256*printerdotwidth,
      printerdotheight,
      0,0,256,1,bmi.bitmap,(LPBITMAPINFO)&bmi,DIB_RGB_COLORS,SRCCOPY);
   printerypos+=printerdotheight;
   if (printerypos>=printerysize) {
      printerypos=0;
      printercolumn += printercolskip;
   }
}

void formfeed(void)
{
   if (!hPrinter) return;
   quitprinter();
}

void endline(void)
{
   if (printerypos+8*printerdotheight > printerysize) {
      printerypos=0;
      printercolumn += printercolskip;
      if (printercolumn>=printerhorz) {
         EndPage(hPrinter);
         StartPage(hPrinter);
         printercolumn=0;
         printerpagecount++;
         ActivatePrinterMenu();
      }
   }
}

