/* Frevo 7 : driver pour regulateur OMRON E5AN
   utilise les services du driver RS485 tmr0_rs485.c
 */

#include <string.h>
#include <stdlib.h>
#include <p18cxxx.h>
#include "tmr0_rs485.h"
#include "recipe.h"

#pragma code jln_lib3
#pragma romdata jln_txt3

// convertir 1 digit hexa en binaire
unsigned char a1h2i( unsigned char c )
{
if ( c > '9' )
   { c |= 0x20; return( c - 0x57 ); } // 'a' - 10 = 0x57
return( c - '0' );
}

// convertir 3 digits hexa en binaire
unsigned short a3h2i( unsigned char * hex )
{
unsigned short retval;
((unsigned char *)&retval)[0] = a1h2i( hex[0] );
retval <<= 4;
((unsigned char *)&retval)[0] |= a1h2i( hex[1] );
retval <<= 4;
((unsigned char *)&retval)[0] |= a1h2i( hex[2] );
return( retval );
}

// convertir binaire en 1 digit hexa
unsigned char i2a1h( unsigned char v )
{
return( (v<=9)?(v+'0'):(v+('A'-10)) );
}

// convertir binaire en 3 digits hexa
void i2a3h( unsigned char * hex, unsigned short v )
{
hex[2] = i2a1h( ((unsigned char *)&v)[0] & 0x0F );
v >>= 4;
hex[1] = i2a1h( ((unsigned char *)&v)[0] & 0x0F );
v >>= 4;
hex[0] = i2a1h( ((unsigned char *)&v)[0] & 0x0F );
}

// interroger un regulateur OMRON (demander consigne SV)
rom char askSVtxt[] = "010000101C10003000001";
void askSV( unsigned char adr )
{
memcpypgm2ram( (void *)uabuf+1, (rom void*)askSVtxt, 22 );
uabuf[2] = adr + '0';
send485();
}

// interroger un regulateur OMRON (demander mesure PV)
rom char askPVtxt[] = "010000101C00000000001";
void askPV( unsigned char adr )
{
memcpypgm2ram( (void *)uabuf+1, (rom void*)askPVtxt, 22 );
uabuf[2] = adr + '0';
send485();
}

// recuperer reponse du regulateur OMRON (consigne SV)
// askSV() --> tempo env 100ms --> getSV()
void getSV( unsigned char adr )
{
if ( rcv485() != 22 )
   { tem[adr].RV = 0; return; }
tem[adr].RV = a3h2i( uabuf+20 ) << 4;	// pos 19 + 1 (STX)
}

// recuperer reponse du regulateur OMRON (mesure PV)
void getPV( unsigned char adr )
{
if ( rcv485() != 22 )
   { tem[adr].PV = 0; return; }
tem[adr].PV = a3h2i( uabuf+20 ) << 4;	// 12 bits left just.
}

// envoyer valeur SV au regulateur OMRON
// sa reponse sera ignoree (mais la valeur relue au prochain tour)
rom char setSVtxt[] = "010000102C1000300000100000xxx";
void setSV( unsigned char adr )
{
memcpypgm2ram( (void *)uabuf+1, (rom void*)setSVtxt, 30 );
uabuf[2] = adr + '0';
i2a3h( uabuf+27, ( tem[adr].SV >> 4 ) );	// pos 26 + 1 (STX)
send485();
}
