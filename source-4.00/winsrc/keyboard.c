#include <windows.h>
#include "helpmap.h"
#include "spectrum.h"

// 0-4: Shift-V
// 5-9: A-G
// 10-14: Q-T
// 15-19: 1-5
// 20-24: 0-6
// 25-29: P-Y
// 30-34: Enter-H
// 35-39: Space-B

#define depressbuflen 32

char KeyMap[9];    // Actual bitmap, read by emulator (last byte=Kempston)
int KeyDepressCount[45];
int Shifted,Ctrled,Alted;
int KeyTable[128];
char depresskey[depressbuflen];
char depressdown[depressbuflen];
int depresstail,depresshead;
char passaltup;
int passaltupdelay;
const char KeyTableInit[]={                     \
	VK_SHIFT,0,'Z',1,'X',2,'C',3,'V',4,          \
	'A',5,'S',6,'D',7,'F',8,'G',9,               \
	'Q',10,'W',11,'E',12,'R',13,'T',14,          \
	'1',15,'2',16,'3',17,'4',18,'5',19,          \
	'0',20,'9',21,'8',22,'7',23,'6',24,          \
	'P',25,'O',26,'I',27,'U',28,'Y',29,          \
	VK_RETURN,30,'L',31,'K',32,'J',33,'H',34,    \
	VK_SPACE,35,VK_CONTROL,36,'M',37,'N',38,'B',39,0};
int DoubleKeys[32];
char DoubleKeyInit[]={'\"',25,'\'',23,':',1,';',26,  \
	',',38,'.',37,'<',13,'>',14,'/',4,'?',3,    \
	'-',33,'_',20,'+',32,'=',31,0};
char joysticks[]={
	19,22,24,23,20,        // Cursor
	41,40,42,43,44,        // Kempston
	15,16,17,18,19,        // Sinclair 1 or User Defined
	24,23,22,21,20};       // Sinclair 2
int currentjoystick;
BOOL bAlways128kshift;



void InitKbd(void)
{
	int i;
	ClearKbd();
	for (i=0;i<128;i++) KeyTable[i]=-1;
	for (i=0;KeyTableInit[i];i+=2)
		KeyTable[KeyTableInit[i]]=KeyTableInit[i+1];
	for (i=0;DoubleKeyInit[i];i+=2) {
		DoubleKeys[i]=VkKeyScan(DoubleKeyInit[i]);
		DoubleKeys[i+1]=DoubleKeyInit[i+1];
	}
	DoubleKeys[i]=0;
	currentjoystick=0;
	SetJoystick(currentjoystick);
	passaltup=FALSE;
	passaltupdelay=0;
	depresstail=depresshead=0;
}


void ClearKbd(void)
{
	int i;
	for (i=0;i<9;i++) KeyMap[i]=0xFF;
	for (i=0;i<45;i++) KeyDepressCount[i]=0;
	Shifted=Ctrled=Alted=0;
	passaltup=FALSE;
	passaltupdelay=0;
	depresstail=depresshead=0;
}


void TranslateJoystickSetting()
// Let's do the shuffle
{
	int i;
	for (i=0;i<5;i++) {
		int j,k;
		j=256;
		k=0;
		while (!(j & z80header.kbdmap[i]) && (k<4)) {
			k++;
			j<<=1;
		}
		joysticks[10+i]=k + 5*(z80header.kbdmap[i] & 7);
	}
}


void ActDepressKey(char key, char keydown)
{
	if (keydown) {
		KeyDepressCount[key]++;
		KeyMap[key/5] &= (0xFF - (1 << (key % 5)));
	} else {
		if (KeyDepressCount[key]) KeyDepressCount[key]--;
		if (!KeyDepressCount[key])
			KeyMap[key/5] |= (1 << (key % 5));
	}
}

void DepressKey(char key, char keydown)
{
	depresskey[depresshead]=key;
	depressdown[depresshead]=keydown;
	depresshead=(depresshead+1)%depressbuflen;
	if (depresshead==depresstail) {
		ActDepressKey(depresskey[depresstail],depressdown[depresstail]);
		depresstail=(depresstail+1)%depressbuflen;
	}
}

void DepressKeyCancel(char key, char keydown)
// Same as above, except that if keydown=FALSE, and a previous key in
// buffer ==key, then it eats that one
{
	if ((!keydown)&&(depresshead!=depresstail)) {
	  if (depresskey[(depresshead-1+depressbuflen)%depressbuflen] ==
				key) {
		  depresshead = (depresshead-1+depressbuflen)%depressbuflen;
		  return;
	  }
	}
	DepressKey(key,keydown);
}

void FlushKeyBuf(int tailptr, int numslices, int slice)
// tailptr points to first key to be sent in first slice
// slice \in [0,numslices-1]
{
	int tosend;
	if (numslices==0) return;
	tosend =
	 1+(((depresshead-tailptr+depressbuflen)%depressbuflen)*slice)/numslices;
	while ((depresstail != ((tosend+tailptr)%depressbuflen)) &&
			  (depresstail != depresshead) ) {
		ActDepressKey(depresskey[depresstail],depressdown[depresstail]);
		depresstail = (depresstail+1)%depressbuflen;
	}
}




// Key presses are dispatched to window, but also directly routed to
//  TranslateKbd.
// Main window passes system key messages on if passaltup==TRUE
// It looks for keyups for alt and tab, and when both are up, passaltup==0
// It continues passing on key msgs for 2 timer messages.

LONG TranslateKbd(WORD wPar, LONG lPar, int iMessage)
{
#define AltPressed 0x20000000L
#define KeyUp 0x80000000L
#define KeyRepeat 0x40000000L
	char keydown=!(lPar & KeyUp);
	char c;
	int context;
	int i,j,flg;
	char shift128k;
	if ((lPar & KeyRepeat)&&keydown) return(0L);
	if (wPar<128) if ((c=KeyTable[wPar]) != -1) {
		if (wPar==VK_SHIFT) {
			if (keydown) Shifted++; else if (Shifted) Shifted=0;
		}
		if (wPar==VK_CONTROL) {
			if (keydown) Ctrled++; else if (Ctrled) Ctrled=0;
		}
		if (CatchKeys) {
			if ((keydown)&&(hJoyDialog)) {
				i=wPar;
				if (i==VK_SHIFT) i='[';
				if (i==VK_RETURN) i='/';
				if (i==VK_SPACE) i='\\';
				if (i==VK_CONTROL) i=']';
				PostMessage(hJoyDialog,WM_USER+1,i,c/5+(0x100<<(c%5)));
			}
			return (0L);
		}
		DepressKey(c,keydown);
		return (0L);
	}
	if (!keydown) {
		if (wPar==VK_MENU) passaltup &= 2;     // reset bit 0 (alt)
		if (wPar==VK_TAB) passaltup &= 1;      // reset bit 1 (tab)
	}
	shift128k = bAlways128kshift && (hmode>=hm_128k) && (hmode<=hm_128kmgt);
	switch(wPar) {
		case VK_MENU:              // ALT-key
			if (keydown) {
				Alted=1;
				if (CatchKeys && hJoyDialog)
					PostMessage(hJoyDialog,WM_USER+1,']',0x207);
			} else {
				Alted=0;
			}
			DepressKey(36,keydown);
			break;
		case VK_ADD:
			DepressKey(36,keydown);
			DepressKey(32,keydown);
			break;
		case VK_BACK:
			DepressKey(0,keydown);
			DepressKey(20,keydown);
			break;
		case VK_CAPITAL:
			DepressKey(0,keydown);
			DepressKey(16,keydown);
			break;
		case VK_MULTIPLY:
			DepressKey(36,keydown);
			DepressKey(39,keydown);
			break;
		case VK_DIVIDE:
			DepressKey(36,keydown);
			DepressKey(4,keydown);
			break;
		case VK_ESCAPE:
			DepressKey(0,keydown);
			DepressKey(15,keydown);
			break;
		case VK_SUBTRACT:
			DepressKey(36,keydown);
			DepressKey(33,keydown);
			break;
		case VK_LEFT:
		case VK_NUMPAD4:
			if ((currentjoystick==0)&&(shift128k || (GetKeyState(VK_NUMLOCK)&1)))
				DepressKey(0,keydown);  // problem: Left+Numlock Off+Left Release
			DepressKey(joysticks[currentjoystick],keydown);
			break;
		case VK_RIGHT:
		case VK_NUMPAD6:
			if ((currentjoystick==0)&&(shift128k || (GetKeyState(VK_NUMLOCK)&1)))
				DepressKey(0,keydown);
			DepressKey(joysticks[currentjoystick+1],keydown);
			break;
		case VK_DOWN:
		case VK_NUMPAD2:
			if ((currentjoystick==0)&&(shift128k || (GetKeyState(VK_NUMLOCK)&1)))
				DepressKey(0,keydown);
			DepressKey(joysticks[currentjoystick+2],keydown);
			break;
		case VK_UP:
		case VK_NUMPAD8:
			if ((currentjoystick==0)&&(shift128k || (GetKeyState(VK_NUMLOCK)&1)))
				DepressKey(0,keydown);
			DepressKey(joysticks[currentjoystick+3],keydown);
			break;
		case VK_TAB:
			if (Alted&&keydown) {
				if (!passaltup) {
//               DefWindowProc(hWndMain,WM_SYSKEYDOWN,18,0x20380001L);
//               DefWindowProc(hWndMain,WM_SYSKEYDOWN,9,0x200F0001L);
				}
				passaltup=3;
			} else {
				DepressKey(joysticks[currentjoystick+4],keydown);
			}
			break;
		case VK_NUMPAD5:
		case VK_NUMPAD0:
		case VK_DECIMAL:
		case VK_INSERT:
		case VK_DELETE:
			DepressKey(joysticks[currentjoystick+4],keydown);
			break;
		case VK_NUMLOCK:
			{
				BYTE buf[256];
				GetKeyboardState(buf);
				SetKeyboardState(buf);
			}
			break;
		case VK_F1:
			if (keydown && Alted) PostMessage(hWndMain,WM_COMMAND,CM_HELPSCREEN,0);
			if (keydown && !Alted) {
				HWND hActive = GetActiveWindow();
				context = 0;
				if (hActive == hHardwareDialog) context = chardwarec;
				if (hActive == hSpeedDialog) context = cspeedc;
				if (hActive == hRsDialog) context = crs232c;
				if (hActive == hMdrvDialog) context = cmicrodrivec;
				if (context)
					WinHelp(hWndMain,gszHelpFile,HELP_CONTEXT,context);
				else
					WinHelp(hWndMain,gszHelpFile,HELP_INDEX,0);
			}
			break;
		case VK_F2:
			if (keydown && Alted) PostMessage(hWndMain,WM_COMMAND,CM_SAVESCREEN,0);
			if (keydown && !Alted) PostMessage(hWndMain,WM_COMMAND,CM_SAVESNAP,0);
			break;
		case VK_F3:
			if (keydown && Alted) PostMessage(hWndMain,WM_COMMAND,CM_LOADSCREEN,0);
			if (keydown && !Alted) PostMessage(hWndMain,WM_COMMAND,CM_LOADSNAP,0);
			break;
		case VK_F4:
			if (keydown && Alted) PostMessage(hWndMain,WM_CLOSE,0L,0);
			if (keydown && (!Alted)) PostMessage(hWndMain,WM_COMMAND,CM_SPEED,0);
			break;
		case VK_F5:
			if (Alted && keydown) Reset();
			else if (keydown) Nmi();
			break;
		case VK_F6:
			if (keydown) SetPauseState(!state.paused);
			break;
		case VK_F7:
			if (Alted && keydown) PostMessage(hWndMain,WM_COMMAND,CM_RECTAP,0);
			if ((!Alted) && keydown) PostMessage(hWndMain,WM_COMMAND,CM_PLAYTAP,0);
			break;
		case VK_F8:
			if (keydown) PostMessage(hWndMain,WM_COMMAND,CM_MDRV,0);
			break;
		case VK_F9:
			if (keydown) PostMessage(hWndMain,WM_COMMAND,CM_HARDWARE,0);
			break;
		case VK_F10:
			if (!keydown) {
				ClearKbd();
				SetFocus(hWndMain);
				PassSysCommand=3;		// After three timer msgs it'll be zero again
			}
			break;
		default:
			i=wPar;
			if (keydown) {
				if (Shifted) i|=0x100;
				if (Ctrled) i|=0x200;
				if (Alted) i|=0x400;
				flg=0x7FF;
			} else {
				flg=0xFF;   // to make sure SHFT+(")+UNSHFT+RELEASE(") is handled OK
			}
			for (j=0;DoubleKeys[j];j+=2) {
				if ((DoubleKeys[j]&flg)==i) {
					if (i & 0x100) DepressKeyCancel(0,!keydown);
					if (i & 0x200) DepressKeyCancel(36,!keydown);
					DepressKey(36,keydown);
					DepressKey(DoubleKeys[j+1],keydown);
				}
			}
			break;
	}
	return (0L);
}

BOOL SpacePressed(void)
{
	return !(KeyMap[7]&1);
}

