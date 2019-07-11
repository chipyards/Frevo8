/* tmr0_rs485.c
 Traitement interruption timer low priority
 Traitement interruption RS485 low priority
 */

#include <p18cxxx.h>
#include "tmr0_rs485.h"

#pragma code jln_lib3

void tmr0init( void )	/* initialise interrupt timer */
{
T0CONbits.T08BIT = 0;	/* mode 16 bits */
T0CONbits.T0CS = 0;	/* internal clk */
T0CONbits.PSA = 0;	/* use prescale */
			/* prescale by 64 */
T0CONbits.T0PS0 = 1;
T0CONbits.T0PS1 = 0;
T0CONbits.T0PS2 = 1;

T0CONbits.TMR0ON = 1;	/* enable timer */
INTCON2bits.TMR0IP = 0;	/* low priority for our timer ! */

TMR0L = 0; TMR0H = 0;
RCONbits.IPEN = 1;	/* priority system enabled */
INTCONbits.TMR0IE = 1;	/* enable timer 0 interrupt */
INTCONbits.GIEL = 1;	/* enable low priority interrupt */
}

void uartinit( void )
{
LATCbits.LATC5 = 0;	// Tx driver enable
TRISCbits.TRISC5 = 0;
TRISCbits.TRISC6 = 1;	// Tx
TRISCbits.TRISC7 = 1;	// Rx

// baud rate
SPBRG = 129;	// 20MHz / (16*(129+1)) = 9615.4 baud (BRGH=1)

// init serial port (note : receiver will be enabled by transmit interrupt)
RCSTA = 0x80;	// SPEN

// init transmitter
TXSTA = 0x24;	// TXEN, BRGH
		
// interrupt (note : transmitter interrupt not to be enabled yet)
IPR1bits.TXIP = 0;	/* low priority for our transmitter */
IPR1bits.RCIP = 0;	/* low priority for our receiver */
RCONbits.IPEN = 1;	/* priority system enabled */
INTCONbits.GIEL = 1;	/* enable low priority interrupt */
PIE1bits.RCIE = 1;	/* receiver interrupt */
}

// lancer transmission de N bytes (presents dans uabuf)
void uatxstart( unsigned char N )
{
RCSTAbits.CREN = 0;	// inhiber reception pour eviter echo
urcnt = 0;		// preparer reception qui sera redemarree automatiquement
uindex = 0;		// pointer sur 1er byte
utcnt = N;
LATCbits.LATC5 = 1;	// TX line driver enable
PIE1bits.TXIE = 1;	// autoriser interruption
}

/* --------------------- fonctions OMRON ------------------ */

 
static unsigned char bcc, bval, oindex;

/* finition et envoi d'une requete pour regu OMRON
   le message doit deja etre en uabuf+1, null terminated
   cette fonction ajoute :
	- STX
	- ETX
	- BCC
	- bit stop en position MSB
   adr = 0 a 9
 */
void send485( void )
{
uabuf[0] = 0x82;	// STX
oindex = 1;  		// sauter STX
bcc = 0x03;		// compter ETX (d'avance)
while( oindex < 30 )
   {
   bval = uabuf[oindex];
   if ( bval == 0 ) break;
   bcc ^= bval;
   uabuf[oindex++] = bval | 0x80;	// MSB = stop bit (7 data bits mode)
   }
uabuf[oindex++] = 0x83;	// ETX
uabuf[oindex++] = bcc | 0x80;
uatxstart( oindex+1 );	// +1 a cause du retard bytes vs driver enable
}

/* recuperation d'une reponse de regu OMRON (si disponible)
   rend la longueur utile ou zero
   le message sera en uabuf+1, stop bits supprimes
 */
unsigned char rcv485( void )
{
if ( PIE1bits.TXIE )	// emission en cours
   return(0);
if ( uabuf[0] != 0x82 )	// STX
   return(0);
oindex = 1;  		// sauter STX
bcc = 0;
while( oindex < 31 )
   {
   bval = uabuf[oindex] & 0x7F;	// strip the stop bit
   bcc ^= bval;
   uabuf[oindex++] = bval;
   if ( bval == 0x03 ) break;	// ETX
   }
bval = uabuf[oindex] & 0x7F;	// BCC
if ( bval != bcc )
   return(0);
return(oindex-2);
}
