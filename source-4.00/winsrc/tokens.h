/* Token definitions, for use in WinZ80 debugger scanner */

#define help_tok 1
#define view_tok 2
#define disable_tok 3
#define display_tok 4
#define break_tok 5
#define quit_tok 6
#define pause_tok 7
#define run_tok 8
#define clear_tok 9
#define singlestep_tok 10
#define execute_tok 11
#define nmi_tok 12
#define reset_tok 13
#define trace_tok 14
#define type_tok 15
#define enable_tok 16
#define di_tok 17
#define ei_tok 18
#define im0_tok 19
#define im1_tok 20
#define im2_tok 21
#define ld_tok 22
#define comma_tok 23
#define asm_tok 24
#define label_tok 25
#define asmfile_tok 26
#define dump_tok 27
#define savelabels_tok 28

#define value_tok 100        /* either hex or dec */
#define dvalue_tok 101       /* dword value */
#define regpair_tok 102      /* AF, PC etc */
#define register_tok 103     /* eg IXh, but not PCl */

#define compare_tok 200      /* =, <>, <, >, but also <= and >= */
#define and_tok 201          /* &, AND */
#define br_open_tok 202      /* ( */
#define br_close_tok 203     /* ) */
#define word_tok 204         /* word */
#define byte_tok 205         /* byte */

#define error_tok 300        /* to signal an error */

#define eof_tok 400





