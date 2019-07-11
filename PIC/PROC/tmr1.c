/* tmr1.c
 utilisation du timer1 comme compteur 16 bits
 entree RC0
 */

#include <p18cxxx.h>
#include "tmr1.h"
#include "recipe.h"

#pragma code jln_lib3

void tmr1init( void )	/* initialise timer */
{
T1CONbits.RD16 = 1;	/* 16-bit R/W */
T1CONbits.T1CKPS1 = 0;	/* no prescale */
T1CONbits.T1CKPS0 = 0;
T1CONbits.T1OSCEN = 0;	/* no osc */
T1CONbits.T1SYNC = 0;	/* sync mode */
T1CONbits.TMR1CS = 1;	/* pin clk */

TMR1L = 0; TMR1H = 0;
T1CONbits.TMR1ON = 1;	/* enable timer */
}

 
/* tache frequencemetre
   mesure le rapport delta_tacks / delta_ticks
	delta_tacks = compte d'impulsions depuis dernier appel
	delta_ticks = duree depuis dernier appel
	(valeur theorique 2**17 = 0x20000
   resultat 16 bits dans FREpv
 */
void freqmeter( void )
{
static unsigned int old_tacks;		// 16 bits
static unsigned short long old_ticks;	// 24 bits
static unsigned long delta_tacks, delta_ticks;	// 32 bits
static unsigned long freq;

// on decale delta_tacks de 17 bits a gauche
((unsigned int *)&delta_tacks)[0] = 0;
((unsigned int *)&delta_tacks)[1] = ( tacks_cnt - old_tacks ) << 1;
old_tacks = tacks_cnt;

// on etend delta_ticks sur 32 bits
*((unsigned short long *)&delta_ticks) = (unsigned short long)ticks_cnt - old_ticks;
((unsigned char *)&delta_ticks)[3] = 0;
old_ticks = ticks_cnt;

freq = delta_tacks / delta_ticks;
fre.PV = *((unsigned int *)&freq);	// 15 bits nom.
fre.PV <<= 1;
}
