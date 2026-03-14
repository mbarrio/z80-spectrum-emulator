include c:/bc4/spectrum/asm/macro.asm




SPECDATA segment dword public 'DATA'

        db ?                    ;for alignment of important rde,r**a,ri* vars

_z80header      equ $           ;structure is defined in SPECTRUM.H

;Important differences: rr_bit7 has high bit of R in bit 7 instead of bit 0
; in the usual header.  Also, bit 0-6 are rubbish.
;Byte 'imode' contains only interrupt mode here; bits 2-7 have no meaning
;Program counter is never stored in rpc2
;

rfa     DW ?                    ;* AF register pair in reversed order
rbc     DW ?                    ;* BC register pair
rhl     DW ?                    ;* HL register pair
rpc     DW ?                    ;* Program counter
rsp     DW ?                    ;* SP
ri      DB ?                    ;I
rr      DB ?                    ;* R register
rr_bit7 DB ?                    ;Bit 7 contains high bit of R register
rde     DW ?                    ;* DE register pair
rbca    DW ?                    ;bc'
rdea    DW ?                    ;de'
rhla    DW ?                    ;hl'
rfaa    DW ?                    ;fa'
riy     DW ?                    ;iy
rix     DW ?                    ;ix
iff     DB ?                    ;Interrupt flip flop, 0=DI
iff2    DB ?                    ;Interrupt flip flop 2
imode   DB ?                    ;Interrupt mode

lngth           dw ?            ;
rpc2            dw ?            ;
hmode           db ?            ;
hstate          db ?            ;
if1paged        db ?            ;
flg2            db ?            ;
fffd            db ?            ;
sregs   db 16 dup (?)           ;
tstatelo        dw ?            ;
tstatehi        db ?            ;
specflg         db ?            ;
mgtpaged        db ?            ;
multipaged      db ?            ;
romwrlo         db ?            ;0 means 0-8191 rom, 1 means ram, 2 means special
romwrhi         db ?            ;0 means 8192-16383 rom, 1 means ram, 2=special
kbdmap          dw 5 dup (?)    ;
kbdasc          dw 5 dup (?)    ;
mgttype         db ?            ;
inhbutton       db ?            ;
inhflag         db ?            ;

                                ;Non-commented fields are not used during
                                ;execution of Z80 emulator.  Starred fields
                                ;are held in registers during execution.

        db 0,0,0                ;To make temp_f start at offset 88

temp_f  db ?
temp_fa db ?

        db ?

_reset   dw 0                   ;1 resets Spectrum
_nmi     dw 0                   ;1 generates nmi
_inter   dw 0                   ;1 generates interrupt
_anyinter dw 0                  ;1 to signal: one of the above
_copper  dw 0                   ;1 means start coppering mode (&vid buf OK)
_novideograb dw 0               ;1 means temporarily no coppering
_logging dw 0                   ;1 means build log file
_debugging dw 0                 ;1 means debugger on -- no log file!
singleinstr     dw 0            ;1 means execute single instrs (log,debug)
_hmode   db 0                   ;hardware mode (used here for timing)


;T state counters
;Tbase is an up-counter
;tglobal and tlocal are counted down.  One being zero means that
; it will generate the next event when BPhi wraps.  tglobal handles
; debugging and periodic interruption by main program; tlocal handles
; screen fetch and interrupts
;timer keeps value of BPhi during IN/OUT/DIHALT handling, and small
; rest time during normal operation

tbase   dd 0                    ;Actual time-after-irupt is tbase - BPhi
_tglobal        equ $
tglobal dd 0                    ;Time to global event is tglobal + BPhi
tlocal  dw 0                    ;Time to local event is tlocal + BPhi
        dw ?                    ;(Align to dword boundary)
_rreg           equ $
rreg    dw 0                    ;BPlo
_coretimer      equ $
timer   dw 0                    ;BPhi
_tbasehi dd 0                    ;20ms counter; used for sound.
_tframe  dd 69888                ;time of 20ms frame

locmode dw offset slice1        ;holds address of handling routine
loccnt  dw 0                    ;counter for screen fetch in coppering mode

wrapmod dw offset wrap_normalmode    ;holds address of BPhi wrap handling routine

corestart dw ?                  ;'state of emulated z80' (see rems in core code)

_border dw 0                    ;last out to FE; border colour
_flashcnt dw ?                  ;counter for flash (mod 32)
floatrnd dw 0                   ;pseudo RND seed for floating bus emulation

_videopage7     dw ?            ;1 if in 128k mode and #7ffd & 8
_page7seg       dw ?            ;points to page 7 in 128k mode if #7ffd & 8
_page7ptr       dd ?
_vidbufbase     dd ?            ;base of 12288 byte vidbuf used in copp mode
scradrbuf       dw 3*192 dup (?)        ;192 offsets
scradrbufptr    dw ?            ;current pointer into offsets

byte13          db 13           ;used by DJNZ instruction
byte16          db 16           ;used by JR NZ instruction

activerlrr      dw 0            ;used by (de)activaterlrr subroutines

_specialdata    db 0            ;used to store data poked into special mem
_specialaddr    dw 0            ;used to store special mem addr

_fffdstate      dw 0            ;used by AY subroutines, updated by C
_soundregs      db 16 dup (?)

_vz80d_addr     dw 0            ;int 2f callback voor vz80d
_vz80d_segm     dw 0

tim56           dd 224*56
tim64           dd 224*64
tim192          dd 224*192
tim96           dd 224*96
tim33           dd 224*33
tim24           dd 224*24
tim1            dd 224
timupper        dd 64*224
timquart        dd 17472
timhmode        db 0

public  _z80header
public  _reset
public  _nmi
public  _inter
public  _anyinter
public  _tglobal
public  _rreg
public  _coretimer
public  _border
public  _copper
public  _flashcnt
public  _novideograb
public  _logging
;public  _logbuflen
;public  _logbufptr
public  _videopage7
public  _page7ptr
public  _page7seg
public  _vidbufbase
public  _tframe
public  _fffdstate
public  _soundregs
public  _vz80d_addr
public  _vz80d_segm

public scradrbuf                ;for vid.asm
public singleinstr              ;for inout.asm

SPECDATA ends

public slice3


CORE segment byte use16 public 'CODE'


;void init_vz80d_api_call(char *name)
_init_vz80d_api_call:
global _init_vz80d_api_call: proc
        push bp
        mov bp,sp
        push es
        push ds
        push di
        mov bx,27h              ;VXDLDR_DEVICE_ID
        mov ax,1684h            ;Get API entry point
        xor di,di
        mov es,di
        int 2fh
        mov ax,es
        or ax,di
        je ivac_error           ;Could not even find vxd-loader, so stop.
        push cs
        push offset ivac_loaded
        push es
        push di
        lds dx,[bp+6]
        mov ax,1                ;"load device"
        retf
ivac_loaded:                    ;carry = error, but ignore; could be "dev. already loaded"
        mov bx,3f12h            ;Identifier of VZ80D
        mov ax,1684h
        xor di,di
        mov es,di
        int 2fh
        mov ax,SPECDATA
        mov ds,ax
ivac_error:
        mov ds:_vz80d_addr,di
        mov ds:_vz80d_segm,es
        pop di
        pop ds
        pop es
        pop bp
        retf

call_vz80d:
        push cs
        push offset call_vz80d_ret
        push ds:_vz80d_segm
        push ds:_vz80d_addr
        retf

call_vz80d_ret:
        ret

;int vz80d_version(void)
_vz80d_version:
global _vz80d_version: proc
        mov ah,0
        call call_vz80d
        mov eax,ebx
        retf

;void vz80d_pokevalue(long handle, long offset, char value)
_vz80d_pokevalue:
global _vz80d_pokevalue: proc
        push bp
        mov bp,sp
        push es
        mov edi,[bp+6]
        mov ebx,[bp+10]
        mov al,[bp+14]
        mov ah,8
        call call_vz80d
        pop es
        pop bp
        retf

;char vz80d_peekvalue(long handle, long offset)
_vz80d_peekvalue:
global _vz80d_peekvalue: proc
        push bp
        mov bp,sp
        push es
        mov edi,[bp+6]
        mov ebx,[bp+10]
        mov ah,7
        call call_vz80d
        xor ah,ah
        pop es
        pop bp
        retf

;int vz80d_alloc(int banks, int pags, long* handle)
_vz80d_alloc:
global _vz80d_alloc: proc
        push bp
        mov bp,sp
        push si
        push di
        push es
        movzx ecx,word ptr [bp+6]
        movzx ebx,word ptr [bp+8]
        mov ah,1
        call call_vz80d
        jc vz80d_err
        les si,[bp+10]
        mov es:[si],edi
        xor ax,ax
        pop es
        pop di
        pop si
        pop bp
        retf

vz80d_err:
        mov al,ah
        xor ah,ah
        pop es
        pop di
        pop si
        pop bp
        retf

;;int vz80d_getbuf(long handle, void **buf)
;_vz80d_getbuf:
;global _vz80d_getbuf: proc
;        push bp
;        mov bp,sp
;        push si
;        push di
;        push es
;        mov edi,[bp+6]
;        mov ah,2
;        call call_vz80d
;        jc vz80d_err
;        push ds
;        lds si,[bp+10]
;        mov [si],dx
;        mov [si+2],es
;        pop ds
;        xor ax,ax
;        pop es
;        pop di
;        pop si
;        pop bp
;        retf


;int vz80d_getframe(long handle, void **buf)
_vz80d_getframe:
global _vz80d_getframe: proc
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov edi,[bp+6]
        mov ah,3
        call call_vz80d
        jc vz80d_err
        push ds
        lds si,[bp+10]
        mov [si],dx
        mov [si+2],es
        pop ds
        xor ax,ax
        pop es
        pop di
        pop si
        pop bp
        retf

;int vz80d_getpage(long handle, long page, void **buf)
_vz80d_getpage:
global _vz80d_getpage: proc
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov edi,[bp+6]
        mov ebx,[bp+10]
        mov ah,6
        call call_vz80d
        jc vz80d_err
        push ds
        lds si,[bp+14]
        mov [si],dx
        mov [si+2],es
        pop ds
        xor ax,ax
        pop es
        pop di
        pop si
        pop bp
        retf


;int vz80d_page(long handle, int bank, int pag)
_vz80d_page:
global _vz80d_page: proc
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov edi,[bp+6]
        movzx ecx,word ptr[bp+10]
        movzx ebx,word ptr[bp+12]
        mov ah,4
        call call_vz80d
        jc vz80d_err
        xor ax,ax
        pop es
        pop di
        pop si
        pop bp
        retf

;int vz80d_free(long handle)
_vz80d_free:
global _vz80d_free: proc
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov edi,[bp+6]
        mov ah,5
        call call_vz80d
        jc vz80d_err
        xor ax,ax
        pop es
        pop di
        pop si
        pop bp
        retf

;void unload_vz80d(void)
_unload_vz80d:
global _unload_vz80d: proc
        push es
        push di
        mov bx,27h              ;VXDLDR_DEVICE_ID
        mov ax,1684h
        xor di,di
        mov es,di
        int 2fh
        mov ax,es
        or ax,di
        je uv_error
        push cs
        push offset uv_finished
        push es
        push di
        mov ax,2                ;"unload VxD"
        mov bx,7fe1h
        retf
uv_finished:
uv_error:
        pop di
        pop es
        retf


debugg:
global debugg: proc
        push es
        pushad
        mov bl,[si]
        push bx
        push di
        push dx
        push ax
        push si
        call FAR PTR _WRITEDEBUG
        pop si
        pop ax
        pop dx
        pop di
        pop bx
        popad
        pop es
        mov bl,[si]
        jmp word ptr es:[offset emultab+2*ebx]


;void putrombyteshigh(byte far*)
_putrombyteshigh:
;
;tijdelijk er even uit, voor het testen van vz80d
;
global _putrombyteshigh: proc
        push bp
        mov bp,sp
        push es
        push di
        xor edi,edi
        les di,[bp+6]
        mov eax,es:[di]
;        mov es:[edi+010000h],eax
        pop di
        pop es
        pop bp
        retf



;void setz80time(dword time)
_setz80time:
global _setz80time: proc
        push bp
        mov bp,sp               ;bp -> bp,ret,ret,dwordlo,dwordhi
        push es
        mov ax,SPECDATA
        mov es,ax
        mov eax,[bp+6]
        mov tbase,0             ;reset internal T-after-irpt counter
        mov ebx,_tframe
        mov tglobal,ebx         ;emulate for 20 ms
        mov ebx,timupper
        mov tlocal,bx          ;64x224 before first local event
        mov timer,0             ;BPhi
        mov locmode,offset slice1
        pop es
        pop bp
        retf


;void SetTGlobal(dword time)
;// Sets number of Tstates to execute from current time.  This overwrites
;// any previous setting of _tglobal
_SetTGlobal:
global _SetTGlobal: proc
        push bp
        mov bp,sp
        push es
        mov ax,SPECDATA
        mov es,ax
        movsx ebx,word ptr [offset rreg+2]     ;get EBPhi (timer)
        mov eax,[bp+6]
        cmp eax,ebx             ;new tglobal large?  Then simply adjust tglobal
        jge _stg_big
        add tglobal,ebx
        add tlocal,bx
        sub tbase,ebx
        mov word ptr [offset rreg+2],0         ;reset EBPhi
        xor ebx,ebx                             ;and set tglobal
_stg_big:
        sub eax,ebx
        mov tglobal,eax
        pop es
        pop bp
        retf


;long getcurrenttime(void)      ;computes time in _soundtime(lo/hi), returns lo
_getcurrenttime:                ;also computes correct value of R register in rr
global _getcurrenttime: proc
        push es
        push ebp
        mov ax,SPECDATA
        mov es,ax
        mov ebp,dword ptr rreg
        mov ax,bp
        mov rr,al               ;store value of r register into header (excl. bit 7)
        computeloctime
        mov eax,_soundtimelo
        mov edx,eax
        shr edx,16
        pop ebp
        pop es
        retf                    ;that's all



;void pokebyte(byte far*,int,byte)
_pokebyte:
global _pokebyte: proc
        push bp
        mov bp,sp
        push ds
        push si
        push es
        mov ax,SPECDATA
        mov es,ax
        lds si,[bp+6]
        mov bx,[bp+10]
        add si,bx
        mov al,[bp+12]
        poke si,al           	;see macro.asm
        pop es
        pop si
        pop ds
        pop bp
        retf



;void installsettings(void)
_installsettings:
global _installsettings: proc
        push es
        push si
        push di
        mov ax,SPECDATA
        mov es,ax
        mov singleinstr,0       ;Signal: emulate normally
        mov corestart,offset core_start_of_opcode    ;reset corestart variable
        mov ax,offset wrap_normalmode
;
        ifndef logouts
;
        cmp _logging,0
        jesh is_normal
        mov ax,offset wrap_logmode
        mov singleinstr,1       ;Signal: emulate single instructions
is_normal:
;
        endif
;
        cmp _debugging,0
        jesh is_nodebug
        mov ax,offset wrap_debugmode
        mov singleinstr,1
is_nodebug:
        mov wrapmod,ax
        cmp _copper,0
        je is_nocopper
        jsh is_copper
is_nocopper:
        mov storeborouts,0      ;stop storing border outs
        cmp locmode,offset slice2       ;coppering NOW?
        jne is_copperdone
        mov locmode,offset slice3
        mov ax,loccnt
        mov dx,word ptr[offset tim1]
        mul dx                          ;rest of slice 2 (plus 64)
        sub ax,64
        add ax,word ptr[offset tim56]   ;slice 3 time
        add tlocal,ax
is_copper:
        mov storeborouts,0      ;only start at beginning of 50hz slice
        mov bx,word ptr [offset _vidbufbase]
        mov dx,16384
        mov cx,22528
        mov di,offset scradrbuf
is_fillscradrbuf:               ;fill scradrbuf with triples of words:
        mov es:[di],bx          ; offset into coppering video buffer,
        add di,2                ; offset into spectrum screen bitmap,
        mov es:[di],dx          ; offset into spectrum attr data, for
        add di,2                ; each line (192 triples)
        mov es:[di],cx
        add di,2
        add bx,64               ;update 1st
        inc dh                  ;update 2nd..
        test dh,7
        jne is_f_1
        add cx,32               ;update 3rd
        sub dh,8
        add dl,32
        jne is_f_1
        add dx,2048             ;2nd updated
is_f_1: cmp dx,22528            ;if screen -> start of attr, finished
        jne is_fillscradrbuf
is_copperdone:
        pop di
        pop si
        pop es
        retf




setslicetimers:
        cmp _hmode,4
        jae sst_128
        mov timupper,64*224
        mov tim1,224
        mov tim56,56*224
        mov tim64,64*224
        mov tim192,192*224
        mov tim96,96*224
        mov tim24,24*224
        mov tim33,33*224
        mov _tframe,69888
        mov timquart,17472
        ret
sst_128:
        mov timupper,63*228
        mov tim1,228
        mov tim56,56*228
        mov tim64,64*228
        mov tim192,192*228
        mov tim96,96*228
        mov tim24,24*228
        mov tim33,33*228
        mov _tframe,70908
        mov timquart,17727
        ret



;word emulate(byte far* spec_segment)
_emulate:
global _emulate: proc
        pushad                  ;8x4 bytes
        mov bp,sp               ;bp -> 32,ret,ret,ptr,seg
        mov ax,[bp+38]
        push es
        push ds
        mov ds,ax               ;ds:0 (hopefully) points to spec segment
        mov ax,SPECDATA
        mov es,ax
        cmp _copper,0
        jesh emulate_nocopper
        mov gs,word ptr [offset _vidbufbase+2]
emulate_nocopper:
        xor esi,esi             ;Clear out hi parts of ESI and EDI (not EBX yet)
        xor edi,edi
        mov ax,es:rfa
        mov cx,es:rbc
        mov dx,es:rde
        rol edx,16              ;EBP and EBX yet to be set/cleared
        mov dx,es:rhl           ;Free: EAXhi, CXhi, ESIhi, EDIhi
        mov si,es:rpc           ;AF=AX,BC=CX,DE=EDXhi,HL=DX,PC=SI,SP=DI
        mov di,es:rsp           ;Only IX,IY,R,I and exchange regs are in mems
;
;Now check whether local or global T counter will wrap first
;
continueemulating:              ;Entry point after local event
        xor ebp,ebp             ;XOR/MOV is faster than MOVZX on 486
        mov ebx,tglobal         ;These are guaranteed to be both >= 0
        mov bp,tlocal           ;They may well be zero, e.g. when we're
        cmp ebp,ebx             ; continuing after a DI/HALT, IN or OUT
        jbsh se_1               ; exception
        mov ebp,ebx
se_1:   sub tglobal,ebp         ;Now ebphi=0, as tlocal<=0xffff
        sub tlocal,bp
        add tbase,ebp           ;Add to tbase; tbase-EBPhi = actual time-after-irupt
        mov ebx,dword ptr[offset rreg]  ;Retrieve R reg and timer
        xor bh,bh               ;Reset dummy hi counter of R reg to prevent wrap
        rol ebp,16              ;Move length of next block to EBPhi
        add ebp,ebx             ;Add to T counter, and move R reg to BP
        xor ebx,ebx             ;Clear EBX
;
;Now all registers have their 'run time' value.
;Exceptions occur
;1. at the end of instructions (time up, screen ready, log buffer full)
;2. at the end of instruction, but before singleinstr code has been exec'd:
; out done, sound buffer full, border buffer full
;3. at the start of instructions (ld hl,(nn) at 8, ret at 0700, IN, sound IN
; but buffer empty, dihalt, ed extensions, load and save traps)
;Case 2: when singleinstr==1 (debug, logging) the end-of-opcode routines
; have still to be executed.
;Case 3: Sometimes instr can be executed by hand in C, and then case 2
; applies.  Sometimes instr can be re-tried (dihalt, sound IN but buffer
; empty).  IN can also be re-tried with appropriate flags set, but this is
; unsafe as IN can also swap memory.  So with IN, a jump has to be made into
; the IN routine with BL holding the right answer.
;
;Jump to appropriate place

        jmp corestart           ;core_start_of_opcode or core_cont_inning or core_end_of_instr

core_start_of_opcode:
;
;Possibly next block is very short or 0 length, e.g. when local and global
;counter wrap simultaneously.  When this happens, EBPhi is just below zero now
;
        cmp ebp,0ff000000h      ;-256<EBPhi<0 ?
        ja wrap_normalmode      ;If so, don't execute any instructions
;
;(Do not jump to wrapped, as the return from the core was either inside,
; that is, before the last opcode was emulated, in which case all relevant
; timing actions have been taken and the opcode has to be executed in
; exactly the same conditions, or it was due to a timer wrap, in which
; case the other timer apparently also wrapped.  In neither case has any
; debug/log action to be taken.)
;
;The processor is now free to execute instructions.  But first check if
;there's a pending Reset, NMI or interrupt.  Before that, check whether
;'single instructions' are to be emulated (logging, debugging)
;
        cmp singleinstr,0
        jnesh exec_single
exec_cont:
        cmp _anyinter,0         ;Any of reset, nmi or interrupt?
        jnesh exec_interr       ; (see also below)
        emulshort
exec_interr:
        mov _anyinter,0
        cmp _reset,0
        jnesh exec_reset
        cmp _nmi,0
        jne exec_nmi
        cmp _inter,0
        jne exec_inter
        emulshort               ;Start emulating
;
;While logging, FS will be loaded but not preserved.
;While coppering, GS will be used but never re-loaded; GS should be preserved
;

exec_single:
        mov dword ptr [offset rreg],ebp         ;save counter and R register
        movzx ebp,bp                            ; reset counter, so as to make
                                                ; sure it wraps after 1 instr.
        cmp _anyinter,0                         ;Any of reset, nmi or interrupt?
        jnesh exec_interr                       ; (see above)
        emulshort

exec_reset:
        mov _reset,0
        mov _nmi,0               ;Reset nmi and interrupt flags
        mov _inter,0
        mov iff,0
        mov iff2,0
        xor si,si               ;Reset PC
        mov bp,-1               ;Reset R
        em 0,4

exec_nmi:                       ;If RESET and NMI occur simultaneously,
        mov _nmi,0               ; then NMI takes precedence
        mov _inter,0
        pushpc                  ;Push PC to the stack
        mov iff,0               ;Value of iff2 is set by EI,DI,INTER or RESET
        mov si,066h
        em 0,11

exec_inter:
        mov _inter,0
        cmp iff,0
        je exec_i_di
        mov word ptr [offset iff],0
        cmp byte ptr [si],118
        jnesh exec_i_nohalt
        inc si
exec_i_nohalt:
        pushpc
        mov bl,imode
        and bl,3
        jesh exec_imode0
        cmp bl,1
        jesh exec_imode1
        cmp bl,2
        jesh exec_imode2          ;IM0 and "IM3" are equivalent

exec_imode0:
        mov si,038h
        em 0,12

exec_imode1:
        mov si,038h
        em 0,13

exec_imode2:
        mov bh,ri
        mov bl,0ffh
        mov si,bx
        mov bl,[si]
        inc si
        mov bh,[si]
        mov si,bx
        xor bx,bx
        em 0,19

exec_i_di:
        cmp byte ptr [si],118
        jesh exec_i_dihalt
        emulshort

exec_i_dihalt:
        mov bx,msg_dihalt
        jsh emul_ret


emul_ret_eoi:                   ;entry when exception occ's before singleinstr
        mov corestart,offset core_end_of_instr  ;code has been exec'd
emul_ret:
        cmp singleinstr,0
        jnesh emul_ret_singleinstr
        mov dword ptr [rreg],ebp
emul_return:                    ;ASSUMES EBP IS STORED IN DWORD PTR [RREG]!
        mov es:rfa,ax           ;Store A,F
        mov es:rbc,cx           ;Store BC
        mov es:rhl,dx           ;Store HL
        rol edx,16
        mov es:rde,dx           ;Store DE
        mov es:rpc,si           ;Store PC
        mov es:rsp,di           ;Store SP
        pop ds
        pop es
        mov bp,sp
        mov [bp+28],bx          ;store return value (message number)
        popad                   ;return value now in AX
        retf

emul_ret_singleinstr:
        mov word ptr [rreg],0           ;reset R register, and
        add dword ptr [rreg],ebp        ; add in EBP to update rreg/timer
        jmp emul_return

;
;other corestart entries:
;

core_end_of_instr:
        mov corestart,offset core_start_of_opcode
        cmp singleinstr,0       ;normal mode? then just continue into next
        je core_start_of_opcode ; opcode.  If not, note that time is updated already
;        mov ebp,dword ptr [rreg]        ;get time and R register
        add ebp,01000000h       ;add 256 T; this makes timer positive
        mov dword ptr [rreg],ebp
        movzx ebp,bp            ;Clear timer part
        sub ebp,01000000h       ;This surely wraps timer if timer was negative;
        jmp wrapmod             ; will not do so if it was positive.



core_cont_inning:                       ;Entry point when C returns result
        mov corestart,offset core_start_of_opcode
                                        ;Reset corestart variable
        cmp _isresult,2                 ;Float bus?
        jesh cc_float                   ;Then float bus
        xor ebx,ebx
        mov bl,_inresult                ;Collect result
returnfromin:
        cmp singleinstr,0
        jne rfi_singleinstr
        jmp inreturn                    ;Return to opcode code

rfi_singleinstr:
        mov dword ptr [offset rreg],ebp         ;save counter and R register
        movzx ebp,bp                            ; reset counter, so as to make
        jmp inreturn

cc_float:
        mov ebx,ebp                  ;current time = tbase - ebphi
        add ebx,01000000h
        shr ebx,16
        add ebx,timupper             ;tbase - ebx = curtime - 64*224
        neg ebx
        add ebx,tbase
        js short execin_fb_ff        ;if negative, in upper part
        sub ebx,tim192
        js short execin_no_ff        ;if too large, in lower part
execin_fb_ff:
        mov ebx,0ffh                 ;Clears EBXhi
        jmp returnfromin
execin_no_ff:
        xor bl,bh                    ;to avoid unwanted synchronisation
        test bx,2
        jesh execin_fb_ff            ;in 50% of time, return #ff
        and ebx,7                    ;Clears EBXhi
        cmp bx,3
        jbsh execin_fb_scr
        cmp _copper,0
        jnesh execin_fb_attr_cpr
        add floatrnd,3797
        mov bx,floatrnd
        shr bx,2                        ;bx/4
        sub bx,floatrnd                 ;bx/4-bx
        neg bx                          ;0-49152
        and bx,0bfffh
        shr bx,6                        ;0-767
        mov bl,[bx+22528]
        xor bh,bh
        jmp returnfromin
execin_fb_scr:
        cmp _copper,0
        jesh execin_fb_scr_nrm
        mov bx,scradrbufptr
        mov bx,es:[bx+2]                ;get scr offset into spec mem
        jsh execin_fb_x_cpr
execin_fb_attr_cpr:
        mov bx,scradrbufptr
        mov bx,es:[bx+4]                ;get attr offset into spec mem
execin_fb_x_cpr:
        rol ecx,16
        mov cx,floatrnd
        add cx,19                       ;closest rel.prime to 32 to 32*gldn mean
        and cx,31
        mov floatrnd,cx
        add bx,cx
        rol ecx,16
        mov bl,[bx]
        xor bh,bh
        jmp returnfromin
execin_fb_scr_nrm:
        mov bx,floatrnd
        shr bx,2
        sub bx,floatrnd
        neg bx
        and bx,0bfffh
        shr bx,3                        ;0-6143
        mov bl,[bx+16384]
        xor bh,bh
        jmp returnfromin




;
;The following routines should preserve the contents of: AX,CX,EDX,SI,DI
;
wrapped:                        ;This jumps to debug/log routines (wrap_logmode)
        jmp wrapmod             ; or to wrap_normalmode




;********************* First wrap routine -- ordinary one ********************

wrap_normalmode:
        mov dword ptr [offset rreg],ebp ;Store r register and timer value
wrap_normalmode_fromlog:
        test tlocal,0ffffh      ;Is it the local timer?
        jne wrapped_global
        jmp locmode             ;Jmp to the control routine


;control routines for screen fetch in coppering mode

slice1:                         ;at T=64*224, ->2,2b,3
        cmp _novideograb,0
        jnesh slice1_nograb
        cmp _copper,0
        jesh slice1_quick
        mov locmode,offset slice2
        mov tlocal,64           ;Emulate half a screen line before first fetch
        mov loccnt,192          ;Number of screen lines
        mov scradrbufptr,offset scradrbuf
        jmp continueemulating
slice1_quick:                   ;Normal mode; display scr halfway through
        mov locmode,offset slice2b      ;Don't jump into coppering code
        mov ebx,tim96           ;half a screen
        mov tlocal,bx
        jmp continueemulating
slice1_nograb:                  ;Do not display screen at all
        mov locmode,offset slice3
        mov ebx,tim192
        add ebx,tim56
        mov tlocal,bx
        mov storeborouts,0      ;turn off; if copper=true slice3 turned it on
        jmp continueemulating

slice2_pg7:
        push ds
        mov bx,scradrbufptr
        mov ds,word ptr _page7seg
        mov di,es:[bx]
        movzx ebx,word ptr es:[bx+2]
        add ebx,dword ptr _page7ptr
        sub ebx,16384
        mov edx,[ebx]
        mov gs:[di],edx
        mov edx,[ebx+4]
        mov gs:[di+4],edx
        mov edx,[ebx+8]
        mov gs:[di+8],edx
        mov edx,[ebx+12]
        mov gs:[di+12],edx
        mov edx,[ebx+16]
        mov gs:[di+16],edx
        mov edx,[ebx+20]
        mov gs:[di+20],edx
        mov edx,[ebx+24]
        mov gs:[di+24],edx
        mov edx,[ebx+28]
        mov gs:[di+28],edx
        mov bx,scradrbufptr
        movzx ebx,word ptr es:[bx+4]        ;get base into spec attrs
        add ebx,dword ptr _page7ptr
        sub ebx,16384
        mov edx,[ebx]
        mov gs:[di+32],edx
        mov edx,[ebx+4]
        mov gs:[di+36],edx
        mov edx,[ebx+8]
        mov gs:[di+40],edx
        mov edx,[ebx+12]
        mov gs:[di+44],edx
        mov edx,[ebx+16]
        mov gs:[di+48],edx
        mov edx,[ebx+20]
        mov gs:[di+52],edx
        mov edx,[ebx+24]
        mov gs:[di+56],edx
        mov edx,[ebx+28]
        mov gs:[di+60],edx
        pop ds
        jmp slice2_cont

slice2:                         ;T=64*224+64+n*224, n=0,..,191; ->2,2bis
        push edx                ;This is the coppering code
        push di
        xor edi,edi
        cmp _videopage7,0
        jne slice2_pg7
        mov bx,scradrbufptr
        mov di,es:[bx]          ;get base into video buffer
        mov bx,es:[bx+2]        ;get base into spec screen bitmap
        mov edx,[bx]
        mov gs:[di],edx
        mov edx,[bx+4]
        mov gs:[di+4],edx
        mov edx,[bx+8]
        mov gs:[di+8],edx
        mov edx,[bx+12]
        mov gs:[di+12],edx
        mov edx,[bx+16]
        mov gs:[di+16],edx
        mov edx,[bx+20]
        mov gs:[di+20],edx
        mov edx,[bx+24]
        mov gs:[di+24],edx
        mov edx,[bx+28]
        mov gs:[di+28],edx
        mov bx,scradrbufptr
        mov bx,es:[bx+4]        ;get base into spec attrs
        mov edx,[bx]
        mov gs:[di+32],edx
        mov edx,[bx+4]
        mov gs:[di+36],edx
        mov edx,[bx+8]
        mov gs:[di+40],edx
        mov edx,[bx+12]
        mov gs:[di+44],edx
        mov edx,[bx+16]
        mov gs:[di+48],edx
        mov edx,[bx+20]
        mov gs:[di+52],edx
        mov edx,[bx+24]
        mov gs:[di+56],edx
        mov edx,[bx+28]
        mov gs:[di+60],edx
slice2_cont:
        add scradrbufptr,6
        pop di
        pop edx
        mov ebx,tim1                    ;full horizontal video line
        mov tlocal,bx
        xor ebx,ebx
        dec loccnt
        jne continueemulating
        mov locmode,offset slice2bis
        mov ebx,tim33
        sub ebx,64
        mov tlocal,bx                   ;Bottom border plus rest of video line
        jmp continueemulating


slice2bis:                              ;T=64512=(64+192+32)*224, ->3
        mov locmode,offset slice3
        mov ebx,tim24                   ;56-32, bottom part
        mov tlocal,bx
        mov storeborouts,0              ;Signal: don't store more outs in out bfr
        xor ebp,ebp
        lfs bp,_outbufptr
        xor ebx,ebx
        mov bx,_outbufoffset
        mov fs:[ebp+4*ebx],012000000h   ;sentinel, T=73728 way beyond e.o.screen
        mov bx,msg_grabok
        jmp emul_return                 ;Signal calling prog to display screen
                                        ; (NOT emul_ret; see wl_wrapped below)


slice2b:                                ;T=(64+96)*224, ->3
        mov locmode,offset slice3
        mov ebx,tim96
        add ebx,tim56
        mov tlocal,bx                   ;bottom half of screen + lower border
        mov bx,msg_halfscreen           ;Signal: display scrn now if ye want to
        jmp emul_return                 ;(NOT emul_ret; see wl_wrapped below)


slice3:                                 ;T=(64+192+56)*224 = 69888, ->1
;        mov tbase,0                    ;BPhi<0, means we're new blk already
        mov ebp,_tframe
        sub tbase,ebp
        inc _tbasehi                     ;add 20ms in own counter
        mov locmode,offset slice1
        mov bl,_hmode
        cmp timhmode,bl
        je slice3_nohmodechange
        mov timhmode,bl
        call setslicetimers
slice3_nohmodechange:
        mov ebx,timupper
        mov tlocal,bx                   ;Upper part
        mov _inter,0ffh                 ;Generate interrupt (now)
        mov _anyinter,0ffh
        inc _flashcnt
        and _flashcnt,31
        mov _outbufoffset,1             ;reset pointer
;        cmp _iicounter,0
;        jesh s3_iiok
;        cmp _iimode,0
;        jesh s3_iiok                    ;if not intelli-inning, don't do it now
;        mov _iimode,1                   ;reset intelli-inner
;s3_iiok:
        mov _iicounter,10               ;tolerate 10 intelliin misses this slice
        inc _miridle                    ;if >1 then saving has ended
                                        ;if _novideograb, then storing is
        mov bp,_border                  ; turned off at start of slice 2
        and ebp,7                       ;not necessary, but well... (IS necc.)
        lfs bx,_outbufptr               ;Even done when _copper=0, for when
        mov fs:[bx],ebp                 ; turning it on there should be
        cmp _copper,0                   ; starter in buffer.
        jesh slice3_nocopper
        mov storeborouts,0ffh           ;start storing outs to out buffer
slice3_nocopper:
        cmp es:_sound,0
        je slice3_50hz
        cmp _soundsilent,0
        jne slice3_50hz
        push eax
        push edx
        push edi
        push _soundnooutyet             ;store this var temporarily
        cmp _soundnooutyet,1
        adc _soundnooutyet,0            ;signal 'block is silent' (0,1->1, 2->2)
        call FAR PTR updatesound_f      ;fill sample buffer till <now>
        mov eax,_soundbufptr
        xor edx,edx
        div _BytesPerBlock              ;ax=cur buf nmbr
        cmp ax,_soundcurblk
        mov _soundcurblk,ax
        pop bp                          ;retrieve _soundnooutyet
        pop edi
        pop edx
        pop eax
        je slice3_50hz_noblk
        cmp _soundnooutyet,0            ;were new edges entered?
        je slice3_noise                 ;if there were, jump forward
        mov _soundsilentq,bp            ;if prev blk was silent, then signal silence
slice3_noise:
;        cmp _soundnooutyet,2            ;this blk is silent up till <now> (NOT: AY!!)
;        if ne mov _soundnooutyet,1      ; but do not reduce 2 (silent from start) to 1
        mov bx,msg_sampleblk50hz        ;signal 'another sample blk is ready'
        jmp emul_return                 ;(NOT emul_ret; see wl_wrapped below)

slice3_50hz_noblk:
        mov _soundnooutyet,bp           ;restore value
slice3_50hz:
        mov bx,msg_50hz
        jmp emul_return

wrapped_global:
        mov bx,msg_twrap
        jmp emul_return                 ;(NOT emul_ret; see wl_wrapped below)





;************** Second wrap routine -- OUT logging, or trace logging *********

wrap_logmode:                           ;Entry point after every instr when
        cmp byte ptr [si],118
        jesh w_l_dontloghalt
        lfs bx,_logbufptr               ; logging (trace, not outs)
        mov word ptr [fs:bx],0fffeh
        mov word ptr [fs:bx+2],si
        mov byte ptr [fs:bx+4],al
;********** temp
;        push ax
;        mov ax,bp
;        mov byte ptr [fs:bx+4],al
;        pop ax
;********** end temp
        add bx,5
        mov word ptr [offset _logbufptr],bx
        sub _logbuflen,5                ;It just filled up (Note: 5|logbuflen)
        jbesh wl_logbuffull
w_l_dontloghalt:
        mov ebx,dword ptr [offset rreg] ;get timer and R register
        xor bx,bx                       ;reset old R register
        add ebx,ebp                     ;Adding in negative time value, and
                                        ; update BX to actual R reg value
        mov dword ptr [offset rreg],ebx ;Store new timer and R reg value
        jncsh wl_wrapped                ;Jump out if counter wrapped
        movzx ebp,bp                    ;Reset BPhi to allow time for 1 instr
        xor ebx,ebx
        emulshort                       ;And go

wl_logbuffull:
        mov ebx,dword ptr [offset rreg] ;get timer and R register
        xor bx,bx                       ;reset old R register
        add ebx,ebp                     ;Adding in negative value
        mov dword ptr [offset rreg],ebx ;Store new timer and R reg value
        mov bx,msg_logbuffull
        jmp emul_return                 ;(NOT emul_ret)

wl_wrapped:
        mov ebp,ebx
        jmp wrap_normalmode_fromlog






;***************** Third wrap routine -- debug *****************************

wrap_debugmode:                         ;entry pt after every instr when dbging
;        mov ebx,dword ptr [offset rreg] ;get timer and R register
;        xor bx,bx                       ;reset old R register
;        add ebx,ebp                     ;Adding in negative time value, and
;                                        ; update BX to actual R reg value
;        mov dword ptr [offset rreg],ebx ;Store new timer and R reg value
        mov word ptr [offset rreg],0    ;Clear R register
        add dword ptr [offset rreg],ebp ;Reset R register, update timer
        jncsh wd_wrapped                ;Jump forward if counter wrapped: make
                                        ; sure that after handling of dbg,
                                        ; timer wrap is handled
        call exec_debug                 ;Doesn't return if trap trips
wrap_dbg_normal:                        ;Normal continuation of wrap_debugmode
        mov bp,[rreg]
        movzx ebp,bp                    ;Reset BPhi to allow time for 1 instr
        xor ebx,ebx
        emulshort                       ;And go

wd_wrapped:
        call exec_debug                 ;Doesn't return if trap trips
        mov ebp,dword ptr [rreg]
        jmp wrap_normalmode_fromlog     ;Handle time-wrap.


;***************** End of wrap routines ************************************





;The following subroutine changes tables cbemultab and xdcbemultab to have
; RL and RR store bits during mirroring
;It is called from INOUT

_activaterlrr:
        cmp _miractive,0
        jesh alrll_not
        mov activerlrr,0ffh
        push di
        push cx
        push ax
        cld
        mov di,offset cbemultab+020h    ;offset for opcode 010h, RL B
        mov ax,offset opactiverlrr
        mov cx,010h
        rep stosw
        mov di,offset xdcbemultab+020h
        mov ax,offset opactivexdrlrr
        mov cx,010h
        rep stosw
        pop ax
        pop cx
        pop di
alrll_not:
        retf

_deactivaterlrr:
        cmp activerlrr,0ffh
        jne alrll_not
        mov activerlrr,0
        push di
        push si
        push ds
        push cx
        cld
        push es
        pop ds
        cld
        mov si,offset cbemul1020
        mov di,offset cbemultab+020h
        mov cx,010h
        rep movsw
        mov si,offset xdcbemul1020
        mov di,offset xdcbemultab+020h
        mov cx,010h
        rep movsw
        pop cx
        pop ds
        pop si
        pop di
        retf



;Here's a little help-function that performs a computation using
;longs, with dlong intermediate results.
;
;long muladd32(long a,long b,long c,long d)
;{ return (a*b)/65536 + c*d }
_muladd32:
global _muladd32: proc
        push bp
        mov bp,sp
        mov eax,[bp+14]
        imul dword ptr [bp+18]
        mov ebx,eax
        mov eax,[bp+6]
        imul dword ptr [bp+10]
        shr eax,16
        add ax,bx
        adc edx,0
        shr ebx,16
        add dx,bx
        pop bp
        retf


CORE ends


end








