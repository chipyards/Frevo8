/* =================== interface debug IPILOT pour process recette ============ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../ipilot.h"
#include "diali.h"
#include "fpilot.h"
#include "crc32.h"
#include "cli_iproc.h"
#include <fstream>
#include <string>
using namespace std;
#include "xmlpb.h"
#include "frevo_dtd.h"
#include "process.h"

extern "C" void gasp( char *fmt, ... );  /* fatal error handling */

// static storage all in one
extern four tube;

// appeler apres recipe::dump_pack()
void save_dump()
{
FILE * fump;
fump = fopen("dump.txt", "w" );
if ( fump == NULL )
   gasp("echec ouverture dump.txt pour ecriture");
fputs( tube.recette.dump.c_str(), fump );
fclose(fump);
}


void compilu( int upflag )
{
unsigned char *pack; int packlen;
static char fnam[1024];

//fflush( stdin );
//printf("repertoire recettes : ");
//scanf("%s", fnam );
//tube.xml_dir = string(fnam);

fflush( stdin );
printf("nom de la recette a charger : ");
scanf("%s", fnam );
tube.recette.filename = string(fnam);

tube.recette.load_xml();
if ( tube.recette.stat > -2 )
   tube.recette.check();
if ( tube.recette.stat > -2 )
   tube.recette.make_pack();

if   ( tube.recette.stat < -1 )
     { printf( "%s\n", tube.recette.errmess.c_str() ); return; }

if   ( upflag )
     {
     pack = tube.recette.pack;
     packlen = ((int)pack[2]) << 8;
     packlen |= pack[1];
     printf("crc = %08X\n", tube.recette.crc );
     printf("about to upload %d=%04X bytes @ 300\n", packlen, packlen );
     if   ( packlen <= (3*256) )
	  upload( 0x300, pack, packlen );
     else gasp("recette trop grosse");
     }
else {
     tube.recette.dump_pack();
     puts( tube.recette.dump.c_str() );
     }
}

void emule_gui()
{
status_full sf; int ivan, imfc, item;
get_status( &sf );

printf("\nVannes : /");
for ( ivan = QVAN-1; ivan >= 0; ivan-- )
    {
    if   ( sf.vannes & ( 1 << ivan ) )
	 printf("%s/", tube.vanne[ivan].name.c_str() );
    }
printf("\n");
printf("MFC PV : ");
for ( imfc = QMFC-1; imfc >= 0 ; imfc-- )
    printf("%9.3f ", ((double)sf.mfc[imfc].pv) * (5.0/65472.0)  );
printf("Volts \n");
printf("    SV : ");
for ( imfc = QMFC-1; imfc >= 0 ; imfc-- )
    printf("%9.3f ", ((double)sf.mfc[imfc].sv) * (5.0/65520.0)  );
printf("\n");
printf("         ");
for ( imfc = QMFC-1; imfc >= 0 ; imfc-- )
    printf("%9s ", tube.mfc[imfc].name.c_str() );
printf("\n");
printf("TEM PV : ");
for ( item = 0; item < QTEM; item++ )
    printf("%9.3f ", ((double)sf.temp[item].pv)/16.0  );
printf("degres \n");
printf("    SV : ");
for ( item = 0; item < QTEM; item++ )
    printf("%9.3f ", ((double)sf.temp[item].sv)/16.0  );
printf("\n");
printf("         ");
for ( item = 0; item < QTEM; item++ )
    printf("%9s ", tube.tem[item].name.c_str() );
printf("\n");
#define KF (10000000.0/33554432.0)
printf("frequ : %7.1f Hz\n", KF * (double)sf.frequ );
printf("step %03d - %d s.   flags=%02X ",
	sf.step, sf.chrono,sf.flags );
if ( sf.flags & PAUSE ) printf("PAUSE ");	
if ( sf.flags & MANU )  printf("MANU ");	
if ( sf.flags & ROOT )  printf("ROOT ");
printf("\n");
}


// fonctions permettant d'acceder a l'objet tube depuis le 'C'

extern "C" void bridge_initfour( int ifou )
{
tube.ifou = ifou;
tube.load_xml();
}

extern "C" unsigned char * bridge_get_destIP()
{
return tube.destIP;
}

/*=========================== CLI ========================= */

static void iproc_usage()
{
printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
if   ( dialogue_get_acces() == 'm' )
     {
     printf("  a : allumer\n");
     printf("  s : afficher status automate secu\n");
#ifndef _WIN32
     printf("  S : afficher status automate secu en boucle\n");
#endif
     }
else {
     printf("  F : afficher status process Frevo 6+\n");
     printf("  f : afficher frequence canal Baratron Frevo 7.5+\n");
     printf("  g : afficher process style GUI 7.8\n");
#ifndef _WIN32
     printf("  G : afficher process style GUI 7.8 en boucle\n");
#endif
     printf("  c : lire CRC\n");
     printf("  a : autoriser CRC\n");
     printf("  + : avancer chrono de 10 s\n");
     printf("  C : compiler + dump hex recette\n");
     printf("  D : compiler + dump hex recette dans fichier dump.txt\n");
     printf("  U : compiler + upload   recette\n");
     printf("  q : quitter ce menu\n");
     }
}

static void show_secu()
{
int sbuf[4];
secu_get_status( sbuf );
double freq;
#define KF (10000000.0/33554432.0)
freq = KF * (double)sbuf[1];
printf("step=%d, frequ=%7.1fHz (%d), t=%d\n",
	sbuf[0], freq, sbuf[1], sbuf[2] );
}

#ifndef _WIN32
static void siproc_poll( int period )
{
while(1)
   {
   show_secu();
   sleep( period );
   }
}

static void iproc_poll( int period )
{
while(1)
   {
   emule_gui();
   sleep( period );
   }
}
#endif

static void siproc_ui()
{
char locar; int fin;
fin = 0;

while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'a' : secu_set_param( 0, 1 );		break;
     case 's' :	show_secu();			break;
#ifndef _WIN32
     case 'S' :	siproc_poll( 2 );		break;
#endif
     case ' ' : iproc_usage();			break;
     case 'q' : fin++;				break;
     default  :					break;
     }
   }
}

extern "C" void iproc_ui()
{
iproc_usage(); fpilot_set_log( stdout );

if ( dialogue_get_acces() == 'm' )
   { siproc_ui(); return; }

char locar; int fin; int i;

fin = 0; 
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case '+' : {
		status_full sf;
		get_status( &sf );
		set_chron( sf.chrono + 10 );
		}
     case 'F' : {
		status_full sf;
		get_status( &sf );
		printf("step=%03d,  vannes=%04X,  flags=%02X, chrono=%d s. ",
			sf.step,
			sf.vannes,
			sf.flags,
			sf.chrono );
		if ( sf.flags & PAUSE ) printf("PAUSE ");	
		if ( sf.flags & MANU )  printf("MANU ");	
		if ( sf.flags & ROOT )  printf("ROOT ");
		printf("\n        SV h      d      u    PV h      d      u\n");

		for ( i = 0; i < 4; i++ )
		    printf("MFC %d   %04X  %5d  %5.3f    %04X  %5d  %5.3f\n", i,
			sf.mfc[i].sv, sf.mfc[i].sv, ((double)sf.mfc[i].sv) * (5.0/65520.0),
			sf.mfc[i].pv, sf.mfc[i].pv, ((double)sf.mfc[i].pv) * (5.0/65472.0)  );
		// printf("%03X=%4d\n", sf.auxADC[0], sf.auxADC[0] );
		// printf("%03X=%4d\n", sf.auxADC[1], sf.auxADC[1] );
		for ( i = 0; i < 3; i++ )
		    printf("TEM %d   %04X  %5d  %5.1f    %04X  %5d  %5.1f\n", i,
			sf.temp[i].sv, sf.temp[i].sv, ((double)sf.temp[i].sv)/16.0,
			sf.temp[i].pv, sf.temp[i].pv, ((double)sf.temp[i].pv)/16.0  );
		}				break;
     case 'g' :	emule_gui();			break;
#ifndef _WIN32
     case 'G' :	iproc_poll( 2 );		break;
#endif
     case 'f' : {
		status_full sf; double freq;
		get_status( &sf );
		freq = KF * (double)sf.frequ;
		printf("frequ : %7.1f Hz (%04X)\n", freq, sf.frequ );
		}				break;
     case 'a' : set_crc_autor();
     case 'c' : {
		status_full sf;
		get_crc( &sf );
		printf("%d bytes ", sf.packlen );
		if   ( sf.crc_stat & CRC_READY )
		     printf("crc = %08X", sf.crc );
		else printf("come back later");
		if ( sf.crc_stat & CRC_AUTOR ) printf(" autor. Ok");
		if ( sf.crc_stat & CRC_CALC ) printf(" calc. en cours");
		printf("\n");
		}				break;
     case 'C' : compilu( 0 );			break;
     case 'D' : compilu( 0 ); save_dump();	break;
     case 'U' : compilu( 1 );			break;
     case 'q' : fin++;				break;
     case ' ' : iproc_usage();			break;
     default  :					break;
     }
   }
}

