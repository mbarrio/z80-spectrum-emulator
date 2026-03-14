: This batch file compiles and links all utilities that come with the
: registered distribution of Z80.
: The assembler A86 must be somewhere in the PATH.

: Root directory of C compiler
set maindir=\borlandc
: Library directory
set lib=%maindir%\lib
: Include directory
set inc=%maindir%\include
: C compiler
set cc=%maindir%\bin\bcc
: Linker
set link=%maindir%\bin\tlink
set maindir=
: Math libraries.  A 'c' or 'l' is added depending on the memory model
set mathlib=%lib%\emu %lib%\fp87 %lib%\math

: Standard compiler options
set stdopts=-1- -y-

set test=test
if not (%test%)==(test) pause Not enough environment space
set test=

:Options used:
:
: -1-   8088/8086 instruction set.  Change to -2 or -3 for '286, '386 set resp.
: -y-   Do not include line number info
: -ml   Select large memory model
: -mc   Select compact memory model
: -G    Optimize for speed
: -G-   Optimize for size
: -O2   Optimization level 2
: -lm   Pass linker option: Map file
: -f    Enable floating point emulation
: -ff   Do some floating point optimization
: -I    Set include directory
: -L    Set library directory
: -c    Compile only

%cc% %stdopts% -ml -G -lm -f -ff -I%inc% -L%lib% out2voc.c

%cc% %stdopts% -ml -G- -f -ff -c -I%inc% -L%lib% convert.c
a86 convcode.8 convcode.obj
%link% %lib%\c0l convcode convert,convert,,%lib%\cl %mathlib%l

%cc% %stdopts% -ml -G- -lm -I%inc% -L%lib% disciple.c

%cc% %stdopts% -ml -G- -lm -I%inc% -L%lib% tap2voc.c

%cc% %stdopts% -ml -G- -c -I%inc% -L%lib% z802tap.c
%link% %lib%\c0l loader z802tap,z802tap,,%lib%\cl

%cc% %stdopts% -ml -G- -lm -I%inc% -L%lib% z80dump.c

%cc% %stdopts% -ml -G- -lm -I%inc% -L%lib% adddat.c

%cc% %stdopts% -mc -O2 -I%inc% -L%lib% -c sounddev.cpp
%cc% %stdopts% -mc -O2 -I%inc% -L%lib% -c sbdevice.cpp
%cc% %stdopts% -mc -O2 -I%inc% -L%lib% -c readsb.cpp
tasm dma_code.asm
echo %lib%\c0c sounddev sbdevice dma_code readsb >respfile
echo readsb >>respfile
echo readsb >>respfile
echo %lib%\cc %mathlib%c >>respfile
%link% @respfile
del respfile

a86 readvoc.8

a86 convz80.8

set lib=
set inc=
set cc=
set mathlib=

del out2voc.obj
del convert.obj
del convcode.obj
del disciple.obj
del tap2voc.obj
del z802tap.obj
del z80dump.obj
del adddat.obj
del sounddev.obj
del sbdevice.obj
del readsb.obj
del dma_code.obj

del *.map
del *.sym

