/* header file voor label.c */

#ifndef _labelh
#define _labelh

#ifndef _donotdefinewUserPtragain
extern int CUR_PC;
#endif

typedef struct {
  long      stringid;
  unsigned  int value; /* of id van een LblRef als type==LBL_UNDEF */
  char      type;      /* 0=leeg, 1=undef'd, 2=val, 3=addr, 4=permaddr */
  char      bank;
} Ident;

#define LBL_EMPTY    0
#define LBL_UNDEF    1
#define LBL_VAL      2
#define LBL_ADDR     3
#define LBL_PERMMASK 0x40
#define LBL_PERMADDR (LBL_ADDR | LBL_PERMMASK)
#define VAL_NOREF    0xffff

typedef struct {
  unsigned int addr;
  char bank;
  char type;
  int next;
} LblRef;

#define LT_N 0  /* bytes, -255 <= byte <= 255 */
#define LT_D 1  /* ix+D , -128 <=  d   <= 127 */
#define LT_E 2  /* jr e , not too far from location */
#define LT_NN 3 /* word */
#define LT_NNHI 4 /* word, hi part */
#define LT_NNLO 5 /* word, lo part */

void  IdInit(void);
Ident *GetIdent(int);      /* id nummer = string nummer*/
char  *GetString(long);
int   GetIdIndex(char *);
int   AddTempString(char *);   /* maakt ook nieuwe Ident index aan als nodig */
void  DeleteTempStrings(void); /* gooi laatste temp strings weg */
void  FreezeTempStrings(void); /* maak temp strings permanent */
int   FindFirstValue(unsigned int);  /* bits 0-13 */
int   FindNext(unsigned int);        /* -1 als niet meer */
void  AddLabelRef(int idx,unsigned int addr,char lbltype);
void  VulRefs(int ididx, unsigned int lblidx);
void  ClearLabels();
void  FreeRefs(unsigned int);
void  FixLabels();
void  PokeAsmBytes(unsigned char,unsigned char,unsigned char,unsigned char,int,unsigned int);
char  *FindAddrLabel(unsigned int addr);
void  WriteLabelsToFile(FILE *f, BOOL b);
void  PrepareForAssembly();
void  WarnForUndefinedLabels();

/* moet uiteindelijk in memory.c komen */

unsigned int  Addr2Word(unsigned int addr,char bank,unsigned int cur_pc);
char          CurrentBank(unsigned int addr);
void pokeasmbyte(unsigned int addr,char bank,unsigned int val);
void pokew(unsigned int addr,char bank,unsigned int val);

/* en dit? */

void warning(char *);
void error(char *);

#endif  /* ifdef _labelh */









