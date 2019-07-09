/* HY lib 0.1		J.L. Noullet 05/2007 */

/* uart 1 et 2 pilotes par interruptions independantes   JL NOULLET 11:12:2006
	- buffer circulaire
	- debordement non gere pour le moment
 */

/* BRU = Bloc de Requete Uart 
	wi == ri signifie fifo vide
	wi et ri sont post-incrementes
 */
#define QBRU 32	// puissance de 2 obligatoire
#define BRU_MSK (QBRU-1)

typedef struct {
unsigned char tx_fifo[QBRU];	// fifo circulaire pour emission
unsigned int tx_wi;	// index d'ecriture dans tx_fifo
unsigned int tx_ri;	// index de lecture dans tx_fifo
unsigned char rx_fifo[QBRU];	// fifo circulaire pour reception
unsigned int rx_wi;	// index d'ecriture dans rx_fifo
unsigned int rx_ri;	// index de lecture dans rx_fifo
} BRU;

typedef enum {
ONE_STOP = 0,
TWO_STOP
} UART12_STOPS;

typedef enum {
NOPAR = 0,
EVEN,
ODD
} UART12_PARITY;


// configurer format uart 1 ou 2
// 8 bits de data - pas le choix
// stops : ONE_STOP, TWO_STOP
// parity : NOPAR, ODD, EVEN
//
void uart1_init( UART12_STOPS stops, UART12_PARITY parity );
void uart2_init( UART12_STOPS stops, UART12_PARITY parity );

// fixer vitesse avec calcul "a chaud"
void uart1_baud( unsigned int bauds );
void uart2_baud( unsigned int bauds );

// savoir s'il y a quelque chose de nouveau
int uart1_hit();	// nommee par analogie avec kbhit() !
int uart2_hit();

// attente d'un byte avec timeout
// max 150 ms @ 10 MIps
int uart2_block( unsigned int timeout );

// lire un byte (-1 s'il n'y en a pas)
int uart1_getc();	// dans le style de getc()
int uart2_getc();

// emettre un byte
void uart1_putc( char c );
void uart2_putc( char c );

// evaluer l'espace disponible dans fifo emission
unsigned int uart1_space( void );
unsigned int uart2_space( void );

// emettre une chaine
void uart1_puts( char * txt );
void uart2_puts( char * txt );

// emettre N caracteres
void uart1_putn( char * txt, int qtxt );
void uart2_putn( char * txt, int qtxt );
