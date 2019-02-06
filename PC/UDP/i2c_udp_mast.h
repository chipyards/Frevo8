void i2c_init();
void i2c_disable();

/* emettre message */
int i2c_put_chk( unsigned char I2Caddr, unsigned char *val, int N );

/* recevoir un bloc */
int i2c_get_chk( unsigned char I2Caddr, unsigned char *val, int N );

/* les memes sans gestion checksum (pour debug only) */
int i2c_put( unsigned char I2Caddr, unsigned char *val, int N );
int i2c_get( unsigned char I2Caddr, unsigned char *val, int N );

