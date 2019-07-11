
#include "asix.h"

#define MEMR 0x14	// registre			
#define MDC  0x01	// clock
#define MDO  0x08	// data out
#define MDI  0x04	// data in
#define MDIR 0x02	// direction, 0 = out

#pragma code jln_lib3


void put_mii_bit( unsigned char b )
{
unsigned char tmp;
tmp = read_asix( MEMR );
if ( b ) tmp |=  MDO;
else     tmp &= ~MDO;
write_asix ( MEMR, tmp );	// data bit
tmp |=  MDC;
write_asix ( MEMR, tmp );	// clock hi
tmp &= ~MDC;
write_asix ( MEMR, tmp );	// clock lo
}

unsigned char get_mii_bit(void)
{
unsigned char tmp, val;
tmp = read_asix( MEMR );
tmp |=  MDC;
write_asix ( MEMR, tmp );	// clock hi
val = read_asix( MEMR );
tmp &= ~MDC;
write_asix ( MEMR, tmp );	// clock lo
return( val & MDI );
}


/* lecture d'un registre MII : transmission serie via registre MEMR */
unsigned int read_mii( unsigned char regad )
{
unsigned char i;
unsigned int  mask16, preamble, result16;
 
preamble = 0b0110100000000010;	// read preamble 0110, phy adr 10000,
				// register adr 5 bits, turnaround 10
regad <<= 2;
regad |= 0b10;	// turnaround
preamble |= regad;

i = read_asix( MEMR );
i &=  ~MDIR;
write_asix ( MEMR, i );	// MDIR lo <==> out

mask16 = 0x8000;
for ( i = 0; i < 15; ++i )	// 15 bits preamble (4 + 5 + 5 + 1)
    {
    if ( mask16 & preamble ) put_mii_bit( 1 );
    else                     put_mii_bit( 0 );
    mask16 >>= 1;	 
    }
 
i = read_asix( MEMR );
i |=  MDIR;
write_asix ( MEMR, i );	// MDIR hi <==> in

mask16 = 0x8000;
result16 = 0;
for ( i = 0; i < 16; ++i )	// 16 bits data
    {
    if ( get_mii_bit() )
       result16 |= mask16;
    mask16 >>= 1;	 
    }  
return result16;
}	
//*/ 


/* ecriture dans un registre MII : transmission serie via registre MEMR */
void write_mii( unsigned char regad, unsigned int mii_data )
{
unsigned char i;
unsigned int  mask16, preamble;

preamble = 0b0101100000000010;	// write preamble 0101, phy adr 10000,
				// register adr 5 bits, turnaround 10
regad <<= 2;
regad |= 0b10;	// turnaround
preamble |= regad;

i = read_asix( MEMR );
i &=  ~MDIR;
write_asix ( MEMR, i );	// MDIR lo <==> out

mask16 = 0x8000;
for ( i = 0; i < 16; ++i )	// 16 bits preamble (4 + 5 + 5 + 2)
    {
    if ( mask16 & preamble ) put_mii_bit( 1 );
    else                     put_mii_bit( 0 );
    mask16 >>= 1;	 
    }

mask16 = 0x8000;	// (1<<15);
for ( i = 0; i < 16; ++i )	// 16 bits data
    {
    if ( mask16 & mii_data ) put_mii_bit( 1 );
    else                     put_mii_bit( 0 );
    mask16 >>= 1;	 
    }
}
//*/

/* ecriture de 32 uns dans le port MII */
void write_mii_pre( void )
{
unsigned char i;

i = read_asix( MEMR );
i &=  ~MDIR;
write_asix ( MEMR, i );	// MDIR lo <==> out

for ( i = 0; i < 32; ++i )	// 32 bits preamble
    put_mii_bit( 1 );
}
//*/

/* inhibition de la vitesse 100 Mbits/s (pour economie d'energie)
 */
void mii_disable100( void )
{
unsigned int val;
write_mii_pre();
write_mii( 4, 0x0061 );
}
