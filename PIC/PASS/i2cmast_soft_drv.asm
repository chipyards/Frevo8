; Driver Master I2C soft - version PIC18
; supporte le clock stretching de la part du slave
; sans limitation de duree
;
	PROCESSOR 18f452
	include "p18f452.inc"
        radix           dec             ; base de numeration par defaut
;
; declaration de symboles exportables	
	global	I2C_start
	global	I2C_stop
	global	I2C_stall	
	global	I2C_ack
	global	I2C_nack
	global	I2C_test_ack	
;
	global	I2C_tx_byte	
;
	global	I2C_rx_byte	
;
;
; ce driver 100% soft requiert seulement que S_CL et S_DA soient sur le meme port.
; ici on prend les bits des 4 premiers DACs de SBC18b :
;
#define	I2CPORT	PORTC
#define	I2CTRIS	TRISC
#define	I2CLAT	LATC
;
#define	S_CL	3
#define	S_DA	4
;
;	S_CL et S_DA sont "open drain"
;	toutes les fonctions sauf STOP laissent S_CL bas
;	toutes les fonctions sauf START laissent S_DA haut (hiz)
;
;	reservation des variables locales a cette unite
;	(unitialized data in access RAM)
;
i2cs_var	UDATA_ACS
shifreg	res	1
shifcnt	res	1
;
; 	programme
;
jln_lib3	CODE
;
I2C_start
	call	I2Ct			; I2Ct assure zero dans S_CL et S_DA
	bsf	INDF0,S_DA,ACCESS	; et INDF0 pointe sur I2CTRIS
	call	I2Ct
	bsf	INDF0,S_CL,ACCESS
	call	I2Ct
	bcf	INDF0,S_DA,ACCESS
	call	I2Ct
	bcf	INDF0,S_CL,ACCESS
	return
;
I2C_stop
	call	I2Ct
	bcf	INDF0,S_DA,ACCESS
I2C_stall
	call	I2Ct
	bsf	INDF0,S_CL,ACCESS
	call	I2Ct
	bsf	INDF0,S_DA,ACCESS
	return
;
I2C_ack
	call	I2Ct
	bcf	INDF0,S_DA,ACCESS
	call	I2CriseC
	call	I2CfallC
	call	I2Ct
	bsf	INDF0,S_DA,ACCESS
	return
;
I2C_nack
	call	I2Ct
	bsf	INDF0,S_DA,ACCESS
	call	I2CriseC
	call	I2CfallC
	return
;
I2C_test_ack	; gives 0xAC in W if ACK, else 0
	call	I2Ct
	bsf	INDF0,S_DA,ACCESS
	call	I2CriseC
	call	I2Ct
	movlw	0
	btfss	I2CPORT,S_DA
	movlw	0xAC
	bcf	INDF0,S_CL,ACCESS
	return
;
I2C_tx_byte
	movlw	-1
	movf	PLUSW1,W,ACCESS ; arg recupere dans W
	movwf   shifreg,ACCESS
	movlw   8
	movwf   shifcnt,ACCESS
i2ctlop
	call	I2Ct
	btfss   shifreg,7,ACCESS	; msb first
	bra	i2ctzero
	bsf	INDF0,S_DA,ACCESS
	bra	i2ctcont
i2ctzero
	bcf	INDF0,S_DA,ACCESS
	bra	i2ctcont
i2ctcont
	call	I2CriseC
	call	I2CfallC
	rlncf	shifreg,F,ACCESS
	decfsz  shifcnt,F,ACCESS
	bra	i2ctlop
;
	call	I2Ct
	bsf	INDF0,S_DA,ACCESS
	return
;
I2C_rx_byte		; resultat dans W
	call	I2Ct
	bsf	INDF0,S_DA,ACCESS
	movlw	8
	movwf	shifcnt
i2crlop
	call	I2CriseC
	rlncf	shifreg,F,ACCESS
	bcf	shifreg,0,ACCESS	; msb first
	call	I2Ct
	btfsc	I2CPORT,S_DA,ACCESS
	bsf	shifreg,0,ACCESS	; msb first
	bcf	INDF0,S_CL,ACCESS
	decfsz  shifcnt,F,ACCESS
	bra	i2crlop
;
	movf	shifreg,W,ACCESS
	return
;
;;;
;;;	I2Ct : tempo I2C, env 2 usec
;;;
I2Ct
	lfsr	0,I2CTRIS	; preparation acces I2CTRIS
				; indirect pour meilleure compat. PIC16
	bcf	I2CLAT,S_CL,ACCESS	; on maintient 0 dans S_CL et S_DA
	bcf	I2CLAT,S_DA,ACCESS	; attention ne pas utiliser bcf sur PORT
;
	nop
	nop
	nop
	return
;;;
;;;	I2CriseC : tempo, relacher SCL, attendre sa montee (sans limite)
;;;
I2CriseC
	call	I2Ct
	bsf	INDF0,S_CL,ACCESS
CriseC	btfsc	I2CPORT,S_CL,ACCESS
	return
;			; ici possible limitation duree
	goto	CriseC
;;;
;;;	I2CfallC : tempo, forcer SCL bas
;;;
I2CfallC
	call	I2Ct
	bcf	INDF0,S_CL,ACCESS
	return
; --------------------------------------------------------------------------
	end
;
; principales differences entre PIC16 et PIC18 dans ce code :
;	PIC16		PIC18
;------------------------------
;	INDF		INDF0
;	FSR		FSR0
;			,ACCESS
;	goto		bra
;	rlf		rlncf
;	movwf FSR	lfsr 0,k

