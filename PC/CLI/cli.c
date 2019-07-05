/* Frevo8 : flashage + debug, PIC 18F452 et PIC 24F
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "../../ipilot.h"
#include "../../version.h"
#include "../diali.h"

#include "cli_bytes.h"
#include "cli_ibase.h"
#include "cli_iproc.h"
#include "cli_flash.h"
#include "cli_onew.h"
#include "cli_rs485.h"

#include "../UDP/i2c_udp_mast.h"
#include "../UDP/dialu.h"
#include "U24/cli24.h"


void gasp( const char *fmt, ... )  /* fatal error handling */
{
  va_list  argptr;
  fprintf( stderr, "\nSTOP :\n" );
  va_start( argptr, fmt );
  vfprintf( stderr, fmt, argptr );
  va_end( argptr );
  fprintf( stderr, "\n" );
  exit(1);
}

/* =================== interface configuration ============ */

void config_menu()
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
config_menu();
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
     case ' ' : config_menu();			break;
     case 'q' : fin++; 				break;
     }
   }
}

/* =================== menu principal ============ */

void pass_menu()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  1 : programmation PIC18F\n" );
   printf("  f : debug flash/PIC\n");
   printf("-----\n");
   printf("  b : dialogue niveau bytes\n" );
   printf("  i : debug base IPILOT\n");
   printf("  c : configuration\n");
   printf("-----\n");
   printf("  q : quitter\n\n");
}

void pass_ui( char * nom )
{
char locar; int fin;
pass_menu();
fin = 0;
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case '1' : prog_ui(nom);	pass_menu();	break;
     case 'f' : flash_ui();	pass_menu();	break;
     case 'b' : bytes_ui();	pass_menu();	break;
     case 'i' : ibase_ui();	pass_menu();	break;
     case 'c' : config_ui();	pass_menu();	break;
     case ' ' : pass_menu();	break;
     case 'q' : fin++;		break;
     default : 			break;
     }
   dialogue_log_flush();
   }
}

void proc_menu()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  1 : programmation PIC18F\n" );
   printf("  f : debug flash/PIC\n");
   printf("-----\n");
   printf("  b : dialogue niveau bytes\n" );
   printf("  i : debug base IPILOT\n");
   printf("  c : configuration\n");
   printf("-----\n");
   printf("  r : debug process recette\n");
   printf("  o : debug RS485/omron\n");
   printf("  w : debug 1-wire\n");
   printf("-----\n");
   printf("  q : quitter\n\n");
}

void proc_ui( char * nom )
{
char locar; int fin;
proc_menu();
fin = 0;
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case '1' : prog_ui(nom);	proc_menu();	break;
     case 'f' : flash_ui();	proc_menu();	break;
     case 'b' : bytes_ui();	proc_menu();	break;
     case 'i' : ibase_ui();	proc_menu();	break;
     case 'c' : config_ui();	proc_menu();	break;
     case 'r' : iproc_ui();	proc_menu();	break;
     case 'o' : rs485_ui();	proc_menu();	break;
     case 'w' : onew_ui();	proc_menu();	break;
     case ' ' : proc_menu();	break;
     case 'q' : fin++;		break;
     default : 			break;
     }
   dialogue_log_flush();
   }
}

void midi_menu()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  2 : programmation PIC24F\n" );
   printf("-----\n");
   printf("  b : dialogue niveau bytes\n" );
   printf("  i : debug base IPILOT\n");
   printf("  c : configuration\n");
   printf("-----\n");
   printf("  w : debug 1-wire\n");
   printf("-----\n");
   printf("  q : quitter\n\n");
}

void midi_ui( char * nom )
{
char locar; int fin;
midi_menu();
fin = 0;
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case '2' : mpar_ui(nom);	midi_menu();	break;
     case 'b' : bytes_ui();	midi_menu();	break;
     case 'i' : ibase_ui();	midi_menu();	break;
     case 'c' : config_ui();	midi_menu();	break;
     case 'w' : onew_ui();	midi_menu();	break;
     case ' ' : midi_menu();	break;
     case 'q' : fin++;		break;
     default : 			break;
     }
   dialogue_log_flush();
   }
}
/* old menu generique
void cli_menu()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  1 : programmation PIC18F\n" );
   printf("  2 : programmation PIC24F\n" );
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
cli_menu();
fin = 0;
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case '1' : prog_ui(nom);	cli_menu();	break;
     case '2' : mpar_ui(nom);	cli_menu();	break;
     case 'b' : bytes_ui();	cli_menu();	break;
     case 'i' : ibase_ui();	cli_menu();	break;
     case 'r' : iproc_ui();	cli_menu();	break;
     case 'f' : flash_ui();	cli_menu();	break;
     case 'o' : rs485_ui();	cli_menu();	break;
     case 'w' : onew_ui();	cli_menu();	break;
     case 'c' : config_ui();	cli_menu();	break;
     case ' ' : cli_menu();	break;
     case 'q' : fin++;		break;
     default : 			break;
     }
   dialogue_log_flush();
   }
}
*/

/* ================= the main ====================== */

void cli_usage( const char * progname )
{
gasp("usage : %s <fours.xml> <numero_four> {nom fichier hex}\n"
     "ou      %s <adresse IP> {nom fichier hex}\n", progname, progname );
}

void main_menu()
{
printf("\nSuperviseur CLI Frevo %d.%d%c (incluant programmeur PIC)\n",
	VERSION, SUBVERS, BETAVER );
printf("  u : acces microcontroleur PASS   (UDP direct)\n");
printf("  p : acces microcontroleur PROC   (i2c via passerelle UDP)\n"
       "      (permet aussi acces PASS via PASS)\n" );
printf("  m : acces microcontroleur SECU24 (MIDI opto/uart via passerelle UDP)\n");
printf("  q : quitter\n");
}

int main( int argc, char ** argv )
{
char locar; int fin=0;
char * hexpath = "";

if	( argc < 2 )
	cli_usage(argv[0]);

if	( isdigit( argv[1][0] ) )
	{
	unsigned char IP[4];
	txt2ip( IP, argv[1] );
	dialugue_set_IP( IP );
	if	( argc > 2 )
		hexpath = argv[2];
	}
else	{
	if	( argc < 3 )
		cli_usage(argv[0]);
	bridge_initfour( argv[1], atoi(argv[2]) );	// lecture fours.xml
	dialugue_set_IP( bridge_get_destIP() );
	if	( argc > 3 )
		hexpath = argv[3];
	}

openUDP();
main_menu();

while ( ! fin )
   {
   locar = getchar();
   switch( locar )
	{
	case 'u' : dialogue_set_acces( locar ); pass_ui( hexpath ); main_menu(); break;
	case 'p' : dialogue_set_acces( locar ); proc_ui( hexpath ); main_menu(); break;
	case 'm' : dialogue_set_acces( locar ); midi_ui( hexpath ); main_menu(); break;
	case ' ' : main_menu(); break;
	case 'q' : fin++;
	}
   }

return(0); 
}
