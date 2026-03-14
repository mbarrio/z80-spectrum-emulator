// Tsja.  Je moet soms wat.  Alles bijelkaarin compileren gaat fout
// (er zijn conflicterende locale vars), maar bij los compileren
// moet je wel gaan vertellen hoe de functies eruit zien.  Bij deze dus.

extern int asm_err;
extern int asm_eof;
extern int zzlineno;

int zzparse(void);

#ifndef yyconst
#define yyconst const
#endif

#ifndef YY_PROTO
#define YY_PROTO proto
#endif

#ifndef yy_size_t
#define yy_size_t size_t
#endif

YY_BUFFER_STATE zz_scan_buffer YY_PROTO(( char *base, yy_size_t size ));
YY_BUFFER_STATE zz_scan_string YY_PROTO(( yyconst char *str ));
YY_BUFFER_STATE zz_scan_bytes YY_PROTO(( yyconst char *bytes, int len ));
void zz_delete_buffer YY_PROTO(( YY_BUFFER_STATE b ));
void zz_switch_to_buffer YY_PROTO(( YY_BUFFER_STATE b ));
YY_BUFFER_STATE zz_create_buffer YY_PROTO(( FILE *f, yy_size_t size ));


