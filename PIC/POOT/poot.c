/* frevo7 / SBC18
	- bootcode nu, n'utilise pas le startup Microchip
 */
#include "..\PASS\flusk.h"

// startup avec saut intermediaire

void _startup(void)	// en lieu et place du startup du C
{
_asm goto flusk _endasm	// retour au bootloader
}

void irtnl3(void)	// stub pour interrupt non prioritaire (dummy)
{}
