/* interface bas niveau pour ASIX */

// SBC18a wiring
#define  asixaddr    PORTD
#define  asixdatar   PORTB
#define  asixdataw   LATB
#define  asixtris    TRISB

#define  asixIREQ    PORTEbits.RE2

// act low
#define  ior_pin     LATDbits.LATD5
#define  iow_pin     LATDbits.LATD6

// act hi
#define  rst_pin     LATDbits.LATD7

#define  ior_tri     TRISDbits.TRISD5
#define  iow_tri     TRISDbits.TRISD6
#define  rst_tri     TRISDbits.TRISD7
// attention : les pins ior et iow sont sur le meme port que adr !
// Le bus d'adresse est suppose deja configure

#define adrmask 0b01100000	// pour | avec LATD

// ASIX constants
// please include "asix_regs.h"

// NIC RAM buffer locations
// 8-bit page addresses
#define TXSTART		0x40
#define RINGSTART	0x46
#define RINGSTOP	0x80
// 16-bit byte addresses from the above
#define TX_start	0x4000
#define RING_start	0x4600
#define RING_stop	0x8000

// bits of global unsigned char err_status
// software only
#define RINGLOST	0x01	// perdu controle du ring
#define RINGOVW		0x02	// debordement du ring
#define BIGRX		0x04	// paquet trop gros (tronque)
#define UDPCHK		0x10	// UDP checksum
// fonctions

/* initialiser ...3 fonctions
	- asix_init1() fait le gros du travail
	- asix_fdx()   met a jour link_status et FDU dans TCR
	- asix_start() demarre l'operation
 */
void asix_init1( unsigned char mymac5 );
void asix_fdx( void );
void asix_start( void );

/* ecrire dans un registre 8 bits */
void write_asix( unsigned char regadr, unsigned char data );

/* lire un registre de 8 bits */
unsigned char read_asix( unsigned char regadr );

/* ecrire dans la RAM ASIX */
void write_ram_asix( unsigned int adr, unsigned int N, unsigned char * data );

/* lire dans la RAM ASIX */
void read_ram_asix( unsigned int adr, unsigned int N, unsigned char * data );

/* envoyer un paquet, taille limitee a 255 bytes
   (cf Frevo6 pour version non limitee)
   len = longueur CRC exclu
 */
void send_tx_asix( unsigned char * src, unsigned char len );

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
 */
unsigned char read_rx_asix( unsigned char * dest );


// fonctions de mii.c
/* ecriture de 32 uns dans le port MII */
void write_mii_pre( void );
/* ecriture dans un registre MII : transmission serie via registre MEMR */
void write_mii( unsigned char regad, unsigned int mii_data );
/* lecture d'un registre MII : transmission serie via registre MEMR */
unsigned int read_mii( unsigned char regad );
/* inhibition de la vitesse 100 Mbits/s (pour economie d'energie) */
void mii_disable100( void );
