include c:/bc4/spectrum/asm/macro.asm




SPECDATA segment dword public 'DATA'

_outaddress     equ $
_outvalue       equ $+2
outdata         dd 0
_inaddress      dw ?            ;Used to pass IN address to C
inreturn        dw ?            ;return address to in routine (see core_...)
                dw ?            ;here CS ('CORE') is dumped, but not used
_isresult       db 0            ;1=in result valid, 2=C says Float bus
_inresult       db ?            ;Used to return IN result

_issue2         db ?

_outbuflen      dw ?            ;length of out buffer in dwords
_outbufptr      dd ?            ;pointer to start of out buffer
_outbufoffset   dw ?            ;offset into out buffer
storeborouts    db 0            ;0 if no outs being stored (when in lo scr part)


_sound          dw ?            ;0 if no sounds are recorded
_TStatesPerSample dw ?
_BytesPerBlock  dd ?            ;bytes (not samples) in one sample block
_soundbufbase   dd ?            ;pointer to sample buffer
_soundbuflen    dd ?            ;total length of sample buffer (ex 32 byte tail)
_soundbufptr    dd ?            ;current position into buffer
_soundlasttlo   dd ?            ;T state time corresponding to above position
_soundlastthi   dd ?            ;20ms counter value corresp. to above position
_soundlastval   dd ?            ;(2 copies of) current signal level
_soundlastamp   db ?            ;current signal (0<=amp<=63)
_sounddiff      db ?            ;delta(soundlastamp); only used internally
_soundnooutyet  dw ?            ;true if no outs since start of current blk
_soundsilent    dw ?            ;true if stopped logging
_soundsilentq   dw ?            ;true if no OUTs in last block.  Copied to ^ by C
_soundtimelo    dd ?            ;time of current edge. Curr time in case of FE out
_soundtimehi    dd ?            ; earlier in AY case.  0<=soundtimelo<tbase.  See computeloctime
_soundcurblk    dw ?            ;next block to be played; msg sent to C; upd'd by asm
_soundnextblock dw ?            ;next block to be sent to sound card; upd'd by C

_AYemul         dw ?            ;1 if AY is emulated.
_AYtimelo       dd ?            ;Time until which AY chip has been emulated.
_AYtimehi       dd ?
_AYcountcur     dd ?            ;Current # of T states to emulate
_AYcounte       dd 100
_AYcounta       dd 110          ;T state counters of tone, noise and env gen's
_AYcountb       dd 120
_AYcountc       dd 130
_AYcountn       dd 140
_AYbitout       db 0            ;bit outputs of noise and tone channels
                db ?
_AYevol         db 0            ;curr volume of envelope
_AYeadd         db 1            ;curr direction of env
_AYevol2        db 0            ;next period
_AYeadd2        db 0            ;next period
_AYetoggle      dw 0            ;!=0 if envelope toggles
_AYnseed        dw 0            ;noise random generator seed

_AYvola         db 0            ;actual volume of channels (amp & env combined)
_AYvolb         db 0
_AYvolc         db 0
                db ?            ;to make 4.  Important!!

_AYampa         db 0            ;final ouput signals of AY channels
_AYampb         db 0            ;These four MUST be grouped like this!
_AYampc         db 0
_feamp          db ?            ;'output signal of FE port'

_felastout      dw ?            ;last state of port
_feearmicmask   dw ?            ;8 or 16 (mic or ear)
_feactuallastout dw ?           ;unmasked last state of #fe; used for inning from #fe
_fevolume       dw 15           ;0-18 (15+15+15+18=63)
_AYvolume       dw 256          ;16*: 0-16

_specdrumport   db 0
_specdrumshift  db 8
_specdrumval    db 0

_DBGnumloops    dd ?
_DBGnumouts     dd ?
public _DBGnumloops
public _DBGnumouts

envtable        db 15+64,-1,0,0
                db 15+64,-1,0,0
                db 15+64,-1,0,0
                db 15+64,-1,0,0
                db 64,1,0,0
                db 64,1,0,0
                db 64,1,0,0
                db 64,1,0,0
                db 15+64,-1,15+64,-1
                db 15+64,-1,0,0
                db 15+64,-1,64,1
                db 15+64,-1,15,0
                db 64,1,64,1
                db 64,1,15,0
                db 64,1,15+64,-1
                db 64,1,0,0

_inning         dw ?            ;0ff if inning from sample buffer
_inbufbase      dd ?            ;pointer to base of in buffer
_intbaselo      dd ?            ;lo counter of time corresp. to base of in bfr
_intbasehi      dd ?            ;hi (50hz) counter
_inbuflen       dd ?            ;# valid data bits in in buffer
_intperbit      dd ?            ;t states per bit

;intelli-in section
iirbc           dw ?            ;storage of registers in intelli-in mode
iirdehl         dd ?
iirpc           dw ?
iirbcmask       dw ?            ;mask for count register
iirdehlmask     dd ?
iishiftright    db ?            ;right-shift needed to get count reg in b0-b7
iidirection     dw ?            ;1 or 0ff
iilastvalue     dw ?            ;bit value
iilasttime      dd ?
iideltat        dd ?
_iimode         dw 1            ;0=off, 1=watch, 2=regs valid, 3=active, 4=inning
_iicounter      dw 10           ;0=don't do (this frame), >0 = active

;warajevo-mode section
_waractive      dw 0            ;1 if currently loading .TAP block in warajevo mode
_warbuffer      dd ?            ;incremental pointer to next byte to be loaded
_warbytesleft   dw ?            ;number of bytes left in buffer.
_warleader      dw ?            ;downcounter.  If =1 sync, if =0 data
_warstopped     dw ?            ;Set if Z80 program stopped polling after sync
_warsignal      dw ?            ;0 or 1.  All signals consist of level 0 then 1
_wardata        dw ?            ;data byte, left shifted. 0xff00 = empty
_warsiglen      dd ?            ;edge-to-edge time of current bit
_warsiglenleft  dd ?            ;T states left in current pulse
_warbytecounter dw ?            ;Total # of bytes loaded
_wartbaselo     dd ?            ;Temporary storage

;tape input mirroring section
_miractive      dw 0            ;mirroring loaded data
_mirinned       dw 0            ;next RL/RR shifts data bit into register
_mirbyte        dw 0            ;storage for current byte
_mircurbit      dw 7            ;current bit no (7-0, counting down)
_mirbitcount    dd 0            ;number of bits received
_mirbufptr      dd 0            ;pointer to next free byte in buffer
_mirbuflen      dw 0            ;bytes left in buffer
_miridle        dw 0            ;# of 20ms blks in which no bits were saved


;variables for logging

;logouts         equ 1          ;not=log trace; def=log outs, for OUT2VOC

_logbuflen      dw 0
_logbufptr      dd 0

        ifdef logouts

lastlogthi      dd 0            ;number of 20ms block of which out was put in bfr
lastquarter     dw 0            ;in which quarter was last out

        endif

public  _outaddress
public  _outvalue
public  _inaddress
public  _inresult
public  _isresult
public  _outbufptr
public  _outbuflen
public  _outbufoffset

public  _soundbufbase
public  _soundbuflen    
public  _soundbufptr    
public  _soundlasttlo   
public  _soundlastthi   
public  _soundlastval
public  _soundlastamp
public  _soundnooutyet  
public  _soundsilent
public  _soundsilentq
public  _soundtimelo
public  _soundtimehi
public  _soundnextblock
public  _soundcurblk

public  _felastout
public  _feearmicmask
public  _fevolume
public  _AYvolume

public  _AYemul
public  _AYtimelo
public  _AYtimehi
public  _AYampa
public  _AYampb
public  _AYampc
public  _feamp

public  _specdrumport
public  _specdrumshift
public  _specdrumval

public  _inning
public  _inbufbase
public  _intbaselo
public  _intbasehi
public  _inbuflen
public  _intperbit

public  _iimode
public  _iicounter

public  _BytesPerBlock

public _waractive
public _warbuffer
public _warbytesleft
public _warleader
public _warstopped
public _warsignal
public _wardata
public _warsiglen
public _warsiglenleft
public _warbytecounter

public _miractive
public _mirinned
public _mirbyte
public _mircurbit
public _mirbitcount
public _mirbufptr
public _mirbuflen
public _miridle

SPECDATA ends

;
;_soundtime holds current time after exec of computeloctime.  In enteredge
; it holds time of edge being entered.
;It must represent same or later time than _soundlastt, which is the time
; of the sample to which the current sound buffer pointer points.
;_AYtime holds time of last AY edge entered.  This can be before _soundlastt
; after an OUT FE or call to flattenbuffer, but the next AY edge will in
; that case be after _soundlastt.  Care must be taken at AY init time to
; ensure that _AYtime does not represent a time before _soundlastt.



INOUT segment byte use16 public 'CODE'

ASSUME cs:INOUT

global _WaveTable

lowquad equ 068806880h
hiquad  equ 098809880h

;if sampsiz==2: lowquad = 2 copies of 0x8080-(amp/2) as defined in sound.c
;                                           + for hiquad

tailblen equ 32                         ;length of tail in bytes
tailslen equ 16                         ;length of tail in samples

;Note all references to tailblen in comments!  Corresponding instructions
; need modification!
;Note also _WaveTable, length 64*tailblen, defined in wavetbl.asm


;
;The execin routine receives the address in BX, yields result in BL, and BH=0
;
execin:
        test bx,1                       ;Keyboard?
        je execin_kbd
execin_joystick:
        pop dword ptr inreturn          ;Store return address, dump CS
        mov _isresult,2                 ;Signal: Float bus
        mov _inaddress,bx               ;Pass in address to C
        mov corestart,offset core_cont_inning   ;At entry in _emulate, go to
        mov bx,msg_emulin                       ; core_cont_inning (must be in
        jmp FAR PTR emul_ret                    ; CORE segment!)


;****************************************************************************
;
;Port 0xFE IN code
;
;****************************************************************************


execin_kbd:
        test bx,0ffh-31                 ;Joystick and keyboard simultaneously?
        je execin_joystick              ;Jump back if so; joystick has precedence
        rol edi,16
        mov di,offset _KeyMap+8
        mov bl,byte ptr [offset _feactuallastout] ;get ear/mic state
        cmp _issue2,0
        jne execin_kbd_issue2
        and bl,0f7h                     ;issue3: always reset mic bit here
execin_kbd_issue2:
        and bl,018h
        dec bl
        and bl,040h                     ;EarMic=00: 040h, otherwise 0
        xor bl,0ffh
        xor bh,0ffh                     ;Clears C flag.  Tests for BH=00
execin_kbd_loop:
        jesh execin_kbd_end
execin_kbd_loop2:
        dec di
        rcl bh,1
        jncsh execin_kbd_loop
        and bl,es:[di]
        test bh,bh
        jnesh execin_kbd_loop2
execin_kbd_end:
        rol edi,16
        cmp _inning,0
        jnesh execin_kbd_frombuf
        cmp _waractive,0
        jne execin_kbd_frombuf
        retf



execin_kbd_single:                   ;Calculates EBP value in case of single
        mov eax,dword ptr [rreg]
        xor ax,ax                    ;Clear R register part
        add ebp,eax                  ;Now actual time in EBPhi, R in EBPlo
        jsh execin_kbd_fb_cont

execin_kbd_frombuf:                  ;also entry for warajevo mode inning
        push eax
        push edx
        cmp singleinstr,0
        jne execin_kbd_single        ;Compute EBP value if necessary
execin_kbd_fb_cont:
        mov eax,_tbasehi
        sub eax,_intbasehi
        mul _tframe
        mov edx,ebp                  ;ebphi=ffff to ff00 to be regarded as
        add edx,01000000h            ; negative, rest as positive. Now all as
        shr edx,16                   ; positive, so shift logical right
        sub edx,0100h                ;Make ffff-ff00 into negative values again
        neg edx
        add edx,tbase                ;This is never negative (or shouldn't be)
        mov _wartbaselo,edx          ;Store current time lo counter temporarily
        add eax,edx
        sub eax,_intbaselo           ;Subtract base time lo counter.  EAX now is
        cmp _iicounter,0             ; # of T states passed since last IN in
                                     ; Warajevo mode, or # of T states passed
                                     ; from start of IN buffer in sample play mode
        jne execin_kbd_intelli       ;Jump to intelli-in routines (use EAX)
execin_kbd_continue:
        cmp _inning,0
        je execin_warajevo           ;Jump out now if in warajevo mode
        push edi
        push cx
execin_kbd_continue_2:
        xor edx,edx
        div _intperbit
        cmp eax,_inbuflen
        jaesh execin_kbd_inbufempty
        mov cx,ax
        and cx,31
        shr eax,3
        and ax,0fffch
        xor edi,edi
        lfs di,_inbufbase
        add edi,eax
        mov eax,fs:[edi]
        shr eax,cl
        cmp _iimode,4
        je execin_doii
        and ax,1
        mov iilastvalue,ax
        shl ax,6
        xor bl,al
        pop cx
        pop edi
        pop edx
        pop eax
execin_retf:
        cmp singleinstr,0               ;EBP has values as if singleinstr=0
        jnesh execin_kbd_resetbp        ; so adjust if necessary
        retf
execin_kbd_resetbp:
        mov dword ptr [rreg],ebp        ;Store time (& dummy R)
        movzx ebp,bp                    ;Clear hi part, keep R
        retf

execin_kbd_inbufempty:
        mov eax,_inbuflen
        mul _intperbit
        add eax,_intbaselo
        xor edx,edx
        div _tframe
        mov _intbaselo,edx
        add _intbasehi,eax
        pop cx
        pop edi
        pop edx
        pop eax
        pop ebx                         ;Dump return address
        mov bx,msg_inbufempty
execin_jmpemulret:
        cmp singleinstr,0               ;EBP has values as if singleinstr=0
        jnesh execin_kbd_resetbp_er     ; so adjust if necessary
        jmp FAR PTR emul_ret
execin_kbd_resetbp_er:
        mov dword ptr [rreg],ebp        ;Store time (& dummy R)
        movzx ebp,bp                    ;Clear hi part, keep R
        jmp FAR PTR emul_ret

execin_doii_donothing:
        mov eax,iilasttime             ;Restore eax
        mov _iimode,3
        jmp execin_kbd_continue_2

execin_doii:
        mov ch,0                ;counter: # of entire samples we can skip + 1
        test iilastvalue,1
        jesh execin_doii_loop
        not eax
execin_doii_loop:
        shr eax,1
        jc short execin_doii_edge
        add cx,0101h
        and cl,31
        jne execin_doii_loop
        cmp ch,127-32
        jaesh execin_doii_edge
        add edi,4
        mov eax,fs:[edi]
        test iilastvalue,1
        jesh execin_doii_loop
        not eax
        jmp execin_doii_loop
execin_doii_edge:
        push edx
        xor edx,edx
        mov dl,ch
        mov eax,_intperbit
        mul edx
        pop edx
        inc edx
        sub eax,edx                     ;EAX = # of T states we can safely skip
        js execin_doii_donothing        ; minus 1; jump out if eax<0
        mov edx,ebp
        add edx,01000000h
        shr edx,16
        sub edx,0100h
        sub edx,iideltat                ;Safety margin (because of rounding up below)
        sub edx,eax
        js execin_doii_donothing        ;Jump out if little time left in slice
        sub di,word ptr _inbufbase      ;Subtract base
        shl edi,3                       ;Make edi into a bit count
        sub edi,_inbuflen
        jns execin_doii_donothing       ;If we went outside buffer, do nothing
        xor edx,edx                     ;NOTE: we need 8*len(buffer)=inbuflen+128
        div iideltat                    ;EAX = # of cycles we can simulate
        inc eax                         ;Round up here (note 'inc edx' above)
        cmp iidirection,1
        jesh execin_doii_dir1
        pop cx
        pop edi
        pop edx
        push edx
        push edi
        push cx
        and edx,iirdehlmask
        and cx,iirbcmask
        or dx,cx
        mov cl,iishiftright
        shr edx,cl
        and dx,07eh
        cmp ax,dx                       ;Compare to # of cycles in counter
        ja short execin_doii_ff_cntr
        mov dx,ax
execin_doii_ff_cntr:
        xor eax,eax
        mov ax,dx
        neg edx
        jsh execin_doii_all
execin_doii_dir1:
        pop cx
        pop edi
        pop edx
        push edx
        push edi
        push cx
        and edx,iirdehlmask
        and cx,iirbcmask
        or dx,cx
        mov cl,iishiftright
        shr edx,cl
        or dx,0ff80h
        neg dx
        dec dx
        cmp ax,dx                       ;Compare to # of cycles in counter
        ja short execin_doii_1_cntr
        mov dx,ax
execin_doii_1_cntr:
        xor eax,eax
        mov ax,dx
execin_doii_all:
        shl edx,cl
        cmp iirbcmask,0
        jesh execin_doii_dordehl
        pop cx
        add cx,dx
        push cx
        jsh execin_doii_cont
execin_doii_dordehl:
        mov edi,esp
        add edx,ss:[di+6]
        mov ss:[di+6],edx
execin_doii_cont:
        mov edx,iideltat
        mul edx
        shl eax,16
        sub ebp,eax                     ;Update timer. NOTE: R reg. is wrong now
        mov _iimode,3                   ;'Nu effe niet'
        mov eax,_tbasehi                ;Compute EAX just as at entry
        sub eax,_intbasehi
        mul _tframe
        mov edx,ebp
        add edx,01000000h
        shr edx,16
        sub edx,0100h
        neg edx
        add edx,tbase
        add eax,edx
        sub eax,_intbaselo
        jmp execin_kbd_continue_2


execin_kbd_intelli:
        cmp _iimode,1                    ;Start
        jb execin_kbd_continue
        je ii_getregs
        cmp _inning,0
        jesh execin_k_i_war              ;Jump out now in Warajevo mode
        mov edx,eax
        sub edx,iilasttime
        mov iilasttime,eax
        cmp _iimode,2                    ;Regs are valid
        je ii_checkregs
        mov _mirinned,0ffh               ;Signal: loading, next RL/RR shifts data bit
        mov _iimode,3                    ;Signal: not just now, next time
        cmp iirpc,si
        jne ii_reset
        cmp iideltat,edx
        jne execin_kbd_continue          ;First IN of sequence: exec. normally
        mov _iimode,4                    ;Signal: all variables valid
        jmp execin_kbd_continue

execin_k_i_war:                          ;Here, already EAX=loop time
        cmp _iimode,2                    ;Regs are valid
        je ii_checkregs_w
        mov _mirinned,0ffh               ;Signal: loading, next RL/RR shifts data bit
        mov _iimode,3                    ;Signal: not just now, next time
        cmp iirpc,si
        jnesh ii_reset
        cmp iideltat,eax
        jne execin_warajevo
        mov _iimode,4                    ;Signal: all variables valid
        jmp execin_warajevo

ii_getregs:
        mov iilasttime,eax
        pop edx
        push edx
        mov iirdehl,edx
        mov iirbc,cx
        mov iirpc,si
        mov _iimode,2
        jmp execin_kbd_continue

ii_reset:
        dec _iicounter                   ;To make sure that eventually we'll
ii_reset0:
        mov _iimode,1                    ; not bother about intelli-inning
        call FAR PTR _deactivaterlrr
        jmp execin_kbd_continue

ii_checkregs_w:
        mov edx,eax
ii_checkregs:
        mov iideltat,edx
        cmp edx,150
        ja ii_reset0
        cmp edx,32
        jb ii_reset0                    ;This excludes INI(R) (Tomahawk)
        cmp iirpc,si
        jne ii_reset
        pop edx
        push edx
        sub iirdehl,edx
        sub iirbc,cx
        push cx
        mov cl,0
ii_checkbits:
        test iirdehl,0ffh
        jnesh ii_found
        test iirbc,0ffh
        jnesh ii_found
        add cl,8
        sar iirdehl,8
        sar iirbc,8
        cmp cl,32
        jne ii_checkbits
ii_resetpopcx:
        pop cx
        jmp ii_reset                    ;no differences found
ii_found:
        mov iirdehlmask,0ffh
        mov iirbcmask,0
        cmp iirdehl,0
        je iif_bc
        cmp iirbc,0
        jne ii_resetpopcx               ;more than 1 difference found
        mov iidirection,0ffh
        cmp iirdehl,1
        je iif_done
        mov iidirection,1
        add iirdehl,1
        je iif_done
        pop cx
        jmp ii_reset                    ;difference != 1,-1 found
iif_bc:
        mov iirdehlmask,0
        mov iirbcmask,0ffh
        mov iidirection,0ffh
        cmp iirbc,1
        je iif_done
        mov iidirection,1
        add iirbc,1
        je iif_done
        pop cx
        jmp ii_reset                    ;difference != 1,-1 found
iif_done:
        shl iirdehlmask,cl              ;shift masks into place
        shl iirbcmask,cl
        mov iishiftright,cl
        mov _iimode,3
        call FAR PTR _activaterlrr
        pop cx
        jmp execin_kbd_continue


;Routines for inning in Warajevo mode
;Inning from sample buffer takes precedence
;Variabeles _intbaselo, _intbasehi are used in this mode as well
;C should poll _warstopped, and reset it, and set _waractive.
;C is not notified when loading has stopped
execin_warajevo:
        sub _warsiglenleft,eax
        jbsh eiw_newpulse
        cmp _iimode,4
        je execin_doii_war
eiw_newpulse0:
        mov ax,_warsignal
        shl ax,6
eiw_newpulse1:
        xor bl,al
        mov eax,_tbasehi             ;Store current time
        mov _intbasehi,eax
        mov eax,_wartbaselo
        mov _intbaselo,eax
        pop edx
        pop eax
        jmp execin_retf
;        retf
eiw_newpulse:
        xor _warsignal,1             ;Has to be undone if C is called & IN is
        jesh eiw_newbit              ; executed a second time
        mov eax,_warsiglen
        add _warsiglenleft,eax
        mov ax,64
        jns eiw_newpulse1
eiw_timeout:
        cmp _warleader,0
        jesh eiw_loadstop
        mov _warleader,768
        mov _warsiglen,2168
        mov _warsiglenleft,2168
        jmp eiw_newpulse0
eiw_loadstop:
        mov _warstopped,1
        mov _waractive,0
        mov _warsiglenleft,0
        pop edx
        pop eax
;        retf
        jmp execin_retf
eiw_newbit:
        cmp _warleader,1
        jbsh eiw_databit
        jesh eiw_sync
        dec _warleader
        mov _warsiglen,2168
        add _warsiglenleft,2168
        jns eiw_newpulse0
        jmp eiw_timeout
eiw_sync:
        dec _warleader                  ;zero now
        mov _wardata,0                  ;signal 'no data'
        mov _warbytecounter,0
        mov _warsiglen,735
        add _warsiglenleft,667
        js eiw_timeout
        jsh eiw_nodata                  ;Go and fetch data for first bit
eiw_databit:
        mov dx,_wardata
        shl dx,1
        mov _wardata,dx
        mov eax,855
        jncsh eiw_data0
        mov eax,855*2
eiw_data0:
        mov _warsiglen,eax
        add _warsiglenleft,eax
        js eiw_loadstop
        test dl,dl                      ;Did we just run out of bits?
        jne eiw_newpulse0               ;Not: jump out
        cmp _warbytesleft,0
        jesh eiw_nodata
        dec _warbytesleft
        inc _warbytecounter
        push fs
        push di
        lfs di,_warbuffer
        mov dh,fs:[di]
        mov dl,0ffh
        inc di
        mov word ptr _warbuffer,di
        pop di
        pop fs
        mov _wardata,dx
        jmp eiw_newpulse0
eiw_nodata:
        mov ax,_warsignal
        shl ax,6
        xor bl,al
        mov _isresult,1                 ;Signal: result to be collected
        mov _inresult,bl                ;Store result
        mov eax,_tbasehi                ;Store current time
        mov _intbasehi,eax
        mov eax,_wartbaselo
        mov _intbaselo,eax
        pop edx
        pop eax
        pop dword ptr inreturn          ;store return address & dump CS ('CORE')
        mov corestart,offset core_cont_inning  ;continue executing there
        mov bx,msg_getwarmodedata       ;After data has been fetched, result will
;        jmp FAR PTR emul_ret            ; be passed directly to calling IN routine
        jmp execin_jmpemulret


;Intelli-in routines for Warajevo-mode.
ei_doii_do0:
        mov eax,_warsiglenleft          ;Eat up all time left and do not update
        mov _iimode,3                   ; counters when sending a 0
        inc eax
        jmp execin_warajevo

ei_doii_donothing_war:
        xor eax,eax
        mov _iimode,3
        jmp execin_warajevo


execin_doii_war:
        cmp _warleader,0
        jne ei_doii_donothing_war
        cmp _warsiglen,855              ;Sending a 0 bit?
        jbesh ei_doii_do0
        pop edx
        push edx
        push cx
        and edx,iirdehlmask
        and cx,iirbcmask
        or dx,cx
        mov cl,iishiftright
        shr edx,cl
        cmp iidirection,1
        jesh ei_doii_dir1_war
        and edx,07eh
        shr edx,1                       ;Divide by 2
        neg edx
        jsh ei_doii_all_war
ei_doii_dir1_war:
        or edx,0ffffff80h
        neg edx
        dec edx
        shr edx,1                       ;Divide by 2
ei_doii_all_war:
        shl edx,cl
        cmp iirbcmask,0
        jesh ei_doii_dordehl_war
        pop cx
        add cx,dx
        push cx
        jsh ei_doii_cont_war
ei_doii_dordehl_war:
        push ebp
        mov ebp,esp
        add [ebp+6],edx
        pop ebp
ei_doii_cont_war:
;        sub ebp,224*010000h             ;Update timer.
;        mov eax,224
;        add eax,_wartbaselo             ;Add skipped T states to 'current time'
;        div _tframe
;        add _intbasehi,eax
;        mov _wartbaselo,edx
        mov eax,_warsiglenleft          ;Eat up all time
        inc eax
        pop cx
        mov _iimode,3                   ;'Nu effe niet'
        jmp execin_warajevo




;****************************************************************************
;
;General OUT code.  Mostly dedicated to port 0xFE
;
;****************************************************************************




;
;The execout routine receives address in BX, value in (EBXH)L.
;Should continue by checking BPhi, then EMULSHORTfar

eo_singleinstr:
        push ebp
        add ebp,dword ptr [rreg]      ;Now R register is not right.  Doesn't matter
        computeloctime
        pop ebp                       ;Restore EBP, makes sure we wrap afterwards
        jmp short eo_continue

eo_other:
        mov outdata,ebx
        mov bx,msg_emulout
        jmp FAR PTR emul_ret_eoi

execout:
global execout: proc
        test bl,1
        jne short eo_other
        push ax
        push edx
        push edi
        cmp singleinstr,0             ;single instructions, i.e. EBP not right?
        jne eo_singleinstr            ;jump out if so.
        computeloctime                ;local time in edx,eax;_soundtime(lo/hi)
eo_continue:
        shr ebx,16
        mov _feactuallastout,bx       ;store EAR/MIC states (for proper issue2/3 in #fe)
        xor bx,_border
        test bx,7
        jne eo_border                 ;Jumps back to eo_sound when done
eo_sound:
        cmp es:_sound,0
        je eo_retpop
;
;addition for re-playing .voc and .tzx files
;
;        cmp _inning,0
;        jne eo_retpop                 ;Do not enter edge if playing vocs/tzxs
;
;End addition (22/10/97) (geen zin)
;
        and bx,_feearmicmask
        cmp _felastout,bx
        je eo_retpop
        mov _felastout,bx

;****************************************************************************
;LOGOUTS:
;****************************************************************************

        ifdef logouts

        cmp _logging,0
        je eo_nologging

        mov eax,edx                   ;get lo counter
        xor edx,edx
        mov ebx,timquart
        div ebx                       ;ax=quarter, dx=rest
        push dx                       ;save rest

        mov edx,_soundtimehi
        cmp edx,lastlogthi
        mov lastlogthi,edx
        mov dx,lastquarter
        mov lastquarter,ax
        jesh eo_no20mswrap
        sub dx,4
eo_no20mswrap:
        lfs bx,_logbufptr

eo_quarterwrap:
        cmp dx,ax
        jesh eo_noquarterwrap
        inc dx
        mov word ptr [fs:bx],0ffffh     ;wrap block
        push edx
        mov edx,timquart
        mov word ptr [fs:bx+2],edx
        pop edx
        add bx,5
        sub _logbuflen,5                ;max 7 times
        jmp eo_quarterwrap

eo_noquarterwrap:
        pop dx
        mov [fs:bx],dx                  ;time
        mov word ptr [fs:bx+2],0feh     ;port address
        mov ax,_soundlastout
        mov [fs:bx+4],al                ;value
        add bx,5
        sub _logbuflen,5                ;max once
        mov word ptr [offset _logbufptr],bx
        cmp _logbuflen,40
        jaesh eo_nologging              ;enough for next out

        pop edi                         ;do not process this out....
        pop edx                         ; Data in log file will be correct
        pop ax
        mov bx,msg_logbuffull           ;signal: log buffer full
        jmp FAR PTR emul_ret_eoi
        mov eax,_soundtimehi
        mov edx,_soundtimelo
eo_nologging:

        endif

;****************************************************************************
;END LOGOUTS
;****************************************************************************

        call AYupdate                   ;first emulate AY chip up to now
                                        ;This leaves _soundtime(hi/lo) == current time
        mov bx,_felastout
        test bx,bx
        setz bl
        dec bl
        and bx,_fevolume                ;Now BX is new FE signal output
        mov _feamp,bl
        call enteredge
eo_retpop:
        mov eax,_soundbufptr
        xor edx,edx
        div _BytesPerBlock              ;ax=blk number
        cmp ax,_soundcurblk
        jnesh eo_newblk
        pop edi
        pop edx
        pop ax
        xor ebx,ebx
        cmp ebp,0ff000000h
        ja short wrapped_f
        emulshortfar
wrapped_f:
        jmp far ptr wrapped

eo_newblk:                              ;Doesn't work properly without this,
        mov _soundcurblk,ax             ; but I do not understand why.
        pop edi                         ; Must eventually work without it,
        pop edx                         ; as AY update behaves similarly to
        pop ax                          ; normal sound without eo_newblk
        mov bx,msg_sampleblk            ;signal: blk ready to be played
        jmp FAR PTR emul_ret_eoi



eo_border:
        xor bx,_border
        push bx
        and bx,7
        mov _border,bx                ;it changed
        test storeborouts,0ffh        ;don't store outs in lowest part of scr
        jesh eo_soundpopbx
        cmp edx,30
        jb eo_soundpopbx              ;do not store outs around 50hz wraparound
        rol edx,8                     ; (potential probl: no sentinel)
        mov dl,bl
        xor edi,edi
        xor ebx,ebx
        lfs di,_outbufptr
        mov bx,_outbufoffset
        mov fs:[edi+4*ebx],edx
        inc bx
        mov _outbufoffset,bx
        cmp bx,_outbuflen
        mov edx,_soundtimelo
        pop bx
        jne eo_sound
        pop edi
        pop edx                       ;note: possible ear/mic change ignored!
        pop ax
        mov bx,msg_outbufextend       ; Probability of problem very low, & lazyness
        jmp FAR PTR emul_ret_eoi

eo_soundpopbx:
        pop bx
        jmp eo_sound



;
;Following routine makes sure sound blocks are generated, even when
;there are no edges.  This is to provide the raw material for the
;currah speech emulation
;

_enable_soundblocks:
public _enable_soundblocks
        push bp
        push si
        push di
        push es
        mov ax,seg rreg
        mov es,ax
        mov ebp,dword ptr [rreg]
        computeloctime                ;local time in edx,eax;_soundtime(lo/hi)
        cmp es:_sound,0
        je es_ret
        call AYupdate                   ;first emulate AY chip up to now
                                        ;This leaves _soundtime(hi/lo) == current time
        mov ah,_soundlastamp
        xor al,al
        call enter_noedge
es_ret:
        pop es
        pop di
        pop si
        pop bp
        retf



;****************************************************************************
;
;Sample buffer routines.  Handles entering of edges into buffer
;
;****************************************************************************




enteredge:                              ;Destroys EAX,EBX,EDX,EDI,FS
        mov ax,word ptr [offset _AYampa]
        add al,ah
        add al,byte ptr [offset _AYampc]
        and ax,63                       ;A+B+C, remove bit 6 (envelope bit)
        mul word ptr _AYvolume          ;16*0=min, 16*16 = max volume
        add ah,byte ptr [offset _feamp]
        add ah,_specdrumval
        mov al,ah
;        mov ax,word ptr [offset _AYampa]        ;get A and B
;        add ax,word ptr [offset _AYampc]        ;add C and FEamp
;        add al,ah
;        and al,63                       ;remove sum of bit 6 (envelope bit)
;        mov ah,al
        sub al,_soundlastamp
        je eo_done                      ;No change in signal level
enter_noedge:                         ;entry point if making silence blocks
        mov _soundlastamp,ah
        mov _sounddiff,al             ;only used in this subroutine, not globally
        mov eax,_soundtimehi
        mov edx,_soundtimelo
enteredge_entry2:
        cmp _soundsilent,0
        jne eo_initialise             ;if we were silent before, first initialise
eo_noisy:
        mov _soundnooutyet,0          ;signal 'current blk dirty'
        mov _soundsilentq,0           ;signal 'buffer dirty'
        sub eax,_soundlastthi
        jne eo_longtime               ;possibly long time since last out
        sub edx,_soundlasttlo         ;time elapsed
        xchg eax,edx
        movzx ebx,es:_TStatesPerSample
        div ebx                       ;(e)ax=# of samples to skip, edx = remainder
        cmp ax,tailslen
        jae eo_longtime_2             ;handle long times separately; also
                                      ; make sure code below does not run
        xor edi,edi                   ; beyond buffer + 2*tailblen
        lfs di,_soundbufbase          ;Value of EDX will be used below!
        add edi,_soundbufptr
        test eax,eax
        jesh eo_noskip
        push edx
        push eax
        mul ebx                       ;# of T states skipped in eax
        add _soundlasttlo,eax         ;time corresponding to pointer
        pop eax                       ; This may lead to eax>=_tframe; no problem
        add _soundbufptr,eax          ;Add # of bytes, 2 times # of samples
        add _soundbufptr,eax          ;Faster than shl eax/shr eax
        mov dx,word ptr[offset _soundlastval]
eo_fill:
        mov fs:[edi+tailblen],dx
        add edi,2
        dec eax
        jnesh eo_fill
        pop edx
eo_noskip:
        xchg eax,edx                  ;eax was zero, edx held T state count
        shl eax,11                    ;multiply by 64*tailblen=2048 (TStatesPerSample <= 0.6 s)
        div ebx                       ;still _TStatesPerSample
        mov bx,ax                     ;ebx is offset into wavelet table
        and bx,63*tailblen            ;Have to offset for volume still

upd macro addsub,shift,num
        mov eax,dword ptr [_WaveTable+num+ebx-2048]    ;WaveTable first entry is amp=1,
        shift                                          ; not amp=0, so there: -2048
        addsub fs:[edi+num],eax
        endm

shf macro
        shl eax,2                     ;Multiply wave signal by 4
        endm

        mov al,_sounddiff
        test al,al
        js eo_low                     ;Downgoing edge
        jz eo_none                    ;Happens when called from _enable_soundblocks
        test ax,48
        je eo_h_nohisig               ;No hi significant bits to include
        push ax                       ;store vol difference
        and ax,60                     ;Leave out 2 lowest bits
        shl ax,9                      ;Mult by 512; * 4 = 2048
        add bx,ax                     ;True offset into table

        upd add,shf,0
        upd add,shf,4
        upd add,shf,8
        upd add,shf,12
        upd add,shf,16
        upd add,shf,20
        upd add,shf,24
        upd add,shf,28

        and bx,63*tailblen            ;Get low-order offset back again
        pop ax                        ;retrieve vol diff
        and ax,3                      ;get lowest 2 sign bits
        je eo_hilow                   ;if zero, done
eo_h_nohisig:
        shl ax,11                     ;mult. by 2048
        add bx,ax

        upd add,noop,0
        upd add,noop,4
        upd add,noop,8
        upd add,noop,12
        upd add,noop,16
        upd add,noop,20
        upd add,noop,24
        upd add,noop,28

        jmp eo_hilow

eo_low:                               ;downgoing edge
        neg al
        test ax,48
        je eo_l_nohisig               ;No hi significant bits to include
        push ax                       ;store vol difference
        and ax,60                     ;Leave out 2 lowest bits
        shl ax,9                      ;Mult by 512; * 4 = 2048
        add bx,ax                     ;True offset into table

        upd sub,shf,0
        upd sub,shf,4
        upd sub,shf,8
        upd sub,shf,12
        upd sub,shf,16
        upd sub,shf,20
        upd sub,shf,24
        upd sub,shf,28

        and bx,63*tailblen            ;Get low-order offset back again
        pop ax                        ;retrieve vol diff
        and ax,3                      ;get lowest 2 sign bits
        je eo_hilow                   ;if zero, done
eo_l_nohisig:
        shl ax,11                     ;mult. by 2048
        add bx,ax

        upd sub,noop,0
        upd sub,noop,4
        upd sub,noop,8
        upd sub,noop,12
        upd sub,noop,16
        upd sub,noop,20
        upd sub,noop,24
        upd sub,noop,28

eo_hilow:
        movzx ax,_soundlastamp
        add al,al
        add al,_soundlastamp
        add ax,08026h                   ;0x26 is base level, (3*15)*3=180 is nice
        xchg ah,al
        mov word ptr [offset _soundlastval],ax
        mov word ptr [offset _soundlastval+2],ax
eo_none:
        mov edi,_soundbufptr            ;we could assume that soundbufbase is 0-based, but well...
        sub edi,_soundbuflen
        jncsh eo_movedown
eo_done:
        ret

eo_movedown:
        mov _soundbufptr,edi            ;new soundbufptr
        mov ebx,_soundbuflen
        lfs di,_soundbufbase
        mov dx,tailblen/2               ;twice the tail length (in dwords)
eo_movedown2:
        mov eax,fs:[edi+ebx]            ;get 4 bytes from above
        mov fs:[edi],eax                ;store here
        add edi,4
        dec dx
        jne eo_movedown2
        ret



eo_initialise:                        ;Initialise sound variables: we are starting to
        mov _soundlastthi,eax         ; record sounds
        mov _soundlasttlo,edx
        xor eax,eax
        mov _soundsilent,ax           ;we're not silent now
        mov _soundsilentq,ax
        mov ax,_soundnextblock        ;next free block number
        mov _soundcurblk,ax
        mov edx,_BytesPerBlock
        mul edx
        mov _soundbufptr,eax
        xor edi,edi
        lfs di,_soundbufbase
        add edi,eax                   ;point to start of free block
        mov eax,_soundlastval
        mov fs:[edi],eax              ;clear out very first of buffer (tailblen)
        mov fs:[edi+4],eax
        mov fs:[edi+8],eax
        mov fs:[edi+12],eax
        mov fs:[edi+16],eax
        mov fs:[edi+20],eax
        mov fs:[edi+24],eax
        mov fs:[edi+28],eax
        mov eax,_soundlastthi
        mov edx,_soundlasttlo
        jmp eo_noisy


eo_longtime:
        call flattenbuffer
        movzx ebx,es:_TStatesPerSample  ;Used in code above
        xor eax,eax                     ;# of samples to skip (0), EDX is remainder still
        xor edi,edi
        lfs di,_soundbufbase
        add edi,_soundbufptr
        jmp eo_noskip


eo_longtime_2:                        ;entry point if _soundtimehi==_soundlastthi
        call flattenbuffer_2          ;eax, edx are set up correctly already
        movzx ebx,es:word ptr _TStatesPerSample
        xor eax,eax
        xor edi,edi
        lfs di,_soundbufbase
        add edi,_soundbufptr
        jmp eo_noskip






flattenbuffer:
;
;uses: eax,ebx,edx,fs,edi
;expects: _soundtime(lo/hi), _soundlastval
;updates values of soundlastt(lo/hi), very close to _soundtime(lo/hi)
;returns T states left in edx
;doesn't send messages to C of course
;may only be called when _sound is true
;
        mov eax,_soundtimehi
        sub eax,_soundlastthi
        imul _tframe                  ;if delta T>5 minutes, DX != 0
        add eax,_soundtimelo          ;Can be <0 too if called from AYupdate
        adc edx,0
        sub eax,_soundlasttlo
        sbb edx,0
        movzx ebx,es:word ptr _TStatesPerSample
        div ebx                       ;eax=# of samples to skip
flattenbuffer_2:
        push edx                      ;store # of T states left
        push eax                      ;store # of samples to skip
        mov ebx,eax
eo_flattenbfr:
        cmp _soundsilentq,2           ;whole buffer empty?
        je eo_flattenfinished         ;then we're done
        test ebx,ebx
        je eo_flattenfinished         ;if no bytes to be skipped, finished
        mov eax,_soundbuflen
        sub eax,_soundbufptr          ;number of bytes to end of buffer
        jbe eo_fb_fillnone
        shr eax,1                     ;convert to # samples
        cmp eax,ebx
        jbesh eo_fb_tillend
        mov eax,ebx                   ;eax is lesser of the two
eo_fb_tillend:
        sub ebx,eax                   ;now ebx is # samples yet to be done
eo_fb_fillnone2:
        xor edi,edi
        lfs di,_soundbufbase
        add edi,_soundbufptr
        mov edx,_soundlastval
eo_fb_bytes1:
        test eax,eax
        je eo_fb_end
        test edi,3
        jesh eo_fb_dwords
        mov fs:[edi+tailblen],dx
        add di,2
        dec eax
eo_fb_dwords:
        sub eax,32                    ;eax=# of samples, not bytes
        jb short eo_fb_bytes2
eo_fb_dwords2:
        mov fs:[edi+tailblen+0],edx
        mov fs:[edi+tailblen+4],edx
        mov fs:[edi+tailblen+8],edx
        mov fs:[edi+tailblen+12],edx
        mov fs:[edi+tailblen+16],edx
        mov fs:[edi+tailblen+20],edx
        mov fs:[edi+tailblen+24],edx
        mov fs:[edi+tailblen+28],edx
        mov fs:[edi+tailblen+32],edx
        mov fs:[edi+tailblen+36],edx
        mov fs:[edi+tailblen+40],edx
        mov fs:[edi+tailblen+44],edx
        mov fs:[edi+tailblen+48],edx
        mov fs:[edi+tailblen+52],edx
        mov fs:[edi+tailblen+56],edx
        mov fs:[edi+tailblen+60],edx
        add edi,64
        sub eax,32
        jae eo_fb_dwords2
eo_fb_bytes2:
        add eax,32
        jesh eo_fb_end
eo_fb_bytes2a:
        mov fs:[edi+tailblen],dx
        add edi,2
        dec eax
        jne eo_fb_bytes2a
eo_fb_end:
        mov ax,word ptr _soundbufbase
        sub edi,eax
        mov _soundbufptr,edi
        sub edi,_soundbuflen
        jb eo_flattenfinished
        mov _soundbufptr,edi          ;we wrapped
        mov di,ax                     ;ax still holds _soundbufbase
        push ebx
        mov ebx,_soundbuflen          ;copy tail to start of buffer (2*tailblen bytes)

cpy     macro num
        mov edx,fs:[edi+ebx+num]
        mov fs:[edi+num],edx
        endm

        cpy 0
        cpy 4
        cpy 8
        cpy 12
        cpy 16
        cpy 20
        cpy 24
        cpy 28
        cpy 32
        cpy 36
        cpy 40
        cpy 44
        cpy 48
        cpy 52
        cpy 56
        cpy 60

        pop ebx
        mov ax,_soundnooutyet
        mov _soundsilentq,ax          ;if new part was silent, now whole bfr silent
;        mov _soundnooutyet,2          ;new part is silent (2 signifies: empty from start)
        mov _soundnooutyet,1
        jmp eo_flattenbfr
eo_flattenfinished:
        mov eax,ebx                   ;get # of bytes left
        add eax,_soundbufptr          ;make new soundbufptr
        xor edx,edx
        div _soundbuflen
        mov _soundbufptr,edx          ;remainder
        pop eax                       ;# of samples skipped
        xor edx,edx
        mov dx,es:_TStatesPerSample
        mul edx                       ;edxeax = # of T states skipped
        add eax,_soundlasttlo         ;add previous time
        adc edx,0                     ;include carry
        div _tframe                   ;# of frames skipped + low order old time
        add _soundlastthi,eax         ;add them to the relevant counter
        mov _soundlasttlo,edx         ;put remainder in low counter
        pop edx                       ;# of T states from cur ptr to cur time
        ret

eo_fb_fillnone:
        xor eax,eax
        jmp eo_fb_fillnone2





;****************************************************************************
;
;Sound routines
;
;****************************************************************************

;env cntr shuts up when envelope becomes straight
;                  when all amps have env bit 0 at 16-period time
;env cntr starts when an amp is written with env bit 1
;chn cntr shuts up when env reaches constant zero
;                  when mixer reg is being written
;                  when amp reg is being written
;                  when env ctrl reg is begin written to
;chn cntr starts when amp reg is written to
;                when mixer reg is written to
;                when env ctrl reg is written to
;noise cntr shuts up when mixer reg is written to and noise ctrl bits are all 1
;noise cntr starts when mixer reg is written to and noise ctrl bits are not all 1
;

;Possible problem when freq is changed at envelop=0 but not stationary: then
; freq generator is turned off and not turned on again until amp reg write
; or mixer reg write or env reg write

;Problem: daley thompsons decathlon.  Gebruikt envelope?  (Lijkt toch goed)
;Problem: chainreaction.  Is stil met veel edges (400)




AYupd_bigtime:                         ;update over frame boundary
        mov ebx,edx                    ;Store _soundtimelo for a while
        mov edx,_AYtimehi
        mov _soundtimehi,edx           ;Set 'edge time' to _AYtime (first hi)
        mov _AYtimehi,eax              ;Set new AY time to current time
        sub eax,edx                    ;current _soundtimehi minus old _AYtimehi
        imul _tframe                   ;EDX is assumed to become zero after
                                       ; rippled carry, so ignore
        add eax,ebx                    ;Add current time (lo)
;
        adc edx,0
        sub eax,_AYtimelo
        sbb edx,0
;        jns ayupd_pos
;        int 3
;ayupd_pos:
;
        mov edx,_AYtimelo
        mov _soundtimelo,edx           ;Now _soundtime is old _AYtime
;        sub eax,edx                    ;Now eax is time difference
        mov _AYtimelo,ebx              ;now _AYtime is curr time
        mov _AYcountcur,eax
        cmp eax,150000
        jbe AYloop                     ;continue if not incredibly large difference
        mov eax,_tframe
        add eax,eax             ;_tframe * 2, approximately 150000 T's
        mov _AYcountcur,eax
        mov ebx,_AYtimehi       ;curr time (hi) at this point
        sub ebx,2
        mov _soundtimehi,ebx    ;set edge time to 2 * _tframe T before curr
        mov ebx,_AYtimelo
        mov _soundtimelo,ebx    ;same (lo)
        jmp AYloop



AYupdate:
;Is entered with eax/edx holding curr time, also in _soundtime(hi/lo).
;Leaves with _soundtime(hi/lo) unchanged;  _AYcountcur=0
;During execution, _soundtime holds time of edge being handled (_AYtime to curr)
;Updates AY chip from time=_AYtime to time=_soundtime.
;Destroys eax,ebx,edx,edi,fs
;Should be called at least once every 20 ms.  If interval is too large,
; the entire interval is skipped.
        cmp _AYemul,0
        je AYdone1
        push eax
        push edx
        cmp eax,_AYtimehi
        jne AYupd_bigtime

        mov eax,_AYtimelo
        mov _soundtimelo,eax
        mov _AYtimelo,edx
        sub edx,eax
        mov _AYcountcur,edx

AYloop: mov edx,_AYcountcur            ;Find first event
        mov bx,offset AYdone
        cmp edx,_AYcounte
        jaesh AYenv
AYnoenv:
        cmp edx,_AYcounta
        jaesh AYcha
AYnoa:
        cmp edx,_AYcountb
        jaesh AYchb
AYnob:
        cmp edx,_AYcountc
        jaesh AYchc
AYnoc:
        cmp edx,_AYcountn
        jae AYn
        jmp bx                  ;edx holds curr time-from-start!  Necc for AYcomputeamps

AYenv:
        mov edx,_AYcounte
        mov bx,offset AYe
        jmp AYnoenv
AYcha:
        mov edx,_AYcounta
        mov bx,offset AYa
        jmp AYnoa
AYchb:
        mov edx,_AYcountb
        mov bx,offset AYb
        jmp AYnob
AYchc:
        mov edx,_AYcountc
        mov bx,offset AYc
        jmp AYnoc


AYdone:                         ;update _AYcount* variables to hold time-to-
        sub _AYcounte,edx       ; first-event
        sub _AYcounta,edx
        sub _AYcountb,edx
        sub _AYcountc,edx
        sub _AYcountn,edx
        pop _soundtimelo        ;restore these to curr time
        pop _soundtimehi
AYdone1:
        ret

AYe:    movzx eax,word ptr [offset _soundregs+11]       ;get env counter
        sub ax,1
        adc ax,1
        shl eax,5                       ;times 32
        add _AYcounte,eax
        mov ax,word ptr [offset _AYevol]        ;and _AYeadd
        add al,ah
        cmp ax,0150h                    ;overflow attack
        je AYe_stop
        cmp ax,0ff3fh                   ;overflow decay
        je AYe_stop
        mov _AYevol,al
AYe_cont:
        call AYcomputeamps
        jmp AYloop

AYe_stop:
        mov ax,word ptr [offset _AYevol2]
        mov word ptr [offset _AYevol],ax
        test ah,ah
        je AYe_stopstop
        cmp _AYetoggle,0
        je AYe_notog
        neg ah                          ;change 0140 <-> FF4F
        xor al,0fh
        mov word ptr [offset _AYevol2],ax
AYe_notog:
        test dword ptr [offset _soundregs+8],0101010h   ;anybody uses my output?
        jne AYe_cont                                    ;jump back if so
AYe_stopstop:
        mov _AYcounte,-1                ;infinity.  Leave other vars be!
        cmp byte ptr [offset _AYevol],0 ;constant zero?
        jne AYe_cont                    ;if so, take out chans that use env
        mov bl,byte ptr [offset _soundregs+7]   ;get noise mask bits
        test byte ptr [offset _soundregs+8],16
        je AYe_s_noa                    ;not using envelope
        mov _AYcounta,-1
        or bl,8                         ;signal: chan A noise not used
AYe_s_noa:
        test byte ptr [offset _soundregs+9],16
        je AYe_s_nob
        mov _AYcountb,-1
        or bl,16
AYe_s_nob:
        test byte ptr [offset _soundregs+10],16
        je AYe_s_noc
        mov _AYcountc,-1
        or bl,32
AYe_s_noc:
        not bl
        and bl,038h
        jne AYe_cont                    ;noise being used
        mov _AYcountn,-1                ;noise not being used
        jmp AYe_cont




aycomputeamp macro channel
;
;Quickly computes new amplitude, then jumps to AYloop
;Also shuts up counter for channels a,b,c if nothing is output
;
        mov al,byte ptr [offset _soundregs+7]   ;get mask register
        or al,_AYbitout                         ;get noise and tone bit outputs
        not al
        and al,9 shl channel             ;both Noise and Tone were 1 => output 1
        setnz ah
        dec ah
        and ah,byte ptr [offset _AYvola+channel] ;compute output amplitude: env included
        mov byte ptr [offset _AYampa+channel],ah
;Next lines are copied from start of enteredge
;Changes: EDX is important!

        push edx

        mov ax,word ptr [offset _AYampa]
        add al,ah
        add al,byte ptr [offset _AYampc]
        and ax,63                       ;A+B+C, remove bit 6 (envelope bit)
        mul word ptr _AYvolume          ;16*0=min, 16*16 = max volume
        add ah,byte ptr [offset _feamp]
        add ah,_specdrumval
        mov al,ah

        pop edx

        sub al,_soundlastamp
        je AYloop                       ;No change in signal level
        mov _soundlastamp,ah
        mov _sounddiff,al
        push _soundtimelo
        add edx,_soundtimelo            ;get current time (base + edx)
        mov _soundtimelo,edx            ;necc for poss call to flattenbuffer
        mov eax,_soundtimehi
        call enteredge_entry2
        pop _soundtimelo
        jmp AYloop

        endm



AYa:    mov ax,word ptr [offset _soundregs]     ;get A counter
        and eax,4095
        shl eax,4
        movzx ebx,_TStatesPerSample
        cmp eax,ebx
        jbe AYa_f
        add _AYcounta,eax
        xor _AYbitout,1                         ;toggle A bit
AYa_ff:
        aycomputeamp 0                  ;computes new amp, then call enteredge

AYa_f:
        mov _AYcounta,-1                ;infinity
        or _AYbitout,1
        jmp AYa_ff

AYb:    mov ax,word ptr [offset _soundregs+2]   ;get B counter
        and eax,4095
        shl eax,4
        movzx ebx,_TStatesPerSample
        cmp eax,ebx
        jbe AYb_f
        add _AYcountb,eax
        xor _AYbitout,2                         ;toggle B bit
AYb_ff:
        aycomputeamp 1                  ;computes new amp, then call enteredge

AYb_f:
        mov _AYcountb,-1                ;infinity
        or _AYbitout,2
        jmp AYb_ff

AYc:    mov ax,word ptr [offset _soundregs+4]   ;get C counter
        and eax,4095
        shl eax,4
        movzx ebx,_TStatesPerSample
        cmp eax,ebx
        jbe AYc_f
        add _AYcountc,eax
        xor _AYbitout,4                         ;toggle C bit
AYc_ff:
        aycomputeamp 2                  ;computes new amp, then call enteredge

AYc_f:
        mov _AYcountc,-1                ;infinity
        or _AYbitout,4
        jmp AYc_ff

AYn:    mov al,byte ptr [offset _soundregs+6]   ;get noise counter
        and eax,31
        shl eax,5
        movzx ebx,_TStatesPerSample
        cmp eax,ebx
        ja AYn_nf
        mov eax,ebx
AYn_nf:
        mov ebx,_AYcountn
        add _AYcountn,eax
        movzx eax,_AYnseed
        inc eax
        mov edx,75
        mul edx
        dec eax
        mov edx,eax
        shr edx,16
        sub ax,dx
        adc ax,0                        ;mod 65537, that is
        mov _AYnseed,ax

        and al,4
        dec al
        and al,8+16+32
        xor _AYbitout,al                ;toggle noise bit randomly

        mov edx,ebx                     ;restore curr time
        mov al,byte ptr [offset _soundregs+7]   ;get mask register
        or al,_AYbitout                         ;get noise and tone bit outputs
        not al
        test al,9 shl 2                         ;both Noise and Tone were 1 => output 1
        setnz bl
        dec bl
        shl ebx,16
        test al,9 shl 1
        setnz bh
        dec bh
        test al,9 shl 0
        setnz bl
        dec bl
        and ebx,dword ptr [offset _AYvola]
        mov word ptr [offset _AYampa],bx
        mov eax,ebx
        shr eax,16
        mov _AYampc,al

        push edx

        mov ax,word ptr [offset _AYampa]
        add al,ah
        add al,byte ptr [offset _AYampc]
        and ax,63                       ;A+B+C, remove bit 6 (envelope bit)
        mul word ptr _AYvolume          ;16*0=min, 16*16 = max volume
        add ah,byte ptr [offset _feamp]
        add ah,_specdrumval
        mov al,ah

        pop edx

        sub al,_soundlastamp
        je AYloop                       ;No change in signal level
        mov _soundlastamp,ah
        mov _sounddiff,al
        push _soundtimelo
        add edx,_soundtimelo            ;get current time (base + edx)
        mov _soundtimelo,edx            ;necc for poss call to flattenbuffer
        mov eax,_soundtimehi
        call enteredge_entry2
        pop _soundtimelo
        jmp AYloop




AYcomputeamps:                          ;EDX must hold time-from-base (soundtime)
;
;Computes output amps.  Also computes _AYvol* using amp regs and env state
;Also shuts up channels if necessary
;Does not shut up channels that use envelope
;
        push edx
        mov eax,dword ptr [offset _soundregs+7] ;get mask reg and amp regs
        and eax,01f1f1fffh                      ;mask out envelope+amp bits
        mov dh,al                               ;save mask reg
        or al,_AYbitout                         ;compute outputs
        mov dl,al                               ;store it too
        not dx
        test ah,16
        jesh AYnoaenv
        mov ah,_AYevol
AYnoaenv:
        test ah,ah              ;running envelope has bit 6=1, never shut off those
        jne AYsounda
        mov _AYcounta,-1        ;shut off chan A
        and dh,255-8            ;signal: noise chan A not used
        mov _AYvola,ah
        mov _AYampa,ah
        jmp AYca_acont
AYsounda:
        test dh,1               ;chan A tone not used? Here 0 means not used
        jnesh AYsounda1         ;jump forward if used
        mov _AYcounta,-1
AYsounda1:
        mov _AYvola,ah
        test dl,9
        setnz bh
        dec bh
        and ah,bh
        mov _AYampa,ah
AYca_acont:
        shr eax,16
        test al,16
        jesh AYnobenv
        mov al,_AYevol
AYnobenv:
        test al,al
        jne AYsoundb
        mov _AYcountb,-1        ;shut off chan B
        and dh,255-16
        jmp AYca_bcont
AYsoundb:
        test dh,2               ;chan B tone not used? Here 0 means not used
        jnesh AYsoundb1         ;jump forward if used
        mov _AYcountb,-1
AYsoundb1:
        test dl,9 shl 1
        setnz bl
        dec bl
AYca_bcont:
        test ah,16
        jesh AYnocenv
        mov ah,_AYevol
AYnocenv:
        test ah,ah
        jne AYsoundc
        mov _AYcountc,-1        ;shut off chan C
        and dh,255-32
        jmp AYca_ccont
AYsoundc:
        test dh,4               ;chan C tone not used? Here 0 means not used
        jnesh AYsoundc1         ;jump forward if used
        mov _AYcountc,-1
AYsoundc1:
        test dl,9 shl 2
        setnz bh
        dec bh
AYca_ccont:
        mov word ptr [offset _AYvolb],ax
        and ax,bx
        mov word ptr [offset _AYampb],ax
        and dh,038h
        sub dh,1
        sbb edx,edx
        or _AYcountn,edx        ;shut off noise if possible
        pop edx
;next code is copied from start of enteredge

        push edx

        mov ax,word ptr [offset _AYampa]
        add al,ah
        add al,byte ptr [offset _AYampc]
        and ax,63                       ;A+B+C, remove bit 6 (envelope bit)
        mul word ptr _AYvolume          ;16*0=min, 16*16 = max volume
        add ah,byte ptr [offset _feamp]
        add ah,_specdrumval
        mov al,ah

        pop edx

        sub al,_soundlastamp
        je AYcadone                     ;No change in signal level
        mov _soundlastamp,ah
        mov _sounddiff,al
        push _soundtimelo
        add edx,_soundtimelo            ;get current time (base + edx)
        mov _soundtimelo,edx            ;necc for poss call to flattenbuffer
        mov eax,_soundtimehi
        call enteredge_entry2
        pop _soundtimelo
AYcadone:
        ret
        jmp AYloop





updatesound_f:                        ;called from CORE.ASM
global updatesound_f: proc
        mov ebp,dword ptr [offset rreg]
        computeloctime
        call AYupdate
        call flattenbuffer
        retf





;*********************************************************************

;Following is the routine that does the updating when the AY registers
;are changed.  Called from C

;*********************************************************************


;
;void AYout(BYTE value)
;
_AYout:
public _AYout
        push bp
        mov cx,sp               ;To save it from corrupting by computeloctime
        mov ax,ds
        mov es,ax
        push ds
        push di
        inc _DBGnumouts         ;************
        cmp _sound,0
        je AYout_nosound        ;Jump to just store the value OUTed
        cmp _AYemul,0
        je AYout_nosound        ;Same
        mov ebp,dword ptr rreg
        computeloctime
        mov bp,cx
        call AYupdate
        mov ax,[bp+6]
        mov bx,_fffdstate
        and ebx,15
        cmp bl,11
        jae ayo_env             ;11-13: envelope
        cmp bl,7
        jb ayo_tonenoise        ;0-6: tone & noise pitches
        je ayo_mixer            ;mixer control
;
;Amplitude code (8-10)
;
        and al,01fh             ;amplitude (8-10).  Sieve away irrelevant bits
        mov byte ptr [offset _soundregs+bx],al
        test al,010h            ;Using envelope?
        je ayo_amp_noenv        ;If not jump forward.  Check whether to start env cntr:
        test word ptr [offset _AYcounte+2],0fff0h
        je ayo_noreact_env      ;If counting, jump forward
        cmp _AYeadd,0           ;Constant volume?
        je ayo_noreact_env      ;If so, jump forward
        mov _AYcounte,0         ;Restart counter
ayo_noreact_env:
                                ;Counter set to infinity?
        test word ptr [offset _AYcounta-32+2+4*ebx],0fff0h
        je ayo_amp_norestart    ;Do not restart if not
        mov dword ptr [offset _AYcounta-32+4*ebx],0
ayo_amp_norestart:
        cmp bl,10               ;10->nc, 8,9->c
        sbb bl,6                ;10->4, 9->2, 8->1
        shl bl,3
        test byte ptr [offset _soundregs+7],bl  ;noise active for this channel?
        jne ayo_compamps        ;jump if not
        test word ptr [offset _AYcountn+2],0fff0h
        je ayo_compamps
        mov _AYcountn,0         ;reactivate noise. (Shut up by AYcomputeamps)
ayo_compamps:
        xor edx,edx             ;time-from-curr-time (0)
        call AYcomputeamps      ;This shuts off channels whenever possible
_AYoexit:
        pop di
        pop ds
        pop bp
        retf

ayo_amp_noenv:
        cmp al,0                ;amplitude zero?
        je ayo_compamps         ;if so, take out this channel
        jmp ayo_noreact_env     ;if not, check whether chan/noise must be restarted




AYout_nosound:                  ;Just store the value OUTed, do not emulate
        mov bp,cx               ; the AY chip.
        mov ax,[bp+6]
        mov bx,_fffdstate
        and ebx,15
        mov byte ptr [offset _soundregs+bx],al
        pop di
        pop ds
        pop bp
        retf




;
;mixer code (7)
;
ayo_mixer:
        mov byte ptr [offset _soundregs+bx],al
        mov ebx,-1
        test al,1
        jne ayo_m_seta          ;deactivate
        cmp _AYvola,0
        je ayo_m_seta
        test word ptr [offset _AYcounta+2],0fff0h
        je ayo_m_noseta
        xor ebx,ebx             ;activate
ayo_m_seta:
        mov _AYcounta,ebx
ayo_m_noseta:
        mov ebx,-1
        test al,2
        jne ayo_m_setb          ;deactivate
        cmp _AYvolb,0
        je ayo_m_setb
        test word ptr [offset _AYcountb+2],0fff0h
        je ayo_m_nosetb
        xor ebx,ebx             ;activate
ayo_m_setb:
        mov _AYcountb,ebx
ayo_m_nosetb:
        mov ebx,-1
        test al,4
        jne ayo_m_setc          ;deactivate
        cmp _AYvolc,0
        je ayo_m_setc
        test word ptr [offset _AYcountc+2],0fff0h
        je ayo_m_nosetc
        xor ebx,ebx             ;activate
ayo_m_setc:
        mov _AYcountc,ebx
ayo_m_nosetc:
        test word ptr [offset _AYcountn+2],0fff0h
        je ayo_compamps
        mov _AYcountn,0         ;activate noise.
        jmp ayo_compamps

;
;tone/noise period code (0-6)
;
ayo_tonenoise:
        mov cx,bx
        and bx,6
        mov dx,[offset _soundregs+bx]   ;get old timer value
        xchg bx,cx
        mov [offset _soundregs+bx],al   ;set new
        xchg bx,cx
        mov ax,[offset _soundregs+bx]   ;get new
        je ayo_a                        ;BL=0?
        cmp bl,2
        je ayo_b
        cmp bl,4
        je ayo_c
        jmp _AYoexit            ;Do not force update with noise: times are
                                ; short anyway.

ayo_a:
        and edx,4095
        shl edx,4
        sub edx,_AYcounta       ;This is the up-counter value
        jb ayo_a_resetcounter   ;Happens when counter is set to infinity
        and eax,4095
        shl eax,4
        cmp eax,edx             ;Do nothing if new counter is high enough
        jae _AYoexit
ayo_a_resetcounter:
        cmp _AYvola,0
        je _AYoexit             ;Do not reset counter when volume is zero
        test byte ptr [offset _soundregs+7],1
        jne _AYoexit            ;Neither when output is turned off
        mov _AYcounta,0         ;Force toggle next time AYupdate is called
        jmp _AYoexit

ayo_b:
        and edx,4095
        shl edx,4
        sub edx,_AYcountb       ;This is the up-counter value
        jb ayo_b_resetcounter   ;Happens when counter is set to infinity
        and eax,4095
        shl eax,4
        cmp eax,edx             ;Do nothing if new counter is high enough
        jae _AYoexit
ayo_b_resetcounter:
        cmp _AYvolb,0
        je _AYoexit             ;Do not reset counter when volume is zero
        test byte ptr [offset _soundregs+7],2
        jne _AYoexit            ;Neither when output is turned off
        mov _AYcountb,0         ;Force toggle next time AYupdate is called
        jmp _AYoexit

ayo_c:
        and edx,4095
        shl edx,4
        sub edx,_AYcountc       ;This is the up-counter value
        jb ayo_c_resetcounter   ;Happens when counter is set to infinity
        and eax,4095
        shl eax,4
        cmp eax,edx             ;Do nothing if new counter is high enough
        jae _AYoexit
ayo_c_resetcounter:
        cmp _AYvolc,0
        je _AYoexit             ;Do not reset counter when volume is zero
        test byte ptr [offset _soundregs+7],4
        jne _AYoexit            ;Neither when output is turned off
        mov _AYcountc,0         ;Force toggle next time AYupdate is called
        jmp _AYoexit

;
;envelope (period & ctrl) (11-13)
;
ayo_env:
        cmp bl,13
        jb ayo_eperiod
        je ayo_ectrl
        mov [offset _soundregs+bx],al
        jmp _AYoexit

ayo_eperiod:
        movzx edx,word ptr [offset _soundregs+11]   ;get old timer value
        mov [offset _soundregs+bx],al               ;set new
        movzx eax,word ptr [offset _soundregs+11]   ;get new
        shl edx,5
        sub edx,_AYcounte       ;This is the up-counter value
        jb ayo_e_resetcounter   ;Happens when counter is set to infinity
        shl eax,5
        cmp eax,edx             ;Do nothing if new counter is high enough
        jae _AYoexit
ayo_e_resetcounter:
        cmp _AYeadd,0           ;Envelope output constant?
        je _AYoexit
        mov _AYcounte,0         ;Force toggle next time AYupdate is called
        jmp _AYoexit

ayo_ectrl:
        mov byte ptr [offset _soundregs+13],al
        mov bx,ax
        and ebx,15
        mov eax,dword ptr [envtable+4*ebx]
        mov dword ptr _AYevol,eax
        and bx,2                                ;alternate bit
        mov _AYetoggle,bx
        mov _AYcounte,0                         ;reset counter
        test byte ptr [offset _soundregs+8],010h
        je ra_nota
        test word ptr [offset _AYcounta+2],0fff0h       ;set to infinity?
        je ra_notas
        mov _AYcounta,0
ra_notas:
        test byte ptr [offset _soundregs+7],08h         ;noise chan a active
        jne ra_nota                                     ;jmp forward if not
        test word ptr [offset _AYcountn+2],0fff0h       ;set to infinity?
        je ra_nota
        mov _AYcountn,0
ra_nota:
        test byte ptr [offset _soundregs+9],010h
        je ra_notb
        test word ptr [offset _AYcountb+2],0fff0h
        je ra_notbs
        mov _AYcountb,0
ra_notbs:
        test byte ptr [offset _soundregs+7],010h        ;noise chan b active
        jne ra_notb                                     ;jmp forward if not
        test word ptr [offset _AYcountn+2],0fff0h       ;set to infinity?
        je ra_notb
        mov _AYcountn,0
ra_notb:
        test byte ptr [offset _soundregs+10],010h
        je ra_notc
        test word ptr [offset _AYcountc+2],0fff0h
        je ra_notcs
        mov _AYcountc,0
ra_notcs:
        test byte ptr [offset _soundregs+7],020h        ;noise chan c active
        jne ra_notc                                     ;jmp forward if not
        test word ptr [offset _AYcountn+2],0fff0h       ;set to infinity?
        je ra_notc
        mov _AYcountn,0
ra_notc:
        xor edx,edx
        call AYcomputeamps                      ;possibly an amp changed
        jmp _AYoexit



;
;void decreaseamplitude(BYTE far*, WORD)
;
;Actually, it changes words into bytes
;The 'length' field is the number of samples to process, not total byte length
;
_decreaseamplitude:
public _decreaseamplitude
        push bp
        mov bp,sp
        push ds
        push si
        push di
        xor esi,esi
        lds si,[bp+6]
        mov edi,esi
        mov cx,[bp+10]
        shr cx,2
decampl:
        mov eax,[si]
        mov dl,ah
        shr eax,16
        mov al,dl
        mov [di],ax
        mov eax,[si+4]
        mov dl,ah
        shr eax,16
        mov al,dl
        mov [di+2],ax
        add si,8
        add di,4
        dec cx
        jne decampl
        pop di
        pop si
        pop ds
        pop bp
        retf



;
;void SpecDRUMout(BYTE value)
;
_SpecDRUMout:
public _SpecDRUMout
        push bp
        mov bp,sp
        mov ax,ds
        mov es,ax
        push ds
        push di
        push word ptr [bp+6]    ;save new value on stack
        mov ebp,dword ptr rreg
        computeloctime
        call AYupdate
        mov cl,byte ptr _specdrumshift
        mov bx,07fh
        shl bx,cl
        pop ax
        mov _specdrumport,al
        add al,bh
        shr ax,cl
        mov _specdrumval,al
        call enteredge
        pop di
        pop ds
        pop bp
        retf



INOUT ends

end
