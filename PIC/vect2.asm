; frevo7 / SBC18
; vecteurs non proteges

; export
	global	salto	; doit etre en finpr
	global	irtnl	; doit etre en finpr + 64
; import
	extern	_startup
	extern	irtnl2

; premiere page, qui sert a la commutation securisee
; contient le goto vers le startup du C suivi de NOPs
finpr_scn	CODE

salto	goto	_startup	; 4 bytes
	fill	0xFFFF, 0x3C	; pour completer a 64 bytes

; seconde page, commence par le vecteur de securite
	goto	0x0010	; retour au bootloader si page precedente effacee
irtnl	goto	irtnl2


	END
