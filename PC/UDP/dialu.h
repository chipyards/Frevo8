/* Frevo 7.8
 dialogue UDP
 */

#include "../irb.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* ================= configuration ========================== */

void dialugue_set_IP( unsigned char * pIP );
unsigned char * dialugue_get_IP();
int dialugue_get_port();

void dialugue_set_tenta( int tenta );
int dialugue_get_tenta();

void dialugue_set_verbose( int verb );
int dialugue_get_verbose();

void dialugue_set_log( FILE * f );
FILE * dialugue_get_log();

// rudimentaire conversion numero IP ascii en tableau de bytes
void txt2ip( unsigned char * IP, char * text );

/* ================= error logging ========================== */

void time_log();

/* ================= traffic UDP ========================== */

void openUDP();
int dialu( irblock * irb );

// int dialogue_U( unsigned char *bibuf, int N );
#ifdef  __cplusplus
}
#endif
