.486p

borpartsize     equ 20          ; size of structure BORPART in spectrum.h

jsh equ jmp short

assume ds:SPECDATA

touched equ 12288
bitmaps equ 12288+192+192

x768    equ 4
x1536   equ 8
x2304   equ 12


calcplanelen macro
        mov ax,ds:[bp+4]
        shr ax,3                ;# pixels / 2 = 4 * # bytes in line
        mov planelen,ax
        add ax,ax
        add ax,planelen
        mov planeadd,ax         ;=3 * planelen
        endm

addplanelen macro
        add si,planeadd
        endm

vid1special macro
        push bp
        xor ebp,ebp             ;signal 'no structure'
        mov bp,planelen
        endm

vid1special2 macro
        local vs_ret
        pop bp
        test ebp,0ff000000h
        je short vs_ret
        mov byte ptr ds:[bp+14],8
vs_ret:
        endm

vid1special3 macro
        or ebp,0ff000000h
        endm




ml_updatebitmaps macro
_updatebitmaps1:
public _updatebitmaps1
        endm

ml_updatebitmapscopper macro
_updatebitmapscopper1:
public _updatebitmapscopper1
        endm

ml_updateborder macro
_updateborder1:
public _updateborder1
        endm




pix32   equ 4*4
pix64   equ 8*4
pix96   equ 12*4
pix128  equ 16*4
pix160  equ 20*4
pix192  equ 24*4
pix224  equ 28*4
pix256  equ 32*4



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
        mov edx,[4*ebx+offset _inkattr]         ;in this case 'andbytes'
        mov eax,[4*ebx+offset _paperattr]       ;in this case 'xorbytes'

ubms_dolines:
        push cx

        mov ch,fs:[si]
        mov es:[di],ch

        mov cl,ch
        mov bx,cx
        shl ecx,16
        mov cx,bx

        and ecx,edx
        xor ecx,eax

        mov gs:[bp],cl
        mov gs:[bp+x768],ch
        shr ecx,16
        mov gs:[bp+x1536],cl
        mov gs:[bp+x2304],ch

        add bp,16               ;skip 4 columns (and planes)
        add si,256
        add di,256              ;go to next line of blocklet
        pop cx
        dec cl                  ;decrease # lines
        jne ubms_dolines

        pop edx                 ;restore attr values
        pop cx                  ;restore horiz byte counter (ch) and line cntr
        pop di
        pop si
        pop bp
        inc bp                  ;go to next 8 pixel column
        inc si
        inc di
        ror edx,8
        dec ch
        jne ubms_dobyte

        pop bx                  ;restore touched flag ptr
        endm






ubmsdoblocknextline macro

ubms_doblock_nextline:
        mov es:[di],eax         ;store bits
        mov es:[di+32],edx      ;store attr bytes
        mov ch,4

ubms_donextbyte:
        xor ebx,ebx
        mov bl,dl               ;get leftmost attr byte

        add bx,_flashoffset
        push edx
        mov edx,[4*ebx+offset _inkattr]         ;now andbytes
        mov ebx,[4*ebx+offset _paperattr]       ;now xorbytes

        push cx
        mov cl,al
        mov ch,al
        rol ecx,8
        mov cl,al
        rol ecx,8
        mov cl,al
        and ecx,edx
        xor ecx,ebx

        mov gs:[bp],cl
        mov gs:[bp+x768],ch
        shr ecx,16
        mov gs:[bp+x1536],cl
        mov gs:[bp+x2304],ch

        inc bp
        pop cx
        pop edx                 ;get attr data
        dec ch
        ror edx,8               ;move next attr byte in position
        ror eax,8               ;and next bit byte
        jne ubms_donextbyte     ;Z flag is not affected by ROR instr

        add bp,12
        add si,64
        add di,64
        dec cl
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
        mov fs:[si],bl
        add si,bp
        mov fs:[si],bh
        rol ebx,16
        add si,bp
        mov fs:[si],bl
        mov fs:[si+bp],bh
        sub si,bp
        sub si,bp
        rol ebx,16
        inc si
        sub ax,4
        endm

fbbmfillsome2 macro
        mov fs:[si],bl
        add si,bp
        mov fs:[si],bh
        rol ebx,16
        add si,bp
        mov fs:[si],bl
        mov fs:[si+bp],bh
        sub si,bp
        sub si,bp
        rol ebx,16
        inc si
        add ecx,4
        sub ax,4
        endm





ml_explodebitmap macro
_explodebitmap1:
public _explodebitmap1
        endm

getcolorplaneoffset macro
        xor ebx,ebx
        mov bx,[bp+16]
        endm

expbyte macro
        sar al,1
        rcl ax,2
        sar al,1
        rol ax,2
        sar al,1
        rol ax,2
        sar al,1
        rol ax,2
        sar al,1
        rol eax,2
        sar al,1
        rol eax,2
        sar al,1
        rol eax,2
        sar al,1
        shr eax,6
        xchg ah,al
        endm


explodeline macro
;zoom 1 line (32 pixels) by factor of 2
        mov al,[si]
        expbyte
        mov es:[di],ax
        mov es:[di+bx],ax
        mov al,[si+1]
        expbyte
        mov es:[di+2],ax
        mov es:[di+2+bx],ax
        mov al,[si+2]
        expbyte
        mov es:[di+4],ax
        mov es:[di+4+bx],ax
        mov al,[si+3]
        expbyte
        mov es:[di+6],ax
        mov es:[di+6+bx],ax
        mov ax,bx               ;tbpp*8, i.e. # of color planes * 8
        shr ax,1
        add si,ax               ;go to next pixel line of curr. color plane
        shl ax,2
        add di,ax
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

global planelen: word
global planeadd: word

SPECDATA ends

global _borpart: byte




video1 segment byte use16 public 'CODE'


assume cs:video1

include c:\bc4\spectrum\asm\vidx.asm


video1 ends


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

