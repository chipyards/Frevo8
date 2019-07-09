// timer 4 et 5 en mode 32 bits free-running pour base de temps generique

#include <p24fxxxx.h>

#include "hard.h"
#include "timer45.h"

void timer45_init( void )
{
T4CON = 0;
T5CON = 0;
_T5IE = 0;		// no interrupt
T4CONbits.T32 = 1;	// 32-bit Timer operation
T4CONbits.TCKPS = 2;	// prescale 64 : periode 6.4us @ 20MHz
PR4 = 0xFFFF;
PR5 = 0xFFFF;
TMR5HLD = 0;
TMR4 = 0;
T4CONbits.TON = 1;	// Start 32-bit timer with internal clock
}
