/* tmr0_uart.h
 Traitement interruption timer low priority
 Traitement interruption uart low priority
 - utilise tmr0_uart_int.asm
 */

/* BRU = Bloc de Requete Uart 
	wi == ri signifie fifo vide
	wi et ri sont post-incrementes modulo QBRU
 */

// ATTENTION maintenir ces valeurs coherentes avec uart_int.asm !
#define QBRU 64	// puissance de 2 obligatoire
#define BRU_MSK QBRU-1

// alloue dans tmr0_uart_int.asm
extern char tx_fifo[QBRU];	// fifo circulaire pour emission
extern near unsigned char tx_wi;	// index d'ecriture dans tx_fifo
extern near unsigned char tx_ri;	// index de lecture dans tx_fifo
extern char rx_fifo[QBRU];	// fifo circulaire pour reception
extern near unsigned char rx_wi;	// index d'ecriture dans rx_fifo
extern near unsigned char rx_ri;	// index de lecture dans rx_fifo
// N.B. la variable du timer tmr0_roll est allouee dans fix_var.c
// et donc declaree comme extern dans fix_var.h

/* initialiser */
void tmr0init( void );
void uart_init( void );

// savoir s'il y a quelque chose de nouveau
int uart_hit( void );	// nommee par analogie avec kbhit() !

// lire un byte (-1 s'il n'y en a pas)
int uart_getc( void );	// dans le style de getc()

// emettre un byte
void uart_putc( char c );

// evaluer l'espace disponible dans fifo emission
unsigned char uart_space( void );

// emettre N caracteres
void uart_putn( char * txt, unsigned char qtxt );
