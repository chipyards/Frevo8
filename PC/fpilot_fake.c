/* fonctions pour supervision
   Dialogue I2C/UDP
   une couche au dessus de dial() (diali.c)
   FAKE pour tester superviseur en l'absence de connexion automate
 */


#include <stdio.h>
#include <string.h>
#include "../ipilot.h"
#include "fpilot.h"


void gasp( char *fmt, ... );  /* fatal error handling */

static status_full s;	// ici l'automate virtuel

// fonction de dialu.c
void dialugue_set_IP( unsigned char * pIP )
{
}

/* ================== config ===================== */

static int verbose = 1;
static FILE * logfile;

void fpilot_set_verbose( int v )
{
verbose = v;
}

void fpilot_set_log( FILE * f )
{
logfile = f;
}

void init_pilot()
{
     s.step   = 0;
     s.vannes = 0;
     s.chrono = 0;
     s.flags = PAUSE;
     s.mfc[0].sv = 13104;
     s.mfc[1].sv = 0;
     s.mfc[2].sv = 0;
     s.mfc[3].sv = 0;
     s.mfc[0].pv = 13000;
     s.mfc[1].pv = 0;
     s.mfc[2].pv = 0;
     s.mfc[3].pv = 0;
     s.frequ = 3355;
     s.temp[0].pv = 0;
     s.temp[1].pv = 0;
     s.temp[2].pv = 0;
     s.temp[0].sv = 400;
     s.temp[1].sv = 400;
     s.temp[2].sv = 400;
}

void end_pilot()
{}

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
int i;
if ( s.qirbring >= 60 )
   s.qirbring = 0;
// execution recette farfelue
if ( ( s.flags == 0 ) && ( s.step != 0 ) )
   {
   s.chrono++;
   if   ( ( s.vannes & 0xFFFF ) == 0 )
        s.vannes = 1;
   else s.vannes <<= 1;
   }
// calcul PV : simule reponse premier ordre
for ( i = 0; i < 4; i++ )
    {
    s.mfc[i].pv += ( s.mfc[i].sv - s.mfc[i].pv ) / 10;
    }
for ( i = 0; i < 3; i++ )
    {
    s.temp[i].pv += ( s.temp[i].sv - s.temp[i].pv ) / 20;
    }
s.qirbring += 1;
*pstat = s;
}

/* ============ traitement requetes simples =========== */

void set_manu( int flag )
{
s.flags = flag;
if ( verbose )
   fprintf( logfile, "manual_call\n" );
}

void set_step( int step )
{
s.step = step; s.chrono = 0;
if ( step == 0 )
   s.vannes = 0;
if ( verbose )
   fprintf( logfile, "set_step_call\n" );
}

void set_van( int ivan )
{
s.vannes |= ( 1 << ivan );
if ( verbose )
   fprintf( logfile, "van_call %d set\n", ivan );
}

void reset_van( int ivan )
{
s.vannes &= (~( 1 << ivan ));
if ( verbose )
   fprintf( logfile, "van_call %d reset\n", ivan );
}

void set_dac( int idac, int val )
{
s.mfc[idac].sv = val;
if ( verbose )
   fprintf( logfile, "mfc_call %d -> %d\n", idac, val );
}

void set_temp( int item, int val )
{
s.temp[item].sv = val;
if ( verbose )
   fprintf( logfile, "temp_call %d -> %d\n", item, val );
}

void set_chron( int t )
{
s.chrono = t;
}

/* ================== traitement recette =============== */

void set_crc_autor()
{
if ( verbose )
   fprintf( logfile, "set_crc_autor\n" );
}

void get_crc( status_full * pstat )
{
if   ( verbose )
     fprintf( logfile, "get_crc\n" );
s.crc_stat = CRC_READY;
s.crc = 0;
s.packlen = 0;
*pstat = s;
}

void upload( int dest_adr, unsigned char * src_buf, int size )
{
}

/* ================== automate securite =============== */

static int secu_fake_step = 0;

void secu_set_param( int index, int val16 )
{
if	( index == 0 )
	secu_fake_step = val16;
if ( verbose )
   fprintf( logfile, "secu_set_param %d %d\n", index, val16 );
}

void secu_get_status( int * sstatus4 )
{
     sstatus4[0] = secu_fake_step;
     sstatus4[1] = 2222;
     sstatus4[2] = 11000;
     sstatus4[3] = 1;
}
