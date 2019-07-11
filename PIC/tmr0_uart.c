/*  Traitement interruption uart low priority
; historique :
;	Frevo7.PROC / PIC18F452 : tmr0 et uart mais specialise pour RS485 (half duplex)
;	JLFS2.FtdU / PIC18F2550 (USB) : pas de trm0 supprime mais buffer circulaire
;	KS/PASS18 / PIC18F2550 (USB) : idem (tencor/tube/PIC18 idem)
;	Frevo8.PASS / PIC18F452 : tmr0 et uart avec buffer circulaire (on remet tmr0)
; differences entre 18F452 et 18F2250
;	baud rate generator : 8-bit vs 16-bit
;	sur 18F2550, JLN a ecrit que SPI est incompatible avec UART (pas clair)

 */

#include <p18cxxx.h>
#include "tmr0_uart.h"

/** I N T E R R U P T  V E C T O R S *****************************************/

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

void uart_init( void )
{
TRISCbits.TRISC6 = 1;	// Tx
TRISCbits.TRISC7 = 1;	// Rx

// baud rate 18F452
SPBRG = 129;	// 20MHz / (16*(129+1)) = 9615.4 baud (bit BRGH=1)

/* baud rate 18F2550 : avec BRGH et BRG16 : baud = Fosc/4(n+1)
SPBRG =  0x70;
SPBRGH = 0x02;
// 0x0270 = 624 -> 9600 bauds juste a 24MHz !
BAUDCON = 0x08;     // BRG16 = 1
*/

// init serial port RX & TX
TXSTA = 0x24;	// TXEN, BRGH
RCSTA = 0x90;	// SPEN, CREN

// buffers
tx_wi = 0; tx_ri = 0;
rx_wi = 0; rx_ri = 0;

// interrupt (note : transmitter interrupt not to be enabled yet)
IPR1bits.TXIP = 0;	/* low priority for our transmitter */
IPR1bits.RCIP = 0;	/* low priority for our receiver */
RCONbits.IPEN = 1;	/* priority system enabled */
INTCONbits.GIEL = 1;	/* enable low priority interrupt */
INTCONbits.GIEH = 1;	/* enable hi priority interrupt */
PIE1bits.RCIE = 1;	/* receiver interrupt */
}


/* savoir s'il y a quelque chose de nouveau */
int uart_hit()		// nommee par analogie avec kbhit() !
{
return( rx_wi != rx_ri );
}
//*/

/* lire un byte (-1 s'il n'y en a pas) */
int uart_getc()	// dans le style de getc()
{
int retval;
if ( rx_wi == rx_ri )
   return(-1);
retval = rx_fifo[rx_ri];
rx_ri++; rx_ri &= BRU_MSK;
return(retval);
}
//*/

/* emettre un byte */
void uart_putc( char c )
{
tx_fifo[tx_wi] = c;
PIE1bits.TXIE = 0;	// securite
tx_wi++; tx_wi &= BRU_MSK;
// flush immediat
PIE1bits.TXIE = 1;	// autoriser interruption
}
//*/

/* evaluer l'espace disponible dans fifo emission */
unsigned char uart_space( void )
{
static unsigned char retval;
retval = ( tx_ri - tx_wi - 1 ) & BRU_MSK;
return( retval );
}
//*/

/* emettre N caracteres */
// ----ooOO ** attention a ne pas faire deborder le fifo ** OOoo----
void uart_putn( char * txt, unsigned char qtxt )
{
while( qtxt-- )
     uart_putc( *(txt++) );
}
//*/
