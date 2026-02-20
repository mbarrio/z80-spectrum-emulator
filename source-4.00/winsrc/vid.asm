.486p

borpartsize     equ 20          ; size of structure BORPART in spectrum.h

jsh equ jmp short

assume cs:CORE, ds:SPECDATA


SPECDATA segment dword public 'DATA'

curbmptr        dw ?            ;global variable used by _updatebitmaps
_flashoffset    dw 0            ;0 or 128
_inkattr        dd 256+128 dup (?)
_paperattr      dd 256+128 dup (?)

xtsize  dd ?                    ;all for _updateborder
xtrest  dd ?
ysize   dw ?
planelen dw ?                   ;for vid1 with bit planes
planeadd dw ?                   ;for vid1 with bit planes

public _inkattr
public _paperattr
public _flashoffset

public xtsize
public xtrest
public ysize
public curbmptr
public planelen
public planeadd


SPECDATA ends

;bitmaps are divided into 8 strips of 4x8 pixels wide, going all the way
;from top to bottom.  The 'touched' array keeps info about the 32 wide x
;8 high blocklets.  The calling C routine makes sure 'touched' cleared
;when necessary.  If visible is zero, the corresponding blocklet is not
;checked for changes (and touched stays false if it was before)

touched equ 12288
bitmaps equ 12288+192+192

global scradrbuf: byte
global _copper: word
global _outbufptr: dword
global _borpart: byte



CORE segment byte use16 public 'CODE'


;void touchflash(BYTE *vidbuf, BYTE *newdata)
_touchflash:
public _touchflash
        push bp
        mov bp,sp       ;bp -> bp,ret,ret,vbseg,vbptr,ndseg,ndptr
        push di
        push si
        push es

        les di,[bp+6]   ;video buf pointer; di, =ndx to screen
        lfs si,[bp+10]  ;new data, points to screen
        add di,6144
        add si,6144
        mov cx,768/4
        mov ebx,080808080h
tf_loop:
        mov eax,fs:[si]
        and eax,ebx
        not eax
        and es:[di],eax
        add si,4
        add di,4
        loop tf_loop

        pop es
        pop si
        pop di
        pop bp
        retf



;void touchflashcopper(BYTE *vidbuf, BYTE *newdata)
_touchflashcopper:
public _touchflashcopper
        push bp
        mov bp,sp       ;bp -> bp,ret,ret,vbseg,vbptr,ndseg,ndptr
        push di
        push si
        push es

        les di,[bp+6]   ;video buf pointer; di, =ndx to screen
        lfs si,[bp+10]  ;new data, points to vidbufbase
        mov ch,24
        mov ebx,080808080h
        add si,32
        add di,32
tf_loop1:
        mov cl,32/4
tf_loop2:
        mov eax,fs:[si]
        and eax,ebx
        not eax
        and es:[di],eax
        add si,4
        add di,4
        dec cl
        jne tf_loop2
        add si,512-32   ;skip 8 lines.  This will only fail if coppering
        add di,512-32   ; is such that top line is not flashing whereas
        dec ch          ; other lines are.  I must still see the first
        jne tf_loop1    ; program doing this.

        pop es
        pop si
        pop di
        pop bp
        retf





;void copyvidbuffer(BYTE *vidbuf, BYTE *newdata)
_copyvidbuffer:
public _copyvidbuffer
        push bp
        mov bp,sp       ;bp -> bp,ret,ret,vbseg,vbptr,ndseg,ndptr
        push di
        push si
        push es

        les di,[bp+6]   ;video buf pointer; di, =ndx to screen
        lfs si,[bp+10]  ;new data
        cmp _copper,0
        jne cvb_copper
        mov cx,6912/4   ;# of dwords to copy
cvb_dword:
        mov eax,fs:[si]
        mov es:[di],eax
        add si,4
        add di,4
        loop cvb_dword
        jmp short cvb_end

cvb_copper:
        mov cx,192      ;# of lines to copy
        mov bx,offset scradrbuf
cvb_lines:
        push cx
        push bx
        mov bx,[bx+2]   ;bx is offset relative to 0000 (rom)
        sub bx,16384
        mov cx,8
cvb_quads:
        mov eax,fs:[si+bx]
        mov es:[di],eax
        add di,4
        add bx,4
        loop cvb_quads
        pop bx
        push bx
        mov bx,[bx+4]
        sub bx,16384
        mov cx,8
cvb_quads2:
        mov eax,fs:[si+bx]
        mov es:[di],eax
        add di,4
        add bx,4
        loop cvb_quads2
        pop bx
        pop cx
        add bx,6
        loop cvb_lines

cvb_end:
        pop es
        pop si
        pop di
        pop bp
        retf



CORE ends

end


