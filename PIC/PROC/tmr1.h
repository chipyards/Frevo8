/* tmr1.c
 utilisation du timer1 comme compteur 16 bits
 entree RC0
 */

extern near short long ticks_cnt;	// temps en ticks de 12.8 us, sur 24 bits
extern near unsigned int tacks_cnt;	// comptage pulses RC0

void tmr1init( void );	/* initialise timer */

void freqmeter( void );	/* tache frequencemetre */
