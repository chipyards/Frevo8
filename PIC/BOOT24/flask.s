; driver acces memoire flash
; instr-words de 24 bits
; adresses flash en "PC units" (toujours paires)
; gerees sur 32 bits mais incrementees sur 16 seulement
; ( dans 1 acces on ne franchit pas la frontiere des 64k PC-units soit 96 kbytes )
; 1 instr-word occupe 2 PC-units contenant 3 bytes

        .include "p24Fxxxx.inc"

	.section .text
	.global _flashr
	.global _ferase
	.global _flashw
	.global _fgorun

;;;
;;;	lecture flash 1 row de 192 bytes (128 PC units) : 
;;;	void flashr( unsigned long adr, unsigned char * buf );
; 
; - les parametres seront passes dans w0, w1 et w2 [attention w8 - w15 sont reserves ]
;   adr DOIT etre pair, buf peut etre impair
; - en C l'underscore initial sera ajoute par le compilateur

_flashr:
	MOV	w1, TBLPAG	; upper byte de l'adresse
;	DO	#63, freal	; instr DSP 30F non supportee par 24H
	MOV	#64, w4		; 64 inst-word pour 128 PC units

frlop:	TBLRDL	[w0], w3	; Read low word to w3
	MOV.B	w3, [w2++]	; 1 byte a la fois car on n'est pas sur de l'alignement
	SWAP	w3
	MOV.B	w3, [w2++]

	TBLRDH	[w0++], w3	; Read high byte to W3
				; ici il faut bien incrementer w0 par 2
				; cependant que w2 a ete incremente par 3
freal:	MOV.B	w3, [w2++]

	SUB	#1, w4
	BRA	NZ, frlop
	return
;;;
;;;	effacement flash 1 page de 1536 bytes (1024 PC units) soit 8 rows
;;;	void ferase( unsigned long adr );
;
; adr doit etre aligne sur un multiple de 1024
; 
_ferase:
	MOV	#0x4042, w3	; code to erase 1 page
	MOV	w3, NVMCON

	MOV	w1, TBLPAG	; upper byte de l'adresse
	TBLWTL	w0, [w0]	; dummy write, set low word of address of erase block

	DISI #5			; Block all interrupts with priority <7 for next 5 instructions

	MOV	#0x55, w0	; the KEY sequence
	MOV	w0, NVMKEY
	MOV	#0xAA, w0
	MOV	w0, NVMKEY

	BSET	NVMCON, #WR	; GO!

	NOP			; two NOPs (required)
	NOP

	return
;;;
;;;	ecriture flash 1 row de 192 bytes (128 PC units) : 
;;;	void flashw( unsigned long adr, unsigned char * buf );
; 
; - les parametres seront passes dans w0, w1 et w2 [attention w8 - w15 sont reserves ]
;   adr DOIT etre pair, buf peut etre impair
;
_flashw:
	MOV	#0x4001, w3	; code to write 1 row
	MOV	w3, NVMCON

	MOV	w1, TBLPAG	; upper byte de l'adresse
;	DO	#63, fwril	; instr DSP 30F non supportee par 24H
	MOV	#64, w5		; 64 inst-word pour 128 PC units

fwlop:	ZE	[w2++], w3	; 1 byte a la fois car on n'est pas sur de l'alignement
	ZE	[w2++], w4	; ZE est un MOV.B qui produit un word comme result
	SWAP	w4
	IOR	w4, w3, w3
	TBLWTL	w3, [w0]

	ZE	[w2++], w3
fwril:	TBLWTH	w3, [w0++]

	SUB	#1, w5
	BRA	NZ, fwlop

	DISI	#5		; Block all interrupts with priority <7 for next 5 instructions

	MOV	#0x55, w0	; the KEY sequence
	MOV	w0, NVMKEY
	MOV	#0xAA, w0
	MOV	w0, NVMKEY

	BSET	NVMCON, #WR	; GO!

	NOP			; two NOPs (required)
	NOP

; en fait c'est le meme code que erase, sauf 4001 au lieu de 4041
	return
;;;
;;;	fgorun : saut sur adresse abs 16 bits
;;;	void fgorun( unsigned int adr );
_fgorun:
	goto	w0

	.end

