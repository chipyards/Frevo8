/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2002 Mark Adler in zlib
 * simplificated by JLN
 */

#include "../ipilot.h"

static int crc_table_empty = 1;
static unsigned int crc_table[256];

/*
  Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:

  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  #define CRC_POLY 0xEDB88320

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.

  The table is simply the CRC of all possible eight bit values.
*/
static void make_crc_table()
{
unsigned int c, n, k;

for ( n = 0; n < 256; n++ )
    {
    c = n;
    for ( k = 0; k < 8; k++ )
        c = c & 1 ? CRC_POLY ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
    }
crc_table_empty = 0;
}


/* ========================================================================= */
unsigned int icrc32( unsigned char *buf, int len )
{
unsigned int crc = 0xffffffff;

if (crc_table_empty)
   make_crc_table();

do  {
    crc = crc_table[(crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
    } while (--len);

return crc ^ 0xffffffff;
}
