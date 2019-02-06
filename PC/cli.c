/* Frevo7 : flashage + debug du PIC 18F452

Code commun commun pour I2C/USB I2C/UDP et UDP
L'executable n'est pas commun a cause des incompatibilites systeme.

 I2C via USB
	Win32 sous VC6

 I2C via UDP
 UDP direct
	Win32 sous Cygwin
	Linux Fedora

Definir PASS_USB pour passerelle USB, sinon c'est UDP

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "../ipilot.h"
#include "../version.h"
#include "diali.h"

#include "cli_bytes.h"
#include "cli_ibase.h"
#include "cli_iproc.h"
#include "cli_flash.h"
#include "cli_onew.h"
#include "cli_rs485.h"

#ifndef PASS_USB
#include "UDP/i2c_udp_mast.h"
#include "UDP/dialu.h"
#endif


void gasp( char *fmt, ... )  /* fatal error handling */
{
  va_list  argptr;
  fprintf( stderr, "\nSTOP : " );
  va_start( argptr, fmt );
  vfprintf( stderr, fmt, argptr );
  va_end( argptr );
  fprintf( stderr, "\n" );
  exit(1);
}

/* =================== interface configuration ============ */

void config_usage()
{
   printf("\nConfig \"dialogue\" verb=%02X\n", dialogue_get_verbose() );
   printf("  vXX : verbose (+2 ok's, +4 mess. entiers, +8 fatalisation, +0x10 time)\n");
   printf("  f   : sortie vers fichier\n");
}

void config_ui()
{
char locar; int fin, verb;
FILE * fil;

fin = 0;
config_usage();
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'v' : scanf("%x", &verb );
		dialogue_set_verbose( verb );
		if ( dialogue_get_log() == NULL )
		   dialogue_set_log( stdout );
		printf(" Ok\n"); fin++;		break;
     case 'f' : fil = fopen("ipilot.log", "w" );
		if ( fil == NULL )
		   gasp("echec creation ipilot.log");
		dialogue_set_log( fil );
		printf(" Ok\n"); fin++;		break;
     case ' ' : config_usage();			break;
     case 'q' : fin++; 				break;
     }
   }
}

/* =================== menu principal ============ */

void cli_usage()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  p : programmation PIC\n" );
   printf("-----\n");
   printf("  b : dialogue niveau bytes\n" );
   printf("  i : debug base IPILOT\n");
   printf("  c : configuration\n");
   printf("-----\n");
   printf("  r : debug process recette\n");
   printf("  f : debug flash/PIC\n");
   printf("  o : debug RS485/omron\n");
   printf("  w : debug 1-wire\n");
   printf("-----\n");
   printf("  q : quitter\n\n");
}

void cli_ui( char * nom )
{
char locar; int fin;
cli_usage();
fin = 0;
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'p' : prog_ui(nom);	cli_usage();	break;
     case 'b' : bytes_ui();	cli_usage();	break;
     case 'i' : ibase_ui();	cli_usage();	break;
     case 'r' : iproc_ui();	cli_usage();	break;
     case 'f' : flash_ui();	cli_usage();	break;
     case 'o' : rs485_ui();	cli_usage();	break;
     case 'w' : onew_ui();	cli_usage();	break;
     case 'c' : config_ui();	cli_usage();	break;
     case ' ' : cli_usage();	break;
     case 'q' : fin++;		break;
     default : 			break;
     }
   dialogue_log_flush();
   }
}


/* ================= the main ====================== */

void main_usage()
{
printf("\nSuperviseur CLI Frevo %d.%d%c (incluant programmeur PIC %s)\n",
	VERSION, SUBVERS, BETAVER, "Flash 18Fxx2" );
printf("  i : acces i2c via USB\n" );
printf("  p : acces i2c via passerelle UDP\n");
printf("  m : acces MIDI (opto/uart) via passerelle UDP\n");
printf("  u : acces UDP direct\n");
printf("  q : quitter\n");
}

int main( int argc, char ** argv )
{
char locar; char * nom = ""; int fin=0;

if ( argc < 2 )
   gasp("usage : %s numero_four_dans_xml {nom fichier hex}\n", argv[0] );

bridge_initfour( atoi(argv[1]) );	// lecture fours.xml
#ifndef PASS_USB
dialugue_set_IP( bridge_get_destIP() );
#endif

if   ( argc >= 3 )
     nom = argv[2];

i2c_init();
main_usage();

while ( ! fin )
   {
   locar = getchar();
   switch( locar )
	{
#ifdef PASS_USB
	case 'i' : dialogue_set_acces( locar );
		   cli_ui( nom ); main_usage(); break;
	case 'p' :
	case 'u' : gasp("UDP non disponible sur cette version"); break;
#else
	case 'i' : gasp("USB non disponible sur cette version"); break;
	case 'p' :
	case 'u' :
	case 'm' : dialogue_set_acces( locar );
		   cli_ui( nom ); main_usage(); break;
#endif
	case ' ' : main_usage(); break;
	case 'q' : fin++;
	}
   }

i2c_disable();

return(0); 
}
