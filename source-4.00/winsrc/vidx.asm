SPECDATA segment dword public 'DATA'

global tim1: dword
global tim192: dword
global timupper: dword

SPECDATA ends


;void updatebitmaps(BYTE *vidbuf, BYTE *newdata)
;_updatebitmaps:
;public _updatebitmaps
        ml_updatebitmaps
        push bp
        mov bp,sp       ;bp -> bp,ret,ret,vbseg,vbptr,ndseg,ndptr
        push di
        push si
        push es

        les di,[bp+6]   ;video buf pointer; di, =ndx to screen
        lfs si,[bp+10]  ;new data

        lgs bp,es:dword ptr [offset bitmaps]
        mov bx,offset touched
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+4]
        mov bx,offset touched+24
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+8]
        mov bx,offset touched+48
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+12]
        mov bx,offset touched+72
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+16]
        mov bx,offset touched+96
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+20]
        mov bx,offset touched+120
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+24]
        mov bx,offset touched+144
        call ubm_strip
        lgs bp,es:dword ptr [offset bitmaps+28]
        mov bx,offset touched+168
        call ubm_strip

        pop es
        pop si
        pop di
        pop bp
        retf

ubm_strip:
;es:bx points to first touched flag
;gs:[curbmptr] points to start of bitmap strip
;fs:si points to new data
;es:di points to old data
        mov curbmptr,bp
        mov cx,3                ;three thirds
        mov bp,6144             ;difference pixel-attr, 1st pel line, 1st third
ubms_third:
        push cx
        mov cx,8
ubms_loop:
        push cx
        cmp byte ptr es:[bx+192],0        ;visible?
        jz ubms_invisible
        mov edx,fs:[si+bp]      ;get corresponding attr dword
        cmp es:[di+bp],edx
        jne ubms_doattr
        mov eax,es:[di]
        cmp fs:[si],eax
        jne ubms_doscr0
        mov eax,es:[di+256]
        cmp fs:[si+256],eax
        jne ubms_doscr1
        mov eax,es:[di+512]
        cmp fs:[si+512],eax
        jne ubms_doscr2
        mov eax,es:[di+768]
        cmp fs:[si+768],eax
        jne ubms_doscr3
        mov eax,es:[di+1024]
        cmp fs:[si+1024],eax
        jne ubms_doscr4
        mov eax,es:[di+1280]
        cmp fs:[si+1280],eax
        jne ubms_doscr5
        mov eax,es:[di+1536]
        cmp fs:[si+1536],eax
        jne ubms_doscr6
        mov eax,es:[di+1792]
        cmp fs:[si+1792],eax
        jne ubms_doscr7
ubms_invisible:
        pop cx
        inc bx
        add si,32
        add di,32
        add curbmptr,pix256     ;32 x 8 pixels
        dec cx
        jne ubms_loop
        add si,-256+2048        ;next third
        add di,-256+2048
        sub bp,-256+2048        ;to make up for third's adjustment for attrs
        pop cx
        dec cx
        jne ubms_third
        add si,-6144+4
        add di,-6144+4
        ret




ubms_doscr7:
        push bp
        mov cx,1
        add di,1792
        add si,1792
        mov bp,curbmptr
        add bp,pix224
        jmp ubms_doscr
ubms_doscr6:
        push bp
        mov cx,2
        add di,1536
        add si,1536
        mov bp,curbmptr
        add bp,pix192
        jmp ubms_doscr
ubms_doscr5:
        push bp
        mov cx,3
        add di,1280
        add si,1280
        mov bp,curbmptr
        add bp,pix160
        jsh ubms_doscr
ubms_doscr4:
        push bp
        mov cx,4
        add di,1024
        add si,1024
        mov bp,curbmptr
        add bp,pix128
        jsh ubms_doscr
ubms_doscr3:
        push bp
        mov cx,5
        add di,768
        add si,768
        mov bp,curbmptr
        add bp,pix96
        jsh ubms_doscr
ubms_doscr2:
        push bp
        mov cx,6
        add di,512
        add si,512
        mov bp,curbmptr
        add bp,pix64
        jsh ubms_doscr
ubms_doscr1:
        push bp
        mov cx,7
        add di,256
        add si,256
        mov bp,curbmptr
        add bp,pix32
        jsh ubms_doscr
ubms_doattr:
        mov es:[di+bp],edx      ;store new attr dword
ubms_doscr0:
        push bp
        mov cx,8                ;8 lines
        mov bp,curbmptr
ubms_doscr:
        ubmsdoscr               ;big macro in vid*.asm

        pop bp                  ;restore scr-attr difference for main loop
        mov ch,8
        sub ch,cl               ;now ch=# of lines skipped
        mov cl,4                ;now cx=256*lines skipped + 4
        sub si,cx               ;restore SI,DI value before jmp to ubms_doscr*
        sub di,cx
        jmp ubms_invisible      ;continue in the loop


;-----------------------------------------------------------------
;Code for video update in coppering mode
;-----------------------------------------------------------------



;void updatebitmapscopper(BYTE *vidbuf, BYTE *newdata)
;_updatebitmapscopper:
;public _updatebitmapscopper
        ml_updatebitmapscopper
        push bp
        mov bp,sp       ;bp -> bp,ret,ret,vbseg,vbptr,ndseg,ndptr
        push di
        push si
        push es

        les di,[bp+6]   ;video buf pointer; di, =ndx to screen
        lfs si,[bp+10]  ;new data

        lgs bp,es:dword ptr [offset bitmaps]
        mov bx,offset touched
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+4]
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+8]
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+12]
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+16]
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+20]
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+24]
        call ubm_strip_cpr
        lgs bp,es:dword ptr [offset bitmaps+28]
        call ubm_strip_cpr

        pop es
        pop si
        pop di
        pop bp
        retf

ubm_strip_cpr:
;es:bx points to first touched flag
;gs:bp points to start of bitmap strip
;fs:si points to new data
;es:di points to old data
        mov cx,24
ubms_loop_cpr:
        cmp byte ptr es:[bx+192],0        ;visible?
        jz ubms_invisible_cpr

ubms_line macro diff,lbl
        mov eax,fs:[si+diff]    ;get bitmap data
        mov edx,fs:[si+32+diff] ;get attr data
        cmp eax,es:[di+diff]
        jne lbl
        cmp edx,es:[di+32+diff]
        jne lbl
        endm

        ubms_line 0,ubms_doline0
        ubms_line 64,ubms_doline1
        ubms_line 128,ubms_doline2
        ubms_line 192,ubms_doline3
        ubms_line 256,ubms_doline4
        ubms_line 320,ubms_doline5
        ubms_line 384,ubms_doline6
        ubms_line 448,ubms_doline7

ubms_invisible_cpr:
        add si,512
        add di,512
        add bp,pix256
ubms_loop_scan:
        inc bx
        dec cx
        jne ubms_loop_cpr
        add si,-12288+4
        add di,-12288+4
        ret

ubms_doline0:
        push cx
        mov cx,8
        jsh ubms_doblock_cpr
ubms_doline1:
        push cx
        mov cx,7
        add di,64
        add si,64
        add bp,pix32
        jsh ubms_doblock_cpr
ubms_doline2:
        push cx
        mov cx,6
        add di,128
        add si,128
        add bp,pix64
        jsh ubms_doblock_cpr
ubms_doline3:
        push cx
        mov cx,5
        add di,192
        add si,192
        add bp,pix96
        jsh ubms_doblock_cpr
ubms_doline4:
        push cx
        mov cx,4
        add di,256
        add si,256
        add bp,pix128
        jsh ubms_doblock_cpr
ubms_doline5:
        push cx
        mov cx,3
        add di,320
        add si,320
        add bp,pix160
        jsh ubms_doblock_cpr
ubms_doline6:
        push cx
        mov cx,2
        add di,384
        add si,384
        add bp,pix192
        jsh ubms_doblock_cpr
ubms_doline7:
        push cx
        mov cx,1
        add di,448
        add si,448
        add bp,pix224
ubms_doblock_cpr:
        mov byte ptr es:[bx],1  ;set touched flag
        push bx

        ubmsdoblocknextline     ;big macro in vid*.asm


        pop bx
        pop cx
        jmp ubms_loop_scan





;void updateborder(void)
;_updateborder:
;public _updateborder
        ml_updateborder
        push bp
        push si
        push di
        push es

        les di,_outbufptr       ;es:di = pointer into out buffer

        mov bp,offset _borpart
        mov ecx,timupper
        add ecx,64              ;first line, middle
        movzx eax,ds:word ptr [bp+4]         ;xsize in pixels = 2 * # of T states
        shr eax,2               ;xsize/4, half # of T states of whole width
        sub ecx,eax
        mov ax,ds:[bp+6]           ;ysize in pixels = # of T states / 224
        mov dx,word ptr [offset tim1]
        mul dx
        movzx eax,ax
        sub ecx,eax             ;ecx = starting time of left-upper border pixel

ub_finduppercolour:             ;skip entries till newT (EDX) > ECX
        add di,4
        mov edx,es:[di]
        shr edx,8
        cmp edx,ecx
        jbe ub_finduppercolour
        xor ebx,ebx
        mov bl,es:[di-4]        ;bl = high border colour

        fixupcolor              ;macro in vid*.asm

        calcplanelen

        call fixborbitmap

        push di                 ;save current pointer into buffer
        push ebx                ;save current border colour
        add bp,borpartsize      ;next border part
        mov ecx,timupper
        movzx eax,ds:word ptr [bp+4]        ;xsize in pixels
        shr eax,1
        sub ecx,eax

        calcplanelen

        call fixborbitmap

        pop ebx
        pop di
        add bp,borpartsize
        mov ecx,timupper
        add ecx,128
        jmp short fbbm1x
fbbm2x:
        xor ebx,ebx
        mov bl,es:[di]
        fixupcolor              ;macro in vid*.asm
        add di,4
fbbm1x:                         ;skip entries till newT (EDX) > ECX
        mov edx,es:[di]
        shr edx,8
        cmp edx,ecx
        jbe fbbm2x

        call fixborbitmap       ;same plane length

        add bp,borpartsize
        mov ecx,timupper
        add ecx,tim192
        add ecx,64
        movzx eax,ds:word ptr [bp+4]       ;xsize
        shr eax,2
        sub ecx,eax

        calcplanelen

        call fixborbitmap

        pop es
        pop di
        pop si
        pop bp
        retf



fixborbitmap:
;ecx = T of left-upper corner
;bp -> borpart structure
;ebx = current border colour
;es:di -> borbuf
        cmp byte ptr ds:[bp+13],0  ;visible
        je short fbbm_ret
        mov al,es:[di-4]           ;colour of last out
        cmp ds:[bp+14],al          ;colour
        je short fbbm_testsame
        mov byte ptr ds:[bp+12],1  ;touched
        mov ds:[bp+14],al          ;set colour, maybe this'll be straight block
fbbm_testsame:                  ;test whether this'll be straight block
        xor eax,eax
        mov ax,ds:[bp+6]           ;ysize
        mov dx,word ptr [offset tim1]
        mul dx
        add eax,ecx
        shl eax,8
        cmp eax,es:[di]
        ja short fbbm_start     ;jmp if final T > next T
fbbm_ret:
        ret                     ;same colour; do nothing
fbbm_start:
        movzx eax,word ptr ds:[bp+4]           ;xsize
        shr eax,1
        mov xtsize,eax          ;xsize in T states
        sub eax,tim1
        neg eax
        mov xtrest,eax          ;rest of scan line in T states
        mov ax,ds:[bp+6]           ;ysize
        mov ysize,ax
        lfs si,ds:[bp]
        mov eax,xtsize
        vid1special
        jmp short fbbm1
fbbm2:
        xor ebx,ebx
        mov bl,es:[di]
        fixupcolor2             ;macro in vid*.asm
        add di,4
fbbm1:                          ;skip entries till newT (EDX) > ECX
        mov edx,es:[di]
        shr edx,8
        cmp edx,ecx
        jbe fbbm2
fbbm_later:
        add ecx,eax
        cmp edx,ecx
        jb short fbbm_structure
        test ax,ax
        je short fbbm_nothing
fbbm_fill:
        fbbmfillsome            ;macro in vid*.asm
        jne short fbbm_fill
fbbm_nothing:
        add ecx,xtrest
        mov eax,xtsize
        addplanelen
        dec ysize
        jne fbbm_later
        vid1special2
        ret

fbbm_structure:
        sub ecx,eax             ;restore current T
;        mov byte ptr ds:[bp+14],8  ;set color to 8 to denote 'border with structure'
        vid1special3
        test ax,ax
        je short fbbms_nothing
fbbms_fill:
        cmp edx,ecx
        jbe fbbm2
        fbbmfillsome2           ;macro in vid*.asm
        jne short fbbms_fill
fbbms_nothing:
        add ecx,xtrest
        mov eax,xtsize
        addplanelen
        dec ysize
        jne fbbm_later
        vid1special2
        ret



;
;void explodebitmap(BYTE *source, BYTE *target, int nlines {,int colorplaneoffset})
;
        ml_explodebitmap
        push bp
        mov bp,sp               ;bp -> bp,ret,ret,srcseg,srcptr,tseg,tptr,n
        push di
        push si
        push es
        push ds
        xor esi,esi
        xor edi,edi
        lds si,[bp+6]
        les di,[bp+10]
        mov cx,[bp+14]
        getcolorplaneoffset
ebm_loop:
        explodeline
        dec cx
        jne ebm_loop
        pop ds
        pop es
        pop si
        pop di
        pop bp
        retf




