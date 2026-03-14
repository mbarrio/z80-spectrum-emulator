
include c:/bc4/spectrum/asm/macro.asm




CORE segment byte use16 public 'CODE'


XDSTUB: inc si                          ;used for inappropriate DD/FD prefixes
        sub ebp,03ffffh
        jb wrapped
        jmp es:[offset emultab+2*ebx]


include c:/bc4/spectrum/asm/ddops.asm
include c:/bc4/spectrum/asm/fdops.asm


CORE ends




SPECDATA segment dword public 'DATA'

ddemultab dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opdd09,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opdd19,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opdd21,opdd22,opdd23,opdd24,opdd25,opdd26,XDSTUB
        dw XDSTUB,opdd29,opdd2a,opdd2b,opdd2c,opdd2d,opdd2e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd34,opdd35,opdd36,XDSTUB
        dw XDSTUB,opdd39,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd44,opdd45,opdd46,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd4c,opdd4d,opdd4e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd54,opdd55,opdd56,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd5c,opdd5d,opdd5e,XDSTUB
        dw opdd60,opdd61,opdd62,opdd63,opdd64,opdd65,opdd66,opdd67
        dw opdd68,opdd69,opdd6a,opdd6b,opdd6c,opdd6d,opdd6e,opdd6f
        dw opdd70,opdd71,opdd72,opdd73,opdd74,opdd75,XDSTUB,opdd77
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd7c,opdd7d,opdd7e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd84,opdd85,opdd86,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd8c,opdd8d,opdd8e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd94,opdd95,opdd96,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdd9c,opdd9d,opdd9e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opdda4,opdda5,opdda6,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opddac,opddad,opddae,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opddb4,opddb5,opddb6,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opddbc,opddbd,opddbe,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,opddcb,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opdde1,XDSTUB,opdde3,XDSTUB,opdde5,XDSTUB,XDSTUB
        dw XDSTUB,opdde9,XDSTUB,opddeb,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opddf9,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB


fdemultab dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opfd09,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opfd19,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opfd21,opfd22,opfd23,opfd24,opfd25,opfd26,XDSTUB
        dw XDSTUB,opfd29,opfd2a,opfd2b,opfd2c,opfd2d,opfd2e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd34,opfd35,opfd36,XDSTUB
        dw XDSTUB,opfd39,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd44,opfd45,opfd46,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd4c,opfd4d,opfd4e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd54,opfd55,opfd56,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd5c,opfd5d,opfd5e,XDSTUB
        dw opfd60,opfd61,opfd62,opfd63,opfd64,opfd65,opfd66,opfd67
        dw opfd68,opfd69,opfd6a,opfd6b,opfd6c,opfd6d,opfd6e,opfd6f
        dw opfd70,opfd71,opfd72,opfd73,opfd74,opfd75,XDSTUB,opfd77
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd7c,opfd7d,opfd7e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd84,opfd85,opfd86,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd8c,opfd8d,opfd8e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd94,opfd95,opfd96,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfd9c,opfd9d,opfd9e,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfda4,opfda5,opfda6,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfdac,opfdad,opfdae,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfdb4,opfdb5,opfdb6,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,opfdbc,opfdbd,opfdbe,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,opfdcb,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opfde1,XDSTUB,opfde3,XDSTUB,opfde5,XDSTUB,XDSTUB
        dw XDSTUB,opfde9,XDSTUB,opfdeb,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB
        dw XDSTUB,opfdf9,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB,XDSTUB


SPECDATA ends


end



