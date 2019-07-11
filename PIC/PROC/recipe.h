/* Frevo 8.1 traitement recette */



/* structure des Process Objects qui vont recevoir les donnees
   extraites de la recette */

typedef struct {	// ipar
unsigned char id;	//   0	numero du step
unsigned char deldg;	//   0	delai de grace (sec)
unsigned int  duree;	//   1	duree du step en sec.
unsigned int  vannes;	//   2	position des 16 vannes
unsigned int  chrono;	// le chronometre (part de zero en croissant)
unsigned char flags;	// pause, manu, root
unsigned char stogo;	// saut a expiration de la duree
unsigned char new;	// hint indiquant que le step est nouveau
} STEP_PO;

#define QMFC 4

typedef struct {	// ipar
unsigned int SV;	//  0
unsigned int SVmi;	//  1
unsigned int SVma;	//  2
unsigned char flags;	//  3
unsigned char stogo;	// step to go : destination en cas de rupture de sequence
unsigned int PV;
} MFC_PO;

#define QTEM 3

typedef struct {	// ipar
unsigned int SV;	//  0  valeur de SV engendree par la recette
unsigned int SVmi;	//  1
unsigned int SVma;	//  2
unsigned char flags;	//  3
unsigned char stogo;	// step to go : destination en cas de rupture de sequence
unsigned int PV;
unsigned int RV;	// valeur de SV relue du regulateur Omron
} TEM_PO;

/* symboles pour les champs des flags des PO */
#define NEWSV 	3	// active l'emission SV vers DAC
#define MICEN	0x80	// mini check enable
#define MACEN	0x40	// max check enable
#define RAMPEN	0x20	// ramp enable
#define CHECKS	(MICEN|MACEN|RAMPEN)

/* vanne speciale qui sera mise a 1 au retour a l'etat repos
   (sur le port D) */
#define VANNE0	1

/* variables pour extraction de donnees de la recette comprimee "pack"
 */
typedef struct {
unsigned int adr;	// adresse courante dans pack
unsigned char ipod;	// indice podget en cours de traitement
unsigned char ipar;	// indice parametre du podget en cours de traitement
MFC_PO * ppod;
unsigned long crc;	// crc fini et complemente
unsigned long crc_buf;	// crc en cours de calcul (cf ipilot.h pour le polynome)
unsigned int  crc_adr;	// adresse byte crc en cours
unsigned char crc_bitp;	// position bit crc en cours
unsigned char crc_shft;	// byte crc en cours de decalage
unsigned char crc_lsb;	// xor lsbs crc en cours
unsigned char crc_stat;	// symboles pour les champs de crc_stat : cf ipilot.h cde PCRC 
} EXTRACTOR;

/* variables exportees de proc.c */
extern near STEP_PO etape;
extern near EXTRACTOR extrac;
extern MFC_PO mfc[];
extern TEM_PO tem[];
extern MFC_PO fre;

/* variable exportee de defpack.c */
extern rom unsigned char defpack[];

// macros pour extraire le header du pack
#define qsteps (*pack)
#define packlen (*((unsigned int *)&pack[1]))

// decalage engendrant la tolerance par defaut sur les checks min/max
#define TOLSHIFT 3	// 12.5 %

/* inititialisation des podgets pour le step 0
   (sauf DV temperatures qu'on laisse tels quel)
   cette fonction ferme les vannes d'urgence
 */
void step_zero( void );

/* initialisation temperatures au demarrage a froid */
void init_tem( void );

/* copie en RAM de la recette par defaut */
void load_def( void );

/* sauter dans un nouveau step s'il existe (sinon : step 0)
   attention :
   cette fonction initialise tout ce qu'il faut sauf etape.chrono
   elle n'interagit pas avec le hardware (sauf si dest=0)
 */
void sauter( unsigned char dest );

/* effectuer les checks et increments de rampe
   sur le podget designe par le pointeur extrac.ppod
   cette fonction appelle sauter() le cas echeant, et rend 0 dans ce cas
   sinon rend 1
 */ 
unsigned char check_ppod( void );

/* avancer d'un bit le calcul du crc de la recette en RAM
   appeler tant que extrac.crc_run != 0 */
void crc_cont( void );

/* initialiser le calcul de crc */
void crc_start( void );
