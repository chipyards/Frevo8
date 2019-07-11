/* frevo7 / SBC18
	- bootcode nu, n'utilise pas le startup Microchip
 */

// startup avec saut intermediaire

void _startup(void)	// en lieu et place du startup du C
{
_asm goto 0x0010 _endasm	// retour au bootloader
}

void irtnl2(void)	// stub pour interrupt non prioritaire (inactive)
{}
