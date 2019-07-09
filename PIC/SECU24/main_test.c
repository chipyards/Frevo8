// prog pour le PIC 24F avec UART 
//	AIME carte SBC24s

#if defined(__PIC24F__)
    #include <p24fxxxx.h>
#elif defined(__PIC24H__)
    #include <p24hxxxx.h>
#elif defined(__dsPIC30F__)
    #include <p30Fxxxx.h>
#endif

// #define ROLE_EXTORCH
#define ROLE_LPCVD
#define ROLE_LPCVDSW

#include "hard.h"
#include "uart2.h"
#include "timer45.h"
#include "timer2.h"
#include "../../ipilot.h"
#include "../../mpar.h"
#include "../../version.h"

// config section : trifa1, SBC24s ont un quartz 20MHz, Tencor un oscillateur 20MHz

#if defined( __PIC24FJ64GA002__ )
     _CONFIG2(IESO_OFF & FNOSC_PRI & FCKSM_CSDCMD & POSCMOD_HS )
     _CONFIG1(JTAGEN_OFF & ICS_PGx1 & FWDTEN_OFF)        // JTAG off, watchdog timer off
#else
   #error This code is designed for the PIC24FJ64GA002
#endif


// buffer pour commandes venues par UART
unsigned char rxbuf[QMPAR];
unsigned int rxcnt;

// -------------- data global ---------------------------
int misci;

// variables liees a la mesure du temps
long ticks_cnt;			// temps en ticks de 6.4 us, sur 32 bits
long next_sec_ticks;		// instant de la prochaine seconde
#define SEC2TICKS 156250L	// nombre de ticks dans 1 seconde
unsigned long sec_cnt;		// secondes depuis demarrage sur 32 bits
unsigned int tacks_cnt;		// comptage pulses  (frequencemetre)

// variables process
unsigned char step;
unsigned int frePV;		// frequence mesuree
unsigned int freSVmin;		// frequence min
unsigned int freSVmax;		// frequence max
int step_timer;			// decompteur, en s
int validite_step_1;		// ou delai pour allumage flamme
#ifdef ROLE_LPCVDSW
int flow_switch_pol;
int flow_switch_status;
#endif

// fonctions process
void jump( unsigned char new_step )
{
switch( new_step )
   {
   case 1 :	// allumage
	if ( step == 0 )	// autorise seulement depuis zero
	   {
	   step_timer = validite_step_1;
	   step = 1;
	   REL2_BIT = 1;
	   }
	break;
   case 0:
	step = 0; REL2_BIT = 0;
	break;
   default : REL2_BIT = 0;
   }
}

#ifdef ROLE_EXTORCH
void process_sec(void)
{
switch( step )
   {
   case 1 :	// allumage
	if   ( (--step_timer) <= 0 )
	     {
	     step = 0;
	     REL2_BIT = 0;
	     }
	else {
	     if ( frePV > freSVmax )
		{
		step = 2;
		step_timer = 0;
		}
	     }
	break;
   case 2 :	// oxydation
	if ( frePV < freSVmin )
	   {
	   step = 0;
	   REL2_BIT = 0;
	   step_timer = 0;
	   }
	break;
   default : REL2_BIT = 0;
   }
}
#endif

#ifdef ROLE_LPCVD
void process_sec(void)
{
#ifdef ROLE_LPCVDSW
if	( FSW_BIT )
	flow_switch_status = flow_switch_pol ^ 1;
else	flow_switch_status = flow_switch_pol;
#endif
switch	( step )
	{
	case 1 :	// allumage
		if	( (--step_timer) <= 0 )
			{
			step = 0;
			REL2_BIT = 0;
			}
		else	{
			if	(
				( frePV > freSVmax ) || ( frePV < freSVmin )
				#ifdef ROLE_LPCVDSW
				|| ( flow_switch_status == 0 )
				#endif
				)
				{
				REL2_BIT = 0;
				step = 0;
				step_timer = 0;
				}
			}
		break;
	default : REL2_BIT = 0;
	}
}
#endif

// -------------------- main loop --------------------- //
int main( void )
{
int i;

// specifique pour code bootloadable :
INTCON2bits.ALTIVT = 1;
_NSTDIS = 1;	// no nested interrupt please

uart2_baud( 9600 );
uart2_init( ONE_STOP, NOPAR );

REL1_BIT = 0; REL2_BIT = 0;
REL1_TRI = 0; REL2_TRI = 0;

timer45_init();
next_sec_ticks = SEC2TICKS; sec_cnt = 0;
freq_optin_init();
step = 0; step_timer = 0;

#ifdef ROLE_EXTORCH
#define ROLE 'E'	// P mTorr = F Hz = 78125/262144 * FREpv
			// FREpv = 3.36 * F Hz
freSVmin = 4950;	// 1470 Hz
freSVmax = 5940;	// 1767 Hz
validite_step_1 = 25;
#endif

#ifdef ROLE_LPCVD
#define ROLE 'L'
freSVmin = 5;		// 1 mT
freSVmax = 2688;	// 800 mT
validite_step_1 = 18000;	// 5 heures
#ifdef ROLE_LPCVDSW
#undef ROLE
#define ROLE 'W'
FSW_TRI = 1;
flow_switch_pol = 0;
#endif
#endif


i = 0;
while(1)
  {
  // lecture timer principal 32 bits
  ((unsigned int *)&ticks_cnt)[0] = TMR4;
  ((unsigned int *)&ticks_cnt)[1] = TMR5HLD;
  // lecture compteur pour frequencemetre
  tacks_cnt = TMR2;

  // mise a jour des secondes
  if ( ( ticks_cnt - next_sec_ticks ) > 0 )
     {
     next_sec_ticks += (long)SEC2TICKS;
     sec_cnt++;
     freqmeter();
     process_sec();
     }

  if ( uart2_hit() )
     {
     if ( i < QMPAR ) rxbuf[i++] = uart2_getc();
     if   ( i == 1 )	// premier byte = opcode
	  {
	  switch( rxbuf[0] )
	     {
	     case SYSVER :		// reponse style Tencor/KS
		uart2_putc( VERSION );
		uart2_putc( SUBVERS );
		uart2_putc( BETAVER );
		uart2_putc( ROLE );	// E comme extorch, L comme LPCVD
		uart2_putc( rxbuf[0] );	// echo opcode final
		i = 0;			// fini
		break;
	     case SYSTIM :
		uart2_putc( ((unsigned char *)&sec_cnt)[0] );
		uart2_putc( ((unsigned char *)&sec_cnt)[1] );
		uart2_putc( ((unsigned char *)&sec_cnt)[2] );
		uart2_putc( ((unsigned char *)&sec_cnt)[3] );
		uart2_putc( rxbuf[0] );	// echo opcode final
		i = 0;			// fini
		break;
	     case SW_RESET :
		rxcnt = 2;	// vient avec 1 byte de check
		break;
	     case SET_PARA :
		rxcnt = 4;	// vient avec 1 char + 1 int de data
		break;
	     case GET_SSTA :
		uart2_putc( step );
		// premiere mesure	
		uart2_putc( ((unsigned char *)&frePV)[0] );
		uart2_putc( ((unsigned char *)&frePV)[1] );
		// seconde mesure
		uart2_putc( ((unsigned char *)&step_timer)[0] );
		uart2_putc( ((unsigned char *)&step_timer)[1] );
		// autre mesure
		#ifdef ROLE_LPCVDSW
		uart2_putc( flow_switch_status );
		#else
		uart2_putc( 0 );
		#endif
		uart2_putc( 0 );
		uart2_putc( rxbuf[0] );	// echo opcode final
		i = 0;			// fini
		break;
	     case SYSNOP :
		uart2_putc( rxbuf[0] );	// echo opcode immediat
		i = 0;			// reponse immediate finie
		break;
	     default :
		uart2_putc( ERR_OP );
		i = 0;			// reponse immediate finie
	     }	// switch
	  }	// if( i == 0 )
     else if ( i >= rxcnt )
	  {			// message fini, de 0 a (rxcnt-1)
	  switch( rxbuf[0] )
	     {
	     case SW_RESET :
		if   ( rxbuf[1] == (unsigned char)~SW_RESET )
		     {
		     asm("reset");
		     // uart2_putc( rxbuf[0] );	// echo opcode immediat
		     }
		else uart2_putc( ERR_PAR );
		i = 0;			// traitement fini
		break;
	     case SET_PARA :
		((unsigned char *)&misci)[0] = rxbuf[2];
		((unsigned char *)&misci)[1] = rxbuf[3];
		switch( rxbuf[1] )
		   {
		   case 0 : jump( (unsigned char) misci ); break;
		   case 1 : REL1_BIT = (misci & 1)?(1):(0);
			    REL2_BIT = (misci & 2)?(1):(0); break;
		   case 2 : freSVmin = misci; break;
		   case 3 : freSVmax = misci; break;
		   case 4 : validite_step_1 = misci; break;
		   #ifdef ROLE_LPCVDSW
		   case 5 : flow_switch_pol = misci; break;
		   #endif
		   }
		uart2_putc( rxbuf[0] );	// echo opcode immediat
		i = 0;			// traitement fini
		break;
	     default :
		uart2_putc( ERR_OP ); i = 0;
	     }			// switch
	  }		// if (i >= rxcnt )
     }		// if ( uart2_hit() )
  }	// while (1)

}
