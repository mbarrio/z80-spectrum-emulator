
include c:/bc4/spectrum/asm/macro.asm


CORE segment byte use16 public 'CODE'



;inofficial flags have not been implemented in ED block instructions



;ld bc,nn
op01:   getop cx
;       em 3,10
        add si,3
        sub ebp,10*010000h-1
        jb wrapped
emulshortfar_label:
        emulshort

;ld de,nn
op11:   rol edx,16
        getop dx
        rol edx,16
        em 3,10

;ld hl,nn
op21:   getop dx
        em 3,10

;ld sp,nn
op31:   getop di
        em 3,10

;ld (bc),a
op02:   getinbx bc
        pokex bx,al,op02x
        xor bx,bx
        em 1,7
op02x:  mov _specialdata,al
opspec: mov _specialaddr,bx
        sub ebp,010000h*7-1
        inc si
        mov bx,msg_memspecial
        jmp FAR PTR emul_ret_eoi

;ld a,(bc)
op0a:   getinbx bc
        mov al,[bx]
        xor bx,bx
        em 1,7

;ld (de),a
op12:   getinbx de
        pokex bx,al,op02x
        xor bx,bx
        em 1,7

;ld a,(de)
op1a:   getinbx de
        mov al,[bx]
        xor bx,bx
        em 1,7

;ld (nn),hl
op22:   getop bx
        pokew bx,dx,dh,dl
        emc 3,16

;ld hl,(nn)                             ;when this opcode is encountered at
op2a:   cmp si,8                        ;addr 8, the if1 should be paged
        jesh op2a_pageif1
        getop bx                        
        mov dx,[bx]
        xor bx,bx
        em 3,16
op2a_pageif1:
        mov bx,msg_rst08
        jmp emul_ret

;ld (nn),a
op32:   getop bx
        pokex bx,al,op32x
        emc 3,13
op32x:  mov _specialdata,al
        mov _specialaddr,bx
        sub ebp,010000h*13-1
        add si,3
        mov bx,msg_memspecial
        jmp FAR PTR emul_ret_eoi

;ld a,(nn)
op3a:   getop bx
        mov al,[bx]
        emc 3,13

;inc bc
op03:   inc cx
        em 1,6

;inc de
op13:   add edx,010000h
        em 1,6

;inc hl
op23:
        cmp si,01708h                   ;inc hl at #1708?  Page if1
        jesh op23_if1
        inc dx
        em 1,6
op23_if1:
        mov bx,msg_rst08
        jmp emul_ret


;inc sp
op33:   inc di
        em 1,6

;dec bc/de/hl/sp
op0b:   dec cx
        cmp dword ptr [si],020b1780bh   ;dec bc/ld a,b/or c/jr nz,x
        jesh op0bx
        em 1,6
op0bx:  mov al,ch
        or al,cl
        lahf
        jnesh op0bjr
        and ah,0edh
        em4 5,21
op0bjr:
        and ah,0edh
        movsx bx,byte ptr[si+4]
        add si,bx
        xor bx,bx
        em4 5,26

op1b:   sub edx,010000h
        cmp dword ptr [si],020b37a1bh   ;dec de/ld a,d/or e/jr nz,x
        jesh op1bx
        em 1,6
op1bx:  rol edx,16
        mov al,dh
        or al,dl
        lahf
        rol edx,16
        jnesh op0bjr
        and ah,0edh
        em4 5,21

op2b:   dec dx
        cmp dword ptr [si],020b57c2bh   ;dec hl/ld a,h/or l/jr nz,x
        jesh op2bx
        em 1,6
op2bx:  mov al,dh
        or al,dl
        lahf
        jne op0bjr
        and ah,0edh
        em4 5,21

op3b:   dec di
        em 1,6



;inc b/c/d/e/h/l/(hl)/a

op04:
        sahf
        inc ch
        setop ch
        ova
        em 1,4
op0c:
        sahf
        inc cl
        setop cl
        ova
        em 1,4
op14:
        rol edx,16
        sahf
        inc dh
        setop dh
        ova
        rol edx,16
        em 1,4
op1c:
        rol edx,16
        sahf
        inc dl
        setop dl
        ova
        rol edx,16
        em 1,4
op24:
        sahf
        inc dh
        setop dh
        ova
        em 1,4
op2c:
        sahf
        inc dl
        setop dl
        ova
        em 1,4
op34:
        mov bx,dx
        testrom bx,op34rom
        sahf
        inc bptrbx
        setop bptrbx
        ova
        xor bx,bx
        em 1,11
op34rom:
        mov bl,[bx]
        sahf
        inc bl
        setop bl
        ova
        xor bx,bx
        em 1,11
op3c:
        sahf
        inc al
        setop al
        ova
        em 1,4



;dec b/c/d/e/h/l/(hl)/a

op05:
        sahf
        dec ch
        setop ch
        ovs
        em 1,4
op0d:
        sahf
        dec cl
        setop cl
        ovs
        em 1,4
op15:
        rol edx,16
        sahf
        dec dh
        setop dh
        ovs
        rol edx,16
        em 1,4
op1d:
        rol edx,16
        sahf
        dec dl
        setop dl
        ovs
        rol edx,16
        em 1,4
op25:
        sahf
        dec dh
        setop dh
        ovs
        em 1,4
op2d:
        sahf
        dec dl
        setop dl
        ovs
        em 1,4
op35:
        mov bx,dx
        testrom bx,op35rom
        sahf
        dec byte ptr [bx]
        setop bptrbx
        ovs
        xor bx,bx
        em 1,11
op35rom:
        mov bl,[bx]
        sahf
        dec bl
        setop bl
        ovs
        xor bx,bx
        em 1,11
op3d:
        sahf
        dec al
        setop al
        ovs
        em 1,4





;ld b/c/d/e/h/l/(hl)/a,n

op06:
        mov ch,[si+1]
        em 2,7
op0e:
        mov cl,[si+1]
        em 2,7
op16:
        rol edx,16
        mov dh,[si+1]
        rol edx,16
        em 2,7
op1e:
        rol edx,16
        mov dl,[si+1]
        rol edx,16
        em 2,7
op26:
        mov dh,[si+1]
        em 2,7
op2e:
        mov dl,[si+1]
        em 2,7
op36:
        testrom dx,op36noram
        rol eax,16
        mov al,[si+1]
        mov bx,dx
        mov [bx],al
        rol eax,16
        xor bx,bx
op36noram:
        em 2,10
op3e:
        mov al,[si+1]
        em 2,7




;rlca/rrca/rla/rra/daa/cpl/scf/ccf

op07:
        sahf
        rol al,1
        setop al
        lahf
        and ah,0edh                     ;added 23/9/98
        em 1,4
op0f:
        sahf
        ror al,1
        setop al
        lahf
        and ah,0edh                     ;added 23/9/98
        em 1,4
op17:
        sahf
        rcl al,1
        setop al
        lahf
        and ah,0edh                     ;added 23/9/98
        em 1,4
op1f:
        sahf
        rcr al,1
        setop al
        lahf
        and ah,0edh                     ;added 23/9/98
        em 1,4
op27:
        test ah,2
        jnesh daa_sub
        sahf
        daa
        setop al
        lahf
        em 1,4
daa_sub:
        sahf
        das
        setop al
        lahf
        em 1,4
op2f:
        not al
        or ah,012h
        setop al
        em 1,4
op37:
        or ah,1
        and ah,0edh
        setop al
        em 1,4
op3f:
        and ah,0edh                     ;added 23/9/98
        xor ah,1
        sahf
        jc op3f_wasnc
        or ah,010h                      ;make H old value of C (23/9/98)
op3f_wasnc:
        setop al
        em 1,4




;nop/ex af,af'/djnz/jr/jr nz/jr z/jr nc/jr c

op00:
        em 1,4
op08:
        xchg ax,rfaa
        mov bl,temp_f
        xchg bl,temp_fa
        mov temp_f,bl
        em 1,4
op10:
        dec ch
        jesh op10_end
        movsx bx,byte ptr[si+1]
        cmp bx,0fffeh                   ;djnz $
        jesh op10_fast
        add si,bx
        xor bx,bx
        em 2,13
op10_end:
        em 2,8
op10_fast:
        mov bx,ax
        mov eax,ebp
        shr eax,16
        cmp eax,3315
        jae short op10_gottime
        div byte ptr [byte13]
        cmp al,ch
        jae short op10_gottime
        sub ch,al
        inc al
        xor ah,ah
        add bp,ax                       ;add to R register
        mul byte ptr [byte13]
        shl eax,16
        sub ebp,eax                     ;update time counter, it wrapped
        mov ax,bx
        xor bx,bx
        jmp wrapped
op10_gottime:
        mov al,ch
        mul byte ptr [byte13]
        dec ax                          ;Compensate for R reg update below
        shl eax,16
        sub al,ch
        dec ah
        xor ch,ch                       ;clear B register
        sub ebp,eax                     ;update time counter and R register
        mov ax,bx                       ; (no wrap)
        xor bx,bx
        em 2,8                          ;execute djnz $ with B=1

op18:
        movsx bx,byte ptr[si+1]
        add si,bx
        xor bx,bx
        em 2,12
op20:
        cmp byte ptr [si+1],0fdh        ;jr nz,$-1
;        jesh op20x
op20_0:
        sahf
        jne op18
        em 2,7
op20x:  cmp byte ptr [si-1],03dh        ;dec a
        jesh op20xx
        sahf
        jne op18
op20fallthrough:
        em 2,7
op20xx:                                 ;ld b,1/xor a/cp b/jr l2/dec a/l2:jr nz,$-1
        sahf                            ; will not work properly
        je op20fallthrough
        mov bx,ax
        mov eax,ebp
        shr eax,16
        cmp eax,4080
        jae short op20_gottime
        shr ax,4                        ;divide by 16
        cmp al,bl
        jae short op20_gottime
        sub bl,al
        xor ah,ah
        shl ax,1
        add bp,ax                       ;add to R register
        shl eax,19                      ;EAXhi = 16 * # steps
        sub ebp,eax                     ;update time counter, it wrapped
        mov ax,bx                       ;keep flags, still NZ
        xor bx,bx
        jmp wrapped
op20_gottime:
        mov al,bl
        xor ah,ah
        shl ax,4
        dec ax                          ;Compensate for R reg update below
        shl eax,16
        sub al,bl
        sbb ah,ah
        sub al,bl
        sbb ah,0
        and bx,06b00h                   ;Clear A, S, H, P/V
        or bh,042h                      ;Set Z, N
        sub ebp,eax                     ;update time counter and R register
        mov ax,bx                       ; (no wrap)
        xor bx,bx
        em 2,7                          ;execute jr nz,$-1 with Z set
op28:
        sahf
        je op18
        em 2,7
op30:
        sahf
        jnc op18
        em 2,7
op38:
        sahf
        jc op18
        em 2,7




;add hl,bc/de/hl/sp

op09:
        and ah,0fch
        add dx,cx
        adc ah,0
        setop dh
        em 1,11
op19:
        and ah,0fch
        mov ebx,edx
        rol ebx,16
        add dx,bx
        adc ah,0
        setop dh
        xor ebx,ebx
        em 1,11
op29:
        and ah,0fch
        add dx,dx
        adc ah,0
        setop dh
        em 1,11
op39:
        and ah,0fch
        add dx,di
        adc ah,0
        setop dh
        em 1,11





;8 bit load group



;ld (b,c,h,l,a),(b,c,h,l,a) : 25

op40:   em 1,4

op41:   mov ch,cl
        em 1,4

op44:   mov ch,dh
        em 1,4

op45:   mov ch,dl
        em 1,4

op47:   mov ch,al
        em 1,4

op48:   mov cl,ch
        em 1,4

op49:   em 1,4

op4c:   mov cl,dh
        em 1,4

op4d:   mov cl,dl
        em 1,4

op4f:   mov cl,al
        em 1,4

op60:   mov dh,ch
        em 1,4

op61:   mov dh,cl
        em 1,4

op64:   em 1,4

op65:   mov dh,dl
        em 1,4

op67:   mov dh,al
        em 1,4

op68:   mov dl,ch
        em 1,4

op69:   mov dl,cl
        em 1,4

op6c:   mov dl,dh
        em 1,4

op6d:   em 1,4

op6f:   mov dl,al
        em 1,4

op78:   mov al,ch
        em 1,4

op79:   mov al,cl
        em 1,4

op7c:   mov al,dh
        em 1,4

op7d:   mov al,dl
        em 1,4

op7f:   em 1,4





;ld (b,c,d,e,h,l,a),(d,e) : 14

op42:   rol edx,16
        mov ch,dh
        rol edx,16
        em 1,4

op43:   rol edx,16
        mov ch,dl
        rol edx,16
        em 1,4

op4a:   rol edx,16
        mov cl,dh
        rol edx,16
        em 1,4

op4b:   rol edx,16
        mov cl,dl
        rol edx,16
        em 1,4

op52:   em 1,4

op53:   rol edx,16
        mov dh,dl
        rol edx,16
        em 1,4

op5a:   rol edx,16
        mov dl,dh
        rol edx,16
        em 1,4

op5b:   em 1,4

op62:   rol edx,16
        mov bl,dh
        rol edx,16
        mov dh,bl
        em 1,4

op63:   ror edx,8               ;ld h,e
        mov dl,dh
        rol edx,8
        em 1,4

op6a:   rol edx,8               ;ld l,d
        mov dh,dl
        ror edx,8
        em 1,4

op6b:   rol edx,16
        mov bl,dl
        rol edx,16
        mov dl,bl
        em 1,4

op7a:   rol edx,16
        mov al,dh
        rol edx,16
        em 1,4

op7b:   rol edx,16
        mov al,dl
        rol edx,16
        em 1,4




;ld (d,e),(b,c,h,l,a) : 10

op50:   rol edx,16
        mov dh,ch
        rol edx,16
        em 1,4

op51:   rol edx,16
        mov dh,cl
        rol edx,16
        em 1,4

op54:   mov bl,dh
        rol edx,16
        mov dh,bl
        rol edx,16
        em 1,4

op55:   rol edx,8
        mov dl,dh
        ror edx,8
        em 1,4

op57:   rol edx,16
        mov dh,al
        rol edx,16
        em 1,4

op58:   rol edx,16
        mov dl,ch
        rol edx,16
        em 1,4

op59:   rol edx,16
        mov dl,cl
        rol edx,16
        em 1,4

op5c:   ror edx,8
        mov dh,dl
        rol edx,8
        em 1,4

op5d:   mov bl,dl
        rol edx,16
        mov dl,bl
        rol edx,16
        em 1,4

op5f:   rol edx,16
        mov dl,al
        rol edx,16
        em 1,4




;ld (b,c,d,e,h,l,a),(hl) : 7

op46:   mov bx,dx
        mov ch,[bx]
        xor bx,bx
        em 1,7

op4e:   mov bx,dx
        mov cl,[bx]
        xor bx,bx
        em 1,7

op56:   mov bx,dx
        rol edx,16
        mov dh,[bx]
        rol edx,16
        xor bx,bx
        em 1,7

op5e:   mov bx,dx
        rol edx,16
        mov dl,[bx]
        rol edx,16
        xor bx,bx
        em 1,7

op66:   mov bx,dx
        mov dh,[bx]
        xor bx,bx
        em 1,7

op6e:   mov bx,dx
        mov dl,[bx]
        xor bx,bx
        em 1,7

op7e:   mov bx,dx
        mov al,[bx]
        xor bx,bx
        em 1,7




;ld (hl),(b,c,d,e,h,l,a) : 7

op70:   mov bx,dx
        pokex bx,ch,op70x
        xor bx,bx
        em 1,7
op70x:  mov _specialdata,ch
        jmp opspec

op71:   mov bx,dx
        pokex bx,cl,op71x
        xor bx,bx
        em 1,7
op71x:  mov _specialdata,cl
        jmp opspec

op72:   mov bx,dx
        rol edx,16
        pokex bx,dh,op72x
        xor bx,bx
        rol edx,16
        em 1,7
op72x:  mov _specialdata,dh
        rol edx,16
        jmp opspec

op73:   mov bx,dx
        rol edx,16
        pokex bx,dl,op73x
        xor bx,bx
        rol edx,16
        em 1,7
op73x:  mov _specialdata,dl
        rol edx,16
        jmp opspec

op74:   mov bx,dx
        pokex bx,dh,op74x
        xor bx,bx
        em 1,7
op74x:  mov _specialdata,dh
        jmp opspec

op75:   mov bx,dx
        pokex bx,dl,op75x
        xor bx,bx
        em 1,7
op75x:  mov _specialdata,dl
        jmp opspec

op77:   mov bx,dx
        pokex bx,al,op77x
        xor bx,bx
        em 1,7
op77x:  mov _specialdata,al
        jmp opspec




;halt

op76:   mov ebx,ebp             ;The interrupt code takes care of DI/HALT
        shr ebx,18              ; checking
        add bp,bx
        and ebp,0300ffh
        xor ebx,ebx
        em 0,4






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

xorah macro                     ;Used only for AND
        xor ah,012h             ;Set H, reset N
        endm

andah macro
        and ah,0edh             ;reset N and H flags (but H is 0 after AND/OR)
        endm


endinstr macro reg
        ifidni <reg>,<[si+1]>   ;N
                em 2,7
        else
         ifidni <reg>,<bptrbx> ;(HL)
                em 1,11
         else
          ifidni <reg>,<ixl>
                em2 2,8
          else
           ifidni <reg>,<ixh>
                em2 2,8
           else
            ifidni <reg>,<iyl>
                em2 2,8
            else
             ifidni <reg>,<iyh>
                em2 2,8
             else
              ifidni <reg>,<ixplusd>
                em2 3,19
              else
               ifidni <reg>,<iyplusd>
                em2 3,19
               else
                em 1,4                          ;regular
               endif
              endif
             endif
            endif
           endif
          endif
         endif
        endif
        endm


makeop macro instr,reg,sahfinstr,flaginstr
        redtape reg             ;put DE in 8 bit regs, or put HL in BX, etc.
        sahfinstr               ;SAHF comes here if necessary
        ifidni <instr>,<cmp>
            instr al,reg        ;For CMP, inoff flags come from operand,
            setop reg           ; not from result.
        else
            instr al,reg
            setop al
        endif
        lahf                    ;Flags are saved
        flaginstr               ;H and N flags are set appr. if neccessary
        bluetape reg            ;Redtape action is undone
        endinstr reg            ;Appropriate EM macro is inserted
        endm


op80:   makeop add,ch,noop,ova
op81:   makeop add,cl,noop,ova
op82:   makeop add,edxh,noop,ova
op83:   makeop add,edxl,noop,ova
op84:   makeop add,dh,noop,ova
op85:   makeop add,dl,noop,ova
opdd84: makeop add,ixh,noop,ova
opdd85: makeop add,ixl,noop,ova
opfd84: makeop add,iyh,noop,ova
opfd85: makeop add,iyl,noop,ova
op86:   makeop add,bptrbx,noop,ova
opdd86: makeop add,ixplusd,noop,ova
opfd86: makeop add,iyplusd,noop,ova
op87:   makeop add,al,noop,ova
opc6:   makeop add,[si+1],noop,ova

op90:   makeop sub,ch,noop,ovs
op91:   makeop sub,cl,noop,ovs
op92:   makeop sub,edxh,noop,ovs
op93:   makeop sub,edxl,noop,ovs
op94:   makeop sub,dh,noop,ovs
op95:   makeop sub,dl,noop,ovs
opdd94: makeop sub,ixh,noop,ovs
opdd95: makeop sub,ixl,noop,ovs
opfd94: makeop sub,iyh,noop,ovs
opfd95: makeop sub,iyl,noop,ovs
op96:   makeop sub,bptrbx,noop,ovs
opdd96: makeop sub,ixplusd,noop,ovs
opfd96: makeop sub,iyplusd,noop,ovs
op97:   makeop sub,al,noop,ovs
opd6:   makeop sub,[si+1],noop,ovs

opa0:   makeop and,ch,noop,xorah
opa1:   makeop and,cl,noop,xorah
opa2:   makeop and,edxh,noop,xorah
opa3:   makeop and,edxl,noop,xorah
opa4:   makeop and,dh,noop,xorah
opa5:   makeop and,dl,noop,xorah
opdda4: makeop and,ixh,noop,xorah
opdda5: makeop and,ixl,noop,xorah
opfda4: makeop and,iyh,noop,xorah
opfda5: makeop and,iyl,noop,xorah
opa6:   makeop and,bptrbx,noop,xorah
opdda6: makeop and,ixplusd,noop,xorah
opfda6: makeop and,iyplusd,noop,xorah
opa7:   makeop and,al,noop,xorah
ope6:   makeop and,[si+1],noop,xorah

opb0:   makeop or,ch,noop,andah
opb1:   makeop or,cl,noop,andah
opb2:   makeop or,edxh,noop,andah
opb3:   makeop or,edxl,noop,andah
opb4:   makeop or,dh,noop,andah
opb5:   makeop or,dl,noop,andah
opddb4: makeop or,ixh,noop,andah
opddb5: makeop or,ixl,noop,andah
opfdb4: makeop or,iyh,noop,andah
opfdb5: makeop or,iyl,noop,andah
opb6:   makeop or,bptrbx,noop,andah
opddb6: makeop or,ixplusd,noop,andah
opfdb6: makeop or,iyplusd,noop,andah
opb7:   makeop or,al,noop,andah
opf6:   makeop or,[si+1],noop,andah

op88:   makeop adc,ch,sahf,ova
op89:   makeop adc,cl,sahf,ova
op8a:   makeop adc,edxh,sahf,ova
op8b:   makeop adc,edxl,sahf,ova
op8c:   makeop adc,dh,sahf,ova
op8d:   makeop adc,dl,sahf,ova
opdd8c: makeop adc,ixh,sahf,ova
opdd8d: makeop adc,ixl,sahf,ova
opfd8c: makeop adc,iyh,sahf,ova
opfd8d: makeop adc,iyl,sahf,ova
op8e:   makeop adc,bptrbx,sahf,ova
opdd8e: makeop adc,ixplusd,sahf,ova
opfd8e: makeop adc,iyplusd,sahf,ova
op8f:   makeop adc,al,sahf,ova
opce:   makeop adc,[si+1],sahf,ova

op98:   makeop sbb,ch,sahf,ovs
op99:   makeop sbb,cl,sahf,ovs
op9a:   makeop sbb,edxh,sahf,ovs
op9b:   makeop sbb,edxl,sahf,ovs
op9c:   makeop sbb,dh,sahf,ovs
op9d:   makeop sbb,dl,sahf,ovs
opdd9c: makeop sbb,ixh,sahf,ovs
opdd9d: makeop sbb,ixl,sahf,ovs
opfd9c: makeop sbb,iyh,sahf,ovs
opfd9d: makeop sbb,iyl,sahf,ovs
op9e:   makeop sbb,bptrbx,sahf,ovs
opdd9e: makeop sbb,ixplusd,sahf,ovs
opfd9e: makeop sbb,iyplusd,sahf,ovs
op9f:   makeop sbb,al,sahf,ovs
opde:   makeop sbb,[si+1],sahf,ovs

opa8:   makeop xor,ch,noop,andah
opa9:   makeop xor,cl,noop,andah
opaa:   makeop xor,edxh,noop,andah
opab:   makeop xor,edxl,noop,andah
opac:   makeop xor,dh,noop,andah
opad:   makeop xor,dl,noop,andah
opddac: makeop xor,ixh,noop,andah
opddad: makeop xor,ixl,noop,andah
opfdac: makeop xor,iyh,noop,andah
opfdad: makeop xor,iyl,noop,andah
opae:   makeop xor,bptrbx,noop,andah
opddae: makeop xor,ixplusd,noop,andah
opfdae: makeop xor,iyplusd,noop,andah
opaf:   makeop xor,al,noop,andah
opee:   makeop xor,[si+1],noop,andah

opb8:   makeop cmp,ch,noop,ovs
opb9:   makeop cmp,cl,noop,ovs
opba:   makeop cmp,edxh,noop,ovs
opbb:   makeop cmp,edxl,noop,ovs
opbc:   makeop cmp,dh,noop,ovs
opbd:   makeop cmp,dl,noop,ovs
opddbc: makeop cmp,ixh,noop,ovs
opddbd: makeop cmp,ixl,noop,ovs
opfdbc: makeop cmp,iyh,noop,ovs
opfdbd: makeop cmp,iyl,noop,ovs
opbe:   makeop cmp,bptrbx,noop,ovs
opddbe: makeop cmp,ixplusd,noop,ovs
opfdbe: makeop cmp,iyplusd,noop,ovs
;opbf:   makeop cmp,al,noop,ovs
opfe:   makeop cmp,[si+1],noop,ovs


opbf:                                   ;cp a
        setop al
        mov ah,042h
        em 1,4


purge makeop
purge xorah
purge andah
purge noop



;ret, ret cc

opc9:   cmp si,0700h
        jesh opc9_pagerom
        inc di
        jesh opc9_segviol
        mov si,[di-1]
        inc di
        em 0,10
opc9_segviol:
        mov bl,[di-1]
        mov bh,[di]
        mov si,bx
        xor bx,bx
        inc di
        em 0,10
opc9_pagerom:
        mov bx,msg_ret0700
        jmp emul_ret

opc0:   sahf
        jnesh exretcc
        cmp si,056bh
        jesh opc0_loadtrap
        em 1,5
opc0_loadtrap:
        mov bx,msg_loadtrap
        jmp emul_ret
exretcc:
        inc di
        jesh exretcc_segviol
        mov si,[di-1]
        inc di
        em 0,11
exretcc_segviol:
        mov bl,[di-1]
        mov bh,[di]
        mov si,bx
        xor bx,bx
        inc di
        em 0,11

opc8:   sahf
        je exretcc
        em 1,5

opd0:   sahf
        jnc exretcc
        em 1,5

opd8:   sahf
        jc exretcc
        em 1,5

ope0:   sahf
        jpo exretcc
        em 1,5

ope8:   sahf
        jpe exretcc
        em 1,5

opf0:   sahf
        jns exretcc               ;ret p
        em 1,5

opf8:   sahf
        js exretcc                ;ret m
        em 1,5





;pop bc/de/hl/af

opc1:   inc di
        jesh popbc_segviol
        mov cx,[di-1]
popbc1: inc di
        em 1,10
popbc_segviol:
        mov cl,[di-1]
        mov ch,[di]
        jsh popbc1

opd1:   rol edx,16
        inc di
        jesh popde_segviol
        mov dx,[di-1]
popde1: rol edx,16
        inc di
        em 1,10
popde_segviol:
        mov dl,[di-1]
        mov dh,[di]
        jsh popde1

ope1:   inc di
        jesh pophl_segviol
        mov dx,[di-1]
pophl1: inc di
        em 1,10
pophl_segviol:
        mov dl,[di-1]
        mov dh,[di]
        jsh pophl1

opf1:   inc di
        jesh popaf_segviol
        mov ax,[di-1]
popaf1: mov temp_f,al
        xchg al,ah
        inc di
        em 1,10
popaf_segviol:
        mov al,[di-1]
        mov ah,[di]
        jsh popaf1




;push bc/de/hl/af

opc5:   pushreg cx,ch,cl
        em 1,11

opd5:   rol edx,16
        pushreg dx,dh,dl
        rol edx,16
        em 1,11

ope5:   pushreg dx,dh,dl
        em 1,11

opf5:   xchg ah,al
        xor al,es:temp_f
        and al,0ffh-028h
        xor al,es:temp_f
pushaf_nochange:
        pushreg ax,ah,al
        xchg ah,al
        cmp si,038h
        je pushaf_038
        em 1,11
pushaf_038:
        inc si
        sub ebp,11*010000h-1
        mov bx,msg_rst38
        jmp emul_ret_eoi



;jp, jp cc

opc3:   getop si
        em 0,10

opc2:   sahf
        jnz opc3
        em 3,10

opca:   sahf
        jz opc3
        em 3,10

opd2:   sahf
        jnc opc3
        em 3,10

opda:   sahf
        jc opc3
        em 3,10

ope2:   sahf
        jpo opc3
        em 3,10

opea:   sahf
        jpe opc3
        em 3,10

opf2:   sahf
        jns opc3
        em 3,10

opfa:   sahf
        js opc3
        em 3,10




;call, call cc

opcd:   getop bx
        add si,3
        pushpc
        mov si,bx
        xor bx,bx
        em 0,17

opc4:   sahf
        jnz opcd
        em 3,10

opcc:   sahf
        jz opcd
        em 3,10

opd4:   sahf
        jnc opcd
        em 3,10

opdc:   sahf
        jc opcd
        em 3,10

ope4:   sahf
        jpo opcd
        em 3,10

opec:   sahf
        jpe opcd
        em 3,10

opf4:   sahf
        jns opcd
        em 3,10

opfc:   sahf
        js opcd
        em 3,10



;rst 00-38

opc7:   xor bx,bx
        jsh exrst

opcf:   mov bx,8
        jsh exrst

opd7:   mov bx,010h
        jsh exrst

opdf:   mov bx,018h
        jsh exrst

ope7:   mov bx,020h
        jsh exrst

opef:   mov bx,028h
        jsh exrst

opf7:   mov bx,030h
        jsh exrst

opff:   mov bx,038h
exrst:  inc si
        pushpc
        mov si,bx
        xor bx,bx
        em 0,11


;varia

opcb:   mov bl,[si+1]
        jmp es:[offset cbemultab+2*ebx]

oped:   mov bl,[si+1]
        jmp es:[offset edemultab+2*ebx]

opdd:   mov bl,[si+1]
        jmp es:[offset ddemultab+2*ebx]

opfd:   mov bl,[si+1]
        jmp es:[offset fdemultab+2*ebx]



;out (n),a

opd3:   mov bl,al
        rol ebx,16
        mov bl,[si+1]
        mov bh,al
        add si,2
        sub ebp,010000h*11-1
        jmp FAR PTR execout


;exx

opd9:   xchg cx,rbca
        rol edx,16
        xchg edx,dword ptr [offset rdea]
        rol edx,16
        em 1,4


;in a,(n)

opdb:   mov bl,[si+1]
        mov bh,al
        call FAR PTR execin
        mov al,bl
        em 2,12                         ;maybe put 13 here


;ex (sp),hl

ope3:   inc di
        cmp di,04001h
        jbsh exsphl_careful
        dec di
        xchg [di],dx
        em 1,19
exsphl_careful:
        testrom di,exsp_nothi
        xchg [di],dh
exsp_nothi:
        dec di
        testrom di,exsp_notlo
        xchg [di],dl
exsp_notlo:
        em 1,19


;jp (hl)

ope9:   mov si,dx
        em 0,4


;ex de,hl

opeb:   rol edx,16
        em 1,4


;di

opf3:   mov word ptr [offset iff],0
        cmp si,04d4h                    ;DI in SAVE routine in ROM
        jesh opf3_savetrap
        em 1,4
opf3_savetrap:
        mov bx,msg_savetrap
        jmp emul_ret


;ld sp,hl

opf9:   mov di,dx
        em 1,4


;ei

opfb:   mov word ptr [offset iff],-1
        em 1,4









; in (b,c,d,e,h,l,,a),(c)

oped40: mov bx,cx
        call FAR PTR execin
        setop bl
        mov ch,bl
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh             ;Reset N and H (sean, 23/9/98).
        em2 2,12

oped48: mov bx,cx
        call FAR PTR execin
        setop bl
        mov cl,bl
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12

oped50: mov bx,cx
        call FAR PTR execin
        setop bl
        rol edx,16
        mov dh,bl
        rol edx,16
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12

oped58: mov bx,cx
        call FAR PTR execin
        setop bl
        rol edx,16
        mov dl,bl
        rol edx,16
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12

oped60: mov bx,cx
        call FAR PTR execin
        setop bl
        mov dh,bl
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12

oped68: mov bx,cx
        call FAR PTR execin
        setop bl
        mov dl,bl
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12

oped70: mov bx,cx
        call FAR PTR execin
        setop bl
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12

oped78: mov bx,cx
        call FAR PTR execin
        setop bl
        mov al,bl
        test bl,bl
        ror ah,1
        lahf
        and ah,0edh
        em2 2,12


;out (c),(b,c,d,e,h,l,0,a)

oped41: mov bl,ch
        rol ebx,16
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped49: mov bl,cl
        rol ebx,16
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped51: mov ebx,edx
        ror ebx,8
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped59: mov ebx,edx
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped61: mov bl,dh
        rol ebx,16
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped69: mov bl,dl
        rol ebx,16
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped71: mov bl,0
        rol ebx,16
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout

oped79: mov bl,al
        rol ebx,16
        mov bx,cx
        add si,2
        sub ebp,010000h*12-2
        jmp FAR PTR execout


;sbc hl,bc/de/hl/sp

oped42: sahf
        sbb dx,cx
        setop dh
        ovs
        em2 2,15

oped52: mov ebx,edx
        rol ebx,16
        sahf
        sbb dx,bx
        setop dh
        ovs
        xor ebx,ebx
        em2 2,15

oped62: sahf
        sbb dx,dx
        setop dh
        ovs
        em2 2,15

oped72: sahf
        sbb dx,di
        setop dh
        ovs
        em2 2,15


;adc hl,bc/de/hl/sp

oped4a: sahf
        adc dx,cx
        setop dh
        ova
        em2 2,15

oped5a: mov ebx,edx
        rol ebx,16
        sahf
        adc dx,bx
        setop dh
        ova
        xor ebx,ebx
        em2 2,15

oped6a: sahf
        adc dx,dx
        setop dh
        ova
        em2 2,15

oped7a: sahf
        adc dx,di
        setop dh
        ova
        em2 2,15


;ld (nn),bc/de/hl/sp

oped43: get2op bx
        pokew bx,cx,ch,cl
        xor ebx,ebx
        em2 4,20

oped53: get2op bx
        rol edx,16
        pokew bx,dx,dh,dl
        rol edx,16
        xor ebx,ebx
        em2 4,20

oped63: get2op bx
        pokew bx,dx,dh,dl
        xor ebx,ebx
        em2 4,20

oped73: get2op bx
        xchg dx,di
        pokew bx,dx,dh,dl
        xchg dx,di
        xor ebx,ebx
        em2 4,20


;ld bc/de/hl/sp,(nn)

oped4b: get2op bx
        mov cx,[bx]
        xor bx,bx
        em2 4,20

oped5b: get2op bx
        rol edx,16
        mov dx,[bx]
        rol edx,16
        xor bx,bx
        em2 4,20

oped6b: get2op bx
        mov dx,[bx]
        xor bx,bx
        em2 4,20

oped7b: get2op bx
        mov di,[bx]
        xor bx,bx
        em2 4,20


;neg (ed44,4c,...,7c)

EDOPNG: neg al
        setop al
        ovs
        em2 2,8


;retn,reti (ed45,ed4d,...,ed7d)

EDRETN:
EDRETI:                                 ;23/9/98: Sean said it.
        mov bl,iff2
        mov iff,bl
        inc di
        jesh oped4d_segviol
        mov si,[di-1]
        inc di
        em2 0,14
oped4d_segviol:
        mov bl,[di-1]
        mov bh,[di]
        mov si,bx
        xor bx,bx
        inc di
        em2 0,14


;im 0, im 0', im 1, im 2

EDIMD0: mov imode,0
        em2 2,8

EDIMD1: mov imode,1
        em2 2,8

EDIMD2: mov imode,2
        em2 2,8


;ld i,a/ld r,a/ld a,i/ld a,r/rrd/rld/nop/nop

oped47: mov ri,al
        em2 2,9

oped4f: mov rr_bit7,al
        mov bl,al
        mov bp,bx
        em2 2,9

oped57: mov al,ri
        setop al
        test al,al
        ror ah,1
        lahf
        and ah,011000001b
        test iff2,0ffh
        setnz bl
        shl bl,2
        or ah,bl
        em2 2,9

oped5f: mov al,rr_bit7
        xor ax,bp
        and al,080h
        xor ax,bp
        setop al
        test al,al
        ror ah,1
        lahf
        and ah,011000001b
        test iff2,0ffh
        setnz bl
        shl bl,2
        or ah,bl
        em2 2,9

oped67: xchg dx,di
        mov bl,[di]
        mov bh,al
        xor al,bl
        and al,0f0h
        xor al,bl
        setop al
        ror ah,1
        lahf
        and ah,0edh
        shr bx,4
        poke di,bl
        xchg dx,di
        xor bx,bx
        em2 2,18

oped6f: xchg dx,di
        mov bl,[di]
        shl bx,4
        xor bl,al
        and bl,0f0h
        xor bl,al
        and al,0f0h
        or al,bh
        setop al
        ror ah,1
        lahf
        and ah,0edh
        poke di,bl
        xchg dx,di
        xor bx,bx
        em2 2,18

EDNOOP: em2 2,8


;ldi/ldd/ldir/lddr

opeda0: xchg edx,edi
        mov bl,[di]
        rol edi,16
        poke di,bl
        rol edi,16
        xchg edx,edi
        add edx,010001h
        dec cx
        setne bl
        and ah,011101001b
        shl bl,2
        or ah,bl
        em2 2,16


opeda8: xchg edx,edi
        mov bl,[di]
        rol edi,16
        poke di,bl
        rol edi,16
        xchg edx,edi
        sub edx,010001h
        dec cx
        setne bl
        and ah,011101001b
        shl bl,2
        or ah,bl
        em2 2,16


opedb0: xchg edx,edi
        mov bl,[di]
        rol edi,16
        poke di,bl
        rol edi,16
        xchg edx,edi
        add edx,010001h
        and ah,011101001b
        dec cx
        jesh opedb0_end
        or ah,4
        em2 0,21
opedb0_end:
        em2 2,16


opedb8: xchg edx,edi
        mov bl,[di]
        rol edi,16
        poke di,bl
        rol edi,16
        xchg edx,edi
        sub edx,010001h
        and ah,011101001b
        dec cx
        jesh opedb8_end
        or ah,4
        em2 0,21
opedb8_end:
        em2 2,16


;cpi/cpd/cpir/cpdr

opeda1: mov bx,dx
        cmp al,[bx]
        ror ah,1
        lahf
        and ah,011111011b
        inc dx
        dec cx
        setne bl
        rol bl,2
        or ah,bl
        xor bx,bx
        em2 2,16


opeda9: mov bx,dx
        cmp al,[bx]
        ror ah,1
        lahf
        and ah,011111011b
        dec dx
        dec cx
        setne bl
        rol bl,2
        or ah,bl
        xor bx,bx
        em2 2,16


opedb1: mov bx,dx
        cmp al,[bx]
        ror ah,1
        lahf
        inc dx
        dec cx
        jesh opedb1_end
        sahf
        jesh opedb1_end
        xor bx,bx
        em2 0,21                 ;if an interrupt occurs, wrong flags are seen
opedb1_end:
        and ah,011111011b
        or ah,2
        test cx,cx
        setne bl
        rol bl,2
        or ah,bl
        xor bx,bx
        em2 2,16


opedb9: mov bx,dx
        cmp al,[bx]
        ror ah,1
        lahf
        dec dx
        dec cx
        jesh opedb9_end
        sahf
        jesh opedb9_end
        xor bx,bx
        em2 0,21                 ;if an interrupt occurs, wrong flags are seen
opedb9_end:
        and ah,011111011b
        or ah,2
        test cx,cx
        setne bl
        rol bl,2
        or ah,bl
        xor bx,bx
        em2 2,16


;ini/inir/ind/indr

opeda2: mov bx,cx
        call FAR PTR execin
        xchg dx,di
        poke di,bl
        xchg dx,di
        inc dx
        dec ch
        lahf
        em2 2,16


opedaa: mov bx,cx
        call FAR PTR execin
        xchg dx,di
        poke di,bl
        xchg dx,di
        dec dx
        dec ch
        lahf
        em2 2,16


opedb2: mov bx,cx
        call FAR PTR execin
        xchg dx,di
        poke di,bl
        xchg dx,di
        inc dx
        dec ch
        jnesh opedb2_cont
        lahf
        em2 2,16
opedb2_cont:
        em2 0,21


opedba: mov bx,cx
        call FAR PTR execin
        xchg dx,di
        poke di,bl
        xchg dx,di
        dec dx
        dec ch
        jnesh opedba_cont
        lahf
        em2 2,16
opedba_cont:
        em2 0,21


;outi/otir/outd/otdr

opeda3: mov bx,dx                       ;outi
        mov bl,[bx]
        rol ebx,16
        mov bx,cx
        inc dx
        dec ch
        lahf
        add si,2
        sub ebp,010000h*16-2
        jmp FAR PTR execout

opedab: mov bx,dx                       ;outd
        mov bl,[bx]
        rol ebx,16
        mov bx,cx
        dec dx
        dec ch
        lahf
        add si,2
        sub ebp,010000h*16-2
        jmp FAR PTR execout

opedb3: mov bx,dx                       ;otir
        mov bl,[bx]
        rol ebx,16
        mov bx,cx
        inc dx
        dec ch
        jnesh opedb8_cont
        lahf
        add si,2
        sub ebp,010000h*16-2
        jmp FAR PTR execout
opedb8_cont:
        sub ebp,010000h*21-2
        jmp FAR PTR execout

opedbb: mov bx,dx                       ;otdr
        mov bl,[bx]
        rol ebx,16
        mov bx,cx
        dec dx
        dec ch
        jnesh opedbb_cont
        lahf
        add si,2
        sub ebp,010000h*16-2
        jmp FAR PTR execout
opedbb_cont:
        sub ebp,010000h*21-2
        jmp FAR PTR execout



opedfe:
        mov bx,msg_edfe
        jmp FAR PTR emul_ret

opedfb:
        mov bx,msg_edfb
        jmp FAR PTR emul_ret

opedfa:
        mov bx,msg_edfa
        jmp FAR PTR emul_ret

opedf9:
        mov bx,msg_edf9
        jmp FAR PTR emul_ret




CORE ends






SPECDATA segment dword public 'DATA'

emultab dw op00,op01,op02,op03,op04,op05,op06,op07
        dw op08,op09,op0a,op0b,op0c,op0d,op0e,op0f
        dw op10,op11,op12,op13,op14,op15,op16,op17
        dw op18,op19,op1a,op1b,op1c,op1d,op1e,op1f
        dw op20,op21,op22,op23,op24,op25,op26,op27
        dw op28,op29,op2a,op2b,op2c,op2d,op2e,op2f
        dw op30,op31,op32,op33,op34,op35,op36,op37
        dw op38,op39,op3a,op3b,op3c,op3d,op3e,op3f
        dw op40,op41,op42,op43,op44,op45,op46,op47
        dw op48,op49,op4a,op4b,op4c,op4d,op4e,op4f
        dw op50,op51,op52,op53,op54,op55,op56,op57
        dw op58,op59,op5a,op5b,op5c,op5d,op5e,op5f
        dw op60,op61,op62,op63,op64,op65,op66,op67
        dw op68,op69,op6a,op6b,op6c,op6d,op6e,op6f
        dw op70,op71,op72,op73,op74,op75,op76,op77
        dw op78,op79,op7a,op7b,op7c,op7d,op7e,op7f
        dw op80,op81,op82,op83,op84,op85,op86,op87
        dw op88,op89,op8a,op8b,op8c,op8d,op8e,op8f
        dw op90,op91,op92,op93,op94,op95,op96,op97
        dw op98,op99,op9a,op9b,op9c,op9d,op9e,op9f
        dw opa0,opa1,opa2,opa3,opa4,opa5,opa6,opa7
        dw opa8,opa9,opaa,opab,opac,opad,opae,opaf
        dw opb0,opb1,opb2,opb3,opb4,opb5,opb6,opb7
        dw opb8,opb9,opba,opbb,opbc,opbd,opbe,opbf
        dw opc0,opc1,opc2,opc3,opc4,opc5,opc6,opc7
        dw opc8,opc9,opca,opcb,opcc,opcd,opce,opcf
        dw opd0,opd1,opd2,opd3,opd4,opd5,opd6,opd7
        dw opd8,opd9,opda,opdb,opdc,opdd,opde,opdf
        dw ope0,ope1,ope2,ope3,ope4,ope5,ope6,ope7
        dw ope8,ope9,opea,opeb,opec,oped,opee,opef
        dw opf0,opf1,opf2,opf3,opf4,opf5,opf6,opf7
        dw opf8,opf9,opfa,opfb,opfc,opfd,opfe,opff



edemultab dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw oped40,oped41,oped42,oped43,EDOPNG,EDRETN,EDIMD0,oped47
        dw oped48,oped49,oped4a,oped4b,EDOPNG,EDRETI,EDIMD0,oped4f
        dw oped50,oped51,oped52,oped53,EDOPNG,EDRETN,EDIMD1,oped57
        dw oped58,oped59,oped5a,oped5b,EDOPNG,EDRETI,EDIMD2,oped5f
        dw oped60,oped61,oped62,oped63,EDOPNG,EDRETN,EDIMD0,oped67
        dw oped68,oped69,oped6a,oped6b,EDOPNG,EDRETI,EDIMD0,oped6f
        dw oped70,oped71,oped72,oped73,EDOPNG,EDRETN,EDIMD1,EDNOOP
        dw oped78,oped79,oped7a,oped7b,EDOPNG,EDRETI,EDIMD2,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw opeda0,opeda1,opeda2,opeda3,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw opeda8,opeda9,opedaa,opedab,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw opedb0,opedb1,opedb2,opedb3,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw opedb8,opedb9,opedba,opedbb,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP,EDNOOP
        dw EDNOOP,opedf9,opedfa,opedfb,EDNOOP,EDNOOP,opedfe,EDNOOP




SPECDATA ends


end



