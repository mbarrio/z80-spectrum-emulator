#include <stdio.h>
#include <windows.h>
#include "spectrum.h"

#include "label.h"

#define __STDC__ 1    
// damit 'const' gebruikt wordt - anders loopt automatic data seg over

#include "asmbison.c"

#undef __TURBOC__
// want anders geeft het opnieuw (?) inladen van io.h problemen

#include "asm-lex.c"

#define __TURBOC

