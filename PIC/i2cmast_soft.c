/* i2cmast_soft.c
	- I2C master 100% soft
	- utilise i2cmast_soft_drv.asm
 */

#include <p18cxxx.h>
#include "i2cmast_soft.h"

#pragma code jln_lib3

/* envoyer N bytes, MSbyte first, return 0 si OK */
unsigned char i2c_put( unsigned char adr, unsigned char * val, unsigned char N )
{
I2C_start();
I2C_tx_byte( adr << 1 );	// write control byte

if  ( I2C_test_ack() == 0 )	// test for ACK condition
    { I2C_stop(); return(255);  }

while( N )
   {
   I2C_tx_byte( *val );

   if  ( I2C_test_ack() == 0 )
       { I2C_stop(); return(N);  }
   ++val; --N;
   }   
I2C_stop(); return(0);
}

/* recevoir N bytes, return 0 si OK  */
unsigned char i2c_get( unsigned char adr, unsigned char * val, unsigned char N )
{
I2C_start();
/* adresse du device = 7 bits suivis de R/Wbar */
I2C_tx_byte( ( adr << 1 ) | 1 );

if  ( I2C_test_ack() == 0 )	// test for ACK condition
    { I2C_stop(); return(255);  }

while( N )
   {
   *val = I2C_rx_byte();
   if   ( N == 1 )
        {		// this was the last byte accepted
        I2C_nack(); I2C_stop();
        }
   else I2C_ack();
   ++val; --N;
   }
return (0);
}
