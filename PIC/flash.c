// Frevo 7
// lecture/ecriture memoire flash programme PIC18F
// adresses 16 bits

#include <p18cxxx.h>
#include "i2csla3.h"
#include "../ipilot.h"
#include "../version.h"
#include "flash.h"
#include "fix_var.h"

#pragma code jln_lib1

void flaskloop( void );

// bootloader flask : on peut y venir directement du reset sans
// passer par main()
// ( fonction sans retour )
void flask( void )
{
_asm	// minimal startup
    lfsr 1, _stack
    lfsr 2, _stack
    clrf TBLPTRU, 0
    goto flaskloop
_endasm
}

// interpreteur de commandes selon ipilot.h,
// - rend status dans txbuf[0]
// - buffers provisoirement dans la pile
// - on ne revient pas de cette fonction,
//   on reste dans cette boucle jusqu'a :
//	WDT reset ou
//	arrivee d'une requete non supportee

void flaskloop( void )
{
// initialisation I2C  : seulement en cas de reset (hard, soft ou WDT)
// ainsi on ne perd pas le message en cours si on vient de main()
if ( ( INTCONbits.GIEH == 0 ) || ( PIE1bits.SSPIE == 0 ) )
   {
   istatus = 0;
   rxcnt = 0; rxsum = 0;
   txcnt = 1; txbuf[0] = XOFF;
   i2csla_init();
   }

		// _RI _TO _PD _POR _BOR set only by soft or POR (not by button !)
reset_status  =	RCON & 0x1F;
	        // STKOVF, STKUNF cleared only by soft or POR
reset_status |=	STKPTR & 0xC0;

TRISAbits.TRISA4 = 0;	// sys LED
LATAbits.LATA4 = 0;


while (1)
   {
   if ( ( rxcnt != 0 ) && ( rxcnt != 0xFF ) )
      {
      SSPADD = ~MYADDR;		// controle de flux
      txcnt = 1;		// defaut pour error codes
      // pas de defaut pour txbuf[0], devra etre mis a jour dans tous les cas
      if   ( rxsum != MYRSUM )
           {
           txbuf[0] = CHKERR;
           }
      else {
           fadr = *( (unsigned int *)( rxbuf + 1 ) );
           switch( rxbuf[0] )
              {
              case FLASHR  :
			if   ( rxcnt == 4 )
			     { 
			     _asm CLRWDT _endasm 
			     fread8( fadr, txbuf+1 );
		             txbuf[0] = FLASHR; txcnt = 9;
			     }
			else txbuf[0] = LENERR;
			break;
	      case FLASHW :
			if   ( rxcnt == 12 )
			     { 
			     _asm CLRWDT _endasm 
	                     if ( fadr < MINFADR )
		                { txbuf[0] = SFERRA; txcnt = 1; break; }
	                     fwrite8( fadr, rxbuf+3 );
		             txbuf[0] = FLASHW; txcnt = 1;
			     }
			else txbuf[0] = LENERR;
			break;
	      case FERASE :
			if   ( rxcnt == 4 )
			     { 
			     _asm CLRWDT _endasm 
	                     if ( fadr < MINFADR )
		                { txbuf[0] = SFERRA; txcnt = 1; break; }
	                     fera64( fadr );
			     txbuf[0] = FERASE; txcnt = 1;
			     }
			else txbuf[0] = LENERR;
			break;
	      case SYSVER :
			if   ( rxcnt == 2 )
			     {
			     _asm CLRWDT _endasm 
			     txbuf[1] = VERSION; txbuf[2] = SUBVERS;
			     txbuf[3] = 'B';		 // role, B = bootloader
			     txbuf[4] = reset_status;
		             txbuf[0] = SYSVER; txcnt = 5;
			     }
			else txbuf[0] = LENERR;
			break;
              default     :
			_asm RESET _endasm
              } // switch
	   } // if rxsum
      txsum = MYTSUM;		// reinitialiser emission
      rxcnt = 0; rxsum = 0;	// reinitialiser reception
      SSPADD = MYADDR;		// fin du controle de flux
      } // if rxcnt
   } // while
}
