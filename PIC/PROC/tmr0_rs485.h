/* tmr0_rs485.h
 Traitement interruption timer low priority
 Traitement interruption RS485 low priority
 - utilise tmr0_rs485_int.asm
 */

// alloue dans tmr0_rs485_int.asm
extern unsigned char uabuf[];
extern unsigned char utcnt;
extern unsigned char uindex;
extern unsigned char urcnt;

/* initialise interrupt timer */
void tmr0init( void );

void uartinit( void );

// lancer transmission de N bytes (presents dans uabuf)
void uatxstart( unsigned char N );

/* finition et envoi d'une requete pour regu OMRON
   le message doit deja etre en uabuf+1, null terminated
   cette fonction ajoute :
	- STX
	- ETX
	- BCC
	- bit stop en position MSB
   adr = 0 a 9
 */
void send485( void );

/* recuperation d'une reponse de regu OMRON (si disponible)
   rend la longueur utile ou zero
   le message sera en uabuf+1, stop bits supprimes
 */
unsigned char rcv485( void );

