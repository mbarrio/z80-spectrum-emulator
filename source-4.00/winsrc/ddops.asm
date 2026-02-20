


opdd09:
        and ah,0fch
        add rix,cx
        adc ah,0
        setop ixh
        em2 2,15


opdd19: and ah,0fch
        mov ebx,edx
        shr ebx,16
        add rix,bx
        adc ah,0
        setop ixh
        xor ebx,ebx
        em2 2,15


opdd21: mov bx,[si+2]
        mov rix,bx
        xor bx,bx
        em2 4,14


opdd22: mov bx,[si+2]
        rol ecx,16
        mov cx,rix
        pokew bx,cx,ch,cl
        rol ecx,16
        xor ebx,ebx
        em2 4,20


opdd23: inc rix
        em2 2,10


opdd24: sahf
        inc byte ptr [offset rix+1]
        ova
        setop ixh
        em2 2,8


opdd25: sahf
        dec byte ptr [offset rix+1]
        ovs
        setop ixh
        em2 2,8


opdd26: mov bl,[si+2]
        mov byte ptr [offset rix+1],bl
        em2 3,11


opdd29: and ah,0fch
        mov bx,rix
        add bx,bx
        adc ah,0
        mov rix,bx
        setop bh
        xor ebx,ebx
        em2 2,15


opdd2a: mov bx,[si+2]
        mov bx,[bx]
        mov rix,bx
        xor ebx,ebx
        em2 4,20


opdd2b: dec rix
        em2 2,10


opdd2c: sahf
        inc byte ptr [offset rix]
        ova
        setop ixl
        em2 2,8


opdd2d: sahf
        dec byte ptr [offset rix]
        ovs
        setop ixl
        em2 2,8


opdd2e: mov bl,[si+2]
        mov byte ptr [offset rix],bl
        em2 3,11


opdd34: movsx bx,byte ptr [si+2]
        add bx,rix
        testrom bx,op34romix
        sahf
        inc byte ptr [bx]
        ova
        setop bptrbx
        xor ebx,ebx
        em2 3,23
op34romix:
        mov bl,[bx]
        sahf
        inc bl
        ova
        setop bl
        xor ebx,ebx
        em2 3,23


opdd35: movsx bx,byte ptr [si+2]
        add bx,rix
        testrom bx,op35romix
        sahf
        dec byte ptr [bx]
        ovs
        setop bptrbx
        xor ebx,ebx
        em2 3,23
op35romix:
        mov bl,[bx]
        sahf
        dec bl
        ovs
        setop bl
        xor ebx,ebx
        em2 3,23


opdd36: movsx bx,byte ptr [si+2]
        add bx,rix
        testrom bx,op36noramix
        rol eax,16
        mov al,[si+3]
        mov [bx],al
        rol eax,16
        xor ebx,ebx
op36noramix:
        em2 4,19


opdd39: and ah,0fch
        add rix,di
        adc ah,0
        setop ixh
        em2 2,15


opdd44: mov ch,byte ptr [offset rix+1]
        em2 2,8


opdd45: mov ch,byte ptr [offset rix]
        em2 2,8


opdd46: movsx bx,byte ptr [si+2]
        add bx,rix
        mov ch,[bx]
        xor bx,bx
        em2 3,19


opdd4c: mov cl,byte ptr [offset rix+1]
        em2 2,8


opdd4d: mov cl,byte ptr [offset rix]
        em2 2,8


opdd4e: movsx bx,byte ptr [si+2]
        add bx,rix
        mov cl,[bx]
        xor bx,bx
        em2 3,19


opdd54: rol edx,16
        mov dh,byte ptr [offset rix+1]
        rol edx,16
        em2 2,8


opdd55: rol edx,16
        mov dh,byte ptr [offset rix]
        ror edx,16
        em2 2,8


opdd56: movsx bx,byte ptr [si+2]
        add bx,rix
        rol edx,16
        mov dh,[bx]
        rol edx,16
        xor bx,bx
        em2 3,19


opdd5c: rol edx,16
        mov dl,byte ptr [offset rix+1]
        rol edx,16
        em2 2,8


opdd5d: rol edx,16
        mov dl,byte ptr [offset rix]
        rol edx,16
        em2 2,8


opdd5e: movsx bx,byte ptr [si+2]
        add bx,rix
        rol edx,16
        mov dl,[bx]
        rol edx,16
        xor bx,bx
        em2 3,19


opdd60: mov byte ptr [offset rix+1],ch
        em2 2,8


opdd61: mov byte ptr [offset rix+1],cl
        em2 2,8


opdd62: rol edx,16
        mov byte ptr [offset rix+1],dh
        rol edx,16
        em2 2,8


opdd63: rol edx,16
        mov byte ptr [offset rix+1],dl
        rol edx,16
        em2 2,8


opdd64: em2 2,8


opdd65: mov bl,byte ptr [offset rix]
        mov byte ptr [offset rix+1],bl
        em2 2,8


opdd66: movsx bx,byte ptr [si+2]
        add bx,rix
        mov dh,[bx]
        xor bx,bx
        em2 3,19


opdd67: mov byte ptr [offset rix+1],al
        em2 2,8


opdd68: mov byte ptr [offset rix],ch
        em2 2,8


opdd69: mov byte ptr [offset rix],cl
        em2 2,8


opdd6a: rol edx,16
        mov byte ptr [offset rix],dh
        rol edx,16
        em2 2,8


opdd6b: rol edx,16
        mov byte ptr [offset rix],dl
        rol edx,16
        em2 2,8


opdd6c: mov bl,byte ptr [offset rix+1]
        mov byte ptr [offset rix],bl
        em2 2,8


opdd6d: em2 2,8


opdd6e: movsx bx,byte ptr [si+2]
        add bx,rix
        mov dl,[bx]
        xor ebx,ebx
        em2 3,19


opdd6f: mov byte ptr [offset rix],al
        em2 2,8


opdd70: movsx bx,[si+2]
        add bx,rix
        poke bx,ch
        xor ebx,ebx
        em2 3,19


opdd71: movsx bx,[si+2]
        add bx,rix
        poke bx,cl
        xor ebx,ebx
        em2 3,19


opdd72: movsx bx,[si+2]
        add bx,rix
        rol edx,16
        poke bx,dh
        rol edx,16
        xor ebx,ebx
        em2 3,19


opdd73: movsx bx,[si+2]
        add bx,rix
        rol edx,16
        poke bx,dl
        rol edx,16
        xor ebx,ebx
        em2 3,19


opdd74: movsx bx,[si+2]
        add bx,rix
        poke bx,dh
        xor ebx,ebx
        em2 3,19


opdd75: movsx bx,[si+2]
        add bx,rix
        poke bx,dl
        xor ebx,ebx
        em2 3,19


opdd77: movsx bx,[si+2]
        add bx,rix
        poke bx,al
        xor ebx,ebx
        em2 3,19



opdd7c: mov al,byte ptr [offset rix+1]
        em2 2,8


opdd7d: mov al,byte ptr [offset rix]
        em2 2,8


opdd7e: movsx bx,[si+2]
        add bx,rix
        mov al,[bx]
        xor ebx,ebx
        em2 3,19




opddcb: rol edi,16
        movsx di,byte ptr [si+2]
        add di,rix
        mov bl,[si+3]
        jmp es:[offset xdcbemultab+2*ebx]


opdde1: inc di
        jesh popix_segviol
        mov bx,[di-1]
popix1: inc di
        mov rix,bx
        xor ebx,ebx
        em2 2,14
popix_segviol:
        mov bl,[di-1]
        mov bh,[di]
        jsh popix1


opdde3: mov bx,rix
        inc di
        cmp di,04001h
        jb exspix_careful
        dec di
        xchg [di],bx
        mov rix,bx
        xor ebx,ebx
        em2 2,23
exspix_careful:
        testrom di,ixsp_nothi
        xchg [di],bh
ixsp_nothi:
        dec di
        testrom di,ixsp_notlo
        xchg [di],bl
ixsp_notlo:
        mov rix,bx
        xor bx,bx
        em2 2,23


opdde5: mov bx,rix
        pushreg bx,bh,bl
        xor ebx,ebx
        em2 2,15


opdde9: mov si,rix
        em2 0,8


opddeb: rol edx,16
        xchg dx,rix
        rol edx,16
        em2 2,8


opddf9: mov di,rix
        em2 2,8



