.486p


msg_dihalt      equ 1
;       start of instr; HALT; interrupt occ'ed with DI with HALT being exec'd
msg_twrap       equ 2
;       start of instr; any; global t counter wrapped
msg_grabok      equ 3
;       start of instr; any; coppered screen ready to be shown
msg_logbuffull  equ 4
;       start of instr; any; log buf filled just now
;       end of instr; core_end_of_instr; out etc
;        This one is actually at the very end of an instruction; start of next one
;        Or: generated when logging OUTs, and then it is at the end of an instr
msg_halfscreen  equ 5
;       start of instr; any; halfway into screen
msg_emulin      equ 6
;       halfway instr: core_cont_inning; in a,(nn)/in r,(c)/in(id)[r]; expects _isresult
;        and _inresult to be set by C; _isresult==2 means let bus float
msg_outbufextend equ 7
;       end of instr: core_end_of_instr; out #fe & need more space in border buffer
msg_sampleblk   equ 8
;       end of instr: core_end_of_instr; out #fe & sample block ready to be played
;       start of instr; any; time elapsed and corresp block now ready to be played
msg_inbufempty  equ 9
;       start of instr; IN with nothing in IN buffer; nothing done
msg_emulout     equ 10
;       end of instr: core_end_of_instr; out etc; yields addr and value in outdata
msg_loadtrap    equ 11
;       start of instr; ret nz at 056b with Z=1; nothing done
msg_getwarmodedata equ 12
;       start of instr; IN in warajevo mode with no data in war'o buffer; nothing done
msg_savetrap    equ 13
;       start of instr; di at 04d4; nothing done
msg_mirbuffull  equ 14
;       start of instr; rl/rr byte in tape mirror mode && buf full; nothing done
msg_edfb        equ 15
;       start of instr; ed fb; nothing done
msg_rst08       equ 16
;       start of instr; ld hl,(nn) at 0008; nothing done
msg_ret0700     equ 17
;       start of instr; ret at 0700; nothing done
msg_edf9        equ 18
;       start of instr; ed f9; nothing done
msg_edfa        equ 19
;       start of instr; ed fa; nothing done
msg_edfe        equ 20
;       start of instr; ed ff; nothing done
msg_memspecial  equ 21
;       end of instr; some 8-bit writes; write into special mem
msg_rst38       equ 22
;       end of instr; push af at address #38; exec'd & ready to switch currah rom
msg_50hz        equ 23
;       start of instr; before 'interrupt' instr, even when DI-ed; nothing done
msg_sampleblk50hz       equ 24
;       start of instr; any; time elapsed and corresp block now ready to be played
;       Also notifies 50hz tick
msg_trip        equ 25
;       end of instr; debug trap tripped; instr exec'd & ready to exec next



assume cs:CORE,es:SPECDATA



edxl equ dl                     ;used to distinguish between D and H, E and L
edxh equ dh

ixh  equ bh
ixl  equ bl
iyh  equ bh
iyl  equ bl

ixplusd equ byte ptr [bx]
iyplusd equ byte ptr [bx]

bptrbx equ byte ptr [bx]

bptrdi equ byte ptr [di]


global _KeyMap: byte
global _hmode: byte


SPECDATA segment dword public 'DATA'
global ri: byte
global rr: byte
global rix: word
global riy: word
global rfaa: word
global rbca: word
global rdea: word
global rhla: word
global rr_bit7: byte
global temp_f: byte
global temp_fa: byte
global romwrhi: byte
global romwrlo: byte
global iff: byte
global iff2: byte
global imode: byte
global emultab: word
global cbemultab: word
global edemultab: word
global fdemultab: word
global ddemultab: word
global xdcbemultab: word

global rreg: word
global tbase: dword
global floatrnd: word
global scradrbufptr: word
global corestart: word
global singleinstr: word
global _debugging: word

global tim1: dword
global tim192: dword
global timupper: dword
global timquart: dword

global byte13: byte
global byte16: byte

global _issue2: byte
global _copper: word
global _tbasehi: dword
global _tframe: dword
global _border: word

global _sound: word
global _TStatesPerSample: word
global _soundsilent: word
global _soundsilentq: word
global _soundtimehi: dword
global _soundtimelo: dword
global _soundnooutyet: word
global _soundlastval: dword
global _soundbufptr: dword
global _soundcurblk: word
global _soundregs: byte
global _fffdstate: word
global _BytesPerBlock: dword

global storeborouts: byte
global _outbufptr: dword
global _outbufoffset: word

global _logbufptr: dword
global _logbuflen: word

global _iicounter: word
global _iimode: word

global _miractive: word
global _mirinned: word
global _mirbyte: word
global _mircurbit: word
global _mirbitcount: dword
global _mirbufptr: dword
global _mirbuflen: word
global _miridle: word

global cbemul1020: word
global xdcbemul1020: word

global _isresult: byte
global _inresult: byte
global inreturn: word

global _specialdata: byte
global _specialaddr: word

SPECDATA ends



CORE segment byte use16 public 'CODE'
global wrapped: proc
global emulshortfar_label: proc
global emul_return: proc
global emul_ret: proc
global emul_ret_eoi: proc
global core_start_of_opcode: proc
global core_cont_inning: proc
global _activaterlrr: proc
global _deactivaterlrr: proc
global opactiverlrr: proc
global opactivexdrlrr: proc
global debugg: proc
global exec_debug: proc

global opdd84: proc
global opdd85: proc
global opdd86: proc
global opdd94: proc
global opdd95: proc
global opdd96: proc
global opdda4: proc
global opdda5: proc
global opdda6: proc
global opddb4: proc
global opddb5: proc
global opddb6: proc
global opdd8c: proc
global opdd8d: proc
global opdd8e: proc
global opdd9c: proc
global opdd9d: proc
global opdd9e: proc
global opddac: proc
global opddad: proc
global opddae: proc
global opddbc: proc
global opddbd: proc
global opddbe: proc

global opfd84: proc
global opfd85: proc
global opfd86: proc
global opfd94: proc
global opfd95: proc
global opfd96: proc
global opfda4: proc
global opfda5: proc
global opfda6: proc
global opfdb4: proc
global opfdb5: proc
global opfdb6: proc
global opfd8c: proc
global opfd8d: proc
global opfd8e: proc
global opfd9c: proc
global opfd9d: proc
global opfd9e: proc
global opfdac: proc
global opfdad: proc
global opfdae: proc
global opfdbc: proc
global opfdbd: proc
global opfdbe: proc

CORE ends



INOUT segment byte use16 public 'CODE'
global execout: proc
global execin: proc
global updatesound_f: proc
INOUT ends



SPECTRUM_TEXT segment byte use16 public 'CODE'
global _WRITEDEBUG: proc
SPECTRUM_TEXT ends



emulshort macro
        mov bl,[si]
        jmp word ptr es:[offset emultab+2*ebx]
        endm


;;debug emulshort macro:
;emulshort macro
;       jmp debugg
;       endm


emulshortfar macro
        jmp FAR PTR emulshortfar_label
        endm



em macro len,tim
        if len eq 0
        else
            if len eq 1
                inc si                  ;Polite towards 386 users
            else
                add si,len
            endif
        endif
        sub ebp,tim*010000h - 1         ;Update timer and R register
        jb wrapped                      ;Near long jump, possible on 386+
        emulshort                       ;If all OK, just jump to the next instr
        endm



em2 macro len,tim                       ;Macro for CBxx and EDxx instruction end
        if len eq 0
        else
            if len eq 1
                inc si                  ;Polite towards 386 users
            else
                add si,len
            endif
        endif
        sub ebp,tim*010000h - 2         ;Update timer and R register
        jb wrapped                      ;Near long jump, possible on 386+
        emulshort                       ;If all OK, just jump to the next instr
        endm



em4 macro len,tim                       ;Macro for DEC BC/LD A,B/OR C/JR NZ instruction end
        add si,len
        sub ebp,tim*010000h - 4         ;Update timer and R register
        jb wrapped                      ;Near long jump, possible on 386+
        emulshort                       ;If all OK, just jump to the next instr
        endm



emc macro len,time
        xor ebx,ebx
        em len,time
        endm


jsh equ jmp short
jesh equ je short
jbesh equ jbe short
jbsh equ jb short
jncsh equ jnc short
jnesh equ jne short
jaesh equ jae short




getinbx macro reg
        ifidni <reg>,<hl>
                mov bx,dx
        endif
        ifidni <reg>,<bc>
                mov bx,cx
        endif
        ifidni <reg>,<de>
                mov ebx,edx
                shr ebx,16
        endif
        ifidni <reg>,<sp>
                mov bx,di
        endif
        endm




testrom macro addr,label_nowrite        ;falls through if [addr] is ram,
        local tr_hi,tr_end              ;jumps to label_nowrite if not, and
        cmp addr,04000h                 ; there c=rom, nc=special
        jaesh tr_end
        cmp addr,02000h
        jaesh tr_hi
        cmp romwrlo,1
        jnesh label_nowrite
        jsh tr_end
tr_hi:
        cmp romwrhi,1
        jnesh label_nowrite
tr_end:
        endm




pushreg macro pair,hireg,loreg
        local ppc_careful,ppc_end
        local ppc_hibankhi,ppc_pokehi,ppc_nopokehi
        local ppc_hibanklo,ppc_pokelo,ppc_nopokelo

;note: pushreg pays no heed to 'special' memory (romwr(hi/lo)=2)

        dec di                  ;dec sp
        cmp di,04000h
        jbesh ppc_careful
        dec di
        mov [di],pair
        jsh ppc_end
ppc_careful:
        ifidni <pair>,<si>
                xchg bx,si
        endif
        jesh ppc_pokehi
        cmp di,02000h
        jaesh ppc_hibankhi
        cmp romwrlo,1
        jnesh ppc_nopokehi
        jsh ppc_pokehi
ppc_hibankhi:
        cmp romwrhi,1
        jnesh ppc_nopokehi
ppc_pokehi:
        mov [di],hireg
ppc_nopokehi:
        dec di
        cmp di,04000h
        jaesh ppc_pokelo
        cmp di,02000h
        jaesh ppc_hibanklo
        cmp romwrlo,1
        jnesh ppc_nopokelo
        jsh ppc_pokelo
ppc_hibanklo:
        cmp romwrhi,1
        jnesh ppc_nopokelo
ppc_pokelo:
        mov [di],loreg
ppc_nopokelo:
        ifidni <pair>,<si>
                xchg bx,si
        endif
ppc_end:
        endm




pushpc macro
        pushreg si,bh,bl
        endm




poke macro addr,value
        local pk_careful,pk_pokelo,pk_end

;note: poke pays no heed to 'special' ram (romwr(lo/hi)=2)

        cmp addr,04000h
        jbsh pk_careful
        mov [addr],value
        jsh pk_end
pk_careful:
        cmp addr,02000h
        jbsh pk_pokelo
        cmp romwrhi,1
        jnesh pk_end
        mov [addr],value
        jsh pk_end
pk_pokelo:
        cmp romwrlo,1
        jnesh pk_end
        mov [addr],value
pk_end:
        endm



pokex macro addr,value,xlabel
        local pk_careful,pk_pokelo,pk_end,pk_end2

;note: pokex DOES pay heed to 'special' ram (romwr(lo/hi)=2), by jumping
; to xlabel without actually poking the value.

        cmp addr,04000h
        jbsh pk_careful
        mov [addr],value
        jsh pk_end2
pk_careful:
        cmp addr,02000h
        jbsh pk_pokelo
        cmp romwrhi,1
        jnesh pk_end
        mov [addr],value
        jsh pk_end2
pk_pokelo:
        cmp romwrlo,1
        jnesh pk_end
        mov [addr],value
pk_end:
        jnc xlabel              ;romwr(hi/lo)=2 implies nc here
pk_end2:
        endm




pokew macro addr,value,vh,vl            ;leaves addr undefined!
        local pw_careful,pw_end
        local pw_hibankhi,pw_pokehi,pw_nopokehi
        local pw_hibanklo,pw_pokelo,pw_nopokelo

;note: pokew pays no heed to 'special' ram (romwr(lo/hi)=2)

        inc addr
        cmp addr,04000h
        jbesh pw_careful
        mov [addr-1],value
        jsh pw_end
pw_careful:
        jesh pw_pokehi
        cmp addr,02000h
        jaesh pw_hibankhi
        cmp romwrlo,1
        jnesh pw_nopokehi
        jsh pw_pokehi
pw_hibankhi:
        cmp romwrhi,1
        jnesh pw_nopokehi
pw_pokehi:
        mov [addr],vh
pw_nopokehi:
        dec addr
        cmp addr,04000h
        jaesh pw_pokelo
        cmp addr,02000h
        jaesh pw_hibanklo
        cmp romwrlo,1
        jnesh pw_nopokelo
        jsh pw_pokelo
pw_hibanklo:
        cmp romwrhi,1
        jnesh pw_nopokelo
pw_pokelo:
        mov [addr],vl
pw_nopokelo:
pw_end:
        endm



getop macro reg
        mov reg,[si+1]          ;to facilitate change to mov bl,mov bh
        endm


get2op macro reg
        mov reg,[si+2]
        endm


noop macro                      ;assembler-time no-operation
        endm


ova macro
        lahf
        seto bl
        and ah,011111001b       ;reset add/sub flag (1), and P/O (2)
        shl bl,2
        or ah,bl
        endm

ovs macro
        lahf
        seto bl
        and ah,011111011b       ;leave add/sub flag 1 (1), reset P/O (2)
        shl bl,2
        or ah,bl
        endm



setop macro oper                ;set operand: stores value of last 'operand'
        ifidni <oper>,<bptrbx>  ;as seen by the ALU.  Used to compute the
          mov bl,[bx]           ;inoff flag bits for PUSH AF.
          mov es:temp_f,bl
        else
          ifidni <oper>,<ixplusd>
            mov bl,[bx]
            mov es:temp_f,bl
          else
            ifidni <oper>,<iyplusd>
              mov bl,[bx]
              mov es:temp_f,bl
            else
              ifidni <oper>,<[si+1]>
                mov bl,[si+1]
                mov es:temp_f,bl
              else
                ifidni <oper>,<bptrdi>
                  mov bl,[di]
                  mov es:temp_f,bl
                else
                  ifidni <oper>,<ixh>
                    mov bl,[offset rix+1]
                    mov es:temp_f,bl
                  else
                    ifidni <oper>,<ixl>
                      mov bl,[offset rix]
                      mov es:temp_f,bl
                    else
                      ifidni <oper>,<iyh>
                        mov bl,[offset riy+1]
                        mov es:temp_f,bl
                      else
                        ifidni <oper>,<iyl>
                          mov bl,[offset riy]
                          mov es:temp_f,bl
                        else
                          mov es:temp_f,oper
                        endif
                      endif
                    endif
                  endif
                endif
              endif
            endif
          endif
        endif
        endm

computeloctime macro
        mov eax,ebp                  ;ebphi=ffff to ff00 to be regarded as
        add eax,01000000h            ; negative, rest as positive. Now all as
        shr eax,16                   ; positive, so shift logical right
        sub eax,0100h                ;Make ffff-ff00 into negative values again
        neg eax
        add eax,tbase
        add eax,_tframe              ;idiv doesn't work as I want it to
        xor edx,edx                  ; (negative remainders, pwea)
        div _tframe
        dec eax
        add eax,_tbasehi
        mov _soundtimehi,eax
        mov _soundtimelo,edx
        endm



