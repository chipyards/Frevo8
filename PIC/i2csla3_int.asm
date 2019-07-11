; Frevo 7
; Traitement interruption MSSP  (I2C slave), prioritaire
; - code relogeable - ne doit pas etre directement en 0008 acr n'assure
;   pas la redirection de l'interruption low priority
; - interruption I2C derivee de Samba 3, mais indirection supplementaire pour
;   les buffers, ce qui permet de les mettre ou on veut tout en ayant des
;   variables fixes dans ce code
; - ATTENTION taille buffers determinee par
;	MASK dans i2csla_int.asm (--> OVFM)
; calcul checksum messages arrives

	PROCESSOR 18f452
	include "p18f452.inc"
        radix           dec             ; base de numeration par defaut
;
; bits de status I2C (istatus) : RT-oiiii
; les LSBs servent d'index temporaire avec 1 bit d'overflow 
MASK	equ	0x1F	; index pour buffers de 32 bytes
OVFM	equ	MASK+1
RX	equ	7
RXM	equ	1<<RX
TX	equ	6
TXM	equ	1<<TX
;
; fixed local static storage in access RAM (see fix_var.c)
	extern	istatus		; I2C status
	extern	txcnt		; pointeurs relatifs
	extern	rxcnt
	extern	rxsum		; checksum message rx (a initialiser par appli)
	extern	txsum		; checksum message tx (idem)

; fixed local static storage in banked RAM (see fix_var.c)
	extern	save_fsr0

; fixed local static storage in banked RAM
i2c_dat		UDATA
txbuf		res	OVFM
rxbuf		res	OVFM

; exportation
	global	txbuf
	global	rxbuf
	global  irtnh
;
jln_lib1	CODE
;;;
;;;	irtnh : W, STATUS et BSR sont deja sauves dans fast stack
;;;
irtnh	movff	FSR0L, save_fsr0	; sauver registre indirection
	movff	FSR0H, save_fsr0+1
;
;	bcf	LATA, 4, ACCESS		; led on
;
; Est-ce une interruption du MSSP ?
	btfss	PIR1, SSPIF, ACCESS
	bra	nextirq2
	bcf	PIR1, SSPIF, ACCESS	; ack this interrupt
;
; est-ce un stop ?
	btfss	SSPSTAT, P, ACCESS
	bra	else1
; quel type de transfert etait-ce ?
	btfss	istatus, RX, ACCESS
	bra	stop2
	movlw	OVFM + MASK		; fin reception
	andwf	istatus, F, ACCESS	; masquer compte
	movff	istatus, rxcnt		; reporter
	clrf	istatus, ACCESS
	bra	nextirq2			; nothing more to do
;
stop2	btfss	istatus, TX, ACCESS
	bra	nextirq2		; spurious stop ..
	clrf	istatus, ACCESS		; fin transmission
;	clrf	txcnt, ACCESS		; reporter
	bra	nextirq2		; nothing more to do
;
; y-a-t'il un byte a traiter ?
else1	btfss	SSPSTAT, BF, ACCESS	; tester BF car on peut arriver ici sur
	bra	else2			; un nak en emission ou un start 
; adresses ou donnee ?
	btfsc	SSPSTAT, D_A, ACCESS	; 0 <==> irq caused by address match
	bra	else3
	bsf	istatus, RX, ACCESS	; l'adresse annonce rx	
	movf	SSPBUF, W, ACCESS	; jeter adr byte pour eviter overflow
	movlw	-1
	movwf	rxcnt, ACCESS		; signaler
	bra	nextirq2		; PAS de controle de flux pour le moment
;
; la donnee est pour traffic reception
else3	movlw	MASK			; RX : nouvelle donnee est arrivee
	andwf	istatus, W, ACCESS	; masquer index
	lfsr	0, rxbuf		; pointer sur le buffer
	movff	SSPBUF, PLUSW0		; byte recu
	movf	PLUSW0, W, ACCESS
	addwf	rxsum, F, ACCESS	; incorpore dans checksum
	incf	istatus, F, ACCESS	; incrementer index
	movlw	RXM + OVFM + MASK
	andwf	istatus, F, ACCESS	; masquer index
	bra	nextirq2		; message trop long bouclera...
;
; le maitre demande-t-il des donnees ?
else2	btfss	SSPSTAT, R_W, ACCESS	; 1 <==> read request from master
	bra	startirq
	bsf	istatus, TX, ACCESS
	movlw	MASK			; TX : nouvelle donnee est demandee
	andwf	istatus, W, ACCESS	; masquer index
	lfsr	0, txbuf		; pointer sur le buffer
	cpfseq	txcnt, ACCESS		; est-ce temps d'envoyer checksum ?
	bra	txcont
	movff	txsum, PLUSW0		; checksum = byte a emettre
	negf	PLUSW0, ACCESS

txcont
	movff	PLUSW0, SSPBUF		; byte a emettre
	bsf	SSPCON1, CKP, ACCESS	; release clock stretch
	movf	PLUSW0, W, ACCESS
	addwf	txsum, F, ACCESS	; incorpore dans checksum
	incf	istatus, F, ACCESS	; incrementer index
	movlw	TXM + OVFM + MASK
	andwf	istatus, F, ACCESS	; masquer index
	bra	nextirq2		; requete trop longue bouclera...
;
startirq		; on arrive ici a chaque start mais il n'y a rien a faire
;
nextirq2
;	bsf	LATA, 4, ACCESS		; led off
;
	movff	save_fsr0, FSR0L	; restituer registre indirection
	movff	save_fsr0+1, FSR0H
;
	retfie	1	; 1 for fast
;

	end
