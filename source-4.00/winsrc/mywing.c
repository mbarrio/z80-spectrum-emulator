// mywing.c -- machinery to dynamically load the WinG library, so as to
//             be able to run without WinG installed.

// Pointers to the actual library routines

HDC _loadds WINAPI (FAR *pWinGCreateDC)( void );
BOOL _loadds WINAPI (FAR *pWinGRecommendDIBFormat)( BITMAPINFO FAR * );
HBITMAP _loadds WINAPI (FAR *pWinGCreateBitmap)( HDC , BITMAPINFO const FAR *,
		  void FAR *FAR * );
void FAR * _loadds WINAPI (FAR *pWinGGetDIBPointer)( HBITMAP , BITMAPINFO FAR * );
UINT _loadds WINAPI (FAR *pWinGSetDIBColorTable)( HDC , UINT , UINT , RGBQUAD const FAR * );
UINT _loadds WINAPI (FAR *pWinGGetDIBColorTable)( HDC , UINT , UINT , RGBQUAD const FAR * );
BOOL _loadds WINAPI (FAR *pWinGBitBlt)( HDC , int ,
		  int , int , int , HDC , int , int );
BOOL _loadds WINAPI (FAR *pWinGStretchBlt)( HDC , int ,
		  int , int , int , HDC , int , int , int , int );

#define myWinGCreateDC (*pWinGCreateDC)
#define myWinGRecommendDIBFormat (*pWinGRecommendDIBFormat)
#define myWinGCreateBitmap (*pWinGCreateBitmap)
#define myWinGGetDIBPointer (*pWinGGetDIBPointer)
#define myWinGSetDIBColorTable (*pWinGSetDIBColorTable)
#define myWinGGetDIBColorTable (*pWinGGetDIBColorTable)
#define myWinGBitBlt (*pWinGBitBlt)
#define myWinGStretchBlt (*pWinGStretchBlt)

// Other variables

HINSTANCE   hWinGInstance=0;

// Library initialisation routine

void InitWinGLib(void)
{
		  if (!gWinG) {
				WriteInfoString("WinG graphics disabled in WinZ80.INI");
				hWinGInstance = NULL;
				return;
		  }
		  hWinGInstance = LoadLibrary("WING.DLL");
		  if (hWinGInstance <= HINSTANCE_ERROR) {
				linkerr:
				if (hWinGInstance > HINSTANCE_ERROR) {
					FreeLibrary(hWinGInstance);
					WriteInfoString("Error initialising WinG library - wrong version?  Using slow graphics.");
				} else {
					WriteInfoString("WinG graphics library not loaded (probably not found).  Using slow graphics.");
				}
				/* fatalerror(fatalWinGerr); */
				gWinG=0;
				hWinGInstance = NULL;
				return;
		  }
		  WriteInfoString("Graphics library WinG loaded.");
		  (FARPROC)pWinGCreateDC = GetProcAddress(hWinGInstance,"WinGCreateDC");
		  if (!pWinGCreateDC) goto linkerr;
		  (FARPROC)pWinGRecommendDIBFormat = GetProcAddress(hWinGInstance,"WinGRecommendDIBFormat");
		  if (!pWinGRecommendDIBFormat) goto linkerr;
		  (FARPROC)pWinGCreateBitmap = GetProcAddress(hWinGInstance,"WinGCreateBitmap");
		  if (!pWinGCreateBitmap) goto linkerr;
		  (FARPROC)pWinGGetDIBPointer = GetProcAddress(hWinGInstance,"WinGGetDIBPointer");
		  if (!pWinGGetDIBPointer) goto linkerr;
		  (FARPROC)pWinGSetDIBColorTable = GetProcAddress(hWinGInstance,"WinGSetDIBColorTable");
		  if (!pWinGSetDIBColorTable) goto linkerr;
		  (FARPROC)pWinGGetDIBColorTable = GetProcAddress(hWinGInstance,"WinGGetDIBColorTable");
		  if (!pWinGGetDIBColorTable) goto linkerr;
		  (FARPROC)pWinGBitBlt = GetProcAddress(hWinGInstance,"WinGBitBlt");
		  if (!pWinGBitBlt) goto linkerr;
		  (FARPROC)pWinGStretchBlt = GetProcAddress(hWinGInstance,"WinGStretchBlt");
		  if (!pWinGStretchBlt) goto linkerr;
		  return;
}

void FreeWinGLib(void)
{
		  if (!hWinGInstance) return;
		  FreeLibrary(hWinGInstance);
		  return;
}


