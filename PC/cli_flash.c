/* Frevo7 : flasheur de PIC 18F

  inclut lecture de fichier hex
  gestion bootloader I2C et UDP

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include "../ipilot.h"
#include "diali.h"


void gasp( char *fmt, ... );  /* fatal error handling */


/* ====================== base de donnees "UPICe" ================= */

/* probleme : le fichier "intel HEX" meme dans sa version
   avancee INHX32 ne supporte que des donnees de 8 bits.

   Le fichier hex est d'abord lu dans un tableau de bytes, ce
   qui est compatible avec les adresses impaires engendrees
   par certains outils (compilateurs F18). Ce tableau de bytes
   est accompagne d'un tableau de zones.

   Alors commence un traitement PIC-specifique :
   Les PICs 14 bits (16F/16C) et 16 bits (18F) utilisent des
   instructions de 2 bytes stockees en little-endian dans le
   fichier HEX, ce qui nous sauve car on peut avec un simple
   casting transformer un tableau de bytes en tableau de words
   (alors le present programme ne tourne que sur PC x86)

   Les frontieres de zones seront ensuite arrondies aux multiples
   - de 2 pour les 16F/16C
   - de 64 pour 18F
   Les zones sont eventuellement fusionnees et completees avec
   des FFs.

*/

#define QDATB (0x8000)  /* espace prog adressable PIC 18F en bytes */

#define QDAT (QDATB/2)  /* espace prog adressable PIC 18F en words */

#define QZON 100

static unsigned char b_to_pic[QDATB];		/* programme pour le PIC, en bytes */
static unsigned char b_from_pic[QDATB];	/* programme lu du PIC, en bytes */

#define USHORT unsigned short

static USHORT *w_to_pic   = (USHORT*)b_to_pic;		/* programme pour le PIC, en words*/
static USHORT *w_from_pic = (USHORT*)b_from_pic;	/* programme lu du PIC, en words */

typedef struct {	/* adresses debut et fin de zone */
int a1;			/* (inclus) */
int a2;			/* (exclu) */
} zone;

static zone zoneb[QZON];	/* table des zones (adresses de bytes) */

static int qzon;		/* nombre de zones valides dans w_to_pic */
static int curpage;		/* page de 64 k bytes selon format INHX32 */
static int nextadr;		/* adresse prochaine donne, utilisee pour 
			   detection des discontinuites */

/* decodage du format .hex INHX32

   - les adresses en debut d'enregistrement sont sur 16 bits big-endian (!!)
   - les enregistrements de type 04 contiennent une adresse de page aussi
     sur 16 bits big-endian dans leur champ data (-> adr. de 32 bits).
   - les data sont en bytes, qui peuvent representer des words en small
     ou big endian a discretion de l'utilisateur (ici small)
   - un enregistrement de longueur nulle est considere comme EOF

   La fonction lec_hex_file() traduit chaque record en une struct 
   INTEL_HEX_RECORD qui est passee a la fonction lec_hex_rec().
   La fonction lec_hex_file() et lec_hex_rec() n'ont rien de specifique
   du PIC, et manipulent des adresses de bytes.

   La fonction lec_hex_rec() interprete les types 0 et 4 et
   cree la base de donnee des zones.

   ATTENTION :
   - les variables globales qzon, curpage, nextadr sont partagees
     par ces 2 fonctions
   - la derniere zone est terminee par lec_hex_file()
 */ 

/* structure deja declaree dans ezusbsys.h */

#define MAX_INTEL_HEX_RECORD_LENGTH 16

typedef struct _INTEL_HEX_RECORD
{
   unsigned char  Length;
   unsigned short Address;
   unsigned char  Type;
   unsigned char  Data[MAX_INTEL_HEX_RECORD_LENGTH];
} INTEL_HEX_RECORD;


static void lec_hex_rec( INTEL_HEX_RECORD * prec );

static int hex2i ( char ascii ) /* CONVERSION  1 char ASCII hexa -> int */
{
if ( ('0' <= ascii) && (ascii <= '9') )
   return( ascii - '0' );
if ( ('A' <= ascii) && (ascii <= 'F') )
   return( ascii + ( 10 - 'A' ) );
if ( ('a' <= ascii) && (ascii <= 'f') )
   return( ascii + ( 10 - 'a' ) );
gasp("illegal hex char %02x", ascii );
return(-1);
}

static void lec_hex_file( char *fnam )
{
char car;
int etat,	/* etape active */
    charcnt,	/* nombre de caracteres a traiter pour le byte courant */
    curbyte,	/* byte courant */
    bytecnt,	/* nombre de bytes a traiter ensuite */
    ibyte,	/* index byte dans champ data */
    checksum;	/* checksum en cours de calcul */

INTEL_HEX_RECORD curec;  /* record en cours de traitement */

FILE *fp;

if ( (fp = fopen( fnam, "r" )) == NULL )
   gasp( "Impossible d'ouvrir le fichier %s\n", fnam );

printf("Ouverture %s\n", fnam );

etat = 0; 
qzon = 0; curpage = 0; nextadr = -1; /* pour lec_hex_rec() */

do  {
    car = getc( fp );  /* lecture d'un car */
    if ( car == EOF ) etat = 6;

    switch( etat )
      {
      case 0  : /* initialisation */
		charcnt = 2; curbyte = 0;
                bytecnt = 0;
                checksum = 0;
                if ( car == ':' ) etat = 1;
                break;

      case 1  : /* traitement longueur */
		curbyte = ( curbyte << 4 ) + hex2i( car );
                charcnt--;                 
                if ( charcnt == 0 )
                   {
                   checksum += curbyte;
                   curec.Length = curbyte;
		   if ( curec.Length > MAX_INTEL_HEX_RECORD_LENGTH )
		      gasp( "record length %d greater than supported (%d )",
			    curec.Length, MAX_INTEL_HEX_RECORD_LENGTH );
                   if ( curec.Length == 0 ) etat = 6;
                   else                     etat = 2;
                   charcnt = 2; curbyte = 0;
		   bytecnt = 2; curec.Address = 0;
                   }
                break;

      case 2  : /* traitement adresse */
		curbyte = ( curbyte << 4 ) | hex2i( car );
                charcnt--;                 
                if ( charcnt == 0 )
                   {
                   checksum += curbyte;
		   curec.Address = ( curec.Address << 8 ) | curbyte;
		   bytecnt--;
                   if ( bytecnt == 0 )
                      etat = 3;
                   charcnt = 2; curbyte = 0;
                   }
                break;

      case 3  : /* traitement type */
		curbyte = ( curbyte << 4 ) + hex2i( car );
                charcnt--;                 
                if ( charcnt == 0 )
                   {
                   checksum += curbyte;
                   curec.Type = curbyte;
                   etat = 4;
		   charcnt = 2; curbyte = 0;
		   bytecnt = curec.Length; ibyte = 0;
                   }
                break;

      case 4  : /* traitement data bytes */
		curbyte = ( curbyte << 4 ) + hex2i( car );
                charcnt--;                 
                if ( charcnt == 0 )
                   {
                   checksum += curbyte;
                   curec.Data[ibyte++] = curbyte;
		   bytecnt--;
                   if ( bytecnt == 0 )
                      etat = 5;                   
		   charcnt = 2; curbyte = 0;
                   }
                break;

      case 5  : /* traitement checksum */
		curbyte = ( curbyte << 4 ) + hex2i( car );
                charcnt--;                 
                if ( charcnt == 0 )
                   {
                   checksum += curbyte;
		   if ( checksum & 0xFF )
                      gasp("checksum fichier hex");
		   lec_hex_rec( &curec );
                   etat = 0;
                   }
                break;
      case 6  : fclose( fp );
                if   ( qzon )  /* fin de la zone courante */
		     zoneb[qzon-1].a2 = nextadr;
		return;

      default : break;
      }
    }
while(1);
}

/* la fonction lec_hex_rec() :
   - met a jour la base de donnee des zones, ou les adresse sont
     des adresses de bytes
   - copie les donnees dans b_to_pic[] sous forme de bytes
 */

static void lec_hex_rec( INTEL_HEX_RECORD * prec )
{
static int curadr;
int i;

if ( prec->Type == 4 )
   {
   if ( prec->Length != 2 )
      gasp("record type 04 has length %d instead of 2", prec->Length );
   curpage = ( prec->Data[0] << 8 ) | prec->Data[1];
   return;
   }
/* ici on ignore les pages de Config et ID, contrairement a UPICe */
if ( ( prec->Type == 0 ) && ( curpage == 0 ) )
   {
   curadr = ( curpage << 16 ) | prec->Address;  /* byte addr. */

   if ( curadr > nextadr ) /* gestion des discontinuites des adresses */
      {
      /* printf("saut : %04x, au lieu de %04x\n", curadr, nextadr ); */	
      if   ( qzon )  /* fin de la zone courante */
           zoneb[qzon-1].a2 = nextadr;
      if   ( qzon <= QZON ) /* creer nouvelle zone */
	   zoneb[qzon++].a1 = curadr;
      else gasp("trop (%d) de zones", qzon+1 );
      }
   if ( curadr < nextadr ) /* detection retour arriere */
      gasp("retour arriere adr. %x sur %x dans fichier hex",
            curadr, nextadr );
   for ( i = 0; i < prec->Length; i++ )
      if   ( curadr >= 0x8000 )	// contrairement a UPICe
           gasp("adresse > 8000 : %x", curadr );
      else b_to_pic[curadr++] = prec->Data[i];
   nextadr = curadr;
   return;
   }
}

/* ===================== traiements PIC - specifiques  ============================= */

/* fonction de preparation des zones :
   - les zones separees par moins de N bytes sont fusionnees
   - les trous sont bouches avec des FF
   - les frontieres de zones sont arrondies aux multiples de N
   - les trous sont bouches avec des FF
 */
static void prep_zones()
{
int N, iz, jz, a, r;
N = 64;
/* fusions */
if ( qzon >= 2 )
   {
   iz = 1;
   while ( iz < qzon )
      {
      if   ( ( zoneb[iz].a1 - zoneb[iz-1].a2 ) < N )
           {
           /* printf("fusion decidee de %x a %x\n", 
                  zoneb[iz-1].a2, zoneb[iz].a1 ); */
	   for ( a = zoneb[iz-1].a2; a < zoneb[iz].a1; a++ )
               b_to_pic[a] = 0xFF;
	   zoneb[iz-1].a2 = zoneb[iz].a2; /* zone iz-1 agrandie */
	   qzon -= 1;			  /* zone iz ecrasee */
	   for ( jz = iz; jz < qzon; jz++ )
	       zoneb[jz] = zoneb[jz+1];
	   /* on n'incremente pas iz parceque c'est qzon qui a -- */
           }
      else iz++;
      }
   }
/* arrondis */
for ( iz = 0; iz < qzon; iz++ )
    {
    r = zoneb[iz].a1 % N;
    if ( r )
       {
       for ( a = zoneb[iz].a1 - r; a < zoneb[iz].a1; a++ )
	   b_to_pic[a] = 0xFF;
       zoneb[iz].a1 -= r;
       }

    r = zoneb[iz].a2 % N;
    if ( r )
       {
       r = N - r;
       for ( a = zoneb[iz].a2; a < zoneb[iz].a2 + r; a++ )
	   b_to_pic[a] = 0xFF;
       zoneb[iz].a2 += r;
       }
    }
/* verif (ici on accepte que les zones se touchent, ce n'est pas grave) */
if ( qzon >= 2 )
   for ( iz = 1; iz < qzon; iz++ )
       if ( zoneb[iz].a1 < zoneb[iz-1].a2 )
          gasp("recouvrement interne %x %x", zoneb[iz-1].a2, zoneb[iz].a1 ); 
/* preparation des zones specifique F18 :
   - la zone ID est ajustee a 8 bytes
   - la zone de config est ajustee a 14 bytes
   - la zone de config est masquee pour que la comparaison donne OK
 *
   for ( iz = 0; iz < qzon; iz++ )
       {
       if ( zoneb[iz].a1 == 0x200000 )
          zoneb[iz].a2 = 0x200008;
       if ( zoneb[iz].a1 == 0x300000 )
	  {
	  int a;
          zoneb[iz].a2 = 0x30000E;
	  a = zoneb[iz].a1 >> 1;
	  w_to_pic[a++] &= 0x2700;
	  w_to_pic[a++] &= 0x0F0F;
	  w_to_pic[a++] &= 0x0100;
	  w_to_pic[a++] &= 0x0085;
	  w_to_pic[a++] &= 0xC00F;
	  w_to_pic[a++] &= 0xE00F;
	  w_to_pic[a++] &= 0x400F;
	  }
       }
*/
}

static void verif( int a1, int a2 )   /* comparaison en memoire */
{
int adr, errcnt;
errcnt = 0;
for ( adr = (a1>>1); adr < (a2>>1); adr++ )
    if   ( w_from_pic[adr] != w_to_pic[adr] )
         { printf("%04x : %04x au lieu de %04x\n",
                   (adr<<1), w_from_pic[adr], w_to_pic[adr] );
           errcnt++;
         }
printf("de %04x a %04x : ", a1, (a2-1) );

if   ( errcnt )
     printf("%d ERREURS\n", errcnt );
else printf("comparaison OK\n" );
}

/* fonction de comparaison de mots :
 - rend zero si egalite 
 - rend -1 si prog impossible 
   ( to contient des uns a des endroits ou from a deja des zeros ) 
 - rend 1 sinon <==> prog possible
 */
static int comp_bits( int from, int to )
{
int x;
if ( from == to ) return(0);
x = to & ( ~from );
if ( x ) return(-1);
return(1);
}


/* ========================== acces au PIC 18F ===================== */

/* ATTENTION ACHTUNG UWAGA
   Les adresses pointant sur la memoire programme du PIC sont le plus
   souvent des adresses de bytes, notamment :
   - pour chargement de TBLPTR*
   - dans les ORG, et la table de symboles de MPASM
   - dans nos fonctions jump18, lec_PIC18, aff_param...
   Ce sont cependant des adresses de words dans :
   - nos tableaux w_from_pic et w_to_pic, pour garder la compatibilite
     avec la lecture de fichier hex des PICs 16C/16F
   - dans le code binaire des goto, call, etc pour economiser 1 bit
 */

static irblock irb;

void lec_PIC18( int a1, int a2 ) /* lecture de a1 (inclus) a a2 (exclu) */
{
int adr, got;

for ( adr = a1; adr < a2; adr+=8 )
    {
    irb.txbuf[0] = FLASHR;
    irb.txbuf[1] = (unsigned char)adr;
    irb.txbuf[2] = (unsigned char)(adr >> 8);
    irb.txcnt = 3; irb.rxcnt = 9;
    got = dial( &irb );
    if ( got )
       gasp("echec dialogue");
    memcpy( b_from_pic + adr, irb.rxbuf + 1, 8 );
    if ( ( adr & 0x3F ) == 0 )
       { printf("."); fflush(stdout);  }
    }
printf("\n");
}

void ecr8_PIC18( int a ) /* ecriture 8 bytes = 4 words consecutifs */
{
int got;

if ( a % 8 )
   gasp("block write only on 8-byte boundary (%04X)", a );
irb.txbuf[0] = FLASHW;
irb.txbuf[1] = (unsigned char)a;
irb.txbuf[2] = (unsigned char)(a >> 8);
memcpy( irb.txbuf + 3, b_to_pic + a, 8 );
irb.txcnt = 11; irb.rxcnt = 1;
got = dial( &irb );
if ( got )
   gasp("echec dialogue");
}

void eff64_PIC18( int a ) /* effacement 64 bytes = 32 words consecutifs */
{
int got;

if ( a % 64 )
   gasp("block erase only on 64-byte boundary (%04X)", a );
irb.txbuf[0] = FERASE;
irb.txbuf[1] = (unsigned char)a;
irb.txbuf[2] = (unsigned char)(a >> 8);
irb.txcnt = 3; irb.rxcnt = 1;
got = dial( &irb  );
if ( got )
   gasp("echec dialogue");
}

// encodage a adr d'un goto pour dest
void encode_goto( unsigned int adr, unsigned int dest )
{
adr >>= 1;
dest >>= 1;	// word adr.
w_to_pic[adr]   = 0xEF00 | ( dest & 0xFF );
w_to_pic[adr+1] = 0xF000 | ( dest >> 8 ); 
}

// decodage a adr d'un goto (-1 si pas de goto)
int decode_goto( unsigned int adr )
{
int retval;
adr >>= 1;
if   (   
     ( ( w_from_pic[adr]   & 0xFF00 ) != 0xef00 ) ||
     ( ( w_from_pic[adr+1] & 0xFF00 ) != 0xf000 )
     ) return(-1);
retval = ( w_from_pic[adr+1] & 0xFF ) << 8;
retval |= w_from_pic[adr] & 0xFF;
retval <<= 1;
return( retval );
}

void ecr64_PIC18( int a ) /* ecriture 64 bytes automatique */
{
int wad, impcnt, diffcnt, i, j;
if ( a % 64 )
   gasp("automatic block write only on 64-byte boundary (%04X)", a );
printf(" ");
/* lecture */
lec_PIC18( a, a + 64 );
/* evaluation ecrivabilite */
impcnt = 0;
for ( wad = (a>>1); wad < ((a+64)>>1); wad++ )
    if ( comp_bits( w_from_pic[wad], w_to_pic[wad] ) < 0 )
       impcnt++;
/* effacement eventuel */
if ( impcnt )
   {
   eff64_PIC18( a );
   printf("@");
   for ( wad = (a>>1); wad < ((a+64)>>1); wad++ )
       w_from_pic[wad] = 0xFFFF;
   }
/* traitement par blocs de 8 bytes */
for ( i = 0; i < 8; i++ )
    {
    diffcnt = 0;
    wad = ( a + (i << 3) ) >> 1;
    for ( j = 0; j < 4; j++ )
        {
        if ( w_from_pic[wad] != w_to_pic[wad] )
           diffcnt++;
	wad++;
        }
    if   ( diffcnt )	/* ecriture si necessaire */
	 {
         ecr8_PIC18( a + (i << 3) );
	 printf("1");
	 }
    else printf("0");
    }
}

void ecr_PIC18( int a1, int a2 )	/* adresses de bytes */
{
int a;
for ( a = a1; a < a2; a += 64 )
    ecr64_PIC18( a );
printf("\n");
}

/* ====================== interface utilisateur debug flash =============== */

void flash_usage()
{
   printf("\n=== acces %s === memoire Flash du PIC ===\n", dialogue_get_acces_text() );
   printf("  raaaa : lire 8 bytes\n");
   printf("  Raaaa : lire 64 bytes\n");
   printf("  waaaa : ecrire 8 random bytes\n");
// printf("  gaaaa : ecrire en 0500 un goto a l'adresse aaaa\n");
   printf("  Eaaaa : effacer 64 bytes\n");
   printf("  q : quitter ce menu\n\n");
}

void flash_ui()
{
char locar; char text[7];
int fin; int i, val;
flash_usage();
fin = 0;
while ( ! fin )
   {
   int qbytes = 8;
   locar = getchar();
   switch( locar )
     {
     case 'R' : qbytes = 64;
     case 'r' : for ( i = 0; i < 4; i++ )
                    text[i] = (unsigned char) getchar();
		text[4] = 0;
                sscanf( text, "%x", &val );
		lec_PIC18( val, val + qbytes );	
		for ( i = 0; i < (qbytes>>1); i++ )
		    printf(" %04x", w_from_pic[(val>>1) + i] );
		printf("\n");
		val = decode_goto(val);
		if ( val >= 0 )
		   printf("--> goto %04x\n", val );	break;
     case 'w' : qbytes = 8;
		for ( i = 0; i < 4; i++ )
                    text[i] = (unsigned char) getchar();
		text[4] = 0;
                sscanf( text, "%x", &val );
		for ( i = 0; i < (qbytes>>1); i++ )
		    {
		    w_to_pic[(val>>1) + i] = rand();
		    printf(" %04x", w_to_pic[(val>>1) + i] );
		    }
                ecr8_PIC18( val );
		printf("\n");			break;
/*
     case 'g' : {
		unsigned int adr = 0x500;
		qbytes = 64;
		for ( i = 0; i < 4; i++ )
                    text[i] = (unsigned char) getchar();
		text[4] = 0;
                sscanf( text, "%x", &val );
		lec_PIC18( adr, adr + qbytes );
		memcpy( b_to_pic + adr, b_from_pic + adr, qbytes );
		encode_goto( adr, val );
		for ( i = 0; i < (qbytes>>1); i++ )
		    printf(" %04x", w_to_pic[(adr>>1) + i] );
		eff64_PIC18( adr );
		for ( i = 0; i < 8; i++ )
                    { ecr8_PIC18( adr ); adr += 8; }
		}
		printf("\n");			break;
//*/
     case 'E' : for ( i = 0; i < 4; i++ )
                    text[i] = (unsigned char) getchar();
		text[4] = 0;
                sscanf( text, "%x", &val );
                eff64_PIC18( val );
		printf("\n");			break;
     case ' ' : flash_usage();			break;
     case 'q' : fin++;				break;
     default  : 				break;
     }
   }
}

/* ====================== gestion securisee bootloader ================ */

// donnees du bootloader Frevo7
int FinProt = 0;	// fin zone protegee
int FinProt2 = 0;	// fin zone protegee
int Flask;		// adresse du mini-startup du bootloader I2C
int Flusk;		// adresse du mini-startup du bootloader UDP
int IPbase = 0x3fC0;	// adresse fonction copy_IP_MAC pour adresse reseau

void lect_disk();

// verification que le nouveau hex contient le meme loader
void verif_loader()
{
int i, fp;
if   ( dialogue_get_acces() == 'u' )
     fp = FinProt2;
else fp = FinProt;
for ( i = 0; i < qzon; i++ )
    if  ( zoneb[i].a1 < fp )
	{
	if   ( zoneb[i].a2 > fp )
             lec_PIC18( zoneb[i].a1, fp );
	else lec_PIC18( zoneb[i].a1, zoneb[i].a2 );
		        }
for ( i = 0; i < qzon; i++ )
    if  ( zoneb[i].a1 < fp )
	{
	if   ( zoneb[i].a2 > fp )
	     verif( zoneb[i].a1, fp );
	else verif( zoneb[i].a1, zoneb[i].a2 );
	}
}

int certify_comm()	/* code commun pour certifications PIC et HEX */
{
int val, adr;
// verification de la zone vect2 (vecteurs non proteges)
val = decode_goto( FinProt );	// goto vers _startup --> main()
if   ( val < 0 )		// pas de goto, on doit avoir des NOPs
     {
     int a; adr = FinProt>>1;
     for ( a = adr; a < adr + 0x20; a++ )
         if ( w_from_pic[a] != 0xFFFF )
	    { printf("Hum.. %04X : vu %04X != NOP\n", a, w_from_pic[a] ); return(-9); }
     printf("%04X <FinPr> : 32 NOPs\n", FinProt );
     }
else printf("%04X <FinPr> : GOTO %04X <_startup>\n", FinProt, val );

val = decode_goto( FinProt + 0x40 );	// goto de secours
if   ( val == 0x10 )
     printf("%04X <Salva> : GOTO %04X <BootL>\n", FinProt + 0x40, val );
else { printf("Hum.. %04X <Salva> : GOTO %04X <BootL> != 0x0010\n", FinProt + 0x40, val );
       return(-10); }

val = decode_goto( FinProt + 0x44 );	// goto irtnl --> irtnl2
if   ( val < 0 )
     if   ( w_from_pic[(FinProt+0x44)>>1] == 0x0012 )
          printf("%04X <irtnl> : RETURN (no low priority interrupt)\n", FinProt + 0x44 );
     else { printf("%04X <irtnl> : vu %04X != RETURN\n", FinProt + 0x44, val );
            return(-11); }
else printf("%04X <irtnl> : GOTO %04X <irtnl2>\n", FinProt + 0x44, val );
return(0);
}

int certify_comm2()	/* code commun pour certifications PIC et HEX */
{
int val, adr;
// verification de la zone vect3 (vecteurs non proteges)
val = decode_goto( FinProt2 );	// goto vers _startup, --> main()
if   ( val < 0 )		// pas de goto, on doit avoir des NOPs
     {
     int a; adr = FinProt2>>1;
     for ( a = adr; a < adr + 0x20; a++ )
         if ( w_from_pic[a] != 0xFFFF )
	    { printf("Hum.. %04X : vu %04X != NOP\n", a, w_from_pic[a] ); return(-9); }
     printf("%04X <FinPr2> : 32 NOPs\n", FinProt );
     }
else printf("%04X <FinPr2> : GOTO %04X <_startup>\n", FinProt2, val );

val = decode_goto( FinProt2 + 0x40 );	// goto de secours
if   ( val < 0 )		// pas de goto, pas de flusk
     { printf("Hum.. %04X <FinPr2+64> : pas de goto\n", FinProt2 + 0x40 ); return(-6);  }
else if   ( val < FinProt2 )
	  { printf("%04X <FinPr2+64> : GOTO %04X <Flusk>\n", FinProt2 + 0x40, val ); Flusk = val; }
     else { printf("Hum.. %04X <FinPr2+64> : GOTO %04X <Flusk> >= FinProt2\n", FinProt2 + 0x40, val );
            return(-5);  }
return(0);
}

int certify_PIC2();

int certify_PIC()	/* inspecter le bootloader I2C Frevo 7 du PIC (mettre a jour FinProt) */
{
int val, adr;
if   ( dialogue_get_acces() == 'u' )
     return( certify_PIC2() );

printf("\nAnalyse PIC :\n");
lec_PIC18( 0, 0x20 );	// lire les 4 vecteurs proteges

val = decode_goto( 0 );
if   ( val < 0 )
     { printf("Hum.. 0000 <Reset> : pas de goto\n"); return(-1);  }
else { printf("0000 <Reset> : GOTO %04X <FinProt>\n", val ); FinProt = val; }

if ( FinProt & 63 )
   { printf("Hum.. FinProt=%04X n'est pas multiple de 64\n", FinProt ); return(-12);  }

val = decode_goto( 8 );
if   ( val < 0 )
     { printf("Hum.. 0008 <IrqHi> : pas de goto\n"); return(-2);  }
else if   ( val < FinProt )
	  { printf("0008 <IrqHi> : GOTO %04X <irtnh>\n", val ); }
     else { printf("Hum.. 0008 <IrqHi> :  GOTO %04X <irtnh> >= FinProt\n", val );
            return(-3);  }

val = decode_goto( 0x10 );
if   ( val < 0 )
     { printf("Hum.. 0010 <BootL> : pas de goto\n"); return(-4);  }
else if   ( val < FinProt )
	  { printf("0010 <BootL> : GOTO %04X <Flask>\n", val ); Flask = val; }
     else { printf("Hum.. 0010 <BootL> : GOTO %04X <Flask> >= FinProt\n", val );
            return(-5);  }

val = decode_goto( 0x18 );
if   ( val < 0 )
     { printf("Hum.. 0018 <IrqLo> : pas de goto\n"); return(-6);  }
else if   ( val == ( FinProt + 0x44 ) )
	  { printf("0018 <IrqLo> : GOTO %04X <irtnL> = FinProt + 0x44\n", val ); }
     else { printf("Hum.. 0018 <IrqLo> : GOTO %04X <irtnL> != FinProt+0x44\n", val );
            return(-7);  }

lec_PIC18( Flask, Flask + 8 );	// lire le debut du mini-startup
adr = Flask>>1;
if   (				// detecter initialisation stack
     ( ( w_from_pic[adr]   & 0xFFF0 ) != 0xee10 ) ||
     ( ( w_from_pic[adr+2] & 0xFFF0 ) != 0xee20 ) ||
     (   w_from_pic[adr+1]            != 0xf000 ) ||
     (   w_from_pic[adr+3]            != 0xf000 )
     )
     { printf("Hum.. %04X <Flask> : pas de mini-startup\n", Flask ); return(-8); }
else printf("%04X <Flask> : vu mini-startup (lfsr 1,_stack lfsr 2,_stack)\n", Flask );

lec_PIC18( FinProt, FinProt + 0x48 );	// lire les vecteurs non proteges
val = certify_comm();
if ( val ) return( val );

printf("PIC : Certification Frevo 7 Ok\n"); return(0);
}

int certify_PIC2()	/* inspecter le bootloader UDP du PIC (mettre a jour Flusk et FinProt2) */
{
int val, adr;
printf("\nAnalyse PIC :\n");
lec_PIC18( 0, 0x20 );	// lire les 4 vecteurs proteges

val = decode_goto( 0 );
if   ( val < 0 )
     { printf("Hum.. 0000 <Reset> : pas de goto\n"); return(-1);  }
else { printf("0000 <Reset> : GOTO %04X <FinProt>\n", val ); FinProt = val; }

if ( FinProt & 63 )
   { printf("Hum.. FinProt=%04X n'est pas multiple de 64\n", FinProt ); return(-12);  }

val = decode_goto( 0x10 );
if   ( val < 0 )
     { printf("Hum.. 0010 <BootL> : pas de goto\n"); return(-4);  }
else if   ( val < FinProt )
	  { printf("0010 <BootL> : GOTO %04X <Flask>\n", val ); Flask = val; }
     else { printf("Hum.. 0010 <BootL> : GOTO %04X <Flask> >= FinProt\n", val );
            return(-5);  }

lec_PIC18( FinProt, FinProt + 0x48 );	// lire "vect2"
val = decode_goto( FinProt );	// goto vers FinProt2
if   ( val < 0 )		// pas de goto, pas de bootloader UDP
     { printf("Hum.. %04X <FinPr> : pas de goto\n", FinProt ); return(-5);  }
else { printf("%04X <FinPr> : GOTO %04X <FinPr2>\n", FinProt, val ); FinProt2 = val; }

if ( FinProt2 & 63 )
   { printf("Hum.. FinProt2=%04X n'est pas multiple de 64\n", FinProt2 ); return(-13);  }

lec_PIC18( FinProt2, FinProt2 + 0x48 );	// lire les vecteurs non proteges
val = certify_comm2();
if ( val ) return( val );

lec_PIC18( Flusk, Flusk + 8 );	// lire le debut du mini-startup
adr = Flusk>>1;
if   (				// detecter initialisation stack
     ( ( w_from_pic[adr]   & 0xFFF0 ) != 0xee10 ) ||
     ( ( w_from_pic[adr+2] & 0xFFF0 ) != 0xee20 ) ||
     (   w_from_pic[adr+1]            != 0xf000 ) ||
     (   w_from_pic[adr+3]            != 0xf000 )
     )
     { printf("Hum.. %04X <Flusk> : pas de mini-startup\n", Flusk ); return(-8); }
else printf("%04X <Flusk> : vu mini-startup (lfsr 1,_stack lfsr 2,_stack)\n", Flusk );

printf("PIC : Certification Frevo 7 Ok\n"); return(0);
}

int certify_HEX2();

int certify_HEX()	/* inspecter le bootloader I2C Frevo 7 du code HEX */
{			/* verifier compatibilite avec FinProt du PIC */
int val;
if   ( dialogue_get_acces() == 'u' )
     return( certify_HEX2() );
printf("\nAnalyse HEX :\n");
if ( qzon == 0 ) lect_disk();

memcpy( b_from_pic + FinProt, b_to_pic + FinProt, 0x48 );
val = certify_comm();
if ( val ) return( val );

printf("HEX : Certification Frevo 7 Ok\n"); return(0);
}

int certify_HEX2()	/* inspecter le bootloader UDP Frevo 7 du code HEX */
{			/* verifier compatibilite avec FinProt2 du PIC */
int val, PICFlusk;
printf("\nAnalyse HEX :\n");
if ( qzon == 0 ) lect_disk();

memcpy( b_from_pic + FinProt2, b_to_pic + FinProt2, 0x48 );
PICFlusk = Flusk;
val = certify_comm2();
if ( val ) return( val );

if ( Flusk != PICFlusk )
   printf("Hum.. %04X<FinPr2+64> : GOTO %04X <Flusk> != Flusk\n", FinProt2 + 0x40, Flusk );

printf("HEX : Certification Frevo 7 Ok\n"); return(0);
}


// ecriture securisee
void secu_prog()
{
int i, adr1, adr2, val, fp;

// analyser le PIC, lire FinProt
val = certify_PIC();
if ( val != 0 )
   { printf("Sorry...\n"); return;  }

// verifier la conformite du code avec FinProt du PIC
val = certify_HEX();
if ( val != 0 )
   { printf("Sorry...\n"); return;  }

if   ( dialogue_get_acces() == 'u' )
     fp = FinProt2;
else fp = FinProt;

// simplement par effacement du goto _startup, on commute sur
// le goto de securite
lec_PIC18( fp, fp + 8 );	// juste pour envoyer un FLASHR
eff64_PIC18( fp );

// eviter la zone protegee et le zone de commutation
for ( i = 0; i < qzon; i++ )
    {
    adr1 = zoneb[i].a1; adr2 = zoneb[i].a2;
    if ( adr2 > (fp+64) )
       ecr_PIC18(  (adr1<(fp+64))?(fp+64):(adr1),  adr2  );
    }
// connecter la nouvelle appli
ecr_PIC18( fp, (fp+64) );
}

/* la fonction a patcher pour changer IP
                                    void copy_IP_MAC5()
                                    {
  0100     MOVLB     0x0            myIP[0] = 192;
  0ec0     MOVLW     0xc0
  6f85     MOVWF     0x85,0x1
  0ea8     MOVLW     0xa8           myIP[1] = 168;
  6f86     MOVWF     0x86,0x1 
  0e01     MOVLW     0x1            myIP[2] = 1;
  6f87     MOVWF     0x87,0x1
  0e59     MOVLW     0x59           myIP[3] = 89;
  6f88     MOVWF     0x88,0x1 
  0100     MOVLB     0x0            myMAC5  = 9;
  0e09     MOVLW     0x9
  6f89     MOVWF     0x89,0x1
  0012     RETURN    0x0            }
*/

int show_IP()
{
int adr;
lec_PIC18( IPbase, IPbase + 0x40 );	// lire la section ipnum_scn
adr = IPbase + 3;
if ( b_from_pic[adr] != 0x0e )
   { printf("manque 1er MOVLW @ %04X\n", adr ); return(-1);  }
adr = IPbase + 7;
if ( b_from_pic[adr] != 0x0e )
   { printf("manque 2nd MOVLW @ %04X\n", adr ); return(-1);  }
adr = IPbase + 11;
if ( b_from_pic[adr] != 0x0e )
   { printf("manque 3e  MOVLW @ %04X\n", adr ); return(-1);  }
adr = IPbase + 15;
if ( b_from_pic[adr] != 0x0e )
   { printf("manque 4e  MOVLW @ %04X\n", adr ); return(-1);  }
adr = IPbase + 21;
if ( b_from_pic[adr] != 0x0e )
   { printf("manque 5e  MOVLW @ %04X\n", adr ); return(-1);  }
printf("Vu adresse IP=%d.%d.%d.%d, MAC5=0x%02X\n",
	b_from_pic[IPbase+2],  b_from_pic[IPbase+6],
	b_from_pic[IPbase+10], b_from_pic[IPbase+14],
	b_from_pic[IPbase+20] );
return(0);
}

void change_IP()
{
int ip0, ip1, ip2, ip3, mac5;
char locar; int fin=0;
if   ( show_IP() )
     return;
fflush( stdin );
printf("Nouvelle adresse IP ? (ou 0 pour laisser inchangee) ");
if ( scanf("%d.%d.%d.%d", &ip0, &ip1, &ip2, &ip3 ) != 4 )
   return;
printf("Nouvelle adresse MAC (1 byte hex) ? ");
if ( scanf("%x", &mac5 ) != 1 )
   return;
ip0 &= 0xFF; ip1 &= 0xFF; ip2 &= 0xFF; ip3 &= 0xFF;
fflush( stdin );
printf("Ok pour %d.%d.%d.%d avec MAC5=0x%02X ? (y/n)\n",
	ip0, ip1, ip2, ip3, mac5 );
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'n' : return;		break;
     case 'y' : fin = 1;	break;
     default  : 		break;
     }
   }
memcpy( b_to_pic + IPbase, b_from_pic + IPbase, 0x40 );
b_to_pic[IPbase+2]  = ip0; b_to_pic[IPbase+6]  = ip1;
b_to_pic[IPbase+10] = ip2; b_to_pic[IPbase+14] = ip3;
b_to_pic[IPbase+20] = mac5;
ecr_PIC18( IPbase, (IPbase+0x40) );
show_IP();
}

/* ====================== interface utilisateur flashage HEX ================== */

char nom[1024];

void traite_nom()
{
int l;
if ( *nom )
   {
   l = strlen( nom );
   if (
      ( nom[l-4] != '.' ) |
      (  ( nom[l-3] != 'h' )  &&  ( nom[l-3] != 'H' )  )
      )
      sprintf( nom+l, ".hex" );
   }
}

void lect_disk()
{
if ( *nom == 0 )
   {
   fflush( stdin );
   printf("nom du programme a charger : ");
   scanf("%s", nom );
   }
traite_nom();
lec_hex_file( nom );
prep_zones();
}

void aff_param()
{
int i;

if ( ( qzon == 0 ) && ( *nom ) ) lect_disk();
if ( qzon )
   {
   printf("Ficher %s\n", nom );

        for ( i = 0; i < qzon; i++ )
            {
            printf("- de %04x a %04x incl. : %4d words\n",
                   zoneb[i].a1, zoneb[i].a2-1,
                   (zoneb[i].a2 - zoneb[i].a1) / 2 );
	    /* {
	    int a;
	    for ( a = zoneb[i].a1; a < zoneb[i].a2; a+=2 )
                printf("  %04X\n", w_to_pic[a>>1] );
	    } */
            }
   }
}


void prog_usage()
{
   printf("\n=== acces %s ===\n", dialogue_get_acces_text() );
   printf("  n : ouverture nouveau fichier HEX\n");
   printf("  r : relecture du ficher %s\n", nom );
   printf("-----\n");
   printf("  z : verification zone protegee 0-%04x vs fichier %s\n",
             (dialogue_get_acces()=='u')?(FinProt2):(FinProt), nom );
   printf("  p : certification du PIC vs Frevo 7\n");
   printf("  h : certification du fichier HEX vs Frevo 7 du PIC\n");
   printf("  E : ecriture securisee selon fichier %s\n", nom );
   printf("  l : lecture/comparaison avec fichier %s\n", nom );
   printf("  a : voir/changer adresse IP et MAC\n" );
   printf("-----\n");
   printf("  # : ecriture brute selon fichier %s\n", nom );
   printf("  q : quitter\n\n");
}

void prog_ui( char * lenom )
{
char locar; int i, fin;
qzon = 0; /* indique qu'aucun fichier n'a ete lu */
strncpy( nom, lenom, 1024 );
if ( *nom )
   traite_nom(); 
prog_usage();
fin = 0;
while ( ! fin )
   {
   locar = getchar();
   switch( locar )
     {
     case 'R' :
     case 'r' : lect_disk(); aff_param();
		prog_usage();	break;
     case 'N' :
     case 'n' : *nom = 0; lect_disk(); aff_param();
		prog_usage();	break;

     case '#' : if ( qzon == 0 ) lect_disk();
                 for ( i = 0; i < qzon; i++ )
		     ecr_PIC18( zoneb[i].a1, zoneb[i].a2 );
		prog_usage();	break;

     case 'z' : if ( qzon == 0 ) lect_disk();
                if ( FinProt == 0 ) certify_PIC();
		verif_loader();
		prog_usage();	break;

     case 'p' : certify_PIC();
		prog_usage();	break;

     case 'h' : if ( qzon == 0 ) lect_disk();
                if ( FinProt == 0 ) certify_PIC();
                certify_HEX();
		prog_usage();	break;

     case 'E' : secu_prog();
		prog_usage();	break;

     case 'l' : if ( qzon == 0 ) lect_disk();
                 for ( i = 0; i < qzon; i++ )
		     lec_PIC18( zoneb[i].a1, zoneb[i].a2 );
                 for ( i = 0; i < qzon; i++ )
                     verif( zoneb[i].a1, zoneb[i].a2 );
		prog_usage();	break;

     case 'a' : change_IP();
		prog_usage();	break;

     case 'q' : fin++; break;

     case ' ' : aff_param();
		prog_usage();	break;
     default : 			break;
     }
   }
}
