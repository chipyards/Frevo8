/* i2cmast_soft
	- I2C master 100% soft
 */

// fonctions de i2cmast_soft_drv.asm
void I2C_start(void);
void I2C_stop(void);
void I2C_stall(void);
void I2C_ack(void);
void I2C_nack(void);
unsigned char I2C_test_ack(void);
	
unsigned char I2C_rx_byte(void);

void I2C_tx_byte(unsigned char);	


// fonctions de i2cmast_soft.c

/* envoyer N bytes, MSbyte first, return 0 si OK */
unsigned char i2c_put( unsigned char adr, unsigned char * val, unsigned char N );

/* recevoir N bytes, return 0 si OK  */
unsigned char i2c_get( unsigned char adr, unsigned char * val, unsigned char N );
