// allocation centralisee de variables fixes

// variables fixes en zone "access" ("near")

#pragma udata access fix_acc

near unsigned char txcnt;	// set by main
near unsigned char rxcnt;	// set by irq, reset by main
near unsigned char txsum;	// init by main, sum by irq
near unsigned char rxsum;	// idem
near unsigned char istatus;	// low level I2C status

near unsigned char reset_status;// detection WDT reset etc...
near unsigned char tmr0_roll;	// horloge incrementee par interruption timer
near unsigned char save_W;	// pour irtnl


// variables fixes hors zone "access"

#pragma udata fix_var

unsigned int save_fsr0;		// pour irtnh 
unsigned int fadr;		// adresse pour flash routines
unsigned char save_status;	// pour irtnl
