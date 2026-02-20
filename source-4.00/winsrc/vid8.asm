.486p

borpartsize     equ 20          ; size of structure BORPART in spectrum.h

jsh equ jmp short

assume ds:SPECDATA

touched equ 12288
bitmaps equ 12288+192+192


addplanelen macro
        endm

calcplanelen macro
        endm

vid1special macro
        endm

vid1special2 macro
        endm

vid1special3 macro
        mov byte ptr ds:[bp+14],8  ;set color to 8 to denote 'border with structure'
        endm




ml_updatebitmaps macro
_updatebitmaps8:
public _updatebitmaps8
        endm

ml_updatebitmapscopper macro
_updatebitmapscopper8:
public _updatebitmapscopper8
        endm

ml_updateborder macro
_updateborder8:
public _updateborder8
        endm




pix32   equ 32
pix64   equ 64
pix96   equ 96
pix128  equ 128
pix160  equ 160
pix192  equ 192
pix224  equ 224
pix256  equ 256


dobit macro
        shr dl,1
        sbb al,al
        and al,ch
        xor al,dh
        endm


ubmsdoscr macro
        mov byte ptr es:[bx],1  ;set touched flag
        push bx                 ;save bx
        mov ch,4                ;4 horizontal bytes
ubms_dobyte:
        push bp
        push si
        push di
        push cx

        xor ebx,ebx
        mov bl,dl               ;get leftmost attr byte
        add bx,_flashoffset
        push edx
        mov edx,[4*ebx+offset _inkattr]
        mov ch,dl
        mov edx,[4*ebx+offset _paperattr]
        xor ch,dl
        mov dh,dl

ubms_dolines:                   ;colors in ch,dh; bits in dl; cntr in cl
        mov dl,fs:[si]
        mov es:[di],dl

        dobit
        shl eax,8
        dobit
        shl eax,8
        dobit
        shl eax,8
        dobit
        mov gs:[bp+4],eax
        dobit
        shl eax,8
        dobit
        shl eax,8
        dobit
        shl eax,8
        dobit
        mov gs:[bp],eax

        add bp,4*8              ;skip 3x8 pixel columns
        add si,256
        add di,256              ;go to next line of blocklet
        dec cl                  ;decrease # lines
        jne ubms_dolines

        pop edx                 ;restore attr values
        pop cx                  ;restore horiz byte counter (ch) and line cntr
        pop di
        pop si
        pop bp
        add bp,8                ;go to next 8 pixel column
        inc si
        inc di
        ror edx,8
        dec ch
        jne ubms_dobyte

        pop bx                  ;restore touched flag ptr
        endm




dobit_cpr macro
        shr eax,1
        sbb bl,bl
        and bl,ch
        xor bl,dh
        endm



ubmsdoblocknextline macro

ubms_doblock_nextline:
        push cx
        mov es:[di],eax         ;store bits
        mov es:[di+32],edx      ;store attr bytes
        mov ch,4

ubms_donextbyte:
        xor ebx,ebx
        mov bl,dl               ;get leftmost attr byte
        mov dl,ch               ;kinky: store horiz byte cntr in dl
        add bx,_flashoffset
        push edx
        mov edx,[4*ebx+offset _inkattr]
        mov ch,dl
        mov edx,[4*ebx+offset _paperattr]
        xor ch,dl
        mov dh,dl

        dobit_cpr
        shl ebx,8
        dobit_cpr
        shl ebx,8
        dobit_cpr
        shl ebx,8
        dobit_cpr
        mov gs:[bp+4],ebx
        dobit_cpr
        shl ebx,8
        dobit_cpr
        shl ebx,8
        dobit_cpr
        shl ebx,8
        dobit_cpr
        mov gs:[bp],ebx

        add bp,8                ;next pixel column
        pop edx                 ;get horiz byte counter and attr data
        dec dl                  ;decrease cntr, set Z flag accordingly
        mov ch,dl
        ror edx,8               ;move next attr byte in position
        jne ubms_donextbyte     ;Z flag is not affected by ROR instr

        pop cx
        add si,64
        add di,64
        dec cx
        je ubms_cpr_goscan
        mov eax,fs:[si]
        mov edx,fs:[si+32]
        jmp ubms_doblock_nextline

ubms_cpr_goscan:
        endm




fixupcolor macro
        mov ebx,[4*ebx+offset _inkattr]
        mov bh,bl
        mov ax,bx
        shl ebx,16
        mov bx,ax
        endm

fixupcolor2 macro
        mov ebx,[4*ebx+offset _inkattr]         ;get colours
        mov bh,bl
        push bx
        shl ebx,16
        pop bx
        endm

fbbmfillsome macro
        mov fs:[si],ebx
        mov fs:[si+4],ebx
        add si,8
        sub ax,4
        endm

fbbmfillsome2 macro
        mov fs:[si],ebx
        mov fs:[si+4],ebx
        add si,8
        add ecx,4
        sub ax,4
        endm




ml_explodebitmap macro
_explodebitmap8:
public _explodebitmap8
        endm

getcolorplaneoffset macro
        endm

expword macro num
        mov ax,[si+num]
        mov bx,ax
        mov bl,bh
        mov ah,al
        mov es:[di+2*num],ax
        mov es:[di+2*num+64],ax
        mov es:[di+2*num+2],bx
        mov es:[di+2*num+66],bx
        mov ax,[si+num+2]
        mov bx,ax
        mov bl,bh
        mov ah,al
        mov es:[di+2*num+4],ax
        mov es:[di+2*num+68],ax
        mov es:[di+2*num+6],bx
        mov es:[di+2*num+70],bx
        endm

explodeline macro
;zoom 1 line (32 pixels) by factor of 2
        expword 0
        expword 4
        expword 8
        expword 12
        expword 16
        expword 20
        expword 24
        expword 28
        add si,32
        add di,128
        endm





SPECDATA segment dword public 'DATA'

global xtsize: dword
global xtrest: dword
global ysize: word
global curbmptr: word

global _inkattr: dword
global _paperattr: dword
global _flashoffset: word

global scradrbuf: byte
global _copper: word
global _outbufptr: dword

SPECDATA ends

global _borpart: byte




video8 segment byte use16 public 'CODE'


assume cs:video8

include c:\bc4\spectrum\asm\vidx.asm


video8 ends


purge fbbmfillsome2
purge fbbmfillsome
purge fixupcolor2
purge fixupcolor
purge ubmsdoblocknextline
purge dobit_cpr
purge ubmsdoscr
purge dobit
purge ml_updateborder
purge ml_updatebitmapscopper
purge ml_updatebitmaps



end

