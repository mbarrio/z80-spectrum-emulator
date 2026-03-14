#include <windows.h>
#include "spectrum.h"

#define debugfilename "d:\\bc4\\spectrum\\debug.txt"

char debugstr[100];
int  debuginited=FALSE;

int AddFile(char *path,char *file)
// Adds filename to path
// If path ends in filename, filename is replaced by file
// If path does not end in '\', it is added
// If file name starts with '\', original path is overwritten
// If file name starts with '/', it is treated as an extension and will replace
//		the original extension, or will be added.
{
	int i,j;
	int pathend=-1;
	int fileend=-1;
	BOOL k=FALSE;
	for (i=0;path[i];i++) {
		if (path[i]=='\\') {
			pathend=i;
			fileend=-1;
			k=FALSE;
		}
		if (path[i]=='.') {
			k=TRUE;
			fileend=i;
		}
		if (path[i]=='*') k=TRUE;	// last portion is file name
		if (path[i]==':') {
			pathend=i;
			fileend=-1;
			k=FALSE;
		}
	}
	j=0;
	if (k) i=pathend+1;	  // start of file name
	if (file[0]=='\\') i=0;	// overwrite existing path
	if (file[0]=='/') {
		if (fileend>=0) i=fileend;
		j=1;
	}
	k=i;						// first position that gets overwritten
	// if there is a path, and we're not adding extensions, and there's no \, add \
	if (i) if (!j) if (path[i-1]!='\\') path[i++]='\\';
	for (;file[j];j++) path[i++]=file[j];
	path[i]=0;
	return(k);
}


void initdebugfile(void)
{
   _lclose(_lcreat(debugfilename,0));
}

void writedebugstr(void)
{
	HFILE hfile;

   if (!debuginited) initdebugfile();
   debuginited=TRUE;
   hfile=_lopen(debugfilename,READ_WRITE);
   _llseek(hfile,0,2);
   _lwrite(hfile,debugstr,lstrlen(debugstr));
   _lclose(hfile);
}

