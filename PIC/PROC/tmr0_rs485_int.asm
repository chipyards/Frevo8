; Frevo7
; Traitement interruption timer + RS485

	PROCESSOR 18f452
	include "p18f452.inc"
        radix           dec             ; base de numeration par defaut
;

; fixed local static storage in access RAM (fix_var.c)
;
	extern	tmr0_roll	; compte 65536 ticks de 12.8 us (Xtal 20 MHz, prescale 64)
	extern	save_W

; fixed local static storage in banked RAM (fix_var.c)

	extern	save_status
	extern	save_fsr0	; pas utilise pour le timer

; local static storage in banked RAM
rs485_dat	UDATA
uabuf		res	32

; local static storage in access RAM
rs485_adat	UDATA_ACS
utcnt		res	1
uindex		res	1
urcnt		res	1

; exportation
	global	irtnl2		; point d'entree
	global	uabuf
	global	utcnt
	global	uindex
	global	urcnt
;
jln_lib3	CODE
;
irtnl2
	movwf	save_W, ACCESS
	movff	STATUS, save_status
;	
; Est-ce une interruption du timer ?
	btfss	INTCON, TMR0IF, ACCESS
	bra	nextirq
	bcf	INTCON, TMR0IF, ACCESS	; clear interrupt flag
	incf	tmr0_roll, F, ACCESS
	bra	nomoreirq

; Est-ce une interruption du transmetteur asynchrone ?
nextirq
	btfss	PIR1, TXIF, ACCESS	; no clear needed for this interrupt flag
	bra	nextirq2
	btfss	PIE1, TXIE, ACCESS	; but need to check that this int is enabled
	bra	nextirq2
	movff	FSR0L, save_fsr0	; sauver registre indirection
	movff	FSR0H, save_fsr0+1
;
;	bcf	LATA, 4, ACCESS		; led on
;
	movf	uindex, W, ACCESS
	cpfseq	utcnt, ACCESS		; est-ce temps d'arreter ?
	bra	txcont
	bcf	PIE1, TXIE, ACCESS	; disable Tx interrupt
	bcf	LATC, 5			; disable TX line driver
	bsf	RCSTA, CREN, ACCESS	; enable receiver
	bra	rxtxend
;
txcont	lfsr	0, uabuf		; pointer sur le buffer
	movff	PLUSW0, TXREG		; byte a emettre
	incf	uindex, F, ACCESS
	bra	rxtxend
;
; Est-ce une interruption du recepteur asynchrone ?
nextirq2
	btfss	PIR1, RCIF, ACCESS	; no clear needed for this interrupt flag
	bra	nomoreirq
	btfss	PIE1, RCIE, ACCESS	; but need to check that this int is enabled
	bra	nomoreirq
	movff	FSR0L, save_fsr0	; sauver registre indirection
	movff	FSR0H, save_fsr0+1
;
;	bcf	LATA, 4, ACCESS		; led on
;
	movf	urcnt, W, ACCESS	; pointer sur le buffer
	andlw	0x1F			; ne jamais deborder
	lfsr	0, uabuf
	movff	RCREG, PLUSW0 		; byte obtenu
	incf	urcnt, F, ACCESS
;
rxtxend
;	bsf	LATA, 4, ACCESS		; led off
;
	movff	save_fsr0, FSR0L	; restituer registre indirection
	movff	save_fsr0+1, FSR0H
;	bra	nomoreirq
;
nomoreirq
	movf	save_W, W, ACCESS	; ATTENTION movf change STATUS
	movff	save_status, STATUS
;
	retfie 0	; 0 for slow
;
	end
