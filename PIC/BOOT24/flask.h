
//	lecture flash 1 row de 192 bytes (128 PC units) : 
void flashr( unsigned long adr, unsigned char * buf );

//	effacement flash 1 page de 1536 bytes (1024 PC units) soit 8 rows
void ferase( unsigned long adr );

//	ecriture flash 1 row de 192 bytes (128 PC units) : 
void flashw( unsigned long adr, unsigned char * buf );

//	saut sur adresse abs 16 bits
void fgorun( unsigned int adr );
