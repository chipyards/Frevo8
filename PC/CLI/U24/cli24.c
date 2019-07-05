#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../../../version.h"
#include "../../../mpar.h"
#include "../../../ipilot.h"
#include "../../UDP/dialu.h"
#include "../../irb.h"
#include "../../diali.h"
#include "flash24.h"

void gasp( char *fmt, ... );  /* fatal error handling */

// stub functions pour les besoins de diali.c
//int i2c_put_chk( unsigned char I2Caddr, unsigned char *val, int N ) { return 0; };
//int i2c_get_chk( unsigned char I2Caddr, unsigned char *val, int N ) { return 0; };

void mpar_usage( const char * firmware_name )
{
   printf("\n=== CLI tests/bootloader PIC33/PIC24 %d.%d%c ===\n",
	   VERSION, SUBVERS, BETAVER );
   printf("  !,? : toggle verbose\n");
   printf("  V : afficher version firmware\n");
   printf("  T : afficher system time Frevo\n");
   printf("  F : flasher fichier \"%s\"\n", firmware_name );
   printf("  R : relire et comparer fichier \"%s\"\n", firmware_name );
   printf("  C : verifier CRC en flash\n");
   printf("  E : executer appli apres verif CRC\n");
   printf("  B : retour au bootloader (sw reset)\n");
   printf("  q : quitter ce programme\n");
}

void mpar_ui( const char * firmware_name )
{
irblock irb;
char locar; int fin;

fin = 0; mpar_usage( firmware_name );
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'F' : flash_file( firmware_name );
		break;
     case 'R' : check_file( firmware_name );
		break;
     case 'C' : remote_crc( );
		break;
     case 'E' : remote_exec( );
		break;
     case 'B' : remote_reset( );
		break;
     case 'V' : {
		char version[8]; char betaver, role;
		if   ( read_version( version ) )
		     printf(" echec dialogue\n");
		printf(" Ok version %c.%c", version[0], version[2] );
		betaver = version[3];
		role = version[4];
		if ( role < ' ' ) role = '_';
		if ( betaver < ' ' ) betaver = '_';
		printf("%c, role %c\n", betaver, role );
		// printf("version PIC %s\n", version );
		} break;
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
		     }
		break;
     case '!' : dialugue_set_verbose( dialugue_get_verbose() ^ 1 );
		break;
     case '?' : dialugue_set_verbose( dialugue_get_verbose() ^ 2 );
		break;
     case 'q' : fin++;
		break;
     case ' ' : mpar_usage( firmware_name );
		break;
     default  :	break;
     }
   }
}

/*
int main(int argc, char ** argv)
{
unsigned char destIP[4];	// adresse internet

if ( argc < 2 )
   gasp("donner adresse IP (dotted decimal)");

if   ( argc > 2 )
     {
     firmware_name = argv[2];
     test_hex_file(firmware_name);
     }
else firmware_name = "no_name";


txt2ip( destIP, argv[1] );
dialugue_set_IP( destIP );
dialogue_set_acces( 'm' );

openUDP();

mpar_ui();

return 0;
}
*/
