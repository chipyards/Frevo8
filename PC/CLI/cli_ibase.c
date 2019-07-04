/* =================== interface debug IPILOT de base ============ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../../ipilot.h"
#include "../diali.h"

void gasp( char *fmt, ... );  /* fatal error handling */

void ibase_usage()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  V : afficher version firmware Frevo 7+\n");
   printf("  T : afficher system time Frevo\n");
   printf("  q : quitter ce menu\n");
   printf("Services optionnels :\n");
   printf("  iAAaaNN : lire NN bytes en RAM PIC a l'adresse hexa AAaa\n");
   printf("  oAAaaXXYYZZ : ecrire bytes X Y Z en RAM PIC a l'adresse AAaa\n\n");
}

static irblock irb;

void ibase_ui()
{
int qtxbytes;
char text[256];
char locar; int fin; int i, j, val;
char betaver, role; unsigned char reset_flags;

fin = 0; ibase_usage();
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     /* il y a 3 formats de version :
		Frevo <= 7.8	VER	SUBVER	ROLE 	RST_flags
		Frevo 8		VER	SUBVER	ROLE	RST_flags BETA	
		Tencor/KS	VER	SUBVER	BETA	ROLE
	sbc24s utilise le format Tencor/KS
      */

     case 'V' : irb.txbuf[0] = SYSVER;
		irb.txcnt = 1; irb.rxcnt = 6;
		if ( dialogue_get_acces() == 'm' )	// sbc24s : format Tencor/KS
		   irb.rxcnt = 5;
		if   ( dial( &irb ) )
		     {
		     irb.txbuf[0] = SYSVER;
		     irb.txcnt = 1; irb.rxcnt = 5;	// retro-compatibilite <= 7.8
		     if   ( dial( &irb ) )
			  { printf(" echec dialogue\n"); break;  }
		     }
		printf(" Ok Frevo %d.%d", irb.rxbuf[1], irb.rxbuf[2] );
		if   ( dialogue_get_acces() == 'm' )
		     {
		     role = irb.rxbuf[4];
		     betaver = irb.rxbuf[3];
		     reset_flags = 0;
		     }
		else {
		     role = irb.rxbuf[3];
		     betaver = (irb.rxcnt==6)?(irb.rxbuf[5]):(' ');
		     reset_flags = irb.rxbuf[4];
		     }
		if ( role < ' ' ) role = '_';
		if ( betaver < ' ' ) betaver = '_';
		printf("%c, usage %c, reset flags %02X", betaver, role, reset_flags );
		if ( dialogue_get_acces() != 'm' )	// not for PIC24
		   {
		   if (   reset_flags & 0x80        ) printf(" Stack Overflow");
		   if (   reset_flags & 0x40        ) printf(" Stack Underflow");
		   if ( ( reset_flags & 0x10 ) == 0 ) printf(" Reset Instruction");
		   if ( ( reset_flags & 0x08 ) == 0 ) printf(" Watchdog Reset");
		   if ( ( reset_flags & 0x04 ) == 0 ) printf(" Wake up");
		   // if ( ( reset_flags & 0x02 ) == 0 ) printf(" POR Reset");
		   // if ( ( reset_flags & 0x01 ) == 0 ) printf(" BOR reset");
		   }
		printf("\n");
						break;
     case 'T' : irb.txbuf[0] = SYSTIM;
		irb.txcnt = 1; irb.rxcnt = 5;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		else {
		     unsigned int h, m, s;
		     s = *( (unsigned int *)(irb.rxbuf+1) );
		     m = s / 60; s = s % 60;
		     h = m / 60; m = m % 60;
		     printf(" Ok %d:%02d:%02d\n", h, m, s );
		     }				break;
     case 'i' :	i = 0;
		while ( ( text[i++] = (char) getchar() ) >= '0' )
		   if ( i > 254 ) break;
		text[i] = 0;
		qtxbytes = strlen(text) / 2; // printf("tx %d bytes : _%s_", qtxbytes, text );
		if  ( qtxbytes != 3 )
                    gasp("Sorry - give aaaaNN");
		for ( i = 0; i < qtxbytes; i++ )
		    {
		    sscanf( text + i + i, "%2x", &val );
		    irb.txbuf[i+2] = (unsigned char) val;
		    }
		j = irb.txbuf[2]; irb.txbuf[2] = irb.txbuf[3]; irb.txbuf[3] = j; // swap adr bytes...
		j = irb.txbuf[4];
		if  ( j > 30 )
		    gasp("NN too big %d", j ); 
		irb.txbuf[0] = SYSDBG; irb.txbuf[1] = 'i';
		irb.txcnt = 5; irb.rxcnt = j+1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
		for ( i = 0; i < j; i++ )
		    printf("%02X ", irb.rxbuf[i+1] );
		printf("\n");
						break;
     case 'o' :	i = 0;
		while ( ( text[i++] = (char) getchar() ) >= '0' )
		   if ( i > 254 ) break;
		text[i] = 0;
		qtxbytes = strlen(text) / 2; // printf("tx %d bytes : _%s_", qtxbytes, text );
		if  ( qtxbytes < 3 )
                    gasp("Sorry - give at least aaaaXX");
		if  ( qtxbytes > 29 )
                    gasp("Too many data bytes %d", qtxbytes - 2 );
		for ( i = 0; i < qtxbytes; i++ )
		    {
		    sscanf( text + i + i, "%2x", &val );
		    irb.txbuf[i+2] = (unsigned char) val;
		    }
		j = irb.txbuf[2]; irb.txbuf[2] = irb.txbuf[3]; irb.txbuf[3] = j; // swap adr bytes...
		irb.txbuf[0] = SYSDBG; irb.txbuf[1] = 'o';
		irb.txcnt = qtxbytes + 2; irb.rxcnt = 1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
						break;
     case 'q' : fin++;				break;
     case ' ' : ibase_usage();			break;
     default  :					break;
     }
   }
}
