
include c:/bc4/spectrum/asm/macro.asm


CORE segment byte use16 public 'CODE'



redtape macro reg               ;Preparation to execution add/etc instruction
        ifidni <reg>,<bptrbx>
                mov ebx,edx
        endif
        ifidni <reg>,<edxl>
                rol edx,16
        endif
        ifidni <reg>,<edxh>
                rol edx,16
        endif
        ifidni <reg>,<ixh>
                mov bx,rix
        endif
        ifidni <reg>,<ixl>
                mov bx,rix
        endif
        ifidni <reg>,<iyh>
                mov bx,riy
        endif
        ifidni <reg>,<iyl>
                mov bx,riy
        endif
        ifidni <reg>,<ixplusd>
                movsx bx,byte ptr [si+2]
                add bx,rix
        endif
        ifidni <reg>,<iyplusd>
                movsx bx,byte ptr [si+2]
                add bx,riy
        endif
        endm

bluetape macro reg              ;Restore effect of above preparation
        ifidni <reg>,<bptrbx>
                xor ebx,ebx
        endif
        ifidni <reg>,<ixh>
                xor ebx,ebx
        endif
        ifidni <reg>,<ixl>
                xor ebx,ebx
        endif
        ifidni <reg>,<iyh>
                xor ebx,ebx
        endif
        ifidni <reg>,<iyl>
                xor ebx,ebx
        endif
        ifidni <reg>,<ixplusd>
                xor ebx,ebx
        endif
        ifidni <reg>,<iyplusd>
                xor ebx,ebx
        endif
        ifidni <reg>,<edxl>
                rol edx,16
        endif
        ifidni <reg>,<edxh>
                rol edx,16
        endif
        endm



makeop macro instr,reg,zflag
        local shiftrom
        redtape reg             ;put DE in 8 bit regs, or put HL in BX
        ifidni <reg>,<bptrbx>
                testrom bx,shiftrom
        endif
        ifidni <instr>,<sll>
                stc
                rcl reg,1
        else
                sahf
                instr reg,1
        endif
        lahf
        ifidni <zflag>,<doz>
            ifidni <reg>,<bptrbx>
                test reg,0ffh
            else
                test reg,reg
            endif
            ror ah,1
            lahf
        endif
        and ah,0edh             ;reset H and N
        setop reg
        bluetape reg
        ifidni <reg>,<bptrbx>   ;(HL)
            em2 2,15
shiftrom:
            rol ecx,16
            mov cl,[bx]
            ifidni <instr>,<sll>
                stc
                rcl cl,1
            else
                sahf
                instr cl,1
            endif
            lahf
            setop cl
            ifidni <zflag>,<doz>
                test cl,cl
                rol ecx,16
                ror ah,1
                lahf
            else
                rol ecx,16
            endif
            and ah,0edh
            xor ebx,ebx
            em2 2,15
        else
            em2 2,8              ;regular
        endif
        endm


opcb00: makeop rol,ch,doz
opcb01: makeop rol,cl,doz
opcb02: makeop rol,edxh,doz
opcb03: makeop rol,edxl,doz
opcb04: makeop rol,dh,doz
opcb05: makeop rol,dl,doz
opcb06: makeop rol,bptrbx,doz
opcb07: makeop rol,al,doz

opcb08: makeop ror,ch,doz
opcb09: makeop ror,cl,doz
opcb0a: makeop ror,edxh,doz
opcb0b: makeop ror,edxl,doz
opcb0c: makeop ror,dh,doz
opcb0d: makeop ror,dl,doz
opcb0e: makeop ror,bptrbx,doz
opcb0f: makeop ror,al,doz

opcb10: makeop rcl,ch,doz
opcb11: makeop rcl,cl,doz
opcb12: makeop rcl,edxh,doz
opcb13: makeop rcl,edxl,doz
opcb14: makeop rcl,dh,doz
opcb15: makeop rcl,dl,doz
opcb16: makeop rcl,bptrbx,doz
opcb17: makeop rcl,al,doz

opcb18: makeop rcr,ch,doz
opcb19: makeop rcr,cl,doz
opcb1a: makeop rcr,edxh,doz
opcb1b: makeop rcr,edxl,doz
opcb1c: makeop rcr,dh,doz
opcb1d: makeop rcr,dl,doz
opcb1e: makeop rcr,bptrbx,doz
opcb1f: makeop rcr,al,doz

opcb20: makeop sal,ch,nozflag
opcb21: makeop sal,cl,nozflag
opcb22: makeop sal,edxh,nozflag
opcb23: makeop sal,edxl,nozflag
opcb24: makeop sal,dh,nozflag
opcb25: makeop sal,dl,nozflag
opcb26: makeop sal,bptrbx,nozflag
opcb27: makeop sal,al,nozflag

opcb28: makeop sar,ch,nozflag
opcb29: makeop sar,cl,nozflag
opcb2a: makeop sar,edxh,nozflag
opcb2b: makeop sar,edxl,nozflag
opcb2c: makeop sar,dh,nozflag
opcb2d: makeop sar,dl,nozflag
opcb2e: makeop sar,bptrbx,nozflag
opcb2f: makeop sar,al,nozflag

opcb30: makeop sll,ch,doz
opcb31: makeop sll,cl,doz
opcb32: makeop sll,edxh,doz
opcb33: makeop sll,edxl,doz
opcb34: makeop sll,dh,doz
opcb35: makeop sll,dl,doz
opcb36: makeop sll,bptrbx,doz
opcb37: makeop sll,al,doz

opcb38: makeop shr,ch,nozflag
opcb39: makeop shr,cl,nozflag
opcb3a: makeop shr,edxh,nozflag
opcb3b: makeop shr,edxl,nozflag
opcb3c: makeop shr,dh,nozflag
opcb3d: makeop shr,dl,nozflag
opcb3e: makeop shr,bptrbx,nozflag
opcb3f: makeop shr,al,nozflag


;
;According to Sean Yong sean@nemesis.msxnet.org, the inofficial bits
;follow operand & bitpos, not bitpos.  Didn't change it yet.
;

makeop macro instr,reg,bitpos
        local noresset
        redtape reg
        ifidni <instr>,<bit>
                setop bitpos            ;this takes care of inoff bits
                test reg,bitpos         ;now S,Z and P are OK
                ror ah,1                ;now C is also OK
                lahf
                xor ah,012h             ;set H, reset N
        else
            ifidni <reg>,<bptrbx>
                testrom bx,noresset
            endif
            ifidni <instr>,<res>
                and reg,0ffh-bitpos
            else
                or reg,bitpos
            endif
noresset:
        endif
        bluetape reg
        ifidni <instr>,<bit>
            ifidni <reg>,<bptrbx>
            else
                xor ebx,ebx
            endif
        endif
        ifidni <reg>,<bptrbx>
            ifidni <instr>,<bit>
                em2 2,12
            else
                em2 2,15
            endif
        else
                em2 2,8
        endif
        endm



opcb40: makeop bit,ch,01h
opcb41: makeop bit,cl,01h
opcb42: makeop bit,edxh,01h
opcb43: makeop bit,edxl,01h
opcb44: makeop bit,dh,01h
opcb45: makeop bit,dl,01h
opcb46: makeop bit,bptrbx,01h
opcb47: makeop bit,al,01h

opcb48: makeop bit,ch,02h
opcb49: makeop bit,cl,02h
opcb4a: makeop bit,edxh,02h
opcb4b: makeop bit,edxl,02h
opcb4c: makeop bit,dh,02h
opcb4d: makeop bit,dl,02h
opcb4e: makeop bit,bptrbx,02h
opcb4f: makeop bit,al,02h

opcb50: makeop bit,ch,04h
opcb51: makeop bit,cl,04h
opcb52: makeop bit,edxh,04h
opcb53: makeop bit,edxl,04h
opcb54: makeop bit,dh,04h
opcb55: makeop bit,dl,04h
opcb56: makeop bit,bptrbx,04h
opcb57: makeop bit,al,04h

opcb58: makeop bit,ch,08h
opcb59: makeop bit,cl,08h
opcb5a: makeop bit,edxh,08h
opcb5b: makeop bit,edxl,08h
opcb5c: makeop bit,dh,08h
opcb5d: makeop bit,dl,08h
opcb5e: makeop bit,bptrbx,08h
opcb5f: makeop bit,al,08h

opcb60: makeop bit,ch,10h
opcb61: makeop bit,cl,10h
opcb62: makeop bit,edxh,10h
opcb63: makeop bit,edxl,10h
opcb64: makeop bit,dh,10h
opcb65: makeop bit,dl,10h
opcb66: makeop bit,bptrbx,10h
opcb67: makeop bit,al,10h

opcb68: makeop bit,ch,20h
opcb69: makeop bit,cl,20h
opcb6a: makeop bit,edxh,20h
opcb6b: makeop bit,edxl,20h
opcb6c: makeop bit,dh,20h
opcb6d: makeop bit,dl,20h
opcb6e: makeop bit,bptrbx,20h
opcb6f: makeop bit,al,20h

opcb70: makeop bit,ch,40h
opcb71: makeop bit,cl,40h
opcb72: makeop bit,edxh,40h
opcb73: makeop bit,edxl,40h
opcb74: makeop bit,dh,40h
opcb75: makeop bit,dl,40h
opcb76: makeop bit,bptrbx,40h
opcb77: makeop bit,al,40h

opcb78: makeop bit,ch,80h
opcb79: makeop bit,cl,80h
opcb7a: makeop bit,edxh,80h
opcb7b: makeop bit,edxl,80h
opcb7c: makeop bit,dh,80h
opcb7d: makeop bit,dl,80h
opcb7e: makeop bit,bptrbx,80h
opcb7f: makeop bit,al,80h

opcb80: makeop res,ch,01h
opcb81: makeop res,cl,01h
opcb82: makeop res,edxh,01h
opcb83: makeop res,edxl,01h
opcb84: makeop res,dh,01h
opcb85: makeop res,dl,01h
opcb86: makeop res,bptrbx,01h
opcb87: makeop res,al,01h

opcb88: makeop res,ch,02h
opcb89: makeop res,cl,02h
opcb8a: makeop res,edxh,02h
opcb8b: makeop res,edxl,02h
opcb8c: makeop res,dh,02h
opcb8d: makeop res,dl,02h
opcb8e: makeop res,bptrbx,02h
opcb8f: makeop res,al,02h

opcb90: makeop res,ch,04h
opcb91: makeop res,cl,04h
opcb92: makeop res,edxh,04h
opcb93: makeop res,edxl,04h
opcb94: makeop res,dh,04h
opcb95: makeop res,dl,04h
opcb96: makeop res,bptrbx,04h
opcb97: makeop res,al,04h

opcb98: makeop res,ch,08h
opcb99: makeop res,cl,08h
opcb9a: makeop res,edxh,08h
opcb9b: makeop res,edxl,08h
opcb9c: makeop res,dh,08h
opcb9d: makeop res,dl,08h
opcb9e: makeop res,bptrbx,08h
opcb9f: makeop res,al,08h

opcba0: makeop res,ch,10h
opcba1: makeop res,cl,10h
opcba2: makeop res,edxh,10h
opcba3: makeop res,edxl,10h
opcba4: makeop res,dh,10h
opcba5: makeop res,dl,10h
opcba6: makeop res,bptrbx,10h
opcba7: makeop res,al,10h

opcba8: makeop res,ch,20h
opcba9: makeop res,cl,20h
opcbaa: makeop res,edxh,20h
opcbab: makeop res,edxl,20h
opcbac: makeop res,dh,20h
opcbad: makeop res,dl,20h
opcbae: makeop res,bptrbx,20h
opcbaf: makeop res,al,20h

opcbb0: makeop res,ch,40h
opcbb1: makeop res,cl,40h
opcbb2: makeop res,edxh,40h
opcbb3: makeop res,edxl,40h
opcbb4: makeop res,dh,40h
opcbb5: makeop res,dl,40h
opcbb6: makeop res,bptrbx,40h
opcbb7: makeop res,al,40h

opcbb8: makeop res,ch,80h
opcbb9: makeop res,cl,80h
opcbba: makeop res,edxh,80h
opcbbb: makeop res,edxl,80h
opcbbc: makeop res,dh,80h
opcbbd: makeop res,dl,80h
opcbbe: makeop res,bptrbx,80h
opcbbf: makeop res,al,80h

opcbc0: makeop set,ch,01h
opcbc1: makeop set,cl,01h
opcbc2: makeop set,edxh,01h
opcbc3: makeop set,edxl,01h
opcbc4: makeop set,dh,01h
opcbc5: makeop set,dl,01h
opcbc6: makeop set,bptrbx,01h
opcbc7: makeop set,al,01h

opcbc8: makeop set,ch,02h
opcbc9: makeop set,cl,02h
opcbca: makeop set,edxh,02h
opcbcb: makeop set,edxl,02h
opcbcc: makeop set,dh,02h
opcbcd: makeop set,dl,02h
opcbce: makeop set,bptrbx,02h
opcbcf: makeop set,al,02h

opcbd0: makeop set,ch,04h
opcbd1: makeop set,cl,04h
opcbd2: makeop set,edxh,04h
opcbd3: makeop set,edxl,04h
opcbd4: makeop set,dh,04h
opcbd5: makeop set,dl,04h
opcbd6: makeop set,bptrbx,04h
opcbd7: makeop set,al,04h

opcbd8: makeop set,ch,08h
opcbd9: makeop set,cl,08h
opcbda: makeop set,edxh,08h
opcbdb: makeop set,edxl,08h
opcbdc: makeop set,dh,08h
opcbdd: makeop set,dl,08h
opcbde: makeop set,bptrbx,08h
opcbdf: makeop set,al,08h

opcbe0: makeop set,ch,10h
opcbe1: makeop set,cl,10h
opcbe2: makeop set,edxh,10h
opcbe3: makeop set,edxl,10h
opcbe4: makeop set,dh,10h
opcbe5: makeop set,dl,10h
opcbe6: makeop set,bptrbx,10h
opcbe7: makeop set,al,10h

opcbe8: makeop set,ch,20h
opcbe9: makeop set,cl,20h
opcbea: makeop set,edxh,20h
opcbeb: makeop set,edxl,20h
opcbec: makeop set,dh,20h
opcbed: makeop set,dl,20h
opcbee: makeop set,bptrbx,20h
opcbef: makeop set,al,20h

opcbf0: makeop set,ch,40h
opcbf1: makeop set,cl,40h
opcbf2: makeop set,edxh,40h
opcbf3: makeop set,edxl,40h
opcbf4: makeop set,dh,40h
opcbf5: makeop set,dl,40h
opcbf6: makeop set,bptrbx,40h
opcbf7: makeop set,al,40h

opcbf8: makeop set,ch,80h
opcbf9: makeop set,cl,80h
opcbfa: makeop set,edxh,80h
opcbfb: makeop set,edxl,80h
opcbfc: makeop set,dh,80h
opcbfd: makeop set,dl,80h
opcbfe: makeop set,bptrbx,80h
opcbff: makeop set,al,80h


purge makeop
purge redtape
purge bluetape


opactiverlrr:                           ;to catch bit that just's been loaded
        cmp _iimode,3
        jbsh activerlrr_cont            ;jump out if not loading at all
        cmp _miractive,0
        jesh activerlrr_cont            ;jump out if not mirroring
        cmp _mirinned,0
        jesh activerlrr_cont            ;jump out if data bit's been processed
        mov _miridle,0                  ;signal: this 20ms blk bits've bn saved
        cmp _mirbuflen,0
        je acrlrr_newbuf                ;send out buffer if no bytes left
        mov _mirinned,0                 ;signal: bit's been processed
        push cx
        inc _mirbitcount
        mov cx,_mircurbit               ;this clears CH
        sahf                            ;get carry
        adc ch,ch
        shl ch,cl
        or byte ptr _mirbyte,ch         ;include bit in right place
        dec cl
        js short acrlrr_newbyte
        mov byte ptr _mircurbit,cl
activerlrr_cont0:
        pop cx
activerlrr_cont:
        jmp es:[offset cbemul1020-020h+2*ebx]
acrlrr_newbyte:
        mov _mircurbit,7                ;reset bit position to 7
        mov cx,_mirbyte
        push si
        push es
        les si,_mirbufptr
        mov es:[si],cl
        pop es
        inc si
        mov word ptr _mirbufptr,si      ;points to new byte
        pop si
        mov _mirbyte,0                  ;reset new byte to 0
        dec _mirbuflen                  ;decrease # of bytes left
        jmp activerlrr_cont0
acrlrr_newbuf:
        mov bx,msg_mirbuffull
        jmp FAR PTR emul_ret



;Only difference here is: jmp to instr uses xdcbemul1020 i.o. cbemul1020
;
opactivexdrlrr:                         ;to catch bit that just's been loaded
        cmp _iimode,3
        jbsh activexdrlrr_cont          ;jump out if not loading at all
        cmp _miractive,0
        jesh activexdrlrr_cont          ;jump out if not mirroring
        cmp _mirinned,0
        jesh activexdrlrr_cont          ;jump out if data bit's been processed
        cmp _mirbuflen,0
        je acxdrlrr_newbuf              ;send out buffer if no bytes left
        mov _mirinned,0                 ;signal: bit's been processed
        mov _miridle,0                  ;signal: this 20ms blk bits've bn saved
        push cx
        inc _mirbitcount
        mov cx,_mircurbit               ;this clears CH
        sahf                            ;get carry
        adc ch,ch
        shl ch,cl
        or byte ptr _mirbyte,ch         ;include bit in right place
        dec cl
        js short acxdrlrr_newbyte
        mov byte ptr _mircurbit,cl
activexdrlrr_cont0:
        pop cx
activexdrlrr_cont:
        jmp es:[offset xdcbemul1020-020h+2*ebx]
acxdrlrr_newbyte:
        mov _mircurbit,7                ;reset bit position to 7
        mov cx,_mirbyte
        push si
        push es
        les si,_mirbufptr
        mov es:[si],cl
        pop es
        inc si
        mov word ptr _mirbufptr,si      ;points to new byte
        pop si
        mov _mirbyte,0                  ;reset new byte to 0
        dec _mirbuflen                  ;decrease # of bytes left
        jmp activexdrlrr_cont0
acxdrlrr_newbuf:
        mov bx,msg_mirbuffull
        jmp FAR PTR emul_ret





CORE ends






SPECDATA segment dword public 'DATA'



cbemultab dw opcb00,opcb01,opcb02,opcb03,opcb04,opcb05,opcb06,opcb07
        dw opcb08,opcb09,opcb0a,opcb0b,opcb0c,opcb0d,opcb0e,opcb0f
        dw opcb10,opcb11,opcb12,opcb13,opcb14,opcb15,opcb16,opcb17
        dw opcb18,opcb19,opcb1a,opcb1b,opcb1c,opcb1d,opcb1e,opcb1f
        dw opcb20,opcb21,opcb22,opcb23,opcb24,opcb25,opcb26,opcb27
        dw opcb28,opcb29,opcb2a,opcb2b,opcb2c,opcb2d,opcb2e,opcb2f
        dw opcb30,opcb31,opcb32,opcb33,opcb34,opcb35,opcb36,opcb37
        dw opcb38,opcb39,opcb3a,opcb3b,opcb3c,opcb3d,opcb3e,opcb3f
        dw opcb40,opcb41,opcb42,opcb43,opcb44,opcb45,opcb46,opcb47
        dw opcb48,opcb49,opcb4a,opcb4b,opcb4c,opcb4d,opcb4e,opcb4f
        dw opcb50,opcb51,opcb52,opcb53,opcb54,opcb55,opcb56,opcb57
        dw opcb58,opcb59,opcb5a,opcb5b,opcb5c,opcb5d,opcb5e,opcb5f
        dw opcb60,opcb61,opcb62,opcb63,opcb64,opcb65,opcb66,opcb67
        dw opcb68,opcb69,opcb6a,opcb6b,opcb6c,opcb6d,opcb6e,opcb6f
        dw opcb70,opcb71,opcb72,opcb73,opcb74,opcb75,opcb76,opcb77
        dw opcb78,opcb79,opcb7a,opcb7b,opcb7c,opcb7d,opcb7e,opcb7f
        dw opcb80,opcb81,opcb82,opcb83,opcb84,opcb85,opcb86,opcb87
        dw opcb88,opcb89,opcb8a,opcb8b,opcb8c,opcb8d,opcb8e,opcb8f
        dw opcb90,opcb91,opcb92,opcb93,opcb94,opcb95,opcb96,opcb97
        dw opcb98,opcb99,opcb9a,opcb9b,opcb9c,opcb9d,opcb9e,opcb9f
        dw opcba0,opcba1,opcba2,opcba3,opcba4,opcba5,opcba6,opcba7
        dw opcba8,opcba9,opcbaa,opcbab,opcbac,opcbad,opcbae,opcbaf
        dw opcbb0,opcbb1,opcbb2,opcbb3,opcbb4,opcbb5,opcbb6,opcbb7
        dw opcbb8,opcbb9,opcbba,opcbbb,opcbbc,opcbbd,opcbbe,opcbbf
        dw opcbc0,opcbc1,opcbc2,opcbc3,opcbc4,opcbc5,opcbc6,opcbc7
        dw opcbc8,opcbc9,opcbca,opcbcb,opcbcc,opcbcd,opcbce,opcbcf
        dw opcbd0,opcbd1,opcbd2,opcbd3,opcbd4,opcbd5,opcbd6,opcbd7
        dw opcbd8,opcbd9,opcbda,opcbdb,opcbdc,opcbdd,opcbde,opcbdf
        dw opcbe0,opcbe1,opcbe2,opcbe3,opcbe4,opcbe5,opcbe6,opcbe7
        dw opcbe8,opcbe9,opcbea,opcbeb,opcbec,opcbed,opcbee,opcbef
        dw opcbf0,opcbf1,opcbf2,opcbf3,opcbf4,opcbf5,opcbf6,opcbf7
        dw opcbf8,opcbf9,opcbfa,opcbfb,opcbfc,opcbfd,opcbfe,opcbff


cbemul1020 dw opcb10,opcb11,opcb12,opcb13,opcb14,opcb15,opcb16,opcb17
        dw opcb18,opcb19,opcb1a,opcb1b,opcb1c,opcb1d,opcb1e,opcb1f



SPECDATA ends


end



