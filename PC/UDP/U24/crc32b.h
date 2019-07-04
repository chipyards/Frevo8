// from ethernet & zlib
#define POLYNOM 0xEDB88320L

// *pcrc doit etre initialise a 0xffffffff
// et complemente en fin de calcul
void update_crc( unsigned int * pcrc, unsigned char byte );
