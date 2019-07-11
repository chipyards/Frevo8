// Frevo7
// lecture/ecriture memoire flash programme PIC18
// adresses 16 bits

#define MINFADR2 0x0EC0	// adresse fin de zone protegee, doit etre
			// coherente avec finp2_pg dans le linker script

// niveau intermediaire : flusk.c
// interpreteur de commandes I2C et UDP
//	I2C : passe la main a flask()
//	UDP : execute les commandes ipilot FLASHR, FLASHW, FERASE, SYSVER
// contient un mini-startup
// fonction sans retour
void flusk( void );
// point d'entree a utiliser si ethernet chip deja initialise
// egalement sans retour
void fluskloop(void);
