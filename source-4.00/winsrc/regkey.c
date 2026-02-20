#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include "spectrum.h"

char RegName[512];
char RegKey[512];

DWORD CheckSum(DWORD);

void RetrieveKey(HKEY hkey)
{
	long bc;
	bc=512;
	RegName[0]=RegKey[0]=0;
	RegQueryValue(hkey,"Name",RegName,&bc);
	bc=512;
	RegQueryValue(hkey,"Key",RegKey,&bc);
}

void RetrieveIniKey()
{
	GetPrivateProfileString(gszAppName,"Name","",RegName,512,gszIniFile);
	GetPrivateProfileString(gszAppName,"Key","",RegKey,512,gszIniFile);
}

BOOL CheckKey()
{
	DWORD chk0=0,chk1=0,chk2=0;
	sscanf(RegKey,"%lx %lx %lx",&chk0,&chk1,&chk2);
    chk0 -= CheckSum(0x1053938fL);
    chk1 -= CheckSum(0x48890097L);
    chk2 -= CheckSum(0xDCBA4DBFL);
	return !( (chk0==0)&&(chk1==0)&&(chk2==0) );
}

DWORD CheckSum(DWORD s)
{
	int i=0;
	DWORD t,u;
	while (RegName[i]) {
		t = (unsigned char)RegName[i];
        t *= 0x10204081L;
        t ^= 0xB3A2FE34L + s;
        u = (t & 0xFFF00000L);
        t = (t & 0x000FFFFFL);
		t += u;
        s = t*175;
		i++;
	}
	return s;
}







