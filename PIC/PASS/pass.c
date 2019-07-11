/* frevo 7 / Passerelle SBC18a
	- supporte watchdog
	- slave I2C hard (MSSP) en interrupt prioritaire en 0x08
	- passage en master I2C soft des reception d'une requete UW en UDP
	  (retour au mode slave seulement sur RESET)
	- timer en interrupt non prioritaire
	- i2c RX et TX de messages de 1 a 32 byte payload
	- application ASIX / NICholas
 */

#include <string.h>
#include <stdlib.h>
#include <p18cxxx.h>
#include "../i2csla3.h"
#include "../tmr0_uart.h"
#include "../fix_var.h"
#include "../../ipilot.h"
#include "../../version.h"
#include "asix_regs.h"
#include "asix.h"
#include "ethernet.h"
#include "offsets.h"
#include "mymac.h"
#include "../i2cmast_soft.h"
#include "fix_var2.h"
#include "flusk.h"

// variables liees a la mesure du temps
short long ticks_cnt;		// temps en ticks de 12.8 us, sur 24 bits
#define SEC2TICKS 78125		// nombre de ticks dans 1 seconde
unsigned long sec_cnt;		// secondes depuis demarrage sur 32 bits

unsigned int blink_pat = 0x8000;	// motif de clignotement de la LED systeme
unsigned char pass_lock;		// timer lock passerelle UDP-I2C
					// 0 <==> pas de lock, sinon duree restante en tonk
#define DURLOCK 14			// valeur init pass_lock en tonks


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


#pragma code jln_lib3

/* ----------------------- initialisations ----------------------- */

void boardinit( void )
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

static unsigned char traite_udp( unsigned char udplen );

/* ----------------------- main ------------------- */

void main(void)
{
unsigned char opcode;
static short long next_sec_ticks;
static unsigned char tonker, oldtonker;
#define TONKMASK 0xE0			// 1 tonk = env. 104.8 ms
static unsigned char blink_index;
static unsigned int blink_piso;
// copies of ASIX registers
unsigned char isr, bnry, cpr;
// ethernet variables
unsigned char len;
unsigned char action;

		// _RI _TO _PD _POR _BOR set only by soft or POR (not by button !)
reset_status  =	RCON & 0x1F;
	        // STKOVF, STKUNF cleared only by soft or POR
reset_status |=	STKPTR & 0xC0;

boardinit();

// initialisation timer
tmr0init(); 
tmr0_roll = 0; next_sec_ticks = SEC2TICKS; sec_cnt = 0;
blink_index = 16; blink_piso = blink_pat;

// initialisation I2C
istatus = 0;
rxcnt = 0; rxsum = 0;
txcnt = 1; txbuf[0] = XOFF;
i2csla_init();

// initialisation ethernet
err_status = 0; pass_lock = 0;
copy_IP_MAC5();
asix_init1( myMAC5 );
asix_fdx();
if ( ( myMAC5 & 0x80 ) == 0 )
   mii_disable100();
asix_start();

// initialisation UART
uart_init();

// rudimentaire interpreteur de commandes selon ipilot.h,
// rend status dans txbuf[0]

while (1)
   {
   _asm CLRWDT _endasm
   SSPADD = MYADDR;
   // |||||||||||||||||||||||||||||||||||||| Tache timer
   ((unsigned char *)&ticks_cnt)[2] = tmr0_roll;
   // lire TMR0L en premier, cela latche TMR0H  !
   ((unsigned char *)&ticks_cnt)[0] = TMR0L;
   ((unsigned char *)&ticks_cnt)[1] = TMR0H;
   // si jamais on a ete interrompu, recommencer
   if ( ((unsigned char *)&ticks_cnt)[2] != tmr0_roll )
      {
      ((unsigned char *)&ticks_cnt)[2] = tmr0_roll;
      // lire TMR0L en premier, cela latche TMR0H  !
      ((unsigned char *)&ticks_cnt)[0] = TMR0L;
      ((unsigned char *)&ticks_cnt)[1] = TMR0H;
      }
   if ( ( ticks_cnt - next_sec_ticks ) > 0 )
      {
      next_sec_ticks += (short long)SEC2TICKS;
      sec_cnt ++;
      }
   tonker = (((unsigned char *)&ticks_cnt)[1]) & TONKMASK;

   // |||||||||||||||||||||||||||||||||||||| tache tonk (blink+lock)
   if ( tonker != oldtonker )	// every 104 ms
      {

      // |||||||||||||||||||||||||| tache blink-the-LED
      if   ( --blink_index == 0 )
	   {
	   blink_index = 16;
	   if ( err_status )	// only cleared by reset & SYSVER
	      {
	      if ( err_status & RINGLOST )
	         blink_pat = 0xFFFE;
	      if ( err_status & RINGOVW )
	         ((unsigned char *)&blink_pat)[1] |= 0x0E;
	      if ( err_status & BIGRX )
	         ((unsigned char *)&blink_pat)[0] |= 0xE0;
	      }

	   /* aff pass lock pour debug, marche seulement si DURLOK >> 16 *
	   if   ( pass_lock )
	        ((unsigned char *)&blink_pat)[1] |= 0x20;
	   else ((unsigned char *)&blink_pat)[1] &= ~0x20;
	   //*/
	   blink_piso = blink_pat;
	   }
      else { blink_piso <<= 1;  }
 
      if   ( (((unsigned char *)&blink_piso)[1]) & 0x80 )
	   LATAbits.LATA4 = 0;
      else LATAbits.LATA4 = 1;

      // ||||||||||||||||||||||||||||||| tache verrouillage passerelle
      if ( pass_lock )
         pass_lock--;

      oldtonker = tonker;
      } // if tonker


   // |||||||||||||||||||||||||||||||||||||| Tache I2C 
   if ( ( rxcnt != 0 ) && ( rxcnt != 0xFF ) )
      {
      SSPADD = ~MYADDR;		// controle de flux
      txcnt = 1;		// defaut pour error codes
      // pas de defaut pour txbuf[0], devra etre mis a jour dans tous les cas
      if   ( rxsum != MYRSUM )
           txbuf[0] = CHKERR;
      else {
           opcode = rxbuf[0];
           if   ( !( opcode & 0x80 ) )
                {			// opcode long
                switch ( opcode )
                   {
	           case SYSTIM :
			    if   ( rxcnt == 2 )
			 	 { 
				 txbuf[1] = ((unsigned char *)&sec_cnt)[0];
				 txbuf[2] = ((unsigned char *)&sec_cnt)[1];
				 txbuf[3] = ((unsigned char *)&sec_cnt)[2];
				 txbuf[4] = ((unsigned char *)&sec_cnt)[3];
				 txbuf[0] = opcode; txcnt = 5;
				 }
			    else txbuf[0] = LENERR;
                            break;
             	   case FLASHR :
			    if   ( rxcnt == 4 )
				 {
				 INTCONbits.GIEL = 0; // disable low priority interrupt
				 _asm CLRWDT _endasm
				 _asm goto 0x0010 _endasm // flash access loop (no return)
				 }
			    break;
		   case SYSVER :
			    if   ( rxcnt == 2 )
				 {
				 txbuf[1] = VERSION; txbuf[2] = SUBVERS;
				 txbuf[3] = 'P';	     // role, 'P' = passerelle
				 txbuf[4] = reset_status;
				 err_status = 0;
				 txbuf[0] = opcode; txcnt = 5;
				 }
			    else txbuf[0] = LENERR;
			    break;
                   default     :
                            txbuf[0] = SOUNK;
                   } // switch opcode long 
                } // if opcode long
           else {			// opcode court
                txbuf[0] = SOUNK;
                }
	   } // if rxsum
      txsum = MYTSUM;		// reinitialiser emission
      rxcnt = 0; rxsum = 0;	// reinitialiser reception
      SSPADD = MYADDR;		// fin du controle de flux
      } // tache I2C

   // |||||||||||||||||||||||||||||||||||||| Tache ethernet
   if ( asixIREQ )
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
			 if ( traite_udp( packet[UDP_len+1]-8 ) )
			    {
			    swapMAC(); swapIP(); swapUDP();
			    send_tx_asix( packet, len );
			    } break;
		case 0x89 : err_status |= UDPCHK;
		}
            }
         } while ( next_pack != cpr );
      }	// if ( isr & PRX )
   }	// if ( asixIREQ )

   } // while(1);
}

#define I2C_slave_adr 0x58
#define UBUF (packet+UDP_data)

/* traitement message UDP :
   cette fonction est appelee pour chaque messsage UDP arrive au bon port
   avec un bon checksum.
   Elle engendre eventuellement une reponse de meme longueur
   a envoyer au client, dans ce cas elle rend 1.
 */
static unsigned char traite_udp( unsigned char udplen )
{
switch ( UBUF[0] )
   {
   case UE : break;	// simple echo a la Mambo 7
   case UW :	// requete I2C write
     {
     i2csla_stop();  // on ne repondra plus au maitre I2C jusqu'au prochain RESET
     if ( udplen >= 3 )
	{
	if   ( pass_lock )
	     UBUF[0] = UL; 	// Lock
        else {
             if   ( i2c_put( I2C_slave_adr, UBUF+1, udplen-1 ) == 0 )
	          {
                  UBUF[0] = UA;	// Ack
	          pass_lock = DURLOCK;
	          }
             else UBUF[0] = UN; 	// Nack
             }
	} // if udplen >= 3
     } break;
   case UR :	// requete I2C read
     {
     i2csla_stop();
     if ( udplen >= 3 )
        {
	pass_lock = 0;
        if   ( i2c_get( I2C_slave_adr, UBUF+1, udplen-1 ) == 0 )
             UBUF[0] = UA;	// Ack
        else UBUF[0] = UN; 	// Nack
        }
     } break;
   case UURW :	// requete UART bidirectionnelle
     {
     if   ( udplen >= 3 )
	  {
	  unsigned char tmpi;
	  // emettre
	  tmpi = UBUF[1];
	  if ( tmpi )
	     uart_putn( (char *)UBUF+2, tmpi );
	  // recevoir
	  tmpi = 2;
	  // on en met tant qu'il y en a,
	  // mais pas plus qu'il ne peut en rentrer
	  while	( ( uart_hit() ) && ( tmpi < udplen ) )
		{
		UBUF[tmpi] = uart_getc();
		tmpi++;
		}
	  UBUF[1] = tmpi - 2;
	  UBUF[0] = UURW;
	  }
     else UBUF[0] = LENERR;
     } break;
   case SYSTIM :
     {
     if   ( udplen == 5 )
	  { 
	  UBUF[1] = ((unsigned char *)&sec_cnt)[0];
	  UBUF[2] = ((unsigned char *)&sec_cnt)[1];
	  UBUF[3] = ((unsigned char *)&sec_cnt)[2];
	  UBUF[4] = ((unsigned char *)&sec_cnt)[3];
	  UBUF[0] = SYSTIM;
	  }
     else UBUF[0] = LENERR;
     } break;
   case SYSVER :
     {
     if   ( udplen == 6 )
	  {
	  UBUF[1] = VERSION; UBUF[2] = SUBVERS;
	  UBUF[3] = 'P';	     // role, 'P' = passerelle
	  UBUF[4] = reset_status;
	  err_status = 0;
	  UBUF[5] = BETAVER;
	  UBUF[0] = SYSVER;
	  }
     else UBUF[0] = LENERR;
     } break;
   case FLASHR  :
     {
     if	  ( udplen == 9 )
	  { 
	  INTCONbits.GIEL = 0; // disable low priority interrupt
	  _asm CLRWDT _endasm 
	  _asm goto fluskloop _endasm
	  }
     else UBUF[0] = LENERR;
     } break;
   default : return(0);
   } // switch UBUF[0]

return(1);
}
