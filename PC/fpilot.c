/* fonctions pour supervision 
   Dialogue I2C/UDP
   une couche au dessus de dial() (diali.c)
 */

#include <stdio.h>
#include <string.h>
#include "../ipilot.h"
#include "diali.h"
#include "fpilot.h"


void gasp( char *fmt, ... );  /* fatal error handling */

/* ================== config ===================== */

static int verbose = 1;
static FILE * logfile;
static irblock irb;

void fpilot_set_verbose( int v )
{
verbose = v; dialogue_set_verbose(verbose);
}

void fpilot_set_log( FILE * f )
{
logfile = f; dialogue_set_log( logfile );
}

void init_pilot()
{
i2c_init();
dialogue_set_acces('p');
logfile = stderr;
dialogue_set_log( logfile );

if ( verbose )
   dialogue_set_verbose(
	1 +	// de base
//	2 +	// log succes aussi
//	4 +	// messages entiers (sinon : 3 bytes)
//	8 +	// fatalisation de certaines erreurs
	0x10 +	// timestamps
	0x20 +	// erreurs UDP
//	0x40 +	// log succes UDP aussi
	0x80 +	// special suppression flush avant emission requete UDP
	0 );
}

void end_pilot()
{ i2c_disable(); }

/* ======== traitement specifique requete PFULL ========== */

// conversion 3 bytes --> 2 entiers pour requete PFULL
void trip2pair( unsigned char * triplet, status_analog * s )
{
s->sv   = triplet[1] & 0x0F;
s->sv <<= 8;
s->sv  |= triplet[0];
s->sv <<= 4;

s->pv   = triplet[2];
s->pv <<= 8;
s->pv  |= triplet[1];
s->pv &= 0xFFF0;
}

void unpack_status( status_full * pstat, unsigned char * irbdat )
{
pstat->step   = irbdat[0];
pstat->vannes = irbdat[1] | ( irbdat[2] << 8 );
pstat->chrono = irbdat[3] | ( irbdat[4] << 8 );
pstat->flags = irbdat[5];
trip2pair( irbdat+6,  &(pstat->mfc[0]) ); 
trip2pair( irbdat+9,  &(pstat->mfc[1]) ); 
trip2pair( irbdat+12, &(pstat->mfc[2]) ); 
trip2pair( irbdat+15, &(pstat->mfc[3]) );
// irbdat[18] est inutilise
pstat->frequ = irbdat[19] | ( irbdat[20] << 8 );
trip2pair( irbdat+21, &(pstat->temp[0]) );
trip2pair( irbdat+24, &(pstat->temp[1]) );
trip2pair( irbdat+27, &(pstat->temp[2]) );
}

void get_status( status_full * pstat )
{
irb.txbuf[0] = PFULL;
irb.txcnt = 1; irb.rxcnt = 31;
if ( pstat->qirbring >= 60 )
   pstat->qirbring = 0;
if   ( dial( &irb ) )
     {
     fprintf( logfile, "status_get : echec dialogue\n" );
     pstat->step = -1;
     // copier data (nulles) dans buffer circulaire pour plot
     memset( pstat->irbring[pstat->qirbring], 0, 30 );
     pstat->qirbring += 1;
     }
else {
     /* les lignes suivantes sont remplacees par l'appel a
	unpack_status(), pour eviter toute duplication de code
     pstat->step   = irb.rxbuf[1];
     pstat->vannes = irb.rxbuf[2] | ( irb.rxbuf[3] << 8 );
     pstat->chrono = irb.rxbuf[4] | ( irb.rxbuf[5] << 8 );
     pstat->flags = irb.rxbuf[6];
     trip2pair( irb.rxbuf+7,  &(pstat->mfc[0]) ); 
     trip2pair( irb.rxbuf+10, &(pstat->mfc[1]) ); 
     trip2pair( irb.rxbuf+13, &(pstat->mfc[2]) ); 
     trip2pair( irb.rxbuf+16, &(pstat->mfc[3]) );
     // irb.rxbuf[19] est inutilise
     pstat->frequ = irb.rxbuf[20] | ( irb.rxbuf[21] << 8 );
     trip2pair( irb.rxbuf+22, &(pstat->temp[0]) );
     trip2pair( irb.rxbuf+25, &(pstat->temp[1]) );
     trip2pair( irb.rxbuf+28, &(pstat->temp[2]) );
     */
     unpack_status( pstat, irb.rxbuf+1 );
     // copier data brutes dans buffer circulaire pour plot
     memcpy( pstat->irbring[pstat->qirbring], irb.rxbuf+1, 30 );
     pstat->qirbring += 1;
     }
//if ( verbose )
//       printf( "status_get  -> %x %x %x %x %x %x etc...\n",
//                irb.rxbuf[0], irb.rxbuf[1], irb.rxbuf[2], irb.rxbuf[3], irb.rxbuf[4], irb.rxbuf[5] );
}

/* ============ traitement requetes simples =========== */

void set_manu( int flag )
{
irb.txbuf[0] = PMANU;
irb.txbuf[1] = (unsigned char)flag;
if ( verbose )
   fprintf( logfile, "manual_call\n" );
irb.txcnt = 2; irb.rxcnt = 1;
if ( dial( &irb ) )
   fprintf( logfile, "manual_call : echec dialogue\n" );;
}

void set_step( int step )
{
irb.txbuf[0] = PSTEP;
irb.txbuf[1] = (unsigned char) step;
if ( verbose )
   fprintf( logfile, "set_step_call\n" );
irb.txcnt = 2; irb.rxcnt = 2;
if ( dial( &irb ) )
   fprintf( logfile, "set_step_call : echec dialogue\n" );;
}

void set_van( int ivan )
{
irb.txbuf[0] = VAON | ivan;
if ( verbose )
   fprintf( logfile, "van_call %d set\n", ivan );
irb.txcnt = 1; irb.rxcnt = 1;
if ( dial( &irb ) )
   fprintf( logfile, "van_call : echec dialogue\n" );;
}

void reset_van( int ivan )
{
irb.txbuf[0] = VAOFF | ivan;
if ( verbose )
   fprintf( logfile, "van_call %d reset\n", ivan );
irb.txcnt = 1; irb.rxcnt = 1;
if ( dial( &irb ) )
   fprintf( logfile, "van_call : echec dialogue\n" );;
}

void set_dac( int idac, int val )
{
irb.txbuf[0] = DAC | idac;
irb.txbuf[1] = val & 0xFF;
irb.txbuf[2] = val >> 8;
if ( verbose )
   fprintf( logfile, "mfc_call %d -> %d\n", idac, val );
irb.txcnt = 3; irb.rxcnt = 1;
if ( dial( &irb ) )
   fprintf( logfile, "van_call : echec dialogue\n" );;
}

void set_temp( int item, int val )
{
irb.txbuf[0] = TCONS | item;
irb.txbuf[1] = val & 0xFF;
irb.txbuf[2] = val >> 8;
if ( verbose )
   fprintf( logfile, "temp_call %d -> %d\n", item, val );
irb.txcnt = 3; irb.rxcnt = 1;
if ( dial( &irb ) )
   fprintf( logfile, "temp_call : echec dialogue\n" );;
}

void set_chron( int t )
{
irb.txbuf[0] = PCHRON;
*((unsigned short *)&(irb.txbuf[1])) = (unsigned short)( t );
irb.txcnt = 3; irb.rxcnt = 1;
if ( dial( &irb ) )
   fprintf( logfile, "set_chron : echec dialogue\n" );
}

/* ================== traitement recette =============== */

void set_crc_autor()
{
irb.txbuf[0] = PCRC; irb.txbuf[1] = 1;
if ( verbose )
   fprintf( logfile, "set_crc_autor\n" );
irb.txcnt = 2; irb.rxcnt = 8;
if ( dial( &irb ) )
   fprintf( logfile, "set_crc_autor : echec dialogue\n" );
}

void get_crc( status_full * pstat )
{
unsigned short slen;
irb.txbuf[0] = PCRC; irb.txbuf[1] = 0;
if   ( verbose )
     fprintf( logfile, "get_crc\n" );
irb.txcnt = 2; irb.rxcnt = 8;
if   ( dial( &irb ) )
     {
     fprintf( logfile, "get_crc : echec dialogue\n" );
     pstat->crc_stat = 0; return;
     }
pstat->crc_stat = (int)irb.rxbuf[1];
if   ( pstat->crc_stat & CRC_READY )
     {
     ((unsigned char *)&(pstat->crc))[0] = irb.rxbuf[2];
     ((unsigned char *)&(pstat->crc))[1] = irb.rxbuf[3];
     ((unsigned char *)&(pstat->crc))[2] = irb.rxbuf[4];
     ((unsigned char *)&(pstat->crc))[3] = irb.rxbuf[5];
     }
else pstat->crc = 0;
slen = *((unsigned short *)&(irb.rxbuf[6]));
pstat->packlen = (unsigned int)slen;
}			

void upload( int dest_adr, unsigned char * src_buf, int size )
{
int qtxbytes;
/*
irb.txbuf[0] = PSTEP; irb.txbuf[1] = 0;
irb.txcnt = 2; irb.rxcnt = 2;
if   ( dial( &irb ) )
     { fprintf( logfile, "upload : echec dialogue\n"); return; }
else fprintf( logfile, "step 0 Ok\n");
*/
irb.txbuf[0] = PMANU; irb.txbuf[1] = PAUSE | MANU | ROOT;
irb.txcnt = 2; irb.rxcnt = 1;
if   ( dial( &irb ) )
     { fprintf( logfile, "upload : echec dialogue\n"); return; }
else fprintf( logfile, "mode ROOT Ok\n");
while ( size > 0 )
   {
   if ( size > MAXUPLOAD ) qtxbytes = MAXUPLOAD;
   else			   qtxbytes = size;
   irb.txbuf[0] = SYSDBG; irb.txbuf[1] = 'o';
   irb.txbuf[2] = (unsigned char)  dest_adr;
   irb.txbuf[3] = (unsigned char)( dest_adr >> 8 );
   memcpy( (void *)(irb.txbuf+4), (void *)src_buf, qtxbytes );
   irb.txcnt = qtxbytes + 4; irb.rxcnt = 1;
   if   ( dial( &irb ) )
        { fprintf( logfile, "upload : echec dialogue\n"); return;  }
   else fprintf( logfile, "uploaded %d bytes @ %04X\n", qtxbytes, dest_adr );
   dest_adr += qtxbytes;
   src_buf += qtxbytes;
   size -= qtxbytes;
   }
irb.txbuf[0] = PMANU; irb.txbuf[1] = PAUSE;
irb.txcnt = 2; irb.rxcnt = 1;
if   ( dial( &irb ) )
     { fprintf( logfile, "upload : echec dialogue\n"); return; }
else fprintf( logfile, "mode ROOT off\n");
}

/* ================== automate securite =============== */

void secu_set_param( int index, int val16 )
{
irb.txbuf[0] = SET_PARA;
irb.txcnt = 4; irb.rxcnt = 1;
irb.txbuf[1] = index;
irb.txbuf[2] = (unsigned char)  val16;
irb.txbuf[3] = (unsigned char)( val16 >> 8 );
if ( verbose )
   fprintf( logfile, "secu_set_param %d %d\n", index, val16 );
if ( dialm( &irb ) )
   fprintf( logfile, "secu_set_param : echec dialogue\n" );
}

void secu_get_status( int * sstatus4 )
{
irb.txbuf[0] = GET_SSTA;
irb.txcnt = 1; irb.rxcnt = 8;
if   ( dialm( &irb ) )
     {
     fprintf( logfile, "secu_get_status : echec dialogue\n" );
     sstatus4[0] = -1;
     }
else {
     sstatus4[0] = irb.rxbuf[1];
     sstatus4[1] = *( (unsigned short int *)(irb.rxbuf+2) );
     sstatus4[2] = *( (unsigned short int *)(irb.rxbuf+4) );
     sstatus4[3] = *( (unsigned short int *)(irb.rxbuf+6) );
     }
}
