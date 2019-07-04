#include "crc32b.h"

// fonction inspiree du livre de Bentham
// *pcrc doit etre initialise a 0xffffffff
// et complemente en fin de calcul
void update_crc( unsigned int * pcrc, unsigned char byte )
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
