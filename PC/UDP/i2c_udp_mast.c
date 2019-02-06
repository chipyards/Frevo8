/* Frevo 7.8
 driver master i2c via UDP pour passerelle Frevo6
 - API I2C similaire a celui de Tango3 sur PIC18
 - avec en plus support checksum style frevo 1
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../ipilot.h"
#include "dialu.h"
#include "i2c_udp_mast.h"

static irblock irb;

/* ---------------- niveau paquet I2C -------------------- */

/* valeurs de retour de ces fonctions :
	0 = Ok
	1 = lock passerelle (seult. i2c_put_chk() et i2c_put_chk() )
	2 = erreur checksum I2C sur reponse ( seult. i2c_get_chk() )
	3 = I2C nack (relaye par firmware frevo3
	4 = erreur protocole frevo3 (retour UDP non conforme firmware frevo3)
	254 = echec socket (send)	cf dialu
	255 = timeout			cf dialu
  
   pour i2c_put_chk() et i2c_get_chk(), N est "payload size, checksum exclu"
*/

int i2c_put( unsigned char I2Caddr, unsigned char *val, int N )  /* emettre message */
{
// adresse I2C est ignoree, c'est la passerelle qui le gere
int retval, i;

irb.txbuf[0] = UW;
for ( i = 0; i < N; i++ )
    {
    irb.txbuf[i+1] = val[i];
    }
irb.txcnt = irb.rxcnt = N+1;
retval = dialu( &irb );
if ( retval != 0 )
   return(retval);

if   ( irb.rxbuf[0] == UA )
     return(0);
if   ( irb.rxbuf[0] == UL )
     return(1);
if   ( irb.rxbuf[0] == UN )
     return(3);
return(4);
}

int i2c_put_chk( unsigned char I2Caddr, unsigned char *val, int N )  /* emettre message */
{
unsigned char txsum;
int retval, i;

irb.txbuf[0] = UW; txsum = MYTSUM;

for ( i = 0; i < N; i++ )
    {
    irb.txbuf[i+1] = val[i]; txsum += val[i];
    }
irb.txbuf[N+1] = ~txsum + 1;
irb.txcnt = irb.rxcnt = N+2;
retval = dialu( &irb );
if ( retval != 0 )
   return(retval);

if   ( irb.rxbuf[0] == UA )
     return(0);
if   ( irb.rxbuf[0] == UL )
     return(1);
if   ( irb.rxbuf[0] == UN )
     return(3);
return(4);
}

int i2c_get( unsigned char I2Caddr, unsigned char *val, int N ) /* recevoir un bloc */
{
int retval, i;

irb.txbuf[0] = UR;
irb.txcnt = irb.rxcnt = N+1;
retval = dialu( &irb );
if ( retval != 0 )
   return(retval);

if   ( irb.rxbuf[0] == UA )
     {
     for ( i = 0; i < N; i++ )
         val[i] = irb.rxbuf[i+1];
     return(0);
     }
if   ( irb.rxbuf[0] == UN )
     return(3);
return(4);
}

int i2c_get_chk( unsigned char I2Caddr, unsigned char *val, int N ) /* recevoir un bloc */
{                       
int retval, i;
unsigned char rxsum;

irb.txbuf[0] = UR;
irb.txcnt = irb.rxcnt = N+2;
retval = dialu( &irb );
if ( retval != 0 )
   return(retval);

if   ( irb.rxbuf[0] == UA )
     {
     rxsum = 0;
     for ( i = 0; i <= N; i++ )
         { val[i] = irb.rxbuf[i+1]; rxsum += val[i];  }
     if   ( rxsum == MYRSUM )
          return(0);
     else return(2);
     }
if   ( irb.rxbuf[0] == UN )
     return(3);
return(4);
}


void i2c_init()
{
openUDP();
}

void i2c_disable()
{
}
