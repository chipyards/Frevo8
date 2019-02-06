/* Frevo 7.8
   Dialogue IPILOT commun pour I2C/USB I2C/UDP et UDP
   Controle de flux I2C par NACK
*/

#include "irb.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* dial_irb
	- fonction bloquante jusqu'a reussite dialogue ou
	  epuisement du nombre de tentatives
	- aucune erreur n'est consideree comme fatale a ce niveau...
	  mais dial_log peut le faire
 */
int dial( irblock * irb );
int diali( irblock * irb );
int dialm( irblock * irb );

unsigned char dialogue_get_I2Caddr();

void dialogue_set_tenta( int tenta );
int dialogue_get_tenta();

void dialogue_set_verbose( int verb );
int dialogue_get_verbose();

void dialogue_set_log( FILE * f );
FILE * dialogue_get_log();
void dialogue_log_flush();

void dialogue_set_acces( char lacces );
char dialogue_get_acces();
char * dialogue_get_acces_text();

char * ipiloterr( unsigned char v );	// selon ipilot.h

void dial_log( char dir, irblock *irb );
void reinterpret( irblock * irb );

#ifdef  __cplusplus
}
#endif
