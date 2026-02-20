typedef char             bool;
typedef unsigned char    byte;
typedef unsigned short   word;                                                                                /* (Must be 16-bit) */
typedef unsigned long    dword;                                                                               /* (Must be 32-bit) */

#ifndef TRUE
#define TRUE             (bool)1
#define FALSE            (bool)0
#endif

void   GIFSetOutFile					  ( FILE * f );
byte   GIFCreate                    (char *FileName, short Width, short Height, short NumColours, short ColourRes,
															bool GIF89a);
void   GIFSetColour                 (byte ColourNum, byte Red, byte Green, byte Blue);
byte   GIFWriteGlobalColorTable     (bool Loop);
byte   GIFCompressImage             (short StartX, short StartY, int Width, int Height,
															short (*GetPixelFunction)(short PixX, short PixY), bool GIF89a, word PictureDelayTime);
byte   GIFClose                     (void);
