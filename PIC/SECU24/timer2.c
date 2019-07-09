// timer 2 en mode compteur - frequencemetre

#include <p24fxxxx.h>

#include "hard.h"
#include "timer2.h"

void timer2_init( void )
{
T2CON = 0;
_T2IE = 0;		// no interrupt
PR2 = 0xFFFF;
TMR2 = 0;
// T2CONbits.TCKPS = 0;	// prescale 1:1 pour memoire
T2CONbits.TCS = 1;	// ext. clock
T2CONbits.TON = 1;
}

// demarrer frequencemetre sur LM331
void freq_vfin_init( void )
{
_T2CKR = VFI_PIN;
VFI_DNA = 1;
timer2_init();
}

// demarrer frequencemetre sur entree opto
void freq_optin_init( void )
{
_T2CKR = OPT_PIN;
OPT_DNA = 1;
timer2_init();
}

/* tache frequencemetre
   mesure le rapport delta_tacks / delta_ticks
	delta_tacks = compte d'impulsions depuis dernier appel
	delta_ticks = duree depuis dernier appel
   resultat 16 bits dans frePV
 */
void freqmeter( void )
{
static unsigned int old_tacks;		// 16 bits
static unsigned long old_ticks;		// 32 bits
unsigned int dt16;
unsigned long delta_tacks, delta_ticks;	// 32 bits
unsigned long freq;

dt16 = tacks_cnt - old_tacks;
old_tacks = tacks_cnt;

// plafonnage sur 14 bits (16383)
if ( dt16 > 0x3FFF )
   dt16 = 0x3FFF;

// on decale delta_tacks de 18 bits a gauche

((unsigned int *)&delta_tacks)[0] = 0;
((unsigned int *)&delta_tacks)[1] = dt16 << 2;

// on calcule delta_ticks sur 32 bits
// valeur optimale 2**17 
delta_ticks = ticks_cnt - old_ticks;
old_ticks = ticks_cnt;

freq = delta_tacks / delta_ticks;
frePV = *((unsigned int *)&freq);	// 15 bits nom.
frePV <<= 1;
}
