; frevo7 / SBC18
; vecteurs non proteges

; export
	global	pulo	; doit etre en finpr2
	global	irtnl2	; doit etre en finpr2+64+4
; import
	extern	_startup
	extern	irtnl3
	extern	flusk

; premiere page, qui sert a la commutation securisee
; contient le goto vers le startup du C suivi de NOPs
finp2_scn	CODE

pulo	goto	_startup	; 4 bytes
	fill	0xFFFF, 0x3C	; pour completer a 64 bytes

; seconde page, commence par le vecteur de securite
	goto	flusk	; retour au bootloader si page precedente effacee
irtnl2	goto	irtnl3

	END
