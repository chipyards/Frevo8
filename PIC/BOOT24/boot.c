// ici seulement le code pour identifier et verifier les applis
// bootables en ROM

#include <p24Fxxxx.h>

#include "../../mpar.h"
#include "boot.h"
#include "flask.h"


// un peu de variables globales
extern unsigned char rowbuf[QROWBYTES];

// fonction inspiree du livre de Bentham
// *pcrc doit etre initialise a 0xffffffff
// et complemente en fin de calcul
void update_crc( unsigned long * pcrc, unsigned char byte )
{
int i; unsigned char lsb;
for ( i = 0; i < 8; i++ )
    {
    lsb = (unsigned char)*pcrc;
    *pcrc >>= 1;
    if ( ( lsb ^ byte ) & 1 )
       *pcrc ^= POLYNOM;
    byte >>= 1;
    }
}


// verif du crc de la mise a jour en ROM
// la taille entiere de la mise a jour est supposee multiple de QROWPCUNITS
// retour 0 si Ok
int boot_verif( unsigned long romadr, unsigned int * majentry, unsigned long * plenPC )
{
// N.B. lenPC est signed pour detecter le franchisement de zero par soustraction
long lenPC;
unsigned long crc, origcrc;
int i;

// lecture de la remiere row
flashr( romadr, rowbuf );

// est-ce a la bonne place ?
if ( ((unsigned long *)rowbuf)[2] != romadr )
   return 20;

// verifier que le point d'entree tient sur 16 bits
if ( ((long *)rowbuf)[3] & 0xFFFF0000 )
   return 21;

origcrc   = ((long *)rowbuf)[0];
lenPC     = ((long *)rowbuf)[1];
*majentry = (unsigned int)((long *)rowbuf)[3];

*plenPC = (unsigned long)lenPC;

// if ( ( romadr + lenPC ) > HIDATA )
//    return 22;

// calcul crc sur la premiere row
crc = 0xffffffff;

for ( i = 4; i < QROWBYTES; i++ )
    update_crc( &crc, rowbuf[i] );

lenPC -= (long)QROWPCUNITS;
romadr += (unsigned long)QROWPCUNITS;

while ( lenPC > 0L )
   {
   flashr( romadr, rowbuf );
   romadr += (unsigned long)QROWPCUNITS;
   lenPC -= (long)QROWPCUNITS;
   for ( i = 0; i < QROWBYTES; i++ )
       update_crc( &crc, rowbuf[i] );
   } 

crc = ~crc;

if ( crc != origcrc )
   return 23; 

return 0;
}


