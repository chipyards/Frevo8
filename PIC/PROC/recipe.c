/* Frevo 8.1 traitement recette */

#include <string.h>
#include <stdlib.h>
#include <p18cxxx.h>
#include "../../ipilot.h"
#include "recipe.h"

/* buffer pour recevoir la recette comprimee
	- clusters de 3 bytes
	- premier cluster :
		- nombre de steps (step 0 exclu) sur 1 byte
		- taille recette en bytes, sur 2 bytes
	- autres clusters :
		- 1 byte d'identification :
			- 4 MSBs = index podget
			- 4 LSBs = index du parametre dans le podget
		- 2 bytes de data
 */
#pragma udata rec_dat
#define QPACK 768
unsigned char pack[QPACK];
#pragma udata 

#pragma code jln_lib3

/* step 0 : ce step n'est jamais decrit dans les recettes et
   on peut toujours y sauter
 */

/* inititialisation des podgets pour le step 0
   (sauf DV temperatures qu'on laisse tels quel)
   cette fonction ferme les vannes d'urgence sauf VANNE0
 */
void step_zero( void )
{
LATD = VANNE0;		// pour le cas d'urgence !
LATB = 0;
etape.id = 0;
etape.deldg = 30;	// delai de grace par defaut
etape.flags = 1;	// step en pause (il l'est par principe
			// mais c'est bien de le signaler)
etape.duree = 0;
etape.vannes = VANNE0;
etape.new = 1;
mfc[0].SV = 13104;	// 1.0 V pour purge N2
mfc[0].flags = NEWSV;
mfc[1].SV = 0;
mfc[1].flags = NEWSV;
mfc[2].SV = 0;
mfc[2].flags = NEWSV;
mfc[3].SV = 0;
mfc[3].flags = NEWSV;
tem[0].flags = 0;
tem[1].flags = 0;
tem[2].flags = 0;
fre.flags = 0;
extrac.adr = 3;
}

/* initialisation temperatures au demarrage a froid */
void init_tem( void )
{
tem[0].SV = 0;	// sera remplace par lecture RV par tache tonk
tem[1].SV = 0;
tem[2].SV = 0;
}

/* copie en RAM de la recette par defaut
  (obligatoire mem si elle ne fait rien - min 6 bytes)
 */
void load_def( void )
{
memcpypgm2ram( (void *)pack, (void *)defpack, 3 );
memcpypgm2ram( (void *)pack+3, (void *)defpack+3, packlen );
extrac.adr = 3;
extrac.crc_stat = CRC_AUTOR; 	// autoriser deja
crc_start(); 			// lancer calcul initial
}

/* sauter dans un nouveau step s'il existe (sinon : step 0)
   attention :
   cette fonction initialise tout ce qu'il faut sauf etape.chrono
   elle n'interagit pas avec le hardware (sauf si dest=0)
 */
void sauter( unsigned char dest )
{
static unsigned int diff;
if ( ( dest == 0 ) || ( !( extrac.crc_stat & CRC_AUTOR ) ) )
   { extrac.adr = 3; step_zero(); return; }

// premiere phase : chercher le step dans le pack
// on teste d'abord le prochain pour gagner du temps
if ( ( pack[extrac.adr] != 0 ) || ( pack[extrac.adr+1] != dest ) )
   {
   extrac.adr = 3;
   while ( extrac.adr < packlen )	// scrutons les steps
      {
      if ( ( pack[extrac.adr] == 0 ) && ( pack[extrac.adr+1] == dest ) )
         break;	// on y est
      extrac.adr += 3;
      }
   }

// si le step n'existe pas on ne fait rien
if ( extrac.adr >= packlen )
   { extrac.adr = 3; return; }

// arreter toute rampe, tout check
mfc[0].flags = 0; mfc[1].flags = 0; mfc[2].flags = 0; mfc[3].flags = 0;
tem[0].flags = 0; tem[1].flags = 0; tem[2].flags = 0;
fre.flags = 0;

// seconde phase : extraire du pack les donnees du step courant
etape.id = pack[++extrac.adr];
etape.deldg = pack[++extrac.adr];
etape.duree = 0;		// duree par defaut
etape.vannes = VANNE0;		// vannes fermees par defaut
etape.flags = PAUSE;		// step en pause par defaut
etape.stogo = etape.id + 1;	// saut au prochain par defaut
extrac.adr++;			// on pointe sur le prochain cluster
while ( extrac.adr < packlen )	// boucle des clusters de 3 bytes
   {
   extrac.ipod = pack[extrac.adr];
   if ( extrac.ipod == 0 )	// c'est deja le prochain step, filons !
      break;			// on sort en laissant adr sur le prochain step
   extrac.ipar = extrac.ipod & 0x0F;
   extrac.ipod >>= 4;
   extrac.adr++;
   if   ( extrac.ipod == 0 )	// un parametre pour le step courant
	switch( extrac.ipar )
	   {
	   case 1 : etape.duree  = *(unsigned int *)&pack[extrac.adr];
		    etape.flags  = 0; break;
	   case 2 : etape.vannes = *(unsigned int *)&pack[extrac.adr];
		    break;
	   case 3 : etape.stogo = pack[extrac.adr];
		    break;
	   }
   else {		// un parametre pour un podget quelconque
	extrac.ipod--;			// vu qu'il y a trois sortes de podget
	if   ( extrac.ipod < QMFC )	// on utilise un pointeur ppod
	     extrac.ppod = &mfc[extrac.ipod];
	else {
	     extrac.ipod -= QMFC;
	     if   ( extrac.ipod < QTEM )
		  extrac.ppod = &tem[extrac.ipod];
	     else if   ( extrac.ipod == QTEM )
		       extrac.ppod = &fre;
	     }
	switch( extrac.ipar )
	   {
	   case 0 : extrac.ppod->SV   = *(unsigned int *)&pack[extrac.adr];
		    ((unsigned char *)&(extrac.ppod->SV))[0] &= 0xF0; // strip the 4 LSBs
		    extrac.ppod->flags = NEWSV;
		    // calcul SVmi et SVma forfaitaires selon TOLSHIFT
		    diff = extrac.ppod->SV;
		    diff >>= TOLSHIFT;
		    extrac.ppod->SVmi = extrac.ppod->SV - diff;
		    // prevention debordement SVma
		    diff += extrac.ppod->SV;
		    if   ( diff < extrac.ppod->SVma )
			 extrac.ppod->SVma = 0xFFFF;	// plafond
		    else extrac.ppod->SVma = diff;
		    break;	   
	   case 1 : extrac.ppod->SVmi = *(unsigned int *)&pack[extrac.adr];
		    break;	   
	   case 2 : extrac.ppod->SVma = *(unsigned int *)&pack[extrac.adr];
		    break;
	   case 3 : extrac.ppod->flags = NEWSV | pack[extrac.adr];
		    extrac.ppod->stogo = pack[extrac.adr+1];	   
	   }
	}
   extrac.adr += 2;	// aller au cluster suivant
   }	// fin de la boucle des clusters
if ( extrac.adr >= packlen )
   extrac.adr = 3;

// troisieme phase : mise en oeuvre
etape.new = 1;	// ce flag en conjonction avec les NEWSV des podgets
		// va activer l'action dans la boucle principale
}

/* effectuer les checks et increments de rampe
   sur le podget designe par le pointeur extrac.ppod
   cette fonction appelle sauter() le cas echeant, et rend 0 dans ce cas
   sinon rend 1
 */ 
unsigned char check_ppod( void )
{
if   ( ( extrac.ppod->flags & CHECKS ) == 0 )
     return 1;
if   ( extrac.ppod->flags & RAMPEN )
     {
     if	( etape.flags == 0 )
	{
	if   ( extrac.ppod->flags & MACEN )	// ramp up
	     {
	     if   ( extrac.ppod->SV >= extrac.ppod->SVma )
		  {
		  extrac.ppod->flags = 0;
		  etape.chrono = 0;
		  sauter( extrac.ppod->stogo ); return 0;
		  }
	     else { extrac.ppod->SV += extrac.ppod->SVmi; extrac.ppod->flags |= 1; }
	     }
	else
	if   ( extrac.ppod->flags & MICEN )	// ramp down
	     {
	     if   ( extrac.ppod->SV <= extrac.ppod->SVmi )
		  {
		  extrac.ppod->flags = 0;
		  etape.chrono = 0;
		  sauter( extrac.ppod->stogo ); return 0;
		  }
	     else { extrac.ppod->SV -= extrac.ppod->SVma; extrac.ppod->flags |= 1; }
	     }
        }
     }
else {
     if   ( extrac.ppod->flags & MACEN )	// max
	  if	( extrac.ppod->PV > extrac.ppod->SVma )
		{
		extrac.ppod->flags = 0;
		etape.chrono = 0;
		sauter( extrac.ppod->stogo ); return 0;
		}
     if   ( extrac.ppod->flags & MICEN )	// max
	  if	( extrac.ppod->PV < extrac.ppod->SVmi )
		{
		extrac.ppod->flags = 0;
		etape.chrono = 0;
		sauter( extrac.ppod->stogo ); return 0;
		}
     }
return 1;
}

/* avancer d'un bit le calcul du crc de la recette en RAM
   appeler tant que extrac.crc_stat & CRC_CALC
   ATTENTION : pour declencher le calcul il ne suffit pas de mettre
   CRC_CALC a 1 - utiliser crc_start() SVP */
void crc_cont()
{
if   ( extrac.crc_bitp == 0 )
     {
     if ( extrac.crc_adr >= packlen )
        {					// fin calcul
	extrac.crc_buf = ~extrac.crc_buf;
	if   ( extrac.crc_stat & CRC_READY )
	     {					// fin calcul de routine
	     if ( extrac.crc != extrac.crc_buf )
		{
		extrac.crc_stat = 0;	// abort !
		}
	     }
	else {					// fin calcul initial
	     extrac.crc = extrac.crc_buf;
	     extrac.crc_stat |= CRC_READY;
	     }
	extrac.crc_stat &= ~CRC_CALC;		// signaler fin calcul
	return;
	}        
     extrac.crc_shft = pack[extrac.crc_adr++];
     }
else extrac.crc_shft >>= 1;

extrac.crc_lsb = ((unsigned char *)&extrac.crc_buf)[0] ^ extrac.crc_shft;
extrac.crc_buf >>= 1;
if ( extrac.crc_lsb & 1 )
   extrac.crc_buf ^= CRC_POLY;
extrac.crc_bitp++; extrac.crc_bitp &= 7;
}

/* initialiser le calcul de crc */
void crc_start()
{
extrac.crc_buf = 0xFFFFFFFF;
extrac.crc_adr = 0;
extrac.crc_bitp = 0;
extrac.crc_stat |= CRC_CALC;
}
