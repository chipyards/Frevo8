; historique :
;	Frevo7.PROC / PIC18F452 : tmr0 et uart mais specialise pour RS485 (half duplex)
;	JLFS2.FtdU / PIC18F2550 (USB) : pas de trm0 supprime mais buffer circulaire
;	KS/PASS18 / PIC18F2550 (USB) : idem (tencor/tube/PIC18 idem)
;	Frevo8.PASS / PIC18F452 : tmr0 et uart avec buffer circulaire (on remet tmr0)

; Traitement interruption uart "low priority"

	PROCESSOR 18f452
	include "p18f452.inc"
        radix           dec             ; base de numeration par defaut
;

; ATTENTION maintenir ces valeurs coherentes avec uart.h !
QBRU		equ	64	; puissance de 2 obligatoire
BRU_MSK 	equ	QBRU-1

; fixed local static storage in access RAM (fix_var.c)
;
	extern	tmr0_roll	; compte 65536 ticks de 12.8 us (Xtal 20 MHz, prescale 64)
	extern	save_W

; fixed local static storage in banked RAM (fix_var.c)

	extern	save_status
	extern	save_fsr0	; pas utilise pour le timer

; local static storage in banked RAM
uart_dat	UDATA
tx_fifo		res	QBRU	; fifo circulaire pour emission
rx_fifo		res	QBRU	; fifo circulaire pour reception

; local static storage in access RAM
uart_adat	UDATA_ACS
tx_wi		res	1	; index d'ecriture dans tx_fifo
tx_ri		res	1	; index de lecture dans tx_fifo

rx_wi		res	1	; index d'ecriture dans rx_fifo
rx_ri		res	1	; index de lecture dans rx_fifo

; exportation
	global	irtnl3		; point d'entree
	global	tx_fifo
	global	tx_wi
	global	tx_ri
	global	rx_fifo
	global	rx_wi
	global	rx_ri
;
jln_lib3	CODE
;
irtnl3
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
	movf	tx_ri, W, ACCESS
	cpfseq	tx_wi, ACCESS		; est-ce temps d'arreter ?
	bra	txcont
	bcf	PIE1, TXIE, ACCESS	; disable Tx interrupt
	bra	rxtxend
;
txcont	lfsr	0, tx_fifo		; pointer sur le buffer
					; W contient deja tx_ri
	movff	PLUSW0, TXREG		; byte a emettre
	incf	tx_ri, F, ACCESS
	movlw	BRU_MSK
	andwf	tx_ri, F, ACCESS
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
	movf	rx_wi, W, ACCESS	; pointer sur le buffer
	lfsr	0, rx_fifo
	movff	RCREG, PLUSW0 		; byte obtenu
	incf	rx_wi, F, ACCESS
	movlw	BRU_MSK
	andwf	rx_wi, F, ACCESS
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
