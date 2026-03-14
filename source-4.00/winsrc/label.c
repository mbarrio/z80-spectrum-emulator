#ifdef __TURBOC__

#include <windows.h>
#include "spectrum.h"

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>    /* toupper */

#include "label.h"

#define maxstrings 2000

int  numIds;
Ident Idtable[maxstrings];
int  stringsortchar[maxstrings];
char stringcharsorted;
int newstringindex;
int  stringsortval[maxstrings];
char stringvalsorted;
int  curval;

char   *strtable;                   /* voor GetString en StoreString */
long   strtablelen;
long   firsttempstring;
int    firsttempId;

LblRef *lblrefs;
int    numlblrefs;
int    firstfreelblref;


/* #define PrDebug(f,s) printf(f,s);  */

#define PrDebug(f,s)

#define warningf(form,oper) {char s[256]; sprintf(s,form,oper); warning(s);}
#define errorf(form,oper) {char s[256]; sprintf(s,form,oper); error(s);}

void IdInit(void)
{
  numIds = 0;
  stringcharsorted = 1;
  stringvalsorted = 1;
  strtable=NULL;
  strtablelen=0;
  firsttempstring=-1;
  lblrefs=NULL;
  numlblrefs=0;
  firstfreelblref=-1;
}

char *GetString(long i)
{
  if ((0<=i)&&(i<strtablelen)) {
	 return strtable+i;
  }
  error ("GetString: Internal error?!");
  return NULL;
}

long StoreString(char *s)
{
  long l=strtablelen;
  strtablelen += strlen(s)+1;
  strtable = realloc(strtable,strtablelen);
  strcpy(strtable+l, s);
  PrDebug("Stored string: %s\n",s);
  return l;
}

char Compare(char *s1, char *s2)
{
  int i=0;
  while (toupper(s1[i])==toupper(s2[i])) {
	 if (s1[i]==0) return 0;
	 i++;
  }
  if (toupper(s1[i])<toupper(s2[i])) return -1;
  return 1;
}

int IdCompareStr(const void *id1, const void *id2)
{
  return Compare(GetString(Idtable[*(int*)id1].stringid),
					  GetString(Idtable[*(int*)id2].stringid));
}

int IdCompareVal(const void *id1, const void *id2)
{
  return (int)(Idtable[*(int*)id1].value & 0x3fff) -
			(int)(Idtable[*(int*)id2].value & 0x3fff);
}

void SortIdString(void)
{
  int i;
  for (i=0;i<numIds;i++) stringsortchar[i]=i;
  qsort(stringsortchar, numIds, sizeof(int), IdCompareStr);
  stringcharsorted = 1;
}

void SortIdVal(void)
{
  int i;
  for (i=0;i<numIds;i++) stringsortval[i]=i;
  qsort(stringsortval, numIds, sizeof(int), IdCompareVal);
  stringvalsorted = 1;
}

int FindNext(unsigned int addr)
{
	curval++;
	if (curval>=numIds) return -1;
	if ((GetIdent(stringsortval[curval])->value ^ addr) & 0x3fff) {
		return -1;
	}
	return stringsortval[curval];
}

int FindFirstValue(unsigned int addr)
{
	int l=0, r=numIds, m;
	int c;
	if (!stringvalsorted)
		SortIdVal();
	while (l<r) {		/* invar: str[k]<s (k=0..l-1), str[k]>=s (k=r,..) */
		m = (l+r)/2;	/* l<=m<r */
		c = (GetIdent(stringsortval[m])->value & 0x3fff) - (addr&0x3fff);
		if (c<0)
			l=m+1;
		else
			r=m;
	}
	curval = l-1;
	return FindNext(addr);
}


char *FindAddrLabel(unsigned int addr)
{
	int lbl=FindFirstValue(addr);
	char bank=CurrentBank(addr);
	while (lbl != -1) {
		if  ((GetIdent(lbl)->bank == bank) &&
			  ((GetIdent(lbl)->type & ~LBL_PERMMASK) == LBL_ADDR)) {
			return GetString(GetIdent(lbl)->stringid);
		}
		lbl = FindNext(addr);
	}
	return NULL;
}


int GetIdIndex(char *s)       /* -1 als niet gevonden.  Dan wordt positie van
 string s opgeslagen in globale var newstringindex, voor AddTempString-
 beetje vies, wel lekker */
{
  int l=0, r=numIds;
  int m;
  char c;
  PrDebug("Looking for string: %s\n",s);
  if (!stringcharsorted)
	 SortIdString();
  while (l<r) {     /* invar: str[k]<s (k=0..l-1), str[k]>s (k=r,...) */
	 m = (l+r)/2;    /* l<=m<r */
	 c = Compare(GetString(Idtable[stringsortchar[m]].stringid),s);
	 if (c==0) {
		m=stringsortchar[m];
		PrDebug("Found: %u\n",m);
		return m;
	 }
	 if (c<0)
		l = m+1;
	 else
		r = m;
  }
  newstringindex = l;
  PrDebug("Not found.\n",0);
  return -1;
}

int AddTempString(char *s)
{
  long l;
  int i;
  PrDebug("Adding string: %s",s);
  if (firsttempstring == -1) {
	 firsttempstring=strtablelen;
	 firsttempId = numIds;
  }
  l=StoreString(s);
  Idtable[numIds].stringid = l;
  Idtable[numIds].value = VAL_NOREF;
  Idtable[numIds].type = LBL_UNDEF;
  stringvalsorted = 0;
  if (stringcharsorted) {
	for (i=numIds;i>newstringindex;i--)
		stringsortchar[i] = stringsortchar[i-1];
	stringsortchar[newstringindex] = numIds;
  }
  if (numIds<maxstrings-1)
	numIds++;
  else
	error("Too many labels... I was too lazy but you work too hard.");
  PrDebug("  Aantal temp strings: %u\n",numIds - firsttempId);
  return (numIds-1);
}

void DeleteTempStrings(void)
{
  int i;
  if (firsttempstring != -1) {
	 PrDebug("Ik gooi %u strings weg.\n",numIds - firsttempId);
	 strtablelen = firsttempstring;
	 for (i=firsttempId;i<numIds;i++) {
		/* gooi referentielijsten weg */
		if (Idtable[i].type == LBL_UNDEF)
			FreeRefs(Idtable[i].value);    /* doe niks als == VAL_NOREF */
	 }
	 numIds = firsttempId;
       	 stringcharsorted = 0;
	 stringvalsorted = 0;
  }
  firsttempstring = -1;
  firsttempId = -1;
}

void FreezeTempStrings(void)
{
  PrDebug("Freeze!\n",0);
  firsttempstring = -1;
  firsttempId = -1;
}


Ident *GetIdent(int identnr)
{
  PrDebug("GetIdent: %u\n",identnr);
  return Idtable+identnr;
}

void InvalidateSortedValues(void)
{
	stringvalsorted=0;
}

void VulRefs(int ididx, unsigned int refidx0)
/* refidx = GetIdent(ididx).value, net voor herdefinieren (1e LblRef) */
{
  unsigned int val=GetIdent(ididx)->value;
  char bank=GetIdent(ididx)->bank;
  char type=GetIdent(ididx)->type;
  int refidx = refidx0;
  unsigned int nn;
  if (refidx0 == VAL_NOREF)
	 return;
  while (refidx != -1) {
	 if (type == LBL_VAL)
		nn = val;
	 else
		nn = Addr2Word(val,bank,CUR_PC);
	 PrDebug("VulRefs: addr = %04x\n",lblrefs[refidx].addr);
	 PrDebug("VulRefs: type = %02x\n",lblrefs[refidx].type);
	 switch (lblrefs[refidx].type) {
	 case LT_N:
		if (((int)nn>255) || ((int)nn<-255)) {
	warningf("Byte value out of range (at addr=%04x)",
                 lblrefs[refidx].addr);
		}
		if (type != LBL_VAL) {
	warningf("Byte expected; address found (at addr=%04x)",
                 lblrefs[refidx].addr);
		}
		pokeasmbyte(lblrefs[refidx].addr,lblrefs[refidx].bank,nn);
		break;
	 case LT_D:
		if (((int)nn>127) || ((int)nn<-128)) {
	warningf("IX/IY+d displacement out of range (at addr=%04x)",
                lblrefs[refidx].addr);
		}
		if (type != LBL_VAL) {
	warningf("Displacement expected; address found (at addr=%04x)",
                lblrefs[refidx].addr);
		}
		pokeasmbyte(lblrefs[refidx].addr,lblrefs[refidx].bank,nn);
		break;
	 case LT_E:
		if ((type != LBL_VAL)&&(lblrefs[refidx].bank==bank)) {
	  nn = (val & 0x3fff) -1 - (lblrefs[refidx].addr & 0x3fff);
		} else {
	nn = nn -1 -
				 Addr2Word(lblrefs[refidx].addr,lblrefs[refidx].bank,CUR_PC);
		}
		if (((int)nn<-128) || ((int)nn>127)) {
		  errorf("Relative jump out of range (at addr=%04x)",
                        lblrefs[refidx].addr);
		}
		pokeasmbyte(lblrefs[refidx].addr,lblrefs[refidx].bank,nn);
		break;
	 case LT_NN:
		pokew(lblrefs[refidx].addr,lblrefs[refidx].bank,nn);
		break;
	 case LT_NNLO:
		pokeasmbyte(lblrefs[refidx].addr,lblrefs[refidx].bank,nn);
		break;
	 case LT_NNHI:
		pokeasmbyte(lblrefs[refidx].addr,lblrefs[refidx].bank,nn>>8);
		break;
	 default:
		error("VulRefs: Internal error (LT-label undefined)");
		break;
	 }
	 refidx = lblrefs[refidx].next;
  }
  FreeRefs(refidx0);
}

void FreeRefs(unsigned int idx0)
{
  int idx=idx0,i;
  if (idx0 == VAL_NOREF) return;
  while (idx != -1) {
	 i = lblrefs[idx].next;
	 lblrefs[idx].next = firstfreelblref;
	 firstfreelblref = idx;
	 idx = i;
  }
}

void AddLabelRef(int idx, unsigned int addr, char lbltype)
{
  int i;
  unsigned int j;
  int tel=0;
  PrDebug("enter AddLabelRef\n",0);
  if (firstfreelblref==-1) { /* op! */
    firstfreelblref = numlblrefs;
    numlblrefs += 100;
    lblrefs = (LblRef*)realloc(lblrefs,numlblrefs * sizeof(LblRef));
    for (i=firstfreelblref;i<numlblrefs;i++)
      lblrefs[i].next = i+1;
	 lblrefs[numlblrefs-1].next=-1;
  }
  i = firstfreelblref;
  firstfreelblref = lblrefs[firstfreelblref].next;
  GetIdent(idx)->type = LBL_UNDEF;   /* remove LBL_PERMMASK bit, for undef warnings */
  j = GetIdent(idx)->value;
  if (j == VAL_NOREF) {
	 GetIdent(idx)->value = i;
  } else {
    tel = 1;
	 while (lblrefs[j].next != -1) {
      j = lblrefs[j].next;
		tel++;
    }
    lblrefs[j].next = i;
  }
  lblrefs[i].next = -1;
  lblrefs[i].addr = addr;
  lblrefs[i].bank = CurrentBank(addr);
  lblrefs[i].type = lbltype;

  /*
printf("Label registered: addr %04x type %u (teller %u)\n",addr,lbltype,tel);
*/
}


void ClearLabels()
{
  int i=0,j;
  while ((i<numIds)&&
			(Idtable[i].type & LBL_PERMMASK)&&
			(Idtable[i].type != (LBL_PERMMASK|LBL_UNDEF)))
	 i++;
  for (j=i;j<numIds;j++)
	 if ((Idtable[j].type & ~LBL_PERMMASK) == LBL_UNDEF)
		FreeRefs(Idtable[j].value);
  numIds = i;
  stringvalsorted = stringcharsorted = 0;
  firsttempstring = -1;
}

void FixLabels()
{
  int i;
  for (i=0;i<numIds;i++)
    if (Idtable[i].type != LBL_UNDEF)
		Idtable[i].type |= LBL_PERMMASK;
  firsttempstring = -1;
}



#ifndef __TURBOC__

void pokeasmbyte(unsigned int addr, char bank, unsigned int val)
{
  printf("Poke: %04x (%2x): %02x\n",addr,bank,val);
}

unsigned int Addr2Word(unsigned int addr, char bank, unsigned int cur_pc)
{
  return addr;
}

char CurrentBank(unsigned int addr)
{
  return 4;
}

#endif

void pokew(unsigned int addr,char bank,unsigned int val)
{
  pokeasmbyte(addr,bank,val);
  pokeasmbyte(addr+1,bank,val>>8);
}

void PokeAsmBytes(unsigned char b1,unsigned char b2,
						unsigned char b3,unsigned char b4,
						int len,unsigned int cur_pc)
{
  unsigned char b;
  int i;
  for (i=0;i<4;i++) {
	 if (len>i) {
		switch(i) {
			case 0:b=b1;break;
			case 1:b=b2;break;
			case 2:b=b3;break;
			case 3:b=b4;break;
		}
		  pokeasmbyte(cur_pc+i,CurrentBank(cur_pc+i),b);
	 }
  }
}

void WriteLabelsToFile(FILE *f, BOOL b)
{
	char written=0;
	unsigned int mask = LBL_PERMMASK;
	int i,j;
	char typ;
	if (!stringcharsorted)
		SortIdString();
	while (!written) {
		for (i=0;i<numIds;i++) {
			j = stringsortchar[i];
			typ = Idtable[j].type;
			if (!(typ & mask)) {
				typ &= ~LBL_PERMMASK;
				if (b) {
					// .SYM file
					if (typ == LBL_VAL) {
						fprintf(f,"$%04X\t%s\n",Idtable[j].value,GetString(Idtable[j].stringid));
						written = 1;
					};
					if (typ == LBL_ADDR) {
						fprintf(f,"$%04X\t%s\n",Addr2Word(Idtable[j].value & 0x3fff,Idtable[j].bank,CUR_PC),
													  GetString(Idtable[j].stringid));
						written = 1;
					};
				} else {
					// .ASM file
					if (typ == LBL_VAL) {
						fprintf(f,"%-10s EQU #%x\n",GetString(Idtable[j].stringid),Idtable[j].value);
						written = 1;
					};
					if (typ == LBL_ADDR) {
						fprintf(f,"%-10s EQUA #%x,#%x\n",GetString(Idtable[j].stringid),
																Addr2Word(Idtable[j].value & 0x3fff,Idtable[j].bank,CUR_PC),
																(unsigned int)Idtable[j].bank);
						written = 1;
					};
				}
			}
		}
		if ((written)&&(!mask)&&(!b)) fprintf(f,"\nfixlabels\n");
		if (!mask) written=1;
		if (!written) mask=0;
	}
	if (!b) fprintf(f,"\n;end of file\n");
}

void PrepareForAssembly()
/* Marks undefined labels as 'permanent', to distinguish them from new temporary
	labels, in order to warn about those at assembly.  If in assembly 'permanent'
	undefined labels are referred to, the 'permanent' flag is removed.
*/
{
  int i;
	for (i=0;i<numIds;i++) {
		if (Idtable[i].type == LBL_UNDEF)
			Idtable[i].type |= LBL_PERMMASK;
	}
}

void WarnForUndefinedLabels()
/* Removes 'permanent' mark from undefined labels; warns about undefined labels */
{
	char str[128],typ;
	int i;
	unsigned int w;
	for (i=0;i<numIds;i++) {
		typ = Idtable[i].type;
		if (typ == LBL_UNDEF) {
			w = Idtable[i].value;
			if (w!=VAL_NOREF) {
				sprintf(str,
						  "Undefined label (%s), first reference @ #%04x",
						  GetString(Idtable[i].stringid),
						  lblrefs[w].addr);
				warning(str);
			}
		}
		if ((typ & ~LBL_PERMMASK) == LBL_UNDEF)
			Idtable[i].type = LBL_UNDEF;
	}
}

