include c:/bc4/spectrum/asm/macro.asm

;
;Code for the Z80 debugger
;
;Routines are built around a 'traplist', containing a list of record
;of the form:
;
;traplist struct {
;           BYTE type;     // byte code, see below.  0=end-of-list
;           BYTE and;      // TRUE if current result must be combined with next
;                          // Hi bits contain = <> <= >= 'comparison type'
;           WORD handler;  // address of handler for this type
;           DWORD par1;    // first parameter; AND mask, memory address in hi part
;           DWORD par2;    // second parameter; value against which previous is matched
;                          //  Also contains comparison type handler address in hi part
;         }
;




SPECDATA segment dword public 'DATA'

_traplist       dd 0            ;list of breakpoints / traps
_trapoffset     dw ?            ;holds tripping trap when debugger trips

public _traplist
public _trapoffset

SPECDATA ends





CORE segment byte use16 public 'CODE'

ASSUME ds:SPECDATA, es:NOTHING

_install_traplist:
global _install_traplist: proc
        push bp
        mov bp,sp
        push es
        push si
        les si,_traplist
        cmp byte ptr es:[si],0  ;first is END, i.e. none in list?
        je _inst_tl_none
_inst_tl_loop:
        mov bl,es:[si]
        and bl,03fh             ;mask out comparison type bits
        cmp bl,16
        ja _inst_tl_none        ;error
        xor bh,bh
        add bx,bx
        mov bx,cs:word ptr [offset handlers+bx]
        mov es:[si+2],bx        ;store address of handler
        mov bl,es:[si]          ;get byte again
        cmp bl,2                ;Do not store comparison handler for Opcode breakpoint
        je _inst_tl_nocomp
        and bx,0c0h
        shr bx,5
        mov bx,cs:word ptr [offset comphandlers+bx]
        mov [es:si+10],bx       ;store comparison handler
_inst_tl_nocomp:
        add si,12               ;next one
        cmp byte ptr [es:si-12],0       ;previous one was 'end'?
        jne _inst_tl_loop
        and byte ptr [es:si-11],0feh    ;set 'AND' flag of last one to FALSE, just to be sure
        mov _debugging,1        ;call to _installsettings necc. to activate things
_inst_tl_end:
        pop si
        pop es
        pop bp
        retf

_inst_tl_none:                  ;nothing in list
        mov _debugging,0
        jmp _inst_tl_end

handlers:       dw hEnd         ;0
                dw hBreakpoint  ;1      PC=(WORD)par1
                dw hOpcode      ;2      *PC & par1 == par2
                dw hMemory      ;3      *(WORD)(par1>>16) & (par1) =?= (WORD)par2
                dw hAF          ;4      AF & par1 =?= (WORD)par2
                dw hBC          ;5      BC & par1 =?= (WORD)par2
                dw hDE          ;6
                dw hHL          ;7
                dw hAFa         ;8
                dw hBCa         ;9
                dw hDEa         ;10
                dw hHLa         ;11
                dw hIX          ;12
                dw hIY          ;13
                dw hPC          ;14
                dw hSP          ;15
                dw hIR          ;16

comphandlers:   dw hEqual
                dw hUnequal
                dw hLess
                dw hLarger

;***** Actual debug handler, called from wrap_debugmode in core.asm *******

ASSUME es:SPECDATA, ds:NOTHING

exec_debug:
        lfs bp,_traplist        ;get address of first trap.  Note: hi part != 0
        jmp word ptr [fs:bp+2]  ;jump to first trap


hEnd:                           ;code 0.  Nothing happened.
        ret


exec_dbg_trip:
        sub bp,word ptr [offset _traplist]      ;offset into trap table
        mov _trapoffset,bp
        pop bx                                  ;drop return address
        mov bx,msg_trip
        jmp emul_return                 ;allowed as [rreg] has been updated




ResultTrue macro                ;Result evaluated to TRUE: see if we should trip
        test byte ptr fs:[bp+1],01      ;This one was last one?
        je exec_dbg_trip              ;Then trip
        add bp,12
        jmp word ptr [fs:bp+2]
        endm


;******* Actual handlers ********

hBreakpoint:                    ;code 1
        cmp si,fs:[bp+4]
        jnesh hNext
        ResultTrue


hOpcode:                        ;code 2
;        mov bx,[si+2]
;        shl ebx,2
;        mov bx,[si]
        mov ebx,[si]
;        movzx ebp,bp            ;clear out hi part
        and ebx,fs:[bp+4]       ;was: ebp
        cmp ebx,fs:[bp+8]       ;was: ebp
        jnesh hNext
        ResultTrue


hNext:                            ;result was False, so skip until next AND train
        add bp,12
        test byte ptr fs:[bp-11],1      ;Previous one was last one?
        jnesh hNext                     ;Continue if not
        jmp word ptr [fs:bp+2]          ;jump to next trap


hMemory:                        ;code 3
        mov bx,[fs:bp+6]        ;get address
        mov bx,[bx]             ;get value
        jmp word ptr fs:[bp+10] ;jump to comparison handler

hEqual:
        and bx,[fs:bp+4]
        cmp bx,[fs:bp+8]
        jnesh hNext
        ResultTrue

hUnequal:
        and bx,[fs:bp+4]
        cmp bx,[fs:bp+8]
        jesh hNext
        ResultTrue

hLess:
        and bx,[fs:bp+4]
        cmp bx,[fs:bp+8]
        ja short hNext
        ResultTrue

hLarger:
        and bx,[fs:bp+4]
        cmp bx,[fs:bp+8]
        jbsh hNext
        ResultTrue


hAF:                            ;code 4
        mov bx,ax
        xchg bh,bl
        jmp word ptr [fs:bp+10]


hBC:                            ;code 5
        mov bx,cx
        jmp word ptr [fs:bp+10]


hDE:                            ;code 6
        mov ebx,edx
        shr ebx,16
        jmp word ptr [fs:bp+10]


hHL:                            ;code 7
        mov bx,dx
        jmp word ptr [fs:bp+10]


hAFa:                           ;code 8
        mov bx,rfaa
        xchg bh,bl
        jmp word ptr [fs:bp+10]


hBCa:                           ;code 9
        mov bx,rbca
        jmp word ptr [fs:bp+10]


hDEa:                           ;code 10
        mov bx,rdea
        jmp word ptr [fs:bp+10]


hHLa:                           ;code 11
        mov bx,rhla
        jmp word ptr [fs:bp+10]


hIX:                            ;code 12
        mov bx,rix
        jmp word ptr [fs:bp+10]


hIY:                            ;code 13
        mov bx,riy
        jmp word ptr [fs:bp+10]


hPC:                            ;code 14
        mov bx,si
        jmp word ptr [fs:bp+10]


hSP:                            ;code 15
        mov bx,di
        jmp word ptr [fs:bp+10]


hIR:                            ;code 16
        mov bh,ri
        mov bl,byte ptr [offset rreg]
        jmp word ptr [fs:bp+10]



CORE ends


end


