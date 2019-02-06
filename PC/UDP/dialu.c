/* Frevo 8.0 dialogue UDP  portable Linux / Cygwin / Win32
   pas de polling, utilise select()
 */

// avec VC6 : inclure ws2_32.lib

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#ifdef _WIN32
// #include <windows.h> // pas compatible avec winsock2 !?!?!?
// #include <winbase.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// #include <fcntl.h>
#include <time.h>

#include "../../ipilot.h"
#include "dialu.h"
void gasp( char *fmt, ... );  /* fatal error handling */

/* ================= configuration ========================== */

static int verbose = 0;
static int ntenta = 4;
static unsigned char * destIP;
static int destport = 1953;
static FILE * dial_logfil = NULL;
#define U_TIMOUT_MS 200

void dialugue_set_IP( unsigned char * pIP )
{ destIP = pIP; }

unsigned char * dialugue_get_IP()
{ return(destIP); }

int dialugue_get_port()
{ return(destport); }

void dialugue_set_tenta( int tenta )
{ ntenta = tenta; }

int dialugue_get_tenta()
{ return(ntenta); }

void dialugue_set_verbose( int verb )
{ verbose = verb; }

int dialugue_get_verbose()
{ return(verbose); }

void dialugue_set_log( FILE * f )
{ dial_logfil = f;  }

FILE * dialugue_get_log()
{ return dial_logfil;  }

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
/* ================= error logging ========================== */

void time_log()
{
#ifdef _WIN32

SYSTEMTIME ST;
GetLocalTime( &ST );
fprintf( dial_logfil, "%02dh%02d:%02d.%03d ", 
	 ST.wHour, ST.wMinute, ST.wSecond, ST.wMilliseconds );

#else

struct timeval cur_tv;
struct tm *t;

gettimeofday( &cur_tv, NULL );
t = localtime( &cur_tv.tv_sec );
fprintf( dial_logfil, "%02dh%02d:%02d.%03d ",
	 t->tm_hour, t->tm_min, t->tm_sec,
	 (int)cur_tv.tv_usec / 1000 );

#endif
}

/* logging : bits de verbose
	0x10 : timestamps
	0x20 : log erreurs UDP
	0x40 : log succes UDP aussi
	0x04 : messages entiers (sinon : 3 bytes)
	0x08 : fatalisation de certaines erreurs
	0x80 : special suppression flush avant emission requete UDP
 */
void dialu_log( char dir, irblock *irb )
{
int got, cnt; unsigned char *buf;

if ( ( dial_logfil == NULL ) || ( verbose == 0 ) )
   return;

if   ( dir == 'w' )
     { got = irb->txgot; cnt = irb->txcnt; buf = irb->txbuf; }
else { got = irb->rxgot; cnt = irb->rxcnt; buf = irb->rxbuf; }

if ( ( got > 0 ) || ( verbose & 0x40 ) || ( irb->tenta < ntenta ) )
   {
   int i, ilim;
   fprintf( dial_logfil, "  ~%c~ ", dir );
   if ( verbose & 0x10 ) time_log();
   fprintf( dial_logfil, "[ " );
   ilim = ( ( cnt > 3 ) && ( ( verbose & 4 ) == 0 ) )?3:cnt;
   for ( i = 0; i < ilim; i++ )
       fprintf( dial_logfil, "%02X ", buf[i] );
   fprintf( dial_logfil, "]\n" );
   }

switch( got )
	{
	case 254 : fprintf( dial_logfil, "  ~%c~ %d echec send socket\n", dir, irb->tenta );		break;
	case 255 : fprintf( dial_logfil, "  ~%c~ %d timout recv socket\n", dir, irb->tenta );		break;
	case   0 : if ( ( verbose & 0x40 ) || ( irb->tenta < ntenta ) )
		      fprintf( dial_logfil, "  ~%c~ %d Ok\n", dir, irb->tenta );				break;
	}

if ( verbose & 8 )
   switch( got )
	{
	case 254 : fflush( dial_logfil ); gasp("echec send socket" );				break;
	}
}

/* ================= traffic UDP ========================== */

/*
structures pour connect() :

struct sockaddr_in {
    sa_family_t    sin_family; // famille d'adresses : AF_INET      
    u_int16_t      sin_port;   // port dans l'ordre d'octets r�eau 
    struct in_addr  sin_addr;   // adresse Internet                  
    };

// Adresse Internet 
struct in_addr {
    u_int32_t      s_addr;     // Adresse dans l'ordre d'octets r�eau 
    };
*/


// structures intermediaires systeme :
#ifdef WIN32
static WSADATA wsastruct;
static SOCKET  udp_socket;
static SOCKADDR_IN soka;
#else
static int udp_socket;
static struct sockaddr_in soka;
#endif


void openUDP()
{
int retval;
printf("connecting to %d.%d.%d.%d port %d\n", destIP[0], destIP[1], destIP[2], destIP[3], destport );

#ifdef WIN32
retval = WSAStartup( 0x0202, &wsastruct );	// 0202 = version
if ( retval )
   gasp("WSA startup failed");
udp_socket = socket( AF_INET, SOCK_DGRAM, 0 );
if ( udp_socket == INVALID_SOCKET )
   gasp("invalid socket");
#else
udp_socket = socket( PF_INET, SOCK_DGRAM, 0 );
if ( udp_socket < 0 )
   gasp("socket() failed");
#endif

soka.sin_family = AF_INET;
soka.sin_port = htons(destport);
// soka.sin_addr.s_addr = *( (u_int32_t *)destIP );
// soka.sin_addr.s_addr = inet_addr("192.168.1.80");;
soka.sin_addr.s_addr = *( (unsigned long *)destIP );

// plus besoin de NONBLOCK, grace a select()
// retval = fcntl( udp_socket, F_SETFL, O_NONBLOCK );
// if ( retval )
//    gasp("fcntl returned %d", retval );

retval = connect( udp_socket, (struct sockaddr *)&soka, sizeof(soka) );
if ( retval )
   gasp("connect returned %d", retval );
}

// le wrapper de select
static void iu_select( irblock * irb )
{
int retval;
static fd_set rx_set;
static struct timeval rx_timout;

// reinitialiser rx_timout a chaque utilisation 
rx_timout.tv_sec = 0;
rx_timout.tv_usec = U_TIMOUT_MS * 1000;
FD_ZERO( &rx_set );	// <--- indispensable, sinon undef shit in rx_set !!!
FD_SET( udp_socket, &rx_set );

// ATTENTION PIEGE :
// - sous UNIX et Cygwin, le premier arg de select doit etre le
//   "plus grand" fd present dans les differents sets, plus 1 !
// - sous Win32 il est ignore
// ce n'est pas le nombre de fd's !!!!!
retval = select( udp_socket+1, &rx_set, NULL, NULL, &rx_timout );

if ( retval < 0 )
   {
   int errcode = 666;
   #ifdef WIN32
   errcode = WSAGetLastError();
   #endif
   gasp("select failed %d", errcode );
   }
if ( retval == 0 )	// timout
   { irb->rxgot = 255; return; }

retval = recv( udp_socket, (char *)irb->rxbuf, irb->rxcnt, 0 );
if ( retval > 0 )
   { irb->rxgot = 0; return; }
}

// le flusher base sur select
static int iu_flush( irblock * irb )
{
int retval;
static fd_set rx_set;
static struct timeval rx_timout;

// mettre 0,0 ce n'est pas comme NULL !!! (NULL --> infinite timout)  
rx_timout.tv_sec = 0;
rx_timout.tv_usec = 0;
FD_ZERO( &rx_set );
FD_SET( udp_socket, &rx_set );

// ATTENTION PIEGE :
// - sous UNIX et Cygwin, le premier arg de select doit etre le
//   "plus grand" fd present dans les differents sets, plus 1 !
// - sous Win32 il est ignore
// ce n'est pas le nombre de fd's !!!!!
retval = select( udp_socket+1, &rx_set, NULL, NULL, &rx_timout );

if ( retval < 0 )
   {
   int errcode = 666;
   #ifdef WIN32
   errcode = WSAGetLastError();
   #endif
   gasp("select failed %d", errcode );
   }
if ( retval == 0 )	// timout
   return 0;

recv( udp_socket, (char *)irb->rxbuf, QIPILOT+4, 0 );
return 1;
}

/* dialu
	- fonction bloquante jusqu'a reussite dialogue ou
	  epuisement du nombre de tentatives
	- aucune erreur n'est consideree comme fatale a ce niveau...
	  mais dial_log peut le faire
	- retourne irb->rxgot (zero si ok)
 */

int dialu( irblock * irb )
{
int got;
irb->tenta = ntenta; irb->txgot = -1;
// reiterer tentatives d'emettre la requete
while ( irb->tenta )
   {
   // que doit-on faire ?
   if	( irb->txgot )	// emettre requete UDP
	{	
	if ( ( verbose & 0x80 ) == 0 )	// flush prealable
	   while ( iu_flush( irb ) )
		 {
		 if ( dial_logfil != NULL )
		    fprintf( dial_logfil, "  ~u~ flushed 1 message\n");
		 }
	got = send( udp_socket, (char *)irb->txbuf, irb->txcnt, 0 );
	if   ( got > 0 )
	     irb->txgot = 0;
	else { irb->txgot = irb->rxgot = 254; irb->tenta = 1; } // pas la peine d'insister
	if ( verbose )
	   dialu_log( 'w', irb );
	}
   if	( irb->txgot == 0 )
	{		// prendre reponse UDP
	iu_select( irb );
	if ( verbose )
	   dialu_log( 'r', irb );
	// est-ce bon finalement ?
	if ( irb->rxgot == 0 )
	   return(irb->rxgot);
	irb->txgot = -1;	// requete consideree perdue
	}
   // ce n'est pas encore bon...
   irb->tenta--; 
   }
return(irb->rxgot);
}
