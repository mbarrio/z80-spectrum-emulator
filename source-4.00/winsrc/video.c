#include <windows.h>
#include <mem.h>
#include <stdio.h>
#include "spectrum.h"
#include "wing.h"
#include "gd.h"

#include "mywing.c"

#define QuickDraw FALSE
#define StripHeight 4         // Must be divisor of 24
#define DebugVideo FALSE

const char SpecColor[32][3]={
 {0,0,0},{0,0,40},{55,0,0},{57,0,45},{0,53,0},{0,53,53},{52,52,0},{50,50,50},
 {0,0,0},{0,0,43},{60,0,0},{63,0,55},{0,60,0},{0,63,63},{63,63,0},{63,63,63},
 {0,0,0},{10,10,10},{16,16,16},{23,23,23},{30,30,30},{37,37,37},{44,44,44},{50,50,50},
 {0,0,0},{10,10,10},{17,17,17},{26,26,26},{34,34,34},{44,44,44},{53,53,53},{63,63,63}};

HDC gmemdc;
BORPART borpart[4],hborpart[4];
VIDEOBUF *videobuf;
HGLOBAL hvideobuf;
HGLOBAL hVidCopperBuf;
HGLOBAL h2x2bits;    // handle for storage area for 2x2 enlargement
BYTE *bp2x2bits;     // global pointer, only locked during video refresh
int Own2x2Video;     // 0 if StretchBitBlt is used for 2x2 enlargement
HPALETTE hPalette;	// handle to logical palette for Spectrum colors
HBITMAP hBlockletStripBitmap;		// Bitmap to paste blocklet-strip to scrn
HBITMAP h2x2BlockletStripBitmap; // Special bitmap used for 2x2 enlargement only
HBITMAP hHorizBorderStripBitmap; // Bitmap for upper/lower part of border
LONG specpalentries[16];	// Bitmap entries (hardware pal regs) for Spec. colors
COLORREF bordercolor[8];
char memallocated=FALSE;
int bpp;                   // # of bits per pixel (on plane); 1, 4, 8 or 24
int tbpp;                  // total # of bits per pixel (4, 8 or 24, never 1)
int curflashcount;

// video save stuff
char bSaveGif;				// true, then video code should save GIF file
char gszGifFile[128];	// name of GIF file

// WinG stuff
HDC hWinGDC;					// handle of WinG DC, created only once because of alleged overhead
BYTE far* WinGbitmapbits;	// actual bitmap
BYTE far* WinG2x2bitmapbits;
HBITMAP hWinGoldbitmap;		// default bitmap

void initborparts(BORPART *bp,int borsize)
{
	bp[0].xsize=256+2*borsize;
	bp[0].ysize=borsize;
	bp[3].xsize=256+2*borsize;
	bp[3].ysize=borsize;
	bp[0].xpos=0;
	bp[0].ypos=0;
	bp[3].xpos=0;
	bp[3].ypos=192+borsize;
	bp[1].xsize=bp[2].xsize=borsize;
	bp[1].ysize=bp[2].ysize=192;
	bp[1].xpos=0;
	bp[2].xpos=256+borsize;
	bp[1].ypos=bp[2].ypos=borsize;
}

void initborsizes()
{
	int i;
	HDC compdc;
	HGLOBAL hmem;
	BITMAPINFO *bmi;

	compdc=GetDC(hWndMain);
	initborparts(borpart,display.borsize);
	initborparts(hborpart,hdisplay.borsize);
	if (gWinG) {
		hmem = GlobalAlloc(GPTR,sizeof(BITMAPINFO)+256*sizeof(RGBQUAD));
		if (!hmem) {fatalerror(FatalMemAlloc);return;}
		bmi=(BITMAPINFO*)GlobalLock(hmem);
		myWinGRecommendDIBFormat(bmi);
		myWinGGetDIBColorTable(hWinGDC,0,256,bmi->bmiColors);
		bmi->bmiHeader.biSize=(char*)&(bmi->bmiColors)-(char*)&(bmi->bmiHeader);
		bmi->bmiHeader.biCompression=BI_RGB;
		bmi->bmiHeader.biClrUsed=256;
	}
	for (i=0;i<4;i++) {
		borpart[i].colour=border&7;
		borpart[i].touched=TRUE;
		if (borpart[i].hactbitmap) {
			DeleteObject(borpart[i].hactbitmap);
		}
		if (!gWinG) {
			borpart[i].hactbitmap=
				CreateCompatibleBitmap(compdc,borpart[i].xsize,borpart[i].ysize);
			if (borpart[i].hborbitmap) {
				GlobalUnlock(borpart[i].hborbitmap);
				GlobalFree(borpart[i].hborbitmap);
			}
			borpart[i].hborbitmap=GlobalAlloc(GPTR,
				(borpart[i].xsize/8)*borpart[i].ysize*tbpp+1);
			if (borpart[i].hborbitmap) {
				borpart[i].borbitmap=(BYTE far*)GlobalLock(borpart[i].hborbitmap);
			} else {
				fatalerror(FatalMemAlloc);
				return;
			}
		} else {
			bmi->bmiHeader.biWidth=borpart[i].xsize;
			bmi->bmiHeader.biHeight=-borpart[i].ysize;		// Note: always use top-down DIBs: I'm lazy
			borpart[i].hactbitmap= myWinGCreateBitmap(hWinGDC,bmi,NULL);
			borpart[i].borbitmap = myWinGGetDIBPointer(borpart[i].hactbitmap,NULL);
		}
	}
	ReleaseDC(hWndMain,compdc);
}

// following routine taken from WinG documentation, with bugfix: (GL)

HPALETTE CreateIdentityPalette(LOGPALETTE *lp)
{
	int i;
	struct {
		WORD Version;
		WORD NumberOfEntries;
		PALETTEENTRY aEntries[256];
	} Palette =
	{
		0x300,
		256
	};

	//*** Just use the screen DC where we need it
	HDC hdc = GetDC(hWndMain);

	// First get current palette
	GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

	//*** For SYSPAL_NOSTATIC, just copy the color table into
	//*** a PALETTEENTRY array and replace the first and last entries
	//*** with black and white
	if (GetSystemPaletteUse(hdc) == SYSPAL_NOSTATIC)

	{
		//*** Fill in the palette with the given values, marking each
		//*** as PC_NOCOLLAPSE
		for(i = 0; i < lp->palNumEntries; i++)
		{
			Palette.aEntries[i].peRed = lp->palPalEntry[i].peRed;
			Palette.aEntries[i].peGreen = lp->palPalEntry[i].peGreen;
			Palette.aEntries[i].peBlue = lp->palPalEntry[i].peBlue;
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
		}

		//*** Mark any unused entries PC_NOCOLLAPSE
		for (; i < 256; ++i)
		{
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;

		}

		//*** Make sure the last entry is white
		//*** This may replace an entry in the array!
		Palette.aEntries[255].peRed = 255;
		Palette.aEntries[255].peGreen = 255;
		Palette.aEntries[255].peBlue = 255;
		Palette.aEntries[255].peFlags = 0;

		//*** And the first is black
		//*** This may replace an entry in the array!
		Palette.aEntries[0].peRed = 0;
		Palette.aEntries[0].peGreen = 0;
		Palette.aEntries[0].peBlue = 0;
		Palette.aEntries[0].peFlags = 0;

	}
	else
	//*** For SYSPAL_STATIC, get the twenty static colors into
	//*** the array, then fill in the empty spaces with the
	//*** given color table
	{
		int nStaticColors;
		int nUsableColors;

		//*** Get the static colors from the system palette
		nStaticColors = GetDeviceCaps(hdc, NUMCOLORS);
		if (nStaticColors > 20)
			nStaticColors = 20;
//		GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

		//*** Set the peFlags of the lower static colors to zero
		nStaticColors = nStaticColors / 2;

		for (i=0; i<nStaticColors; i++)
			Palette.aEntries[i].peFlags = 0;

		//*** Fill in the entries from the given color table
//		nUsableColors = lp->palNumEntries - nStaticColors;
		nUsableColors = min(256 - nStaticColors, lp->palNumEntries);		// GL mod
		for (; i<nUsableColors; i++)
		{
			Palette.aEntries[i].peRed = lp->palPalEntry[i].peRed;
			Palette.aEntries[i].peGreen = lp->palPalEntry[i].peGreen;
			Palette.aEntries[i].peBlue = lp->palPalEntry[i].peBlue;
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
		}

		//*** Mark any empty entries as PC_NOCOLLAPSE

		for (; i<256 - nStaticColors; i++)
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;

		//*** Set the peFlags of the upper static colors to zero
		for (i = 256 - nStaticColors; i<256; i++)
			Palette.aEntries[i].peFlags = 0;
	}

	//*** Remember to release the DC!
	ReleaseDC(hWndMain, hdc);

	//*** Return the palette
	return CreatePalette((LOGPALETTE *)&Palette);
}




int init_video(void)
{
	LOGPALETTE FAR* logpal;
	BITMAPINFO FAR* bmi;
	HBITMAP hbitmap;
	BITMAP bitmapinfo;
	HGLOBAL hmem;
	HDC compdc;
	char string[100];
	char bitmapinit[256];   		// For some reason size 16 gives GP fault
	char bitmapbits[4*16];
	int i,j;

	bSaveGif = FALSE;
	InitWinGLib();
	compdc=GetDC(hWndMain);
	hmem = GlobalAlloc(GPTR,sizeof(LOGPALETTE)+256*sizeof(PALETTEENTRY));		// GL mod
	if (!hmem) {fatalerror(FatalMemAlloc);return(1);}
	logpal=(LOGPALETTE*)GlobalLock(hmem);
	logpal->palVersion = 0x300;
	logpal->palNumEntries = 26;															// GL mod
	GetSystemPaletteEntries(compdc, 0, 256, logpal->palPalEntry);
	j = -10 + 16*state.blackandwhite;
	for (i=10;i<26;i++) {																	// GL mod
		COLORREF color=//GetNearestColor(compdc, 										// GL mod
			RGB(SpecColor[i+j][0]*4,SpecColor[i+j][1]*4,SpecColor[i+j][2]*4);
		logpal->palPalEntry[i].peRed = color & 0xFF;
		logpal->palPalEntry[i].peGreen = (color>>8) & 0xFF;
		logpal->palPalEntry[i].peBlue = (color>>16) & 0xFF;
		logpal->palPalEntry[i].peFlags = NULL;
	}
	hPalette=CreateIdentityPalette(logpal);
	GlobalUnlock(hmem);
	GlobalFree(hmem);
	if (!hPalette) {fatalerror(FatalCreatePal);return(1);}
	SelectPalette(compdc,hPalette,FALSE);
	RealizePalette(compdc);
	j = 16*state.blackandwhite;
	for (i=0;i<8;i++) bordercolor[i]=
		PALETTERGB(SpecColor[i+j][0]*4,SpecColor[i+j][1]*4,SpecColor[i+j][2]*4);

	hmem = GlobalAlloc(GPTR,sizeof(BITMAPINFO)+256*sizeof(RGBQUAD));
	if (!hmem) {fatalerror(FatalMemAlloc);return(1);}
	bmi=(BITMAPINFO*)GlobalLock(hmem);
	// If using WinG, then 1. assume the recommended DIB format is 8 bpp, 2. fill
	// in all values directly, 3. create WinGDC and a 32 x 8*StripHeight WinGDIB.
	if (gWinG) {
		  WriteInfoString("Using WinG for graphics");
		  hWinGDC = myWinGCreateDC();
		  myWinGRecommendDIBFormat(bmi);
		if ((bmi->bmiHeader.biBitCount != 8)||(!hWinGDC)) {
			gWinG = FALSE;
			if (hWinGDC)
				DeleteDC(hWinGDC);
			WriteInfoString("Eror initialising WinG; using ordinary bitblt");
		} else {
			PALETTEENTRY aPalette[256];
			RGBQUAD aPaletteRGB[256];
			bpp=8;
			tbpp=8;
			for (i=0;i<16;i++)
				specpalentries[i]=i+10;
			bmi->bmiHeader.biSize=(char*)&(bmi->bmiColors)-(char*)&(bmi->bmiHeader);
			bmi->bmiHeader.biWidth=32;
			bmi->bmiHeader.biHeight=-(8*StripHeight);		// Note: always use top-down DIBs: I'm lazy
			bmi->bmiHeader.biCompression=BI_RGB;
			bmi->bmiHeader.biClrUsed=256;
			hBlockletStripBitmap = myWinGCreateBitmap(hWinGDC,bmi,NULL);
			hWinGoldbitmap = SelectObject(hWinGDC,hBlockletStripBitmap);
			GetPaletteEntries(hPalette, 0, 256, aPalette);
			for (i=0; i<256; ++i) {
				aPaletteRGB[i].rgbRed = aPalette[i].peRed;
				aPaletteRGB[i].rgbGreen = aPalette[i].peGreen;
				aPaletteRGB[i].rgbBlue = aPalette[i].peBlue;
				aPaletteRGB[i].rgbReserved = 0;
			}
			myWinGSetDIBColorTable(hWinGDC, 0, 256, aPaletteRGB);
			WinGbitmapbits = myWinGGetDIBPointer(hBlockletStripBitmap,NULL);

			if (Own2x2Video) {
//				h2x2bits=GlobalAlloc(GHND,(64/8)*16*StripHeight*tbpp);
//				if (!h2x2bits) {fatalerror(FatalMemAlloc);return(1);}
				bmi->bmiHeader.biWidth=64;
				bmi->bmiHeader.biHeight=-(16*StripHeight);
					 h2x2BlockletStripBitmap = myWinGCreateBitmap(hWinGDC,bmi,NULL);
				SelectObject(hWinGDC, h2x2BlockletStripBitmap);
					 myWinGSetDIBColorTable(hWinGDC, 0, 256, aPaletteRGB);
					 WinG2x2bitmapbits = myWinGGetDIBPointer(h2x2BlockletStripBitmap,NULL);
			}
		}
	}

	if (!gWinG) {
	// now setup DIB bitmap with all 16 Spectrum colors in it, and select it into
	// the main window's DC.  This 1. collapses the 16 colors onto the actually
	// available palette colors (when there are less than 32 colors available, e.g.
	// on 4 bpp displays), and 2. codes the colors into 'hardware' form, i.e. using
	// the number of bpp that the physical video memory uses.
	// Logical palette color coding is used (see above)
		bmi->bmiHeader.biSize=(char*)&(bmi->bmiColors)-(char*)&(bmi->bmiHeader);
		bmi->bmiHeader.biWidth=16;
		bmi->bmiHeader.biHeight=1;
		bmi->bmiHeader.biPlanes=1;
		bmi->bmiHeader.biBitCount=8;
		bmi->bmiHeader.biCompression=BI_RGB;
		bmi->bmiHeader.biSizeImage=0;
		bmi->bmiHeader.biXPelsPerMeter=0;
		bmi->bmiHeader.biYPelsPerMeter=0;
		bmi->bmiHeader.biClrUsed=16;
		bmi->bmiHeader.biClrImportant=16;
		j = 16*state.blackandwhite;
		for (i=0;i<16;i++) {
			bmi->bmiColors[i].rgbRed=SpecColor[i+j][0]*4;
			bmi->bmiColors[i].rgbGreen=SpecColor[i+j][1]*4;
			bmi->bmiColors[i].rgbBlue=SpecColor[i+j][2]*4;
//			((int*)&(bmi->bmiColors[0].rgbRed))[i]=i+10;
		}
		for (i=0;i<16;i++) {
			bitmapinit[i]=i;
		}
		SelectPalette(compdc,hPalette,FALSE);
		RealizePalette(compdc);
		hbitmap=CreateDIBitmap(compdc,
									  &(bmi->bmiHeader),
									  CBM_INIT,
									  bitmapinit,
									  bmi,
									  DIB_RGB_COLORS);
//									  DIB_PAL_COLORS);
		GetObject(hbitmap,sizeof(BITMAP),&bitmapinfo);
		GetBitmapBits(hbitmap,4*16,bitmapbits);
		bpp=bitmapinfo.bmBitsPixel;
		tbpp=bpp * bitmapinfo.bmPlanes;
		if ((tbpp!=4)&&(bpp!=8)&&(bpp!=24)&&(bpp!=16)&&(bpp!=32))
			{fatalerror(FatalDisplayNotSupp); return(1); }
		sprintf(string,"Using bitmaps of %d bits per pixel",tbpp);
		WriteInfoString(string);
		for (i=0;i<16;i++) {
			if (bpp==1) {
				LONG l=0;
				int s=(23-i)&15;
				if ((((WORD *)bitmapbits)[0]>>s)&1) l=0xFFL;
				if ((((WORD *)bitmapbits)[1]>>s)&1) l|=0xFF00L;
				if ((((WORD *)bitmapbits)[2]>>s)&1) l|=0xFF0000L;
				if ((((WORD *)bitmapbits)[3]>>s)&1) l|=0xFF000000L;
				specpalentries[i]=l;
			}
			if (bpp==4) specpalentries[i]=(BYTE)(bitmapbits[i/2]>>(i&1 ? 4 : 0))&0x0F;
			if (bpp==8) specpalentries[i]=(BYTE)bitmapbits[i];
			if (bpp==16) specpalentries[i]=((WORD*)bitmapbits)[i];
			if (bpp==24) specpalentries[i]=(*(LONG*)(&bitmapbits[3*i])) & 0xFFFFFFL;
			if (bpp==32) specpalentries[i]=((LONG*)bitmapbits)[i];
		}
		hBlockletStripBitmap=CreateCompatibleBitmap(compdc,32,8*StripHeight);
		if (Own2x2Video) {
			h2x2bits=GlobalAlloc(GHND,(64/8)*16*StripHeight*tbpp);
			if (!h2x2bits) {fatalerror(FatalMemAlloc);return(1);}
			h2x2BlockletStripBitmap=CreateCompatibleBitmap(compdc,64,16*StripHeight);
		}
		DeleteObject(hbitmap);
	}

//	// This works, but I haven't got any idea why.
//	if (bpp==4)
//		for (i=0;i<8;i++) bordercolor[i]=GetNearestColor(compdc,bordercolor[i]);

	ReleaseDC(hWndMain,compdc);

	for (i=0;i<4;i++) borpart[i].hborbitmap=NULL;
	initborsizes();
	hvideobuf=GlobalAlloc(GHND,sizeof(VIDEOBUF));
	if (!hvideobuf) {fatalerror(FatalMemAlloc);return(1);}
	videobuf=(VIDEOBUF*)GlobalLock(hvideobuf);
	for (i=0;i<8;i++) {
		videobuf->hbitmaps[i]=GlobalAlloc(GHND,(32/8)*192*tbpp);
		if (!videobuf->hbitmaps[i]) {fatalerror(FatalMemAlloc);return(1);}
		videobuf->bitmaps[i]=(BYTE*)GlobalLock(videobuf->hbitmaps[i]);
	}
	for (i=0;i<192;i++) {
		videobuf->touched[i]=0;
		videobuf->visible[i]=1;
	}

	for (i=0;i<256+128;i++) {
		if (bpp==1) {
			int fg,bg,gr;
			fg=(i&7)+((i&64)>>3);
			bg=((i>>3)&7)+((i&64)>>3);
			if (i>256) {gr=fg;fg=bg;bg=gr;}
			inkattr[i]=specpalentries[fg] ^ specpalentries[bg];    // AND bytes
			paperattr[i]=specpalentries[bg];                       // XOR bytes
		} else {
			paperattr[i]=specpalentries[((i>>(i<=256?3:0))&7) + ((i&64)>>3)];
			inkattr[i]=specpalentries[((i>>(i<=256?0:3))&7) + ((i&64)>>3)];
		}
	}

	memallocated=TRUE;
	curflashcount=0;
	flashoffset=0;
	videobuf->updatevisibility=TRUE;
	AdjustVideoSize();
	GlobalUnlock(hmem);
	GlobalFree(hmem);
	return(0);
}

void AdjustVideoSize(void)
// called when window size changes; used to select right bitmap into WinGdC
{
	if (gWinG) {
		if ((display.Xfac==2)&&(display.Yfac==2)&&(Own2x2Video)) {
			SelectObject(hWinGDC, h2x2BlockletStripBitmap);
		} else {
			SelectObject(hWinGDC, hBlockletStripBitmap);
		}
	}
}

void FreeVideo(void)
{
	int i;
	if (gWinG) {
		SelectObject(hWinGDC,hWinGoldbitmap);
		DeleteDC(hWinGDC);
	}
	if (memallocated) {
		DeleteObject(hPalette);
		DeleteObject(hBlockletStripBitmap);
		if (Own2x2Video) {
			DeleteObject(h2x2BlockletStripBitmap);
			if (!gWinG) {
				GlobalUnlock(h2x2bits);
				GlobalFree(h2x2bits);
			}
		}
		for (i=0;i<8;i++) {
			GlobalUnlock(videobuf->hbitmaps[i]);
			GlobalFree(videobuf->hbitmaps[i]);
		}
		GlobalUnlock(hvideobuf);
		GlobalFree(hvideobuf);
	}
	for (i=0;i<4;i++) {
		DeleteObject(borpart[i].hactbitmap);
		if ((!gWinG) && borpart[i].hborbitmap) {
			GlobalUnlock(borpart[i].hborbitmap);
			GlobalFree(borpart[i].hborbitmap);
		}
	}
	FreeWinGLib();
}


void ShowBlocks(BYTE far* bitmapbits,
	int x, int y1, int y2, int Xfac, int Yfac, int borsize,
	HDC hMemDC,HDC hDestDC)
// y2-y1+1 guaranteed not to exceed StripHeight
{
	HBITMAP hOld;
	int i;
	if ((Xfac==2)&&(Yfac==2)&&Own2x2Video) {
		// Special routines handle 2x2 case, as Stretch... often is dead slow
		BYTE far* bmbits;
		if (gWinG)
			bmbits = WinG2x2bitmapbits;
		else
			bmbits = bp2x2bits;
		switch (bpp) {
		case 1:  for (i=0;i<tbpp;i++)
						explodebitmap1(bitmapbits+4*i,bp2x2bits+8*i,8*(y2-y1+1),8*tbpp);
					break;
		case 4:  explodebitmap4(bitmapbits,bmbits,8*(y2-y1+1));break;
		case 8:  explodebitmap8(bitmapbits,bmbits,8*(y2-y1+1));break;
		case 16: explodebitmap16(bitmapbits,bmbits,8*(y2-y1+1));break;
		case 24: explodebitmap24(bitmapbits,bmbits,8*(y2-y1+1));break;
		case 32: explodebitmap32(bitmapbits,bmbits,8*(y2-y1+1));break;
		}
		if (gWinG) {
//			_fmemcpy(WinG2x2bitmapbits, bp2x2bits, 4*32*(8/8)*(y2-y1+1)*tbpp);
				myWinGBitBlt(hDestDC,borsize*2+x*64,borsize*2+y1*16,64,16*(y2-y1+1),
					hWinGDC,0,0);
		} else {
			hOld=SelectObject(hMemDC,h2x2BlockletStripBitmap);
			SetBitmapBits(h2x2BlockletStripBitmap,4*32*(8/8)*(y2-y1+1)*tbpp,bp2x2bits);
			BitBlt(hDestDC,borsize*2+x*64,borsize*2+y1*16,
					64,16*(y2-y1+1),hMemDC,0,0,SRCCOPY);
			SelectObject(hMemDC,hOld);
		}
		return;
	}
	if (gWinG) {
		_fmemcpy(WinGbitmapbits, bitmapbits, 32*(8/8)*(y2-y1+1)*tbpp);
		if ((Xfac==1)&&(Yfac==1)) {
			myWinGBitBlt(hDestDC,borsize+x*32,borsize+y1*8,32,8*(y2-y1+1),
				hWinGDC,0,0);
		} else {
			myWinGStretchBlt(hDestDC,(borsize+x*32)*Xfac,(borsize+y1*8)*Yfac,
				32*Xfac,8*(y2-y1+1)*Yfac,
				hWinGDC,0,0,32,8*(y2-y1+1));
		}
	} else {
		hOld=SelectObject(hMemDC,hBlockletStripBitmap);
		SetBitmapBits(hBlockletStripBitmap,32*(8/8)*(y2-y1+1)*tbpp,bitmapbits);
		if ((Xfac==1)&&(Yfac==1)) {
		#if DebugVideo
			Rectangle(hDestDC,borsize+x*32,borsize+y1*8,
				borsize+x*32+32,borsize+y1*8+8*(y2-y1+1));
		#endif
			BitBlt(hDestDC,borsize+x*32,borsize+y1*8,
				32,8*(y2-y1+1),hMemDC,0,0,SRCCOPY);
		} else {
		#if DebugVideo
			Rectangle(hDestDC,
				(x*32+borsize)*Xfac,(y1*8+borsize)*Yfac,
				(borsize+x*32+32)*Xfac,(borsize+y1*8+8*(y2-y1+1))*Yfac);
		#endif
			StretchBlt(hDestDC,(x*32+borsize)*Xfac,
							(y1*8+borsize)*Yfac,
							32*Xfac,8*(y2-y1+1)*Yfac,
							hMemDC,0,0,32,8*(y2-y1+1),SRCCOPY);
		}
		SelectObject(hMemDC,hOld);
	}
}


void ShowScreen(int Yield)
{
	HDC hDC=GetDC(hWndMain),hMemDC=NULL;
	HPALETTE holdpalette;
	HBRUSH hbrush;
	MSG msg;
	static int stopcol=0;
	static int currentcol=0;
	char yieldtotimer=FALSE;
	char WinGtouched=FALSE;		// if TRUE, then select proper bitmap back into WinGDC
	int i,j,j0,j1,brushcolor;
	if (!gWinG) hMemDC=CreateCompatibleDC(hDC);
	holdpalette=SelectPalette(hDC,hPalette,FALSE);
	RealizePalette(hDC);
	SelectPalette(hMemDC,hPalette,FALSE);
	RealizePalette(hMemDC);
	hbrush=0;
	brushcolor=8;
	if ((display.Xfac==2)&&(display.Yfac==2)&&Own2x2Video&&(!gWinG))
		bp2x2bits=GlobalLock(h2x2bits);
	do {
		BYTE far* bits=videobuf->bitmaps[currentcol];
		char donesomething=FALSE;
		j0=-1;
		for (j=0;j<24;j++) {
			if (videobuf->touched[j+24*currentcol]) {
				videobuf->touched[j+24*currentcol]=FALSE;
				if (j0==-1) {
					j0=j;
					j1=j;
				} else {
					if ((j-j1<=MaxIdleBlockletBlts+1)&&(j-j0+1<=StripHeight))
						j1=j;
					else {
						ShowBlocks(bits+j0*32*(8/8)*tbpp,currentcol,j0,j1,
							display.Xfac,display.Yfac,display.borsize,hMemDC,hDC);
						donesomething=TRUE;
						j0=j;
						j1=j;
					}
				}
			}
		}
		if (j0!=-1) {
			ShowBlocks(bits+j0*32*(8/8)*tbpp,currentcol,j0,j1,
				display.Xfac,display.Yfac,display.borsize,hMemDC,hDC);
			donesomething=TRUE;
		}
			// Now yield to any pending WM_TIMER messages
		if (QuickDraw) {
			if (Yield&&donesomething)
				yieldtotimer=
					PeekMessage(&msg,hWndMain,WM_TIMER,WM_TIMER,PM_NOREMOVE|PM_NOYIELD);
		}
		currentcol=(currentcol+1)&7;
	} while ((currentcol!=stopcol)&&(!yieldtotimer));
	if (currentcol!=stopcol)
		stopcol=currentcol;
	else {
		currentcol=stopcol=0;
		for (i=0;i<4;i++) if (borpart[i].visible) {
			 if ((borpart[i].touched)||
				  ((!copper)&&(borpart[i].colour!=(border&7)))) {
					RECT r;
					borpart[i].touched=FALSE;
					if (copper && (borpart[i].colour==8)) {
						HBITMAP hbitmapold;
						if (gWinG) {
							WinGtouched = TRUE;
							SelectObject(hWinGDC,borpart[i].hactbitmap);
							if ((display.Xfac==1)&&(display.Yfac==1)) {
								myWinGBitBlt(hDC,borpart[i].xpos,borpart[i].ypos,
													  borpart[i].xsize,borpart[i].ysize,
													  hWinGDC,0,0);
							} else {
								myWinGStretchBlt(hDC,
													borpart[i].xpos*display.Xfac,
													borpart[i].ypos*display.Yfac,
													borpart[i].xsize*display.Xfac,
													borpart[i].ysize*display.Yfac,
													hWinGDC,0,0,
													borpart[i].xsize,
													borpart[i].ysize);
							}
						} else {
							// non-WinG
							if (!hMemDC)
								hMemDC=CreateCompatibleDC(hDC);
							hbitmapold=SelectObject(hMemDC,borpart[i].hactbitmap);
							SetBitmapBits(borpart[i].hactbitmap,(borpart[i].xsize/8) *
								borpart[i].ysize*tbpp,borpart[i].borbitmap);
							if ((display.Xfac==1)&&(display.Yfac==1))
							{
								BitBlt(hDC,borpart[i].xpos,borpart[i].ypos,
											borpart[i].xsize,borpart[i].ysize,hMemDC,0,0,SRCCOPY);
							}
							else
							{
								StretchBlt(hDC,
												borpart[i].xpos*display.Xfac,
												borpart[i].ypos*display.Yfac,
												borpart[i].xsize*display.Xfac,
												borpart[i].ysize*display.Yfac,
												hMemDC,0,0,borpart[i].xsize,borpart[i].ysize,SRCCOPY);
							}
							SelectObject(hMemDC,hbitmapold);
						}
					} else {
						if (!copper)
							borpart[i].colour=border&7;
						if (borpart[i].colour != brushcolor) {
							DeleteObject(hbrush);
							j=borpart[i].colour;
							hbrush=CreateSolidBrush(bordercolor[borpart[i].colour]);
							brushcolor=borpart[i].colour;
						}
						SetRect(&r,borpart[i].xpos*display.Xfac,borpart[i].ypos*display.Yfac,
							(borpart[i].xpos+borpart[i].xsize)*display.Xfac,
							(borpart[i].ypos+borpart[i].ysize)*display.Yfac);
						FillRect(hDC,&r,hbrush);
					}
			 }
		}
	}
	SelectPalette(hDC,holdpalette,FALSE);
	ReleaseDC(hWndMain,hDC);
	if (hMemDC)
		DeleteDC(hMemDC);
	DeleteObject(hbrush);
	if ((display.Xfac==2)&&(display.Yfac==2)&&Own2x2Video&&(!gWinG))
		GlobalUnlock(h2x2bits);
	if (WinGtouched)
		AdjustVideoSize();		// to re-select h[2x2]BlockletStripBitmap back into hWinGDC
}


void ShowHelpScreen(void)
{
	HDC hDC=GetDC(hWndHelp),hMemDC;
	HPALETTE holdpalette;
	HBRUSH hbrush;
	RECT r;
	int currentcol;
	int i;

	hMemDC=CreateCompatibleDC(hDC);
	holdpalette=SelectPalette(hDC,hPalette,FALSE);
	RealizePalette(hDC);
	if ((hdisplay.Xfac==2)&&(hdisplay.Yfac==2)&&Own2x2Video&&(!gWinG))
		bp2x2bits=GlobalLock(h2x2bits);
	for (currentcol=0;currentcol<8;currentcol++) {
		for (i=0;i<24;i+=StripHeight) {
			ShowBlocks(videobuf->bitmaps[currentcol]+32*(8/8)*tbpp*i,
				currentcol,i,i+StripHeight-1,
				hdisplay.Xfac,hdisplay.Yfac,hdisplay.borsize,hMemDC,hDC);
		}
	}
	hbrush=CreateSolidBrush(PALETTEINDEX(HelpScreenBorderColour));
	for (i=0;i<4;i++) {
		 SetRect(&r,hborpart[i].xpos*hdisplay.Xfac,hborpart[i].ypos*hdisplay.Yfac,
						(hborpart[i].xpos+hborpart[i].xsize)*hdisplay.Xfac,
						(hborpart[i].ypos+hborpart[i].ysize)*hdisplay.Yfac);
		 FillRect(hDC,&r,hbrush);
	}
	SelectPalette(hDC,holdpalette,FALSE);
	ReleaseDC(hWndHelp,hDC);
	DeleteDC(hMemDC);
	DeleteObject(hbrush);
	if ((hdisplay.Xfac==2)&&(hdisplay.Yfac==2)&&Own2x2Video&&(!gWinG))
		GlobalUnlock(h2x2bits);
}


void TouchAllBlocks(void)
{
	int i;
	for (i=0;i<192;i++) videobuf->touched[i]=TRUE;
	for (i=0;i<4;i++) {
		borpart[i].touched=TRUE;
	}
}

void UpdateVisibility(void)
{
	int i,j,visible;
	HDC hDC=GetDC(hWndMain);
	for (i=0;i<8;i++) for (j=0;j<24;j++) {
		RECT r;
		r.left=(i*32+display.borsize)*display.Xfac;
		r.right=((i+1)*32+display.borsize)*display.Xfac;
		r.top=(j*8+display.borsize)*display.Yfac;
		r.bottom=((j+1)*8+display.borsize)*display.Yfac;
		visible=RectVisible(hDC,&r) || bSaveGif;		// when saving GIF, compute all!
//      if (visible && (!(videobuf->visible[j+24*i]))) {
			#if DebugVideo
//         MoveTo(hDC,r.left,r.bottom);
//         LineTo(hDC,r.right,r.top);
			#endif
//         if (copper) {
//             for (k=0;k<8;k++) videobuf->olddata[i*4+j*512+k*64]++;
//         } else {
//            videobuf->olddata[6144+4*i+32*j]++;
//         }
//      }
		videobuf->visible[j+24*i]=visible;
	}
	for (i=0;i<4;i++) {
		RECT r;
		int visible;
		SetRect(&r,borpart[i].xpos*display.Xfac,borpart[i].ypos*display.Yfac,
			(borpart[i].xpos+borpart[i].xsize)*display.Xfac,
			(borpart[i].ypos+borpart[i].ysize)*display.Yfac);
		visible=RectVisible(hDC,&r) || bSaveGif;
		if (visible && (!borpart[i].visible)) {
			borpart[i].touched=TRUE;
		}
		borpart[i].visible=visible;
	}
	ReleaseDC(hWndMain,hDC);
	videobuf->updatevisibility=FALSE;
}


void TouchRectangle(RECT *rect)
{
	int i,j;
	RECT r,rdummy;
	for (i=0;i<8;i++) for (j=0;j<24;j++) {
		SetRect(&r,(i*32+display.borsize)*display.Xfac,
						(j*8+display.borsize)*display.Yfac,
						((i+1)*32+display.borsize)*display.Xfac,
						((j+1)*8+display.borsize)*display.Yfac);
		if (IntersectRect(&rdummy,&r,rect)) videobuf->touched[j+24*i]=TRUE;
	}
	for (i=0;i<4;i++) {
		SetRect(&r,borpart[i].xpos*display.Xfac,
				borpart[i].ypos*display.Yfac,
				(borpart[i].xpos+borpart[i].xsize)*display.Xfac,
				(borpart[i].ypos+borpart[i].ysize)*display.Yfac);
		if (IntersectRect(&rdummy,&r,rect)) borpart[i].touched=TRUE;
	}
}

void InvalidateVidBuffer(BYTE *specmem)
{
	int i;
	copyvidbuffer((BYTE*)videobuf,specmem+16384);
	for (i=0;i<12288;i++) videobuf->olddata[i]++;
	for (i=0;i<4;i++) {
		borpart[i].touched=TRUE;
	}
}

void UpdateVideo(BYTE *vidmem)
{
	if (copper) {
		// data in videobuf.olddata is in coppering format, therefore we
		// should use UpdateVideoCopper.  First translate screen data to
		// coppering format (overwriting possible current info, but well..)
		// If saving a GIF, then ignore call!
		if (bSaveGif) return;
		copyvidbuffer((BYTE*)vidbufbase,vidmem);
		outbufptr[0]=border&7;       // time 0
		outbufptr[1]=70000L*256;     // time \infinity
		UpdateVideoCopper(vidbufbase);
		return;
	}
	if (videobuf->updatevisibility) UpdateVisibility();
	if ((flashcnt&16)!=curflashcount) {
		curflashcount=flashcnt&16;
		flashoffset=curflashcount<<3;    // 0 or 128
		touchflash((BYTE*)videobuf,vidmem);
	}
	if (bpp==8) {
		updatebitmaps8((BYTE*)videobuf,vidmem);
	} else if (bpp==4) {
		updatebitmaps4((BYTE*)videobuf,vidmem);
	} else if (bpp==1) {
		updatebitmaps1((BYTE*)videobuf,vidmem);
	} else if (bpp==24) {
		updatebitmaps24((BYTE*)videobuf,vidmem);
	} else if (bpp==16) {
		updatebitmaps16((BYTE*)videobuf,vidmem);
	} else if (bpp==32) {
		updatebitmaps32((BYTE*)videobuf,vidmem);
	}
}

void UpdateVideoCopper(BYTE *vidmem)
{
	if (videobuf->updatevisibility || bSaveGif) UpdateVisibility();
	if ((flashcnt&16)!=curflashcount) {
		curflashcount=flashcnt&16;
		flashoffset=curflashcount<<3;    // 0 or 128
		touchflashcopper((BYTE*)videobuf,vidmem);
	}
	if (bpp==8) {
		updateborder8();
		updatebitmapscopper8((BYTE*)videobuf,vidmem);
	} else if (bpp==4) {
		updateborder4();
		updatebitmapscopper4((BYTE*)videobuf,vidmem);
	} else if (bpp==1) {
		updateborder1();
		updatebitmapscopper1((BYTE*)videobuf,vidmem);
	} else if (bpp==24) {
		updateborder24();
		updatebitmapscopper24((BYTE*)videobuf,vidmem);
	} else if (bpp==16) {
		updateborder16();
		updatebitmapscopper16((BYTE*)videobuf,vidmem);
	} else if (bpp==32) {
		updateborder32();
		updatebitmapscopper32((BYTE*)videobuf,vidmem);
	}
}

BOOL DisplayHelpScreen(void)
// returns TRUE if in error
{
	HGLOBAL hHelp;
	char far *screen;
	HFILE handle;
	HDC hDC;
	char msg[] = "Couldn't find file 'layout.scr', or file corrupt";
	char tempvisible[192],temptouched[192];
	int i;

	hHelp=GlobalAlloc(GMEM_MOVEABLE,6912+12288);
	if (!hHelp) return TRUE;
	screen = GlobalLock(hHelp);
	handle=OpenRead(gszLayoutFile);
	if (handle==-1) {
		help_err:
		GlobalUnlock(hHelp);
		GlobalFree(hHelp);
		hDC = GetDC(hWndHelp);
		Rectangle(hDC,-1,-1,30000,30000);
		TextOut(hDC,10,10,msg,lstrlen(msg));
		ReleaseDC(hWndHelp,hDC);
		return TRUE;
	}
	if (_lread(handle,screen,6912)!=6912) {
		_lclose(handle);
		goto help_err;
	}
	_lclose(handle);
	_fmemcpy(screen+6912,&((*videobuf).olddata),12288);
	for (i=0;i<192;i++) {
		tempvisible[i]=(*videobuf).visible[i];
		temptouched[i]=(*videobuf).touched[i];
		(*videobuf).visible[i]=TRUE;
	}
	if (bpp==8) {
		updatebitmaps8((BYTE*)videobuf,screen);
	} else if (bpp==4) {
		updatebitmaps4((BYTE*)videobuf,screen);
	} else if (bpp==1) {
		updatebitmaps1((BYTE*)videobuf,screen);
	} else if (bpp==24) {
		updatebitmaps24((BYTE*)videobuf,screen);
	} else if (bpp==16) {
		updatebitmaps16((BYTE*)videobuf,screen);
	} else if (bpp==32) {
		updatebitmaps32((BYTE*)videobuf,screen);
	}
	ShowHelpScreen();
	if (copper) copyvidbuffer((BYTE*)videobuf,screen);
	for (i=0;i<192;i++) {
		(*videobuf).visible[i]=tempvisible[i];
	}
	if (copper)
		UpdateVideoCopper(screen+6912);
	else
		UpdateVideo(screen+6912);
	for (i=0;i<192;i++) (*videobuf).touched[i]=temptouched[i];
	GlobalUnlock(hHelp);
	GlobalFree(hHelp);
	return FALSE;
}



int loadscreen(HANDLE handle)
{
	BYTE *scr;
	if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked || useVz80d))
		scr=page7fp;
	else
		scr=SpecMem+16384;
	_lread(handle,scr,6912);
	return 0;
}

int savescreen(HANDLE handle)
{
	BYTE *scr;
	int i,j,pix,attr;
	if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked || useVz80d))
		scr=page7fp;
	else
		scr=SpecMem+16384;
	_lwrite(handle,scr,6912);
	return 0;
}

