/* Frevo 7 : driver pour regulateur OMRON E5AN
   utilise les services du driver RS485 tmr0_rs485.c
 */


// convertir 1 digit hexa en binaire
unsigned char a1h2i( unsigned char c );

// convertir 3 digits hexa en binaire
unsigned short a3h2i( unsigned char * hex );

// convertir binaire en 1 digit hexa
unsigned char i2a1h( unsigned char v );

// convertir binaire en 3 digits hexa
void i2a3h( unsigned char * hex, unsigned short v );

// interroger un regulateur OMRON (demander consigne SV)
void askSV( unsigned char adr );

// interroger un regulateur OMRON (demander mesure PV)
void askPV( unsigned char adr );

/* les fonctions suivantes ciommuniquent directement avec les
   podgets de tem[]
 */

// recuperer reponse du regulateur OMRON (consigne SV)
// askSV() --> tempo env 100ms --> getSV()
void getSV( unsigned char adr );

// recuperer reponse du regulateur OMRON (mesure PV)
void getPV( unsigned char adr );

// envoyer valeur SV au regulateur OMRON
// sa reponse sera ignoree (mais la valeur relue au prochain tour)
void setSV( unsigned char adr );

// ecart minimal justifiant une mise a jour du regu OMRON
// (economiseur) en 16emes de degre
#define MINDIFT 160
