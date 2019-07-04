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
extern unsigned char destIP[];	// cf i2c_udp_mast.c
extern int destport;
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
   printf("\nConfig \"dialogue\"\n");
   printf("  v : verbose\n");
   printf("  s : silent\n");
}

void config_ui()
{
char locar; int fin;

fin = 0;
config_usage();
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'v' : dialogue_set_verbose( 1 );  
		printf(" Ok\n"); fin++;		break;
     case 's' : dialogue_set_verbose( 0 );       
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
   }
}

#ifndef PASS_USB
// rudimentaire conversion numero IP ascii en tableau de bytes
void txt2ip( unsigned char * IP, char * text )
{
unsigned int II[4];
if ( sscanf( text, "%u.%u.%u.%u", II, II+1, II+2, II+3 ) != 4 )
   gasp( "incorrect IP number %s", text );
IP[0] = (unsigned char) II[0];
IP[1] = (unsigned char) II[1];
IP[2] = (unsigned char) II[2];
IP[3] = (unsigned char) II[3];
}
#endif


/* ================= the main ====================== */

void main_usage()
{
printf("\nProgrammeur PIC %s version Frevo %d.%d\n",
	"Flash 18Fxx2", VERSION, SUBVERS );
printf("  i : acces i2c via USB\n" );
printf("  p : acces i2c via passerelle UDP\n");
printf("  u : acces UDP direct\n");
printf("  q : quitter\n");
}

int main( int argc, char ** argv )
{
char locar; char * nom = ""; int fin=0;

#ifdef PASS_USB
if   ( argc >= 2 )
     sprintf( nom, "%s", argv[1] );
#else
/* accepter 0, 1 ou 2 arguments
   si argument numerique : numero IP
   sinon : nom de fichier hex 
 */
if   ( argc >= 2 )
     {
     if   ( isdigit(argv[1][0]) )
          txt2ip( destIP, argv[1] );
     else nom = argv[1];
     }

if   ( argc == 3 )
     {
     if   ( isdigit(argv[2][0]) )
          txt2ip( destIP, argv[2] );
     else nom = argv[2];
     }
#endif

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
	case 'u' : dialogue_set_acces( locar );
		   cli_ui( nom ); main_usage(); break;
#endif
	case ' ' : main_usage(); break;
	case 'q' : fin++;
	}
   }

i2c_disable();

return(0); 
}
