/* frevo8.1 / SBC18b
	- supporte watchdog
	- repond aux commandes IPILOT FLASHR, SYSVER, SYSTIM, SYSDBG et
	  FLASHW, FERASE juste apres FLASHR
	- plus PSTEP, PMANU, PFULL, VAON, VAOFF, DAC
	- plus TCONS : pilotage temperatures
	- plus RS485T, RS485R : mode rs485 transparent
	- mesure frequence sur pin RC0 pour interface Baratron (-->PFULL)
	- execution recette chargee au moyen de SYSDBG
	- 5 modules optionnels :
		- LEDS_RUN_PAUSE	Leds sur sorties electrovannes
		- BOUTONS_ANALOG	boutons avec resistances sur adc canal 4
		- AUDIO			Audio sur sortie electrovanne
		- SECU_PSIH4		Limitation pression SiH4
		- SECU_VSIH4		interlock vannes SiH4 pour tube 4.3 (A COMPLETER)
	  la lettre ROLE resume le choix des modules
	  (bienque le frequencemetre ne soit pas optionnel)
		'F' = Automate avec V/F
		'S' = Automate avec V/F + securites SiH4 tube 4.3
		'O' = Automate avec V/F + securites SiH4 tube 3.1
	- vanne 0 est active au repos (new v8.0s, cf recipe.h et recipe.c, #define VANNE0)
 */

#define LEDS_RUN_PAUSE
// #define BOUTONS_ANALOG
#define AUDIO
#define SECU_PSIH4
// #define SECU_VSIH4
#define ROLE 'O'

#include <string.h>
#include <stdlib.h>
#include <p18cxxx.h>
#include <delays.h>
#include "../i2csla3.h"
#include "tmr0_rs485.h"
#include "omron.h"
#include "../fix_var.h"
#include "../../ipilot.h"
#include "../../version.h"
#include "../i2cmast_soft.h"
#include "tmr1.h"
#include "recipe.h"

#pragma code jln_lib3


/* -------------- variables liees a la mesure du temps ------------ */

#pragma udata access accessram

// variables liees a la mesure du temps
near short long ticks_cnt;		// temps en ticks de 12.8 us, sur 24 bits
#define SEC2TICKS 78125		// nombre de ticks dans 1 seconde
near unsigned long sec_cnt;		// secondes depuis demarrage sur 32 bits
near unsigned int tacks_cnt;		// comptage pulses sur pin RC0 (frequencemetre)

#pragma udata grp1

unsigned int blink_pat = 0x8000;	// motif de clignotement de la LED systeme
#define ADC_INTERVAL 16			// intervalle entre lecture de 2 ADCs, en tinks

unsigned char new_sec;			// flag "nouvelle seconde"


/* --------------------- variables process ------------------ */

// les Process Objects
#pragma udata access accessram
near STEP_PO etape;
near EXTRACTOR extrac;

#pragma udata grp1

MFC_PO mfc[QMFC];
TEM_PO tem[QTEM];
MFC_PO fre;

unsigned int adc4PV;		// adc canal 4 (eventuellement boutons analog)


/* ----------------- variables des modules optionnels ---------------- */

#ifdef LEDS_RUN_PAUSE	//-----	// LEDs de signalisation sur sorties vannes
#define RUN_SIG  6	// PORT D
#define HOLD_SIG 7	// PORT D
#define END_SIG  8	// PORT B	geree par recette
#define END_BIT  LATBbits.LATB0	// <==> END_SIG
#endif

#ifdef BOUTONS_ANALOG	//-----	// boutons de la face avant
unsigned char curbut = 0;
unsigned char ibut;
#define START_BUT 1
#define TERM_BUT  2 
#define CONT_BUT  3
#define PREP_BUT  4
// anti-rebond rudimentaire pour bouton analog
unsigned char andebou( unsigned char val );	// val sur 8 bits 
#endif

#ifdef AUDIO 
#define AUDIO_BIT LATBbits.LATB7	// PORT B MSB = vanne 15
unsigned int beep_pat  = 0x0000;	// motif de beep du HP systeme
unsigned char audio_per_l = 8;
unsigned char audio_per_h = 2;
unsigned char audio_pers[16] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
#define ALM_BEEP_PAT 0xFFFE		// alarme quelconque
#define END_BEEP_PAT 0x0501		// fin recette (si END_BIT)
#endif

// vannes du four 4.3 - incompletement implemente
#define V_SIH4EVT  9	// sera peut etre supprimee
#define V_SIH4SRC 14
// pressions pour fours LPCVD
// pression maxi pour admission SiH4 (en unites FREpv)
// P mTorr = F Hz = 78125/262144 * FREpv
// FREMAX est utilise en 2 endroits :
//	- juste apres acquisition pression pour fermer SiH4
//	- dans vanne() pour inhiber ouverture
#define FREMAX	3355	// 1000 mTorr
// pression maxi pour ouverture VV
#define FREVV	36000	// 10.7 Torr


/* ----------------------- initialisations ----------------------- */

void boardinit( void )
{
/*  RA0, RA1, RA2, RA3, RA5 analog in; RA4 digital out (LED) */
TRISA = 0b00101111;
/* ports B et D : digital out pour electrovannes (lsb = D0) */
LATD = VANNE0; LATB = 0; 
TRISB   = 0b00000000;
TRISD   = 0b00000000;
/* I2C hard et soft , UART, timer1 */
TRISC = 0b11111111;
}

/* --------------------- fonctions process ------------------ */

void vanne( unsigned char val, unsigned char devadr )
{
unsigned char thebit = ( 1 << ( devadr & 7 ) );

// ---------------- securites specifiques tubes LPCVD incompletement implemente
#ifdef SECU_PSIH4 
if ( ( devadr == V_SIH4SRC ) && ( val == 1 ) )
   {
   if ( fre.PV > FREMAX )
      return;
   #ifdef SECU_VSIH4
   vanne( 0, V_SIH4EVT );
   #endif
   } 
#endif

#ifdef SECU_VSIH4
if ( ( devadr == V_SIH4EVT ) && ( val == 1 ) )
   {
   vanne( 0, V_SIH4SRC );
   } 
#endif
// ---------------- fin securites specifiques des tubes LPCVD

if   ( devadr & 8 )
     if   ( val )
          LATB |=  thebit;
     else LATB &= ~thebit; 
else if   ( val )
          LATD |=  thebit;
     else LATD &= ~thebit; 
}

/* envoyer SV au DAC */
unsigned char sendDAC( unsigned char devadr )
{
static unsigned char vbuf[2];
if ( devadr >= QMFC )
   return(255);

// permuter les bytes, DAC AD5321 est big-endian...
vbuf[0] = ( ( unsigned char *)( &mfc[devadr].SV ) )[1];
vbuf[1] = ( ( unsigned char *)( &mfc[devadr].SV ) )[0];

// decaler de 4 bits a droite, en big-endian
vbuf[1] >>= 4;
vbuf[1] |= ( vbuf[0] << 4 );
vbuf[0] >>= 4; 	//(PD0=PD1=0)

/* adresse du device = 7 bits (suivis plus tard de R/Wbar)
   pour AD5321 : 00011_A1_A0 */
return( i2c_put( 0x0C | devadr, vbuf, 2 ) );
}

/* preparer reponse a la commande PFULL */
void pfull( void )
{
txbuf[1]  = etape.id;
txbuf[2]  = LATD; 
txbuf[3]  = LATB;
txbuf[4]  = ( ( unsigned char *) &etape.chrono )[0];
txbuf[5]  = ( ( unsigned char *) &etape.chrono )[1];
txbuf[6]  = etape.flags;

// les donnees qui vont suivre vont etre considerees comme
// 12 bits left_justified dans 2 bytes
// SAUF fre.PV qui est envoye sur 16 bits

txbuf[7]   = ( ( unsigned char *) &mfc[0].SV )[0];
txbuf[8]   = ( ( unsigned char *) &mfc[0].SV )[1];
*( (unsigned int *)&txbuf[7] ) >>= 4;
txbuf[8]  |= ( ( unsigned char *) &mfc[0].PV )[0] & 0xF0; 
txbuf[9]   = ( ( unsigned char *) &mfc[0].PV )[1]; 

txbuf[10]  = ( ( unsigned char *) &mfc[1].SV )[0];
txbuf[11]  = ( ( unsigned char *) &mfc[1].SV )[1];
*( (unsigned int *)&txbuf[10] ) >>= 4;
txbuf[11] |= ( ( unsigned char *) &mfc[1].PV )[0] & 0xF0; 
txbuf[12]  = ( ( unsigned char *) &mfc[1].PV )[1]; 

txbuf[13]  = ( ( unsigned char *) &mfc[2].SV )[0];
txbuf[14]  = ( ( unsigned char *) &mfc[2].SV )[1];
*( (unsigned int *)&txbuf[13] ) >>= 4;
txbuf[14] |= ( ( unsigned char *) &mfc[2].PV )[0] & 0xF0; 
txbuf[15]  = ( ( unsigned char *) &mfc[2].PV )[1]; 

txbuf[16]  = ( ( unsigned char *) &mfc[3].SV )[0];
txbuf[17]  = ( ( unsigned char *) &mfc[3].SV )[1];
*( (unsigned int *)&txbuf[16] ) >>= 4;
txbuf[17] |= ( ( unsigned char *) &mfc[3].PV )[0] & 0xF0; 
txbuf[18]  = ( ( unsigned char *) &mfc[3].PV )[1]; 

// ici l'exception
txbuf[19] = 0;
txbuf[20] = ( ( unsigned char *) &fre.PV  )[0];
txbuf[21] = ( ( unsigned char *) &fre.PV  )[1];
// fre.PV apparait sur 12 bits dans le second canal aux,
// ses 4 LSBs apparaissent comme MSBs du premier aux...

txbuf[22]  = ( ( unsigned char *) &tem[0].SV )[0];
txbuf[23]  = ( ( unsigned char *) &tem[0].SV )[1];
*( (unsigned int *)&txbuf[22] ) >>= 4;
txbuf[23] |= ( ( unsigned char *) &tem[0].PV )[0] & 0xF0; 
txbuf[24]  = ( ( unsigned char *) &tem[0].PV )[1]; 

txbuf[25]  = ( ( unsigned char *) &tem[1].SV )[0];
txbuf[26]  = ( ( unsigned char *) &tem[1].SV )[1];
*( (unsigned int *)&txbuf[25] ) >>= 4;
txbuf[26] |= ( ( unsigned char *) &tem[1].PV )[0] & 0xF0; 
txbuf[27]  = ( ( unsigned char *) &tem[1].PV )[1]; 

txbuf[28]  = ( ( unsigned char *) &tem[2].SV )[0];
txbuf[29]  = ( ( unsigned char *) &tem[2].SV )[1];
*( (unsigned int *)&txbuf[28] ) >>= 4;
txbuf[29] |= ( ( unsigned char *) &tem[2].PV )[0] & 0xF0; 
txbuf[30]  = ( ( unsigned char *) &tem[2].PV )[1]; 
}


/* ----------------------- main loop ------------------------ */
extern unsigned char pack[];

#pragma udata access accessram

void main(void)
{

static near unsigned char opcode, devadr;

near static short long next_sec_ticks;		// 1 tick = 12.8 us
near static unsigned char tinker, oldtinker;	// detection des multiples de 8 ticks
#define TINKMASK 0xF8			// 1 tink = 102.4 us
near static unsigned char tonker, oldtonker;
#define TONKMASK 0xE0			// 1 tonk = env. 104.8 ms
near static unsigned char tonk16;		// compteur tonks modulo 16 --> periode 1.68 s

near static unsigned char audio_tinks;
near static unsigned int blink_piso, beep_piso;

near static unsigned char ADC_index;
near static unsigned char ADC_conv, ADC_tinks;

		// _RI _TO _PD _POR _BOR set only by soft or POR (not by button !)
reset_status  =	RCON & 0x1F;
	        // STKOVF, STKUNF cleared only by soft or POR
reset_status |=	STKPTR & 0xC0;

boardinit();

// initialisation timer
tmr0init(); 
tmr0_roll = 0; next_sec_ticks = SEC2TICKS; sec_cnt = 0;;
ADC_index = 0;
tonk16 = 0; 
blink_piso = blink_pat;
audio_tinks = 1;

// initialisation I2C
istatus = 0;
rxcnt = 0; rxsum = 0;
txcnt = 1; txbuf[0] = XOFF;
i2csla_init();

// initialisation RS485
uartinit();

// initialisation frequencemetre
tmr1init(); 

// initialisation process
etape.chrono = 0; step_zero();
init_tem();
load_def();

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
      ((unsigned char *)&ticks_cnt)[0] = TMR0L;
      ((unsigned char *)&ticks_cnt)[1] = TMR0H;
      }
   // pre-lecture timer 1 pour frequencemetre
   // lire TMR1L en premier, cela latche TMR1H  !
   ((unsigned char *)&tacks_cnt)[0] = TMR1L;
   ((unsigned char *)&tacks_cnt)[1] = TMR1H;
   if ( ( ticks_cnt - next_sec_ticks ) > 0 )
      {
      next_sec_ticks += (short long)SEC2TICKS;
      sec_cnt ++;
      if ( ( etape.id ) && ( etape.flags == 0 ) )
	 etape.chrono += 1;
      new_sec = 1;
      }
   tinker = (((unsigned char *)&ticks_cnt)[0]) & TINKMASK;

   // |||||||||||||||||||||||||||||||||||||| Taches tink - haute priorite mais duree breve
   if   ( tinker != oldtinker )
        {
	// |||||||||||||||||||||||||| tache beep
	#ifdef AUDIO
	if   ( (((unsigned char *)&beep_piso)[1]) & 0x80 )
             {
	     audio_tinks--;
	     if ( audio_tinks == 0 )
	        {
	        if   ( AUDIO_BIT )
                     {
		     AUDIO_BIT = 0;
		     audio_tinks = audio_per_l;
		     }
	        else {
		     AUDIO_BIT = 1;
		     audio_tinks = audio_per_h;
		     }
	        }
	     }
	#endif

	// |||||||||||||||||||||||||| tache ADC
	/* les conversions se font a intervalle constant ADC_INTERVAL
	   tout le temps qui n'est pas utilise par la conversion proprement dite (SAR)
	   est utilise pour l'acquisition du canal suivant
	 */
	ADC_tinks--;
	if   ( ADC_conv )
	     {
	     if ( ADCON0bits.GO == 0 )
		{
		if   ( ADC_index < 4 )	// cas des MFCs
		     {
		     ( ( unsigned char *) &mfc[ADC_index].PV )[0] = ADRESL;
		     ( ( unsigned char *) &mfc[ADC_index].PV )[1] = ADRESH;
		     }
		else {			// canal 4 (eventuellement boutons analog)
		     ( ( unsigned char *) &adc4PV )[0] = ADRESL;
		     ( ( unsigned char *) &adc4PV )[1] = ADRESH;
		     }
		ADC_conv = 0;
		// conversion d'un canal est finie, commencer l'acquisition du suivant
		ADC_index ++;
		if ( ADC_index >= 5 )	// 5 canaux !
		   ADC_index = 0;
		/* config port
		   r0--aaaa  r = right justify, 0 = Fosc/32, aaaa = port config
		   aaaa = 0100 ==> AN0, AN1,      AN3 analog (not AN2 !!)
		   aaaa = 0010 ==> AN0, AN1, AN2, AN3, AN4 analog
		   aaaa = 1001 ==> AN0, AN1, AN2, AN3, AN4, AN5 analog
		                   RA0  RA1  RA2  RA3  RA5  RE0
		 */
		ADCON1      = 0b00001001;	// left, 6 analog inputs
		/* Select ANn channel, Fosc/32 clock 
		   10cccg-o  ccc = channel, G = go, o = adc on
		   10 = Fosc/32 ==> Tad = 50ns * 32 = 1.6us exactement ok */
		ADCON0 = ( ADC_index << 3 ) | 0b10000001;
		}
	     }
	else if   ( ADC_tinks == 0 )	// acquisition terminee, lancer conversion
		  {
		  ADC_tinks = ADC_INTERVAL;	// comprend conversion + acquisition
		  ADC_conv = 1;
		  ADCON0bits.GO = 1;
		  }
	     // |||||||||||||||||||||||||| tache CRC (plus basse priorite)
	     else {
		  if ( extrac.crc_stat & CRC_CALC )
		  crc_cont();
		  }

	oldtinker = tinker;
        } // if tinker
   else tonker = (((unsigned char *)&ticks_cnt)[1]) & TONKMASK;

   // |||||||||||||||||||||||||||||||||||||| Tache I2C - quasi asynchrone
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
		   case SYSDBG :
			    switch( rxbuf[1] )
			      {
			      case 'i' :	// [SYSDBG]['i'][ADRL][ADRH][cnt]
				txcnt = rxbuf[4];
				if ( txcnt <= MAXDNLOAD )
				   memcpy( (void *)txbuf+1, *((void **)(rxbuf+2)), txcnt );
				txbuf[0] = opcode; txcnt += 1;
				break;
			      case 'o' :	// [SYSDBG]['o'][ADRL][ADRH][D0][D1]...
			        if   ( etape.flags & ROOT )
				     {
				     rxcnt -= 5;	// soustraire opcode, subop, addresse et I2C checksum
				     if ( rxcnt <= MAXUPLOAD )
				        memcpy( *((void **)(rxbuf+2)), (void *)rxbuf+4, rxcnt );
				     extrac.crc_stat = 0;	// recette invalidee
				     txbuf[0] = opcode;
				     }
				else txbuf[0] = ILLERR;
				break;
			      default  :
				txbuf[0] = ILLERR;
				break;
			      }	// switch
			    break;
	           case RS485T :
			    if   ( etape.flags & ROOT )
				 {			// accepte toute longueur !
				 rxcnt -= 2;	// soustraire opcode et I2C checksum
				 if	( ( rxcnt > 0 ) && ( rxcnt <= 29 ) ) // securiser memcpy !
					{
					memcpy( (void *)uabuf+1, (void *)rxbuf+1, rxcnt );
					uabuf[rxcnt+1] = 0;	// terminateur pour send485
					send485();
					}
				 txbuf[0] = opcode;
				 }
			    else txbuf[0] = ILLERR;
			    break;
	           case RS485R :
			    if	 ( etape.flags & ROOT )
				 {
				 if	( rxcnt == 2 )
					{
					txbuf[1] = rcv485();
					if ( ( txbuf[1] > 0 ) && ( txbuf[1] <= 29 ) ) // securiser memcpy !
					   memcpy( (void *)txbuf+2, (void *)uabuf+1, txbuf[1] );
					txbuf[0] = opcode; txcnt = 31;
					}
				 }
			    else txbuf[0] = ILLERR;
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
				 txbuf[3] = ROLE;	// role, un caractere ASCII
				 txbuf[4] = reset_status;
				 txbuf[5] = BETAVER;		// beta sub version
				 txbuf[0] = opcode; txcnt = 6;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case ALLOFF :
			    if   ( rxcnt == 2 )
			 	 { 
				 LATD = VANNE0; LATB = 0;
				 txbuf[0] = opcode; txcnt = 1;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case PSTEP  :
			    if   ( rxcnt == 3 )
			 	 {
				 if ( ( etape.flags & ROOT ) == 0 )
				    {
				    etape.flags = 0;
				    etape.chrono = 0; sauter( rxbuf[1] );
				    }
				 txbuf[1] = etape.id;
				 txbuf[0] = opcode; txcnt = 2;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case PMANU  :
			    if   ( rxcnt == 3 )
			 	 {
				 if ( ( ( rxbuf[1] & ROOT ) == 0 ) || ( etape.id == 0 ) )
				    etape.flags = rxbuf[1];
				 txbuf[0] = opcode; txcnt = 1;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case PFULL :
			    if   ( rxcnt == 2 )
			 	 {
				 pfull();
				 txbuf[0] = opcode; txcnt = 31;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case PCRC :
			    if   ( rxcnt == 3 )
			 	 {
				 if   ( extrac.crc_stat & CRC_READY )
				      {
				      if ( rxbuf[1] )
					 extrac.crc_stat |= CRC_AUTOR;	// autorisation
				      }
				 else crc_start();			// lancer calcul initial
				 txbuf[1] = extrac.crc_stat;
				 *((unsigned long *)&(txbuf[2])) = extrac.crc;
				 *((unsigned int  *)&(txbuf[6])) = packlen;
				 txbuf[0] = opcode; txcnt = 8;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case PCHRON :
			    if   ( rxcnt == 4 )
			 	 {
				 *((unsigned int  *)&etape.chrono) = *((unsigned int  *)&(rxbuf[1]));
				 txbuf[0] = opcode; txcnt = 1;
				 }
			    else txbuf[0] = LENERR;
			    break;
                   default     :
                            txbuf[0] = SOUNK;
                   } // switch opcode long 
                } // if opcode long
           else {			// opcode court
		devadr = opcode & 0x0F;
		opcode &= 0xF0;
		switch ( opcode )
		   {
		   case VAON  :
		   case VAOFF :
			    if   ( rxcnt == 2 )
			 	 { 
				 if	( etape.flags & MANU )
					{
					vanne( ( opcode == VAON ), devadr );
					txbuf[0] = opcode | devadr;
					}
				 else   txbuf[0] = ILLERR;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case DAC   :
			    if   ( rxcnt == 4 )
			 	 {
				 if	( ( etape.flags & MANU ) && ( devadr < QMFC ) )
					{
					mfc[devadr].SV = *( (unsigned int *)(rxbuf+1) );
					if   ( sendDAC( devadr ) )
					     txbuf[0] = NAKI2C;
					else txbuf[0] = opcode | devadr;
					}
				 else   txbuf[0] = ILLERR;
				 }
			    else txbuf[0] = LENERR;
			    break;
		   case TCONS :
			    if   ( rxcnt == 4 )
			 	 {
				 if	( ( etape.flags & MANU ) && ( devadr < QTEM ) )
					{
					tem[devadr].SV = *( (unsigned int *)(rxbuf+1) );
					txbuf[0] = opcode | devadr;
					}
				 else   txbuf[0] = ILLERR;
				 }
			    else txbuf[0] = LENERR;
			    break;

		   default    : txbuf[0] = SOUNK;
		   } // switch
		} // else opcode long
	   } // if rxsum
      txsum = MYTSUM;		// reinitialiser emission
      rxcnt = 0; rxsum = 0;	// reinitialiser reception
      SSPADD = MYADDR;		// fin du controle de flux
      } // tache I2C

   // |||||||||||||||||||||||||||||||||||||| Taches tonk - reparties sur cycle de 1.68 s
   if 	( tonker != oldtonker )	// every 104 ms
  	{
	near static unsigned int diff;

	tonk16++; tonk16 &= 0x0F;

	// |||||||||||||||||||||||||||||||||||||| Tache boutons
	#ifdef BOUTONS_ANALOG
	ibut = andebou( ((unsigned char *)&adc4PV)[1] );
	if ( ibut != 0xFF )
	   curbut = ibut;
	#endif

	// |||||||||||||||||||||||||| tache beep_&_blink-the-LED
	if ( ( reset_status & 0x08 ) == 0 )	// Watchdog Reset
	   blink_pat = 0xBA00;

	if   ( tonk16 == 0 )
	     {
	     blink_piso = blink_pat;
	     #ifdef AUDIO
	     beep_piso = beep_pat;
	     #endif
	     }
	else {
	     blink_piso <<= 1;
	     #ifdef AUDIO
	     beep_piso <<= 1;
	     #endif
	     } 

	if   ( (((unsigned char *)&blink_piso)[1]) & 0x80 )
	     LATAbits.LATA4 = 0;
	else LATAbits.LATA4 = 1;

	#ifdef AUDIO
	if   ( (((unsigned char *)&beep_piso)[1]) & 0x80 )
	     audio_per_l = audio_pers[tonk16];	// nouvelle note
	else AUDIO_BIT = 0;
	#endif

	// |||||||||||||||||||||||||| tache controle temperatures
	if ( ( etape.flags & ROOT ) == 0 )
	   switch( tonk16 )
		{
		// lectures PV/SV
		case 1  :           askSV(0); break;
		case 2  : getSV(0); askSV(1); break;
		case 3  : getSV(1); askSV(2); break;
		case 6  : getSV(2); askPV(0); break;
		case 7  : getPV(0); askPV(1); break;
		case 9  : getPV(1); askPV(2); break;
		case 10 : getPV(2); break;
		// ecriture SV
		case 11 :
			if 	( tem[0].flags & RAMPEN )
				{
				diff = tem[0].RV - tem[0].SV;			// grosse astuce
				diff += MINDIFT;				// pour eviter calcul
				if ( diff > (MINDIFT+MINDIFT) ) setSV(0);	// de valeur absolue !!
				}
			else	if ( tem[0].RV != tem[0].SV )
				   {
				   if   ( tem[0].SV ) setSV(0);
				   else tem[0].SV = tem[0].RV;	// cas de reset de l'automate
				   }
			break;
		case 14 :
			if 	( tem[1].flags & RAMPEN )
				{
				diff = tem[1].RV - tem[1].SV;
				diff += MINDIFT;
				if ( diff > (MINDIFT+MINDIFT) ) setSV(1);
				}
			else	if ( tem[1].RV != tem[1].SV )
				   {
				   if   ( tem[1].SV ) setSV(1);
				   else tem[1].SV = tem[1].RV;
				   }
			break;
		case 15 :
			if 	( tem[2].flags & RAMPEN )
				{
				diff = tem[2].RV - tem[2].SV;
				diff += MINDIFT;
				if ( diff > (MINDIFT+MINDIFT) ) setSV(2);
				}
			else	if ( tem[2].RV != tem[2].SV )
				   {
				   if   ( tem[2].SV ) setSV(2);
				   else tem[2].SV = tem[2].RV;
				   }
			break;
		}
	 // |||||||||||||||||||||||||| tache frequencemetre
	 if ( ( tonk16 == 5 ) || ( tonk16 == 13 ) )
	    {
            freqmeter();
	    #ifdef SECU_PSIH4
	    // securite SiH4 inconditionnelle
	    if ( fre.PV > FREMAX )
	       vanne( 0, V_SIH4SRC );
	    #endif	
	    }

	oldtonker = tonker;
        } // if tonker

   // |||||||||||||||||||||||||||||||||||||| Tache process ( tonk16 = 0, 4, 8 ou 12 ) 
   if ( ( new_sec ) && ( ( tonk16 & 3 ) == 0 ) )
      {
      // chronometre ( etape.chrono est deja incremente en haute priorite )
      new_sec = 0;
      if ( ( etape.id ) && ( etape.flags == 0 ) )
	 {
	 if ( etape.chrono > etape.duree )
	    {
	    etape.chrono -= etape.duree;
	    sauter( etape.stogo );
	    }
	 }
      // checks (gestion rampe incluse, les SV peuvent changer et on peut sauter)
      // la fonction check_ppod() rend 0 si elle appelle sauter(), dans ce cas on elude
      // les autres tests	
      if ( etape.chrono >= etape.deldg )
	 {
	 extrac.ppod = &mfc[0]; if ( check_ppod() )
	  { extrac.ppod = &mfc[1]; if ( check_ppod() )
	    { extrac.ppod = &mfc[2]; if ( check_ppod() )
	      { extrac.ppod = &mfc[3]; if ( check_ppod() )
		{ extrac.ppod = &tem[0]; if ( check_ppod() )
		  { extrac.ppod = &tem[1]; if ( check_ppod() )
		    { extrac.ppod = &tem[2]; if ( check_ppod() )
		      { extrac.ppod = &fre; check_ppod();
	  } } } } } } }
	 }
      // autres sauts possibles
      #ifdef BOUTONS_ANALOG	//-----	// boutons de la face avant
      switch( curbut )
	{
	case TERM_BUT  : etape.chrono = 0; step_zero(); break;
	case START_BUT : if ( etape.id == 0 )
			    { etape.flags = 0; etape.chrono = 0; sauter( 1 );  } break;
	case CONT_BUT  : if ( etape.id )
			    { etape.flags = 0; } break;
	}
      #endif

      // actions sur les MFCs et vannes
      if ( mfc[0].flags & NEWSV )
	 {
	 sendDAC( 0 );
	 mfc[0].flags --;
	 }
      if ( mfc[1].flags & NEWSV )
	 {
	 sendDAC( 1 );
	 mfc[1].flags --;
	 }
      if ( mfc[2].flags & NEWSV )
	 {
	 sendDAC( 2 );
	 mfc[2].flags --;
	 }
      if ( mfc[3].flags & NEWSV )
	 {
	 sendDAC( 3 );
	 mfc[3].flags --;
	 }
      if ( ( etape.new ) || ( ( etape.flags & MANU ) == 0 ) )
	 {
	 LATD = ((unsigned char *)&etape.vannes)[0];
	 LATB = ((unsigned char *)&etape.vannes)[1];

	 #ifdef LEDS_RUN_PAUSE	//-----	// LEDs de signalisation sur sorties vannes
	 if ( etape.id )
	    vanne( 1, RUN_SIG );
	 if ( etape.flags )
	    vanne( 1, HOLD_SIG );
	 #endif
	 
	 #ifdef AUDIO
	 if   ( AUDIO_BIT )	// AUDIO_BIT de LATB qui vient d'etre copie de etape.vannes
	      {
	      if   ( END_BIT )
		   beep_pat = END_BEEP_PAT;
	      else beep_pat = ALM_BEEP_PAT;
	      }
	 else beep_pat = 0;
	 #endif

	 etape.new = 0;
	 }
      } // if new_sec

   #ifdef BOUTONS_ANALOG	//-----	// boutons de la face avant
   if ( curbut == TERM_BUT )
      { etape.chrono = 0; step_zero(); }
   #endif

   } // while(1);
}

#ifdef BOUTONS_ANALOG	//-----	// boutons de la face avant
/* anti-rebond rudimentaire pour bouton analog
   retourne FF si indecis
 */
#pragma udata grp1
unsigned char andebou( unsigned char val )	// val sur 8 bits 
{
static unsigned char oldibu1, oldibu2; unsigned char ibu;
if   ( val < 20 )  ibu = 0;
else if   ( val < 40 ) ibu = 1;
     else if   ( val < 72 ) ibu = 2;
          else if   ( val < 115 ) ibu = 3;
               else  if   ( val < 150 ) ibu = 4;
                     else ibu = 0;	// cable debranche !! 
if   ( ( ibu == oldibu1 ) && ( ibu == oldibu2 ) )
     return ibu;
oldibu2 = oldibu1;
oldibu1 = ibu;
return 0xFF;
} 
#endif

#pragma romdata jln_txt4

// info version pour identification fichier hex (ignore par le PIC)
rom unsigned char verpack[] = {
VERSION, SUBVERS, BETAVER, ROLE
};
