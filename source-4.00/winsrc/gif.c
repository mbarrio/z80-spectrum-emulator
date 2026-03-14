#include <windows.h>
#include <stdio.h>
#include "spectrum.h"
#include "gd.h"
#include "giflib.h"

int 	x0=0,y0=191,x1=255,y1=0;
BOOL 	gifRealTime=TRUE;		// 1=time exps according to FrameCounter, 0=use MovieSpeed
BOOL	gifRecording=FALSE;	// 1=busy now
BOOL	gifLoop=FALSE;			// 1=loop around
int 	gifFrameCounter=0;	// 20ms counter from first exposure
int	gifExposureCounter=0;// # of exposures done up till now
int	gifMovieSpeed=4;		// # of frames (20ms) per exposure, if !gifRealTime
int	gifRecSpeed=4;			// # of frames per exposure
long	gifLastExposureTime;	// Frame time of last exposure
int	gifFinalDelay=1000;	// Delay after final exposure
int	gifFactor=1;
BYTE *gbpScr;
fpos_t	gifLastExposureLoc;	// Location of beginning last exposure (gets set by gbImageGif)
HWND	hGifDlg;
FARPROC lpfnGifProc;

FILE*	gifFout;					// output GIF file handle


void GifResetVars(void)
{
	gifRecording = FALSE;
	gifFrameCounter = 0;
	gifExposureCounter = 0;
}

void GifSetTime(void)
{
	WORD delay;
	fpos_t current_pos;

	if (gifExposureCounter == 0) {
		gifFrameCounter = 0;
		gifLastExposureTime = soundtimehi;
	} else {
		if (gifRealTime)
			delay = (soundtimehi - gifLastExposureTime)*2;
		else
			delay = gifMovieSpeed*2;
		gifLastExposureTime = soundtimehi;
		fgetpos(gifFout, &current_pos);
		fsetpos(gifFout, &gifLastExposureLoc);
		fputc( delay & 0xFF , gifFout );
		fputc( delay >> 8, gifFout );
		fsetpos(gifFout, &current_pos);
	}
}


void GifSaveGD(void)
{
	gdImagePtr im;
	int colors[16];
	BYTE *scr;
	int i,j,n,m;
	BYTE pix,attr;

	if ((x0>=x1)||(y0<=y1)) return;
	GifSetTime();
	if (copper) scr=vidbufbase; else scr=vidbufbase;
	im = gdImageCreate(gifFactor*(x1-x0+1),gifFactor*(y0-y1+1));
	for (i=0;i<16;i++)
		colors[i] =
			gdImageColorAllocate(im,((WORD)255*(WORD)SpecColor[i][0])/63,((WORD)255*(WORD)SpecColor[i][1])/63,((WORD)255*(WORD)SpecColor[i][2])/63);
	gdImageColorTransparent(im,-1);
	for (i=x0;i<=x1;i++)
		for (j=191-y0;j<=191-y1;j++) {
			if (copper) {
				pix  = scr[i/8+64*j];
				attr = scr[i/8+64*j+32];
			} else {
				pix  = scr[i/8 + 256*(j&7) + 4*(j&56) + 32*(j&192)];
				attr = scr[i/8 + 32*(j/8) + 6144];
			}
			pix <<= (i&7);									// pixel now in bit 7
			if (curflashcount && (attr&0x80))
				pix ^= 0x80;								// do the flash
			if ((pix & 0x80) == 0)
				attr = colors[((attr>>3)&15)];		// paper, so shift ppr clr right
			else
				attr = colors[(attr & 7) + (((attr & 64)>>3))];	// ink; include bright
			if (attr==8) attr=0;
			for (n=0;n<gifFactor;n++) for (m=0;m<gifFactor;m++)
				gdImageSetPixel(im,n+gifFactor*(i-x0),m+gifFactor*(j-191+y0),attr);
	}
	// If final delay is shorter than delay between exposures, then make this
	//  latter delay the final delay.  Sounds simple, requires little bit of care
	if (gifRealTime)
		i = gifRecSpeed*2;
	else
		i = gifMovieSpeed*2;
	if (!(gifLoop && gifRecording && (gifFinalDelay/10 < i)))
		i = gifFinalDelay/10;
	gdImageGif(im,gifFout,gifExposureCounter==0,gifLoop,i);
	gdImageDestroy(im);
	gifExposureCounter++;
}


int ColorDist(int i,int j)
{
	long dist,m;
	m=SpecColor[i][0]-SpecColor[j][0];
	dist = m*m*5;
	m=SpecColor[i][1]-SpecColor[j][1];
	dist += m*m*10;
	m=SpecColor[i][2]-SpecColor[j][2];
	dist += m*m*4;
	return dist/4;
}

int GifLoad(FILE *fin)
{
	gdImagePtr im,im2;
	BYTE *scr;
	int xsiz,ysiz;
	int bw;
	int x,y,i,j,k,l;
	int maxdist,dist,c1,c2;
	int colors[16];
	int colsused[16];

	im = gdImageCreateFromGif(fin);
	if (im==NULL) return 1;
	xsiz = im->sx;
	ysiz = im->sy;
	if (xsiz > 256) {
		ysiz = ((long)ysiz*256L)/xsiz;
		xsiz = 256;
	}
	if (ysiz > 192) {
		xsiz = ((long)xsiz*192L)/ysiz;
		ysiz = 192;
	}
	if (xsiz&7)
		xsiz = ((xsiz|3)+1)&0x1F8;
	if (ysiz&7)
		ysiz = ((ysiz|3)+1)&0x1F8;
	im2 = gdImageCreate(xsiz,ysiz);
	bw = 16*state.blackandwhite;
	for (i=0;i<16;i++)
		colors[i] =
			gdImageColorAllocate(im2,((WORD)255*(WORD)SpecColor[i+bw][0])/63,((WORD)255*(WORD)SpecColor[i+bw][1])/63,((WORD)255*(WORD)SpecColor[i+bw][2])/63);
	// Fill in color table with black, to prevent gdImageCopyResized to invent new colors
	do {} while (gdImageColorAllocate(im2,0,0,0) != -1);
	// Find out where to put screen
	if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked || useVz80d))
		scr=page7fp;
	else
		scr=SpecMem+16384;
	// Now make a point-color-resolution copy of `im' using Spectrum colors
	gdImageCopyResized(im2,im,0,0,0,0,xsiz,ysiz,im->sx,im->sy);
	// Do the Color Clash!
	for (i=0;i<xsiz;i+=8) for (j=0;j<ysiz;j+=8) {
		// look which colors are used in this 8x8 block
		for (k=0;k<16;k++)
			colsused[k]=0;
		for (x=0;x<8;x++) for (y=0;y<8;y++)
			colsused[gdImageGetPixel(im2,i+x,j+y)] = 1;
		// find pair that maximises distance
		maxdist = -1;
		for (k=0;k<16;k++) if (colsused[k]) {
			if (maxdist==-1) {
				c1=c2=k;
			}
			for (l=k+1;l<16;l++) if (colsused[l]) {
				dist = ColorDist(k+bw,l+bw);
				if (dist > maxdist) {
					maxdist = dist;
					c1 = k;
					c2 = l;
				}
			}
		}
		// Care for the bright bit
		if ((c1<8)&&(c2>=8)) {
			if ((c1&7)<(c2&7))
				c1 |= 8;
			else
				c2 &= 7;
		}
		// Store attribute byte; c1 is ink
		scr[i/8 + 32*(j/8) + 6144] = (c1&7)|(c2<<3);
		// Store bits
		for (y=0;y<8;y++) {
			k=0;
			for (x=0;x<8;x++) {
				l = gdImageGetPixel(im2,i+x,j+y);
				if (ColorDist(l+bw,c1+bw) < ColorDist(l+bw,c2+bw)) {
					// ink bit
					k |= (0x80 >> x);
				}
			}
			scr[i/8 + 256*y + 4*(j&56) + 32*(j&192)] = k;
		}
	}
	gdImageDestroy(im);
	gdImageDestroy(im2);
	return 0;
}


short GifGetPixel(short i,short j)
{
			short attr,pix;
			long time,l;
			short curj=j;
			static short lastj;
			static long startlinidx;
			static long curidx;
			static long curframectr=-1;
			static short curborder;
			static short startlinborder;

			i/=gifFactor;
			j/=gifFactor;
			if ((i<0)||(i>255)||(j<0)||(j>191)) {
				// border.  First calculate time
				if (j==185) {
					time=0;
				}
				if (hmode < hm_128k) {
					// 48k modes
					time = 14345L + 224L*j + 4L*(i/8);
				} else {
					// 128k modes
					time = 14364L + 228L*j + 4L*(i/8);
				}
				// if y coord increased, reset curidx.
				// if current frame counter has changed, also reset startlinidx
				if (curj > lastj) {
					curidx = startlinidx;
					curborder = startlinborder;
				}
				if (curframectr != soundtimehi) {
					curidx = 0;
					curframectr = soundtimehi;
					lastj = curj-1;
				}
				// now look for border colour
				l = *(outbufptr + curidx);
				while ( (l>>8) < time ) {
					curborder = l&7;
					l = *(++curidx + outbufptr);
				}
				// store starting index of this line, if appropriate
				if (curj > lastj) {
					startlinidx = curidx;
					startlinborder = curborder;
					lastj = curj;
				}
				// finished
				attr = curborder;
			} else {
				if (copper) {
					pix  = gbpScr[i/8+64*j];
					attr = gbpScr[i/8+64*j+32];
				} else {
					pix  = gbpScr[i/8 + 256*(j&7) + 4*(j&56) + 32*(j&192)];
					attr = gbpScr[i/8 + 32*(j/8) + 6144];
				}
				pix <<= (i&7);									// pixel now in bit 7
				if (curflashcount && (attr&0x80))
					pix ^= 0x80;								// do the flash
				if ((pix & 0x80) == 0)
					attr = ((attr>>3)&15);					// paper, so shift ppr clr right
				else
					attr = (attr & 7) + (((attr & 64)>>3));	// ink; include bright
				if (attr==8) attr=0;
			}
			return attr;
}

void GifSave(void)
{
	WORD i;
	int bw;

	if ((x0>=x1)||(y0<=y1)) return;
	GifSetTime();
	if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked || useVz80d))
		gbpScr=page7fp;
	else
		gbpScr=SpecMem+16384;
	if (copper) gbpScr=vidbufbase;
	if (gifExposureCounter == 0) {
		GIFSetOutFile(gifFout);
		GIFCreate("",gifFactor*(x1-x0+1),gifFactor*(y0-y1+1),16,8,TRUE);
		bw = 16*state.blackandwhite;
		for (i=0;i<16;i++)
			GIFSetColour(i,((WORD)255*(WORD)SpecColor[i+bw][0])/63,
								 ((WORD)255*(WORD)SpecColor[i+bw][1])/63,
								 ((WORD)255*(WORD)SpecColor[i+bw][2])/63 );
		GIFWriteGlobalColorTable(gifLoop);
	}
	if (gifRealTime)
		i = gifRecSpeed*2;
	else
		i = gifMovieSpeed*2;
	if (!(gifLoop && gifRecording && (gifFinalDelay/10 < i)))
		i = gifFinalDelay/10;
	GIFCompressImage(x0*gifFactor,(191-y0)*gifFactor,(x1-x0+1)*gifFactor,(y0-y1+1)*gifFactor,GifGetPixel,TRUE,i);
	gifExposureCounter++;
}



void GifFrame(void)
{
	if (!gifRecording) return;		// to be sure
	if ( (gifRecSpeed<=1) || ((gifFrameCounter % gifRecSpeed)==0) )
		GifSave();
	gifFrameCounter++;
	SendMessage(hGifDlg,WM_INITDIALOG,0,0);
}


void GifSetCoord(int x, int y)
{
	static char uppleft=TRUE;		// where do we expect new point?
	// first see if expectation isn't frustrated by stupid user
	if (uppleft && (x1>x) && (y1<y))
		goto gotit;
	if ((!uppleft) && (x0<x)&&(y0>y))
		goto gotit;
	// ah, you'll always see.  Let's see if we can make any sense
	if ((x1>x)&&(y1<y))
		uppleft = TRUE;
	if ((x0<x)&&(y0>y))
		uppleft = FALSE;
	gotit:
	// now set the relevant point
	if (uppleft) {
		x0=x;
		y0=y;
		uppleft=FALSE;
	} else {
		x1=x;
		y1=y;
		uppleft=TRUE;
	}
	if (hGifDlg) SendMessage(hGifDlg,WM_INITDIALOG,0,0);
}

void GifGetVars(HWND hDlg)
{
	BOOL b;
	x0 = GetDlgItemInt(hDlg,RM_X0,&b,TRUE);
	y0 = GetDlgItemInt(hDlg,RM_Y0,&b,TRUE);
	x1 = GetDlgItemInt(hDlg,RM_X1,&b,TRUE);
	y1 = GetDlgItemInt(hDlg,RM_Y1,&b,TRUE);
	gifRecSpeed = GetDlgItemInt(hDlg,RM_RECSPD,&b,FALSE);
	gifMovieSpeed = GetDlgItemInt(hDlg,RM_MOVIESPD,&b,FALSE);
	gifFinalDelay = GetDlgItemInt(hDlg,RM_DELAY,&b,FALSE);
	gifLoop = IsDlgButtonChecked(hDlg,RM_LOOP);
	gifFactor = GetDlgItemInt(hDlg,RM_FACTOR,&b,FALSE);
	if (gifFactor<1) gifFactor=1;
	if (gifFactor>3) gifFactor=3;
	SendMessage(hDlg,WM_INITDIALOG,0,0);
}


BOOL CALLBACK GifDlgProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	char str[80];
	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		if (gifExposureCounter==0)
			sprintf(str,"Exposures: <none>");
		else
			sprintf(str,"Exposures: %d",gifExposureCounter);
		SetDlgItemText(hDlg,RM_EXPOSURES,str);
		if (gifRecording)
			SetDlgItemText(hDlg,RM_RECORD,"-Pause-");
		else
			SetDlgItemText(hDlg,RM_RECORD,"Record");
		SetDlgItemInt(hDlg,RM_X0,x0,TRUE);
		SetDlgItemInt(hDlg,RM_X1,x1,TRUE);
		SetDlgItemInt(hDlg,RM_Y0,y0,TRUE);
		SetDlgItemInt(hDlg,RM_Y1,y1,TRUE);
		SetDlgItemInt(hDlg,RM_MOVIESPD,gifMovieSpeed,FALSE);
		SetDlgItemInt(hDlg,RM_RECSPD,gifRecSpeed,FALSE);
		SetDlgItemInt(hDlg,RM_DELAY,gifFinalDelay,FALSE);
		SetDlgItemInt(hDlg,RM_FACTOR,gifFactor,FALSE);
		CheckRadioButton(hDlg,RM_REALTIME,RM_MOVIE,RM_REALTIME+!gifRealTime);
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		hGifDlg=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnGifProc);
		goto gif_done;
	case WM_COMMAND:
		switch (wPar) {
//		case 2:              // don't ask me why
//			hrd=RepaintData(hDlg);
//			DestroyWindow(hDlg);
//			return TRUE;
		case RM_WHOLE:
			x0 = 0;
			y0 = 191;
			x1 = 255;
			y1 = 0;
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case RM_SCRBORDER:
			if (display.borsize==2) {
				x0=-32; y0=223; x1=287; y1=-32;
			} else {
				x0=-16; y0=207; x1=271; y1=-16;
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case RM_REALTIME:
			gifRealTime = TRUE;
			return TRUE;
		case RM_MOVIE:
			gifRealTime = FALSE;
			return TRUE;
		case RM_DONE:
			if ((gifExposureCounter == 0)&&(gifFout)) GifSave();
			DestroyWindow(hDlg);
			gif_done:
//			if (gifFout && gifExposureCounter) {
//				// GIF Trailer
//				fputc(';',gifFout);
//			}
//			if (gifFout) fclose(gifFout);
			if (gifFout)
				GIFClose();
			gifFout = NULL;
			GifResetVars();
			return TRUE;
		case RM_SNAPSHOT:
			GifGetVars(hDlg);
			if ((x0>x1)||(y0<y1)) {
				MessageBeep(-1);
				return TRUE;
			}
			if (gifExposureCounter == 0)
				gifFrameCounter=0;
			GifSave();
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case RM_RECORD:
			GifGetVars(hDlg);
			if ((x0>x1)||(y0<y1)) {
				MessageBeep(-1);
				return TRUE;
			}
			gifRecording = !gifRecording;
			if (gifRecording && (gifExposureCounter==0)) {
				gifFrameCounter=0;
			}
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			// actual recording handled by main loop, by calling GifFrame
			return TRUE;
		}
		return FALSE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}

