; vecteurs proteges
; normalement ils sont dans le startup de mcc18
; nous les rassemblons ici et nous utilisons un startup modifie

	extern	salto	; vect2.asm
	extern	irtnh	; i2csla_int.asm
	extern	flask	; flash.c
	extern	irtnl	; vect2.asm

vecto_scn	CODE

	goto	salto	; 0000 --> salto, second vecteur fixe dans finpr_scn
	nop
	nop
	goto	irtnh	; 0008 --> irtnh, code relogeable, dans jln_lib1
	nop
	nop
	goto	flask	; 0010 --> flask, code relogeable, dans jln_lib1
	nop
	nop
	goto	irtnl	; 0018 --> irtnl, second vecteur fixe dans finpr_scn

; bits de config
#ifdef __18F452
    #include <p18f452.inc>

    __CONFIG    _CONFIG1H, _OSCS_OFF_1H & _HS_OSC_1H ; _OSCS_OFF_1H is deflt
    __CONFIG    _CONFIG2L, _PWRT_ON_2L			; jln added
    __CONFIG    _CONFIG2H, _WDT_ON_2H & _WDTPS_128_2H	; jln enabled the WDT !
;    __CONFIG    _CONFIG3H, _CCP2MX_OFF_3H 
							; jln disables LVP
    __CONFIG    _CONFIG4L, _STVR_ON_4L & _LVP_OFF_4L & _DEBUG_OFF_4L
#endif

    END
