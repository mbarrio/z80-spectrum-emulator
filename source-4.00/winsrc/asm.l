/* flex input file for WinZ80 debugger scanner */

%{

#define ERROR_TOK -1

%}

%option yylineno
%option always-interactive

ws [ \t]*

%%

{ws}                 { /* ignore whitespace */ }
\n                   { return '\n'; }
";".*\n              { return '\n'; /* ignore comments */ }

defb                 { return DEFBT; }
defw                 { return DEFWT; }
equ                  { return EQUT;  }
equa                 { return EQUAT; }
org                  { return ORGT;  }
clearlabels          { return CLEARLABELST; }
fixlabels            { return FIXLABELST;   }
  
adc                  { return ADCT; }
add                  { return ADDT; }
and                  { return ANDT; }
bit                  { return BITT; }
ccf                  { return CCFT; }
cpl                  { return CPLT; }
call                 { return CALLT;}
cp                   { return CPT;  }
daa                  { return DAAT; }
dec                  { return DECT; }
inc                  { return INCT; }
di                   { return DIT;  }
ei                   { return EIT;  }
djnz                 { return DJNZT;}
exx                  { return EXXT; }
halt                 { return HALTT;}
im                   { return IMT;  }
in                   { return INT;  }
ex                   { return EXT;  }
jp                   { return JPT;  }
jr                   { return JRT;  }
or                   { return ORT;  }
out                  { return OUTT; }
ld                   { return LDT;  }
push                 { return PUSHT; }
pop                  { return POPT; }
res                  { return REST; }
set                  { return SETT; }
rl                   { return RLT;}
rr                   { return RRT;}
rlc                  { return RLCT;}
rrc                  { return RRCT;}
sla                  { return SLAT;}
sra                  { return SRAT;}
srl                  { return SRLT;}
sll                  { return SLLT;}
xor                  { return XORT;}
sbc                  { return SBCT;}
sub                  { return SUBT;}
rlca                 { return RLCAT;}
rla                  { return RLAT;}
rld                  { return RLDT;}
rra                  { return RRAT;}
rrca                 { return RRCAT;}
rrd                  { return RRDT;}
scf                  { return SCFT;}
ldd                  { return LDDT;}
lddr                 { return LDDRT;}
ldi                  { return LDIT;}
ldir                 { return LDIRT;}
neg                  { return NEGT;}
nop                  { return NOPT;}
ind                  { return INDT;}
indr                 { return INDRT;}
ini                  { return INIT;}
inir                 { return INIRT;}
cpd                  { return CPDT;}
cpdr                 { return CPDRT;}
cpi                  { return CPIT;}
cpir                 { return CPIRT;}
otir                 { return OTIRT;}
otdr                 { return OTDRT;}
outi                 { return OUTIT;}
outd                 { return OUTDT;}
rst                  { return RSTT;}
reti                 { return RETIT;}
retn                 { return RETNT;}
ret                  { return RETT;}

nz                   { return NZT; }
z                    { return ZT; }
nc                   { return NCT; }

po                   { return POT; }
pe                   { return PET; }
p                    { return PT; }
m                    { return MT; }

[,()+'/*%|&^~:]      { return yytext[0]; }
"["                  { return '['; }
"]"                  { return ']'; }
"-"                  { return '-'; }
"<<"                 { return TWEELINKS; }
">>"                 { return TWEERECHTS; }

a                    { return REGA; }
b                    { return REGB; }
c                    { return REGC; }
d                    { return REGD; }
e                    { return REGE; }
h                    { return REGH; }
l                    { return REGL; }
r                    { return REGR; }
i                    { return REGI; }
ixh                  { return REGIXH; }
ixl                  { return REGIXL; }
iyh                  { return REGIYH; }
iyl                  { return REGIYL; }

bc                   { return REGBC; }
de                   { return REGDE; }
hl                   { return REGHL; }
sp                   { return REGSP; }
ix                   { return REGIX; }
iy                   { return REGIY; }
af                   { return REGAF; }

"#"?[0-9a-f]+   { sscanf(yytext+(*yytext=='#'),"%lx",&yylval.arg.n);
                     return NUM; }

"$"[0-9]+       { sscanf(yytext+1,"%ld",&yylval.arg.n);
                     return NUM; }

"$"             { yylval.arg.n = CUR_PC;
                     return NUM; }

'.'             { yylval.arg.n = yytext[1];
                     return NUM; }

[A-Za-z_@][A-Za-z_@0-9]* { Ident *id;
                           int i = GetIdIndex(yytext);
                           if (i == -1) {
                             i = AddTempString(yytext);
                           }
                           yylval.id.index = i;
                           id = GetIdent(i);
                           if ((id->type & ~LBL_PERMMASK) == LBL_UNDEF) {
                             yylval.id.type = LBL_UNDEF;
                             return LABEL_UNDEF;
                           } else {
                             yylval.id.type = LBL_VAL;
                             if ((id->type & ~LBL_PERMMASK) == LBL_VAL)
                               yylval.id.val = id->value;
                             else
                               /* LBL_ADDR of LBL_PERMADDR */
                               yylval.id.val = 
                                 Addr2Word(id->value,id->bank,CUR_PC);
                           }
                           return LABEL;
                         }

.               { return ERROR_TOK; }

<<EOF>>         { return '\n'; }

%%

    





