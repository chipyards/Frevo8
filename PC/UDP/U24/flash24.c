// systeme de flashage PIC 24, 33 pour TENCOR et KS, derive du projet Blossac
// adapte pour Frevo/SECU24
// - accepte fichier .BIN avec header contenant le CRC
// - accepte directement fichier .hex, dans ce cas produit le .BIN pour usage immediat
// - transparent au mode de communication (SPI ou UART) et au type de PIC (24F, 24H, 33F)

// attention dans le code de l'appli a flasher,
// le debut de la region "program" autorisee au niveau .gld
// doit etre egal a debut de bloc + 16 PC-units pour laisser 24 bytes de header  
// (le header sera entierement copie en ROM a l'insu de MPLAB)

// ATTENTION : renseigner intelhex.h en fonction de la capacite ROM du PIC

// ATTENTION : constantes MINROMADR (boot.h ou mpar.h) redondantes avec .gld
// zone(s) favorite(s)
#define APPLI_A MINROMADR
// #define APPLI_B 0x0B000

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "intelhex.h"
#include "crc32b.h"
#include "../../../mpar.h"
#include "../../../ipilot.h"
#include "../../irb.h"
#include "../../diali.h"

void gasp( char *fmt, ... );

/* donnees de la memoire flash PIC30F, en "rawbytes" (style fichier .hex)
   - 4 bytes par instr, dont un nul (MSB), big endian,
   - les adresses PIC (PC) sont egales aux adresses de raw bytes
     divisees par 2.
   - on ne transmettra que les 3 bytes utiles sur la liaison bootloader...
*/

extern unsigned char rawbytes[];
extern unsigned char rawblocks[];

unsigned char *b_to_pic = rawbytes;	/* programme pour le PIC, en raw bytes */
// les memes, en instructions
unsigned int *i_to_pic   = (unsigned int *)rawbytes;

unsigned char blobytes[QINSTR*3];	// donnees binaires pretes a flasher (ou a stocker
					// dans un fichier .BIN pour carte SD)

int firstblock, endblock;		// blocs de debut (inclus) et fin (exclus) a sortir

unsigned char blkbuf[QBLOCKBYTES];	// buffer pour lecture fichier .BIN

static irblock irb;			// objet pour dial()

/* ===================== traiements PIC - specifiques  ============================= */

// initialisation avant lecture hex
static void init_raw()
{
int i;
for ( i = 0; i < QINSTR; i++ )
    i_to_pic[i] = ERASED;
for ( i = 0; i < (QBLOCKS+1); i++ )
    rawblocks[i] = 0;
}

// decodage destination goto (rend 0 si echec)
static unsigned int decogoto( unsigned int adr )
{
unsigned int instr, dest;
adr >>= 1;	// PC-unit --> instr index
instr = i_to_pic[adr];
if ( ( instr  & 0x00FF0000 ) != 0x00040000 )
   return 0;
dest = instr & 0xFFFF;
instr = i_to_pic[adr+1];
if ( instr & 0xFFFF80 )
   return 0;
dest |= ( instr << 16 );
return dest;
}

/* ====================== traitement HEX--> BIN ================== */

// affichage zones (en adresses PC_PIC)
static void aff_param()
{
int i; int zflag = 0;

// on saute le premier block systematiquement
if ( rawblocks[0] )
   {
   printf("- de %06X ", 0 );
   printf("a %06X\n", LOCBLOCK );
   }

firstblock = QBLOCKS;
endblock = 0;

for ( i = 1; i < QBLOCKS+1; i++ )
    {
    if   ( rawblocks[i] )
         {
	 if ( zflag == 0 )
	    {
	    printf("- de %06X ", i * LOCBLOCK );
	    zflag = 1;
	    if ( i < firstblock )
	       firstblock = i;
	    }
	 }
    else {
	 if ( zflag )
	    {
	    printf("a %06X\n", i * LOCBLOCK );
	    zflag = 0;
	    if ( i > endblock )
	       endblock = i;
	    }
	 }
    }
printf("premier bloc (inclus) %03x\n", firstblock );
printf("bloc final   (exclus) %03x\n", endblock );
}

// conversion de 4 bytes/instr en 3 bytes/instr + calcul CRC --> fichier BIN
// si majname est NULL, affiche header seulement
static void convert( const char * majname )
{
unsigned int i, j, lenbytes, lenPC, adr, entry, cnt;
unsigned int crc;
FILE * df;

j = 0;

for ( i = (firstblock * INSBLOCK); i < (endblock * INSBLOCK); i++ )
    {
    blobytes[j++] = ((unsigned char *)&i_to_pic[i])[0];
    blobytes[j++] = ((unsigned char *)&i_to_pic[i])[1];
    blobytes[j++] = ((unsigned char *)&i_to_pic[i])[2];
    }
lenbytes = j;
lenPC = (endblock - firstblock) * LOCBLOCK;

if ( ( lenPC * 3 ) != ( lenbytes * 2 ) )
   gasp("lenPC=%d, lenbytes=%d", lenPC, lenbytes);

adr = firstblock * LOCBLOCK;
entry = decogoto( 0 );
if ( entry == 0 )
   gasp("echec decodage goto __reset");

/*
if ( adr == APPLI_A )
   majname = "MAJA.BIN";
else if   ( adr == APPLI_B )
          majname = "MAJB.BIN";
     else majname = "MAJ.BIN";
*/

printf("adresse chargement : 0x%06x\n", adr );
printf("point d'entree     : 0x%06x\n", entry );
printf("adresse fin        : 0x%06x\n", adr + lenPC );
printf("taille en PC-unit  : 0x%06x\n", lenPC );
printf("taille en bytes    : %d\n", lenbytes );

if ( majname == NULL )
   return;

printf("nom de fichier     : %s\n", majname );

((unsigned int *)blobytes)[1] = lenPC;
((unsigned int *)blobytes)[2] = adr;
((unsigned int *)blobytes)[3] = entry;

crc = 0xFFFFFFFF;
for ( j = 4; j < lenbytes; j++ )
    update_crc( &crc, blobytes[j] );
crc = ~crc;
((unsigned int *)blobytes)[0] = crc;

df = fopen( majname, "wb" );
if ( df == NULL )
   gasp("echec ouverture %s", majname );

cnt = fwrite( blobytes, 1, lenbytes, df );
if ( cnt != lenbytes )
   gasp("pb ecriture %s cnt=%d lenbytes=%d", majname, cnt, lenbytes );
fclose( df );
}

static const char * filext( const char * fnam )
{
int len;
len = strlen( fnam );
if ( len > 4 )
   return( fnam + len - 4 );
return("");
}

// conversion "a blanc"
void test_hex_file( const char * majname )
{
if ( strcmp( filext(majname), ".hex" ) == 0 )
   {
   printf("conversion hex->bin pour PIC %s\n", PICTYPE );
   init_raw();
   lec_hex_file( majname );
   aff_param();
   convert(NULL);
   printf("\n");
   }
}

/* ====================== interface bootloader ================== */

// cette fonction flashe le fichier .bin a l'adresse MINROMADR
int flash_file( const char * majname )
{
unsigned int cnt, rowi, segi, romadr, i, j;
int retval;
FILE *fp;

if ( strcmp( filext(majname), ".hex" ) == 0 )
   {
   printf("conversion hex->bin pour PIC %s\n", PICTYPE );
   init_raw();
   lec_hex_file( majname );
   aff_param();
   convert("MAJA.BIN");
   majname = "MAJA.BIN";
   }

if ( (fp = fopen( majname, "rb" )) == NULL )
   gasp("echec ouverture %s en lecture", majname );

romadr = MINROMADR;

do {			// boucle des blocs
   cnt = fread( blkbuf, 1, QBLOCKBYTES, fp );
   if ( cnt == 0 ) break;
   if ( cnt != QBLOCKBYTES )
      gasp("fichier n'est pas multiple de %d (%d)", QBLOCKBYTES, cnt );
   
   // tentons un effacement (1 bloc)
   irb.txbuf[0] = ERA_FLASH;
   irb.txbuf[1] = ((unsigned char *)&romadr)[0];
   irb.txbuf[2] = ((unsigned char *)&romadr)[1];
   irb.txbuf[3] = ((unsigned char *)&romadr)[2];
   irb.txcnt = 4;
   irb.rxcnt = 1;
   retval = dial( &irb );
   if ( retval )
      gasp("echec dial sur effacement (%d)", retval );
   printf("effacement %06x Ok\n", romadr );
   // gravons les donnees
   j = 0;	// indice de byte dans le bloc
   for	( rowi = 0; rowi < QROWPERBLOCK; rowi++ )
	{
	// copions les data d'1 row dans la RAM du PIC 33
	for ( segi = 0; segi < QFSEG; segi++ )
	    {
	    irb.txbuf[0] = WR_FSEG;
	    irb.txbuf[1] = (unsigned char)segi;
	    for ( i = 2; i < ( QSEGBYTES + 2 ); i++ )
	        irb.txbuf[i] = blkbuf[j++];
	    irb.txcnt = QSEGBYTES + 2;
	    irb.rxcnt = 2;
	    retval = dial( &irb );
	    if ( retval )
	       gasp("echec dial sur copie segment (%d)", retval );
	    if ( irb.rxbuf[1] != (unsigned char)segi )
	       gasp("erreur sequencement segment (%d vs %d)", irb.rxbuf[1], segi );
	    }
	if ( j != ( (rowi+1) * QROWBYTES ) )
	   gasp("erreur compte bytes (%d vs %d)", j, ( (rowi+1) * QROWBYTES ) );
	// gravons 1 row
	irb.txbuf[0] = WR_FLASH;
	irb.txbuf[1] = ((unsigned char *)&romadr)[0];
	irb.txbuf[2] = ((unsigned char *)&romadr)[1];
	irb.txbuf[3] = ((unsigned char *)&romadr)[2];
	irb.txcnt = 4;
	irb.rxcnt = 1;
	retval = dial( &irb );
	if ( retval )
	   gasp("echec dial sur ecriture (%d)", retval );
	printf("ecriture %06x Ok\n", romadr );
	romadr += QROWPCUNITS;
	}	// boucle des rows
   if ( j != QBLOCKBYTES )
      gasp("erreur total bytes (%d vs %d)", j, QBLOCKBYTES );
   } while( cnt );	// boucle des blocs

fclose( fp );
return 0;
}

// cette fonction compare le fichier .bin a la flash en MINROMADR
int check_file( const char * majname )
{
unsigned int cnt, rowi, segi, romadr, i, j, errcnt;
int retval;
FILE *fp;

if ( strcmp( filext(majname), ".hex" ) == 0 )
   {
   printf("conversion hex->bin pour PIC %s\n", PICTYPE );
   init_raw();
   lec_hex_file( majname );
   aff_param();
   convert("MAJA.BIN");
   majname = "MAJA.BIN";
   }

if ( (fp = fopen( majname, "rb" )) == NULL )
   gasp("echec ouverture %s en lecture", majname );

romadr = MINROMADR; errcnt = 0;

do {			// boucle des blocs
   cnt = fread( blkbuf, 1, QBLOCKBYTES, fp );
   if ( cnt == 0 ) break;
   if ( cnt != QBLOCKBYTES )
      gasp("fichier n'est pas multiple de %d (%d)", QBLOCKBYTES, cnt );
   
   j = 0;	// indice de byte dans le bloc
   for	( rowi = 0; rowi < QROWPERBLOCK; rowi++ )
	{
	// lisons 1 row
	irb.txbuf[0] = RD_FLASH;
	irb.txbuf[1] = ((unsigned char *)&romadr)[0];
	irb.txbuf[2] = ((unsigned char *)&romadr)[1];
	irb.txbuf[3] = ((unsigned char *)&romadr)[2];
	irb.txcnt = 4;
	irb.rxcnt = 1;
	retval = dial( &irb );
	if ( retval )
	   gasp("echec dial sur lecture (%d)", retval );
	printf("lecture %06x Ok\n", romadr );
	// verifions les data d'1 row dans la RAM du PIC 33
	for ( segi = 0; segi < QFSEG; segi++ )
	    {
	    irb.txbuf[0] = RD_FSEG;
	    irb.txbuf[1] = (unsigned char)segi;
	    irb.txcnt = 2;
	    irb.rxcnt = QSEGBYTES + 2;
	    retval = dial( &irb );
	    if ( retval )
	       gasp("echec dial sur copie segment (%d)", retval );
	    if ( irb.rxbuf[1] != (unsigned char)segi )
	       gasp("erreur sequencement segment (%d vs %d)", irb.rxbuf[1], segi );
	    for ( i = 2; i < ( QSEGBYTES + 2 ); i++ )
	        if ( irb.rxbuf[i] != blkbuf[j++] )
		   errcnt++;
	    }
	if ( j != ( (rowi+1) * QROWBYTES ) )
	   gasp("erreur compte bytes (%d vs %d)", j, ( (rowi+1) * QROWBYTES ) );
	romadr += QROWPCUNITS;
	}	// boucle des rows
   if ( j != QBLOCKBYTES )
      gasp("erreur total bytes (%d vs %d)", j, QBLOCKBYTES );
   } while( cnt );	// boucle des blocs

fclose( fp );
printf("nombre d'erreurs data : %d\n", errcnt );
return 0;
}

// cette fonction demande au pic cible de verifier le CRC de l'appli a MINROMADR
int remote_crc( )
{
int retval;
unsigned int entry, len;
irb.txbuf[0] = CRC_FLASH;
irb.txcnt = 1;
irb.rxcnt = 7;
retval = dial( &irb );
if ( retval )
   gasp("echec dial sur crc flash (%d)", retval );
entry = 0; len = 0;
((unsigned char *)&entry)[0] = irb.rxbuf[2];
((unsigned char *)&entry)[1] = irb.rxbuf[3];
((unsigned char *)&len)[0] = irb.rxbuf[4];
((unsigned char *)&len)[1] = irb.rxbuf[5];
((unsigned char *)&len)[2] = irb.rxbuf[6];
printf("code retour = %d, pt d'entree = 0x%04x, long = 0x%06x\n",
       irb.rxbuf[1], entry, len );
return( irb.rxbuf[1] );
}

// cette fonction demande l'execution de l'appli a MINROMADR (si crc ok)
// on n'attend pas de retour du pic cible...
int remote_exec( )
{
int retval;
irb.txbuf[0] = EXE_FLASH;
irb.txcnt = 1;
irb.rxcnt = 0;
retval = dial( &irb );
if ( retval )
   gasp("echec tran_send sur exe flash (%d)", retval );
return 0;
}

// cette fonction demande le soft reset
// on n'attend pas de retour du pic cible...
int remote_reset( )
{
int retval;
irb.txbuf[0] = SW_RESET;
irb.txbuf[1] = (unsigned char)~SW_RESET;
irb.txcnt = 2;
irb.rxcnt = 0;
retval = dial( &irb );
if ( retval )
   gasp("echec tran_send sur sw reset (%d)", retval );
return 0;
}

// lecture version, remplit chaine ASCII
int read_version( char * tbuf )
{
int retval;
irb.txbuf[0] = SYSVER;
irb.txcnt = 1;
irb.rxcnt = 5;
retval = dial( &irb );
if   ( retval == 0 )
     {
     tbuf[0] = irb.rxbuf[1] + '0';
     tbuf[1] = '.';
     tbuf[2] = irb.rxbuf[2] + '0';
     tbuf[3] = irb.rxbuf[3];
     tbuf[4] = irb.rxbuf[4];
     tbuf[5] = 0;
     }
else tbuf[0] = 0;
return( retval );
}
