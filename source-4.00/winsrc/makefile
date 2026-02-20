#****************************************************************************
#                                                                           *
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
# PURPOSE.                                                                  *
#                                                                           *
# Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
#                                                                           *
#****************************************************************************

NAME = VZ80D

!ifdef DEBUG
DDEBUG  =-DDEBLEVEL=1 -DDEBUG
!else
DDEBUG  =-DDEBLEVEL=0
!endif

#       Definitions for MASM 6 Assembler

ASM    = ml
AFLAGS = -Ic:\ddk\inc32 -coff -DBLD_COFF -DIS_32 -W2 -c -Cx -Zm -DMASM6 $(DDEBUG)
ASMENV = ML
LFLAGS = /VXD /NOD

#       MASM 6 only inference rules

.asm.obj:
    set $(ASMENV)=$(AFLAGS)
    $(ASM) -Fo$*.obj $< 

all : $(NAME).vxd

# vxdstub.exe: vxdstub.obj
#    $(LINK) vxdstub.obj;

vz80d.obj vz80d.lst: vz80d.asm

OBJS = vz80d.obj

$(NAME).vxd: $(NAME).def $(OBJS)
#        link @<<$(NAME).lnk
        \msdev\bin\link @<<$(NAME).lnk
$(LFLAGS) 
/OUT:$(NAME).vxd
/MAP:$(NAME).map
/DEF:$(NAME).def
$(OBJS)
<<
        mapsym -s -o $(NAME).sym $(NAME).map

clean:
	-@del *.obj
	-@del *.vxd
	-@del *.exp
	-@del *.lib
	-@del *.map
	-@del *.sym
