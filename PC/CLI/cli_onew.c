/* =================== interface test 1-wire DALLAS ============ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../../ipilot.h"
#include "../diali.h"

void gasp( char *fmt, ... );  /* fatal error handling */

static irblock irb;

void onew_usage()
{
   printf("\n=== acces %s === ", dialogue_get_acces_text() );
   printf("Bus 1-wire DALLAS ===\n" );
   printf("Niveau transaction -------\n");
   printf("  QnnXXYYZZ... : emettre une requete Reset-Transmission-Reception\n");
   printf("                   nn = nombre de bytes a recevoir\n");
   printf("                   XX, YY, ZZ donnees a emettre\n");
   printf("  R : voir resultat\n");
   printf("  A : requete adresse (device solitaire)\n");
   printf("  C : lancer conversion DS18B20\n");
   printf("  T : requete temperature DS18B20 solitaire\n");
   printf("  Uaaaaaaaaaaaaaaaa : requete temperature DS18B20 d'adresse A\n");
   printf("Niveau bit ---------------\n");
   printf("  r : reset pulse\n");
   printf("  0 : emettre un bit 0\n");
   printf("  1 : emettre un bit 1\n");
   printf("  b : recevoir 1 bit\n");
   printf("  q : quitter ce menu\n\n");
}

void onew_ui()
{
int qtxbytes;
char text[256];
char locar; int fin; int i, val, onew_rcnt;
int tempflag = 0;

fin = 0; onew_rcnt = 0;
onew_usage();
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'Q' :	i = 0;
		while ( ( text[i++] = (char) getchar() ) >= '0' )
		   if ( i > 254 ) break;
		text[i] = 0;
		qtxbytes = strlen(text) / 2;
		if  ( qtxbytes < 2 )
                    gasp("Sorry - give at least nnXX");
		if  ( qtxbytes > 17 )
                    gasp("Too many data bytes %d", qtxbytes - 1 );
		for ( i = 0; i < qtxbytes; i++ )
		    {
		    sscanf( text + i + i, "%2x", &val );
		    irb.txbuf[i+1] = (unsigned char) val;
		    }
		irb.txbuf[0] = ONEW_Q; onew_rcnt = irb.txbuf[1]; tempflag = 0;
		if   ( onew_rcnt > 16 )
		     gasp("nn too big %d", onew_rcnt );
		irb.txcnt = qtxbytes + 1; irb.rxcnt = 1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
		else printf("faire R pour resultat\n");
		break;
     case 'R' : if ( onew_rcnt )
		   {
		   irb.txbuf[0] = ONEW_R; irb.txbuf[1] = onew_rcnt;
		   irb.txcnt = 2; irb.rxcnt = onew_rcnt + 1;
		   if   ( dial( &irb ) )
		        printf(" echec dialogue\n");
		   else	{
			for ( i = 0; i < onew_rcnt; i++ )
			    printf("%02X ", irb.rxbuf[i+1] );
			if ( tempflag )
			   {
			   unsigned int deg; double T;
			   deg = irb.rxbuf[2] << 8;
			   deg |= irb.rxbuf[1];
			   T = (double)deg; T /= 16.0;
			   printf(" %6.2f degres", T );
			   }
			printf("\n");
			}
		   }
		break;
     case 'A' : irb.txbuf[1] = 8; irb.txbuf[2] = 0x33;	// Read ROM
		qtxbytes = 2; tempflag = 0;
		irb.txbuf[0] = ONEW_Q; onew_rcnt = irb.txbuf[1];
		irb.txcnt = qtxbytes + 1; irb.rxcnt = 1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
		else printf("faire R pour resultat\n");
		break;
     case 'C' : irb.txbuf[1] = 0; irb.txbuf[2] = 0xCC;	// Skip ROM
			          irb.txbuf[3] = 0x44;	// Convert
		qtxbytes = 3;
		irb.txbuf[0] = ONEW_Q; onew_rcnt = irb.txbuf[1];
		irb.txcnt = qtxbytes + 1; irb.rxcnt = 1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
		else printf("faire T pour demander resultat\n");
		break;
     case 'T' : irb.txbuf[1] = 2; irb.txbuf[2] = 0xCC;	// Skip ROM
			          irb.txbuf[3] = 0xBE;	// Read RAM
		qtxbytes = 3; tempflag = 1;
		irb.txbuf[0] = ONEW_Q; onew_rcnt = irb.txbuf[1];
		irb.txcnt = qtxbytes + 1; irb.rxcnt = 1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
		else printf("faire R pour resultat\n");
		break;
     case 'U' :	i = 0;
		while ( ( text[i++] = (char) getchar() ) >= '0' )
		   if ( i > 254 ) break;
		text[i] = 0;
		qtxbytes = strlen(text) / 2;
		if  ( qtxbytes != 8 )
                    gasp("Sorry - 8 bytes of address");
		for ( i = 0; i < qtxbytes; i++ )
		    {
		    sscanf( text + i + i, "%2x", &val );
		    irb.txbuf[i+3] = (unsigned char) val;
		    }
		irb.txbuf[0] = ONEW_Q;
		irb.txbuf[1]  = 2;		// 2 bytes to read
		irb.txbuf[2]  = 0x55;	// Match ROM
		irb.txbuf[11] = 0xBE;	// Read RAM
		onew_rcnt = irb.txbuf[1]; tempflag = 1;
		for ( i = 0; i < 12; i++ )
		    printf("%02X ", irb.txbuf[i] );
		printf("\n");
		irb.txcnt = 12; irb.rxcnt = 1;
		if   ( dial( &irb ) )
		     printf(" echec dialogue\n");
		else printf("faire R pour resultat\n");
		break;
     case 'r' : irb.txbuf[0] = ONEWRST; 
		irb.txcnt = 1; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		break;
     case '0' : irb.txbuf[0] = ONEWBT; irb.txbuf[1] = 0; 
		irb.txcnt = 2; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		break;
     case '1' : irb.txbuf[0] = ONEWBT; irb.txbuf[1] = 1; 
		irb.txcnt = 2; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		break;
     case 'b' : irb.txbuf[0] = ONEWBR;
		irb.txcnt = 1; irb.rxcnt = 2;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		else printf("--> %d\n", irb.txbuf[1] );
		break;
     case 'q' : fin++;				break;
     case ' ' : onew_usage();			break;
     default  :					break;
     }
   }
}
