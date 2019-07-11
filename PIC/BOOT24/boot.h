// bootloader derive de Blossac mais non-ping-pong :


// CRC from ethernet & zlib
#define POLYNOM 0xEDB88320L

// fonction inspiree du livre de Bentham
// *pcrc doit etre initialise a 0xffffffff
// et complemente en fin de calcul
void update_crc( unsigned long * pcrc, unsigned char byte );

// verif du crc de la mise a jour en ROM
// la taille entiere de la mise a jour est supposee multiple de QROWPCUNITS
// retour 0 si Ok
int boot_verif( unsigned long romadr, unsigned int * majentry, unsigned long * plenPC );
