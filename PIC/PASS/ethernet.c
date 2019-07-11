/* Mambo 6 : fonctions reseau, hardware indep. en principe */

#include <string.h>
#include <stdlib.h>
#include <p18cxxx.h>
#include <delays.h>
#include "../fix_var.h"
#include "mymac.h"
#include "fix_var2.h"
#include "offsets.h"
#include "ipsum.h"
#include "ethernet.h"


// static fixed global storage : current packet
#pragma udata eth_dat
#define QPACK 256	// ATTENTION a la compat. avec read_rx_asix() de asix.c
unsigned char packet[QPACK];


#pragma code ipnum_scn
/* fonction reflashable pour adresse IP et fin adresse MAC
   ecrit dans des variables fixes (fix_var.c)
 */
void copy_IP_MAC5()
{
myIP[0] = 192;
myIP[1] = 168;
myIP[2] = 1;
myIP[3] = 80;
myMAC5  = 8;
}

#pragma code jln_lib2

/* fonction pour preparer header ethernet en reponse au packet courant */
void swapMAC( void )
{
packet[DIX_dest]   = packet[DIX_src];
packet[DIX_dest+1] = packet[DIX_src+1];
packet[DIX_dest+2] = packet[DIX_src+2];
packet[DIX_dest+3] = packet[DIX_src+3];
packet[DIX_dest+4] = packet[DIX_src+4];
packet[DIX_dest+5] = packet[DIX_src+5];
// pour 6 bytes, ce code a plat est plus performant que la boucle ...
// memcpypgm2ram( &packet[DIX_src], myMAC, 6 );
packet[DIX_src]   = MYMAC0;
packet[DIX_src+1] = MYMAC1;
packet[DIX_src+2] = MYMAC2;
packet[DIX_src+3] = MYMAC3;
packet[DIX_src+4] = MYMAC4;
packet[DIX_src+5] = myMAC5;	// !! en RAM
}

/* fonction pour preparer packet ARP en reponse au packet courant */
void swapARP( void )
{
packet[ARP_op+1] = 0x02;			// ARP op reply
packet[ARP_thaddr]    = packet[ARP_shaddr];	// target mac address
packet[ARP_thaddr+1]  = packet[ARP_shaddr+1];
packet[ARP_thaddr+2]  = packet[ARP_shaddr+2];
packet[ARP_thaddr+3]  = packet[ARP_shaddr+3];
packet[ARP_thaddr+4]  = packet[ARP_shaddr+4];
packet[ARP_thaddr+5]  = packet[ARP_shaddr+5];
packet[ARP_tipaddr]   = packet[ARP_sipaddr];	// target ip address
packet[ARP_tipaddr+1] = packet[ARP_sipaddr+1];
packet[ARP_tipaddr+2] = packet[ARP_sipaddr+2];
packet[ARP_tipaddr+3] = packet[ARP_sipaddr+3];
packet[ARP_shaddr]    = MYMAC0;		// source mac address
packet[ARP_shaddr+1]  = MYMAC1;
packet[ARP_shaddr+2]  = MYMAC2;
packet[ARP_shaddr+3]  = MYMAC3;
packet[ARP_shaddr+4]  = MYMAC4;
packet[ARP_shaddr+5]  = myMAC5;	// !! en RAM
packet[ARP_sipaddr]   = myIP[0];		// source ip address
packet[ARP_sipaddr+1] = myIP[1];
packet[ARP_sipaddr+2] = myIP[2];
packet[ARP_sipaddr+3] = myIP[3];
}

/* fonction pour preparer header IP en reponse au packet courant */
void swapIP( void )
{
unsigned char hdrlen;

packet[IP_destaddr]   = packet[IP_srcaddr];
packet[IP_destaddr+1] = packet[IP_srcaddr+1];
packet[IP_destaddr+2] = packet[IP_srcaddr+2];
packet[IP_destaddr+3] = packet[IP_srcaddr+3];
packet[IP_srcaddr]   = myIP[0];
packet[IP_srcaddr+1] = myIP[1];
packet[IP_srcaddr+2] = myIP[2];
packet[IP_srcaddr+3] = myIP[3];

//calculate the IP header checksum
packet[IP_hdr_cksum]   = 0x00;
packet[IP_hdr_cksum+1] = 0x00;

hdrlen = ( packet[IP_vers_len] & 0x0F ) * 4;

curIPsum = 0;
IPsum( packet + IP_vers_len, hdrlen );
packet[IP_hdr_cksum+1] = ~(((unsigned char *) &curIPsum)[1]);
packet[IP_hdr_cksum]   = ~( (unsigned char)    curIPsum    );
}

/* fonction pour preparer header UDP en reponse au packet courant
   longueur limitee a 256 bytes */
void swapUDP( void )
{
unsigned char tmp;

tmp                    = packet[UDP_srcport];
packet[UDP_srcport]    = packet[UDP_destport];
packet[UDP_destport]   = tmp;
tmp                    = packet[UDP_srcport+1];
packet[UDP_srcport+1]  = packet[UDP_destport+1];
packet[UDP_destport+1] = tmp;

packet[UDP_cksum]      = 0;
packet[UDP_cksum+1]    = 0;

/* checksum optionnel a calculer en 4 morceaux */
// initial checksum sur zero_pcol
curIPsum = 0x1100;
// checksum sur adresses IP
IPsum( packet + IP_srcaddr, 8 );
// checksum sur longueur udp
IPsum( packet + UDP_len, 2 );
// checksum sur udp header + data
IPsum( packet + UDP_srcport, packet[UDP_len+1] );
packet[UDP_cksum+1] = ~(((unsigned char *) &curIPsum)[1]);
packet[UDP_cksum]   = ~(((unsigned char *) &curIPsum)[0]);
//*/
}


/* traiter un paquet dans le buffer packet[],
   eventuellement preparer reponse dans le meme buffer
   le code retour indique l'action prise :
	- 0  : rien, packet n'est pas pour nous
	- 1  : requete ARP, reponse standard prete a envoyer
	- 2  : requete icmp, reponse standard prete a envoyer
	- 3  : requete UDP, pour le bon port, echo a elaborer et envoyer
	- 88 : requete UDP, pas pour le bon port
	- 89 : requete UDP, erreur checksum
	- FF : non supporte
 */

unsigned char process_pack( void )
{
// test ARP packet, revenir le + vite possible si ce n'est pas pour nous
if ( packet[DIX_type] == 0x08 && packet[DIX_type+1] == 0x06 )
   {
   if   (
	packet[ARP_tipaddr+3] == myIP[3] &&
        packet[ARP_tipaddr+2] == myIP[2] &&
        packet[ARP_op+1]      == 0x01 &&				 // ARP op is request
	packet[ARP_prtype]    == 0x08 && packet[ARP_prtype+1] == 0x00 && // IP protocol
        packet[ARP_hwlen]     == 0x06 && packet[ARP_prlen]    == 0x04 && // MAC and IP adr. len
        packet[ARP_tipaddr+1] == myIP[1] &&
        packet[ARP_tipaddr+0] == myIP[0]
        )
        {	// YES, arp request is for us
        swapMAC();
        swapARP();
        return(1);
        } else return(0);
   } // arp packet processed

// process an IP packet
if ( packet[DIX_type] == 0x08 && packet[DIX_type+1] == 0x00 )
   {
   if ( packet[IP_destaddr]   == myIP[0] &&
        packet[IP_destaddr+1] == myIP[1] &&
        packet[IP_destaddr+2] == myIP[2] &&
        packet[IP_destaddr+3] == myIP[3]
      )
      {
      if   ( packet[IP_proto] == PROT_ICMP )
           {
           unsigned char icmplen;
           if ( packet[ICMP_type] != 0x08 )	// PING request
              return(0xFF);
           swapMAC();
           swapIP();           
           packet[ICMP_type] = 0;		// PING reply
           packet[ICMP_code] = 0;
           // calculate the ICMP checksum
           packet[ICMP_cksum]   = 0;
           packet[ICMP_cksum+1] = 0;
           icmplen = packet[IP_pktlen+1];	// icmplen truncated to 8 bits !
           icmplen -= ( packet[IP_vers_len] & 0x0F ) * 4;
           curIPsum = 0;
           IPsum( packet + ICMP_type, icmplen );
           packet[ICMP_cksum+1] = ~(((unsigned char *) &curIPsum)[1]);
           packet[ICMP_cksum]   = ~( (unsigned char)    curIPsum    );
           return(2);
           } // icmp packet processed
      else if	( packet[IP_proto] == PROT_UDP )
		{
		if ( ( packet[UDP_destport]   != MYUPORT0 ) ||
                     ( packet[UDP_destport+1] != MYUPORT1 )
		   ) return 0x88;

		// verif checksum
		curIPsum = 0x1100;			// initial checksum sur zero_pcol	
		IPsum( packet + IP_srcaddr, 8 );		// checksum sur adresses IP
		IPsum( packet + UDP_len, 2 );		// checksum sur longueur udp
		IPsum( packet + UDP_srcport, packet[UDP_len+1] );	// checksum sur udp header + data
									// udplen truncated to 8 bits !
		if ( curIPsum != 0xFFFF )
	           return(0x89);
		return(3);
		} // udp packet processed
	   else if ( packet[IP_proto] == PROT_TCP )
		   { return(0xFF); } // tcp packet processed
      } // ip packet for us processed
   } // ip packet processed
return(0);
}
