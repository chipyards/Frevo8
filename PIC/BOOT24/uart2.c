/* HY lib 0.1		J.L. Noullet 05/2007 */

/* uart 1 et 2 pilote par interruptions independantes   JL NOULLET 16:12:2006
	- buffer circulaire
	- debordement non gere pour le moment
 */

#if defined(__PIC24F__)
    #include <p24fxxxx.h>
#elif defined(__PIC24H__)
    #include <p24hxxxx.h>
#elif defined(__dsPIC30F__)
    #include <p30Fxxxx.h>
#endif

#include "hard.h"
#include "uart2.h"

// fifos statiques (un BRU = une paire de fifos; un BRU par uart)
static BRU bru2;

/* Routines d'interruption emission
 */

void __attribute__((interrupt, auto_psv))  _U2TXInterrupt(void)
/* This function can reference const variables and
string literals with the constants-in-code memory model. */
{
IFS1bits.U2TXIF = 0;
if ( bru2.tx_wi != bru2.tx_ri )	// quelque chose a emettre ?
   if ( U2STAbits.UTXBF == 0 )	// s'il y a de la place dans l'UART
      {
      U2TXREG = bru2.tx_fifo[bru2.tx_ri & BRU_MSK];
      bru2.tx_ri++;
      }
}	// fin interruption emission 2


/* Routines d'interruption reception
 */
void __attribute__((interrupt, auto_psv)) _U2RXInterrupt(void)
{
IFS1bits.U2RXIF = 0;
while ( U2STAbits.URXDA )	// quelque chose de nouveau a prendre ?
   {
   bru2.rx_fifo[bru2.rx_wi & BRU_MSK] = U2RXREG;
   bru2.rx_wi++;
   }
}	// fin interruption reception 2

// calcul du Baud Rate generator "a chaud"
// BRG = ( Fcy / ( 16 * bauds ) ) -1 + 0.5
// on ajoute + 0.5 pour un arrondi optimal
static unsigned int baud2brg( unsigned int bauds )
{
unsigned long lbr;
lbr = FCY_KHZ * 1000L;
lbr /= (unsigned long)bauds;
lbr -= 8;	// -16 pour -1, +8 pour +0.5
lbr >>= 4;	// /16
return( (unsigned int)lbr );
}


/* --------------- interface application ----------------- */

// configurer format uart 1 ou 2
// 8 bits de data - pas le choix
// stops : ONE_STOP, TWO_STOP
// parity : NOPAR, ODD, EVEN
// flow : NO_CTS_RTS, CTS_RTS (NO_CTS_RTS ==> pins CTS et RTS sont I/O ordinaires)
//

void uart2_init( UART12_STOPS stops, UART12_PARITY parity )
{
// remap pins
UTX_PIN = U2TXSEL;	// programmation RPORx
 _U2RXR = URX_PIN;	// programmation RPINRx 

U2MODE = 0;
U2MODEbits.STSEL = stops;
U2MODEbits.PDSEL = parity;
U2MODEbits.UARTEN = 1;	// enable uart
// UxSTA : UTXISEL = URXISEL = 0 ==> interrupt au plus tot
U2STA = 0;	
U2STAbits.UTXEN = 1;	// enable tx
bru2.tx_wi = 0;
bru2.tx_ri = 0;
bru2.rx_wi = 0;
bru2.rx_ri = 0;
// configurer interruptions
_U2RXIF = 0;
_U2TXIF = 0;
_U2RXIP = 4;	// moyenne priorite
_U2TXIP = 2;	// faible priorite
_U2RXIE = 1;
_U2TXIE = 1;
}

// fixer vitesse avec calcul "a chaud"
void uart2_baud( unsigned int bauds )
{
U2BRG = baud2brg( bauds );
}

// savoir s'il y a quelque chose de nouveau

int uart2_hit()		// nommee par analogie avec kbhit() !
{
return( bru2.rx_wi != bru2.rx_ri );
}

// attente d'un byte avec timeout
// max 150 ms @ 10 MIps
int uart2_block( unsigned int timeout )
{
while( --timeout )
   {
   if ( uart2_hit() )
      return 1;
   }
return 0;
}

// lire un byte (-1 s'il n'y en a pas)

int uart2_getc()	// dans le style de getc()
{
int retval;
if ( bru2.rx_wi == bru2.rx_ri )
   return(-1);
retval = bru2.rx_fifo[bru2.rx_ri & BRU_MSK];
bru2.rx_ri++;
return(retval);
}

// emettre un byte

void uart2_putc( char c )
{
bru2.tx_fifo[bru2.tx_wi & BRU_MSK] = c;
bru2.tx_wi++;
// flush immediat
IEC1bits.U2TXIE = 0;		// masquer interrupt
if ( bru2.tx_wi != bru2.tx_ri )	// quelque chose a emettre ?
   if ( U2STAbits.UTXBF == 0 )	// s'il y a de la place dans l'UART
      {
      U2TXREG = bru2.tx_fifo[bru2.tx_ri & BRU_MSK];
      bru2.tx_ri++;
      }
IEC1bits.U2TXIE = 1;
}

// evaluer l'espace disponible dans fifo emission

unsigned int uart2_space( void )
{
static unsigned int retval;
retval = ( bru2.tx_ri - bru2.tx_wi - 1 ) & BRU_MSK;
return( retval );
}

// emettre une chaine
// ----ooOO ** attention a ne pas faire deborder le fifo ** OOoo----

void uart2_puts( char * txt )
{
char c;
while( ( c = *(txt++) ) )
     uart2_putc( c );
}

// emettre N caracteres
// ----ooOO ** attention a ne pas faire deborder le fifo ** OOoo----

void uart2_putn( char * txt, int qtxt )
{
while( qtxt-- )
     uart2_putc( *(txt++) );
}
