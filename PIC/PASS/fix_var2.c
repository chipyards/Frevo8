// allocation centralisee de variables fixes pour ethernet/asix

// variables fixes en zone "access" ("near")

#pragma udata access fix_acc

// variables fixes hors zone "access"

#pragma udata fix_var

unsigned char myIP[4];		// pour ethernet
unsigned char myMAC5;
unsigned char next_pack;	// "next packet" page from RX NE2000 header
unsigned char link_status;	// see bits in asix.h
unsigned char err_status;
unsigned char txflag;
