/* =================== interface test RS485 vs OMRON ============ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../ipilot.h"
#include "diali.h"

void gasp( char *fmt, ... );  /* fatal error handling */

static int OMRaddr = 1;
static irblock irb;

void rs485_usage()
{
   printf("\n=== acces %s === ", dialogue_get_acces_text() );
   printf("Adr regu OMRON = %d ===\n", OMRaddr );
   printf("  T : activer mode transparent RS485 (ROOT)\n");
   printf("  axyz : transmettre donnees ascii xyz pour regu OMRON\n");
   printf("  r    : voir donnees ascii rendues par regu OMRON\n");
   printf("  0, 1, 2, 3 : fixer adresse regu OMRON\n");
   printf("  sttt : transmettre SV decimal ttt pour regu OMRON\n");
   printf("  S    : demander SV decimal au regu OMRON\n");
   printf("  P    : demander PV decimal au regu OMRON\n");
   printf("  A    : demander attributs au regu OMRON\n");
   printf("  X    : demander status au regu OMRON\n");
   printf("  W	  : passer write mode en \"RAM\"\n");
   printf("  q : quitter ce menu\n\n");
}

void rs485_ui()
{
int qtxbytes;
char text[256];
char locar; int fin; int i, val;

fin = 0;
rs485_usage();
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'T' : irb.txbuf[0] = PMANU; irb.txbuf[1] = PAUSE; 
		irb.txcnt = 2; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		else {
		     irb.txbuf[0] = PMANU; irb.txbuf[1] = PAUSE | MANU | ROOT;
		     irb.txcnt = 2; irb.rxcnt = 1;
		     if   ( dial( &irb ) )
			  printf(" echec dialogue\n");
		     else printf("mode transparent RS485 Ok\n");
		     } break;
     case '0' :
     case '1' :
     case '2' :
     case '3' : OMRaddr = locar - '0'; rs485_usage(); break;

     case 'a' : i = 0;
		while ( i < QIPILOT )
		   {
		   locar = (char) getchar();
		   if ( locar < ' ' ) break;
		   text[i++] = locar;
		   }
		text[i] = 0;
		qtxbytes = strlen(text);
		if ( qtxbytes > 29 )
		   gasp("message RS485 limite a 29 bytes");

		irb.txbuf[0] = RS485T;
		qtxbytes += 1; // opcode
		for ( i = 1; i < qtxbytes; i++ )
		    irb.txbuf[i] = (unsigned char)text[i-1];

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		break;

     case 'r' : irb.txbuf[0] = RS485R;
		irb.txcnt = 1; irb.rxcnt = 31;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		else {
		     printf(" ");
		     qtxbytes = irb.rxbuf[1];
		     if ( qtxbytes > 29 )
		        gasp("longueur message RS485 %d", qtxbytes );
		     for ( i = 2; i < qtxbytes+2; i++ )
			 if   ( irb.rxbuf[i] < ' ' )
			      printf("_");
			 else printf("%c", irb.rxbuf[i] );
		     printf("\n");
		     }				break;
     case 's' : i = 0;
		while ( i < QIPILOT )
		   {
		   locar = (char) getchar();
		   if ( locar < ' ' ) break;
		   text[i++] = locar;
		   }
		text[i] = 0;
		sscanf( text, "%d", &val );
		printf(" %d degres\n", val );
		irb.txbuf[0] = RS485T;
		//                   5    4   2 4   2 4   8       total 29 
		//		     adr--    ty    bi    val-----
		//		          cmd-  var-  num-
		sprintf( irb.txbuf+1, "0%1d0000102C100030000010000%04X",   OMRaddr, val );
		printf(          " 0%1d0000102C100030000010000%04X\n", OMRaddr,  val );
		qtxbytes = strlen( irb.txbuf );
		if ( qtxbytes != 30 )
		   gasp("erreur codage temperature %d bytes", qtxbytes );

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		break;

     case 'S' : irb.txbuf[0] = RS485T;
		//                   5    4   2 4   2 4    total 21 
		//		     adr--    ty    bi    
		//		          cmd-  var-  num-
		sprintf( irb.txbuf+1, "0%1d0000101C10003000001",   OMRaddr );
		printf(          " 0%1d0000101C10003000001\n", OMRaddr );
		qtxbytes = strlen( irb.txbuf );

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");

		/* solution audacieuse sans tempo
		   ne marche que si la longueur de la reponse est differente de celle
		   de la requete !!! */
		{
		int trips = 0;
		do {
		   irb.txbuf[0] = RS485R;
		   irb.txcnt = 1; irb.rxcnt = 31;
		   if   ( dial( &irb ) )
                        { printf(" echec dialogue\n"); irb.rxbuf[1] = 0; break;  }
		   qtxbytes = irb.rxbuf[1]; printf("_%d_", qtxbytes );
		   if ( qtxbytes == 21 ) qtxbytes = 0; 	// ce n'est que notre requete
		   } while( ( qtxbytes == 0 ) && ( ++trips < 20 ) );
		printf("\n");
		}
		//*/

		/* solution avec tempo...
		sleep(1);
		irb.txbuf[0] = RS485R;
		irb.txcnt = 1; irb.rxcnt = 31;
		if   ( dial( &irb ) )
                     { printf(" echec dialogue\n"); irb.rxbuf[1] = 0; break;  }
		qtxbytes = irb.rxbuf[1];
		//*/

		if ( qtxbytes > 29 )
		   gasp("longueur message RS485 %d", qtxbytes );
		irb.rxbuf[qtxbytes+2] = 0;	// ASCIIZ terminator
		printf(" %s\n", irb.rxbuf+2 );
		// reponse attendue 22 bytes
		//  adr-  cmd-    val--hex
		//      ok    ok--
		//  0100000101000000000xxx
		if   ( qtxbytes == 22 )
		     { sscanf( irb.rxbuf+16, "%X", &val ); printf(" %d degres\n", val ); }
		else printf("reponse %d bytes !?\n", qtxbytes );
		break;

     case 'P' : irb.txbuf[0] = RS485T;
		//                   5    4   2 4   2 4    total 21 
		//		     adr--    ty    bi    
		//		          cmd-  var-  num-
		sprintf( irb.txbuf+1, "0%1d0000101C00000000001",   OMRaddr );
		printf(          " 0%1d0000101C00000000001\n", OMRaddr );
		qtxbytes = strlen( irb.txbuf );

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");

		/* solution audacieuse sans tempo... cf plus haut */
		{
		int trips = 0;
		do {
		   irb.txbuf[0] = RS485R;
		   irb.txcnt = 1; irb.rxcnt = 31;
		   if   ( dial( &irb ) )
                        { printf(" echec dialogue\n"); irb.rxbuf[1] = 0; break;  }
		   qtxbytes = irb.rxbuf[1]; printf("_%d_", qtxbytes );
		   if ( qtxbytes == 21 ) qtxbytes = 0; 	// ce n'est que notre requete
		   } while( ( qtxbytes == 0 ) && ( ++trips < 20 ) );
		printf("\n");
		}

		if ( qtxbytes > 29 )
		   gasp("longueur message RS485 %d", qtxbytes );
		irb.rxbuf[qtxbytes+2] = 0;	// ASCIIZ terminator
		printf(" %s\n", irb.rxbuf+2 );
		if   ( qtxbytes == 22 )
		     { sscanf( irb.rxbuf+16, "%X", &val ); printf(" %d degres\n", val ); }
		else printf("reponse %d bytes !?\n", qtxbytes );
		break;

     case 'A' : irb.txbuf[0] = RS485T;
		//		     5    4   total 9 bytes 
		//		     adr--        
		//		          cmd-
		sprintf( irb.txbuf+1, "0%1d0000503",   OMRaddr );
		printf(          " 0%1d0000503\n", OMRaddr );
		qtxbytes = strlen( irb.txbuf );

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");

		/* solution audacieuse sans tempo... cf plus haut */
		{
		int trips = 0;
		do {
		   irb.txbuf[0] = RS485R;
		   irb.txcnt = 1; irb.rxcnt = 31;
		   if   ( dial( &irb ) )
                        { printf(" echec dialogue\n"); irb.rxbuf[1] = 0; break;  }
		   qtxbytes = irb.rxbuf[1]; printf("_%d_", qtxbytes );
		   if ( qtxbytes == 9 ) qtxbytes = 0; 	// ce n'est que notre requete
		   } while( ( qtxbytes == 0 ) && ( ++trips < 20 ) );
		printf("\n");
		}

		if ( qtxbytes > 29 )
		   gasp("longueur message RS485 %d", qtxbytes );
		irb.rxbuf[qtxbytes+2] = 0;	// ASCIIZ terminator
		printf(" %s\n", irb.rxbuf+2 );
		break;

     case 'X' : irb.txbuf[0] = RS485T;
		//                   5    4   2 4   2 4    total 21 
		//		     adr--    ty    bi    
		//		          cmd-  var-  num-
		sprintf( irb.txbuf+1, "0%1d0000101C00001000001",   OMRaddr );
		printf(          " 0%1d0000101C00001000001\n", OMRaddr );
		qtxbytes = strlen( irb.txbuf );

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");

		/* solution audacieuse sans tempo... cf plus haut */
		{
		int trips = 0;
		do {
		   irb.txbuf[0] = RS485R;
		   irb.txcnt = 1; irb.rxcnt = 31;
		   if   ( dial( &irb ) )
                        { printf(" echec dialogue\n"); irb.rxbuf[1] = 0; break;  }
		   qtxbytes = irb.rxbuf[1]; printf("_%d_", qtxbytes );
		   if ( qtxbytes == 21 ) qtxbytes = 0; 	// ce n'est que notre requete
		   } while( ( qtxbytes == 0 ) && ( ++trips < 20 ) );
		printf("\n");
		}

		if ( qtxbytes > 29 )
		   gasp("longueur message RS485 %d", qtxbytes );
		irb.rxbuf[qtxbytes+2] = 0;	// ASCIIZ terminator
		printf(" %s\n", irb.rxbuf+2 );
		if   ( qtxbytes == 22 )
		     { printf("status %s, write mode %s\n", irb.rxbuf+16,
			       (irb.rxbuf[18] & 1)?("RAM"):("EEPROM") ); }
		else printf("reponse %d bytes !?\n", qtxbytes );
		break;

     case 'W' : irb.txbuf[0] = RS485T;
		//		     5    4   2 2 total 13 bytes 
		//		     adr--    op    
		//		          cmd-  v-
		sprintf( irb.txbuf+1, "0%1d00030050401",   OMRaddr );
		printf(          " 0%1d00030050401\n", OMRaddr );
		qtxbytes = strlen( irb.txbuf );

		irb.txcnt = qtxbytes; irb.rxcnt = 1;
		if   ( dial( &irb ) )
                     printf(" echec dialogue\n");
		else printf(" write mode is RAM\n");
		break;

     case 'q' : fin++;				break;
     case ' ' : rs485_usage();			break;
     default  :					break;
     }
   }
}
