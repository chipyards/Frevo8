/* frevo 7 / Bootloader UDP sur Passerelle SBC18a
	- supporte watchdog
	- slave I2C hard (MSSP) en interrupt prioritaire en 0x08
	- timer en interrupt non prioritaire
	- i2c RX et TX de messages de 1 a 32 byte payload
	- application ASIX / NICholas
 */

#include <string.h>
#include <stdlib.h>
#include <p18cxxx.h>
#include "../i2csla3.h"
#include "../fix_var.h"
#include "../../ipilot.h"
#include "../../version.h"
#include "asix_regs.h"
#include "asix.h"
#include "ethernet.h"
#include "offsets.h"
#include "mymac.h"
#include "fix_var2.h"
#include "../flash.h"
#include "flusk.h"

static unsigned char proc_udp( unsigned char udplen );

/* I2C handshake :
   RX :
	- main initialise checksum
	- irq met rxcnt a FF pendant reception
	- irq met rxcnt a jour quand reception finie (rxcnt checksum inclus)
	  (mais le maitre peut ecraser ces donnees inopportunement)
	- main met rxcnt a zero apres lecture data et checksum
   TX : 
	- main met
		- echo opcode ou errcode dans txbuf[0],
		- txcnt a jour (checksum non inclus)
		- init checksum
	- master peut lire les data une fois (les suivantes, checksum sera invalide)
 */ 


#pragma code jln_lib2

/* ----------------------- initialisations ----------------------- */

static void boardinit( void )
{
/*  RA0, RA1, RA2, RA3, RA5 analog in; RA4 digital out (LED) */
TRISA = 0b00101111;
/*  data bus */
TRISB = 0b11111111;
/*  kbd cols, I2C, UART */
TRISC = 0b11111111;
// ADR bus : RD[4:0]
TRISD = 0b11100000;
/* config port
   r0--aaaa  r = right justify, 0 = Fosc/32, aaaa = port config
   aaaa = 0100 ==> AN0, AN1,      AN3 analog (not AN2 !!)
   aaaa = 0010 ==> AN0, AN1, AN2, AN3, AN4 analog
   aaaa = 1001 ==> AN0, AN1, AN2, AN3, AN4, AN5 analog
                   RA0  RA1  RA2  RA3  RA5  RE0
 */
ADCON1      = 0b10001001;	// necessaire pour utiliser RE2 en input
}


void fluskloop( void );

// bootloader flusk : on peut y venir directement du reset sans
// passer par main()
// ( fonction sans retour )
void flusk( void )
{
_asm	// minimal startup
    lfsr 1, _stack
    lfsr 2, _stack
    clrf TBLPTRU, 0
_endasm
		// _RI _TO _PD _POR _BOR set only by soft or POR (not by button !)
reset_status  =	RCON & 0x1F;
	        // STKOVF, STKUNF cleared only by soft or POR
reset_status |=	STKPTR & 0xC0;

boardinit();
LATAbits.LATA4 = 1;	// led off

// initialisation I2C
istatus = 0;
rxcnt = 0; rxsum = 0;
txcnt = 1; txbuf[0] = XOFF;
i2csla_init();

// initialisation ethernet
err_status = 0;
copy_IP_MAC5();
asix_init1( myMAC5 );
asix_fdx();
asix_start();
_asm
    goto fluskloop
_endasm
}


void fluskloop(void)
{
// copies of ASIX registers
unsigned char isr, bnry, cpr;
// ethernet variables
unsigned char len;
unsigned char action;
short long loops_cnt;


loops_cnt = 0;

// rudimentaire interpreteur de commandes selon ipilot.h,
// rend status dans txbuf[0]

while (1)
   {
   SSPADD = MYADDR;

   // |||||||||||||||||||||||||||||||||||||| Tache I2C 
   if ( ( rxcnt != 0 ) && ( rxcnt != 0xFF ) )
      {
      SSPADD = ~MYADDR;		// controle de flux
      txcnt = 1;		// defaut pour error codes
      // pas de defaut pour txbuf[0], devra etre mis a jour dans tous les cas
      if   ( rxsum != MYRSUM )
           txbuf[0] = CHKERR;
      else {
           switch( rxbuf[0] )
              {
	      case SYSVER :	// les 2 opcodes supportes par flask
              case FLASHR :
			INTCONbits.GIEL = 0; // disable low priority interrupt
			_asm CLRWDT _endasm
			_asm goto 0x0010 _endasm // flash access loop (no return)
			break;
	      default     :
			txbuf[0] = SOUNK;
              }
	   } // if rxsum
      txsum = MYTSUM;		// reinitialiser emission
      rxcnt = 0; rxsum = 0;	// reinitialiser reception
      SSPADD = MYADDR;		// fin du controle de flux
      } // tache I2C

   // |||||||||||||||||||||||||||||||||||||| Tache ethernet
   if ( asixIREQ )	// macro definie dans asix.h
   {
   isr = read_asix( ISR );
   if ( isr & OVW )
      {
      err_status |= RINGOVW;
      write_asix( ISR, OVW );
      }
   if ( isr & PRX )
      {
      write_asix( ISR, PRX );
      /* voici ce qui est fait a la lecture de chaque paquet (asix.c) :
	bnry = next_pack - 1;
	if ( bnry < RINGSTART ) // arrive seult si ASIX a mis next_pack = RINGSTART
   	   bnry = RINGSTOP - 1;
	s'il n'y a pas eu de nouveau paquet, cpr est egal a next_pack, alors
		- soit bnry est egal a cpr-1
		- soit bnry = RINGSTOP - 1 et cpr = RINGSTART
        donc  while ( bnry != ( cpr - 1 ) ) peut planter !
	Soit on corrige (cpr-1), soit on compare avec next_pack
      */
      do {
         bnry = read_asix( BNRY );
         cpr  = read_asix( CPRRD ); 	// ASIX has a read_only CPR in page 0

         if ( next_pack == cpr ) break;	// is ring empty ?

         len = read_rx_asix( packet );
         				// analyse protocolaire
         if ( len )
            {
            action = process_pack();
	    switch(action)
		{
		case 1 :
		case 2 : send_tx_asix( packet, len ); break;
		case 3 : // note : UDP length field is UDP header included (8)
			 // so we remove 8 to present the payload only
			 // La decision d'emettre un echo depend de la valeur de retour de proc_udp
			 if ( proc_udp( packet[UDP_len+1]-8 ) )
			    {
			    swapMAC(); swapIP(); swapUDP();
			    send_tx_asix( packet, len );
			    } break;
		// case 0x89 : err_status |= UDPCHK;
		}
            }
         } while ( next_pack != cpr );
      }	// if ( isr & PRX )
   }	// if ( asixIREQ )
   // |||||||||||||||||||||||||||||||||||||| Tache watchdog
   // bloquer le watchdog pendant env. 2000000 tours de boucle de 5 us
   // soit a peu pres 10 s
   loops_cnt++;
   if ( (((char *)&loops_cnt)[2] & 0xE0 ) == 0 )
      {
      _asm CLRWDT _endasm
      }

   } // while(1);
}

#define UBUF (packet+UDP_data)

/* traitement message UDP :
   cette fonction est appelee pour chaque messsage UDP arrive au bon port
   avec un bon checksum.
   Elle engendre eventuellement une reponse de meme longueur
   a envoyer au client, dans ce cas elle rend 1.
 */
static unsigned char proc_udp( unsigned char udplen )
{
fadr = *( (unsigned int *)( UBUF + 1 ) );
switch ( UBUF[0] )
   {
   case FLASHR  :
	  if	( udplen == 9 )
		{ 
		_asm CLRWDT _endasm 
		fread8( fadr, UBUF+1 );
		UBUF[0] = FLASHR;
		}
	  else	UBUF[0] = LENERR;
	  break;
   case FLASHW :
	  if	( udplen == 11 )
		{ 
		_asm CLRWDT _endasm 
	        if ( fadr < MINFADR2 )
		   { UBUF[0] = SFERRA; break; }
	        fwrite8( fadr, UBUF+3 );
		UBUF[0] = FLASHW;
		}
	  else	UBUF[0] = LENERR;
	  break;
   case FERASE :
	  if	( udplen == 3 )
		{ 
		_asm CLRWDT _endasm 
	        if ( fadr < MINFADR2 )
		   { UBUF[0] = SFERRA; break; }
	        fera64( fadr );
		UBUF[0] = FERASE;
		}
	  else	UBUF[0] = LENERR;
	  break;
   case SYSVER :
	  if	( udplen == 5 )
		{
		_asm CLRWDT _endasm 
		UBUF[1] = VERSION; UBUF[2] = SUBVERS;
		UBUF[3] = 'U';	     // role, 'U' = bootloader via UDP
		UBUF[4] = reset_status;
		UBUF[0] = SYSVER;
		}
	  else	UBUF[0] = LENERR;
	  break;
   case UE :
	  _asm CLRWDT _endasm 
	  break;	// simple echo
   default :
	  // _asm RESET _endasm // trop draconien, risque d'echec du download si superviseur actif 
	  return(0);
   } // switch UBUF[0]

return(1);
}
