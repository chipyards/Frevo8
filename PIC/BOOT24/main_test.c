// prog pour le PIC 24F avec UART 
// 	Tencor carte TAJ1
//	KS carte trifa1
//	AIME carte SBC24s

#if defined(__PIC24F__)
    #include <p24fxxxx.h>
#elif defined(__PIC24H__)
    #include <p24hxxxx.h>
#elif defined(__dsPIC30F__)
    #include <p30Fxxxx.h>
#endif


#include "hard.h"
#include "uart2.h"
#include "../../mpar.h"
#include "../../version.h"
#include "flask.h"
#include "boot.h"

// config section : trifa1, SBC24s ont un quartz 20MHz, Tencor un oscillateur 20MHz

#if defined( __PIC24FJ64GA002__ )
     _CONFIG2(IESO_OFF & FNOSC_PRI & FCKSM_CSDCMD & POSCMOD_HS )
     _CONFIG1(JTAGEN_OFF & ICS_PGx1 & FWDTEN_OFF)        // JTAG off, watchdog timer off
#else
   #error This code is designed for the PIC24FJ64GA002
#endif

// placeholders pour les interrupts de l'appli
void __attribute__((interrupt, section(".myU2TX"), auto_psv)) _AltU2TXInterrupt(void)
{
IFS1bits.U2TXIF = 0;
}

void __attribute__((interrupt, section(".myU2RX"), auto_psv)) _AltU2RXInterrupt(void)
{
IFS1bits.U2RXIF = 0;
}

void __attribute__((interrupt, section(".myINT0"), auto_psv)) _AltINT0Interrupt(void)
{
_INT0IF = 0;
}

void __attribute__((interrupt, section(".myT2"), auto_psv)) _AltT2Interrupt(void)
{
_T2IF = 0;
}

void __attribute__((interrupt, section(".myT3"), auto_psv)) _AltT3Interrupt(void)
{
_T3IF = 0;
}

// buffer pour flash
unsigned char rowbuf[QROWBYTES];

// buffer pour commandes venues par UART
unsigned char rxbuf[QMPAR];
unsigned int rxcnt;

int main( void )
{
// variables pour le bootloader
unsigned long romadr;
unsigned int i, j, segi;

// interpretation reset :
//	SWR ou MCLR pin ==> on reste en bootloader
//	POR, WDT ou trap ==> on tente l'appli
if   ( ( RCONbits.SWR ) || ( RCONbits.EXTR ) )
     RCON = 0;
else {
     i = boot_verif( MINROMADR, &j, &romadr );
     if ( i == 0 )
	fgorun( j );	// executer appli si valide
     }

_NSTDIS = 1;	// no nested interrupt please

uart2_baud( 9600 );
uart2_init( ONE_STOP, NOPAR );
// uart2_puts("12345 ~~~ 123");
// PAS DE STRING PB AU LINK IL LES MET APRES LES INT DE L'APPLI

i = 0;
while(1)
  {
  if ( uart2_hit() )
     {
     if ( i < QMPAR ) rxbuf[i++] = uart2_getc();
     if   ( i == 1 )	// premier byte = opcode
	  {
	  switch( rxbuf[0] )
	     {
	     case SYSVER :		// reponse immediate
		uart2_putc( VERSION );
		uart2_putc( SUBVERS );
		uart2_putc( BETAVER );
		uart2_putc( 'B' );
		uart2_putc( rxbuf[0] );	// echo opcode final
		i = 0;			// fini
		break;
	     // bootloader commands
	     case WR_FSEG :	// ecriture vers rowbuf d'un segment de data a flasher
		rxcnt = QSEGBYTES + 2;	// vient avec 1 byte d'index de segment
	        break;
	     case WR_FLASH :	// copie de rowbuf dans la flash meme
		rxcnt = 4;	// vient avec 3 bytes d'adresse
	        break;
	     case ERA_FLASH :	// effacement d'un bloc de la flash
		rxcnt = 4;	// vient avec 3 bytes d'adresse
		break;
	     case RD_FLASH :	// copie de la flash vers rowbuf
		rxcnt = 4;	// vient avec 3 bytes d'adresse
		break;
	     case RD_FSEG :	// lecture d'un segment de data de rowbuf
		rxcnt = 2;	// vient avec 1 byte d'index de segment
		break;
	     case CRC_FLASH :	// verifie le CRC a partir de MINROMADR
		i = boot_verif( MINROMADR, &j, &romadr );
		uart2_putc( (unsigned char) i );	// code retour (0 si Ok)
		uart2_putc( ((unsigned char *)&j)[0] );	// point d'entree sur 16 bits
		uart2_putc( ((unsigned char *)&j)[1] );
		uart2_putc( ((unsigned char *)&romadr)[0] );	// longueur en PCU
		uart2_putc( ((unsigned char *)&romadr)[1] );
		uart2_putc( ((unsigned char *)&romadr)[2] );
		uart2_putc( rxbuf[0] );	// echo opcode
		i = 0;			// reponse immediate finie
		break;
	     case EXE_FLASH :	// execute apres verif CRC, a partir de MINROMADR
		i = boot_verif( MINROMADR, &j, &romadr );
		if ( i == 0 )
		   fgorun( j );
		uart2_putc( rxbuf[0] );	// echo opcode
		i = 0;			// reponse immediate finie
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
	     // bootloader commands
	     case WR_FSEG :	// ecriture vers rowbuf d'un segment de data a flasher
		segi = (unsigned int) rxbuf[1];	// index du segment
		if   ( segi < QFSEG )
		     {
		     j = segi * QSEGBYTES;
		     for ( i = 2; i < QSEGBYTES+2; i++ )
			 { rowbuf[j++] = rxbuf[i];  }
		     uart2_putc( segi );
		     uart2_putc( rxbuf[0] );	// echo opcode
		     }
		else uart2_putc( ERR_PAR );
		i = 0;			// traitement fini
		break;
	     case WR_FLASH :	// copie de rowbuf dans la flash meme
		((unsigned char *)&romadr)[0] = rxbuf[1];
		((unsigned char *)&romadr)[1] = rxbuf[2];
		((unsigned char *)&romadr)[2] = rxbuf[3];
		((unsigned char *)&romadr)[3] = 0;
		if   ( ( romadr >= MINROMADR ) && ( ( romadr & 1 ) == 0 ) )
		     {
		     flashw( romadr, rowbuf );	// interrupt disable is included
		     // uart2_init( ONE_STOP, NOPAR );	// deplantage ??
		     uart2_putc( rxbuf[0] );	// echo opcode
		     }
		else uart2_putc( ERR_PAR );
		i = 0;			// traitement fini
		break;
	     case ERA_FLASH :	// effacement d'un bloc de la flash
		((unsigned char *)&romadr)[0] = rxbuf[1];
		((unsigned char *)&romadr)[1] = rxbuf[2];
		((unsigned char *)&romadr)[2] = rxbuf[3];
		((unsigned char *)&romadr)[3] = 0;
		if   ( ( romadr >= MINROMADR ) && ( ( romadr & 1 ) == 0 ) )
		     {
		     ferase( romadr );	// interrupt disable is included
		     // uart2_init( ONE_STOP, NOPAR );	// deplantage ??
		     uart2_putc( rxbuf[0] );	// echo opcode
		     }
		else uart2_putc( ERR_PAR );
		i = 0;			// traitement fini
		break;
	     case RD_FLASH :	// copie de la flash vers rowbuf
		((unsigned char *)&romadr)[0] = rxbuf[1];
		((unsigned char *)&romadr)[1] = rxbuf[2];
		((unsigned char *)&romadr)[2] = rxbuf[3];
		((unsigned char *)&romadr)[3] = 0;
		if   ( ( romadr & 1 ) == 0 )
		     {
		     flashr( romadr, rowbuf );
		     uart2_putc( rxbuf[0] );	// echo opcode
		     }
		else uart2_putc( ERR_PAR );
		i = 0;			// traitement fini
		break;
	     case RD_FSEG :	// lecture d'un segment de data de rowbuf
		segi = (unsigned int) rxbuf[1];	// index du segment
		if   ( segi < QFSEG )
		     {
		     uart2_putc( segi );
		     j = segi * QSEGBYTES;
		     for ( i = 2; i < QSEGBYTES+2; i++ )
			 { uart2_putc( rowbuf[j++] );  }
		     uart2_putc( rxbuf[0] );	// echo opcode
		     }
		else uart2_putc( ERR_PAR );
		i = 0;			// traitement fini
		break;
	     default :
		uart2_putc( ERR_OP ); i = 0;
	     }			// switch
	  }		// if (i >= rxcnt )
     }		// if ( uart2_hit() )
  }	// while (1)

}
