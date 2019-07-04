#ifdef  __cplusplus
extern "C" {
#endif

// cette fonction flashe le fichier .bin a l'adresse MINROMADR
int flash_file( const char * majname );

// cette fonction compare le fichier .bin a la flash en MINROMADR
int check_file( const char * majname );

// conversion "a blanc"
void test_hex_file( const char * majname );

// cette fonction demande au pic de verifier le CRC de l'appli a MINROMADR
int remote_crc( );

// cette fonction demande l'execution de l'appli a MINROMADR (si crc ok)
int remote_exec( );

// cette fonction demande le soft reset
int remote_reset( );

// lecture version, remplit chaine ASCII
int read_version( char * tbuf );

#ifdef  __cplusplus
}
#endif

