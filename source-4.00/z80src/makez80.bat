:
: This batch file compiles the shareware version of Z80 version 4.00
:
: You need: A86 version 3.22 for assembling the .8 files
:           Any C compiler (I use Borland C version 4.02)
:           Any linker (I use Turbo Link version 6.10)
:
: The compiler switches are mostly optimizations, except -ml: use large model.
:
:
:
a86 emul.8 xdcb.8 xdcb.8 dd.8 dd.8 cb.8 ed.8 emul.obj
a86 z80p1.8 z80p2.8 mdrvp1.8 mdrvp2.8 z80.obj
a86 miscp1.8 miscp2.8 miscp3.8 misc.obj
a86 mdrvbuf.8 mdrvbuf.obj
a86 video.8 video.obj
a86 cif.8 cif.obj
a86 vga2.8 vga2.obj
a86 vidtab.8 vidtab.obj
a86 tables.8 tables.obj
a86 voc.8 voc.obj
a86 discp1.8 discp2.8 disc.obj
bcc -1- -a1 -y- -f- -G- -Oe -Op -Ob -O -Z -d -ml -c -otzx.obj tzx.c
bcc -1- -y- -f- -G- -Oe -Op -Ob -O -Z -d -ml -c -oxtra.obj xtra.c
tlink %1 xtra tzx disc mdrvbuf z80 misc voc video vga2 cif vidtab tables emul introscr,z80,z80;
del *.sym
z80
del emul.obj
del z80.obj
del misc.obj
del mdrvbuf.obj
del video.obj
del cif.obj
del vga2.obj
del vidtab.obj
del tables.obj
del voc.obj
del disc.obj
del tzx.obj
del xtra.obj
