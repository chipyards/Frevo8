/* fonctions pour supervision 
   Dialogue I2C/UDP
   une couche au dessus de dial() (diali.c)
 */

// masques LEDs de signalisation sur sorties vannes
#define RUN_SIG  1<<6
#define HOLD_SIG 1<<7
#define END_SIG  1<<8

/* ======== traitement specifique structure status_full ========== */

// status d'un podget analog
typedef struct {
int sv;
int pv;
} status_analog;

// contenu decomprime d'un irb obtenu en reponse a PFULL
// contient aussi crc courant et buffer circulaire pour plot
typedef struct {
int step;
int vannes;
int chrono;
int flags;
status_analog mfc[4];
int frequ;
status_analog temp[3];
unsigned int crc_stat;
unsigned int crc;
unsigned int packlen;
// buffer circulaire pour fichier plot - contient les valeurs d'une mn d'activite
unsigned char irbring[60][30];	// 60 irbs PFULL bruts de 30 bytes
unsigned int qirbring;		// nombre d'irbs deja ecrits			
} status_full;

#ifdef  __cplusplus
extern "C" {
#endif

// actions associees
void unpack_status( status_full * pstat, unsigned char * irbdat );
void get_status( status_full * pstat );
void get_crc( status_full * pstat );

/* ================== config ===================== */

void fpilot_set_verbose( int v );
void fpilot_set_log( FILE * f );
void init_pilot();
void end_pilot();

/* ============ traitement requetes simples =========== */

void set_manu( int flag );
void set_step( int step );
void set_van( int ivan );
void reset_van( int ivan );
void set_dac( int idac, int val );
void set_temp( int item, int val );
void set_chron( int t );

/* ================== traitement recette =============== */

void set_crc_autor();
void upload( int dest_adr, unsigned char * src_buf, int size );

/* ================== automate securite =============== */

void secu_set_param( int index, int val16 );
void secu_get_status( int * sstatus4 );

#ifdef  __cplusplus
}
#endif
