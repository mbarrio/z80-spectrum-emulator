


opfd09:
        and ah,0fch
        add riy,cx
        adc ah,0
        setop iyh
        em2 2,15


opfd19: and ah,0fch
        mov ebx,edx
        shr ebx,16
        add riy,bx
        adc ah,0
        setop iyh
        xor ebx,ebx
        em2 2,15


opfd21: mov bx,[si+2]
        mov riy,bx
        xor bx,bx
        em2 4,14


opfd22: mov bx,[si+2]
        rol ecx,16
        mov cx,riy
        pokew bx,cx,ch,cl
        rol ecx,16
        xor ebx,ebx
        em2 4,20


opfd23: inc riy
        em2 2,10


opfd24: sahf
        inc byte ptr [offset riy+1]
        ova
        setop iyh
        em2 2,8


opfd25: sahf
        dec byte ptr [offset riy+1]
        ovs
        setop iyh
        em2 2,8


opfd26: mov bl,[si+2]
        mov byte ptr [offset riy+1],bl
        em2 3,11


opfd29: and ah,0fch
        mov bx,riy
        add bx,bx
        adc ah,0
        mov riy,bx
        setop bh
        xor ebx,ebx
        em2 2,15


opfd2a: mov bx,[si+2]
        mov bx,[bx]
        mov riy,bx
        xor ebx,ebx
        em2 4,20


opfd2b: dec riy
        em2 2,10


opfd2c: sahf
        inc byte ptr [offset riy]
        ova
        setop iyl
        em2 2,8


opfd2d: sahf
        dec byte ptr [offset riy]
        ovs
        setop iyl
        em2 2,8


opfd2e: mov bl,[si+2]
        mov byte ptr [offset riy],bl
        em2 3,11


opfd34: movsx bx,byte ptr [si+2]
        add bx,riy
        testrom bx,op34romiy
        sahf
        inc byte ptr [bx]
        ova
        setop bptrbx
        xor ebx,ebx
        em2 3,23
op34romiy:
        mov bl,[bx]
        sahf
        inc bl
        ova
        setop bl
        xor ebx,ebx
        em2 3,23


opfd35: movsx bx,byte ptr [si+2]
        add bx,riy
        testrom bx,op35romiy
        sahf
        dec byte ptr [bx]
        ovs
        setop bptrbx
        xor ebx,ebx
        em2 3,23
op35romiy:
        mov bl,[bx]
        sahf
        dec bl
        ovs
        setop bl
        xor ebx,ebx
        em2 3,23


opfd36: movsx bx,byte ptr [si+2]
        add bx,riy
        testrom bx,op36noramiy
        rol eax,16
        mov al,[si+3]
        mov [bx],al
        rol eax,16
        xor ebx,ebx
op36noramiy:
        em2 4,19


opfd39: and ah,0fch
        add riy,di
        adc ah,0
        setop iyh
        em2 2,15


opfd44: mov ch,byte ptr [offset riy+1]
        em2 2,8


opfd45: mov ch,byte ptr [offset riy]
        em2 2,8


opfd46: movsx bx,byte ptr [si+2]
        add bx,riy
        mov ch,[bx]
        xor bx,bx
        em2 3,19


opfd4c: mov cl,byte ptr [offset riy+1]
        em2 2,8


opfd4d: mov cl,byte ptr [offset riy]
        em2 2,8


opfd4e: movsx bx,byte ptr [si+2]
        add bx,riy
        mov cl,[bx]
        xor bx,bx
        em2 3,19


opfd54: rol edx,16
        mov dh,byte ptr [offset riy+1]
        rol edx,16
        em2 2,8


opfd55: rol edx,16
        mov dh,byte ptr [offset riy]
        ror edx,16
        em2 2,8


opfd56: movsx bx,byte ptr [si+2]
        add bx,riy
        rol edx,16
        mov dh,[bx]
        rol edx,16
        xor bx,bx
        em2 3,19


opfd5c: rol edx,16
        mov dl,byte ptr [offset riy+1]
        rol edx,16
        em2 2,8


opfd5d: rol edx,16
        mov dl,byte ptr [offset riy]
        rol edx,16
        em2 2,8


opfd5e: movsx bx,byte ptr [si+2]
        add bx,riy
        rol edx,16
        mov dl,[bx]
        rol edx,16
        xor bx,bx
        em2 3,19


opfd60: mov byte ptr [offset riy+1],ch
        em2 2,8


opfd61: mov byte ptr [offset riy+1],cl
        em2 2,8


opfd62: rol edx,16
        mov byte ptr [offset riy+1],dh
        rol edx,16
        em2 2,8


opfd63: rol edx,16
        mov byte ptr [offset riy+1],dl
        rol edx,16
        em2 2,8


opfd64: em2 2,8


opfd65: mov bl,byte ptr [offset riy]
        mov byte ptr [offset riy+1],bl
        em2 2,8


opfd66: movsx bx,byte ptr [si+2]
        add bx,riy
        mov dh,[bx]
        xor bx,bx
        em2 3,19


opfd67: mov byte ptr [offset riy+1],al
        em2 2,8


opfd68: mov byte ptr [offset riy],ch
        em2 2,8


opfd69: mov byte ptr [offset riy],cl
        em2 2,8


opfd6a: rol edx,16
        mov byte ptr [offset riy],dh
        rol edx,16
        em2 2,8


opfd6b: rol edx,16
        mov byte ptr [offset riy],dl
        rol edx,16
        em2 2,8


opfd6c: mov bl,byte ptr [offset riy+1]
        mov byte ptr [offset riy],bl
        em2 2,8


opfd6d: em2 2,8


opfd6e: movsx bx,byte ptr [si+2]
        add bx,riy
        mov dl,[bx]
        xor ebx,ebx
        em2 3,19


opfd6f: mov byte ptr [offset riy],al
        em2 2,8


opfd70: movsx bx,[si+2]
        add bx,riy
        poke bx,ch
        xor ebx,ebx
        em2 3,19


opfd71: movsx bx,[si+2]
        add bx,riy
        poke bx,cl
        xor ebx,ebx
        em2 3,19


opfd72: movsx bx,[si+2]
        add bx,riy
        rol edx,16
        poke bx,dh
        rol edx,16
        xor ebx,ebx
        em2 3,19


opfd73: movsx bx,[si+2]
        add bx,riy
        rol edx,16
        poke bx,dl
        rol edx,16
        xor ebx,ebx
        em2 3,19


opfd74: movsx bx,[si+2]
        add bx,riy
        poke bx,dh
        xor ebx,ebx
        em2 3,19


opfd75: movsx bx,[si+2]
        add bx,riy
        poke bx,dl
        xor ebx,ebx
        em2 3,19


opfd77: movsx bx,[si+2]
        add bx,riy
        poke bx,al
        xor ebx,ebx
        em2 3,19



opfd7c: mov al,byte ptr [offset riy+1]
        em2 2,8


opfd7d: mov al,byte ptr [offset riy]
        em2 2,8


opfd7e: movsx bx,[si+2]
        add bx,riy
        mov al,[bx]
        xor ebx,ebx
        em2 3,19




opfdcb: rol edi,16
        movsx di,byte ptr [si+2]
        add di,riy
        mov bl,[si+3]
        jmp es:[offset xdcbemultab+2*ebx]


opfde1: inc di
        jesh popiy_segviol
        mov bx,[di-1]
popiy1: inc di
        mov riy,bx
        xor ebx,ebx
        em2 2,14
popiy_segviol:
        mov bl,[di-1]
        mov bh,[di]
        jsh popiy1


opfde3: mov bx,riy
        inc di
        cmp di,04001h
        jb exspiy_careful
        dec di
        xchg [di],bx
        mov riy,bx
        xor ebx,ebx
        em2 2,23
exspiy_careful:
        testrom di,iysp_nothi
        xchg [di],bh
iysp_nothi:
        dec di
        testrom di,iysp_notlo
        xchg [di],bl
iysp_notlo:
        mov riy,bx
        xor bx,bx
        em2 2,23


opfde5: mov bx,riy
        pushreg bx,bh,bl
        xor ebx,ebx
        em2 2,15


opfde9: mov si,riy
        em2 0,8


opfdeb: rol edx,16
        xchg dx,riy
        rol edx,16
        em2 2,8


opfdf9: mov di,riy
        em2 2,8



