include c:/bc4/spectrum/asm/macro.asm

CORE segment byte use16 public 'CODE'


public xdcbemultab


;now for the ddcb/fdcb opcodes
;Note: inofficial RES/SET instructions in ROM do not work correctly



makeop macro instr,reg,zflag
        local shiftrom
        testrom di,shiftrom
        ifidni <instr>,<sll>
                stc
                rcl byte ptr [di],1
        else
                sahf
                instr byte ptr [di],1
        endif
        lahf
        ifidni <zflag>,<doz>
            test byte ptr [di],0ffh
            ror ah,1
            lahf
        endif
        and ah,0edh                     ;reset H and N
        ifidni <reg>,<bptrdi>           ;official opcode
        else
            and ebx,7
            call word ptr es:[offset xdcbinofftab+2*ebx]
        endif
        setop bptrdi
        rol edi,16              ;Retrieve SP, DI was used to hold IX/Y+d
        xor ebx,ebx
        em2 4,23
shiftrom:
        mov bl,[di]
        ifidni <instr>,<sll>
            stc
            rcl bl,1
        else
            sahf
            instr bl,1
        endif
        lahf
        ifidni <zflag>,<doz>
            test bl,bl
            ror ah,1
            lahf
        endif
        and ah,0edh
        setop bl
        ifidni <reg>,<bptrdi>           ;official opcode
        else
            mov bl,[si+3]
            and ebx,7
            call word ptr es:[offset xdcbinofftab+2*ebx]
        endif
        rol edi,16              ;Retrieve SP, DI was used to hold IX/Y+d
        xor ebx,ebx
        em2 4,23
        endm


xdcb06: makeop rol,bptrdi,doz
xdcb00: 
xdcb01: 
xdcb02: 
xdcb03: 
xdcb04: 
xdcb05: 
xdcb07: makeop rol,anders,doz

xdcb0e: makeop ror,bptrdi,doz
xdcb08: 
xdcb09: 
xdcb0a: 
xdcb0b: 
xdcb0c: 
xdcb0d: 
xdcb0f: makeop ror,anders,doz

xdcb16: makeop rcl,bptrdi,doz
xdcb10: 
xdcb11: 
xdcb12: 
xdcb13: 
xdcb14: 
xdcb15: 
xdcb17: makeop rcl,anders,doz

xdcb1e: makeop rcr,bptrdi,doz
xdcb18: 
xdcb19: 
xdcb1a: 
xdcb1b: 
xdcb1c: 
xdcb1d: 
xdcb1f: makeop rcr,anders,doz

xdcb26: makeop sal,bptrdi,nozflag
xdcb20: 
xdcb21: 
xdcb22: 
xdcb23: 
xdcb24: 
xdcb25: 
xdcb27: makeop sal,anders,nozflag

xdcb2e: makeop sar,bptrdi,nozflag
xdcb28: 
xdcb29: 
xdcb2a: 
xdcb2b: 
xdcb2c: 
xdcb2d: 
xdcb2f: makeop sar,anders,nozflag

xdcb36: makeop sll,bptrdi,doz
xdcb30: 
xdcb31: 
xdcb32: 
xdcb33: 
xdcb34: 
xdcb35: 
xdcb37: makeop sll,anders,doz

xdcb3e: makeop shr,bptrdi,nozflag
xdcb38: 
xdcb39: 
xdcb3a: 
xdcb3b: 
xdcb3c: 
xdcb3d: 
xdcb3f: makeop shr,anders,nozflag




purge makeop


makeop macro instr,reg,bitpos
        local bitnotset,noresset
        ifidni <instr>,<bit>
                setop bitpos
                test byte ptr [di],bitpos
                ror ah,1                ;now C is also OK
                lahf
                xor ah,012h             ;set H, reset N
        else
            testrom di,noresset
            ifidni <instr>,<res>
                and byte ptr [di],0ffh-bitpos
            else
                or byte ptr [di],bitpos
            endif
noresset:
        endif
        ifidni <reg>,<bptrdi>
        else
            and ebx,7
            call word ptr es:[offset xdcbinofftab+2*ebx]
        endif
        rol edi,16              ;Retrieve SP, DI was used to hold IX/Y+d
        xor ebx,ebx
        ifidni <instr>,<bit>
            em2 4,20
        else
            em2 4,23
        endif
        endm


xdcb46: makeop bit,bptrdi,01h
xdcb40: 
xdcb41: 
xdcb42: 
xdcb43: 
xdcb44: 
xdcb45: 
xdcb47: makeop bit,anders,01h

xdcb4e: makeop bit,bptrdi,02h
xdcb48: 
xdcb49: 
xdcb4a: 
xdcb4b: 
xdcb4c: 
xdcb4d: 
xdcb4f: makeop bit,anders,02h

xdcb56: makeop bit,bptrdi,04h
xdcb50: 
xdcb51: 
xdcb52: 
xdcb53: 
xdcb54: 
xdcb55: 
xdcb57: makeop bit,anders,04h

xdcb5e: makeop bit,bptrdi,08h
xdcb58: 
xdcb59: 
xdcb5a: 
xdcb5b: 
xdcb5c: 
xdcb5d: 
xdcb5f: makeop bit,anders,08h

xdcb66: makeop bit,bptrdi,10h
xdcb60: 
xdcb61: 
xdcb62: 
xdcb63: 
xdcb64: 
xdcb65: 
xdcb67: makeop bit,anders,10h

xdcb6e: makeop bit,bptrdi,20h
xdcb68: 
xdcb69: 
xdcb6a: 
xdcb6b: 
xdcb6c: 
xdcb6d: 
xdcb6f: makeop bit,anders,20h

xdcb76: makeop bit,bptrdi,40h
xdcb70: 
xdcb71: 
xdcb72: 
xdcb73: 
xdcb74: 
xdcb75: 
xdcb77: makeop bit,anders,40h

xdcb7e: makeop bit,bptrdi,80h
xdcb78: 
xdcb79: 
xdcb7a: 
xdcb7b: 
xdcb7c: 
xdcb7d: 
xdcb7f: makeop bit,anders,80h

xdcb86: makeop res,bptrdi,01h
xdcb80: 
xdcb81: 
xdcb82: 
xdcb83: 
xdcb84: 
xdcb85: 
xdcb87: makeop res,anders,01h

xdcb8e: makeop res,bptrdi,02h
xdcb88: 
xdcb89: 
xdcb8a: 
xdcb8b: 
xdcb8c: 
xdcb8d: 
xdcb8f: makeop res,anders,02h

xdcb96: makeop res,bptrdi,04h
xdcb90: 
xdcb91: 
xdcb92: 
xdcb93: 
xdcb94: 
xdcb95: 
xdcb97: makeop res,anders,04h

xdcb9e: makeop res,bptrdi,08h
xdcb98: 
xdcb99: 
xdcb9a: 
xdcb9b: 
xdcb9c: 
xdcb9d: 
xdcb9f: makeop res,anders,08h

xdcba6: makeop res,bptrdi,10h
xdcba0: 
xdcba1: 
xdcba2: 
xdcba3: 
xdcba4: 
xdcba5: 
xdcba7: makeop res,anders,10h

xdcbae: makeop res,bptrdi,20h
xdcba8: 
xdcba9: 
xdcbaa: 
xdcbab: 
xdcbac: 
xdcbad: 
xdcbaf: makeop res,anders,20h

xdcbb6: makeop res,bptrdi,40h
xdcbb0: 
xdcbb1: 
xdcbb2: 
xdcbb3: 
xdcbb4: 
xdcbb5: 
xdcbb7: makeop res,anders,40h

xdcbbe: makeop res,bptrdi,80h
xdcbb8: 
xdcbb9: 
xdcbba: 
xdcbbb: 
xdcbbc: 
xdcbbd: 
xdcbbf: makeop res,anders,80h

xdcbc6: makeop set,bptrdi,01h
xdcbc0: 
xdcbc1: 
xdcbc2: 
xdcbc3: 
xdcbc4: 
xdcbc5: 
xdcbc7: makeop set,anders,01h

xdcbce: makeop set,bptrdi,02h
xdcbc8: 
xdcbc9: 
xdcbca: 
xdcbcb: 
xdcbcc: 
xdcbcd: 
xdcbcf: makeop set,anders,02h

xdcbd6: makeop set,bptrdi,04h
xdcbd0: 
xdcbd1: 
xdcbd2: 
xdcbd3: 
xdcbd4: 
xdcbd5: 
xdcbd7: makeop set,anders,04h

xdcbde: makeop set,bptrdi,08h
xdcbd8: 
xdcbd9: 
xdcbda: 
xdcbdb: 
xdcbdc: 
xdcbdd: 
xdcbdf: makeop set,anders,08h

xdcbe6: makeop set,bptrdi,10h
xdcbe0: 
xdcbe1: 
xdcbe2: 
xdcbe3: 
xdcbe4: 
xdcbe5: 
xdcbe7: makeop set,anders,10h

xdcbee: makeop set,bptrdi,20h
xdcbe8: 
xdcbe9: 
xdcbea: 
xdcbeb: 
xdcbec: 
xdcbed: 
xdcbef: makeop set,anders,20h

xdcbf6: makeop set,bptrdi,40h
xdcbf0: 
xdcbf1: 
xdcbf2: 
xdcbf3: 
xdcbf4: 
xdcbf5: 
xdcbf7: makeop set,anders,40h

xdcbfe: makeop set,bptrdi,80h
xdcbf8: 
xdcbf9: 
xdcbfa: 
xdcbfb: 
xdcbfc: 
xdcbfd: 
xdcbff: makeop set,anders,80h


;
;Additional small subroutines that handle inofficial XD CB opcodes
;
xdcbx0:
        mov ch,[di]
        ret

xdcbx1:
        mov cl,[di]
        ret

xdcbx2:
        rol edx,16
        mov dh,[di]
        rol edx,16
        ret

xdcbx3:
        rol edx,16
        mov dl,[di]
        rol edx,16
        ret

xdcbx4:
        mov dh,[di]
        ret

xdcbx5:
        mov dl,[di]
        ret

xdcbx6: ;dummy

xdcbx7:
        mov al,[di]
        ret


CORE ends



SPECDATA segment dword public 'DATA'

xdcbemultab dw xdcb00,xdcb01,xdcb02,xdcb03,xdcb04,xdcb05,xdcb06,xdcb07
        dw xdcb08,xdcb09,xdcb0a,xdcb0b,xdcb0c,xdcb0d,xdcb0e,xdcb0f
        dw xdcb10,xdcb11,xdcb12,xdcb13,xdcb14,xdcb15,xdcb16,xdcb17
        dw xdcb18,xdcb19,xdcb1a,xdcb1b,xdcb1c,xdcb1d,xdcb1e,xdcb1f
        dw xdcb20,xdcb21,xdcb22,xdcb23,xdcb24,xdcb25,xdcb26,xdcb27
        dw xdcb28,xdcb29,xdcb2a,xdcb2b,xdcb2c,xdcb2d,xdcb2e,xdcb2f
        dw xdcb30,xdcb31,xdcb32,xdcb33,xdcb34,xdcb35,xdcb36,xdcb37
        dw xdcb38,xdcb39,xdcb3a,xdcb3b,xdcb3c,xdcb3d,xdcb3e,xdcb3f
        dw xdcb40,xdcb41,xdcb42,xdcb43,xdcb44,xdcb45,xdcb46,xdcb47
        dw xdcb48,xdcb49,xdcb4a,xdcb4b,xdcb4c,xdcb4d,xdcb4e,xdcb4f
        dw xdcb50,xdcb51,xdcb52,xdcb53,xdcb54,xdcb55,xdcb56,xdcb57
        dw xdcb58,xdcb59,xdcb5a,xdcb5b,xdcb5c,xdcb5d,xdcb5e,xdcb5f
        dw xdcb60,xdcb61,xdcb62,xdcb63,xdcb64,xdcb65,xdcb66,xdcb67
        dw xdcb68,xdcb69,xdcb6a,xdcb6b,xdcb6c,xdcb6d,xdcb6e,xdcb6f
        dw xdcb70,xdcb71,xdcb72,xdcb73,xdcb74,xdcb75,xdcb76,xdcb77
        dw xdcb78,xdcb79,xdcb7a,xdcb7b,xdcb7c,xdcb7d,xdcb7e,xdcb7f
        dw xdcb80,xdcb81,xdcb82,xdcb83,xdcb84,xdcb85,xdcb86,xdcb87
        dw xdcb88,xdcb89,xdcb8a,xdcb8b,xdcb8c,xdcb8d,xdcb8e,xdcb8f
        dw xdcb90,xdcb91,xdcb92,xdcb93,xdcb94,xdcb95,xdcb96,xdcb97
        dw xdcb98,xdcb99,xdcb9a,xdcb9b,xdcb9c,xdcb9d,xdcb9e,xdcb9f
        dw xdcba0,xdcba1,xdcba2,xdcba3,xdcba4,xdcba5,xdcba6,xdcba7
        dw xdcba8,xdcba9,xdcbaa,xdcbab,xdcbac,xdcbad,xdcbae,xdcbaf
        dw xdcbb0,xdcbb1,xdcbb2,xdcbb3,xdcbb4,xdcbb5,xdcbb6,xdcbb7
        dw xdcbb8,xdcbb9,xdcbba,xdcbbb,xdcbbc,xdcbbd,xdcbbe,xdcbbf
        dw xdcbc0,xdcbc1,xdcbc2,xdcbc3,xdcbc4,xdcbc5,xdcbc6,xdcbc7
        dw xdcbc8,xdcbc9,xdcbca,xdcbcb,xdcbcc,xdcbcd,xdcbce,xdcbcf
        dw xdcbd0,xdcbd1,xdcbd2,xdcbd3,xdcbd4,xdcbd5,xdcbd6,xdcbd7
        dw xdcbd8,xdcbd9,xdcbda,xdcbdb,xdcbdc,xdcbdd,xdcbde,xdcbdf
        dw xdcbe0,xdcbe1,xdcbe2,xdcbe3,xdcbe4,xdcbe5,xdcbe6,xdcbe7
        dw xdcbe8,xdcbe9,xdcbea,xdcbeb,xdcbec,xdcbed,xdcbee,xdcbef
        dw xdcbf0,xdcbf1,xdcbf2,xdcbf3,xdcbf4,xdcbf5,xdcbf6,xdcbf7
        dw xdcbf8,xdcbf9,xdcbfa,xdcbfb,xdcbfc,xdcbfd,xdcbfe,xdcbff

xdcbemul1020:
        dw xdcb10,xdcb11,xdcb12,xdcb13,xdcb14,xdcb15,xdcb16,xdcb17
        dw xdcb18,xdcb19,xdcb1a,xdcb1b,xdcb1c,xdcb1d,xdcb1e,xdcb1f

xdcbinofftab:
        dw xdcbx0,xdcbx1,xdcbx2,xdcbx3,xdcbx4,xdcbx5,xdcbx6,xdcbx7

public xdcbemul1020


SPECDATA ends



end

