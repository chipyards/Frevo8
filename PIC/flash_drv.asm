; Mambo 2
; lecture/ecriture memoire flash programme PIC18
; adresses 16 bits


	PROCESSOR 18f452
	include "p18f452.inc"
        radix           dec             ; base de numeration par defaut
;
; declaration de symboles exportables
; void fread8( unsigned int adr, unsigned char * buf );
; void fwrite8( unsigned int adr, unsigned char * buf );
; void fera64( unsigned int adr );
	global	fread8
	global	fwrite8
	global	fera64

jln_lib1	CODE
;;;
;;;	fread8 : lecture 8 bytes
;;;
fread8
; Note : le premier arg est le plus "haut" dans la pile.
; Solution elegante mais un peu perilleuse (=utiliser INDF0
; pour ecrire dans FSR0...
;	movff	FSR1L,FSR0L	; copier stack pointer
;	movff	FSR1H,FSR0H
;	movf	POSTDEC0,W,ACCESS	; decrementer FSR0
;
;	movff	POSTDEC0,TBLPTRH	; adresse flash
;	movff	POSTDEC0,TBLPTRL
;
;	movf	POSTDEC0,W,ACCESS	; adresse buffer pour resu
;	movff	INDF0,FSR0L
;	movwf	FSR0H,ACCESS

; Solution robuste et pas plus chere...
	movlw	-2		; adresse flash
	movff	PLUSW1,TBLPTRL
	movlw	-1
	movff	PLUSW1,TBLPTRH
	clrf	TBLPTRU,ACCESS
	movlw	-4		; adresse buffer pour resultat
	movff	PLUSW1,FSR0L
	movlw	-3
	movff	PLUSW1,FSR0H
;

	movlw	8
fread8l	tblrd*+			; read - postincrement
	movff	TABLAT,POSTINC0
	addlw	-1
	btfss	STATUS,Z
	goto	fread8l

	return
;
;;;
;;;	fwrite8 : ecriture 8 bytes
;;;	(N.B.: verif adr et checksum supposees deja faites)
;;;
fwrite8
; Note : le premier arg est le plus "haut" dans la pile.
	movlw	-2		; adresse flash
	movff	PLUSW1,TBLPTRL
	movlw	-1
	movff	PLUSW1,TBLPTRH
	clrf	TBLPTRU,ACCESS
	movlw	-4		; adresse buffer donnees a ecrire
	movff	PLUSW1,FSR0L
	movlw	-3
	movff	PLUSW1,FSR0H
;
; copier data dans tampon ecriture
	movlw	8
fwri8l	movff	POSTINC0,TABLAT
	tblwt*+			; write - postincrement
	addlw	-1
	btfss	STATUS,Z
	goto	fwri8l
	tblrd*-			; decrement pointer to stay in same 8-byte page
;
	bcf	LATA, 4, ACCESS		; led on
;
; ecrire en flash (self timed)
	bsf	EECON1,EEPGD,ACCESS	; point to FLASH program memory
	bcf	EECON1,CFGS,ACCESS	; access regular ROM (not config ROM)
	bsf	EECON1,WREN,ACCESS	; enable write to memory
	bcf	INTCON,GIE,ACCESS	; disable interrupts
	movlw	0x55
	movwf	EECON2,ACCESS
	movlw	0xAA
	movwf	EECON2,ACCESS
	bsf	EECON1,WR		; start program (CPU stall)(self-resets)
	bsf	INTCON,GIE,ACCESS	; re-enable interrupts
	bcf	EECON1,WREN,ACCESS	; disable write to memory
;
	bsf	LATA, 4, ACCESS		; led off
	return
;
;;;
;;;	fera64 : effacement 64 bytes
;;;	(N.B.: verif adr et checksum supposees deja faites)
;;;
fera64
; Note : le premier arg est le plus "haut" dans la pile.
	movlw	-2		; adresse flash
	movff	PLUSW1,TBLPTRL
	movlw	-1
	movff	PLUSW1,TBLPTRH
	clrf	TBLPTRU,ACCESS
;
	bcf	LATA, 4, ACCESS		; led on
;
; effacer en flash (self timed)
	bsf	EECON1,EEPGD,ACCESS	; point to FLASH program memory
	bcf	EECON1,CFGS,ACCESS	; access regular ROM (not config ROM)
	bsf	EECON1,WREN,ACCESS	; enable write to memory
	bsf	EECON1,FREE,ACCESS	; flash rom erase enable (self-resets)
	bcf	INTCON,GIE,ACCESS	; disable interrupts
	movlw	0x55
	movwf	EECON2,ACCESS
	movlw	0xAA
	movwf	EECON2,ACCESS
	bsf	EECON1,WR		; start program (CPU stall)(self-resets)
	bsf	INTCON,GIE,ACCESS	; re-enable interrupts
	bcf	EECON1,WREN,ACCESS	; disable write to memory
;
	bsf	LATA, 4, ACCESS		; led off
	return
;
	end
