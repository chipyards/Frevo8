// allocation centralisee de variables fixes pour ethernet/asix

// variables fixes en zone "access" ("near")

extern near unsigned int curIPsum;	// alloue dans ipsum.asm

// variables fixes hors zone "access"

extern unsigned char myIP[4];		// pour ethernet
extern unsigned char myMAC5;
extern unsigned char next_pack;		// "next packet" page from RX NE2000 header
extern unsigned char link_status;	// see bits in asix.h
extern unsigned char err_status;
extern unsigned char txflag;

// variables fixes allouees dans ethernet.c
extern unsigned char packet[];
