/* decodage du format .hex INHX32

   - le fichier "intel HEX" meme dans sa version avancee INHX32 ne supporte
     que des donnees de 8 bits.
   - les adresses en debut d'enregistrement sont sur 16 bits big-endian (!!)
   - les enregistrements de type 04 contiennent une adresse de page aussi
     sur 16 bits big-endian dans leur champ data (-> adr. de 32 bits).
   - les data sont en bytes, qui peuvent representer des words ou longs en
     lill' ou big endian a discretion de l'utilisateur (lill' chez MCHP)
   - un enregistrement de longueur nulle est considere comme EOF

   La fonction lec_hex_file() traduit chaque record en une struct 
   INTEL_HEX_RECORD qui est passee a la fonction lec_hex_rec().
   La fonction lec_hex_file() et lec_hex_rec() n'ont rien de specifique
   du PIC, et manipulent des adresses de bytes.

   La fonction lec_hex_rec() interprete les types 0 et 4.

   ATTENTION :
   - la variable globale curpage est partagee par ces 2 fonctions

   Pour commodite, un tableau de QBLOCK blocs de taille RAWBLOCK bytes est
   renseigné : 1 si bloc modifie par le fichier hex
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "intelhex.h"

void gasp( char *fmt, ... );  /* fatal error handling */


int curpage;		/* page de 64 k bytes selon format INHX32 */

int hex_dump_flag = 0;

unsigned char rawbytes[QRAWBYTES];
unsigned char rawblocks[QBLOCKS+1];

// CONVERSION  1 char ASCII hexa -> int
int hex2i ( char ascii )
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

// on doit allouer le buffer databytes a la capacite maxqdata
// avant d'appeler cette fonction.
void lec_hex_file( const char *fnam )
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

etat = 0; curpage = 0;

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
		return;

      default : break;
      }
    }
while(1);
}

// la fonction lec_hex_rec() :
// copie les donnees dans rawbytes[], met a jour rawblocks[]
void lec_hex_rec( INTEL_HEX_RECORD * prec )
{
static int curadr;
int i;

if ( prec->Type == 4 )
   {
   if ( prec->Length != 2 )
      gasp("record type 04 has length %d instead of 2", prec->Length );
   curpage = ( prec->Data[0] << 8 ) | prec->Data[1];
   }

if ( prec->Type == 0 )
   {
   curadr = ( curpage << 16 ) | prec->Address;  /* byte addr. */

   for  ( i = 0; i < prec->Length; i++ )
	// ici on ignore les pages de Config et ID
        if  ( curadr < QRAWBYTES )
	    {
            rawbytes[curadr+i] = prec->Data[i];
	    rawblocks[(curadr+i)/RAWBLOCK] = 1;
	    }
   }

if ( hex_dump_flag & 1 )
   {
   printf("type=%2X ", prec->Type );
   if   ( prec->Type == 0 )
        printf(" adr=%4X  next=%4X,  page=%4X  full_adr=%07X : %d bytes\n",
	        prec->Address, prec->Address + prec->Length, 
                curpage, curadr, prec->Length );
   else printf("\n");
   }

if ( hex_dump_flag & 2 )
   {
   if   ( prec->Type == 0 )
	{
	printf("%08X :", curadr );
	for ( i = 0; i < prec->Length; i++ )
	    printf(" %02X", prec->Data[i] );
	printf("\n");
	}
   }

}
