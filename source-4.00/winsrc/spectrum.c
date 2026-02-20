#include <windows.h>
#include <stdio.h>      // for sprintf
#include <stdlib.h>     // for random()
#include <math.h>       // for log(), exp()
#include <mmsystem.h>   // for MM_WOM_DONE
#include <string.h>     // for strcpy
#include <ctype.h>		// for toupper
#include <dos.h>			// for MK_FP
#include <shellapi.h>		// for DragAcceptFiles & registry
#include "spectrum.h"
#include <bwcc.h>

#define SHW 0

//	#include <winreg.h>	// buggy
#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )

// #include <winuser.h>		// for GCL_STYLE, SetClassLong // buggy
#define GCL_STYLE (-26)

/* begin added by jts 4/2/97 */
#include <commdlg.h>
#define OFN_EXPLORER                 0x00080000     // new look commdlg
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES                0x00200000     // force long names for 3.x modules
/* end added by jts 4/2/97 */

#define SHWTIME 10

#define shw_x 500
#define shw_y 450
#define okay_x 80
#define okay_y 25
#define lpszShareware "This program is shareware.  If you like it, you can register. \
Registered users receive a fully enabled version (without this intro window) as well as Z80 for \
DOS, several utility programs, and the full source code.  See the help files for details."

// Z80 v4.0 for Windows
// Eerste regel geschreven 12/11/95
// Eerste programma's geladen via SBlaster: 8/8/96
// vz80d lijkt te werken: 27/5/98
// Twee beta testrondes doorstaan: 9/9/98


// To Do:
//
//
// TZX select/message/stop in 48k mode
// 128k roms: editor/48kbasic
// .z80 load problems??
// Dlg boxes crash on NT
// TZX with lots of entries dont show last few
// mf128 werkt niet??
// Currah werkt niet meer
// winz80 crasht soms bij inladen .z80 file (geluid?) met divide error
// shortcut keys in help file

// Help scherm bugs: bij update gaan graphics over en weer, en verschillende
//  groottes tegelijkertijd werkt niet.
// MDA Demo laadt niet (VOC/TZX; 128k mode emulatie?), Winter-a.tzx laadt niet
// Zynaps: movement is not smooth.  Timing while playing sound should be better.
// Illusion demo: reset doesn't reset to 128k mode; sound is not properly switched off
// Sound makes emulator slow.  Maybe rep stosd works better.
// 128k timing
// Catch sound blocks with interrupts instead of polling
// Memory allocation for border blocks doesn't seem to be right (32 bpp)
// Border colours are not right.  8 bpp seems okay, 4 bpp not.

// notify werkt pas na ShowWindow in initall.  Controleren of andere initialisatie-
//  proc''s niet notify''s kunnen doen.
// Fout in geluid: save van lege computer in SamRam geeft af en toe een
//  hoge bip; het lijkt alsof stukjes twee maal van word->byte worden getrans-
//  formeerd.
// Hoe krijg ik de emulator gladder te lopen met geluid?  Hij schokt nu wat.
// Loading screens in SLTs laten zien.
// Over core nadenken.  Wat als IN de instructie zelf wegswapt?  Deze uitz.
//  situatie apart behandelen?  De IN zelf afhandelen?  Speciale ED Fx instrs
//  zijn ook probleem: debug en log worden niet aangeroepen nu.  Of afhandelen
//  zoals ook IN wordt afgehandeld - maar dan geven zij hetzelfde probleem -
//  of een nette 'dummy exec' procedure maken, die ook debug en log doet.
//  OUT geeft NA instructie, maar VOOR log/debug, een exception.  Hiervoor
//  moet zeker een 'dummy exec' procedure komen (PC niks ophogen, maar wel
//  log/debug doen)
// In 8 bit video mode moet ik GetNearestPaletteIndex doen voor egale border,
//  in 1x4 bit mode moet ik GetPixel doen.
// Als geen SB aanwezig, lijkt de zaak niet goed te lopen
// Rare 'stack exception': zynaps lang laten draaien, chasehq ingeladen
//  in Warajevo mode, penetrator, mspacman, poleposition, ...
// Inladen standaardsettings: snelheidswindow updaten
// Andere ROM file
// Toetsenbord-instelling voor spelletjes of typen (penetrator, zynaps)
// Joystick support (HET)
// Flags hiram, loram schijnen 0xff te zijn in .z80 files; 0 echter binnen
// VOC, WAV files schrijven
// WAV, raw files lezen
// Emulator core (klaar; Uridium doet raar met Paused)
// Spectrum font

// 128 timings (2/1/99)
// bpp != 8, WinG gives problems with border (we use ordinary routines there)
//   Bad solution: don't use WinG if != 8 bpp. (gedaan)
// help index (gedaan)
// Border troep (gedaan)
// Hardware settings in INI (gedaan)
// Loading speed in %   (gedaan: switch voor fastest speed tijdens laden)
// Game Over 2, Ghosts'n'Ghoulds laden niet als TZX, wel als VOC. (Zet intelliin uit)
// Aargh werkt nog steeds niet.  VOC + intelli in uit gaat goed, TZX niet. (bit1=2*b0,
//  nu niet meer.)
// Sommige nieuwe .TZX files laden niet.  AARGH! laadt wel als je logging aanzet.
//  (Oplossing: zet IntelliIn uit!)
// Breakpoints doen .TZX files niet meer goed laden.  T state counter fout no doubt.
//  Maar wat? (Opgelost)
// F10, quit, yes (gedaan)
// Multiple copies of WinZ80 crash (Halfzachte oplossing: sta max 1x opstarten toe)
// Loading a .Z80 file while music is playing causes gen prot exceptions.  Time
//  variables probably not properly reset. (amps were incorr reset in ResetAY;14/9/97)
// RS232 window: edit windowlet doesnt get keyboard focus (13/9/97)
// Nice lines in hardware dialog use BorShade window class, must be initialised.
//  If it is used program will not be portable to Microsoft C	(done; 28/5/97)
// Save heeft nu ruimere marge (langere en meer sample blocks; 10/7/97)
// DIHALT dialog procedure werkt niet: EI+CONT (10/7/97)
// Alien8 fout: scherm gaat weg (reageerde op 128k outs; 10/7/97)
// Loading new snapshot while music is being produced sometimes gives protection
//  violation.  Probably error in time setting. (done? 28/5/97)
// Border coppering werkt niet: alleen onder zijn streepjes (14/8/96)
// Foutje uit border coppering (kleur in 1e lijn veranderd -> rechts foute kleur)(15/4)
// Window is 1 pixel te groot? (inderdaad, zeg!)(aug 96)
// .DAT files laden (13/4/96)
// 'Close' moet werken (11/4/96)
// User-defined opties in .INI file in aparte categorie onderbrengen (11/4/96)
// Foutje weggehaald: LD IYh,IYl deed LD IYl,IYh (Chase HQ) (3/4/96)
// 'Warajevo' mirror van VOCs naar TAPs (3/4/96) (Chase HQ werkt!)
// naar TAPs mirroren (makkelijk, behalve VOC -> TAP) (3/4/96)
// naar TAPs saven (27/3/96)
// Jetset Willy, Antics gaan ervandoor; rare joystick? Iets met floating bus? (14/3)
// Bij lage snelheid worden geluidsblokjes vaker dan 1 keer aangeboden. (14/3)
// .TAP file support (14/3: load)
// Geen tikken als geen geluid, is dat mogelijk? (26/2/96)
// Bij stilte niet naar SB schrijven.  Gebruiken om bij fixed pitch
//   minder geluid over te slaan. (25/2/96)
// SPD_CHUNCKY radio button wordt niet goed geset.  Heel vreemd. (24/2/96)
// ALT + TAB (24/2/96)
// Er zitten 'gaps' in de geluid output; na enige tijd, bv in attrs van schermpje
//  (24/2/96)
// Andere video modes: 4 bit en 3 byte (11/2)
//  IntelliIn werkt nog niet naar behoren, wel een beetje.
//  Laden geeft ook verkeerde tijd aan, af en toe.  Tot tijd is nog steeds
//  te kort (of looptijd te lang).  Pauzes schijnen te worden overgeslagen. (11/2)
// Laden van VOC files werkt nog niet goed. ALIEN8.VOC geeft tape error. (11/2)
// Printer moet Abort knop hebben. Send Form Feed moet Print pages (n)
//  worden.  Printen moet sneller; werken met bitmaps? (11/2)
// Spiegelkarakters in veel kleuren modes (18/1/96)
// Bright kleuren niet goed in veel kleuren modes (18/1/96)
// Rasperig geluid, en tikken (18/1/96)
// Achtergrondkleurtje van pijltje bij speedadjust (18/1/96)
// Op langzame computers meer tijd vrijgeven (18/1/96)
// Timing werkt niet goed; \u.z80 doet te lang over 20ms, soms 2x20ms voor
//    een interrupt. (19/12)
// BIT met nieuwe S flag schijnt niet goed te werken (klaar; 10/12)
// Na file load + cancel wordt scherm niet geupdate (Wel; 10/12)
// Border (Klaar, zowat; 16/12)
// Groter scherm (gedaan; 9/12)
// Video code in assembly (gedaan; 8/12)
// Keyboard handling (gedaan; 8/12)
// Joysticks geimplementeerd (9/12)
// Floating bus geimplementeerd (9/12)

// Grote projecten:
//
// HELP systeem
// Geintegreerde debugger
// Snelle geheugenswapper
// Disciple emulatie
// zx81 emulatie

#define spd_xw 96             // size of speed dialog windows
#define spd_yw 138
//#define scrb_x 81             // pos and size of scroll bar
#define scrb_x 78
#define scrb_y 6
#define scrb_xw 11
#define scrb_yw 128           // these are copied from SPEEDDIALOG, spectrum.rc

#define wait10 1
#define wait1 2
#define bump 3
#define smallbump 4
#define up 5
#define okay 6
#define sright 7
#define qleft 8
#define qright 9
#define wait2 10
#define newtoninit 11
#define newton 12
#define newtoninit2 13
#define mousenewtoninit 14
#define mousenewton 15
#define newtoninit3 16

char		anim1[] = {wait10,up,mousenewtoninit,mousenewton,newtoninit3,newton,okay};
char		anim2[] = {wait10,up,newtoninit2,newton,okay};
char		anim3[] = {wait10,up,newtoninit,newton,okay};
char		anim4[] = {wait10,up,sright,wait1,qleft,wait2,qright,okay};
char		anim5[] = {wait10,bump,bump,bump,bump,bump,bump,bump,smallbump,wait1,up,okay};
char		*animation;
int		animptr;
long		animstarttime;
int		animx;
int		animy;
char		animdone;

#define debugmsg 0

displaytype display;
displaytype hdisplay;         // for help screen
HWND		hWndMain=NULL,hWndHelp=NULL;
RECT     InvalidDialogRect;   // Used to get around bug in Windows
HANDLE   ghInstance;
HWND		hPrevWndMain;
int      hTimer;
/* begin add/modify by jts 4/3/97 */
int		gWinG;
/* end add/modify by jts 4/3/97 */
char		gszFilter[300];
char		gszAppName[]="WinZ80";
char     gszUDIni[]="WinZ80 - User settings";
char		gszIniFile[256];
char		gszRomFile[128];
char		gszZ80File[128];		// Used to pass snapshot file around
/* added by jts 4/8/97 */	char	gszDefaultFile[128];
char		gszZ80FileDir[128];
char     gszSampleFile[128];
char     gszInFile[128];
char     gszInFileDir[128];
char     gszPlayTapFile[128];
char     gszRecTapFile[128];
char		gszScrDir[128];
char     gszTapDir[128];
char		gszHelpFile[128];
char		gszMdrvDir[128];
char		gszRsDir[128];
char     gszLayoutFile[128];
char		gszCurrahFile[128];
char     gszLabelFile[128];
char		gszScrFile[128];
char		gszVz80dName[128];
char		gszRom48k[128],gszRomMface[128],gszRomCurrah[128],gszRomIf1[128],
				gszRom1281[128],gszRom1282[128];
char		gszErrorLogFile[128];
BOOL		romif1_8k;
WORD     loadsampletype,savesampletype,loadz80type;
int		extensionsub;			// sub-type counter for extension; output of FileDlg
int		iFileDlgFlg;			// -1 = nieuwe tap/tzx, of FE_APPEND / FE_OVERWRITE
int      Z80FileSelection=0;  // Holds last selection
int      InFileSelection=0;
int      SampleFileSelection=0;
int      TapFileSelection=0;
long     SampleBufLen;
HGLOBAL  hSampleBuf;
long     TotalRecorded;
char     outval;
char     gszLogFile[128];
char     gszInitSnapFile[128];
HBRUSH	hbrBackground;
FILE		*FDebug=NULL;

#define maxwindows 30
int		iWndIdent[maxwindows]={-2,WIN_AIB,WIN_LD,WIN_TSB,WIN_QN,CM_PLAYSAMPLE,CM_PLAYTAP,CM_RECORD,CM_RECTAP,
	CM_INFO,CM_ALTROMS,CM_DEFKEYS,CM_MDRV,CM_RS,CM_SO,CM_CONTROL,CM_SPEED,CM_DEBUG,CM_HARDWARE,-1};
HWND		hWndHandle[maxwindows];
long		lWndPos[maxwindows];
int		iNumWindows;

int      loadinitsnap;
int      fontpts;             // docs say 8, on my Windows only 10 works
int      oldspdptr=-1;        // to stop sending msgs to speeddlg too early
STATE		state;
// BYTE		hmode;				// actually defined in CORE.ASM
// WORD     sound;            // actually defined in CORE.ASM
char     CatchKeys;
char		PassSysCommand=FALSE;
HWND		hDebugMsg=NULL;
HWND     hSpeedDialog;
FARPROC  lpfnSpeedDialog;
HWND     hRecordDialog;
FARPROC  lpfnRecordProc;
HWND     hCtrlDialog;
FARPROC  lpfnCtrlDialog;
HWND     hJoyDialog;
FARPROC  lpfnJoyDialog;
HWND     hPlaySample;
FARPROC  lpfnPlaySampleProc;
HWND     hPlayTapDialog;
FARPROC  lpfnPlayTapProc;
HWND     hRecTapDialog;
FARPROC  lpfnRecTapProc;
HWND     hHardwareDialog;
FARPROC  lpfnHardwareProc;
HWND     hQNDlg;
FARPROC  lpfnQNDlg;
HWND		hMdrvDialog;
FARPROC	lpfnMdrvDialog;
HWND		hRsDialog;
FARPROC	lpfnRsDialog;
HWND		hRomDialog;
FARPROC	lpfnRomProc;
HWND		hSODialog;
FARPROC	lpfnSOProc;
HWND		hInfoBox;
HGLOBAL	hInfoMemory;
HWND     hAsmInfoBox;
HGLOBAL  hAsmInfoMemory;
long     QNTime;
HMENU    hEmptyMenu;
int      HelpScreenBorderColour;
BOOL		BWCCMessageBoxes;

BOOL        Shareware=1;
BOOL		Played5Minutes=FALSE;
HWND		hWndShareware;
HWND		hOk;
long		SharewareSessionTime;
LPSTR		glpszCmdLn;
RECT		rect;
HDC		hAll;

#include <alloc.h>

int pascal WinMain(HANDLE hInstance, HANDLE hPrev, LPSTR lpszCmdLn, int nCmdShow)
{
	MSG msg;
	LONG bc;
	HKEY hkey;
	int i;

	ghInstance = hInstance;
	BWCCMessageBoxes = FALSE;		// to make MyMessageBox work

	if (RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\GertonLunter\\WinZ80\\400",&hkey)
		 != ERROR_SUCCESS) {
		 // Assume this is windows 3.1 then
		 strcpy(gszIniFile,"winz80.ini");
//		 MyMessageBox(NULL,"WinZ80 not properly installed!  Run installation program again.","WinZ80 fatal error",MB_ICONHAND|MB_OK);
//		return 0;
	} else {
		bc = 256;
		RegQueryValue(hkey,NULL,&gszIniFile,&bc);
	}

	if (hPrev) {
		GetInstanceData(hPrev, &hPrevWndMain, sizeof(HWND));
		SendMessage(hPrevWndMain,CM_OTHERINSTANCEFILE,0,lpszCmdLn);
//		MyMessageBox(NULL,"Another copy of WinZ80 already running!","WinZ80 fatal error:",MB_ICONHAND|MB_OK);
		return(0);
	}
	initfirst();
//	if (!hPrev)	initfirst();

// Nu de shareware-onzin

	SharewareSessionTime = -1;

	i = GetTickCount() % 5;
	switch (i) {
	case 0: animation = anim1; break;
	case 1: animation = anim2; break;
	case 2: animation = anim3; break;
	case 3: animation = anim4; break;
	case 4: animation = anim5; break;
	}
	animptr = 0;
	animx = -100;
	animy = shw_y - 200;
	animdone = FALSE;
	hWndMain = NULL;
	hAll=GetDC(NULL);
	GetClipBox(hAll,&rect);
	Shareware = 1;
	ReleaseDC(NULL,hAll);
	hWndShareware = CreateWindow(
			gszAppName,
			NULL,
//			WS_OVERLAPPEDWINDOW,
			WS_BORDER,
			(rect.right+rect.left-shw_x)/2,
			(rect.top+rect.bottom-shw_y)/2,
			shw_x,
			shw_y,
			HWND_DESKTOP,
			NULL,
			ghInstance,
			NULL);
	if (!hWndShareware) {
		MyMessageBox(NULL,"Could not create window!","WinZ80 fatal error:",MB_ICONHAND|MB_OK);
		return 0;
	}
	//	SetClassLong(hWndShareware,GCL_MENUNAME,NULL);		// is re-set inside wndproc
	SetMenu(hWndShareware,CreateMenu());
	hOk = CreateWindow(
		"BUTTON",
		"ok",
		WS_CHILD,
		-okay_x-10,
		shw_y - 200,
		okay_x,
		okay_y,
		hWndShareware,
		NULL,
		ghInstance,
		NULL);
	hTimer = SetTimer(hWndShareware,0,50,NULL);
	if (!hTimer) {
		fatalerror(FatalNoTimersLeft);
		return 0;
	}
	// nonsense to get msg loop running while no hWndMain is up
	hPlaySample = NULL;
	hInfoBox = NULL;
	hDebugDialog = NULL;
	hRsDialog = NULL;
	hGifDlg = NULL;

	// Now either run intro thing or start
	glpszCmdLn = lpszCmdLn;		// see start of main message procedure
	ShowWindow(hWndShareware, SW_SHOW);
	ShowWindow(hOk, SW_SHOW);
	while (GetMessage(&msg,NULL,0,0)) {
//		if (((hPlaySample|hRecordDialog)==NULL)||
//          ((msg.message>=WM_KEYFIRST)&&(msg.message<=WM_KEYLAST))||
//          ((!IsDialogMessage(hPlaySample,&msg))&&
//            (!IsDialogMessage(hRecordDialog,&msg)))) {
		if ( ((hPlaySample==NULL)||(!IsDialogMessage(hPlaySample,&msg))) &&
			  ((hInfoBox==NULL)||(!IsDialogMessage(hInfoBox,&msg)))  )
		{
			HWND hAct = GetActiveWindow();
			if (((hAct == hDebugDialog)&&(hDebugDialog)) ||
				 ((hAct == hRsDialog)&&(hRsDialog)) ||
				 ((hAct == hGifDlg)&&(hGifDlg))) {
				TranslateMessage(&msg);
				if ( (msg.hwnd == hDebugInputWindow) &&
					  ((msg.message==WM_CHAR)&&(msg.wParam==VK_RETURN)&&((msg.lParam & 0x80000000L) == 0)) ||
					  ((msg.message==WM_KEYDOWN)&&
						((msg.wParam==VK_PRIOR)||
						 (msg.wParam==VK_NEXT)||
						 (msg.wParam==VK_UP)||
						 (msg.wParam==VK_DOWN)||
						 ((msg.wParam>=VK_F1) && (msg.wParam<=VK_F10)))
					  )
					) {
							msg.message = WM_CHAR;
							msg.hwnd = hDebugDialog;				// re-route to dialog procedure
				}
			}
			else if ((msg.message>=WM_KEYFIRST)&&(msg.message<=WM_KEYLAST)) {
				TranslateKbd(msg.wParam,msg.lParam,msg.message);
			}
			DispatchMessage(&msg);
		}
	}
	return (msg.wParam);
}

void initfirst()
{
	WNDCLASS wndclass;

	wndclass.style = CS_BYTEALIGNCLIENT | CS_DBLCLKS | CS_HREDRAW |
						  CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = ghInstance;
	wndclass.hIcon = LoadIcon(ghInstance,"z79icon");
	wndclass.hCursor = LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground = NULL;		// No background
	wndclass.lpszMenuName = "MAINMENU";
	wndclass.lpszClassName = gszAppName;
	if (!RegisterClass (&wndclass)) {
		MyMessageBox(NULL,"Could not register Window class!","WinZ80 fatal error:",MB_ICONHAND|MB_OK);
	}
	BWCCRegister(ghInstance);
}

void initall(int cmdShow)
{
	int i,j;
	int loadmsg;
	char chReplace;
	long temppos;
	hWndMain = CreateWindow(
		gszAppName,
		"Z80 for Windows",
//		WS_OVERLAPPEDWINDOW,
		WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		256*display.Xfac+2*display.borsize,
		192*display.Yfac+2*display.borsize,
		HWND_DESKTOP,
		NULL,
		ghInstance,
		NULL);
	if (!hWndMain) {
		MyMessageBox(NULL,"Could not create window!","WinZ80 fatal error:",MB_ICONHAND|MB_OK);
		PostQuitMessage(0);
		return;
	}
	hPrevWndMain = hWndMain;	// For GetInstanceData call
	hInfoMemory=NULL;
	hAsmInfoBox=NULL;
	hAsmInfoMemory=NULL;
	WriteInfoString("\t           -----WinZ80-----\r\n   Sinclair ZX Spectrum emulator for Windows\r\n          Version 4.00, (c) 1999 Gerton Lunter\r\n\r\n*******************************************************\r\n");
	WriteInfoString("Shareware version.\r\n");
	WriteInfoString("(Run-time messages follow:)");
	{
		char s[256];
		sprintf(s,"Using .ini file `%s'",gszIniFile);
		WriteInfoString(s);
	}

	for (i=0;iWndIdent[i]!=-1;i++) {}
	iNumWindows=i;
	ReadInitValues();
	IdInit();
	hbrBackground = CreateSolidBrush(GetSysColor(MyBackgroundColor));
	state.logging=FALSE;
	state.paused=FALSE;
	feearmicmask=0;
//	ToggleMenu(hWndMain,CM_LISTENMIC,&(char)feearmicmask);
	feearmicmask=16-8*feearmicmask;
	hdisplay.borsize=16;
	if (init_video()) return;
	InitKbd();
	initload();
	initsound();
	ResetAY();
	currah_init();
	currah_reset();
	AllocBuffers();      // for sound
	initinning();
	initprinter();
	if (InitEmulator()) return;
	hTimer = SetTimer(hWndMain,0,50,NULL);
	if (!hTimer) {
		fatalerror(FatalNoTimersLeft);
		return;
	}
	InstallSettings();
	LoadLabels();
	i=display.Xfac;
	j=display.Yfac;
	loadmsg=0;
	if (loadinitsnap) {
		HFILE handle;
		handle=OpenRead(gszInitSnapFile);
		if (!handle) {
			loadmsg=LoadZ80Error;
		} else {
			loadmsg=LoadZ80File(handle);
			_lclose(handle);
			if (loadmsg) {
				Reset();
			}
		}
	}
	InvalidateVidBuffer(SpecMem);
	temppos = lWndPos[0];
	ShowWindow(hWndMain, cmdShow);
//	SendMessage(hWndMain,WM_SIZE,SIZENORMAL,
//		0xC00000L*j+256*i);
	SetWindowPos(hWndMain,NULL,0,0,(256+16)*i,(192+16)*j,SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
	if (loadmsg) notify(loadmsg);			// must be after ShowWindow, that's why
	hSpeedDialog=NULL;
	hRecordDialog=NULL;
	hCtrlDialog=NULL;
	hJoyDialog=NULL;
	hWndHelp=NULL;
	hSampleBuf=NULL;
	hHardwareDialog=NULL;
	hInfoBox=NULL;
	SampleBufLen=0;
	CatchKeys=FALSE;
	loadsampletype=savesampletype=loadz80type=LZF_VOC;    // i.e. VOC or Z80
	lWndPos[0]=temppos;
	DoDefaultWindowPositions();
	DragAcceptFiles(hWndMain,TRUE);
/* added by jts 4/2/97 */
//	if ((i = LoadString(ghInstance, IDS_FILTERSTRING, gszFilter, sizeof(gszFilter))) == 0)
//	{
//		fatalerror(FatalNoFilterString);
//		return;
//	}
	strcpy(gszFilter,"Snapshots (z80,slt,sna)|*.z80;*.sna;*.slt|Sound (voc,raw)|*.voc;*.raw|Tapes (tap,tzx)|*.tap;*.tzx|All Spectrum files|*.z80;*.slt;*.sna;*.raw;*.voc;*.tap;*.tzx;*.mdr;*.sav;*.rom;*.scr;*.gif|Cartridges (mdr)|*.mdr|RS232 files (sav)|*.sav|ROMs (rom)|*.rom|Screenshots (scr,gif)|*.scr;*.gif|");
	i = strlen(gszFilter);
	chReplace = gszFilter[i - 1];
	for (i = 0; gszFilter[i] != '\0'; i++)
	{
		if (gszFilter[i] == chReplace)	gszFilter[i] = '\0';
	}
/* end added by jts 4/2/97 */
	#if debugmsg == 1
	hDebugMsg = CreateDialog(ghInstance,"DEBUGMSG",hWndMain,
		MyMakeProcInstance(DebugMsgProc,ghInstance));
	#endif
}

void DoDefaultWindowPositions()
{
	int i;
	if (lWndPos[0]!=-1)
		SetWindowPos(hWndMain,0,LOWORD(lWndPos[0]),HIWORD(lWndPos[0]),0,0,SWP_NOSIZE|SWP_NOZORDER);
	for (i=5;i<iNumWindows;i++) {
		if (hWndHandle[i]) {
			PostMessage(hWndMain,WM_COMMAND,iWndIdent[i],0);
		}
	}
}

void WRITEDEBUG(WORD w1,WORD w2,WORD w3, WORD w4, WORD w5)
{
	if (!FDebug) FDebug=fopen("c:\\debug","w");
	fprintf(FDebug,"%4x %4x %4x %4x %4x\n",w1,w2,w3,w4,w5);
}

void WRITEDEBUGMSG(const char* msg,int n)
{
	if (!hDebugMsg) return;
	SetDlgItemText(hDebugMsg,DEBUG1-1+n,msg);
}

// MyDlgProc: default msg procc func for dialog procedures.  Paints background
// color.  Keeps track of position. Sets first position
BOOL MyDlgProc (HWND hDlg,WORD wMess,WORD wPar,LONG lPar)
{
	int i;
	switch (wMess) {
	case WM_CTLCOLOR:
		if ((int)(HIWORD(lPar)) != CTLCOLOR_EDIT) {
			SetBkColor( (HDC)wPar, GetSysColor(MyBackgroundColor) );
			SetTextColor( (HDC)wPar, GetSysColor(MyForegroundColor) );
			return (LRESULT) hbrBackground;
		} else {
			return (LRESULT) NULL;
		}
	case WM_WINDOWPOSCHANGED:
		for (i=0;i<iNumWindows;i++) {
			if (hWndHandle[i]==hDlg) {
				lWndPos[i] = ((long)((LPWINDOWPOS)lPar)->x) +
								(((long)((LPWINDOWPOS)lPar)->y) << 16);
				i=iNumWindows;
			}
		}
		return FALSE;
	case WM_DESTROY:
		for (i=0;i<iNumWindows;i++)
			if (hWndHandle[i]==hDlg) {
				hWndHandle[i]=0;
				i=iNumWindows;
			}
		return TRUE;
	case WM_INITDIALOG:
		for (i=0;i<iNumWindows;i++) {
			if (iWndIdent[i]==(int)lPar) {
				if (lWndPos[i] != -1) {
					SetWindowPos(hDlg,0,LOWORD(lWndPos[i]),HIWORD(lWndPos[i]),0,0,SWP_NOSIZE|SWP_NOZORDER);
				}
				hWndHandle[i]=hDlg;
				i=iNumWindows;
			}
		}
		return TRUE;
	}
	return FALSE;
}


BOOL CALLBACK DebugMsgProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	switch (wMess) {
	case WM_INITDIALOG:
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return FALSE;
	case WM_DESTROY:
		hDebugMsg=NULL;
		return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


HWND MyCreateDialogParam(HINSTANCE hinst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
	HWND hWnd;
	DWORD style;
	hWnd = CreateDialogParam(hinst,lpszDlgTemp,HWND_DESKTOP,dlgprc,lParamInit);
//	SetClassWord(hWnd,GCW_HICON,LoadIcon(ghInstance,"z80icon"));
//	style = GetWindowLong(hWnd,GWL_STYLE);
//	style = (~WS_POPUP) & style;
//	SetWindowLong(hWnd,GWL_STYLE,style);
	return hWnd;
}

FARPROC MyMakeProcInstance(FARPROC proc, HINSTANCE hInst)
{
	return proc;
}

void MyFreeProcInstance(FARPROC proc)
{
	return;
}

void WriteInfoString(char *str)
{
	char *p;
	int size;
	if (!hInfoMemory) {
		hInfoMemory=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 10);
	}
	p = GlobalLock(hInfoMemory);
	size = strlen(p);
	GlobalUnlock(hInfoMemory);
	hInfoMemory = GlobalReAlloc(hInfoMemory,size + strlen(str) + 10,GMEM_MOVEABLE);
	p = GlobalLock(hInfoMemory);
	sprintf(p+size,"%s\r\n",str);
	GlobalUnlock(hInfoMemory);
}

//BOOL SetRegistry()
//{
//	HKEY h;
//	char path[256];
//	FILE* fin;
//	fin = fopen("winz80.exe","r");
//	if (fin) {
//		fclose(fin);
//	} else {
//		MyMessageBox(NULL,"Cannot find WinZ80.exe in current directory, and WinZ80 is not yet installed!  Change to home dir and start program again.","WinZ80 fatal error:",MB_ICONHAND|MB_OK);
//		SendMessage(hWndMain,WM_CLOSE,0,0L);
//		return FALSE;
//	}
//	_fullpath(path,".\\winz80.ini",256);
//	RegCreateKey(HKEY_LOCAL_MACHINE,"Software\\Software voor de leuk\\WinZ80\\400",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//	_fullpath(path,"winz80.exe",256);
//	strcat(path," %1");
//	RegSetValue(HKEY_CLASSES_ROOT,".slt",REG_SZ,"WinZ80.snap",0);
//	RegSetValue(HKEY_CLASSES_ROOT,".sna",REG_SZ,"WinZ80.snap",0);
//	RegSetValue(HKEY_CLASSES_ROOT,".z80",REG_SZ,"WinZ80.snap",0);
//	RegSetValue(HKEY_CLASSES_ROOT,"WinZ80.snap",REG_SZ,"WinZ80 snapshot",0);
//	RegCreateKey(HKEY_CLASSES_ROOT,"WinZ80.snap\\shell\\open\\command",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//	RegSetValue(HKEY_CLASSES_ROOT,".tap",REG_SZ,"WinZ80.tape",0);
//	RegSetValue(HKEY_CLASSES_ROOT,".tzx",REG_SZ,"WinZ80.tape",0);
//	RegSetValue(HKEY_CLASSES_ROOT,"WinZ80.tape",REG_SZ,"WinZ80 tape file",0);
//	RegCreateKey(HKEY_CLASSES_ROOT,"WinZ80.tape\\shell\\open\\command",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//	RegSetValue(HKEY_CLASSES_ROOT,".mdr",REG_SZ,"WinZ80.mdrv",0);
//	RegSetValue(HKEY_CLASSES_ROOT,"WinZ80.mdrv",REG_SZ,"WinZ80 microdrive file",0);
//	RegCreateKey(HKEY_CLASSES_ROOT,"WinZ80.mdrv\\shell\\open\\command",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//
//	_fullpath(path,"winz80.exe",256);
//	strcat(path,",0");
//	RegCreateKey(HKEY_CLASSES_ROOT,"WinZ80.mdrv\\DefaultIcon",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//	_fullpath(path,"winz80.exe",256);
//	strcat(path,",1");
//	RegCreateKey(HKEY_CLASSES_ROOT,"WinZ80.tape\\DefaultIcon",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//	_fullpath(path,"winz80.exe",256);
//	strcat(path,",2");
//	RegCreateKey(HKEY_CLASSES_ROOT,"WinZ80.snap\\DefaultIcon",&h);
//	RegSetValue(h,NULL,REG_SZ,path,0);
//	RegCloseKey(h);
//	return TRUE;
//}


void ReadInitValues(void)
{
// Read in values, toggle twice and update menu checks
	int version;
	int i;
	char path[256];

	#define gppi(name,var,val) var=GetPrivateProfileInt(gszAppName,name,val,gszIniFile)
	#define gppiu(name,var,val) var=GetPrivateProfileInt(gszUDIni,name,val,gszIniFile)
	issue2=!GetPrivateProfileInt(gszAppName,"Issue2",0,gszIniFile);
	state.sound=!GetPrivateProfileInt(gszAppName,"Sound",1,gszIniFile);
	state.coppering=!GetPrivateProfileInt(gszAppName,"Coppering",1,gszIniFile);
	display.borsize=!GetPrivateProfileInt(gszAppName,"LargeBorder",0,gszIniFile);
	state.truepitch=!GetPrivateProfileInt(gszAppName,"TruePitch",0,gszIniFile);
	gppi("IntelliIn",iimode,1);  iimode=!iimode;
	gppi("ZxPrinter",zxprinter,1);  zxprinter=!zxprinter;
	gppi("SHIFT128",bAlways128kshift,1);  bAlways128kshift=!bAlways128kshift;
	ToggleMenu(hWndMain,CM_ISSUE2,&issue2);
	ToggleMenu(hWndMain,CM_SOUND,(char*)&state.sound);
	ToggleMenu(hWndMain,CM_COPPERING,&state.coppering);
	ToggleMenu(hWndMain,CM_LARGEBORDER,&(char)display.borsize);
	ToggleMenu(hWndMain,CM_PITCH,&(char)state.truepitch);
	ToggleMenu(hWndMain,CM_INTELLIIN,&(char)iimode);
	ToggleMenu(hWndMain,CM_ZXPRINTER,&(char)zxprinter);
	ToggleMenu(hWndMain,CM_128SHIFT,&(char)bAlways128kshift);
	sound=state.sound;
	GetPrivateProfileString(gszUDIni,"InitSnapFile","",gszInitSnapFile,100,gszIniFile);
	loadinitsnap=GetPrivateProfileInt(gszUDIni,"LoadInitSnap",0,gszIniFile);
	display.borsize=16+16*display.borsize;
	gppiu("SoundShutUpDelay",ShutUpDelay,250);
	if (!Played5Minutes)
		gppi("InitialSpeed",state.speed,100);
	else
		state.speed = shw_speed;
	gppi("TapeMaxLoadSpeed",TapeMaxLoadSpeed,0);
	gppi("VIDSTATE",state.vidstate,1);
	gppi("FASTEST",state.fastest,0);
//	gppiu("PRINTERYSIZE",printerysize,800);
	gppiu("PRINTERHSCALE",printerhscale,50);
	gppiu("PRINTERVSCALE",printervscale,50);
	gppiu("PRINTERLEFTMARGIN",printerleftmargin,32);
	gppiu("PRINTERMULTICOLUMN",multicolumn,1);
	gppiu("OWN2x2VIDEO",Own2x2Video,0);
	gppiu("UseWinG",gWinG,1);
	gppiu("VZ80D",IniUseVz80d,1);
	gppi("RESETATHDWCHANGE",state.resetathchange,1);
	gppi("multifaceemulated",state.multifaceemulated,0);
	gppi("currahemulated",state.currahemulated,0);
	gppi("hmode",hmode,hm_48k);
	gppi("ayemu48k",state.ayemu48k,0);
	gppi("specdrumemulated",state.specdrumemu,0);
	gppiu("blackandwhite",state.blackandwhite,0);
	display.Xfac=GetPrivateProfileInt(gszAppName,"XFAC",2,gszIniFile);
	display.Yfac=GetPrivateProfileInt(gszAppName,"YFAC",2,gszIniFile);
	hdisplay.Xfac=GetPrivateProfileInt(gszAppName,"HELPXFAC",2,gszIniFile);
	hdisplay.Yfac=GetPrivateProfileInt(gszAppName,"HELPYFAC",2,gszIniFile);
	HelpScreenBorderColour=GetPrivateProfileInt(gszUDIni,"HELPBORDERCOLOUR",0,gszIniFile);
//	fontpts=GetPrivateProfileInt(gszUDIni,"FONTPOINTSIZE",10,gszIniFile);
	fontpts=10;
	state.maxnoscreenrefresh=GetPrivateProfileInt(
		gszAppName,"MaxNumberOfNonScreenRefreshCycles",10,gszIniFile);
	_fullpath(path,"roms.bin",256);
	GetPrivateProfileString(gszUDIni,"ROMFILE",path,gszRomFile,80,gszIniFile);
	_fullpath(path,"layout.scr",256);
	GetPrivateProfileString(gszUDIni,"LAYOUTFILE",path,gszLayoutFile,100,gszIniFile);
	_fullpath(path,"labels.asm",256);
	GetPrivateProfileString(gszUDIni,"LABELFILE",path,gszLabelFile,100,gszIniFile);
	_fullpath(path,"currah.dat",256);
	GetPrivateProfileString(gszUDIni,"CurrahFile",path,gszCurrahFile,100,gszIniFile);
	_fullpath(path,"winz80.hlp",256);
	GetPrivateProfileString(gszUDIni,"HelpFile",path,gszHelpFile,100,gszIniFile);
	_fullpath(path,"vz80d.vxd",256);
	GetPrivateProfileString(gszUDIni,"VZ80DFILE",path,gszVz80dName,128,gszIniFile);
	GetPrivateProfileString(gszAppName,"SNAPSHOTDIR","",gszZ80FileDir,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"LOGFILE","log.out",gszLogFile,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"SAMPLEFILEDIR","",gszInFileDir,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"TAPEFILEDIR","",gszTapDir,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"MDRFILEDIR","",gszMdrvDir,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"RSFILEDIR","",gszRsDir,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"SCRDIR","",gszScrDir,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"AlternateRom48k","",gszRom48k,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"AlternateRomMFace","",gszRomMface,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"AlternateRomCurrah","",gszRomCurrah,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"AlternateRomIf1","",gszRomIf1,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"AlternateRom1281","",gszRom1281,100,gszIniFile);
	GetPrivateProfileString(gszAppName,"AlternateRom1282","",gszRom1282,100,gszIniFile);
	gppi("Knop1",iSoundKnobs[0],15);
	gppi("Knop2",iSoundKnobs[1],15);
	gppi("Knop3",iSoundKnobs[2],0);
	gppi("Knop4",iSoundKnobs[3],15);
	gppi("Knop5",iSoundKnobs[4],7);
	gppi("Knop6",iSoundKnobs[5],7);
	gppi("Knop7",iSoundKnobs[6],15);
	gppi("AlternateRomIf18k",romif1_8k,1);
	gppi("Version",version,0);
	gppiu("BWCCMessageBoxes",BWCCMessageBoxes,1);
	gppi("TzxVerboseList",TzxVerbose,0);
	GetPrivateProfileString(gszUDIni,"ErrorLogFile","",gszErrorLogFile,100,gszIniFile);
	for (i=0;i<iNumWindows;i++) {
		char str[80];
		sprintf(str,"WinData%x",iWndIdent[i]);
		GetPrivateProfileString(gszAppName,str,"-1,0",str,80,gszIniFile);
		sscanf(str,"%ld,%u",&lWndPos[i],&hWndHandle[i]);
	}
	if (version==0) {
		WriteInfoString("Writing default WinZ80.INI file");
		SaveSettings();		// If no INI file, make one.
	}
}

void SaveSettings(void)
{
	#define wppix(name,nmbr,file) sprintf(temp,"%d",nmbr);WritePrivateProfileString \
		(file,name,temp,gszIniFile);
	#define wppi(name,nmbr) wppix(name,nmbr,gszAppName)
	#define wppiu(name,nmbr) wppix(name,nmbr,gszUDIni)
	char temp[50];
	int i;
	WritePrivateProfileString(gszUDIni,"RomFile",gszRomFile,gszIniFile);
	WritePrivateProfileString(gszUDIni,"HelpFile",gszHelpFile,gszIniFile);
	WritePrivateProfileString(gszUDIni,"LayoutFile",gszLayoutFile,gszIniFile);
	WritePrivateProfileString(gszUDIni,"LabelFile",gszLabelFile,gszIniFile);
	WritePrivateProfileString(gszUDIni,"CurrahFile",gszCurrahFile,gszIniFile);
	WritePrivateProfileString(gszUDIni,"Vz80DFile",gszVz80dName,gszIniFile);
	WritePrivateProfileString(gszUDIni,"InitSnapFile",gszInitSnapFile,gszIniFile);
	wppiu("LoadInitSnap",loadinitsnap);
	WritePrivateProfileString(gszUDIni,"LogFile",gszLogFile,gszIniFile);
	WritePrivateProfileString(gszUDIni,"ErrorLogFile",gszErrorLogFile,gszIniFile);
	wppiu("PrinterHScale",printerhscale);
	wppiu("PrinterVScale",printervscale);
	wppiu("PrinterLeftMargin",printerleftmargin);
	wppiu("PrinterMultiColumn",multicolumn);
	wppiu("HelpBorderColour",HelpScreenBorderColour);
	wppiu("SoundShutUpDelay",ShutUpDelay);
//	wppiu("PrinterYSize",printerysize);
//	wppiu("FontPointSize",fontpts);
	wppiu("Own2x2Video",Own2x2Video);
	wppiu("UseWinG",gWinG);
	wppiu("VZ80D",IniUseVz80d);
	wppiu("BWCCMessageBoxes",BWCCMessageBoxes);
	wppi("Version",400);
	WritePrivateProfileString(gszAppName,"Issue2",issue2?"1":"0",gszIniFile);
	WritePrivateProfileString(gszAppName,"Sound",state.sound?"1":"0",gszIniFile);
	WritePrivateProfileString(gszAppName,"Coppering",state.coppering?"1":"0",gszIniFile);
	WritePrivateProfileString(gszAppName,"LargeBorder",display.borsize==32?"1":"0",gszIniFile);
	WritePrivateProfileString(gszUDIni,"BlackAndWhite",state.blackandwhite?"1":"0",gszIniFile);
	WritePrivateProfileString(gszAppName,"SnapShotDir",gszZ80FileDir,gszIniFile);
	WritePrivateProfileString(gszAppName,"SampleFileDir",gszInFileDir,gszIniFile);
	WritePrivateProfileString(gszAppName,"TapeFileDir",gszTapDir,gszIniFile);
	WritePrivateProfileString(gszAppName,"MdrFileDir",gszMdrvDir,gszIniFile);
	WritePrivateProfileString(gszAppName,"RsFileDir",gszRsDir,gszIniFile);
	WritePrivateProfileString(gszAppName,"ScrDir",gszScrDir,gszIniFile);
	WritePrivateProfileString(gszAppName,"AlternateRom48k",gszRom48k,gszIniFile);
	WritePrivateProfileString(gszAppName,"AlternateRomMFace",gszRomMface,gszIniFile);
	WritePrivateProfileString(gszAppName,"AlternateRomCurrah",gszRomCurrah,gszIniFile);
	WritePrivateProfileString(gszAppName,"AlternateRomIf1",gszRomIf1,gszIniFile);
	wppi("AlternateRomIf18k",romif1_8k);
	WritePrivateProfileString(gszAppName,"AlternateRom1281",gszRom1281,gszIniFile);
	WritePrivateProfileString(gszAppName,"AlternateRom1282",gszRom1282,gszIniFile);
	sprintf(temp,"%d",state.speed);
	WritePrivateProfileString(gszAppName,"InitialSpeed",temp,gszIniFile);
	wppi("VidState",state.vidstate);
	wppi("Fastest",state.fastest);
	wppi("TapeMaxLoadSpeed",TapeMaxLoadSpeed);
	wppi("TruePitch",state.truepitch);
	wppi("IntelliIn",!!iimode);
	wppi("ZxPrinter",zxprinter);
	wppi("ResetAtHdwChange",state.resetathchange);
	wppi("HMode",hmode);
	wppi("MultifaceEmulated",state.multifaceemulated);
	wppi("CurrahEmulated",state.currahemulated);
	wppi("SpecDRUMEmulated",state.specdrumemu);
	wppi("AyEmu48K",state.ayemu48k);
	wppi("Shift128",bAlways128kshift);
	wppi("TzxVerboseList",TzxVerbose);
	sprintf(temp,"%u",state.maxnoscreenrefresh);
	WritePrivateProfileString(gszAppName,"MaxNumberOfNonScreenRefreshCycles",temp,gszIniFile);
	sprintf(temp,"%u",display.Xfac);
	WritePrivateProfileString(gszAppName,"XFac",temp,gszIniFile);
	sprintf(temp,"%u",display.Yfac);
	WritePrivateProfileString(gszAppName,"YFac",temp,gszIniFile);
	sprintf(temp,"%u",display.borsize);
	wppi("Knop1",iSoundKnobs[0]);
	wppi("Knop2",iSoundKnobs[1]);
	wppi("Knop3",iSoundKnobs[2]);
	wppi("Knop4",iSoundKnobs[3]);
	wppi("Knop5",iSoundKnobs[4]);
	wppi("Knop6",iSoundKnobs[5]);
	wppi("Knop7",iSoundKnobs[6]);
	wppi("HelpXFac",hdisplay.Xfac);
	wppi("HelpYFac",hdisplay.Yfac);
	for (i=0;i<iNumWindows;i++) {
		char str[80],str2[80];
		sprintf(str,"WinData%x",iWndIdent[i]);
		sprintf(str2,"%ld,%u",lWndPos[i],!!hWndHandle[i]);
		WritePrivateProfileString(gszAppName,str,str2,gszIniFile);
	}
}


void fatalerror(int hMessage)
{
	char errmsg[300];
	state.paused=TRUE;
	LoadString(ghInstance, hMessage, errmsg, sizeof(errmsg));
	MyMessageBox(hWndMain,errmsg,"WinZ80 fatal error:",MB_ICONSTOP|MB_OK|MB_APPLMODAL);
	PostMessage(hWndMain,WM_CLOSE,0,0L);
}

void notify(int hMessage)
{
	char msg[300];
	LoadString(ghInstance,hMessage,msg,sizeof(msg));
	MyMessageBox(hWndMain,msg,"WinZ80 error:",MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
}

void message(int hMessage)
{
	char msg[300];
	LoadString(ghInstance,hMessage,msg,sizeof(msg));
	MyMessageBox(hWndMain,msg,"Attention:",MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
}

int MyMessageBox(HWND hparent, LPCSTR txt, LPCSTR title, UINT flags)
{
	HFILE h;
	if ((flags != MB_OK) && (gszErrorLogFile[0])) {
		h=_lopen(gszErrorLogFile,READ_WRITE);
		if (h==HFILE_ERROR) {
			h=_lcreat(gszErrorLogFile,0);
		}
		if (h!=HFILE_ERROR) {
			_llseek(h,0,2);
			_lwrite(h,title,strlen(title));
			_lwrite(h,txt,strlen(txt));
			_lwrite(h,&"\r\n",2);
			_lclose(h);
		}
	}
	if (BWCCMessageBoxes) {
		return BWCCMessageBox(hparent,txt,title,flags);
	} else {
		return MessageBox(hparent,txt,title,flags);
	}
}


BOOL CALLBACK QNDialogProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	static BOOL held;
	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		held=FALSE;
		break;
	case WM_CLOSE:
		if ((wPar==12345)&&held) return TRUE;
		DestroyWindow(hDlg);
		return FALSE;
	case WM_DESTROY:
		hQNDlg=0;
		QNTime=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnQNDlg);
		break;
	case WM_COMMAND:
		if (wPar==QN_HOLD) {
			if (held) SendMessage(hDlg,WM_CLOSE,0,0L);
			held=TRUE;
			SetDlgItemText(hDlg,QN_HOLD,"Close");
		}
		return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}

void quicknotify(char *string)
{
	if (!hQNDlg) {
		lpfnQNDlg=MyMakeProcInstance(QNDialogProc,ghInstance);
		hQNDlg=CreateDialogParam(ghInstance,"QNDIALOG",hWndMain,lpfnQNDlg,WIN_QN);
	}
	SendDlgItemMessage(hQNDlg,QN_TEXT,WM_SETTEXT,0,(LONG)string);
	QNTime=GetCurrentTime();
}


BOOL CALLBACK InfoBoxProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	char *p;
	switch (wMess) {
	case WM_INITDIALOG:
		if (hInfoMemory) {
			p=GlobalLock(hInfoMemory);
			SendDlgItemMessage(hDlg,INFO_EDIT,WM_SETTEXT,0,(DWORD)p);
			PostMessage(GetDlgItem(hDlg,INFO_EDIT),EM_SETSEL,-1,0);
			GlobalUnlock(hInfoMemory);
		}
		break;
	case WM_CREATE:
		wPar++;
		return wPar;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return FALSE;
	case WM_COMMAND:
		if (wPar==INFO_OK)
			SendMessage(hDlg,WM_CLOSE,0,0);
		return TRUE;
	case WM_DESTROY:
		hInfoBox=NULL;
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}



void HelpScreen(int toggle)
{
	int i,j;
	if (hWndHelp) {
		DestroyWindow(hWndHelp);
		hWndHelp=NULL;
		nohelpwindow:
		ModifyMenu(GetMenu(hWndMain),CM_HELPSCREEN,
			MF_BYCOMMAND|MF_ENABLED|MF_STRING,CM_HELPSCREEN,"Show keyboard lay-out");
		return;
	}
	if (!toggle) return;			// asked to destroy, but wasn''t there
	ModifyMenu(GetMenu(hWndMain),CM_HELPSCREEN,
		MF_BYCOMMAND|MF_ENABLED|MF_STRING,CM_HELPSCREEN,"Remove lay-out window");
	hEmptyMenu=LoadMenu(ghInstance,"EmptyMenu");    // destroyed by DestroyWindow
	hWndHelp = CreateWindow(
		gszAppName,
		"Keyboard lay-out",
		WS_THICKFRAME|WS_VISIBLE|WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		256*hdisplay.Xfac+2*hdisplay.borsize,
		192*hdisplay.Yfac+2*hdisplay.borsize,
		HWND_DESKTOP,
		hEmptyMenu,
		ghInstance,
		NULL);
	if (hWndHelp==NULL) goto nohelpwindow;
	i=hdisplay.Xfac;
	j=hdisplay.Yfac;
	ShowWindow(hWndHelp, SW_SHOWNA);
	SendMessage(hWndHelp,WM_SIZE,SIZENORMAL,
		0xC00000L*j+256*i);
}

#define check(x) (((x)?MF_CHECKED:MF_UNCHECKED)|MF_BYCOMMAND)

char ToggleMenu(HWND hWnd, WORD id, char *var)
{
	UINT i;
	*var=!*var;
	CheckMenuItem(GetMenu(hWnd),id,check(*var));
	i=GetMenuState(GetMenu(hWnd),id,MF_BYCOMMAND);
	i=i&MF_CHECKED;
	return(*var);
}

void SetJoystick(int currentjoystick)
{
	CheckMenuItem(GetMenu(hWndMain),CM_CURSOR,check(currentjoystick==0));
	CheckMenuItem(GetMenu(hWndMain),CM_KEMPSTON,check(currentjoystick==5));
	CheckMenuItem(GetMenu(hWndMain),CM_SINCLAIR,check(currentjoystick==15));
	CheckMenuItem(GetMenu(hWndMain),CM_USERDEFJOY,check(currentjoystick==10));
}


void YieldIfYeNeed(void)
{
	static DWORD lasttime=0;
	DWORD now=GetCurrentTime();
	int i;
	MSG msg;

	if (now-lasttime<250) return;
	if (now-lasttime>10000) {
		lasttime=now;
		return;
	}
	lasttime=now;  // now holds time of calling, lasttime holds current time
	do {
		i=PeekMessage(&msg,hWndMain,0,0,PM_REMOVE);
		if (i) DispatchMessage(&msg);
		lasttime=GetCurrentTime();
	} while (i && (lasttime-now<250));
}

HGLOBAL RepaintData(HWND hDlg)
{
	HGLOBAL hmem;
	REPAINTDATA *rd;
	HDC hdc;
	RECT r;
	hmem=GlobalAlloc(GMEM_MOVEABLE,sizeof(REPAINTDATA));
	if (hmem==NULL) return hmem;
	rd=(REPAINTDATA*)GlobalLock(hmem);
	hdc=GetWindowDC(hDlg);
	(*rd).origin=GetDCOrg(hdc);
	GetClipBox(hdc,&((*rd).rect));
	GetClipBox(hdc,&r);
	ReleaseDC(hDlg,hdc);
	GlobalUnlock(hmem);
	return hmem;
}

void RepaintMainD(HGLOBAL hrd)
{
	// Compute rectangle in main window which was hidden by dialog window.
	// As Windows thinks the main window doesn't change, it redraws it from
	// an internal buffer and validates the region.
	RECT mainrect;
	HDC hdc;
	REPAINTDATA *rd;

	if (hrd==NULL) return;
	rd=(REPAINTDATA*)GlobalLock(hrd);
	(*rd).rect.left+=LOWORD((*rd).origin);
	(*rd).rect.right+=LOWORD((*rd).origin);
	(*rd).rect.top+=HIWORD((*rd).origin);
	(*rd).rect.bottom+=HIWORD((*rd).origin);
	hdc=GetDC(hWndMain);
	GetClipBox(hdc,&mainrect);
	ReleaseDC(hWndMain,hdc);
	MapWindowPoints(NULL,hWndMain,(POINT far*)&((*rd).rect),2);
	IntersectRect(&mainrect,&((*rd).rect),&mainrect);
	RedrawWindow(hWndMain,&mainrect,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
	GlobalUnlock(hrd);
	GlobalFree(hrd);
}

void RepaintMain(HWND hDlg)
{
	RepaintMainD(RepaintData(hDlg));
}


int thumb2speed(int thumb)
{
	return ((int) ( exp( log(10)*((double)(400-thumb))/100 ) ));
}

int speed2thumb(int speed)
{
	if (speed<10) return(300);
	return (400-(int) ( 100*log(speed)/log(10) ));
}

int speed2ord(int speed)
{
	if (speed<=20) return(0);
	if (speed<=25) return(1);
	if (speed<=30) return(2);
	if (speed<=40) return(3);
	if (speed<=50) return(4);
	if (speed<=65) return(5);
	if (speed<=80) return(6);
	if (speed<=100) return(7);
	if (speed<=125) return(8);
	if (speed<=160) return(9);
	return(10+speed2ord(speed/10));
}

int ord2speed(int ord)
{
	switch (ord) {
	case 0:return(20);
	case 1:return(25);
	case 2:return(30);
	case 3:return(40);
	case 4:return(50);
	case 5:return(65);
	case 6:return(80);
	case 7:return(100);
	case 8:return(125);
	case 9:return(160);
	default: return(10*ord2speed(ord-10));
	}
}


char *joystring(char code,char *string)
{
	switch (code) {
	case ']':   strcpy(string,"SymShift");break;
	case '[':   strcpy(string,"Shift");break;
	case '/':   strcpy(string,"Enter");break;
	case '\\':  strcpy(string,"Space");break;
	default:    string[0]=code;
					string[1]=0;
	}
	return string;
}


BOOL CALLBACK JoyDialogProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	char temp[10];
	int i;
	static WORD kbda[5],kbdm[5];
	static int kbdptr;
	static HGLOBAL hrd;

	switch (wMess) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,JOY_L,joystring(z80header.kbdasc[0],temp));
		SetDlgItemText(hDlg,JOY_R,joystring(z80header.kbdasc[1],temp));
		SetDlgItemText(hDlg,JOY_U,joystring(z80header.kbdasc[3],temp));
		SetDlgItemText(hDlg,JOY_D,joystring(z80header.kbdasc[2],temp));
		SetDlgItemText(hDlg,JOY_F,joystring(z80header.kbdasc[4],temp));
		SetDlgItemText(hDlg,JOY_MESSAGE,"");
		SetDlgItemText(hDlg,JOY_OKCANCEL,"OK");
		kbdptr=-1;
		hrd=0;
		CatchKeys=FALSE;
//		return TRUE;
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnJoyDialog);
		hJoyDialog=NULL;
		CatchKeys=FALSE;
		break;
	case WM_COMMAND:
		switch (wPar) {
		case JOY_DEFINE:
			if (kbdptr!=-1) {
				MessageBeep(-1);
				return TRUE;
			}
			SetActiveWindow(hWndMain);
			SetDlgItemText(hDlg,JOY_OKCANCEL,"Cancel");
			SetDlgItemText(hDlg,JOY_MESSAGE,"Enter direction keys in Spectrum window");
			kbdptr=0;
			SetDlgItemText(hDlg,JOY_L," ---");
			CatchKeys=TRUE;
			return TRUE;
		case JOY_OKCANCEL:
			if (kbdptr==-1) {
				hrd=RepaintData(hDlg);
				DestroyWindow(hDlg);
				return TRUE;
			}
			SetActiveWindow(hDlg);
			PostMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		}
		return FALSE;
	case WM_USER+1:
		if ((kbdptr<0)||(kbdptr>4)) return FALSE;
		for (i=0;i<kbdptr;i++) {
			if (kbda[i]==wPar) {
				MessageBeep(-1);
				return TRUE;
			}
		}
		kbda[kbdptr]=wPar;
		kbdm[kbdptr]=lPar;
		SetDlgItemText(hDlg,JOY_L+kbdptr,joystring(wPar,temp));
		kbdptr++;
		if (kbdptr<5) {
			SetDlgItemText(hDlg,JOY_L+kbdptr,"---");
			return TRUE;
		}
		SetActiveWindow(hDlg);
		for (kbdptr=0;kbdptr<5;kbdptr++) {
			const int mapper[]={0,1,3,2,4};
			z80header.kbdasc[kbdptr]=kbda[mapper[kbdptr]];
			z80header.kbdmap[kbdptr]=kbdm[mapper[kbdptr]];
		}
		TranslateJoystickSetting();
		currentjoystick=10;
		SetJoystick(currentjoystick);
		PostMessage(hDlg,WM_INITDIALOG,0,0);
		return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


void UpdateWinZ80Caption(void)
// Currently, writes '-- paused' in caption when appropriate.
{
	if (state.paused)
		SetWindowText(hWndMain,"Z80 for Windows [paused]");
	else
		SetWindowText(hWndMain,"Z80 for Windows");
}


BOOL CALLBACK CtrlDialogProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	switch (wMess) {
	case WM_INITDIALOG:
		hrd=0;
		if (loadpossible && (!loading)) {
			 SetDlgItemText(hDlg,CTRL_LOAD,"Load");
		} else {
			 SetDlgItemText(hDlg,CTRL_LOAD,loadpossible?"-stop-":"(no load)");
		}
		if (feearmicmask == 16) {
			SetDlgItemText(hDlg,CTRL_SAVE,"Save");
		} else {
			SetDlgItemText(hDlg,CTRL_SAVE,"-stop-");
		}
		if (state.paused)
			SetDlgItemText(hDlg,CTRL_PAUSE,"RESUME");
		else
			SetDlgItemText(hDlg,CTRL_PAUSE,"Pause");
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_SETFOCUS:
		SetActiveWindow(hDebugDialog ? hDebugDialog : hWndMain);
		return TRUE;
	case WM_DESTROY:
		hCtrlDialog=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnCtrlDialog);
		break;
	case WM_COMMAND:
		SetActiveWindow(hDebugDialog ? hDebugDialog : hWndMain);
		switch (wPar) {
		case 2:              // don't ask me why
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		case CTRL_PAUSE:
			state.paused=!state.paused;
			UpdateWinZ80Caption();
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case CTRL_LOAD:
			LoadButton();
			return TRUE;
		case CTRL_SAVE:
			feearmicmask = 24 - feearmicmask;
			SpeedChanged = TRUE;					// this forces re-allocation of sound buffers
			SendMessage(hDlg,WM_INITDIALOG,0,0);
			return TRUE;
		case CTRL_NMI:
			Nmi();
			return TRUE;
		case CTRL_RESET:
			Reset();
			return TRUE;
		}
		return FALSE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}



BOOL CALLBACK RomProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	static HGLOBAL hrd;
	int i;
	char name[128];
	char *fn;
	switch (wMess) {
	case WM_INITDIALOG:
		if (gszRom48k[0]) {
			SetDlgItemText(hDlg,ROM_FN1,gszRom48k);
			SetDlgItemText(hDlg,ROM_OPEN1,"-Close-");
		} else {
			SetDlgItemText(hDlg,ROM_FN1,"-default-");
			SetDlgItemText(hDlg,ROM_OPEN1,"Open");
		}
		if (gszRom1281[0]) {
			SetDlgItemText(hDlg,ROM_FN2,gszRom1281);
			SetDlgItemText(hDlg,ROM_OPEN2,"-Close-");
		} else {
			SetDlgItemText(hDlg,ROM_FN2,"-default-");
			SetDlgItemText(hDlg,ROM_OPEN2,"Open");
		}
		if (gszRom1282[0]) {
			SetDlgItemText(hDlg,ROM_FN3,gszRom1282);
			SetDlgItemText(hDlg,ROM_OPEN3,"-Close-");
		} else {
			SetDlgItemText(hDlg,ROM_FN3,"-default-");
			SetDlgItemText(hDlg,ROM_OPEN3,"Open");
		}
		if (gszRomIf1[0]) {
			SetDlgItemText(hDlg,ROM_FN4,gszRomIf1);
			SetDlgItemText(hDlg,ROM_OPEN4,"-Close-");
		} else {
			SetDlgItemText(hDlg,ROM_FN4,"-default-");
			SetDlgItemText(hDlg,ROM_OPEN4,"Open");
		}
		if (romif1_8k)
			CheckRadioButton(hDlg,ROM_PB8K,ROM_PB16K,ROM_PB8K);
		else
			CheckRadioButton(hDlg,ROM_PB8K,ROM_PB16K,ROM_PB16K);
		if (gszRomMface[0]) {
			SetDlgItemText(hDlg,ROM_FN5,gszRomMface);
			SetDlgItemText(hDlg,ROM_OPEN5,"-Close-");
		} else {
			SetDlgItemText(hDlg,ROM_FN5,"-default-");
			SetDlgItemText(hDlg,ROM_OPEN5,"Open");
		}
		if (gszRomCurrah[0]) {
			SetDlgItemText(hDlg,ROM_FN6,gszRomCurrah);
			SetDlgItemText(hDlg,ROM_OPEN6,"-Close-");
		} else {
			SetDlgItemText(hDlg,ROM_FN6,"-default-");
			SetDlgItemText(hDlg,ROM_OPEN6,"Open");
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		hRomDialog=0;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnRomProc);
		break;
	case WM_USER+1:		// open .rom file through main menu
		for (i=0;((char*)lPar)[i];i++)
			gszRom48k[i] = ((char*)lPar)[i];
		gszRom48k[i]=0;
		SendMessage(hDlg,WM_INITDIALOG,0,0);
		reloadroms();
		return TRUE;
	case WM_COMMAND:
		fn=NULL;
		switch (wPar) {
			case ROM_OPEN1:	fn=gszRom48k;	break;
			case ROM_OPEN2:	fn=gszRom1281;	break;
			case ROM_OPEN3:	fn=gszRom1282;	break;
			case ROM_OPEN4:	fn=gszRomIf1;	break;
			case ROM_OPEN5:	fn=gszRomMface;break;
			case ROM_OPEN6:	fn=gszRomCurrah;break;
			case RD_FINISH:	PostMessage(hDlg,WM_CLOSE,0,0); return TRUE;
		}
		if (fn) {
			if (fn[0]) {
				fn[0]=0;
				SendMessage(hDlg,WM_INITDIALOG,0,0);
				reloadroms();
				return TRUE;
			}
			name[0]=0;
			i = FileDlg(6,name,TRUE,hDlg);
			if (i==-1) return TRUE;
			for (i=0;name[i];i++)
				fn[i]=name[i];
			fn[i]=0;
		} else {
			if (wPar==ROM_PB8K)
				romif1_8k=TRUE;
			else
				romif1_8k=FALSE;
			if (!gszRomIf1[0] && !romif1_8k) {
				MessageBeep(-1);
				romif1_8k=TRUE;
			}
		}
		SendMessage(hDlg,WM_INITDIALOG,0,0);
		reloadroms();
		return TRUE;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}





BOOL CALLBACK SpeedDialogProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	char temp[50];
	HDC hdc;
	HBRUSH hbrush,holdbrush;
	PAINTSTRUCT ps;
	static HGLOBAL hrd;
	int i;
	int x,y,xfac,yfac;
	int pol[6];

	switch (wMess) {
	case WM_INITDIALOG:
		sprintf(temp,"Set speed: %d %%",state.speed);
		SetDlgItemText(hDlg,SPD_SETSPEED,temp);
		CheckRadioButton(hDlg,SPD_CHUNCKY,SPD_SMOOTH,SPD_CHUNCKY+state.vidstate);
		CheckDlgButton(hDlg,SPD_MAX,state.fastest);
		SetScrollRange( GetDlgItem(hDlg,SPD_KNOB),SB_CTL,100,300,FALSE);
		SetScrollPos( GetDlgItem(hDlg,SPD_KNOB),SB_CTL,speed2thumb(state.speed),TRUE);
		EnableMenuItem(GetMenu(hWndMain),CM_SPEED,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		hrd=0;
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_PAINT:
		wPar=state.actspeed;
		BeginPaint(hDlg,&ps);
		EndPaint(hDlg,&ps);
	case WM_USER+1:
		sprintf(temp,"Actual speed: %d %%",wPar);
		SetDlgItemText(hDlg,SPD_ACTSPEED,temp);
		hdc=GetDC(hDlg);
		SelectObject(hdc,GetStockObject(NULL_PEN));
		i=2;
		while (i>0) {
			y=speed2thumb(wPar)-100;               // 0=fastest, 200=slowest
			if (i==2) {
				hbrush=CreateSolidBrush(GetPixel(hdc,0,0));
				UnrealizeObject(hbrush);
				holdbrush = SelectObject(hdc,hbrush);
			} else {
				hbrush=CreateSolidBrush(y<-10?(y=-10,RGB(0xFF,0,0)):GetSysColor(COLOR_WINDOWTEXT));
				SelectObject(hdc,hbrush);
			}
			xfac=LOWORD(GetDialogBaseUnits());
			yfac=HIWORD(GetDialogBaseUnits());
			x=(((scrb_x-scrb_xw)*(xfac))/4);
			// The following is very, verry strange, but I don't see another solution
			if (xfac>=10) x += (scrb_x-scrb_xw)/8;
			y=(((LONG)y)*yfac*(scrb_yw-(3*scrb_xw)/2))/(fontpts*200);    // relative Y coord of spd pointer
			y+=((scrb_y+(3*scrb_xw)/4)*yfac)/fontpts;          // now absolute
			if (i==2) y=oldspdptr; else oldspdptr=y;
			// fontpts=8 says docs, but fontpts=10 works on my machine with
			// 10 point fonts.  The value is retrieved from the SPECTRUM.INI
			// file, variable FONTPOINTSIZE
			pol[0]=x;
			pol[1]=y;
			pol[2]=x-(xfac*2)/3;
			pol[3]=y-yfac/3;
			pol[4]=x-(xfac*2)/3;
			pol[5]=y+yfac/3;
			if (y!=-1) Polygon(hdc,(POINT*)pol,3);
			x+=(scrb_xw*xfac)/4;
			pol[0]=x;
			pol[2]=x+(xfac*2)/3;
			pol[4]=x+(xfac*2)/3;
			if (y!=-1) Polygon(hdc,(POINT*)pol,3);
//			if (i==1) {
				SelectObject(hdc,holdbrush);
				DeleteObject(hbrush);
//			}
			i--;
		}
		ReleaseDC(hDlg,hdc);
		return(TRUE);
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
		return(TRUE);
	case WM_COMMAND:
		switch (wPar) {
		case SPD_MAX:
			state.fastest=!state.fastest;
			CheckDlgButton(hDlg,SPD_MAX,state.fastest);
			ClearAvBuf();
			return(TRUE);
		case SPD_CHUNCKY:
		case SPD_NORMAL:
		case SPD_SMOOTH:
			state.vidstate=wPar-SPD_CHUNCKY;
			CheckRadioButton(hDlg,SPD_CHUNCKY,SPD_SMOOTH,SPD_CHUNCKY+state.vidstate);
			ClearAvBuf();
			return(TRUE);
		case SPD_CLOSE:
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
			return (TRUE);
		}
		break;
	case WM_VSCROLL:
		if (Played5Minutes)
			break;
		switch (wPar) {
		case SB_BOTTOM:
			if (state.speed!=10) {
				state.speed=10;
				SpeedChanged=TRUE;
			}
			break;
		case SB_TOP:
			if (state.speed!=1000) {
				state.speed=1000;
				SpeedChanged=TRUE;
			}
			break;
		case SB_LINEUP:
		case SB_PAGEUP:
			i=speed2ord((11*state.speed)/10);
			i=ord2speed(i);
			if (i>1000) i=1000;
			if (state.speed!=i) {
				state.speed=i;
				SpeedChanged=TRUE;
			}
			break;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			i=speed2ord(state.speed);
			if (i==0) {
				if (state.speed!=10) {
					state.speed=10;
					SpeedChanged=TRUE;
				}
			} else {
				state.speed=ord2speed(i-1);
				SpeedChanged=TRUE;
			}
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			state.speed=thumb2speed(LOWORD(lPar));
			SpeedChanged=TRUE;
			break;
		case SB_ENDSCROLL:
			ClearAvBuf();
			break;
		}
		if (hRecordDialog) {
			SpeedChanged=FALSE;
			if (wPar==SB_ENDSCROLL) MessageBeep(-1);
		} else {
			SetScrollPos(GetDlgItem(hDlg,SPD_KNOB),SB_CTL,speed2thumb(state.speed),TRUE);
			sprintf(temp,"Set speed: %d %%",state.speed);
			SetDlgItemText(hDlg,SPD_SETSPEED,temp);
		}
		return(TRUE);
	case WM_DESTROY:
		hSpeedDialog=NULL;
		EnableMenuItem(GetMenu(hWndMain),CM_SPEED,MF_BYCOMMAND|MF_ENABLED);
		oldspdptr=-1;
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnSpeedDialog);
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


BOOL CALLBACK FileExistsDialProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	char far* text;
	switch (wMess) {
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,FILE_TEXT,lPar);
		break;
	case WM_COMMAND:
		switch (wPar) {
		case FE_CANCEL:
		case FE_OVERWRITE:
		case FE_APPEND:
			EndDialog(hDlg,(int)wPar);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		EndDialog(hDlg,FE_CANCEL);
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


BOOL CALLBACK LoadZ80DialProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	HLOCAL hTemp;
	char *ptr;
	int i,j;
	static long type;
	static char allfiles[8];
	static char extension[8];
	static char *gszFile;
	static char *gszDir;
	static int *LastSel;
	static int doubleclicksel;

	switch (wMess) {
	case WM_INITDIALOG:
		type=lPar;
		if (type==z80_load) {
			ShowWindow(GetDlgItem(hDlg,LZF_RAW),SW_HIDE);
			SetDlgItemText(hDlg,LZF_VOC,".Z80");
			SetDlgItemText(hDlg,LZF_WAV,".SNA");
		} else if ((type!=wave_save)&&(type!=wave_load)) {
			ShowWindow(GetDlgItem(hDlg,LZF_VOC),SW_HIDE);
			ShowWindow(GetDlgItem(hDlg,LZF_WAV),SW_HIDE);
			ShowWindow(GetDlgItem(hDlg,LZF_RAW),SW_HIDE);
		}
		switch (type) {
			case z80_save:
				SetWindowText(hDlg,"Save .Z80 snapshot");
				strcpy(allfiles,"*.z80");
				gszFile=gszZ80File;
				gszDir=gszZ80FileDir;
				LastSel=NULL;
				break;
			case z80_load:
				if (loadz80type==LZF_WAV) {      // i.e. .SNA
					strcpy(allfiles,"*.sna");
					SetWindowText(hDlg,"Load .SNA snapshot");
				} else {
					strcpy(allfiles,"*.z80");
				}
				CheckRadioButton(hDlg,LZF_VOC,LZF_WAV,loadz80type);
				gszFile=gszZ80File;
				gszDir=gszZ80FileDir;
				LastSel=&Z80FileSelection;
				break;
			case tap_save:
				SetWindowText(hDlg,"Record blocks in .TAP file");
				gszFile=gszRecTapFile;
				LastSel=NULL;
				goto tapgeneric;
			case tap_load:
				SetWindowText(hDlg,"Play blocks from .TAP file");
				gszFile=gszPlayTapFile;
				LastSel=&TapFileSelection;
				tapgeneric:
				gszDir=gszTapDir;
				strcpy(allfiles,"*.tap");
				break;
			case dat_load:
				SetWindowText(hDlg,DatLoadTitle);
				gszFile=datfile;
				gszDir=datdir;
				LastSel=NULL;
				strcpy(allfiles,"*.dat");
				break;
			case wave_save:
				if (savesampletype==LZF_RAW) {
					SetWindowText(hDlg,"Save to .RAW sample file");
					strcpy(allfiles,"*.raw");
					CheckRadioButton(hDlg,LZF_VOC,LZF_RAW,LZF_RAW);
				} else if (savesampletype==LZF_VOC) {
					SetWindowText(hDlg,"Save to .VOC sample file");
					strcpy(allfiles,"*.voc");
					CheckRadioButton(hDlg,LZF_VOC,LZF_RAW,LZF_VOC);
				} else {
					SetWindowText(hDlg,"Save to .WAV sample file");
					strcpy(allfiles,"*.wav");
					CheckRadioButton(hDlg,LZF_VOC,LZF_RAW,LZF_WAV);
				}
				gszFile=gszSampleFile;
				gszDir=gszInFileDir;
				LastSel=NULL;
				break;
			case wave_load:
				if (loadsampletype==LZF_RAW) {
					SetWindowText(hDlg,"Play raw sample file");
					strcpy(allfiles,"*.*");
					CheckRadioButton(hDlg,LZF_VOC,LZF_RAW,LZF_RAW);
				} else if (loadsampletype==LZF_VOC) {
					SetWindowText(hDlg,"Play .VOC sample file");
					strcpy(allfiles,"*.voc");
					CheckRadioButton(hDlg,LZF_VOC,LZF_RAW,LZF_VOC);
				} else {
					SetWindowText(hDlg,"Play .WAV sample file");
					strcpy(allfiles,"*.wav");
					CheckRadioButton(hDlg,LZF_VOC,LZF_RAW,LZF_WAV);
				}
				gszFile=gszInFile;
				gszDir=gszInFileDir;
				LastSel=&InFileSelection;
				break;
		}
		strcpy(gszFile,gszDir);
		strcpy(extension,allfiles);
		extension[0]='/';
		AddFile(gszFile,allfiles);
		DlgDirList(hDlg,gszFile,LZF_LISTBOX,LZF_DIRECTORY,
			DDL_ARCHIVE|DDL_READWRITE|DDL_READONLY|DDL_DIRECTORY);
		SetDlgItemText(hDlg,LZF_EDIT,"");
		if (LastSel)
			SendDlgItemMessage(hDlg,LZF_LISTBOX,LB_SETCURSEL,*LastSel,0L);
		videobuf->updatevisibility=TRUE;
		return TRUE;
	case WM_COMMAND:
		switch (wPar) {
		case LZF_VOC:
		case LZF_WAV:
		case LZF_RAW:
			if (type==wave_save) savesampletype=wPar;
			else if (type==wave_load) loadsampletype=wPar;
			else if (type==z80_load) loadz80type=wPar;
			SendMessage(hDlg,WM_INITDIALOG,0,type);
			break;
		case LZF_DELETE:
		case LZF_OK:
			lzf_ok:
				// Get drive+directory, conveniently put in text control by DlgDirList
			GetDlgItemText(hDlg,LZF_DIRECTORY,gszDir,100);
				// Get user input / selection
			for (j=0;gszDir[j];j++) gszFile[j]=gszDir[j];
			gszFile[j]=0;
			hTemp=LocalAlloc(LPTR,100);
			ptr=LocalLock(hTemp);
			i=SendDlgItemMessage(hDlg,LZF_LISTBOX,LB_GETCURSEL,0,0L);
			if (i!=LB_ERR) {
				if (LastSel) *LastSel=i;
				i=DlgDirSelectEx(hDlg,ptr,100,LZF_LISTBOX);
				if (((ptr[1]==':')&& ptr[0])||(ptr[0]=='\\')) {
					gszFile[0]=0;
				}
				AddFile(gszFile,ptr);
				LocalUnlock(hTemp);
				LocalFree(hTemp);
					// Was it a directory?
				if (i) goto lzfok_dir;
			} else {
					// Construct full path+file
				i=SendDlgItemMessage(hDlg,LZF_EDIT,EM_LINELENGTH,0,0L);
				GetDlgItemText(hDlg,LZF_EDIT,ptr,100);
				ptr[i]=0;
				if (((ptr[1]==':')&& ptr[0])||(ptr[0]=='\\')) {
					gszFile[0]=0;
				}
				AddFile(gszFile,ptr);
				LocalUnlock(hTemp);
				LocalFree(hTemp);
			}
			// Now try to open it, to see if it is a file or just a dir
			// When saving however, always add default extension first
			if (type & 1) {   // loading
				if (OpenExists(gszFile)!=HFILE_ERROR) goto filefound;
			}
			i=AddFile(gszFile,extension);		// The '/' signifies: add extension
			if (OpenExists(gszFile)!=HFILE_ERROR) {
				filefound:
				if (wPar==LZF_DELETE) {
					char text[128];
					sprintf(text,"I'm about to delete %s",gszFile);
					j=MessageBox(hDlg,text,"About to DELETE a FILE!",
						MB_ICONEXCLAMATION|MB_OKCANCEL);
					if (j!=IDOK) return FALSE;
					OpenDelete(gszFile);
					strcpy(gszFile,".");
					wPar=LZF_OK;
					goto lzfok_dir;
				}
				if (!(type & 1)) {   // saving
					HGLOBAL hMem;
					char far* text;
					FARPROC lpfnDialProc;
					if (type==tap_save) {            // always append to .TAPs
						EndDialog(hDlg,of_append);
						return TRUE;
					}
					hMem=GlobalAlloc(GMEM_MOVEABLE,256);
					text=GlobalLock(hMem);
					sprintf(text,"File %s already exists.  Press one of following:",gszFile);
					GlobalUnlock(hMem);
					lpfnDialProc=MyMakeProcInstance(FileExistsDialProc,ghInstance);
					if (type==z80_save) {
						j=DialogBoxParam(ghInstance,"FileExists2",hDlg,lpfnDialProc,hMem);
						if (j==FE_APPEND) j=FE_CANCEL; 	// $10 for an explanation
					} else {
						j=DialogBoxParam(ghInstance,"FileExists",hDlg,lpfnDialProc,hMem);
					}
					GlobalFree(hMem);
					MyFreeProcInstance(lpfnDialProc);
					RepaintMain(hDlg);
					if (j==FE_OVERWRITE)
						EndDialog(hDlg,of_create);
					else if (j==FE_APPEND)
						EndDialog(hDlg,of_append);
					else
						EndDialog(hDlg,of_error);
					return TRUE;
				}
				RepaintMain(hDlg);
				EndDialog(hDlg,of_exists);
				return TRUE;
			}
			// We did not succeed in opening it.  When saving, it may be a new file
			if (!(type & 1)) {
				HFILE hFile;
				hFile=OpenCreate(gszFile);
				if (hFile!=HFILE_ERROR) {
					_lclose(hFile);
					OpenDelete(gszFile);
					RepaintMain(hDlg);
					EndDialog(hDlg,of_create);
					return TRUE;
				}
			}
			gszFile[i]=0;
		lzfok_dir:
			if (wPar==LZF_DELETE) {
				MessageBeep(-1);
				return FALSE;
			}
			AddFile(gszFile,allfiles);
			DlgDirList(hDlg,gszFile,LZF_LISTBOX,LZF_DIRECTORY,
				DDL_ARCHIVE|DDL_READWRITE|DDL_READONLY|DDL_DIRECTORY);
			SetDlgItemText(hDlg,LZF_EDIT,"");
			GetDlgItemText(hDlg,LZF_DIRECTORY,gszDir,100);
			break;
		case LZF_CANCEL:
			RepaintMain(hDlg);
			EndDialog(hDlg,of_error);
			return TRUE;
		case LZF_LISTBOX:
			if (HIWORD(lPar)==LBN_DBLCLK) {
				i=SendDlgItemMessage(hDlg,LZF_LISTBOX,LB_GETCURSEL,0,0L);
				if (i==LB_ERR) i=doubleclicksel;
				if (i!=LB_ERR) goto lzf_ok;
			}
			if (HIWORD(lPar)==LBN_SELCHANGE) {
				doubleclicksel=i=
					SendDlgItemMessage(hDlg,LZF_LISTBOX,LB_GETCURSEL,0,0L);
				if (i!=LB_ERR) {
					hTemp=LocalAlloc(LPTR,100);
					ptr=LocalLock(hTemp);
					DlgDirSelectEx(hDlg,ptr,100,LZF_LISTBOX);
					SetDlgItemText(hDlg,LZF_EDIT,ptr);
					LocalUnlock(hTemp);
					LocalFree(hTemp);
				}
			}
			return TRUE;
		case LZF_EDIT:
			if (HIWORD(lPar)==EN_UPDATE) {
				i=SendDlgItemMessage(hDlg,LZF_EDIT,EM_LINELENGTH,0,0L);
				hTemp=LocalAlloc(LPTR,100);
				ptr=LocalLock(hTemp);
				GetDlgItemText(hDlg,LZF_EDIT,ptr,100);
				ptr[i]=0;
				i=SendDlgItemMessage(hDlg,LZF_LISTBOX,LB_FINDSTRING,-1,(LONG)ptr);
				// if not found then i==-1 and next call will deselect all names
				SendDlgItemMessage(hDlg,LZF_LISTBOX,LB_SETCURSEL,i,0L);
				LocalUnlock(hTemp);
				LocalFree(hTemp);
			}
			break;
		}
		break;
	case WM_CLOSE:
//		DestroyWindow(hDlg);
		EndDialog(hDlg,of_error);
		return 0;
	case WM_DESTROY:
		EndDialog(hDlg,of_error);
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}


BOOL CALLBACK PlaySampleProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	HFILE handle;
	LONG curtpos;
	static long lasttpos;
	static int lastthumbpos;
	static int busy=0;
	static char sratechanged;
	static HGLOBAL hrd;
	int i;
	WORD w,w2;
	char temp[128];

	switch (wMess) {
	case WM_INITDIALOG:
		strcpy(temp,"File name: ");
		if (InPlaying) strcat(temp,gszInFile); else strcat(temp,"<none>");
		SetDlgItemText(hDlg,PSF_FILENAME,temp);
		if (InPlaying) {
			if (inning) strcpy(temp,"Pause"); else
				if (InTimePos==0) strcpy(temp,"Play"); else strcpy(temp,"Resume");
		} else strcpy(temp,"Open file");
		SetDlgItemText(hDlg,PSF_OPENFILE,temp);
		if (loadsampletype==LZF_RAW) {
			ShowWindow(GetDlgItem(hDlg,PSF_SRATE),SW_HIDE);
			ShowWindow(GetDlgItem(hDlg,PSF_SRATEEDIT),SW_SHOWNA);
			if (!InPlaying)
				SetDlgItemInt(hDlg,PSF_SRATEEDIT,SampleRate,FALSE);
			else
				SetDlgItemInt(hDlg,PSF_SRATEEDIT,1750000L/Curintperbit,FALSE);
		} else {
			ShowWindow(GetDlgItem(hDlg,PSF_SRATE),SW_SHOWNA);
			ShowWindow(GetDlgItem(hDlg,PSF_SRATEEDIT),SW_HIDE);
			if (!InPlaying)
				SetDlgItemInt(hDlg,PSF_SRATE,0,FALSE);
			else
				SetDlgItemInt(hDlg,PSF_SRATE,1750000L/Curintperbit,FALSE);
		}
		SetScrollRange( GetDlgItem(hDlg,PSF_SCROLLBAR),SB_CTL,0,1000,FALSE);
		lastthumbpos=-1;
		lasttpos=-1;
		SendMessage(hDlg,PSF_SHOWPOSITION,0,0);
		hrd=0;
		break;
	case PSF_RESET:
		inning=0;
		InPlaying=FALSE;
		PostMessage(hDlg,WM_INITDIALOG,0,0);
		if (wPar==1) PostMessage(hDlg,WM_COMMAND,PSF_CANCEL,0);
		break;
	case WM_COMMAND:
		if (busy) {
			MessageBeep(-1);
			return TRUE;
		}
		switch (wPar) {
		case PSF_OPENFILE:      // Also: play/pause/resume
			if (InPlaying && (lPar == CM_OPENGLOBALFILE)) {
				SendMessage(hDlg,PSF_RESET,0,0);
			}
			if (!InPlaying) {    // open file
				if (loading) {    // when loading via SB, you cannot play .VOC files
					MessageBeep(-1);
					return TRUE;
				}
				if ( lPar != CM_OPENGLOBALFILE )
				{
					i = FileDlg(1, gszInFile, TRUE, hDlg);
					videobuf->updatevisibility=TRUE;
					if (i==-1)	return TRUE;	// If Cancel.
				} else {
					strcpy(gszInFile, gszDefaultFile);
				}
				loadsampletype = LZF_VOC + extensionsub;
				handle=OpenRead(gszInFile);
				if (handle==HFILE_ERROR) {
					notify(SampleFileError);
					return TRUE;
				}
				_lclose(handle);
				strcpy(temp,"File name: ");
				strcat(temp,gszInFile);
				SetDlgItemText(hDlg,PSF_FILENAME,temp);
				inning=0;
				initinning();			// allocate memory if necessary
				InPlaying=TRUE;      // flag, to see whether error occurred
				busy=1;
				SetDlgItemText(hDlg,PSF_POSITION," --- Please wait ---");
				Curintperbit = intperbit = 1750000L/SampleRate;   // for RAWs
				sratechanged=FALSE;
				SendMessage(hDlg,PSF_WIND,0,613000L);    // find total length
				busy=0;
				if (!InPlaying) {
					PostMessage(hDlg,PSF_RESET,0,0);
					return TRUE;
				}
				InTotalLength=InTimePos;
				InTimePos=0;
				InPlaying=TRUE;
				iimode=!!iimode;         // initialise intelli-in variable
				SetDlgItemText(hDlg,PSF_OPENFILE,"Play");
				lasttpos=-1;
				PostMessage(hDlg,WM_INITDIALOG,0,0);
//            SendMessage(hDlg,PSF_SHOWPOSITION,0,0);
			} else { // play, pause or resume
				if (!inning) {    // play or resume
					SetDlgItemText(hDlg,PSF_OPENFILE,"Pause");
					SetDlgItemText(hDlg,PSF_POSITION," --- Please wait ---");
					busy=1;
					SendMessage(hDlg,PSF_WIND,0,InTimePos);
					busy=0;
					if (!InPlaying) {
						PostMessage(hDlg,PSF_RESET,0,0);
						return TRUE;
					}
					inning=1;
					lasttpos=-1;
					PostMessage(hDlg,PSF_SHOWPOSITION,0,0);
				} else {
					SetDlgItemText(hDlg,PSF_OPENFILE,"Resume");
					inning=0;
					getcurrenttime();
					InTimePos-=
						(tframe*(soundtimehi-intbasehi)+soundtimelo-intbaselo)/3500;
				}
			}
			return TRUE;
		case PSF_CANCEL:
			inning=0;
			InPlaying=FALSE;
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		case PSF_SRATEEDIT:
			w=GetDlgItemInt(hDlg,PSF_SRATEEDIT,NULL,FALSE);
			if (InPlaying && inning) {
				w2=(1750000L+(Curintperbit>>1))/Curintperbit;
				if (w2!=w)
					SetDlgItemInt(hDlg,PSF_SRATEEDIT,w2,FALSE);
				return TRUE;
			}
			if ((w>44100L)||(w<5000)) return TRUE;
			setsamplerate((1750000L+(w>>1))/w);
			sratechanged=TRUE;
			return TRUE;
		}
		return FALSE;
	case WM_HSCROLL:
		if (!InPlaying) return TRUE;
		if (inning) SendMessage(hDlg,WM_COMMAND,PSF_OPENFILE,0); // pause
		switch (wPar) {
		case SB_TOP:
		case SB_LINEUP:
			InTimePos=1000*((InTimePos-1)/1000);
			break;
		case SB_BOTTOM:
		case SB_LINEDOWN:
			InTimePos=1000*(InTimePos/1000+1);
			break;
		case SB_PAGEUP:
			InTimePos=10000*((InTimePos-1)/10000);
			break;
		case SB_PAGEDOWN:
			InTimePos=10000*(InTimePos/10000+1);
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			InTimePos=LOWORD(lPar)*(InTotalLength/1000);
			break;
		}
		if (InTimePos<0) InTimePos=0;
		if (InTimePos>InTotalLength) InTimePos=InTotalLength;
	case PSF_SHOWPOSITION:
		if (InPlaying) {
			curtpos=InTimePos;
			if (inning) {
				getcurrenttime();
				curtpos=curtpos - (intperbit*inbuflen)/3500L +
					(tframe*(soundtimehi-intbasehi)+soundtimelo-intbaselo)/3500L;
			}
		} else {
			curtpos=0;
			InTotalLength=1000;
		}
		if (lasttpos==curtpos) return TRUE;
		lasttpos=curtpos;
		w=(1750000L+(Curintperbit>>1))/Curintperbit;
		w2=GetDlgItemInt(hDlg,
							  loadsampletype==LZF_RAW?PSF_SRATEEDIT:PSF_SRATE,
                       NULL,FALSE);
		if (w2!=w) SetDlgItemInt(hDlg,
								loadsampletype==LZF_RAW?PSF_SRATEEDIT:PSF_SRATE,
								w,FALSE);
		sprintf(temp,"%2d:%02d.%02d (%d%%)",
			(int)(curtpos/60000L),
			(int)((curtpos/1000)%60),
			(int)(curtpos%1000)/10,
			(int)(curtpos / (InTotalLength/100)));
		SetDlgItemText(hDlg,PSF_POSITION,temp);
		i=curtpos/(max(0,InTotalLength/1000));
		if (i==lastthumbpos) return TRUE;
      lastthumbpos=i;
		SetScrollPos( GetDlgItem(hDlg,PSF_SCROLLBAR),SB_CTL,i,TRUE);
		return TRUE;
	case PSF_WIND:
		if (sratechanged) {
			sratechanged=FALSE;
			SendMessage(hDlg,PSF_WIND,0,613000L);
			InTotalLength=InTimePos;
		}
		handle=OpenRead(gszInFile);
		if (handle==HFILE_ERROR) {
			psfw_err:
			InPlaying=0;
         inning=0;
			PostMessage(hDlg,PSF_RESET,0,0);
			notify(SampleFileError);
			return TRUE;
		}
		if (loadsampletype==LZF_VOC) {
			_llseek(handle,0x13,0);
			if (_lread(handle,temp,3)!=3) goto psfw_err;
			if ((temp[0]!=0x1a)||(temp[1]!=0x1a)||(temp[2]!=0)) goto psfw_err;
			InFilePos=0x1a;
		} else {
         InFilePos=0;
      }
		_lclose(handle);
		InInBlock=0;
		InTimePos=0;
		if (loadsampletype!=LZF_RAW) intperbit=1;
		readinbuffer(lPar);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_DESTROY:
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnPlaySampleProc);
		hPlaySample=NULL;
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}





BOOL CALLBACK RecordDialProc(HWND hDlg, WORD wMess, WORD wPar, LONG lPar)
{
	HFILE handle;
	static HGLOBAL hrd;
	int i;
	long tim;
	char temp[128];
	switch (wMess) {
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return 0;
	case WM_INITDIALOG:
		EnableMenuItem(GetMenu(hWndMain),CM_RECORD,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		state.record=FALSE;
		state.recpaused=TRUE;
		TotalRecorded=0;
		hSampleBuf=NULL;
		SampleBufLen=0;
		hRecordDialog=hDlg;
		SetDlgItemInt(hDlg,REC_SRATE,SampleRate,FALSE);
		ShowWindow(GetDlgItem(hDlg,REC_SRATEEDIT),SW_HIDE);
		hrd=0;
		break;
	case WM_COMMAND:
		switch (wPar) {
		case REC_OPENFILE:         // Also 'Pause, resume'
			if (!state.record) {
				i = FileDlg(1, gszSampleFile, FALSE, hDlg);
				videobuf->updatevisibility=TRUE;
				if (i==-1)	return TRUE;	// If Cancel.
				loadsampletype = LZF_VOC + extensionsub;	// VOC, RAW, WAV
				videobuf->updatevisibility=TRUE;
				handle=OpenCreate(gszSampleFile);
				if (handle==HFILE_ERROR) {
					notify(SaveSampleError);
					break;
				}
				if (i==of_create) {
					WriteSampleHeader(handle);
				} else {
					if (savesampletype==LZF_VOC) {
						const char silenceblock[]={3,3,0,0,0xff,0,0xd2};
						_llseek(handle,-1L,2);     // overwrite end marker byte
						_lwrite(handle,silenceblock,7);
					}                             // RAW files: simply append
				}
				_lclose(handle);
				strcpy(temp,"File name: ");
				strcat(temp,gszSampleFile);
				SetDlgItemText(hDlg,REC_FILENAME,temp);
				SetDlgItemText(hDlg,REC_STOP,"STOP");
				state.record=TRUE;
				if (!sound) {
					sound=TRUE;
					AllocBuffers();
				}
				state.recpaused=!state.recpaused;
				SendMessage(hDlg,WM_COMMAND,REC_OPENFILE,0L);  // pause, to set text
				TotalRecorded=0;
				hSampleBuf=NULL;
            PostMessage(hDlg,WM_USER+1,0,0L);      // update time / paused
            return TRUE;
			} else {
				if (!state.record) return TRUE;
				state.recpaused=!state.recpaused;
				if (state.recpaused)
					SetDlgItemText(hDlg,REC_OPENFILE,"Resume");
				else {
					SetDlgItemText(hDlg,REC_OPENFILE,"Pause");
					if (soundsilent && TotalRecorded) {
						getcurrenttime();
                  TimeOfLastSample=soundlastthi;
					} else
						TimeOfLastSample=0;
				}
				PostMessage(hDlg,WM_USER+1,0,0);
				return TRUE;
			}
      case REC_STOP:
		case REC_CLOSEFILE:
			goto stopclose;
		}
		return FALSE;
	case WM_USER+1:
		tim=(100*TotalRecorded)/SampleRate;
		sprintf(temp,"Time recorded: %d:%02d.%02d",(int)(tim/6000),
			(int)((tim/100)%60),(int)(tim%100));
		if (state.recpaused) strcat(temp,"    (paused)");
		SetDlgItemText(hDlg,REC_TIME,temp);
		return TRUE;
	case WM_DESTROY:
		stopclose:
		FlushRecordBuf();    // writes buffer to file, frees memory
		if (state.record) WriteSampleTrailer();
		state.record=FALSE;
		if (!state.sound) {
			sound=FALSE;
			AllocBuffers();
		}
		if ((wMess==WM_COMMAND)&&(wPar==REC_CLOSEFILE)) return TRUE;
		if (wMess != WM_DESTROY) {
			hrd=RepaintData(hDlg);
			DestroyWindow(hDlg);
		}
		if ((wMess==WM_COMMAND)&&(wPar==REC_STOP)) return TRUE;
		EnableMenuItem(GetMenu(hWndMain),CM_RECORD,MF_BYCOMMAND|MF_ENABLED);
		PostMessage(hWndMain,IK_FREELPFN,hrd,(LONG)lpfnRecordProc);
		hRecordDialog=NULL;
		break;
	}
	return MyDlgProc(hDlg,wMess,wPar,lPar);
}

/* begin added by jts 4/2/97 */
/* Modified GL 4/27/97 */
/*	uses the global variable gszFilter that's set up in WinMain()
 * returns offset in szFT of the extension (at the '.') */
/* If hParentWnd==NULL, szPathName is assumed to be valid name, and a
 * mere existence check is done.  */
int	FileDlg(	int nIndex,		// index of file filter to use
					LPSTR szPathName,	// returns the path and filename (max len 128)
					BOOL bLoad,			// TRUE=Open file;  FALSE = Save File
					HWND hParentWnd)	// handle of the parent window
{
	int i=0, j=0, cnt;
	DWORD	Error;
	OPENFILENAME ofn;
	char DefExt[5], *Path, *InitialPath, *Title, SaveFilter[256];
	alloveragain:
	Path = (char*)malloc(256);
	switch (nIndex) {
		case 0:	// snaps
			InitialPath = gszZ80FileDir;
			Title = "Open snapshot file";
			break;
		case 1:	// sample files
			InitialPath = gszInFileDir;
			Title = "Open sample file";
			break;
		case 2:	// tape files
			InitialPath = gszTapDir;
			Title = "Open tape file";
			break;
		case 3:	// all files
			InitialPath = NULL;
			Title = "Open Spectrum file";
			break;
		case 4:	// microdrive files
			InitialPath = gszMdrvDir;
			Title = "Open microdrive file";
			break;
		case 5:	// rs232 files
			InitialPath = gszRsDir;
			Title = "Open RS232 file";
			break;
		case 6:	// rom files
			InitialPath = NULL;
			Title = "Open ROM file";
			break;
		case 7:	// scr files
			InitialPath = gszScrDir;
			Title = "Load screen";
			if (!bLoad) Title = "Save screen or movie";
			break;
	}
	if (!bLoad) {
		strncpy(Title,"Save",4);
	} else {
		strncpy(Title,"Open",4);
	}
	if (InitialPath)
		strcpy(Path, InitialPath);
	else
		Path[0]=0;
	ofn.hwndOwner = hParentWnd;
	ofn.hInstance = ghInstance;
//	if (nIndex==3)
//		ofn.lpstrFilter = gszFilter;
//	else
	{
		cnt = 0;
		while( cnt < nIndex*2 )
		{
			if (gszFilter[i]=='\0')
			{
				cnt++;
				if (gszFilter[i+1]=='\0')	{	i=0;	break;	}
			}
			i++;
		}
		cnt=0;
		j = i;
		while ( cnt < 2 )
		{
			if (gszFilter[j]=='\0')
			{
				cnt++;
				if (cnt==2) break;
				if (gszFilter[j+1]=='\0')	{	j=i;	break;	}
			}
			j++;
		}
		memcpy(SaveFilter, &(gszFilter[i]), j-i);
		SaveFilter[j-i] = '\0';	SaveFilter[j-i+1] = '\0';
		ofn.lpstrFilter = SaveFilter;
	}
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;

	if (bLoad)
		ofn.nFilterIndex = nIndex+1;
	else
		ofn.nFilterIndex = 0;

	ofn.lpstrFile = szPathName;
	ofn.nMaxFile = 128;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = Path;
	ofn.lpstrTitle = Title;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;
	if (nIndex != 2)
		ofn.Flags |= OFN_OVERWRITEPROMPT;		// tap/tzx are checked later for append/overwrite
	ofn.nFileOffset = -1;
	ofn.nFileExtension = -1;

//	strncpy(DefExt, &(szPathName[i+2]), 3);
//	DefExt[3] = '\0';
//	ofn.lpstrDefExt = DefExt;

	strncpy(DefExt, SaveFilter + strlen(SaveFilter) + 3,3);
	ofn.lpstrDefExt = DefExt;

	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	ofn.lStructSize = sizeof( ofn );
	Error = 0;
	if (hParentWnd==NULL) {				// file from command line
		if (OpenExists( ofn.lpstrFile ) != HFILE_ERROR) {
			Error = 0;
		} else {
			Error = 1;
		}
		ofn.nFileExtension=0;
		ofn.nFileOffset=0;
		for (i=0;ofn.lpstrFile[i];i++) {
			if (ofn.lpstrFile[i]=='.')
				ofn.nFileExtension=i+1;
			if ((ofn.lpstrFile[i]=='\\')||(ofn.lpstrFile[i]==':'))
				ofn.nFileOffset=i+1;
		}
	} else {
		if (bLoad)
		{
			if ( GetOpenFileName( &ofn ) == FALSE)
				Error = CommDlgExtendedError()+1;
		}
		else
		{
			if ( GetSaveFileName( &ofn ) == FALSE)
				Error = CommDlgExtendedError()+1;
		}
	}
	free(Path);
	if (Error==1) {		// cancel
		return(-1);
	}
	if (Error) {
		sprintf (Path,"GetOpenFileName dialog returns error %ld.",Error-1);
		MyMessageBox(hWndMain,Path,"WinZ80 error:",MB_ICONHAND|MB_OK);
		Path[0]=0;
		return(-1);
	}
	if (InitialPath) {
		strcpy(InitialPath,ofn.lpstrFile);
		InitialPath[ofn.nFileOffset]=0;
	}
// find out what file type has been selected
//	if (!ofn.nFileExtension) {
//		extensionsub=0;
//		return(nIndex);
//	}
	j=0;
	for (i=0;i<8;i++) {
		j += 1+strlen(gszFilter+j);	// skip description string
		extensionsub=0;					// holds sub type counter (global output)
		while (gszFilter[j]) {			// skip 'all files' (i==3)
	// If no extension and nIndex != 3 (all files) and bLoad, try all relevant extensions
			if ((!szPathName[ofn.nFileExtension]) && (nIndex!=3) && bLoad && (i==nIndex)) {
				strncpy(szPathName+ofn.nFileExtension,gszFilter+j+1,4);	// copy ".ext"
				szPathName[ofn.nFileExtension+4]=0;
				if (OpenExists(szPathName) == HFILE_ERROR)		// remove if not exists
					szPathName[ofn.nFileExtension]=0;
			}
			if ((!strncmpi(gszFilter+j+2,
								szPathName+ofn.nFileExtension + (szPathName[ofn.nFileExtension]=='.')
								,3))&&(i!=3)) {
				switch (i) {
				case 0:	InitialPath = gszZ80FileDir; break;
				case 1:	InitialPath = gszInFileDir; break;
				case 2:	InitialPath = gszTapDir; break;
				case 4:	InitialPath = gszMdrvDir; break;
				case 5:	InitialPath = gszRsDir; break;
				case 6:	InitialPath = NULL; break;
				case 7:	InitialPath = gszScrDir; break;
				}
				if (InitialPath) {
					strcpy(InitialPath,ofn.lpstrFile);
					InitialPath[ofn.nFileOffset]=0;
				}
				// Now find out whether a .TAP or .TZX was to be saved, and
				// an existing file has been selected.  If so, then ask for
				// overwrite/append/cancel
				if ((i==2)&&(!bLoad)) {		// tzx/tap save
					char text[100];
					FARPROC TijdelProc;
					iFileDlgFlg = -1;
					if (OpenExists(ofn.lpstrFile) != HFILE_ERROR) {
						sprintf(text,"File %s already exists.  Press one of following:",ofn.lpstrFile + ofn.nFileOffset);
						TijdelProc=MyMakeProcInstance(FileExistsDialProc,ghInstance);
//					j=DialogBoxParam(ghInstance,"FileExists2",hDlg,lpfnDialProc,hMem);
//					if (j==FE_APPEND) j=FE_CANCEL; 	// $10 for an explanation
						j=DialogBoxParam(ghInstance,"FileExists",hParentWnd,TijdelProc,text);
						MyFreeProcInstance(TijdelProc);
						RepaintMain(hParentWnd);
						if ((j==FE_OVERWRITE)||(j==FE_APPEND))
							iFileDlgFlg = j;
						else
							goto alloveragain;
					}
				}
				return i;
			}
			if (gszFilter[j+5]==';') {
				j+=6;
				extensionsub++;
			}
			else
				j+=5;
		}
		j++;		// skip terminating \0 of extension string
	}
	extensionsub=0;
	return (nIndex);		// 3 means: don't know
}
/* end added by jts 4/2/97 */


long CALLBACK WndProc (HWND hWnd, unsigned iMessage, WORD wPar, LONG lPar)
{
	int i;
	WORD w;
	PAINTSTRUCT ps;
	HDC hdc;
	HFILE handle;

	HBRUSH hbr,hbrold;
	HFONT hf,holdf;
	long l;
	RECT rect;
	char *str;
	BOOL done;
	BOOL show;
	long wx,wy;

	static WORD mousex=-200;
	static WORD mousey=-200;

	if (hWnd == hWndShareware) {
		// Only during the first few seconds

		// vars for newton animation:
		static float vx,vy,alfa,alfa2,x0,y0,xc,yc,springc,springc2;
		static long curanimtime;

		switch (iMessage) {
		case WM_PAINT:
			BeginPaint(hWnd,&ps);
			SetBkColor(ps.hdc,PALETTERGB(0,0,70));
			hbr = CreateSolidBrush(PALETTERGB(0,0,70));
			hbrold = SelectObject(ps.hdc,hbr);
			Rectangle(ps.hdc,0,0,shw_x,shw_y);
			SelectObject(ps.hdc,hbrold);
			DeleteObject(hbr);

			hf = CreateFont(50,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,
					 OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,DRAFT_QUALITY,
					 VARIABLE_PITCH | 0x04 | FF_ROMAN,"times");
			holdf=SelectObject(ps.hdc,hf);
			SetTextAlign(ps.hdc,TA_CENTER);
			SetTextColor(ps.hdc,RGB(255,80,80));
			TextOut(ps.hdc,shw_x/2,20,"Z80 v4.00",9);
			SelectObject(ps.hdc,holdf);
			DeleteObject(hf);

			hf = CreateFont(50,0,0,0,FW_NORMAL,TRUE,FALSE,FALSE,ANSI_CHARSET,
					 OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,DRAFT_QUALITY,
					 VARIABLE_PITCH | 0x04 | FF_ROMAN,"times");
			SetTextColor(ps.hdc,RGB(0,255,255));
			holdf=SelectObject(ps.hdc,hf);
			TextOut(ps.hdc,shw_x/2,60,"for Windows",11);
			SelectObject(ps.hdc,holdf);
			DeleteObject(hf);

			hf = CreateFont(-8,0,0,0,FW_BOLD,0,0,0,0,0,0,DEFAULT_QUALITY,FIXED_PITCH,"Courier");
//			hf = CreateFont(10,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,OEM_CHARSET,
//					OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
//					VARIABLE_PITCH | FF_SWISS,NULL);
			holdf=SelectObject(ps.hdc,hf);
			SetTextColor(ps.hdc,RGB(255,255,255));
			str = "The Sinclair ZX Spectrum 48/128 Emulator";
			TextOut(ps.hdc,shw_x/2,140,str,strlen(str));
			str = "(c) 1999 Gerton Lunter";
			TextOut(ps.hdc,shw_x/2,160,str,strlen(str));
			SelectObject(ps.hdc,holdf);
			DeleteObject(hf);

			hf = CreateFont(-16,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,OEM_CHARSET,
					OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
					VARIABLE_PITCH | FF_SWISS,NULL);
			holdf=SelectObject(ps.hdc,hf);
			SetTextAlign(ps.hdc,TA_LEFT);
			SetRect(&rect,40,shw_y-130,shw_x-40,shw_y);
			DrawText(ps.hdc,lpszShareware,-1,&rect,DT_CENTER|DT_WORDBREAK);
			SelectObject(ps.hdc,holdf);
			DeleteObject(hf);

			EndPaint(hWnd,&ps);
			break;
		case WM_MOUSEMOVE:
			mousex = LOWORD(lPar);
			mousey = HIWORD(lPar);
			break;
		case WM_TIMER:
			l = GetTickCount();
			if (SharewareSessionTime < 0) {
				SharewareSessionTime = l;
				animstarttime = l;
			}
			do {
				long animtime = l - animstarttime;
				done = TRUE;
				show = TRUE;
				wx = wy = 0;
				switch (animation[animptr]) {
				case wait10:
					if (animtime > 10000) {
						animstarttime += 10000;
						done = FALSE;
					}
					show = FALSE;
					break;
				case wait1:
					if (animtime > 400) {
						animstarttime += 400;
						done = FALSE;
					}
					break;
				case wait2:
					if (animtime > 2000) {
						animstarttime += 2000;
						done = FALSE;
					}
					break;
				case newtoninit:
					vx = 0.0;
					vy = 0.0;
					alfa = 1-0.001;
					alfa2 = 0;
					x0 = animx;
					xc = (shw_x-okay_x)/2.0;
					y0 = animy;
					yc = animy;
					springc = -1/50000.0;
					springc2 = 0;
					curanimtime = 0;
					done = FALSE;
					break;
				case newtoninit2:
					vx = 0.0;
					vy = -0.75;
					alfa = 1-0.001;
					alfa2 = 1-0.0015;
					x0 = animx;
					xc = (shw_x-okay_x)/2.0;
					y0 = animy;
					yc = animy;
					springc = -1/50000.0;
					springc2 = -1/30000.0;
					curanimtime = 0;
					done = FALSE;
					break;
				case newtoninit3:
					// na mousenewton
					alfa = 1-0.005;
					alfa2 = 1-0.005;
					xc = (shw_x-okay_x)/2.0;
					yc = shw_y-225;
					springc = springc2 = -1/90000.0;
					curanimtime = 5000;
					animstarttime -= curanimtime;		// bit dirty, this...
					done = FALSE;
					break;
				case newton:
					while ((curanimtime < 10000) && (curanimtime < animtime)) {
						curanimtime++;
						vx = vx + (x0-xc)*springc;
						vy = vy + (y0-yc)*springc2;
						vx = vx * alfa;
						vy = vy * alfa2;
						x0 = x0 + vx;
						y0 = y0 + vy;
					}
					wx = x0 - animx;
					wy = y0 - animy;
					if (curanimtime >= 10000) {
						animx = x0;
						animy = y0;
						animstarttime += 10000;
						done = FALSE;
					}
					break;
				case mousenewtoninit:
					x0 = animx;
					y0 = animy;
					vx = 0.1;
					vy = 0.72;
					springc = -1/15.0;
					alfa = 1-0.001;
					done = FALSE;
					curanimtime = 0;
					break;
				case mousenewton:
					while ((curanimtime < 25000) && (curanimtime < animtime)) {
						float dist,mx,my;
						curanimtime++;
						mx = mousex - (x0 + (okay_x/2));
						if (mx>shw_x/2.0) mx-=shw_x;
						if (mx<-shw_x/2.0) mx+=shw_x;
						my = mousey - (y0 + (okay_y/2));
						if (my>shw_y/2.0) my-=shw_y;
						if (my<-shw_y/2.0) my+=shw_y;
						dist = mx*mx + my*my;
						if (dist<200.0)
							dist = 200.0;
						if (dist>200000.0)
							dist = -dist;
						dist = 1.0/dist;
						vx = vx + dist*mx*springc;
						vy = vy + dist*my*springc;
						vx = vx * alfa;
						vy = vy * alfa;
						x0 = x0 + vx;
						y0 = y0 + vy;
					}
					if (x0<-okay_x) x0+=shw_x+okay_x;
					if (x0>shw_x) x0-=shw_x+okay_x;
					if (y0<-okay_y) y0+=shw_y+okay_y;
					if (y0>shw_y) y0-=shw_y+okay_y;
					wx = x0 - animx;
					wy = y0 - animy;
					if (curanimtime >= 25000) {
						animx = x0;
						animy = y0;
						animstarttime += 25000;
						done = FALSE;
					}
					break;
				case bump:
					#define bumptime 375
					#define bumpx 44
					#define bumpy 30
					wx = ((long)bumpx*(long)animtime)/bumptime;
					wy = 2*(long)animtime - bumptime;
					wy = bumpy*wy*wy/((long)bumptime*(long)bumptime) - bumpy;
					if (animtime > bumptime) {
						animstarttime += bumptime;
						animx += bumpx;
						done = FALSE;
					}
					break;
				case smallbump:
					#define sbumptime 375
					#define sbumpx 0
					#define sbumpy 15
					wx = ((long)sbumpx*(long)animtime)/sbumptime;
					wy = 2*(long)animtime - sbumptime;
					wy = sbumpy*wy*wy/((long)sbumptime*(long)sbumptime) - sbumpy;
					if (animtime > sbumptime) {
						animstarttime += sbumptime;
						animx += sbumpx;
						done = FALSE;
					}
					break;
				case up:
					#define uptime 200
					#define upy 25
					if (animtime > uptime) {
						animstarttime += uptime;
						animy -= upy;
						done = FALSE;
					} else {
						wy = - ((long)upy*animtime)/uptime;
					}
					break;
				case qleft:
					#define qltime 200
					#define qlx -75
					if (animtime > qltime) {
						animstarttime += qltime;
						animx += qlx;
						done = FALSE;
					} else {
						wx = ((long)qlx*animtime)/qltime;
					}
					break;
				case sright:
					#define srtime 2000
					#define srx 75
					if (animtime > srtime) {
						animstarttime += srtime;
						animx += srx;
						done = FALSE;
					} else {
						wx = ((long)srx*animtime)/srtime;
					}
					break;
				case qright:
					#define qrtime 750
					#define qrx 310
					if (animtime > qrtime) {
						animstarttime += qrtime;
						animx += qrx;
						done = FALSE;
					} else {
						wx = ((long)qrx*animtime)/qrtime;
					}
					break;
				case okay:
					if (animdone) {
						show = FALSE;
						if ((animtime > 25000) && (animdone == 1)) {
							SendMessage(hOk, WM_SETTEXT, 0, "Well?");
							animdone++;
						}
					} else {
						animdone = 1;
						SendMessage(hOk, WM_SETTEXT, 0, "Okay !");
					}
				}
				if (!done) {
					animptr++;
				}
			} while (!done);
			if (show) {
				HDWP hdwp;
				hdwp=BeginDeferWindowPos(1);
				DeferWindowPos(hdwp,hOk,HWND_TOP,wx+animx,wy+animy,0,0,SWP_NOSIZE);
				EndDeferWindowPos(hdwp);
			}
			break;
		case WM_COMMAND:
			if ((wPar == BN_CLICKED) && (animdone))
				PostMessage(hWnd,WM_CLOSE,0,0);
			break;
		case WM_CLOSE:
			DestroyWindow(hOk);
			hWndShareware = NULL;
			DestroyWindow(hWnd);
			KillTimer(hWnd,hTimer);
			initall(SW_SHOW);			// ignore nCmdShow....
			SetClassLong(hWndMain,GCL_MENUNAME,"MAINMENU");
			if (glpszCmdLn[0]) {
				strcpy(gszDefaultFile,glpszCmdLn);
				PostMessage(hWndMain,WM_COMMAND,CM_OPENGLOBALFILE,0);
			}
			break;
		default:
			return DefWindowProc(hWnd,iMessage,wPar,lPar);
		}
	} else {
	// Indentation is wrong -- didn't bother
	switch(iMessage) {
	case WM_PAINT:
		BeginPaint(hWnd,&ps);
//		SelectPalette(ps.hdc,hPalette,FALSE);
//		RealizePalette(ps.hdc);
		if (hWnd==hWndMain) {
//			TouchRectangle(&ps.rcPaint);
			TouchAllBlocks();
			videobuf->updatevisibility=TRUE;
			if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked || useVz80d))
				UpdateVideo(page7fp);
			else
				UpdateVideo(SpecMem+16384);
			ShowScreen(TRUE);
		} else {
			DisplayHelpScreen();
		}
		EndPaint(hWnd,&ps);
		break;
	case WM_PALETTECHANGED:
		if (wPar!=hWnd) {
//			HPALETTE hp;
//			hdc=GetDC(hWnd);
//			hp=SelectPalette(hdc,hPalette,FALSE);
//			RealizePalette(hdc);
			if (hWnd==hWndMain) {
				TouchAllBlocks();
				videobuf->updatevisibility=TRUE;
				if ((hmode>=hm_128k)&&(state.hstate&0x08)&&(page7locked||useVz80d))
					UpdateVideo(page7fp);
				else
					UpdateVideo(SpecMem+16384);
				ShowScreen(TRUE);
			} else {
				DisplayHelpScreen();
			}
//			SelectPalette(hdc,hp,FALSE);
//			ReleaseDC(hWnd,hdc);
		}
		break;
	case WM_TIMER:
		if (passaltupdelay) passaltupdelay--;
		if (PassSysCommand) PassSysCommand--;
		BlockDone();         // remove any played samples from queue
		PollLoading();       // send empty blocks to sample reader, process filled ones
		executepiece();
		if ((soundtimehi > SHWTIME*60*50 ) && (!Played5Minutes)) {
			Played5Minutes = TRUE;
			state.speed = shw_speed;
			SpeedChanged = TRUE;
			message(SharewareTimeMsg);
		}
		break;
	case MM_WOM_DONE:
		BlockDone();
		break;
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
		return(DefWindowProc(hWndMain,iMessage,wPar,lPar));
	case WM_KILLFOCUS:
		if (hWnd==hWndMain) ClearKbd();
		break;
	case WM_SYSCOMMAND:
		w=wPar & 0xFFF0;
		if (((w==SC_MINIMIZE)||(w==SC_MOVE)||(w==SC_MOUSEMENU))&&(hWnd==hWndMain))
			ClearKbd();
		if ( ((w!=SC_HOTKEY)&&((w!=SC_KEYMENU)||PassSysCommand)) )
			return DefWindowProc(hWnd,iMessage,wPar,lPar);
		break;
	case WM_LBUTTONDBLCLK:
		if (hWnd==hWndMain) {
			int x,y;
			if (!hGifDlg) break;
			x = (LOWORD(lPar)/display.Xfac) - display.borsize;
			y = (HIWORD(lPar)/display.Yfac) - display.borsize;
			GifSetCoord(x,191-y);
		} else {
			HelpScreen(0);     // this closes the help window
		}
		break;
	case WM_SIZE:
		if ((wPar==SIZEFULLSCREEN)||(wPar==SIZENORMAL)) {
			displaytype *disp;
			int xf,yf,xs,ys;
			RECT r,s;
			if (hWnd==hWndMain) disp=&display; else disp=&hdisplay;
			GetClientRect(hWnd,&r);
			xf = max(1,(128-32+r.right-2*(*disp).borsize)/256);		// calculate sizing factor
			yf = max(1,(96-32+r.bottom-2*(*disp).borsize)/192);		// -32: large->small border problem
			xs = (256+2*(*disp).borsize)*xf;							// desired client size
			ys = (192+2*(*disp).borsize)*yf;
			(*disp).Xfac=xf;
			(*disp).Yfac=yf;
			AdjustVideoSize();
			if ((r.right != xs) || (r.bottom != ys)) {			// client size ok?
					GetWindowRect(hWnd,&s);								// no. Get window size
					s.right+=xs-s.left-r.right;
					s.bottom+=ys-s.top-r.bottom;
					SetWindowPos(hWnd,NULL,0,0,s.right,s.bottom,SWP_NOMOVE);
					if (hWnd==hWndMain)
						TouchAllBlocks();		// Make sure window is redrawn later
					else
						PostMessage(hWnd,WM_PAINT,0,0);
			}
		}
		if (hWnd==hWndMain) videobuf->updatevisibility=TRUE;
		break;
	case WM_WINDOWPOSCHANGED:
		lWndPos[0] = ((long)((LPWINDOWPOS)lPar)->x) +
						(((long)((LPWINDOWPOS)lPar)->y) << 16);
		goto Default;
	case WM_MOVE:
		if (hWnd==hWndMain) videobuf->updatevisibility=TRUE;
		break;
	case WM_DROPFILES:
		DragQueryFile((HANDLE)wPar,0,gszDefaultFile,128);
		DragFinish((HANDLE)wPar);
		goto openfile_any;
	case WM_COMMAND:
		switch(wPar) {
		case CM_OTHERINSTANCEFILE:
			strcpy(gszDefaultFile,(char*)lPar);
			// & continue
		case CM_OPENGLOBALFILE:		// command line file name
			openfile_any:
			i = FileDlg(3, gszDefaultFile, TRUE, NULL);
			goto openfile_cont;
		case CM_OPENFILE:				// menu item
			i = FileDlg(3, gszDefaultFile, TRUE, hWnd);
			openfile_cont:
			if (i==-1) break; /* invalid return, probably cancel */
			switch (i) {
				case 0:
					strcpy(gszZ80File,gszDefaultFile);
					goto OpenZ80File;
				case 1:
					strcpy(gszInFile, gszDefaultFile);
					if (!hPlaySample) {
						lpfnPlaySampleProc=MyMakeProcInstance(PlaySampleProc,ghInstance);
						hPlaySample=MyCreateDialogParam(ghInstance,"PLAYDIALOG",hWndMain,lpfnPlaySampleProc,CM_PLAYSAMPLE);
					}
					SendMessage(hPlaySample, WM_COMMAND, PSF_OPENFILE, CM_OPENGLOBALFILE);
					break;
				case 2:
					if (!hPlayTapDialog) {
						lpfnPlayTapProc=MyMakeProcInstance(PlayTapProc,ghInstance);
						hPlayTapDialog=MyCreateDialogParam(ghInstance,"PLAYTAP",hWndMain,lpfnPlayTapProc,CM_PLAYTAP);
					}
					SendMessage(hPlayTapDialog, WM_COMMAND, PT_OPEN, CM_OPENGLOBALFILE);
					break;
				case 3:
					notify(UnknownFileType);
					break;
				case 4:
					SendMessage(hWnd,WM_COMMAND,CM_MDRV,0);		// pop-up mdrv windows if necessary
					SendMessage(hMdrvDialog,WM_USER+1,0,(long)gszDefaultFile);
					break;
				case 5:
					SendMessage(hWnd,WM_COMMAND,CM_RS,0);			// pop-up rs232 window if necessary
					SendMessage(hRsDialog,WM_USER+4,0,(long)gszDefaultFile);
					break;
				case 6:
					SendMessage(hWnd,WM_COMMAND,CM_ALTROMS,0);	// pop-up rom window
					SendMessage(hRomDialog,WM_USER+1,0,(long)gszDefaultFile);
					break;
				case 7:
					strcpy(gszScrFile,gszDefaultFile);
					goto OpenScrFile;
					break;
			}
			break;
		case CM_LOADSNAP:
/* begin added/modified by jts 4/2/97 */
			i = FileDlg(0, gszZ80File, TRUE, hWnd);
			if (i==-1) break;
OpenZ80File:
			if (extensionsub == 1)
				loadz80type = LZF_VOC+1;		// sna
			else
				loadz80type = LZF_VOC;			// z80 (or slt)
			videobuf->updatevisibility=TRUE;
			handle=OpenRead(gszZ80File);
			if (handle==HFILE_ERROR) {
				notify(loadz80type==LZF_VOC?LoadZ80Error:LoadSNAError);
				break;
			}
			if (loadz80type==LZF_VOC) {      // i.e. Z80 file
				i=LoadZ80File(handle);
			} else {
				i=LoadSNAFile(handle);
			}
			if (i) {
				Reset();
				notify(i);
			}
			_lclose(handle);
			break;
		case CM_SAVESNAP:
/* begin added/modified by jts 4/2/97 */
// GL modified 4/27/97
			i = FileDlg(0, gszZ80File, FALSE, hWnd);
			videobuf->updatevisibility=TRUE;
			if (i==-1)	break; // Cancel;
			if (extensionsub == 1) {
				notify(WontSaveSNA);
				break;
			}
			if (extensionsub == 2) {
				notify(WontSaveSLT);
				break;
			}
			handle=OpenCreate(gszZ80File);
/* end added/modified by jts 4/2/97 */
			if (handle==HFILE_ERROR) {
				notify(SaveZ80Error);
				break;
			}
			i=SaveZ80File(handle);
			_lclose(handle);
			if (i) {
				notify(i);
			}
			break;
		case CM_LOADDEFAULT:
			ReadInitValues();
			initborsizes();
			SendMessage(hWndMain,WM_SIZE,SIZENORMAL,
				0xC00000L*display.Yfac+256*display.Xfac);
			InstallSettings();
			AllocBuffers();
			MyMessageBox(hWndMain,"Default settings loaded","Done:",MB_OK);
			break;
		case CM_LOADSCREEN:
			i = FileDlg(7,gszScrFile,TRUE,hWnd);
			if (i==-1) break;
			OpenScrFile:
			handle = OpenRead(gszScrFile);
			if (handle == HFILE_ERROR) {
				notify(LoadScreenError);
			} else {
				if (extensionsub==0) {
					if (loadscreen(handle)) {
						notify(LoadScreenError2);
					}
				} else {
					FILE *Fin = fdopen(handle,"rb");
					if (GifLoad(Fin)) {
						notify(LoadScreenError2);
					}
					fclose(Fin);
					handle = NULL;
				}
			}
			_lclose(handle);
			break;
		case CM_SAVESCREEN:
			if (hGifDlg) {
				MessageBeep(-1);
				break;
			}
			i = FileDlg(7,gszScrFile,FALSE,hWnd);
			if (i==-1) break;
			handle = OpenCreate(gszScrFile);
			if (handle==HFILE_ERROR) {
				notify(SaveScreenError);
			} else {
				if (extensionsub == 0) { 	// .scr
					if (savescreen(handle)) {
						notify(SaveScreenError2);
					}
				} else {							// .gif
			if (hRecordDialog) break;
					lpfnGifProc = MyMakeProcInstance(GifDlgProc,ghInstance);
					gifFout = fdopen(handle,"wb");
					hGifDlg = MyCreateDialogParam(ghInstance, "GIFDIALOG", hWndMain, lpfnGifProc, 0);
					handle = NULL;
				}
			}
			_lclose(handle);
			break;
		case CM_SAVEDEFAULTS:
			SaveSettings();
			MyMessageBox(hWndMain,"Current settings saved as defaults","Done:",MB_OK);
			break;
		case CM_INTELLIIN:
			ToggleMenu(hWnd,CM_INTELLIIN,&(char)iimode);
			break;
		case CM_FORMFEED:
			if (hPrinter) formfeed(); else MessageBeep(-1);
			break;
		case CM_ZXPRINTER:
			ToggleMenu(hWnd,CM_ZXPRINTER,&zxprinter);
			break;
		case CM_QUIT:
			if (hWnd!=hWndMain) {   // let HelpScreen() destroy help window
				HelpScreen(0);
			} else {
				DestroyWindow(hWnd);
			}
			break;
		case CM_COPPERING:
			ToggleMenu(hWnd,CM_COPPERING,&state.coppering);
			InstallSettings();
			break;
//		case CM_BW:
//			ToggleMenu(hWnd,CM_BW,&state.blackandwhite);
//			FreeVideo();
//			init_video();
//			break;
		case CM_SOUND:
			ToggleMenu(hWnd,CM_SOUND,(char*)&state.sound);
			sound = state.sound | state.record;
			AllocBuffers();
			break;
		case CM_PITCH:
			ToggleMenu(hWnd,CM_PITCH,(char*)&state.truepitch);
			AllocBuffers();
			break;
		case CM_RECORD:
			if (hRecordDialog) break;
			lpfnRecordProc=MyMakeProcInstance(RecordDialProc,ghInstance);
			hRecordDialog=MyCreateDialogParam(ghInstance,"RECORD",hWndMain,lpfnRecordProc,wPar);
			break;
		case CM_DEBUG:
			DebugWindow();
			break;
		case CM_PLAYSAMPLE:
			if (hPlaySample) {
				DestroyWindow(hPlaySample);
				hPlaySample=NULL;
				break;
			}
			lpfnPlaySampleProc=MyMakeProcInstance(PlaySampleProc,ghInstance);
			hPlaySample=MyCreateDialogParam(ghInstance,"PLAYDIALOG",hWndMain,lpfnPlaySampleProc,wPar);
			break;
		case CM_RECTAP:
			if (hRecTapDialog) {
				SetFocus(hRecTapDialog);
				break;
			}
			lpfnRecTapProc=MyMakeProcInstance(RecTapProc,ghInstance);
			hRecTapDialog=MyCreateDialogParam(ghInstance,"RECTAP",hWndMain,lpfnRecTapProc,wPar);
			break;
		case CM_PLAYTAP:
			if (hPlayTapDialog) {
				SetFocus(hPlayTapDialog);
				break;
			}
			lpfnPlayTapProc=MyMakeProcInstance(PlayTapProc,ghInstance);
			hPlayTapDialog=MyCreateDialogParam(ghInstance,"PLAYTAP",hWndMain,lpfnPlayTapProc,wPar);
			break;
		case CM_INFO:
			if (hInfoBox) {
				SetFocus(hInfoBox);
				break;
			}
			hInfoBox = MyCreateDialogParam(ghInstance,"InfoBox",hWndMain,InfoBoxProc,wPar);
			break;
		case CM_ALTROMS:
			if (hRomDialog) break;
			lpfnRomProc=MyMakeProcInstance(RomProc,ghInstance);
			hRomDialog=MyCreateDialogParam(ghInstance,"RomDialog",hWndMain,lpfnRomProc,wPar);
			break;
		case CM_ISSUE2:
			ToggleMenu(hWnd,CM_ISSUE2,&issue2);
			break;
		case CM_128SHIFT:
			ToggleMenu(hWnd,CM_128SHIFT,&bAlways128kshift);
			break;
		case CM_HELPSCREEN:
			HelpScreen(1);
			break;
		case CM_CURSOR:
			currentjoystick=0;
			SetJoystick(currentjoystick);
			break;
		case CM_KEMPSTON:
			currentjoystick=5;
			SetJoystick(currentjoystick);
			break;
		case CM_SINCLAIR:
			currentjoystick=15;
			SetJoystick(currentjoystick);
			break;
		case CM_USERDEFJOY:
			currentjoystick=10;
			SetJoystick(currentjoystick);
			break;
		case CM_DEFKEYS:
			if (hJoyDialog) break;
			lpfnJoyDialog=MyMakeProcInstance(JoyDialogProc,ghInstance);
			hJoyDialog=MyCreateDialogParam(ghInstance,"JOYDIALOG",hWndMain,lpfnJoyDialog,wPar);
			break;
		case CM_MDRV:
			if (hMdrvDialog) {
				SetFocus(hMdrvDialog);
				break;
			}
			lpfnMdrvDialog=MyMakeProcInstance(MdrvDialogProc,ghInstance);
			hMdrvDialog=MyCreateDialogParam(ghInstance,"MDRDIALOG",hWndMain,lpfnMdrvDialog,wPar);
			break;
		case CM_RS:
			if (hRsDialog) break;
			lpfnRsDialog=MyMakeProcInstance(RsDialogProc,ghInstance);
			hRsDialog=MyCreateDialogParam(ghInstance,"RSDIALOG",hWndMain,lpfnRsDialog,wPar);
			break;
		case CM_SO:
			if (hSODialog) break;
			lpfnSOProc=MyMakeProcInstance(SOProc,ghInstance);
			hSODialog=MyCreateDialogParam(ghInstance,"SoundOutputDialog",hWndMain,lpfnSOProc,wPar);
			break;
		case CM_HELP:
			WinHelp(hWndMain,gszHelpFile,HELP_INDEX,0);
			break;
		case CM_CONTROL:
			if (hCtrlDialog) {
				DestroyWindow(hCtrlDialog);
			} else {
				lpfnCtrlDialog=MyMakeProcInstance(CtrlDialogProc,ghInstance);
				hCtrlDialog=MyCreateDialogParam(ghInstance,"CONTROLDIALOG",hWndMain,lpfnCtrlDialog,wPar);
			}
			break;
		case CM_LARGEBORDER:
			display.borsize=(display.borsize==32);
			ToggleMenu(hWnd,CM_LARGEBORDER,&(char)display.borsize);
			display.borsize=16+16*display.borsize;
			initborsizes();
			SendMessage(hWndMain,WM_SIZE,SIZENORMAL,
				0x10000L*(2*display.borsize+192)*display.Yfac+
				(2*display.borsize+256)*display.Xfac);
			break;
		case CM_SPEED:
			if (hSpeedDialog) {
				SetFocus(hSpeedDialog);
				break;
			}
			lpfnSpeedDialog=MyMakeProcInstance(SpeedDialogProc,ghInstance);
			hSpeedDialog=MyCreateDialogParam(ghInstance,"SPEEDDIALOG",hWndMain,lpfnSpeedDialog,wPar);
			break;
		case CM_HARDWARE:
			if (hHardwareDialog) {
				SetFocus(hHardwareDialog);
				break;
			}
			lpfnHardwareProc=MyMakeProcInstance(HardwareProc,ghInstance);
			hHardwareDialog=MyCreateDialogParam(ghInstance,"CHANGEHARDWARE",hWndMain,lpfnHardwareProc,wPar);
			break;
//		case CM_LISTENMIC:
//			feearmicmask=(feearmicmask==8);
//			ToggleMenu(hWndMain,CM_LISTENMIC,&(char)feearmicmask);
//			feearmicmask=16-8*feearmicmask;
//			break;
		}
		break;
	case IK_FREELPFN:
		MyFreeProcInstance((FARPROC)lPar);
		RepaintMainD((HGLOBAL)wPar);
		break;
	case WM_SHOWWINDOW:		// windows is to be hidden or shown
		if (hWnd==hWndMain) videobuf->updatevisibility=TRUE;
		goto Default;
	case WM_CLOSE:
		if (hWnd!=hWndMain) {   // let HelpScreen() destroy help window
			HelpScreen(0);
		} else {
			DestroyWindow(hWnd);
		}
		break;
	case WM_DESTROY:
		if (hWnd!=hWndMain) return 0L;			// help screen window or shareware window
		KillTimer(hWndMain,hTimer);
		FreeVideo();
		FlushLogBuf();
		quitprinter();
		currah_end();
		if (hSpeedDialog) {
			DestroyWindow(hSpeedDialog);
		}
		if (hRecordDialog) {
			FlushRecordBuf();
			WriteSampleTrailer();
			DestroyWindow(hRecordDialog);
			state.record=FALSE;
		}
		if (hCtrlDialog) {
			DestroyWindow(hCtrlDialog);
		}
		if (hJoyDialog) {
			DestroyWindow(hJoyDialog);
		}
		if (hSODialog) {
			DestroyWindow(hSODialog);
		}
		if (hPlaySample) {
			DestroyWindow(hPlaySample);
			hPlaySample=NULL;
		}
		if (hPlayTapDialog) {
			DestroyWindow(hPlayTapDialog);
		}
		if (hRecTapDialog) {
			DestroyWindow(hRecTapDialog);
		}
		if (hRecTapDialog) {
			DestroyWindow(hRecTapDialog);
		}
		if (hHardwareDialog) {
			DestroyWindow(hHardwareDialog);
		}
		if (hMdrvDialog) {
			DestroyWindow(hMdrvDialog);
		}
		if (hRsDialog) {
			DestroyWindow(hRsDialog);
		}
		if (hInfoBox) {
			DestroyWindow(hInfoBox);
		}
		if (hInfoMemory) {
			GlobalFree(hInfoMemory);
		}
		if (hInBuf) {
			GlobalUnlock(hInBuf);
			GlobalFree(hInBuf);
		}
		if (hWarBuffer) {
			GlobalUnlock(hWarBuffer);
			GlobalFree(hWarBuffer);
		}
		if (hLoadDlg) {
			DestroyWindow(hLoadDlg);
		}
		FreeLoadMemChain();
		QuitSound();
		if (FDebug) fclose(FDebug);
		dealloc_memory();
		WinHelp(hWndMain,gszHelpFile,HELP_QUIT,0);
		PostQuitMessage(0);
		DeleteObject(hbrBackground);
		break;
	default:
	Default:
		return DefWindowProc(hWnd,iMessage,wPar,lPar);
	}
	} // Bad indentation; it's the "if(bSharewareSession) { blabla } else {" above
	return(0L);
}

