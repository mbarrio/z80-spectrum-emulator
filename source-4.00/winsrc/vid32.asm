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
_updatebitmaps32:
public _updatebitmaps32
        endm

ml_updatebitmapscopper macro
_updatebitmapscopper32:
public _updatebitmapscopper32
        endm

ml_updateborder macro
_updateborder32:
public _updateborder32
        endm




pix32   equ 32*4
pix64   equ 64*4
pix96   equ 96*4
pix128  equ 128*4
pix160  equ 160*4
pix192  equ 192*4
pix224  equ 224*4
pix256  equ 256*4


dobit macro diff
        local dobit1
        local dobit0
        shr al,1
        jnc short dobit0
        mov gs:[bp+diff],edx
        jsh dobit1
dobit0: mov gs:[bp+diff],ebx
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
        mov ebx,[4*ebx+offset _paperattr]

ubms_dolines:                   ;colors in ch,dh; bits in dl; cntr in cl
        mov al,fs:[si]
        mov es:[di],al

        dobit 28
        dobit 24
        dobit 20
        dobit 16
        dobit 12
        dobit 8
        dobit 4
        dobit 0

        add bp,4*8*4            ;skip 4x8 pixel columns
        add si,256
        add di,256              ;go to next line of blocklet
        dec cl                  ;decrease # lines
        jne ubms_dolines

        pop edx                 ;restore attr values
        pop cx                  ;restore horiz byte counter (ch) and line cntr
        pop di
        pop si
        pop bp
        add bp,8*4              ;go to next 8 pixel column
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
        mov gs:[bp+diff],edx
        jsh dobit1
dobit0: mov gs:[bp+diff],ebx
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
        mov ebx,[4*ebx+offset _paperattr]

        dobit_cpr 28
        dobit_cpr 24
        dobit_cpr 20
        dobit_cpr 16
        dobit_cpr 12
        dobit_cpr 8
        dobit_cpr 4
        dobit_cpr 0

        add bp,8*4              ;next pixel column
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
        mov fs:[si+4],ebx
        mov fs:[si+8],ebx
        mov fs:[si+12],ebx
        mov fs:[si+16],ebx
        mov fs:[si+20],ebx
        mov fs:[si+24],ebx
        mov fs:[si+28],ebx
        add si,32
        sub ax,4
        endm

fbbmfillsome2 macro
        mov fs:[si],ebx
        mov fs:[si+4],ebx
        mov fs:[si+8],ebx
        mov fs:[si+12],ebx
        mov fs:[si+16],ebx
        mov fs:[si+20],ebx
        mov fs:[si+24],ebx
        mov fs:[si+28],ebx
        add si,32
        add ecx,4
        sub ax,4
        endm




ml_explodebitmap macro
_explodebitmap32:
public _explodebitmap32
        endm

getcolorplaneoffset macro
        endm

exppix macro num
        mov eax,[si+4*num]
        mov es:[di+8*num],eax
        mov es:[di+8*num+4],eax
        mov es:[di+8*num+256],eax
        mov es:[di+8*num+260],eax
        endm

explodeline macro
;zoom 1 line (32 pixels) by factor of 2
        exppix 0
        exppix 1
        exppix 2
        exppix 3
        exppix 4
        exppix 5
        exppix 6
        exppix 7
        exppix 8
        exppix 9
        exppix 10
        exppix 11
        exppix 12
        exppix 13
        exppix 14
        exppix 15
        exppix 16
        exppix 17
        exppix 18
        exppix 19
        exppix 20
        exppix 21
        exppix 22
        exppix 23
        exppix 24
        exppix 25
        exppix 26
        exppix 27
        exppix 28
        exppix 29
        exppix 30
        exppix 31
        add si,128
        add di,512
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




video32 segment byte use16 public 'CODE'


assume cs:video32

include c:\bc4\spectrum\asm\vidx.asm


video32 ends


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

