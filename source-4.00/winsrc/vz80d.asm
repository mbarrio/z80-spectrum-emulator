TITLE vz80d - virtueel Spectrum geheugen - Gerton Lunter, 24/5/98


        .386p


;******************************************************************************
;                             I N C L U D E S
;******************************************************************************


        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        .LIST



;******************************************************************************
;                             I N C L U D E S :  vz80d.inc
;******************************************************************************



listnode struc

vmid            dd ?            ;Id. van VM
lnid            dd ?            ;Id. van listnode
frameadr        dd ?            ;Linear adres van 64k frame
framesel        dd ?            ;Selector frame
nbanks          dd ?            ;Aantal banken (normaal 4 = 64K)
bufsel          dd ?            ;Selector buffer
pagesel1        dd ?            ;Selector single page
pageselpag1     dd ?            ;Number of single page selector
npages          dd ?            ;Aantal 16k paginas van buffer
bufphysadr      dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
                                ;Fysieke adres van buffer
bufadr          dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
                                ;Linear adres van buffer

listnode ends



;******************************************************************************
;                V I R T U A L   D E V I C E   D E C L A R A T I O N
;******************************************************************************


VZ80D_ID        equ 3f12h       ; zelfde als vwatchd, tijdelijk...


Declare_Virtual_Device VZ80D, 1, 0, VZ80D_Control, VZ80D_ID, \
                       Undefined_Init_Order, VZ80D_API_PROC, VZ80D_API_PROC


;******************************************************************************
;                         L O C A L   D A T A
;******************************************************************************

VxD_IDATA_SEG

VxD_IDATA_ENDS

VxD_DATA_SEG

listhandle      dd -1            ;handle van lijst

VxD_DATA_ENDS


;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG


;******************************************************************************
;
; Installeert lijst
;
;==============================================================================

BeginProc VZ80D_Init_Complete

        cmp listhandle,-1
        jne Init_IsAl                   ;Misschien 2x ingeladen??
        mov eax,0                       ;flags: niks aan de hand.
        mov ecx,SIZE listnode
        VMMCall List_Create
        jc Init_Luktnie
        mov listhandle,esi
Init_IsAl:
        clc
        ret

Init_Luktnie:
        mov listhandle,-1
        stc
        ret

EndProc VZ80D_Init_Complete



VxD_ICODE_ENDS




VxD_CODE_SEG



;******************************************************************************
;
;   Zoek_lijst: in edi, uit zf->gevonden (edi=node), nzf->niet (edi=max)
;
;==============================================================================

BeginProc Zoek_Lijst

        mov esi,listhandle
        cmp esi,-1
        je zl_err
        push ecx
        xor ecx,ecx
        VMMCall List_Get_First
zl_loop:
        jz zl_nf
        cmp [eax.lnid],edi
        jz zl_found
        cmp [eax.lnid],ecx
        jbe zl_nietgroter
        mov ecx,[eax.lnid]
zl_nietgroter:
        VMMCall List_Get_Next
        jmp zl_loop

zl_nf:
        cmp ecx,-1                      ;reset z flag
        mov eax,ecx                     ;get max into eax
zl_found:
        mov edi,eax                     ;store max or node into edi
        pop ecx
        ret

zl_err:
        cmp esi,0
        mov edi,0
        ret

EndProc Zoek_Lijst


;******************************************************************************
;
;   VZ80D_System_Exit
;
;
;==============================================================================

BeginProc VZ80D_System_Exit

        xor ebx,ebx
        call free_vm_ebx
        mov esi,listhandle
        cmp esi,-1
        je se_nolist
        VMMCall List_Destroy
        mov listhandle,-1
se_nolist:
        clc
        ret

EndProc VZ80D_System_Exit


;******************************************************************************
;
;   VZ80D_Sys_Dyn_Dev_Exit
;
;  Dynamisch unloaden.  Lukt alleen als er geen VM's meer bezig zijn
;
;==============================================================================

BeginProc VZ80D_Sys_Dyn_Dev_Exit

        mov edi,-1
        call Zoek_Lijst
        cmp edi,0
        jne sdde_MagNiet
        jmp VZ80D_System_Exit

sdde_MagNiet:                   ;Er zijn nog blokjes in gebruik
        stc
        ret

EndProc VZ80D_Sys_Dyn_Dev_Exit

;******************************************************************************
;
;   VZ80D_Destroy_VM
;
;
;==============================================================================

BeginProc VZ80D_Destroy_VM

        VMMCall Get_Cur_VM_Handle
        call free_vm_ebx
        stc
        ret

EndProc VZ80D_Destroy_VM


;******************************************************************************
;
;   free_vm_ebx
;
;   Gooit geheugen geassocieerd met VM ebx weg.  Gooit alles weg als ebx=0
;
;==============================================================================


BeginProc free_vm_ebx

        mov esi,listhandle
        cmp esi,-1
        je fve_end
        VMMCall List_Get_First
fve_loop:
        jz fve_end
        cmp ebx,0
        je fve_kill
        cmp [eax.vmid],ebx
        jz fve_kill
        VMMCall List_Get_Next
        jmp fve_loop
fve_kill:
        push esi
        push ebx
        push eax
        call free_memory_eax
        pop eax
        pop ebx
        pop esi
        VMMCall List_Get_First
        jmp fve_loop

fve_end:
        clc
        ret

EndProc free_vm_ebx



;******************************************************************************
;
;   alloc_memory
;
;   Maakt geheugenblok, van eax 16K paginas, en ecx 16K banken
;   Geeft in eax ptr naar lijst terug
;   Geeft 0 terug als problemen
;
;==============================================================================

BeginProc alloc_memory

        cmp eax,1
        jb am_problemen                 ;At least allocate 1 page
        cmp ax,16
        ja am_problemen                 ;Can only allocate 16 pages
        push eax                        ;# paginas
        push ecx                        ;# banks

        mov edi,-1
        call Zoek_Lijst                 ;edi = max list node id.

        mov esi,listhandle
        VMMCall List_Allocate           ;nieuwe node
        pop ecx
        pop ebx                         ;EBX pags, ECX banks, EAX node, EDI max
        jc am_problemen

        xchg edi,eax                    ;node addr in EDI, max list node in EAX
        inc eax                         ;nieuwe list node id.
        mov [edi.lnid],eax              ;sla nieuwe lide node id. op
        mov [edi.npages],ebx            ;sla # paginas op
        mov [edi.nbanks],ecx            ;sla # banks op
        VMMCall Get_Cur_VM_Handle
        mov [edi.vmid],ebx              ;sla VM id op

;
;Now allocate 16K `pages' one by one, increasing edi.npages by one each time.
;If an error occurs, they should all be deallocated.
;The desired number of pages is kept on the stack
;

        mov eax,[edi.npages]            ;Number of pages to allocate
        mov [edi.npages],0              ;Counter

am_loop:
        push eax                        ;Keep counter on stack
        mov ecx,4                       ;4 4K pages makes 16K
        mov ebx,[edi.npages]            ;Get current page number
        lea eax,[edi.bufphysadr+4*ebx]  ;Address to store physical address
        mov ebx,[edi.vmid]              ;Get virtual machine ID
        VMMCall _PageAllocate <ecx,PG_VM,ebx,0,0,-1,eax,PAGECONTIG + PAGEUSEALIGN + PAGEFIXED>
        test eax,eax
        je am_problemen_dealloc         ;Jump out if couldn't allocate
        mov ebx,[edi.npages]
        mov [edi.bufadr+4*ebx],eax      ;Store linear address
        inc [edi.npages]                ;Increase counter
        pop eax
        cmp [edi.npages],eax
        jb am_loop                      ;Go 'round the loop if not done

;
;Now allocate a frame
;

        mov eax,[edi.nbanks]
        shl eax,2
        VMMCall _PageReserve <PR_PRIVATE, eax, 0>        ;Reserveer 16K * # banks
        cmp eax,-1
        je am_problemen_dealloc2
        mov [edi.frameadr],eax

        mov [edi.framesel],0            ;Initialiseer wat variabeltjes
        mov [edi.pagesel1],0
        mov [edi.pageselpag1],-1

        mov eax,edi
        VMMCall List_Attach             ;stop nieuwe node in de lijst

        mov eax,[edi.lnid]
        ret


am_problemen_dealloc:
        pop ax
am_problemen_dealloc2:
        call dealloc_buffer
        mov eax,edi
        VMMCall List_Deallocate
am_problemen:
        xor eax,eax
        ret

EndProc alloc_memory


;
;Small subroutine, also used below
;


dealloc_buffer:
        mov ebx,[edi.npages]
        test ebx,ebx
        je db_return
        dec ebx
        mov [edi.npages],ebx
        mov eax,[edi.bufadr + 4*ebx]
        VMMCall _PageFree <eax, 0>
        jmp dealloc_buffer
db_return:
        ret



;******************************************************************************
;
;   free_memory_eax
;
;   Gooit geheugenblok (en node) geassoc. met node te eax weg
;
;==============================================================================

BeginProc free_memory_eax

        mov edi,eax
        mov ebx,[edi.vmid]
        mov eax,[edi.framesel]
        test eax,eax
        je fme_no_frameLDT
        VMMCall _Free_LDT_Selector <ebx,eax,0>          ;Does not use EBX
fme_no_frameLDT:
;        mov eax,[edi.bufsel]
;        test eax,eax
;        je fme_no_bufLDT
;        VMMCall _Free_LDT_Selector <ebx,eax,0>
;fme_no_bufLDT:
        mov eax,[edi.pagesel1]
        test eax,eax
        je fme_no_pageLDT
        VMMCall _Free_LDT_Selector <ebx,eax,0>
fme_no_pageLDT:
        call dealloc_buffer
        mov eax,[edi.frameadr]
        VMMCall _PageFree <eax, 0>
        mov esi,listhandle
        mov eax,edi
        VMMCall List_Remove
        mov eax,edi
        VMMCall List_Deallocate
        ret

EndProc free_memory_eax

;******************************************************************************
;
;   get_page
;
;   Retourneert in eax selector corresp met pagina ebx
;
;==============================================================================

BeginProc get_page

        cmp ebx,[edi.npages]
        jae mm_errorrng                 ;max bank # is npages-1
        cmp [edi.pageselpag1],ebx
        je gp_hev                       ;Hep um al
        cmp [edi.pageselpag1],-1
        jne gp_nohev                    ;Hep andere, jammer.
        mov [edi.pageselpag1],ebx

        mov eax,[edi.bufadr + 4*ebx]
        mov ebx,4                       ;4 4K pagina's = 16k
        mov ecx,RW_DATA_TYPE
        mov edx,D_GRAN_PAGE
        VMMCall _BuildDescriptorDWORDs <eax,ebx,ecx,edx,0>
        mov ebx,[edi.vmid]
        VMMCall _Allocate_LDT_Selector <ebx,edx,eax,1,0>
        mov [edi.pagesel1],eax

gp_hev: mov eax,[edi.pagesel1]
        clc
        ret

gp_nohev:
        mov ah,7
        stc
        ret

EndProc get_page


;******************************************************************************
;
;   map_memory
;
;   Mept pagina ebx in bank eax; in edi zit handle
;
;==============================================================================

BeginProc map_memory

        cmp eax,[edi.nbanks]
        jae mm_errorrng                 ;max bank # is nbanks-1
        cmp ebx,[edi.npages]
        jae mm_errorrng                 ;max pagina # is npages-1

        shl eax,14                      ;offset van bank
        add eax,[edi.frameadr]
        shr eax,12
        push eax
        push ebx
        VMMCall _PageDecommit <eax, 4, 0>       ;Decommit 16K
        test eax,eax
        pop ebx
        pop eax
        je mm_errordec

        mov ebx,[edi.bufphysadr + 4*ebx]        ;Linear address van pagina
        shr ebx,12
        VMMCall _PageCommitPhys <eax, 4, ebx, PC_INCR + PC_USER + PC_WRITEABLE>
        test eax,eax
        je mm_errorcom

        ret

mm_errordec:
        mov ah,4
        stc
        ret

mm_errorcom:
        mov ah,5
        stc
        ret

mm_errorrng:
        mov ah,6
        stc
        ret

EndProc map_memory


;******************************************************************************
;
;   get_frame
;
;   Alloceert LDT selector indien nodig, geeft in eax resultaat. EDI=node
;
;==============================================================================

BeginProc get_frame

        mov eax,[edi.framesel]
        test eax,eax
        jne fb_klaar

        mov eax,[edi.frameadr]
        mov ebx,[edi.nbanks]
        shl ebx,2                       ;4k pages
        mov ecx,RW_DATA_TYPE
        mov edx,D_GRAN_PAGE
        VMMCall _BuildDescriptorDWORDs <eax,ebx,ecx,edx,0>
        mov ebx,[edi.vmid]
        VMMCall _Allocate_LDT_Selector <ebx,edx,eax,1,0>
        mov [edi.framesel],eax

fb_klaar:
        ret


EndProc get_frame

VxD_CODE_ENDS








;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VZ80D_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's.
;
;
;==============================================================================

BeginProc VZ80D_Control

        Control_Dispatch Sys_Dynamic_Device_Init, VZ80D_Init_Complete
        Control_Dispatch Init_Complete, VZ80D_Init_Complete
        Control_Dispatch Sys_Dynamic_Device_Exit, VZ80D_Sys_Dyn_Dev_Exit
        Control_Dispatch System_Exit, VZ80D_System_Exit
        Control_Dispatch Destroy_VM, VZ80D_Destroy_VM
        clc
        ret

EndProc VZ80D_Control



;******************************************************************************
;
;   VZ80D_API_PROC
;
;   ah=0 -> ebx = 0100h
;   ah=1, ebx = # 16k paginas, ecx = # banks -> edi=handle (c=err)
;   ah=2, edi=handle -> es:edx = buffer (c=err) (DOET HET NIET MEER)
;   ah=3, edi=handle -> es:edx = frame (c=err)
;   ah=4, ebx=pagina, ecx=bank (0..3), edi=handle -> gemapt (c=err)
;   ah=5, edi=handle -> alles weg (c=err)
;   ah=6, ebx=pagina, edi=handle -> es:edx = 16k buffer van pagina (c=err)
;         (opm: Er kan maar 1 pagina gemapt worden op deze manier)
;   ah=7, ebx=adres, edi=handle -> al=waarde op ebx&3fff, bank ebx&c000
;   ah=8, ebx=adres, edi=handle, al=waarde -> waarde gepoked
;
;   ah niet in range -> carry geset, AH=0
;   handle invalid -> carry geset, AH=1
;   geen listhandle kunnen alloceren -> carry geset, AH=2
;   problemen bij allocatie -> carry geset, AH=3
;   problemen bij mappen -> carry geset, AH=4 (decomm) 5 (comm) 6 (range)
;   ah=6, er was al een pagina gemapt -> AH=7
;   ah=7,8: address buiten bereik -> AH=8
;
;==============================================================================

BeginProc VZ80D_API_PROC

        cmp listhandle,-1
        mov ah,2
        je vz80api_err

        and [ebp.client_EFLAGS],NOT cf_mask
        mov ebx,[ebp.client_EBX]
        mov ecx,[ebp.client_ECX]
        mov edx,[ebp.client_EDX]
        mov edi,[ebp.client_EDI]

        mov ah,[ebp.client_AH]
        cmp ah,0
        je vz80api_version
        cmp ah,1
        je vz80api_alloc

        call Zoek_Lijst
        mov ah,1
        jnz vz80api_err

        mov ah,[ebp.client_AH]
        cmp ah,2
        je vz80api_error
        cmp ah,3
        je vz80api_getframe
        cmp ah,4
        je vz80api_map
        cmp ah,5
        je vz80api_free
        cmp ah,6
        je vz80api_getpage
        cmp ah,7
        je vz80api_peek
        cmp ah,8
        je vz80api_poke

vz80api_error:
        mov ah,0
vz80api_err:
        or [ebp.client_EFLAGS],cf_mask
        mov [ebp.client_AH],ah
        clc
        ret

vz80api_version:
        mov [ebp.client_EBX],0102h
        clc
        ret

vz80api_alloc:
        mov eax,ebx
        call alloc_memory
        mov [ebp.client_EDI],eax
        test eax,eax
        mov ah,3
        je vz80api_err
        clc
        ret

vz80api_map:
        mov eax,ecx
        call map_memory
        jc vz80api_err
        clc
        ret

vz80api_free:
        mov eax,edi
        call free_memory_eax
        clc
        ret

vz80api_getpage:
        call get_page
        jc vz80api_err
        jmp vz80api_getptr

vz80api_getframe:
        call get_frame
vz80api_getptr:
        test eax,eax
        je vz80api_err
        mov [ebp.client_ES],ax
        mov [ebp.client_EDX],0
        clc
        ret

vz80api_peek:
        call get_addr
        jc vz80api_err
        mov al,[ebx]
        mov [ebp.client_AL],al
        clc
        ret

vz80api_poke:
        call get_addr
        jc vz80api_err
        mov al,[ebp.client_AL]
        mov [ebx],al
        clc
        ret

get_addr:
        mov eax,ebx
        shr ebx,14
        cmp ebx,[edi.npages]
        jae ga_err
        mov ebx,[edi.bufadr+4*ebx]
        and eax,3fffh
        add ebx,eax
        clc
        ret
ga_err:
        mov ah,8
        stc
        ret



EndProc VZ80D_API_PROC



VxD_LOCKED_CODE_ENDS

        END



