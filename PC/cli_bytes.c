/* Frevo 7 : interface utilisateur pour test messages "niveau byte"
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../ipilot.h"
#include "diali.h"

#ifdef PASS_USB
#include "USB/i2c_usb_mast.h"
#else
#include "UDP/i2c_udp_mast.h"
#include "UDP/dialu.h"
unsigned char * destIP;
#endif


void gasp( char *fmt, ... );  /* fatal error handling */


void bytes_usage()
{
printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
switch( dialogue_get_acces() )
   {
   case 'i' :
#ifdef PASS_USB
   printf("---- passerelle USB (UPICf)\n");
   printf("  d : descripteur USB\n");
   printf("  e : test echo USB par UPIC\n");
   printf("---- messages I2C Adr %02x ----\n", dialogue_get_I2Caddr() );
   printf("  txxyyzz : transmettre donnees hexa X Y Z\n");
   printf("  axxyyzz : transmettre donnees hexa X Y Z avec auto checksum\n");
   printf("  r : recevoir donnees/test hexa \n");
#endif
   break;
   case 'p' : 
#ifndef PASS_USB
   printf("---- messages I2C ----\n" );
   printf("  txxyyzz : transmettre donnees hexa X Y Z\n");
   printf("  axxyyzz : transmettre donnees hexa X Y Z avec auto checksum\n");
   printf("  r : recevoir donnees/test hexa \n");
#endif
   break;
   case 'u' :
#ifndef PASS_USB
   destIP = dialugue_get_IP();
   printf("%d.%d.%d.%d, port UDP %d\n",
           destIP[0], destIP[1], destIP[2], destIP[3], dialugue_get_port() );
   printf("  txxyyzz...: transmettre donnees hexa X Y Z .., voir reponse\n");
#endif
   break;
   case 'm' :
#ifndef PASS_USB
   destIP = dialugue_get_IP();
   printf("%d.%d.%d.%d, port UDP %d\n",
           destIP[0], destIP[1], destIP[2], destIP[3], dialugue_get_port() );
   printf("  uXXyyZZ : envoyer bytes X Y Z etc vers l'UART, recevoir..\n");
#endif
   break;
   }
printf("  q : quitter ce menu\n\n");
}

/* ----------------------- code commun pour i et p : commandes a, t, r --------------------- */

void I2C_atr( char locar )
{
int qtxbytes;
unsigned char rxbuf[QIPILOT];
unsigned char txbuf[QIPILOT];
unsigned char rxsum;
unsigned char rxsums[QIPILOT];
int autosum=0; char text[256];
int i, j, hits, val;
unsigned char I2Caddr;

I2Caddr = dialogue_get_I2Caddr();

switch( locar )
     {
     case 'a' : autosum = 1; 
     case 't' : i = 0;
		while ( ( text[i++] = (char) getchar() ) >= '0' )
		   if ( i > 254 ) break;
		text[i] = 0;
		qtxbytes = strlen(text) / 2; // printf("tx %d bytes : _%s_", qtxbytes, text );
		for ( i = 0; i < qtxbytes; i++ )
		    {
		    sscanf( text + i + i, "%2x", &val );
		    txbuf[i] = (unsigned char) val;
		    }
		i = (autosum)?(i2c_put_chk( I2Caddr, txbuf, qtxbytes ))
			     :(i2c_put( I2Caddr, txbuf, qtxbytes ));
		if   ( i )
                     printf(" echec i2c_put (%d)\n", i );
		else printf(" Ok\n");
		autosum = 0;
		break;
     case 'r' : if   ( i2c_get( I2Caddr, rxbuf, QIPILOT ) )
                     printf(" echec\n");
		else {				// on va essayer de deviner la longueur du message...
		     rxsum = 0; printf(" ");
		     for ( i = 0; i < QIPILOT; i++ )
			 {
			 rxsum += rxbuf[i];
			 rxsums[i] = rxsum;
			 }
		     hits = 0;
		     for ( i = 0; i < QIPILOT; i++ )
			 {
			 if ( rxsums[i] == MYRSUM )
			    {
			    hits++;
			    for ( j = 0; j < i; j++ )
				printf("%02X", rxbuf[j] );
			    printf("  sum=%02X\n", rxsums[i] );
			    }
			 }
		     if ( hits == 0 )
			{
			if   ( rxbuf[0] < 0xF0 )
			     {
			     for ( j = 0; j < QIPILOT; j++ )
				 printf("%02X", rxbuf[j] );
			     printf("  no good sum\n" );
			     }
			}
		     if ( rxbuf[0] >= 0xF0 )
			printf("%02X = %s\n", rxbuf[0], ipiloterr(rxbuf[0]) );
		     }
		break;
     } // switch locar
}
/* ------------------------------------------------------------------------- */

void bytes_ui()
{
char locar; int fin=0;

bytes_usage();
switch( dialogue_get_acces() )
   {
   case 'i' :
#ifdef PASS_USB
	while ( ! fin )
	   {
	   locar = getchar();
	   switch(locar)
		{  
		case 'e' : rndtest5(1);			break;
		case 'd' : printf(" lecture device descriptor :\n");
			   readDescriptor( 1, 0 );	break;         
		case 'q' : fin++;			break;
		case ' ' : bytes_usage();		break;
		default  : I2C_atr( locar );
		}
	   } // while fin
#endif
   break;
   case 'p' : 
	while ( ! fin )
	   {
	   locar = getchar();
	   switch(locar)
		{  
		case 'q' : fin++;			break;
		case ' ' : bytes_usage();		break;
		default  : I2C_atr( locar );
		}
	   } // while fin
   break;
   case 'u' :
#ifndef PASS_USB
	{
	irblock irb;
	char text[256];
	int i, val;
	while ( ! fin )
	   {
	   locar = getchar();
	   switch( locar )
		{
		case 't' : i = 0;
			while ( ( text[i++] = (char) getchar() ) >= '0' )
			   if ( i > 254 ) break;
			text[i] = 0;
			irb.txcnt = irb.rxcnt = strlen(text) / 2;
			if ( irb.txcnt > (QIPILOT+4) )
			   { printf("message trop grand\n"); break;  }
			// printf("tx %d bytes : _%s_", qtxbytes, text );
			for ( i = 0; i < irb.txcnt; i++ )
			    {
			    sscanf( text + i + i, "%2x", &val );
			    irb.txbuf[i] = (unsigned char) val;
			    }
			if   ( dial( &irb ) )
			     printf(" echec dialogue\n");
			else {
			     printf(" ");
			     for ( i = 0; i < irb.rxcnt; i++ )
				 printf("%02X", irb.rxbuf[i] );
			     printf("\n");
			     }				break;
		case 'q' : fin++;			break;
		case ' ' : bytes_usage();		break;
		}
	   } // while fin
	}
#endif
   break;
   case 'm' :
#ifndef PASS_USB
	{
	irblock irb;
	char text[256];
	int i, j, val, qtxbytes;
	while ( ! fin )
	   {
	   locar = getchar();
	   switch( locar )
		{
		case 'u' :	i = 0;
			while ( ( text[i++] = (char) getchar() ) >= '0' )
			   if ( i > 254 ) break;
			text[i] = 0;
			qtxbytes = strlen(text) / 2; // printf("tx %d bytes : _%s_", qtxbytes, text );
			if  ( qtxbytes > (QIPILOT+2) )
			    gasp("Too many data bytes %d", qtxbytes );
			for ( i = 0; i < qtxbytes; i++ )
			    {
			    sscanf( text + i + i, "%2x", &val );
			    irb.txbuf[i+2] = (unsigned char) val;
			    }
			irb.txbuf[1] = qtxbytes;
			irb.txbuf[0] = UURW;
			irb.txcnt = irb.rxcnt = QIPILOT+2;	// paquet maxi
			dialu( &irb );
			reinterpret( &irb ); // dialu ne prend pas en charge la verif opcode ipilot
			if ( dialogue_get_verbose() )
			   dial_log( 'U', &irb );
			if   ( irb.rxgot )
			     printf(" echec dialogue\n");
			else {
			     j = irb.rxbuf[1];
			     for ( i = 0; i < j; i++ )
			     printf("%02X ", irb.rxbuf[i+2] );
			     }
			printf("\n");
		break;
		case 'q' : fin++;			break;
		case ' ' : bytes_usage();		break;
		}
	   } // while fin
	}
#endif
   break;
   }
}
