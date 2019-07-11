/* i2csla2.h
 Traitement interruption MSSP  (I2C slave), prioritaire
 - ATTENTION taille buffers determinee par
	MASK dans i2csla_int.asm
 - utilise i2csla2_int.asm

I2C handshake :
   RX :
	- main initialise checksum
	- irq met rxcnt a FF pendant reception
	- irq met rxcnt a jour quand reception finie (rxcnt checksum inclus)
	  (mais le maitre peut ecraser ces donnees inopportunement)
	- main met rxcnt a zero apres lecture data et checksum
   TX : 
	- main met XOFF dans txbuf[0] tant que traitement en cours
	  signifie 2 choses :
		- aucune requete write ne doit etre envoyee
		- le contenu pour la requete read n'est pas encore pret
	- main met
		- echo opcode ou errcode dans txbuf[0],
		- txcnt a jour (checksum non inclus)
		- init checksum
	- master peut lire les data une fois (les suivantes, checksum sera invalide)
*/

#define MYADDR 0b10110000;	/* justifee a gauche !! */

// #define QI2C 32

/* initialise port et interrupt I2C (initialisation de variables non incluse)
	SSPSTAT : slew rate ctrl on (0)
	SSPCON1 : enable synchronous serial port, CKP, slave i2c with 7 bit addr. + START/STOP irq
	IPR1bits.SSPIP : high priority for our I2C !
	PIE1bits.SSPIE : enable MSSP interrupt 
	RCONbits.IPEN : priority system enabled 
	INTCONbits.GIEH : enable global hi interrupt
 */
#define i2csla_init() TRISCbits.TRISC3=1; TRISCbits.TRISC4=1;\
		      SSPSTAT=0; SSPCON1=0b111110; SSPCON2=0;\
		      IPR1bits.SSPIP=1; PIE1bits.SSPIE=1; RCONbits.IPEN=1; INTCONbits.GIEH=1;\
		      SSPADD=MYADDR;

/* desactive port et interrupt I2C
	PIE1bits.SSPIE = 0;	// disable MSSP interrupt
	SSPCON1bits.SSPEN = 0;	// disable synchronous serial port
 */

#define i2csla_stop() PIE1bits.SSPIE=0; SSPCON1bits.SSPEN=0;

