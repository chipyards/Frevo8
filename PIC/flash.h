// Frevo7
// lecture/ecriture memoire flash programme PIC18
// adresses 16 bits

// bas niveau : flash.asm
void fread8( unsigned int adr, unsigned char * buf );
void fwrite8( unsigned int adr, unsigned char * buf );
void fera64( unsigned int adr );

#define MINFADR 0x300	// adresse fin de zone protegee, doit etre
			// coherente avec finpr_pg dans le linker script

// niveau intermediaire : flash.c
// interpreteur de commandes selon ipilot.h, rend status dans txbuf[0]
// contient un mini-startup
// fonction sans retour
void flask( void );
