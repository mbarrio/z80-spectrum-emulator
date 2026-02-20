/* flex input file for WinZ80 debugger scanner */

%{

#include "tokens.h"
#include <string.h>

#define YY_USER_ACTION scan_index+=yyleng;

#define tokf(f,a) sprintf(scan_str+strlen(scan_str),f,a)
#define tok(s) tokf("%s",s)

unsigned int scan_word;
unsigned long scan_dword;
int scan_index;       /* to see where error occurred */
char scan_type;       /* 0=none, 1=byte, 2=word */
char scan_reg;        /* id number of register scanned */
char scan_comp;       /* id number of comp operator scanned */
char scan_str[128];   /* expanded token and rest */
int  label_id;        /* holds id of label scanned, or -1 */

%}

%option noyywrap

ws [ \t]*

%%


^{ws}h(elp)?         { tok("help") ; return help_tok; }
^{ws}v(iew)?         { tok("view") ; return view_tok; }
^{ws}off             { tok("off") ; return disable_tok; }
^{ws}d(isplay)?      { tok("display") ; return display_tok; }
^{ws}b(reak)?        { tok("break") ; return break_tok; }
^{ws}q(uit)?         { tok("quit") ; return quit_tok; }
^{ws}p(ause)?        { tok("pause") ; return pause_tok; }
^{ws}r(un)?          { tok("run") ; return run_tok; }
^{ws}c(lear)?        { tok("clear") ; return clear_tok; }
^{ws}s(inglestep)?   { tok("singlestep") ; return singlestep_tok; }
^{ws}e(xecute)?      { tok("execute") ; return execute_tok; }
^{ws}n(mi)?          { tok("nmi") ; return nmi_tok; }
^{ws}re(set)?        { tok("reset") ; return reset_tok; }   /* Beetje vies...
^{ws}tr(ace)?        { tok("trace") ; return trace_tok; }   */
^{ws}t(ype)?         { tok("type") ; return type_tok; }
^{ws}on              { tok("on") ; return enable_tok; }
^{ws}di              { tok("DI"); return di_tok; }
^{ws}ei              { tok("EI"); return ei_tok; }
^{ws}im{ws}0         { tok("IM 0"); return im0_tok; }
^{ws}im{ws}1         { tok("IM 1"); return im1_tok; }
^{ws}im{ws}2         { tok("IM 2"); return im2_tok; }
^{ws}ld              { tok("ld"); return ld_tok; }
^{ws}a(sm)?{ws}      { tok("asm"); return asm_tok; }
^{ws}l(abel)?        { tok("label"); return label_tok; }
^{ws}dump            { tok("dump"); return dump_tok; }
^{ws}asmfile         { tok("asmfile"); return asmfile_tok; }
^{ws}savelabels      { tok("savelabels"); return savelabels_tok; }
^{ws}sl              { tok("savelabels"); return savelabels_tok; }

{ws}            { tok(" "); /* ignore whitespace */ }

,               { tok(","); return comma_tok; }

af              { scan_reg = 0; tok("af"); return regpair_tok; }
bc              { scan_reg = 2; tok("bc"); return regpair_tok; }
de              { scan_reg = 4; tok("de"); return regpair_tok; }
hl              { scan_reg = 6; tok("hl"); return regpair_tok; }
af'             { scan_reg = 8; tok("af'"); return regpair_tok; }
bc'             { scan_reg = 10;tok("bc'"); return regpair_tok; }
de'             { scan_reg = 12;tok("de'"); return regpair_tok; }
hl'             { scan_reg = 14;tok("hl'"); return regpair_tok; }
ix              { scan_reg = 16;tok("ix"); return regpair_tok; }
iy              { scan_reg = 18;tok("iy"); return regpair_tok; }
pc              { scan_reg = 20;tok("pc"); return regpair_tok; }
sp              { scan_reg = 22;tok("sp"); return regpair_tok; }
f               { scan_reg = 0; tok("f"); return register_tok; }
a               { scan_reg = 1; tok("a"); return register_tok; }
c               { scan_reg = 2; tok("c"); return register_tok; }
b               { scan_reg = 3; tok("b"); return register_tok; }
e               { scan_reg = 4; tok("e"); return register_tok; }
d               { scan_reg = 5; tok("d"); return register_tok; }
l               { scan_reg = 6; tok("l"); return register_tok; }
h               { scan_reg = 7; tok("h"); return register_tok; }
f'              { scan_reg = 8; tok("f'"); return register_tok; }
a'              { scan_reg = 9; tok("a'"); return register_tok; }
c'              { scan_reg = 10;tok("c'"); return register_tok; }
b'              { scan_reg = 11;tok("b'"); return register_tok; }
e'              { scan_reg = 12;tok("e'"); return register_tok; }
d'              { scan_reg = 13;tok("d'"); return register_tok; }
l'              { scan_reg = 14;tok("l'"); return register_tok; }
h'              { scan_reg = 15;tok("h'"); return register_tok; }
ixl             { scan_reg = 16;tok("ixl"); return register_tok; }
ixh             { scan_reg = 17;tok("ixh"); return register_tok; }
iyl             { scan_reg = 18;tok("iyl"); return register_tok; }
iyh             { scan_reg = 19;tok("iyh"); return register_tok; }
r               { scan_reg = 24;tok("r"); return register_tok; }
i               { scan_reg = 25;tok("i"); return register_tok; }

"#"?[0-9a-f]+   { sscanf(yytext+(*yytext=='#'),"%lx",&scan_dword);
                   scan_word = scan_dword;
                   tokf("#%lx",scan_dword);
                   if (scan_dword != scan_word)
                     return dvalue_tok;
                   else
                     return value_tok; }
"$"[0-9]+       { sscanf(yytext+1,"%ld",&scan_dword);
                   scan_word = scan_dword;
                   tokf("$%ld",scan_dword);
                   if (scan_dword != scan_word)
                     return dvalue_tok;
                   else
                     return value_tok; }
"$"             { if (scan_type==1)
                     return error_tok;
                  scan_type = 2;
                  scan_word = scan_dword = wUserPtr ;
                  tokf("#%x",scan_word); 
                  return value_tok; }
'.+'            { int i;
                  if (scan_type==2)
                     return error_tok;
                  scan_type = 1;
                  scan_dword = 0;
                  sscanf(yytext+1,"%c",&scan_dword) ;
                  scan_word = scan_dword;
                  i = strlen(scan_str);
                  if (i && (scan_str[i-1]=='\'')) {
                     scan_str[i-1]=0;
                  } else {
                     scan_str[i]='\''; 
                     scan_str[i+1]=0;
                  }
                  tokf("%c'",scan_word);
                  if (yyleng > 3) {
                      for (i=yyleng; i>2; )
                          unput(yytext[--i]);
                      unput('\'');
                  }
                  return value_tok; }

=               { scan_comp = 0 ; tok("="); return compare_tok; }
"<>"            |
!=              { scan_comp = 1 ; tok("!="); return compare_tok; }
"<="            { scan_comp = 2 ; tok("<="); return compare_tok; }
">="            { scan_comp = 3 ; tok(">="); return compare_tok; }
"<"             { scan_comp = 4 ; tok("<"); return compare_tok; }
">"             { scan_comp = 5 ; tok(">"); return compare_tok; }

&               |
and             { tok("&"); return and_tok; }

"("             { tok("("); return br_open_tok; }
")"             { tok(")"); return br_close_tok; }
word            { if (scan_type==1)
                    return error_tok;
                  else {
                    scan_type=2;
                    tok("word");
                  }
                }
byte            { if (scan_type==2)
                    return error_tok;
                  else {
                    scan_type=1;
                    tok("byte");
                  }
                }

[A-Za-z_@][A-Za-z_@0-9]* { Ident *id;
                           int i = GetIdIndex(yytext);
			   label_id = i;
                           if (i == -1) {
                             return error_tok;   /* label not found */
                           }
                           id = GetIdent(i);
                           tok(yytext);
                           if ((id->type & ~LBL_PERMMASK) == LBL_UNDEF) {
			     return error_tok;   /* but possibly refs */
                           }
                           if ((id->type & ~LBL_PERMMASK) == LBL_VAL)
			     scan_word = scan_dword = id->value;
                           else
                             scan_word = scan_dword = 
                               Addr2Word(id->value,id->bank,CUR_PC);
                           return value_tok;
                         }
       
.|\n            { return error_tok; }

<<EOF>>         { return eof_tok; }

%%

    





