;-----------------------------------------------------------------------------
; Torque Shader Engine
; Copyright (C) GarageGames.com, Inc.
;-----------------------------------------------------------------------------


; syntax: export_fn <function name>
%macro export_fn 1
   %ifdef LINUX
   ; No underscore needed for ELF object files
   global %1
   %1:
   %else
   global _%1
   _%1:
   %endif
%endmacro

%macro global_var 1
   %ifdef LINUX
   global %1
   %else
   global _%1
   %define %1 _%1
   %endif
%endmacro

%macro extern_var 1
   %ifdef LINUX
   extern %1
   %else
   extern _%1
   %define %1 _%1
   %endif
%endmacro

; push registers 
%macro prologue 0
    push ebp
    mov ebp, esp    ; set up ebp for parameter access
;    pushad         ;
    push ebx
    push esi
    push edi
%endmacro

; pop registers
%macro epilogue 0
    pop edi
    pop esi
    pop ebx
;    popad
    pop ebp

%endmacro

segment .data

ix dd 0
iy dd 0

; times 2 dd == 64 bits == sizeof(QWORD) in blender.cc
delta_a times 2 dd 0
delta_b times 2 dd 0
delta_c times 2 dd 0
delta_d times 2 dd 0

alpha_a0 times 2 dd 0
alpha_b0 times 2 dd 0
alpha_c0 times 2 dd 0
alpha_d0 times 2 dd 0

alpha_a1 times 2 dd 0
alpha_b1 times 2 dd 0
alpha_c1 times 2 dd 0
alpha_d1 times 2 dd 0

alpha_c2 times 2 dd 0
alpha_d2 times 2 dd 0

ldelt_a times 2 dd 0
ldelt_b times 2 dd 0
ldelt_c times 2 dd 0
ldelt_d times 2 dd 0

rdelt_a times 2 dd 0
rdelt_b times 2 dd 0
rdelt_c times 2 dd 0
rdelt_d times 2 dd 0

zero times 2 dd 0

redLightMask   dw 0xf800, 0, 0, 0
greenLightMask dw 0x07c0, 0, 0, 0
blueLightMask  dw 0x003e, 0, 0, 0

delta2 times 2 dd 0
delta3 times 2 dd 0
rdelt_x2 times 2 dd 0
ldelt_x2 times 2 dd 0

leftq times 2 dd 0
rightq times 2 dd 0

rdeltq times 2 dd 0
ldeltq times 2 dd 0

mulfact     dw 0x2000, 0x0008, 0x2000, 0x0008 
redblue     dw 0x00f8, 0x00f8, 0x00f8, 0x00f8 
green       dw 0xf800, 0, 0xf800, 0 
alpha       dw 0x0001, 0x0001, 0, 0 

mask_7c0              dw 0x07c0, 0, 0, 0
mask_f8               dw 0x00f8, 0, 0, 0
mask_f800000000       dw 0, 0, 0x00f8, 0
mask_0000ffff0000ffff dw 0xffff, 0, 0xffff, 0
mask_00007fff00007fff dw 0x7fff, 0, 0x7fff, 0

; externs for global variables declared in blender.cc
extern_var lumels

; declare global variables
global_var sTargetTexelsPerLumel_log2
global_var sTargetTexelsPerLumel
global_var sTargetTexelsPerLumelDiv2
global_var nextsrcrow
global_var nextdstrow
global_var mip0_dstrowadd
global_var mip1_dstrowadd
global_var minus1srcrowsPlus8
global_var srcrows_x2_MinusTPL

; define global variables
sTargetTexelsPerLumel_log2 dd 0
sTargetTexelsPerLumel      dd 0
sTargetTexelsPerLumelDiv2  dd 0

nextsrcrow                 dd 0
nextdstrow                 dd 0
mip0_dstrowadd             dd 0
mip1_dstrowadd             dd 0
minus1srcrowsPlus8         dd 0
srcrows_x2_MinusTPL        dd 0


segment .text

; parameter accessors for all of the doSquareX functions
%define dst [ebp+8]
%define sq_shift [ebp+12]
%define aoff [ebp+16]
%define bmp_ptrs [ebp+20]
%define alpha_ptrs [ebp+24]

; void doSquare4( 
;   U32 *dst, 
;   int sq_shift, 
;   const int *aoff, 
;   const U32 *const *bmp_ptrs, 
;   const U8 *const *alpha_ptrs );
export_fn doSquare4

    prologue

    ; init iy
	mov eax, 1
	mov cl, sq_shift
	shl eax, cl
	mov dword [iy], eax

    ; init ix
	shr eax, 1
	mov dword [ix], eax

        movd    mm1, sq_shift
        ; get alpha values for the corners of the square for each texture type.
        ;  replicate the values into 4 words of the qwords.  Also calc vertical
        ;  stepping values for the alpha values on left and right edges.
        ; load alpha value into bh to mul by 256 for precision. then
        ; punpcklwd mm0, mm0 followed by punpckldq mm0, mm0 
        ;  to replicate the low word into all words of mm0.
        ; shift down difference by sqshift to divide by pixels per square to get
        ;  increment.

        mov     esi, aoff
        mov     edi, alpha_ptrs
        mov     eax, [edi]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx,ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_a0], mm2
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_a1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_a], mm0
        psraw   mm3, mm1
        movq    [rdelt_a], mm3

        mov     eax, [edi+4]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_b0], mm2
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_b1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_b], mm0
        psraw   mm3, mm1
        movq    [rdelt_b], mm3

        mov     eax, [edi+8]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_c0], mm2
        movq    [alpha_c2], mm0
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_c1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_c], mm0
        psraw   mm3, mm1
        movq    [rdelt_c], mm3

        mov     eax, [edi+12]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_d0], mm2
        movq    [alpha_d2], mm0
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_d1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_d], mm0
        psraw   mm3, mm1
        movq    [rdelt_d], mm3

        mov     esi, bmp_ptrs
        mov     eax, [esi]
        mov     ebx, [esi+4]
        mov     ecx, [esi+8]
        mov     edx, [esi+12]

        movq    mm0, [alpha_a1]
        movq    mm2, [alpha_b1]
        movq    mm3, [alpha_c1]
        movq    mm4, [alpha_a0]
        movq    mm5, [alpha_b0]
        movq    mm6, [alpha_c0]
        movq    mm7, [alpha_d0]
        mov     edi, dst

yloop4:
        ; mm1 should be sq_shift at this point

        ; calculate alpha step increments...word-size steps are replicated
        ;  to fill qword.
        psubw   mm0, mm4
        psraw   mm0, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_a], mm0         ;delta = ainc ainc ainc ainc
        
        psubw   mm2, mm5
        psraw   mm2, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_b], mm2         ;delta = ainc ainc ainc ainc
        
        psubw   mm3, mm6
        psraw   mm3, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_c], mm3         ;delta = ainc ainc ainc ainc
        
        movq    mm0, [alpha_d1]
        psubw   mm0, mm7
        psraw   mm0, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_d], mm0         ;delta = ainc ainc ainc ainc

        mov     esi, [ix]
        pxor    mm2, mm2

xloop4:
        movq    mm0, [eax]
        movq    mm1, mm0
        punpcklbw mm0, mm2
        pmulhw  mm0, mm4
        paddw   mm4, [delta_a]
        punpckhbw mm1, mm2
        pmulhw  mm1, mm4
        paddw   mm4, [delta_a]
        packuswb mm0, mm1

        movq    mm3, [ebx]
        movq    mm1, mm3
        punpcklbw mm3, mm2
        pmulhw  mm3, mm5
        paddw   mm5, [delta_b]
        punpckhbw mm1, mm2
        pmulhw  mm1, mm5
        paddw   mm5, [delta_b]
        packuswb mm3, mm1
        paddb   mm0, mm3

        movq    mm3, [ecx]
        movq    mm1, mm3
        punpcklbw mm3, mm2
        pmulhw  mm3, mm6
        paddw   mm6, [delta_c]
        punpckhbw mm1, mm2
        pmulhw  mm1, mm6
        paddw   mm6, [delta_c]
        packuswb mm3, mm1
        paddb   mm0, mm3

        movq    mm3, [edx]
        movq    mm1, mm3
        punpcklbw mm3, mm2
        pmulhw  mm3, mm7
        paddw   mm7, [delta_d]
        punpckhbw mm1, mm2
        pmulhw  mm1, mm7
        paddw   mm7, [delta_d]
        packuswb mm3, mm1
        paddb   mm0, mm3

        ; double result, to make up for alpha vals being signed (max = 127)
        ; so our math turns out a bit short, example:
        ;  (0x7f00 * 0xff) >> 16 = 0x7e....* 2 = 252...not quite 255
        ; would have been (0xff00 * 0xff) >> 16 = 0xfe = 254, 
        ;  if I could do an unsigned pmulhw...
        ; pmulhuw is in an intel document I found, but doesn't compile....
        paddb   mm0, mm0        

        movq    [edi], mm0

        add     eax, 8
        add     ebx, 8
        add     ecx, 8
        add     edx, 8
        add     edi, 8

        dec     esi
        jnz near xloop4

        movq    mm4, [alpha_a0]
        paddw   mm4, [ldelt_a]
        movq    [alpha_a0], mm4

        movq    mm5, [alpha_b0]
        paddw   mm5, [ldelt_b]
        movq    [alpha_b0], mm5

        movq    mm6, [alpha_c0]
        paddw   mm6, [ldelt_c]
        movq    [alpha_c0], mm6

        movq    mm7, [alpha_d0]
        paddw   mm7, [ldelt_d]
        movq    [alpha_d0], mm7

        movq    mm0, [alpha_d1]
        paddw   mm0, [rdelt_d]
        movq    [alpha_d1], mm0

        movq    mm2, [alpha_b1]
        paddw   mm2, [rdelt_b]
        movq    [alpha_b1], mm2

        movq    mm3, [alpha_c1]
        paddw   mm3, [rdelt_c]
        movq    [alpha_c1], mm3

        movq    mm0, [alpha_a1]
        paddw   mm0, [rdelt_a]
        movq    [alpha_a1], mm0

        movd    mm1, sq_shift       ; top of loop expects this

        dec     dword [iy]
        jnz near yloop4

        emms

    epilogue

    ret



; void doSquare3( 
;   U32 *dst, 
;   int sq_shift, 
;   const int *aoff, 
;   const U32 *const *bmp_ptrs, 
;   const U8 *const *alpha_ptrs );
export_fn doSquare3

    prologue

    ; init iy
	mov eax, 1
	mov cl, sq_shift
	shl eax, cl
	mov dword [iy], eax

    ; init ix
	shr eax, 1
	mov dword [ix], eax

        movd    mm1, sq_shift
        ; get alpha values for the corners of the square for each texture type.
        ;  replicate the values into 4 words of the qwords.  Also calc vertical
        ;  stepping values for the alpha values on left and right edges.
        ; load alpha value into bh to mul by 256 for precision. then
        ; punpcklwd mm0, mm0 followed by punpckldq mm0, mm0 
        ;  to replicate the low word into all words of mm0.
        ; shift down difference by sqshift to divide by pixels per square to get
        ;  increment.

        mov     esi, aoff
        mov     edi, alpha_ptrs
        mov     eax, [edi]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_a0], mm2
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_a1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_a], mm0
        psraw   mm3, mm1
        movq    [rdelt_a], mm3

        mov     eax, [edi+4]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_b0], mm2
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_b1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_b], mm0
        psraw   mm3, mm1
        movq    [rdelt_b], mm3

        mov     eax, [edi+8]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_c0], mm2
        movq    [alpha_c2], mm0
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_c1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_c], mm0
        psraw   mm3, mm1
        movq    [rdelt_c], mm3

        mov     esi, bmp_ptrs
        mov     eax, [esi]
        mov     ebx, [esi+4]
        mov     ecx, [esi+8]

        movq    mm0, [alpha_a1]
        movq    mm2, [alpha_b1]
        movq    mm3, [alpha_c1]
        movq    mm4, [alpha_a0]
        movq    mm5, [alpha_b0]
        movq    mm6, [alpha_c0]
        mov     edi, dst

yloop3:
        ; mm1 should be sq_shift at this point
        ; mm0 should be alpha_a1
        ; mm2 should be alpha_b1
        ; mm3 should be alpha_c1

        ; calculate alpha step increments...word-size steps are replicated
        ;  to fill qword.
        psubw   mm0, mm4
        psraw   mm0, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_a], mm0         ;delta = ainc ainc ainc ainc
        
        psubw   mm2, mm5
        psraw   mm2, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_b], mm2         ;delta = ainc ainc ainc ainc
        
        psubw   mm3, mm6
        psraw   mm3, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_c], mm3         ;delta = ainc ainc ainc ainc
        
        mov     esi, [ix]
        pxor    mm2, mm2


        movq    mm7, [delta_a]
xloop3:
        movq    mm0, [eax]
        movq    mm1, mm0
        punpcklbw mm0, mm2
        pmulhw  mm0, mm4
        paddw   mm4, mm7
        punpckhbw mm1, mm2
        pmulhw  mm1, mm4
        paddw   mm4, mm7
        packuswb mm0, mm1

        movq    mm3, [ebx]
        movq    mm1, mm3
        punpcklbw mm3, mm2
        pmulhw  mm3, mm5
        paddw   mm5, [delta_b]
        punpckhbw mm1, mm2
        pmulhw  mm1, mm5
        paddw   mm5, [delta_b]
        packuswb mm3, mm1
        paddb   mm0, mm3

        movq    mm3, [ecx]
        movq    mm1, mm3
        punpcklbw mm3, mm2
        pmulhw  mm3, mm6
        paddw   mm6, [delta_c]
        punpckhbw mm1, mm2
        pmulhw  mm1, mm6
        paddw   mm6, [delta_c]
        packuswb mm3, mm1
        paddb   mm0, mm3
        paddb   mm0, mm0

        movq    [edi], mm0

        add     eax, 8
        add     ebx, 8
        add     ecx, 8
        add     edi, 8

        dec     esi
        jnz     near xloop3

        movq    mm4, [alpha_a0]
        paddw   mm4, [ldelt_a]
        movq    [alpha_a0], mm4

        movq    mm5, [alpha_b0]
        paddw   mm5, [ldelt_b]
        movq    [alpha_b0], mm5

        movq    mm6, [alpha_c0]
        paddw   mm6, [ldelt_c]
        movq    [alpha_c0], mm6

        movq    mm2, [alpha_b1]
        paddw   mm2, [rdelt_b]
        movq    [alpha_b1], mm2

        movq    mm3, [alpha_c1]
        paddw   mm3, [rdelt_c]
        movq    [alpha_c1], mm3

        movq    mm0, [alpha_a1]
        paddw   mm0, [rdelt_a]
        movq    [alpha_a1], mm0

        movd    mm1, sq_shift       ; top of loop expects this

        dec     dword [iy]
        jnz     near yloop3

        emms

    epilogue

    ret



; void doSquare2( 
;   U32 *dst, 
;   int sq_shift, 
;   const int *aoff, 
;   const U32 *const *bmp_ptrs, 
;   const U8 *const *alpha_ptrs );
export_fn doSquare2

    prologue

    ; init iy
	mov eax, 1
	mov cl, sq_shift
	shl eax, cl
	mov dword [iy], eax

    ; init ix
	shr eax, 1
	mov dword [ix], eax

        movd    mm1, sq_shift
        ; get alpha values for the corners of the square for each texture type.
        ;  replicate the values into 4 words of the qwords.  Also calc vertical
        ;  stepping values for the alpha values on left and right edges.
        ; punpcklwd mm0, mm0 followed by punpckldq mm0, mm0 
        ;  to replicate the low word into all words of mm0.
        ; shift down difference by sqshift to divide by pixels per square to get
        ;  increment.

        mov     esi, aoff
        mov     edi, alpha_ptrs
        mov     eax, [edi]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_a0], mm2
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_a1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_a], mm0
        psraw   mm3, mm1
        movq    [rdelt_a], mm3

        mov     eax, [edi+4]
        mov     edx, eax
        add     eax, [esi]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm2, ebx
        punpcklwd mm2, mm2
        add     eax, [esi+8]
        punpckldq mm2, mm2
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm0, ebx
        punpcklwd mm0, mm0
        punpckldq mm0, mm0
        movq    [alpha_b0], mm2
        psubw   mm0, mm2
        add     eax, [esi+4]
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        mov     eax, edx
        movd    mm4, ebx
        punpcklwd mm4, mm4
        add     eax, [esi+12]
        punpckldq mm4, mm4
        xor     ebx, ebx
        mov     bl, [eax]
        shl     ebx, 7
        movd    mm3, ebx
        movq    [alpha_b1], mm4
        punpcklwd mm3, mm3
        punpckldq mm3, mm3
        psraw   mm0, mm1
        psubw   mm3, mm4
        movq    [ldelt_b], mm0
        psraw   mm3, mm1
        movq    [rdelt_b], mm3

        mov     esi, bmp_ptrs
        mov     eax, [esi]
        mov     ebx, [esi+4]

        movq    mm0, [alpha_a1]
        movq    mm2, [alpha_b1]
        movq    mm4, [alpha_a0]
        movq    mm5, [alpha_b0]
        mov     edi, dst

yloop2:
        ; mm1 should be sq_shift at this point
        ; mm0 should be alpha_a1
        ; mm2 should be alpha_b1

        ; calculate alpha step increments...word-size steps are replicated
        ;  to fill qword.
        psubw   mm0, mm4
        psraw   mm0, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_a], mm0         ;delta = ainc ainc ainc ainc
        
        psubw   mm2, mm5
        psraw   mm2, mm1            ;mm0 = (right-left) >> sq_shift
        movq    [delta_b], mm2         ;delta = ainc ainc ainc ainc
        
        mov     esi, [ix]
        pxor    mm2, mm2

        movq    mm6, [delta_a]
        movq    mm7, [delta_b]

xloop2:
        movq    mm0, [eax]
        movq    mm3, [ebx]

        movq    mm1, mm0
        punpcklbw mm0, mm2
        pmulhw  mm0, mm4
        paddw   mm4, mm6
        punpckhbw mm1, mm2
        pmulhw  mm1, mm4
        paddw   mm4, mm6
        packuswb mm0, mm1

        movq    mm1, mm3
        punpcklbw mm3, mm2
        pmulhw  mm3, mm5
        paddw   mm5, mm7
        punpckhbw mm1, mm2
        pmulhw  mm1, mm5
        paddw   mm5, mm7
        packuswb mm3, mm1
        paddb   mm0, mm3
        paddb   mm0, mm0

        movq    [edi], mm0

        add     edi, 8
        add     eax, 8
        add     ebx, 8

        dec     esi
        jnz     xloop2

        movq    mm4, [alpha_a0]
        paddw   mm4, [ldelt_a]
        movq    [alpha_a0], mm4

        movq    mm5, [alpha_b0]
        paddw   mm5, [ldelt_b]
        movq    [alpha_b0], mm5

        movq    mm2, [alpha_b1]
        paddw   mm2, [rdelt_b]
        movq    [alpha_b1], mm2

        movq    mm0, [alpha_a1]
        paddw   mm0, [rdelt_a]
        movq    [alpha_a1], mm0

        movd    mm1, sq_shift       ; top of loop expects this

        dec     dword [iy]
        jnz     near yloop2

        emms

    epilogue

    ret



; params for doLumelPlus1Mip
%define dstmip0 [ebp+8]
%define dstmip1 [ebp+12]
%define srcptr [ebp+16]

; void doLumelPlus1Mip( U16 *dstmip0, U16 *dstmip1, const U32 *srcptr );
export_fn doLumelPlus1Mip

    prologue

        movd    mm7, [sTargetTexelsPerLumel_log2]

        movd    mm0, [lumels]
        movq    mm4, mm0
        pand    mm0, [redLightMask]
        movq    mm5, mm4
        pand    mm4, [greenLightMask]
        psllq   mm0, 31
        pand    mm5, [blueLightMask]
        psllq   mm4, 20
        paddw   mm0, mm4
        psllq   mm5, 9
        paddw   mm0, mm5        ; mm0 = 0000rrrrggggbbbb qword for lp[0]
        movq    [leftq], mm0

        movd    mm1, [lumels+8]    ; get lp2
        movq    mm4, mm1
        pand    mm1, [redLightMask]
        movq    mm5, mm4
        pand    mm4, [greenLightMask]
        psllq   mm1, 31
        pand    mm5, [blueLightMask]
        psllq   mm4, 20
        paddw   mm1, mm4
        psllq   mm5, 9
        paddw   mm1, mm5        ; mm1 = 0000rrrrggggbbbb qword for lp[2]

        psubw   mm1, mm0
        psraw   mm1, mm7
        movq    [ldeltq], mm1
        psllw   mm1, 1
        movq    [ldelt_x2], mm1


        movd    mm2, [lumels+4]    ; get lp[1]
        movq    mm4, mm2
        pand    mm2, [redLightMask]
        movq    mm5, mm4
        pand    mm4, [greenLightMask]
        psllq   mm2, 31
        pand    mm5, [blueLightMask]
        psllq   mm4, 20
        paddw   mm2, mm4
        psllq   mm5, 9
        paddw   mm2, mm5        ; mm2 = 0000rrrrggggbbbb qword for lp[1]
        movq    [rightq], mm2

        movd    mm3, [lumels+12]    ; get lp3
        movq    mm4, mm3
        pand    mm3, [redLightMask]
        movq    mm5, mm4
        pand    mm4, [greenLightMask]
        psllq   mm3, 31
        pand    mm5, [blueLightMask]
        psllq   mm4, 20
        paddw   mm3, mm4
        psllq   mm5, 9
        paddw   mm3, mm5        ; mm3 = 0000rrrrggggbbbb qword for lp[3]

        psubw   mm3, mm2
        psraw   mm3, mm7
        movq    [rdeltq], mm3
        psllw   mm3, 1
        movq    [rdelt_x2], mm3

        mov     edi, dstmip0
        mov     esi, srcptr
        mov     edx, dstmip1
        pxor    mm6, mm6

        mov     ecx, [sTargetTexelsPerLumelDiv2] ; yloop count
        movq    mm2, [leftq]
        movq    mm3, [rightq]

        ; mm2 is left, mm3 is right
yloop_dlpm:
        movd    mm7, [sTargetTexelsPerLumel_log2]
        movq    mm6, mm2
        movq    mm1, mm2        ; mm1 is light1
        movq    mm5, mm3
        movq    mm4, mm3

        paddw   mm5, [rdeltq]     ; right + rdelt
        psubw   mm4, mm6        ; right - left
        paddw   mm6, [ldeltq]     ; left + ldelt
        psraw   mm4, mm7        
        movq    [delta2], mm4

        psubw   mm5, mm6        ; mm6 is light2
        psraw   mm5, mm7
        movq    [delta3], mm5

        mov     ebx, [sTargetTexelsPerLumelDiv2] ; loop count
                      
        ; do 4 source pixels per loop
        ; mm1 is light1
        ; mm6 is light2
        pxor    mm7, mm7

xloop_dlpm:
        ; get first of source, col 0 and 1
        movq    mm4, [esi]
        add     esi, [nextsrcrow]
        movq    mm5, mm4
        punpcklbw mm4, [zero]
        pmulhw  mm4, mm1        ; mm1 is light factor for first row
        paddw   mm1, [delta2]
        punpckhbw mm5, [zero]
        pmulhw  mm5, mm1
        paddw   mm1, [delta2]

        movq    mm7, [esi]
        add     esi, [minus1srcrowsPlus8]

        movq    mm0, mm4
        paddw   mm0, mm5        ; mm0 is the avg, for mip1[0,1]

        packuswb    mm4, mm5          ; put both pixels in same qword
        paddw       mm4, mm4          ; double it, because lighting mul halved it
        movq        mm5, mm4          ; save the original data
        pand        mm4, [redblue]      ; mask out all but the 5MSBits of red and blue
        pmaddwd     mm4, [mulfact]      ; multiply each word by
                                      ;   2^13, 2^3, 2^13, 2^3 and add results
        pand        mm5, [green]        ; mask out all but the 5MSBits of green
        por         mm4, mm5          ; combine the red, green, and blue bits
        psrld       mm4, 6            ; shift into position
        packssdw    mm4, [zero]         ; pack into single dword
        pslld       mm4, 1            ; shift into final position
        por         mm4, [alpha]        ; add the alpha bit

        ; write 2 pixels to mip0
        movd    [edi], mm4

        ; get second row, cols 0 and 1
        movq    mm5, mm7
        pxor    mm4, mm4
        punpcklbw mm7, mm4
        pmulhw  mm7, mm6        ; mm6 is light factor for 2nd row
        paddw   mm6, [delta3]
        punpckhbw mm5, mm4
        pmulhw  mm5, mm6
        paddw   mm6, [delta3]
        paddw   mm0, mm7
        paddw   mm0, mm5
        psrlw   mm0, 1          ; mm0 is mip1[0,1] average
        
        packuswb    mm7, mm5          ; put both pixels in same qword
        paddw       mm7, mm7          ; double it, because lighting mul halved it
        movq        mm5, mm7          ; save the original data
        pand        mm7, [redblue]      ; mask out all but the 5MSBits of red and blue
        pmaddwd     mm7, [mulfact]      ; multiply each word by
                                      ;   2^13, 2^3, 2^13, 2^3 and add results
        pand        mm5, [green]        ; mask out all but the 5MSBits of green
        por         mm7, mm5          ; combine the red, green, and blue bits
        psrld       mm7, 6            ; shift into position
        packssdw    mm7, mm4         ; pack into single dword
        pslld       mm7, 1            ; shift into final position
        por         mm7, [alpha]        ; add the alpha bit

        ; write 2 16-bit pixels to mip0, 2nd row
        movd    [edi+0x100], mm7

        movq        mm5, mm0
        movq        mm4, mm0
        pand        mm4, [mask_f8]    ; red
        psrlq       mm5, 13
        pand        mm5, [mask_7c0]   ; green
        psllq       mm4, 8
        pand        mm0, [mask_f800000000]    ; blue
        paddw       mm5, mm4
        psrlq       mm0, 34
        paddw       mm0, mm5

        ; write 1 pixels to mip1
        movd        eax, mm0
        mov         [edx], ax

        ; increment ptrs
        add     edx, 2      ; mip1
        add     edi, 4      ; mip0

        dec     ebx
        jnz     near xloop_dlpm

        add     esi, [srcrows_x2_MinusTPL]
        add     edx, [mip1_dstrowadd]
        add     edi, [mip0_dstrowadd]
        paddw   mm2, [ldelt_x2]       ; mm2 is left
        paddw   mm3, [rdelt_x2]       ; mm3 is right
        dec     ecx
        jnz     near yloop_dlpm

        emms

    epilogue

    ret



; params for do1x1Lumel
%define dstptr [ebp+8]
%define srcptr [ebp+12]

; void do1x1Lumel( U16 *dstptr, const U32 *srcptr );
export_fn do1x1Lumel

    prologue

        movd    mm0, [lumels]
        movq    mm4, mm0
        pand    mm0, [redLightMask]
        movq    mm5, mm4
        pand    mm4, [greenLightMask]
        psllq   mm0, 31
        pand    mm5, [blueLightMask]
        psllq   mm4, 20
        paddw   mm0, mm4
        psllq   mm5, 9
        paddw   mm0, mm5        ; mm0 = 0000rrrrggggbbbb qword for lp[0]

        mov     edi, dstptr
        mov     esi, srcptr
        pxor    mm6, mm6

        movd    mm4, [esi]
        punpcklbw mm4, mm6      ; mm6 is expected to be 0 here
        pmulhw  mm4, mm0
        paddw   mm4, mm4

        movq    mm7, mm4
        movq    mm6, mm4
        psrlq   mm4, 34
        pand    mm7, [mask_f8]
        psrlq   mm6, 13
        psllq   mm7, 8
        pand    mm6, [mask_7c0]
        paddw   mm4, mm7
        paddw   mm4, mm6
        movd    eax, mm4
        mov     [edi],ax
        emms

    epilogue

    ret

; params for cheatmips
%define srcptr [ebp+8]
%define dstmip0 [ebp+12]
%define dstmip1 [ebp+16]
%define wid [ebp+20]

; void cheatmips( U16 *srcptr, U16 *dstmip0, U16 *dstmip1, int wid );
export_fn cheatmips

    prologue

        mov     esi, srcptr
        mov     edi, dstmip0
        mov     edx, dstmip1

        mov     ecx, wid
        shr     ecx, 1
        mov     eax, ecx
        shr     eax, 3
        shl     dword wid, 1

        movq    mm6, [mask_0000ffff0000ffff]
        movq    mm7, [mask_00007fff00007fff]

yloop_cm:
        mov     ebx, eax
xloop_cm:
        movq    mm0, [esi]
        movq    mm1, [esi+8]
        movq    mm2, [esi+16]
        movq    mm3, [esi+24]

        pand    mm0, mm6
        psrlw   mm1, 1
        pand    mm1, mm7
        psrlw   mm0, 1
        pand    mm2, mm6
        psrlw   mm3, 1
        pand    mm3, mm7
        psrlw   mm2, 1
        packssdw mm0, mm1
        packssdw mm2, mm3
        psllw   mm0, 1          ;mip1, qw 0
        movq    [edi], mm0
        psllw   mm2, 1          ;mip1, qw 1
        movq    [edi+8], mm2

        test    ecx, 1
        jnz     nomip2

        movq    mm1, mm0
        movq    mm3, mm2
        pand    mm1, mm6
        psrlw   mm3, 1
        pand    mm3, mm7
        psrlw   mm1, 1
        packssdw mm1, mm3
        psllw   mm1, 1          ;mip2, qw 0
        movq    [edx], mm1
        add     edx, 8
nomip2:

        add     esi, 32
        add     edi, 16

        dec     ebx
        jnz     xloop_cm

        add     esi, wid
        dec     ecx
        jnz     near yloop_cm

        emms

    epilogue

    ret

; params for cheatmips4x4
%define srcptr [ebp+8]
%define dstmip0 [ebp+12]
%define dstmip1 [ebp+16]

; void cheatmips4x4( U16 *srcptr, U16 *dstmip0, U16 *dstmip1 );
export_fn cheatmips4x4

    prologue

        mov     esi, srcptr
        mov     edi, dstmip0
        mov     edx, dstmip1

        movq    mm0, [esi]
        movq    mm1, [esi+16]
        pand    mm0, [mask_0000ffff0000ffff]
        psrlw   mm1, 1
        pand    mm1, [mask_00007fff00007fff]
        psrlw   mm0, 1
        packssdw mm0, mm1
        psllw   mm0, 1          ; mip1, qw 0
        movq    [edi], mm0

        movd    eax, mm0
        mov     [edx], ax

        emms

    epilogue

    ret
