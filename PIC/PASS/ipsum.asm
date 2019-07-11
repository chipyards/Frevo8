; IPsum
	PROCESSOR 18f452
	include "p18f452.inc"
        radix           dec             ; base de numeration par defaut
;
; declaration de symboles exportables
fix_acc		UDATA_ACS	; access RAM;
curIPsum	RES	2
	global	curIPsum

; void IPsumA( unsigned char * sbuf, unsigned char len );
	global	IPsum
	

jln_lib2	CODE
					; /* fonction pour calculer checksum a la mode IP (ip, icmp, udp, tcp)
					; - longueur limitee a 255 bytes
					; - resultat accumule dans curIPsum
					; - initialier curIPsum a zero, le complementer a la fin
					; */
IPsum					; void IPsum( unsigned char * sbuf, unsigned char len )
 					
	MOVLW	-2		; recuperer pointeur sbuf dans FSR0	
 	MOVFF	PLUSW1,FSR0L
 	MOVLW	-1
 	MOVFF	PLUSW1,FSR0H

	MOVLW	-3		
 	MOVFF	PLUSW1,PRODH		; recuperer len dans PRODH

	CLRF	PRODL,ACCESS		; PRODL pour sauver carry

wlop					; while ( len & 0xFE )	// tant qu'il reste plus que 1
 	MOVF	PRODH,W,ACCESS
 	ANDLW	0xfe
 	BZ	flop

	MOVFF	PRODL,STATUS		; {
	MOVF	POSTINC0,W,ACCESS	; C = add8( ((unsigned char *)&curIPsum), *(sbuf++), C );
 	ADDWFC	curIPsum,F,ACCESS
	MOVF	POSTINC0,W,ACCESS	; C = add8( ((unsigned char *)&curIPsum3)+1, *(sbuf++), C );
 	ADDWFC	curIPsum+1,F,ACCESS
	MOVFF	STATUS,PRODL

 	DECF	PRODH,F,ACCESS		; len--;
 	DECF	PRODH,F,ACCESS

	BRA	wlop			; }

flop	MOVFF	PRODL,STATUS		; ajouter evenuellement dernier byte
	MOVF	PRODH,F,ACCESS		; if (odd) {
	BZ	finsum

	MOVF	INDF0,W,ACCESS		; C = add8( ((unsigned char *)&curIPsum3), *sbuf, C );;
 	ADDWFC	curIPsum,F,ACCESS
	MOVLW	0			; C = add8( ((unsigned char *)&curIPsum3)+1, 0, C );
 	ADDWFC	curIPsum+1,F,ACCESS	; }

finsum	MOVLW	0			; purger carry
 	ADDWFC	curIPsum,F,ACCESS	; C = add8( ((unsigned char *)&curIPsum3), 0, C );
 	ADDWFC	curIPsum+1,F,ACCESS	; C = add8( ((unsigned char *)&curIPsum3)+1, 0, C );

  	RETURN	
;
	end
