
/* bison broncode, voor een Z80 assember voor WinZ80 */

%{

#pragma option -w-

  /* anders geeft borland c warnings bij "int -32768" en kapt ermee */

  typedef struct {
    unsigned char c1,c2,c3,c4;
    int len;
  } instructie;

  typedef struct {
    long n;
  } argument;

  typedef struct {
    unsigned int val;
    int index;
    char bank;
    char type;
  } ident;

  typedef struct {
    unsigned char d;
    unsigned char type;  /* 0xdd of 0xfd */
    int n;               /* zie cbarg, verder niet gebruikt */
  } ixiyplusd;

int asm_err; /* 0=syntax, 1=byte overflow, 2=jr overflow, 3=div zero, 
                4=undef label in expr 5=ix+d ovf 6=error tijdens def label */
int error_undeflabelstringid;

int asm_eof;

char usen, usenn, used, usee;  /* houdt gebruik ongedef labels bij */
int  lbln, lblnn, lbld, lble;  /* string id's van labels */

unsigned int tempcurpc;        /* zie 'regel' */ 

#define init_asm {usen=usenn=used=usee=asm_err=asm_eof=0; tempcurpc=CUR_PC;}

int zzwrap(void)
{
  asm_eof = 1;
  return 1;
}

void process(instructie p)
{
  unsigned char opc1 = p.c1;
  int j=((opc1==0xdd) || (opc1==0xfd) || (opc1==0xed)) ? 1 : 0;
  if (usenn && (p.len==2)) j=-1;
  if (usen && (p.len==1)) j=-1;
  if (usenn) {
    if (((CUR_PC+1+j)&0x3fff) == 0x3fff) {  /* op bank-rand */
      AddLabelRef(lblnn,CUR_PC+1+j,LT_NNLO);
      AddLabelRef(lblnn,CUR_PC+2+j,LT_NNHI);
    } else {
      AddLabelRef(lblnn,CUR_PC+1+j,LT_NN);
    }
  } else { 
    if (usee) 
      AddLabelRef(lble,CUR_PC+1,LT_E); 
    else { 
      if (used) { 
        j++; 
        AddLabelRef(lbld,CUR_PC+2,LT_D); 
      }
      if (usen) 
        AddLabelRef(lbln,CUR_PC+1+j,LT_N); 
    } 
  }
  /* en doe er wat mee!! */
  PokeAsmBytes(p.c1,p.c2,p.c3,p.c4,p.len,CUR_PC);
  if (p.len != -1) CUR_PC += p.len;
  init_asm;
}

#define errorf(form,oper) {char s[256]; sprintf(s,form,oper); error(s);}
#define warningf(form,oper) {char s[256]; sprintf(s,form,oper); warning(s);}

void redeflabel(ident id, unsigned int val, char type,char bank)
{
  Ident *Id = GetIdent(id.index);
  int mask=0x3fff;
  if ((type & ~LBL_PERMMASK) == LBL_VAL) mask=0xffff;
  if (Id->type == (LBL_PERMMASK | LBL_UNDEF)) {
    Id->type = LBL_UNDEF;
  }
  if ((Id->type & LBL_PERMMASK) &&
        (((val&mask)!=(Id->value&mask))||(type !=(Id->type& ~LBL_PERMMASK)))) {
    errorf("Trying to redefine permanent label (%s)",GetString(Id->stringid));
    asm_err=6;
  }
  if (Id->type & LBL_PERMMASK) {
    return;
  } else {
    if ((Id->type != LBL_UNDEF) && 
        (((val&mask)!=(Id->value&mask))||(type != Id->type))) {
      warningf("Redefining label (%s)",GetString(Id->stringid));
    }
    Id->value = val;
    if ((type == LBL_VAL)||(bank != -1))
      Id->bank = ((bank>=0)&&(bank<=11)) ? bank : 0;
    else
      Id->bank = CurrentBank(val);   /* LBL_ADDR */
    Id->type = type;
    InvalidateSortedValues();
  }
}

void deflabel(ident id, unsigned int val, char type,char bank)
{
  Ident *Id = GetIdent(id.index);
  unsigned int refidx = Id->value;
  Id->type = LBL_UNDEF;      
  redeflabel(id,val,type,bank);
  VulRefs(id.index,refidx); 
}

void defredeflabel(ident id, unsigned int val, char type, char bank)
{
  if (id.type == LBL_EMPTY)
    return;
  else if (id.type == LBL_UNDEF)
    deflabel(id,val,type,bank);
  else
    redeflabel(id,val,type,bank);
}


void WriteErrorString()
{
  switch (asm_err) {
  case 0:
    error("Syntax");
    break;
  case 1:
    error("Overflow in byte expression");
    break;
  case 2:
    error("Overflow in relative jump");
    break;
  case 3:
    error("Division by zero");
    break;
  case 4:
    errorf("Undefined label in expression (%s)",
            GetString(error_undeflabelstringid));
    break;
  case 5:
    error("Overflow in IX/IY+d expression");
    break;
  }
}

#define MYACCEPT {FreezeTempStrings(); YYACCEPT;}

#define MYABORT {WriteErrorString(); YYABORT;}

#define mk1(a) $$.c1=(a);$$.len=1;
#define mk2(a,b) $$.c1=(a);$$.c2=(b);$$.len=2;
#define mk3(a,b,c) $$.c1=(a);$$.c2=(b);$$.c3=(c);$$.len=3;
#define mk4(a,b,c,d) $$.c1=(a);$$.c2=(b);$$.c3=(c);$$.c4=(d);$$.len=4;
#define mw3(a,b) $$.c1=(a);$$.c2=(b);$$.c3=(b)>>8;$$.len=3;
#define mw4(a,b,c) $$.c1=(a);$$.c2=(b);$$.c3=(c);$$.c4=(c)>>8;$$.len=4;

#define copy2 $$.c1=$2.c1;$$.c2=$2.c2;$$.c3=$2.c3;$$.c4=$2.c4;$$.len=$2.len;

#define mkarg8(p,base,mult) { \
      if (p.type==0)  { /* 8 bit register als argument */ \
	mk1(base+mult*p.n) \
      } else if (p.n==0) { /* (ix+d) of (iy+d) */ \
        mk3(p.type,base+mult*6,p.d) \
      } else { /* ixh oid */ \
        mk2(p.type,base+mult*p.n) \
      } \
   }

%}

/* Declaraties enzo */

%union{
  instructie instr;
  argument arg;
  ixiyplusd ixiyarg;
  ident id;
}

%start initplusregel

%token  EQUT
%token  EQUAT
%token  DEFBT
%token  DEFWT
%token  ORGT
%token  CLEARLABELST
%token  FIXLABELST

%token  ADCT
%token  ADDT
%token  ANDT
%token  BITT
%token  CCFT
%token  DIT
%token  EIT
%token  DJNZT
%token  DECT
%token  INCT
%token  EXXT
%token  IMT
%token  INT
%token  CPT
%token  CALLT
%token  HALTT
%token  DAAT
%token  CPLT
%token  EXT
%token  LDT
%token  JPT
%token  JRT
%token  ORT
%token  OUTT
%token  PUSHT
%token  POPT
%token  REST
%token  SETT
%token  RETT
%token  RLT
%token  RLCT 
%token  RRT
%token  RRCT
%token  SLAT
%token  SRAT
%token  SRLT
%token  SLLT
%token  XORT
%token  SBCT
%token  SUBT
%token  RLCAT
%token  RLDT
%token  RRAT
%token  RRCAT
%token  RRDT
%token  SCFT
%token  RLAT
%token  LDDT
%token  LDDRT
%token  LDIT
%token  LDIRT
%token  NEGT
%token  NOPT
%token  INDT
%token  INDRT
%token  INIT
%token  INIRT
%token  CPDT
%token  CPDRT
%token  CPIT
%token  CPIRT
%token  OUTIT
%token  OUTDT
%token  OTIRT
%token  OTDRT
%token  RSTT
%token  RETNT
%token  RETIT

%token <arg> NUM

%token <id> LABEL
%token <id> LABEL_UNDEF

%token REGA
%token REGB
%token REGC
%token REGD
%token REGE
%token REGH
%token REGL
%token REGIXL
%token REGIYL
%token REGIXH
%token REGIYH

%token REGR
%token REGI

%token REGHL
%token REGBC
%token REGDE
%token REGSP
%token REGAF
%token REGIX
%token REGIY

%token NZT
%token ZT
%token NCT
/* %token CT */
%token POT
%token PET
%token PT
%token MT

%left '|'
%left '^'
%left '&'
%nonassoc TWEELINKS TWEERECHTS
%left '+' '-'            
%left '*' '/' '%'
%left COMPLEMENT UNAIREMIN

%type <instr> statement
%type <instr> initplusregel
%type <instr> regel
%type <instr> dummyopc
%type <id> labelpart
%type <id> labelwithcolon
%type <arg> reg8
%type <arg> reg7
%type <arg> reg6
%type <arg> reg5
%type <arg> reg16a  
%type <arg> reg16b
%type <arg> regixy
%type <arg> flag8
%type <arg> flag4
%type <ixiyarg> ixiy
%type <arg> cbopc
%type <ixiyarg> cbarg
%type <arg> expr1
%type <arg> exprmin
%type <arg> expr2
%type <arg> brexpr
%type <arg> expr8
%type <arg> expr16
%type <arg> expr16list
%type <arg> expre
%type <arg> exprd
%type <arg> exprdmin
%type <arg> expr3
%type <ixiyarg> reg8ixy
%type <ixiyarg> arg8

%type <instr> add
%type <instr> adc
%type <instr> sub
%type <instr> sbc
%type <instr> and
%type <instr> call
%type <instr> in
%type <instr> ina
%type <instr> im
%type <instr> djnz
%type <instr> inc
%type <instr> dec
%type <instr> cp
%type <instr> jp
%type <instr> jr
%type <instr> ex
%type <instr> ld
%type <instr> or
%type <instr> out
%type <instr> push
%type <instr> pop
%type <instr> xor
%type <instr> ret
%type <instr> rst
%type <instr> cbblock

%expect 9     /* ld hl,nn ld hl,(nn) ld a,n ld a,(n), (expr)/xpr */

%%

/* lexer haalt zelf commentaar eruit */

initplusregel:
  {init_asm} regel
;

regel:
  labelpart statement '\n'
    {process($2);defredeflabel($1,tempcurpc,LBL_ADDR,-1);MYACCEPT;}
| labelwithcolon '\n' 
    {defredeflabel($1,CUR_PC,LBL_ADDR,-1);MYACCEPT;}
| LABEL colon EQUT expr1 '\n'  
    {redeflabel($1,$4.n,LBL_VAL,-1);MYACCEPT;}
| LABEL_UNDEF colon EQUT expr1 '\n'  
    {deflabel($1,$4.n,LBL_VAL,-1);MYACCEPT;}
| LABEL colon EQUAT expr1 '\n'  
    {redeflabel($1,$4.n,LBL_ADDR,-1);MYACCEPT;}
| LABEL_UNDEF colon EQUAT expr1 '\n'  
    {deflabel($1,$4.n,LBL_ADDR,-1);MYACCEPT;}
| LABEL colon EQUAT expr1 ',' expr1 '\n'  
   {redeflabel($1,$4.n,LBL_ADDR,$6.n);MYACCEPT;}
| LABEL_UNDEF colon EQUAT expr1 ',' expr1 '\n'
   {deflabel($1,$4.n,LBL_ADDR,$6.n);MYACCEPT;}
| labelpart DEFBT expr8list '\n'
   {defredeflabel($1,tempcurpc,LBL_ADDR,-1); MYACCEPT; }
| labelpart DEFWT expr16list '\n'
   {defredeflabel($1,tempcurpc,LBL_ADDR,-1); MYACCEPT; }
| ORGT expr1 dummyopc '\n'
   {CUR_PC = $2.n; $3.len=-1; process($3); MYACCEPT; }
| CLEARLABELST '\n' 
   {ClearLabels(); MYACCEPT; }
| FIXLABELST '\n'
   {FixLabels(); MYACCEPT; }
;

labelwithcolon:
  /* empty */ {$$.type = LBL_EMPTY;}
| LABEL ':'       {$$.val = $1.val; $$.index=$1.index; $$.type=$1.type;}
| LABEL_UNDEF ':' {$$.val = $1.val; $$.index=$1.index; $$.type=$1.type;}
;

labelpart:
  /* empty */       {$$.type = LBL_EMPTY;}
| LABEL colon       {$$.val = $1.val; $$.index=$1.index; $$.type=$1.type;}
| LABEL_UNDEF colon {$$.val = $1.val; $$.index=$1.index; $$.type=$1.type;}
;

colon: 
  /* empty */
| ':'
;

dummyopc: { /* leeg, om even resultaatje vast te houden */ } ;

expr8list:
  dummyopc expr8 
    {$1.c1=$2.n; $1.len=1; process($1); }
| dummyopc expr8 ','
    {$1.c1=$2.n; $1.len=1; process($1); } expr8list
;

expr16list:
  dummyopc expr16 
    {$1.c1=$2.n; $1.c2=$2.n>>8; $1.len=2; process($1);}
| dummyopc expr16 ',' 
    {$1.c1=$2.n; $1.c2=$2.n>>8; $1.len=2; process($1);} expr16list 
;

reg5:   REGB {$$.n=0;}
      | REGC {$$.n=1;}
      | REGD {$$.n=2;}
      | REGE {$$.n=3;}
      | REGA {$$.n=7;}
;

reg6:   REGB {$$.n=0;}
      | REGC {$$.n=1;}
      | REGD {$$.n=2;}
      | REGE {$$.n=3;}
      | REGH {$$.n=4;}
      | REGL {$$.n=5;}
;

reg7:   REGA {$$.n=7;}
      | reg6 {$$.n=$1.n;}
;

reg8:
        '(' REGHL ')' {$$.n=6;}
      | reg7 {$$.n=$1.n;}       
;

reg8ixy:
      REGIXH {$$.n=0x4;$$.type=0xdd;}
|     REGIXL {$$.n=0x5;$$.type=0xdd;}
|     REGIYH {$$.n=0x4;$$.type=0xfd;}
|     REGIYL {$$.n=0x5;$$.type=0xfd;}
;

reg16a:  
  REGBC {$$.n=0;}
| REGDE {$$.n=0x10;}
| REGHL {$$.n=0x20;}
| REGSP {$$.n=0x30;}
;

reg16b:
  REGBC {$$.n=0;}
| REGDE {$$.n=0x10;}
| REGHL {$$.n=0x20;}
| REGAF {$$.n=0x30;}


regixy:
  REGIX {$$.n=0xdd;}
| REGIY {$$.n=0xfd;}
;

flag8:
flag4 {$$.n=$1.n}
| POT {$$.n=32;}
| PET {$$.n=40;}
| PT {$$.n=48;}
| MT {$$.n=56;}
;

flag4:
  NZT {$$.n=0;}
| ZT {$$.n=8;}
| NCT {$$.n=16;}
| REGC {$$.n=24;}
;

ixiy: 
  '(' REGIX '+' exprd ')' {$$.d = $4.n; $$.type=0xdd;}
| '(' REGIX exprdmin ')' {$$.d = $3.n; $$.type=0xdd;} 
| '(' REGIX ')' {$$.d = 0; $$.type=0xdd;}
| '(' REGIY '+' exprd ')' {$$.d = $4.n; $$.type=0xfd;}
| '(' REGIY exprdmin ')' {$$.d = $3.n; $$.type=0xfd;} 
| '(' REGIY ')' {$$.d = 0; $$.type=0xfd;}
;

arg8:
  ixiy {$$.d=$1.d;$$.n=0;$$.type=$1.type;}
| reg8ixy {$$.n=$1.n;$$.type=$1.type;}
| reg8 {$$.n=$1.n; $$.type=0;}
;

expr8:
  LABEL_UNDEF {$$.n = 0x20;usen=1;lbln=$1.index;}
| expr1 {$$.n = $1.n; 
         if (($1.n>255)||($1.n<-255)) 
	   {asm_err=1;MYABORT;}}
;

expr16:
  LABEL_UNDEF {$$.n = 0x8405;usenn=1;lblnn=$1.index;}
| expr1 {$$.n = $1.n;}
;


expre:
  LABEL_UNDEF {$$.n=CUR_PC+0x30;usee=1;lble=$1.index;}  /* 0x30 = 0x2e + 2 */
| expr1 {$$.n = $1.n; 
        if (((int)($1.n-CUR_PC)>129)||((int)($1.n-CUR_PC)<-126)) 
	  {asm_err=2;MYABORT;}}
;

exprd:
  LABEL_UNDEF {$$.n=0x05;used=1;lbld=$1.index;}
| expr1 {$$.n = $1.n;
        if (($1.n>127)||($1.n<-128)) {asm_err=5;MYABORT;}}
;

exprdmin:
  '-' exprmin {$$.n = $2.n;        
               if (($2.n>127)||($2.n<-128)) {asm_err=5;MYABORT;}}
;

expr3:
  expr1 {$$.n = $1.n;
        if (($1.n<0)||($1.n>7)) /* syntax */ MYABORT;}
;

brexpr:
  '(' expr2 ')' {$$.n = $2.n;}
;

expr2:
  expr1 {$$.n = $1.n;}
| brexpr {$$.n = $1.n;}
;

expr1:
  NUM {$$.n = $1.n;}
| LABEL {$$.n = $1.val;}
| LABEL_UNDEF {asm_err=4;
               error_undeflabelstringid=GetIdent($1.index)->stringid;
               MYABORT;}
| ADCT {$$.n = 0xadc;}
| ADDT {$$.n = 0xadd;}
| CCFT {$$.n = 0xccf;}
| DAAT {$$.n = 0xdaa;}
| expr2 '+' expr2 {$$.n = $1.n + $3.n;}
| expr2 '-' expr2 {$$.n = $1.n - $3.n;}
| expr2 '*' expr2 {$$.n = $1.n * $3.n;}
| expr2 '/' expr2 {if (!$3.n) {asm_err=3;MYABORT;} else { $$.n = $1.n / $3.n; }}
| expr2 TWEELINKS expr2 {$$.n = $1.n << (int)$3.n;}
| expr2 TWEERECHTS expr2 {$$.n = $1.n >> (int)$3.n;}
| expr2 '&' expr2 {$$.n = $1.n & $3.n;}
| expr2 '|' expr2 {$$.n = $1.n | $3.n;}
| expr2 '^' expr2 {$$.n = $1.n ^ $3.n;}
| expr2 '%' expr2 {if (!$3.n) {asm_err=3;MYABORT;} else { $$.n = $1.n % $3.n; }}
| '-' expr2 %prec UNAIREMIN {$$.n = -$2.n;}
| '~' expr2 %prec COMPLEMENT {$$.n = ~$2.n;}
;

exprmin:               /* voor IX- en IY- expressies */
  NUM {$$.n = -$1.n;}
| LABEL {$$.n = -$1.val;}
| LABEL_UNDEF {asm_err=4;MYABORT;}
| ADCT {$$.n = 0xadc;}
| ADDT {$$.n = 0xadd;}
| CCFT {$$.n = 0xccf;}
| DAAT {$$.n = 0xdaa;}
| '(' expr2 ')' {$$.n = -$2.n;}
| exprmin '+' expr2 {$$.n = $1.n + $3.n;}
| exprmin '-' expr2 {$$.n = $1.n - $3.n;}
| exprmin '*' expr2 {$$.n = $1.n * $3.n;}
| exprmin '/' expr2 {if (!$3.n) {asm_err=3;MYABORT;} else { $$.n = $1.n / $3.n; }}
| exprmin TWEELINKS expr2 {$$.n = $1.n << (int)$3.n;}
| exprmin TWEERECHTS expr2 {$$.n = $1.n >> (int)$3.n;}
| exprmin '&' expr2 {$$.n = $1.n & $3.n;}
| exprmin '|' expr2 {$$.n = $1.n | $3.n;}
| exprmin '^' expr2 {$$.n = $1.n ^ $3.n;}
| exprmin '%' expr2 {if (!$3.n) {asm_err=3;MYABORT;} else { $$.n = $1.n % $3.n; }}
| '-' expr2 %prec UNAIREMIN {$$.n = $2.n;}
| '~' expr2 %prec COMPLEMENT {$$.n = -(~$2.n);}
;

statement: 
      adc
    | add
    | and
    | cp
    | dec
    | call
    | djnz
    | im
    | in
    | inc
    | DIT {mk1(0xf3)}
    | EIT {mk1(0xfb)}
    | EXXT {mk1(0xd9)}
    | HALTT {mk1(0x76)}
    | DAAT {mk1(0x27)}
    | CPLT {mk1(0x2f)}
    | CCFT {mk1(0x3f)}
    | NOPT {mk1(0)}    
    | RLCAT {mk1(7)}
    | RRCAT {mk1(15)}
    | RLAT {mk1(0x17)}
    | RRAT {mk1(0x1f)}
    | SCFT {mk1(0x37)}
    | NEGT {mk2(0xed,0x44)}
    | RETNT {mk2(0xed,0x45)}
    | RETIT {mk2(0xed,0x4d)}
    | RRDT {mk2(0xed,0x67)}
    | RLDT {mk2(0xed,0x6f)}
    | LDIT {mk2(0xed,0xa0)}
    | CPIT {mk2(0xed,0xa1)}
    | INIT {mk2(0xed,0xa2)}
    | OUTIT {mk2(0xed,0xa3)}
    | LDDT {mk2(0xed,0xa8)}
    | CPDT {mk2(0xed,0xa9)}
    | INDT {mk2(0xed,0xaa)}
    | OUTDT {mk2(0xed,0xab)}
    | LDIRT {mk2(0xed,0xb0)}
    | CPIRT {mk2(0xed,0xb1)}
    | INIRT {mk2(0xed,0xb2)}
    | OTIRT {mk2(0xed,0xb3)}
    | LDDRT {mk2(0xed,0xb8)}
    | CPDRT {mk2(0xed,0xb9)}
    | INDRT {mk2(0xed,0xba)}
    | OTDRT {mk2(0xed,0xbb)}
    | cbblock
    | ex
    | jp
    | jr
    | ld
    | or
    | xor
    | out
    | push
    | pop
    | ret
    | rst
    | sub
    | sbc
;

adc:
  ADCT REGA ',' arg8 mkarg8($4,0x88,1)
| ADCT REGA ',' expr8 {mk2(0xce,$4.n)}
| ADCT REGHL ',' reg16a {mk2(0xed,0x4a+$4.n)}
;

add:
  ADDT REGA ',' arg8 mkarg8($4,0x80,1)
| ADDT REGA ',' expr8 {mk2(0xc6,$4.n)}
| ADDT REGHL ',' reg16a {mk1(0x09+$4.n)}
| ADDT REGIX ',' reg16a {mk2(0xdd,0x09+$4.n)}
| ADDT REGIY ',' reg16a {mk2(0xfd,0x09+$4.n)}
;

sbc:
  SBCT REGA ',' arg8 mkarg8($4,0x98,1)
| SBCT REGA ',' expr8 {mk2(0xde,$4.n)}
| SBCT REGHL ',' reg16a {mk2(0xed,0x42+$4.n)}
;

sub:
  SUBT arg8 mkarg8($2,0x90,1)
| SUBT expr8 {mk2(0xd6,$2.n)}
;

and:
  ANDT arg8 mkarg8($2,0xa0,1)
| ANDT expr8  {mk2(0xe6,$2.n)}
;

or:
  ORT arg8 mkarg8($2,0xb0,1)
| ORT expr8  {mk2(0xf6,$2.n)}
;

xor:
  XORT arg8 mkarg8($2,0xa8,1)
| XORT expr8  {mk2(0xee,$2.n)}
;

cp:
  CPT arg8 mkarg8($2,0xb8,1)
| CPT expr8  {mk2(0xfe,$2.n)}
;

inc:
  INCT arg8 mkarg8($2,0x04,8)
| INCT reg16a {mk1(0x03+$2.n)}
| INCT regixy {mk2($2.n,0x23)}
;

dec:
  DECT arg8 mkarg8($2,0x05,8)
| DECT reg16a {mk1(0x0b+$2.n)}
| DECT regixy {mk2($2.n,0x2b)}
;

call:
  CALLT expr16 {mw3(0xcd,$2.n)}
| CALLT flag8 ',' expr16 {mw3(0xc4+$2.n,$4.n)}
;

push:
  PUSHT reg16b {mk1(0xc5+$2.n)}
| PUSHT regixy {mk2($2.n,0xe5)}
;

pop:
  POPT reg16b {mk1(0xc1+$2.n)}
| POPT regixy {mk2($2.n,0xe1)}
;

in:
  INT reg6 ',' '(' REGC ')' {mk2(0xed,0x40+8*$2.n)}
| INT '(' REGC ')' {mk2(0xed,0x70)}
| INT ina {copy2}
;

out:
  OUTT '(' REGC ')' ',' reg7 {mk2(0xed,0x41+8*$6.n)}
| OUTT '(' REGC ')' {mk2(0xed,0x71)}
| OUTT '(' expr8 ')' ',' REGA {mk2(0xd3,$3.n)}
;

ina:
  REGA ',' '(' expr8 ')' {mk2(0xdb,$4.n)}
| REGA ',' '(' REGC ')' {mk2(0xed,0x78)}
;

djnz:
  DJNZT expre {mk2(0x10,$2.n-CUR_PC-2)}
;

im:
IMT NUM {
  unsigned char b=0;
  if ($2.n==0) b=0x46;
  if ($2.n==1) b=0x56;
  if ($2.n==2) b=0x5e;
  if (!b) MYABORT /* syntax error */;
  mk2(0xed,b)}
;

cbblock:
cbopc cbarg {
  if ($2.type==0) {
    /* Geen ix/iy type */
    mk2(0xcb,$1.n + $2.n)
  } else {
    /* Wel ix/iy */
    mk4($2.type,0xcb,$2.d,$1.n + $2.n)
  }
}
;

cbopc:
  BITT expr3 ',' {$$.n = 8*$2.n+0x40;}
| REST expr3 ',' {$$.n = 8*$2.n+0x80;}
| SETT expr3 ',' {$$.n = 8*$2.n+0xc0;}
| RLCT {$$.n = 0;}
| RRCT {$$.n = 8;}
| RLT {$$.n = 16;}
| RRT {$$.n = 24;}
| SLAT {$$.n = 32;}
| SRAT {$$.n = 40;}
| SLLT {$$.n = 48;}
| SRLT {$$.n = 56;}
;

cbarg:
  reg8 {$$.n = $1.n; $$.type = 0;}
| reg7 ',' ixiy {$$.n=$1.n; $$.type = $3.type; $$.d = $3.d;}
| ixiy {$$.n = 6; $$.type = $1.type; $$.d = $1.d;}
;

ex:
     EXT '(' REGSP ')' ',' REGHL {mk1(0xe3);}
|    EXT '(' REGSP ')' ',' regixy {mk2($6.n,0xe3);}
|    EXT REGAF ',' REGAF '\'' {mk1(0x08)}
|    EXT REGDE ',' REGHL {mk1(0xeb)}
;

jp:
  JPT expr16 {mw3(0xc3,$2.n)}
| JPT '(' REGHL ')' {mk1(0xe9)}
| JPT '(' regixy ')' {mk2($3.n,0xe9)}
| JPT flag8 ',' expr16 {mw3(0xc2+$2.n,$4.n)}
;

jr:
  JRT expre {mk2(0x18,$2.n-CUR_PC-2)}
| JRT flag4 ',' expre {mk2(0x20+$2.n,$4.n-CUR_PC-2)}
;

ret:
  RETT {mk1(0xc9)}
| RETT flag8 {mk1(0xc0+$2.n)}
;

rst:
  RSTT NUM {mk1(0xc7+($2.n & 0x38))}
;

ld:
  LDT '(' REGBC ')' ',' REGA {mk1(0x02)}
| LDT '(' REGDE ')' ',' REGA {mk1(0x12)}
| LDT reg6 ',' reg7 {mk1(0x40 + $4.n + 8*$2.n)}
| LDT reg6 ',' expr8 {mk2(0x06+8*$2.n,$4.n)}
| LDT reg6 ',' '(' REGHL ')' {mk1(0x46 + 8*$2.n)}
| LDT reg6 ',' ixiy {mk3($4.type,0x46 + 8*$2.n,$4.d)}
| LDT reg8ixy ',' reg5 {mk2($2.type,0x40 + $4.n + 8*$2.n)}
| LDT reg8ixy ',' reg8ixy {if ($2.type!=$4.type) {MYABORT;} 
                           mk2($2.type,0x40 + $4.n + 8*$2.n)}
| LDT reg8ixy ',' expr8 {mk3($2.type,0x06+8*$2.n,$4.n)}
| LDT REGA ',' reg7 {mk1(0x40 + $4.n + 8*7 )}
| LDT REGA ',' reg8ixy {mk2($4.type,0x40 + $4.n + 8*7 )}
| LDT REGA ',' expr8 {mk2(0x06+8*7 ,$4.n)}
| LDT REGA ',' '(' REGHL ')' {mk1(0x46 + 8*7 )}
| LDT REGA ',' ixiy {mk3($4.type,0x46 + 8*7,$4.d)}
| LDT REGA ',' '(' REGBC ')' {mk1(0x0a)}
| LDT REGA ',' '(' REGDE ')' {mk1(0x1a)}
| LDT REGA ',' '(' expr16 ')' {mw3(0x3a,$5.n)}
| LDT '(' REGHL ')' ',' expr8 {mk2(0x36,$6.n)}
| LDT '(' REGHL ')' ',' reg7 {mk1(0x70+$6.n)}
| LDT ixiy ',' reg7 {mk3($2.type,0x70+$4.n,$2.d)}
| LDT ixiy ',' expr8 {mk4($2.type,0x36,$2.d,$4.n)}
| LDT '(' expr16 ')' ',' REGA {mw3(0x32,$3.n)}
| LDT '(' expr16 ')' ',' REGBC {mw4(0xed,0x43,$3.n)}
| LDT '(' expr16 ')' ',' REGDE {mw4(0xed,0x53,$3.n)}
| LDT '(' expr16 ')' ',' REGSP {mw4(0xed,0x73,$3.n)}
| LDT '(' expr16 ')' ',' REGHL {mw3(0x22,$3.n)}
| LDT '(' expr16 ')' ',' regixy {mw4($6.n,0x22,$3.n)}
| LDT REGHL ',' expr16 {mw3(0x21,$4.n)} 
| LDT regixy ',' expr16 {mw4($2.n,0x21,$4.n)}
| LDT reg16a ',' expr16 {mw3(0x01+$2.n,$4.n)}
| LDT REGHL ',' '(' expr16 ')' {mw3(0x2a,$5.n)}
| LDT regixy ',' '(' expr16 ')' {mw4($2.n,0x2a,$5.n)}
| LDT reg16a ',' '(' expr16 ')' {mw4(0xed,0x4b+$2.n,$5.n)} 
| LDT REGSP ',' REGHL {mk1(0xf9)}
| LDT REGSP ',' regixy {mk2($4.n,0xf9)}
| LDT REGSP ',' expr16 {mw3(0x31,$4.n)} 
| LDT REGSP ',' '(' expr16 ')' {mw4(0xed,0x7b,$5.n)}
| LDT REGI ',' REGA {mk2(0xed,0x47)}
| LDT REGR ',' REGA {mk2(0xed,0x4f)}
| LDT REGA ',' REGI {mk2(0xed,0x57)}
| LDT REGA ',' REGR {mk2(0xed,0x5f)}
;


















