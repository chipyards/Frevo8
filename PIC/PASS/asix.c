/* interface bas niveau pour ASIX */

#include <p18cxxx.h>
#include <delays.h>
#include "asix_regs.h"
#include "asix.h"
#include "mymac.h"
#include "fix_var2.h"

#define CLOCK_FREQ	20000000
#define DelayMs(ms)	delay_1kcy(((CLOCK_FREQ/4000000)*ms))

#pragma code jln_lib2

/* fonction derivee de Delay1KTCYx de la lib mcc18
   unite = 1000 cycles */
void delay_1kcy( unsigned char nkcy )
{
do
  {
  _asm
	movlw   0x4c	// 3*76  = 228 cycles
	movwf   INDF1,0
  kl1:	decfsz  INDF1,1,0
	bra     kl1
	clrf    INDF1,0	// 3*256 = 768 cycles
  kl2:	decfsz  INDF1,1,0
	bra     kl2
  _endasm
  }
while (--nkcy);
}


/* initialiser ...3 fonctions
	- asix_init1() fait le gros du travail
	- asix_fdx()   met a jour link_status et FDU dans TCR
	- asix_start() demarre l'operation
 */
void asix_init1( unsigned char mymac5 )
{
iow_pin = 1;
ior_pin = 1;
rst_pin = 1;
iow_tri = 0;
ior_tri = 0;
rst_tri = 0;			// hard reset sur ASIX reset pin
DelayMs(4);
rst_pin = 0;			// hard reset done
read_asix( RSTPORT );		// NE2000 tradition
write_asix( CR, ADMA | STOP );
write_asix( GPOC, MPSEL );	// select internal PHY

write_asix( DCR, 0 );		// byte wide bus 

write_asix( BNRY, RINGSTART );	// BNRY = max RX page before OVW 
write_asix( PSTART, RINGSTART );// RING head 
write_asix( PSTOP, RINGSTOP );	// RING tail
write_asix( TPSR, TXSTART );	// TX RAM head 
 
write_asix( CR, PAGE1 | ADMA | STOP );	// page 1
write_asix( CPR, RINGSTART+1 );		// CURRENT page = next RX page to write 
next_pack = RINGSTART+1;
				// MAC address for unicast RX
write_asix( MACA0,   MYMAC0 );
write_asix( MACA0+1, MYMAC1 );
write_asix( MACA0+2, MYMAC2 );
write_asix( MACA0+3, MYMAC3 );
write_asix( MACA0+4, MYMAC4 );
write_asix( MACA0+5, mymac5 );

write_asix( CR, ADMA | STOP );	// page 0
write_asix( RCR, INTT | AB );	// int pin act hi, accept broadcast 

write_asix( ISR, 0xFF );		// clear interrupt flags 
write_asix( IMR, OVWE | PRXE );	// interrupt on RX & RX overflow 
txflag = 0;
}

void asix_fdx( void )
{
link_status = read_asix( GPI );
link_status &= 0x07;
if   ( link_status & I_DPX )	// full duplex ?
     write_asix( TCR, FDU );	// ... et on rend cette info au MAC
else write_asix( TCR, 0 );
}

void asix_start( void ) 
{
write_asix( CR, ADMA | START );
}

/* ecrire dans un registre 8 bits */
void write_asix( unsigned char regadr, unsigned char data )
{
asixaddr = regadr | adrmask;	// ior and iow are here ! 
asixdataw = data;
asixtris = 0;
iow_pin = 0;
_asm
	nop nop nop nop
_endasm
iow_pin = 1;
asixtris = 0xFF;
}

/* lire un registre de 8 bits */
unsigned char read_asix( unsigned char regadr )
{
unsigned char res;
asixtris = 0xFF;
asixaddr = regadr | adrmask;	// ior and iow are here ! 
ior_pin = 0;
_asm
	nop nop nop
_endasm
res = asixdatar;
ior_pin = 1;
return(res);
}
//*/

/* lire un paquet dans le RX ring, rendre taille CRC exclus
   utilise la var. globale next_pack pour trouver le paquet
   puis la met a jour sur le suivant.
   PAQUETS limites a 252 bytes, NE2000 header exclus, CRC exclus
   car on ne sait pas copier plus d'une page.
   les paquets de plus de 248 bytes CRC exclus prennent 2 pages
   dans le ring car le CRC depasse dans la page suivante mais on
   sait les traiter OK 
   Les paquets plus gros ne doivent pas planter la machine mais
   seront tronques.
   var. globale err_status sera mise a jour
   retourne longueur CRC exclus
 */
unsigned char read_rx_asix( unsigned char * dest )
{
unsigned char i, page, status, bnry;
unsigned int len;

page = next_pack;
// effacer la requete d'interrupt DMA
write_asix( ISR, RDC );
// charger adresse et compteur
write_asix( RSAR0, 0 );
write_asix( RSAR1, page );
write_asix( RBCR0, 4 );
write_asix( RBCR1, 0 );
write_asix( CR, RDMA | START );
// lire header "NE2000"
status    = read_asix( DMAPORT );
next_pack = read_asix( DMAPORT );
((unsigned char *)&len)[0] = read_asix( DMAPORT );
((unsigned char *)&len)[1] = read_asix( DMAPORT );
// len      |= ( read_asix( DMAPORT ) << 8 ); // BUG COMPILO

// error processing
if ( ( next_pack < RINGSTART ) || ( next_pack >= RINGSTOP ) )
   { err_status |= RINGLOST; return( 0 ); }
len -= 4;	// soustraction CRC
if ( len > 252 )
   { err_status |= BIGRX; len = 252; }
// data copy
write_asix( ISR, RDC );
write_asix( RSAR0, 4 );
write_asix( RSAR1, page );
write_asix( RBCR0, ((unsigned char *)&len)[0] );
write_asix( RBCR1, 0 );
write_asix( CR, RDMA | START );
// envoyer donnees
for ( i = 0; i < ((unsigned char *)&len)[0]; i++ )
    dest[i] = read_asix( DMAPORT );
write_asix( ISR, RDC );
// liberer espace lu dans le RING
bnry = next_pack - 1;
if ( bnry < RINGSTART )
   bnry = RINGSTOP - 1;
write_asix( BNRY, bnry );
return( ((unsigned char *)&len)[0] );
}

/* envoyer un paquet, taille limitee a 255 bytes
   (cf Frevo6 pour version non limitee)
   len = longueur CRC exclu
 */
void send_tx_asix( unsigned char * src, unsigned char len )
{
unsigned char i;
// attendre que le transmetteur soit libre...

/* bug ASIX : TXP est 0 meme si transmission en cours ...
do { LATAbits.LATA4 = 0; }
   while ( read_asix( CR ) & TXP );
LATAbits.LATA4 = 1;
//*/

if ( txflag )
// do { LATAbits.LATA4 = 0; }
   while ( ( read_asix( ISR ) & PTX ) == 0 );
//LATAbits.LATA4 = 1;
write_asix( ISR, PTX );
   
// copier paquet dans la RAM asix
write_asix( ISR, RDC );
write_asix( RSAR0, 0 );
write_asix( RSAR1, TXSTART );
write_asix( RBCR0, len );
write_asix( RBCR1, 0 );
write_asix( CR, WDMA | START );
for ( i = 0; i < len; i++ )
    write_asix( DMAPORT, src[i] );
write_asix( ISR, RDC );
// envoyer paquet
write_asix( TPSR, TXSTART );	// TX RAM head 
write_asix( TBCR0, len );	// TX RAM cnt
write_asix( TBCR1, 0 );
write_asix( CR, ADMA | TXP );	// GO !
txflag = 1;
}
