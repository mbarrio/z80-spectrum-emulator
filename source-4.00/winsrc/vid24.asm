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
_updatebitmaps24:
public _updatebitmaps24
        endm

ml_updatebitmapscopper macro
_updatebitmapscopper24:
public _updatebitmapscopper24
        endm

ml_updateborder macro
_updateborder24:
public _updateborder24
        endm




pix32   equ 32*3
pix64   equ 64*3
pix96   equ 96*3
pix128  equ 128*3
pix160  equ 160*3
pix192  equ 192*3
pix224  equ 224*3
pix256  equ 256*3


dobit macro diff
        local dobit1
        local dobit0
        shr al,1
        jnc short dobit0
        mov gs:[bp+diff-1],edx
        jsh dobit1
dobit0: mov gs:[bp+diff-1],ebx
dobit1:
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
        rol edx,8
        mov ebx,[4*ebx+offset _paperattr]
        rol ebx,8

ubms_dolines:                   ;colors in ch,dh; bits in dl; cntr in cl
        mov al,fs:[si]
        mov es:[di],al

        dobit 21
        dobit 18
        dobit 15
        dobit 12
        dobit 9
        dobit 6
        dobit 3
        shr al,1
        jnc short dobit0
        mov gs:[bp],dh
        rol edx,16
        mov gs:[bp+1],dx
        rol edx,16
        jsh dobit1
dobit0: mov gs:[bp],bh
        rol ebx,16
        mov gs:[bp+1],bx
        rol ebx,16
dobit1:

        add bp,4*8*3            ;skip 4x8 pixel columns
        add si,256
        add di,256              ;go to next line of blocklet
        dec cl                  ;decrease # lines
        jne ubms_dolines

        pop edx                 ;restore attr values
        pop cx                  ;restore horiz byte counter (ch) and line cntr
        pop di
        pop si
        pop bp
        add bp,8*3              ;go to next 8 pixel column
        inc si
        inc di
        ror edx,8
        dec ch
        jne ubms_dobyte

        pop bx                  ;restore touched flag ptr
        endm




dobit_cpr macro diff
        local dobit1
        local dobit0
        shr eax,1
        jnc short dobit0
        mov gs:[bp+diff-1],edx
        jsh dobit1
dobit0: mov gs:[bp+diff-1],ebx
dobit1:
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
        add bx,_flashoffset
        push edx
        mov edx,[4*ebx+offset _inkattr]
        rol edx,8
        mov ebx,[4*ebx+offset _paperattr]
        rol ebx,8

        dobit_cpr 21
        dobit_cpr 18
        dobit_cpr 15
        dobit_cpr 12
        dobit_cpr 9
        dobit_cpr 6
        dobit_cpr 3
        shr eax,1
        jnc short dobitc0
        mov gs:[bp],dh
        rol edx,16
        mov gs:[bp+1],dx
        jsh dobitc1
dobitc0: mov gs:[bp],bh
        rol ebx,16
        mov gs:[bp+1],bx
dobitc1:

        add bp,8*3              ;next pixel column
        pop edx                 ;get horiz byte counter and attr data
        dec ch                  ;decrease cntr, set Z flag accordingly
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
        endm

fixupcolor2 macro
        mov ebx,[4*ebx+offset _inkattr]         ;get colours
        endm

fbbmfillsome macro
        mov fs:[si],ebx
        mov fs:[si+3],ebx
        mov fs:[si+6],ebx
        mov fs:[si+9],ebx
        mov fs:[si+12],ebx
        mov fs:[si+15],ebx
        mov fs:[si+18],ebx
        mov fs:[si+21],ebx
        add si,24
        sub ax,4
        endm

fbbmfillsome2 macro
        mov fs:[si],ebx
        mov fs:[si+3],ebx
        mov fs:[si+6],ebx
        mov fs:[si+9],ebx
        mov fs:[si+12],ebx
        mov fs:[si+15],ebx
        mov fs:[si+18],ebx
        mov fs:[si+21],ebx
        add si,24
        add ecx,4
        sub ax,4
        endm





ml_explodebitmap macro
_explodebitmap24:
public _explodebitmap24
        endm

;Macro is still the 16-bit version!!  I suspect 24 bit doesn't happen

getcolorplaneoffset macro
        endm
 
exppix macro num
        mov eax,[si+2*num]
        mov ebx,eax
        shl ebx,16
        xchg bx,ax
        mov es:[di+4*num],ebx
        mov es:[di+4*num+4],eax
        mov es:[di+4*num+128],ebx
        mov es:[di+4*num+132],eax
        endm

explodeline macro
;zoom 1 line (32 pixels) by factor of 2
        exppix 0
        exppix 2
        exppix 4
        exppix 6
        exppix 8
        exppix 10
        exppix 12
        exppix 14
        exppix 16
        exppix 18
        exppix 20
        exppix 22
        exppix 24
        exppix 26
        exppix 28
        exppix 30
        add si,64
        add di,256
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




video24 segment byte use16 public 'CODE'


assume cs:video24

include c:\bc4\spectrum\asm\vidx.asm


video24 ends


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

