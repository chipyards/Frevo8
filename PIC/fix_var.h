// declaration variables fixes, allouees dans fix_var.c
// sauf mention contraire

// variables fixes en zone "access" ("near")

extern near unsigned char txcnt;	// set by main
extern near unsigned char rxcnt;	// set by irq, reset by main
extern near unsigned char txsum;	// init by main, sum by irq
extern near unsigned char rxsum;	// idem
extern near unsigned char istatus;	// low level I2C status

extern near unsigned char reset_status;	// detection WDT reset etc...
extern near unsigned char tmr0_roll;	// horloge incrementee par interruption timer
extern near unsigned char save_W;	// pour irtnl

// variables fixes hors zone "access"

// I2C
extern unsigned char txbuf[];	// alloue dans i2csla3_int.asm
extern unsigned char rxbuf[];	// alloue dans i2csla3_int.asm


extern unsigned int save_fsr0;		// pour irtnh 
extern unsigned int fadr;		// adresse pour flash routines
extern unsigned char save_status;	// pour irtnl
