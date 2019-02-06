/* fonctions pour supervision 
   Dialogue IPILOT commun pour I2C/USB I2C/UDP et UDP
   via dial() (diali.c)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../ipilot.h"
#include "diali.h"

#ifdef PASS_USB
#include "USB/i2c_usb_mast.h"
#include "USB/dialu_stub.h"
#else
#include "UDP/i2c_udp_mast.h"
#include "UDP/dialu.h"
#endif

void gasp( char *fmt, ... );  /* fatal error handling */

/* ================= configuration ========================== */

static int verbose = 0;
static int ntenta = 10;
static unsigned char I2Caddr = 0x58;
static char acces = 'x';
static FILE * dial_logfil = NULL;

unsigned char dialogue_get_I2Caddr()
{ return(I2Caddr); }

void dialogue_set_tenta( int tenta )
{ ntenta = tenta; dialugue_set_tenta( 4 ); }

int dialogue_get_tenta()
{ return(ntenta); }

void dialogue_set_verbose( int verb )
{ verbose = verb; dialugue_set_verbose( verb ); }

int dialogue_get_verbose()
{ return(verbose); }

void dialogue_set_log( FILE * f )
{ dial_logfil = f; dialugue_set_log( f ); }

FILE * dialogue_get_log()
{ return dial_logfil;  }

void dialogue_log_flush()
{ if ( dial_logfil ) fflush( dial_logfil ); }

void dialogue_set_acces( char lacces )
{ acces = lacces; }

char dialogue_get_acces()
{ return(acces);  }

char * dialogue_get_acces_text()
{
switch(acces)
   {
   case 'i' : return("I2C via USB"); break;
   case 'p' : return("I2C via UDP"); break;
   case 'u' : return("direct UDP "); break;
   case 'm' : return("MID via UDP"); break;
   }
return("");
}

/* ================= error logging ========================== */


char * ipiloterr( unsigned char v )	// selon ipilot.h
{
switch(v)
   {
   case SOUNK  : return("opcode inconnu");
   case ILLERR : return("requete illegale");
   case SFERRA : return("tentative ecriture sur adresse protegee");
   case CHKERR : return("erreur checksum I2C detectee par slave I2C");
   case NAKI2C : return("erreur sur I2C secondaire");
   case LENERR : return("erreur longueur message");
   case XOFF   : return("XOFF lecture prematuree");
   default     : {
		 static char lbuf[64];
		 sprintf( lbuf, "erreur ipilot non specifiee %02X", v );
		 return(lbuf);
		 }
   }
}


/* code "got"
	cree par :				i2c_put_chk()	i2c_get_chk()	dialu	reinterpret()
	0 = Ok					x		x
	1 = lock passerelle			x
	2 = erreur checksum I2C sur reponse			x
	3 = I2C nack (relaye par passerelle)	x		x
	4 = erreur protocole (retour UDP )	x		x
	7 = erreur opcode retour "ipiloterr"							x
	254 = echec socket (send)						x
	255 = timeout								x

	un got 0 ou 2 peut etre reinterprete en 7 : cf dial

   pour i2c_put_chk() et i2c_get_chk(), N est "payload size, checksum exclu"
*/


/* logging : bits de verbose
	1 : log erreurs seulement
	2 : log succes aussi
	4 : messages entiers (sinon : 3 bytes)
	8 : fatalisation de certaines erreurs
       16 : timestamps
 */
void dial_log( char dir, irblock *irb )
{
int got, cnt; unsigned char *buf;

if ( ( dial_logfil == NULL ) || ( verbose == 0 ) )
   return;

if   ( dir == 'W' )
     { got = irb->txgot; cnt = irb->txcnt; buf = irb->txbuf; }
else { got = irb->rxgot; cnt = irb->rxcnt; buf = irb->rxbuf; }

if ( ( got > 0 ) || ( verbose & 2 ) || ( irb->tenta < ntenta ) )
   {
   int i;
   fprintf( dial_logfil, "~%c~ ", dir );
   #ifndef PASS_USB
   if ( verbose & 16 ) time_log();
   #endif
   fprintf( dial_logfil, "[ " );
   if ( ( cnt > 3 ) && ( ( verbose & 4 ) == 0 ) )
      cnt = 3;
   for ( i = 0; i < cnt; i++ )
       fprintf( dial_logfil, "%02X ", buf[i] );
   fprintf( dial_logfil, "]\n" );
   }

switch( got )
	{
	case   1 : fprintf( dial_logfil, "~%c~ %d lock passerelle\n", dir, irb->tenta );		break;
	case   2 : fprintf( dial_logfil, "~%c~ %d erreur checksum I2C\n", dir, irb->tenta );		break;
	case   3 : fprintf( dial_logfil, "~%c~ %d I2C nack (vu par passerelle)\n", dir, irb->tenta );	break;
	case   4 : fprintf( dial_logfil, "~%c~ %d erreur protocole retour UDP\n", dir, irb->tenta );	break;
	case 253 : fprintf( dial_logfil, "~%c~ timout reponse UART-MIDI\n", dir);			break;
	case 254 : fprintf( dial_logfil, "~%c~ %d echec send socket\n", dir, irb->tenta );		break;
	case 255 : fprintf( dial_logfil, "~%c~ %d timout recv socket\n", dir, irb->tenta );		break;
	case   7 : fprintf( dial_logfil, "~%c~ erreur IPILOT %s (opcode %02x)\n", dir,
					 ipiloterr( irb->rxbuf[0] ), irb->txbuf[0] );			break;
	case   0 : if ( ( verbose & 2 ) || ( irb->tenta < ntenta ) )
		      fprintf( dial_logfil, "~%c~ %d Ok\n", dir, irb->tenta );				break;
	}

if ( verbose & 8 )
   switch( got )
	{
	case   4 : fflush( dial_logfil ); gasp("erreur protocole frevo6, retour UDP" );		break;
	case 254 : fflush( dial_logfil ); gasp("echec send socket" );				break;
	case   7 : fflush( dial_logfil ); gasp("erreur IPILOT %s (opcode %02x)",
						ipiloterr( irb->rxbuf[0] ), irb->txbuf[0] );	break;
	}
}

/* ================= dialogue ========================== */

/* reinterpret
	- verification du retour de l'opcode ipilot
	- mise a jour eventuelle de rxgot
 */
void reinterpret( irblock * irb )
{
switch ( irb->rxgot )
   {
   case 2 :	// checksum error
	{
	// l'erreur de checksum peut etre due a un code d'erreur ipilot
	// le code d'erreur est transmis avec un checksum correct, mais
	// calcule sur 1 byte, il faut qu'on le verifie ici :
	if ( ( ( irb->rxbuf[0] + irb->rxbuf[1] ) & 0xFF ) == MYRSUM )
	   if ( irb->rxbuf[0] != irb->txbuf[0] )
	      irb->rxgot = 7;
	} break;
   case 0 :	// looks good...verifier opcode
	{
	if ( irb->rxbuf[0] == irb->txbuf[0] )
	   break;
	if ( ( ( irb->txbuf[0] & 0xF0 ) == 0x20 ) &&	// opcodes UE UW UR UL UN UA
	     ( ( irb->rxbuf[0] & 0xF0 ) == 0x20 )
	   ) break;
	irb->rxgot = 7;
	} break;
   }
}

/* diali
	- supporte les acces i, p
	- fonction bloquante jusqu'a reussite dialogue ou
	  epuisement du nombre de tentatives
	- aucune erreur n'est consideree comme fatale a ce niveau...
	  mais dial_log peut le faire
	- retourne irb->rxgot (zero si ok)
 */

int diali( irblock * irb )
{
irb->tenta = ntenta; irb->txgot = -1;
// reiterer tentatives d'emettre la requete
while ( irb->tenta )
   {
   // que doit-on faire ?
   if	( irb->txgot )
	{		// emettre requete write I2C
	irb->txgot = i2c_put_chk( I2Caddr, irb->txbuf, irb->txcnt );
	if ( verbose )
	   dial_log( 'W', irb );
	}
   if	( irb->txgot == 0 )
	{		// alors emettre requete read I2C
	irb->rxgot = i2c_get_chk( I2Caddr, irb->rxbuf, irb->rxcnt );
	reinterpret( irb );
	if ( verbose )
	   dial_log( 'R', irb );
	// est-ce bon finalement ?
	if ( irb->rxgot == 0 )
	   return(irb->rxgot);
	}
   // ce n'est pas encore bon...
   irb->tenta--;
   }
irb->rxgot = -1;
return(irb->rxgot);
}

// communication avec le PIC24F : uart via UDP =====================

// fonction ping-pong :
// 	- envoir le message pour PIC24 prealablement prepare dans irb
//	- reitere les requetes UDP jusqu'a collecter le nombre de bytes demandes (ou timout)
// le contenu destine au PIC24 commence par un opcode MPAR
// la fonction verifie l'echo de cet opcode EN FIN DE REPONSE et le remet AU DEBUT
// cas part. : si rxcnt = 0, n'attend pas la reponse

int dialm( irblock * irb )
{
int ucnt, ui, i;
irblock uirb;		// bloc pour emballage dans requete UDP
unsigned char op;
op = irb->txbuf[0];	// pour reference ulterieure
ucnt = irb->txcnt;
if ( ucnt > (QIPILOT+2) )
   ucnt = (QIPILOT+2);	// securite (N.B. buffers font QIPILOT+4)
uirb.txbuf[0] = UURW;
uirb.txbuf[1] = ucnt;
for ( i = 0; i < ucnt; i++ )
    uirb.txbuf[i+2] = irb->txbuf[i];
uirb.txcnt = uirb.rxcnt = ucnt + 2;		
dialu( &uirb );
reinterpret( &uirb ); // dialu ne prend pas en charge la verif opcode ipilot
if ( verbose )
   dial_log( 'U', &uirb );
if ( uirb.rxgot )
   { irb->rxgot = uirb.rxgot; return( irb->rxgot ); }
if ( irb->rxcnt == 0 )
   { irb->rxgot = 0; return( irb->rxgot ); }

// attendre la reponse
uirb.txbuf[0] = UURW;
uirb.txbuf[1] = 0;
ucnt = irb->rxcnt;
if ( ucnt > (QIPILOT+2) )
   ucnt = (QIPILOT+2);	// securite (N.B. buffers font QIPILOT+4)
uirb.txcnt = uirb.rxcnt = ucnt + 2;
irb->tenta = 100; ui = 1;
while (--irb->tenta)	// boucle pour attendre disponibilite reponse via PIC18
   {
   dialu( &uirb );
   reinterpret( &uirb ); // dialu ne prend pas en charge la verif opcode ipilot
   if ( verbose )
      dial_log( 'U', &uirb );
   if ( uirb.rxgot )
      { irb->rxgot = uirb.rxgot; return( irb->rxgot ); }
   if ( ( ucnt = uirb.rxbuf[1] ) )	// c'est bon
      {
      for ( i = 2; i < ( ucnt + 2 ); i++ )
	  {
          irb->rxbuf[ui++] = uirb.rxbuf[i];
	  if ( ui >= ( irb->rxcnt + 1 ) )
	     {
	     irb->rxbuf[0] = irb->rxbuf[ui-1];	// rapatriement opcode
	     if ( irb->rxbuf[0] != op )
		{ irb->rxgot = 7; return( irb->rxgot ); };
	     if ( verbose & 2 )
		printf("{%d}\n", irb->tenta );
	     return 0;
	     }
	  }
      }
   }
return 253;		// timout
}

/* dial
	- supporte les 3 acces i, p, u, m
	- fonction bloquante jusqu'a reussite dialogue ou
	  epuisement du nombre de tentatives
	- aucune erreur n'est consideree comme fatale a ce niveau...
	  mais dial_log peut le faire
	- retourne irb->rxgot (zero si ok)
 */

int dial( irblock * irb )
{
switch( acces )
   {
   case 'i' :
   case 'p' : return( diali( irb ) );
	      break;
   case 'u' : if   ( irb->txcnt > irb->rxcnt )	// aligner les tailles au max
	           irb->rxcnt = irb->txcnt;
	      else irb->txcnt = irb->rxcnt;
	      dialu( irb );
	      reinterpret( irb ); // dialu ne prend pas en charge la verif opcode ipilot
	      if ( verbose )
	         dial_log( 'U', irb );
	      return(irb->rxgot);
	      break;
   case 'm' : return( dialm( irb ) );
	      break;
   default : gasp("mode d'acces non supporte : %c", acces );
   }
return(1);	// unreachable
}
