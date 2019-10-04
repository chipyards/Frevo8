/* definition de classes pour le compilateur de recette et l'initialisation du four */

#include <string>
#include <map>
#include <vector>

using namespace std;

/* l'etat d'un podget (mfc, tem ou fre) dans un certain step, ce qui correspond
   au podget dans le soft de l'automate */
class epod {
public :
		// ipar
int SV;		//  0
int SVmi;	//  1
int SVma;	//  2
int flags;	//  3
int stogo;	//  3
void init();	// pas de constructeur, utiliser init()
};
/* symboles pour les champs des flags des epod ATTENTION la coherence avec recipe.h */
#define MICEN	0x80	// mini check enable
#define MACEN	0x40	// max check enable
#define RAMPEN	0x20	// ramp enable
// decalage pour tolerance par defaut sur checks min/max ATTENTION coherence avec recipe.h
#define TOLSHIFT 3	// 12.5 %

#define QMFC 4
#define QTEM 3
#define QPRE 3	// nombre de presets de temperature
#define QVAN 16

/* un step dans le recette
   N.B le numero de step est son indice dans le tableau
 */
class etape {
public :
int existe;	// indique si existe dans la recette
		// ipar :
int deldg;	//  0	delai de grace (sec)
int duree;	//  1	duree du step en sec. (-1 si duree absente)
int vannes;	//  2	position des 16 vannes
int stogo;	//  3	saut inconditionnel (defaut = indice+1)
epod mfc[QMFC];	// etat des MFC
epod tem[QTEM];	// etat des regus de temperature
epod fre;	// frequencemetre
string titre;
int secstat;	// status a envoyer a l'automate secu (-1 si absent)
void init();	// pas de constructeur, utiliser init()
};

/* etats de la recette selon stat :
	-1	pas de recette
	-2	erreur, voir errmess
	0	xml ok
	1	pack ok
		- si crc automate ok passer en 3 ou 4 selon autor.
		- si recette automate vide passer en 2
		- sinon demander confirmation pour charger
	2	expedition a l'automate faite, attente lecture premier crc
	3	retour crc vu correct, autorisation lancee
	4	autorisation confirmee --> run possible

 */

/* la recette, resumee ou entiere */

#define QPACK (3*(1+255*(3+8*4)))	// plus gros pack possible
class four;		// decrite plus loin

// base class : un resume-bilan de la recette
class recipe_summary {
public :
string filename;	// nom de fichier court
string titre;
int stat;	// etat de la recette et de ses verifications
string errmess;	// message d'erreur eventuel
unsigned int qstep;	// nombre de steps
unsigned int packlen;	// taille pack en bytes
unsigned int crc;
};

// la recette complete
class recipe : public recipe_summary {
public :
four * ptube;	// four destinataire ou proprietaire de la recette
int errlin;	// numero de ligne pour message d'erreur
etape step[256];
unsigned char pack[QPACK];
string dump;
string logname;		// nom de fichier court du log en cours
// class variables
static DTD_recette dtd;		// DTD pour verification du fichier xml
// constuctors
recipe( four * montube ) : ptube(montube) { init(); };
// methods
void init();
// version short filename, cherche dans le repertoire de recettes du tube
void load_xml();
// version fullpath, il faut quand meme renseigner le membre filename
void load_xml( string fullpath );
void check();
void make_pack();
void dump_pack();
void make_xml();
private :
recipe() {};	// constructeur volontairement rendu inutilisable
void errtxt( const char * txt );	// fomattage de message d'erreur
};


/* les podgets de la machine (essentiellement leur caracteristiques statiques,
   plus le flow qui sert a la visu graphique des gaz)
   l'etat reel des podgets est pris dans la struct glo->status decrite dans fpilot.h
 */

// flux de gaz virtuel utilise pour la visu
class flow {
public :
char gaz;
char debit;
flow() : gaz(0), debit(0) {};
};


// base class
class podget {
public :
string name;
int x; 		int y;		// origine pour placer textes sur GUI
int pixx;	int pixy;	// origine pour placer graphique  = centre hot spot
int x1;		int y1;		// coordonnees hot spot
int x2;		int y2;
void * pang;			// pointeur sur un PangoLayout

podget() : name("--"), x(0), y(0), pixx(0), pixy(0), pang(NULL) {};
virtual ~podget() {};
void setbase( xelem * elem );	// lire quelques attributs dans le xml
virtual char tipo( void ) { return ' '; };	// (portable RTTI)
void basedump( void ) {
  printf("%c %10s txt[%04d:%03d] pix[%04d:%03d] ",
	tipo(), name.c_str(), x, y, pixx, pixy );
  }
virtual void dump( void ) {};
};

// vanne
class vodget : public podget {
public :
vector <string> src0;	// sources de gaz quand vanne au repos
vector <string> src1;	// sources de gaz quand vanne activee
string pix;			// nom de base element graphique
int gazx;	int gazy;	// origine pour placer graphique gaz
flow sortie;
vodget() : gazx(0), gazy(0) {};
virtual ~vodget() {};
virtual char tipo( void ) { return 'V'; };	// (portable RTTI)
// methode pour debug
virtual void dump( void ) {
  basedump();
  printf("pix=%1s gaz[%04d:%03d] src0=", pix.c_str(), gazx, gazy );
  unsigned int i;
  for ( i = 0; i < src0.size(); i++ )
      printf("%s ", src0[i].c_str() );
  printf(" src1=");
  for ( i = 0; i < src1.size(); i++ )
      printf("%s ", src1[i].c_str() );
  printf("\n");
  }
};

// mfc
class modget : public podget {
public :
double fs;
string unit;
vector <string> src;		// sources de gaz
int gazx;	int gazy;	// origine pour placer graphique gaz
flow sortie;
modget() : fs(1.0), unit(""), gazx(0), gazy(0) {};
virtual ~modget() {};
// methodes de conversion
int txt2pcu( string s );
int uiu2pcu( double sv, char type );
double pcu2uiu( int pcuval, char type );
void pcu2stream( ostream & obuf, int pcuval, char type );
virtual char tipo( void ) { return 'M'; };	// (portable RTTI)
// methode pour debug
virtual void dump( void ) {
  basedump();
  printf("      gaz[%04d:%03d] src=", gazx, gazy );
  for ( unsigned int i = 0; i < src.size(); i++ )
      printf("%s ", src[i].c_str() );
  printf("\n");
  }
};
//V"        --" [0000:0000] src{   |   } pix=     [0000:0000] gaz[0000:0000]
//M" Reg. vide" [0420:0280] src{ Fn}           pix[0000:0000] gaz[0000:0000]

// regu temperature
class todget : public modget {
public :
// preset pour temperature en manuel
int preset[QPRE];
todget() {
  for ( int i = 0; i < QPRE; i++ )
      preset[i] = 0;
  }
virtual ~todget() {};
virtual char tipo( void ) { return 'T'; };	// (portable RTTI)
virtual void dump( void ) {
  basedump(); printf("\n");
  }
};

// entree V/F
class fodget : public modget {
public :
double offset;
fodget() : offset(0.0) {};
virtual ~fodget() {};
virtual char tipo( void ) { return 'V'; };	// (portable RTTI)
virtual void dump( void ) {
  basedump(); printf("\n");
  }
};

/* la machine */

// bits des flags d'epod
#define MICEN	0x80	// mini check enable
#define MACEN	0x40	// max check enable
#define RAMPEN	0x20	// ramp enable

class four {
public :
int    ifou;			// numero du four
unsigned char destIP[4];	// adresse internet
string nom;			// nom du tube
modget mfc[QMFC];		// MFCs
todget tem[QTEM];		// regulateurs de temperature
fodget fre;			// frequencemetre
vodget vanne[QVAN];		// et les vannes
vector <podget *> ppod;		// tableau de pointeurs sur les podgets reunis
map <string, int> mfc_num;	// les numeros des mfcs en fonction des noms (selon fours.xml)
map <string, int> tem_num;	// les numeros des tems en fonction des noms (predefinis : S,C,H)
map <string, int> van_num;	// les numeros des vannes en fonction des noms
map <string, int> epod_flags;	// flags d'epod en fonction de noms predefinis (min, max, etc)
recipe recette;			// la recette en cours dans l'automate
vector <recipe_summary> reclist;// liste de recettes lues
string xml_ver;			// version lue dans fichier fours.xml
string xml_dir;			// repertoire pour les recettes XML
string pix_dir;			// repertoire pour les fichiers image pour GUI
string plot_dir;		// repertoire pour les fichiers bin pour plot courbes
string text_editor;		// editeur de texte local
int comm_verbose;		// verbosite de la communication IPILOT
string comm_log;		// fichier log de la communication IPILOT
string auto_secu;		// parametres de l'automate-securite
//int magic_step;			// step special pour armement automate securite
// class variables
static DTD_four dtd;		// DTD pour verification du fichier de config xml
// methodes
four() : recette(this)		// la recette a besoin de savoir a quel four elle appartient
  { 				// alors on passe this au constructeur de la recette
  epod_flags[string("0")]   = 0;
  epod_flags[string("min")] = MICEN;
  epod_flags[string("max")] = MACEN;
  epod_flags[string("minmax")]   = MICEN | MACEN;
  epod_flags[string("montee")]   = RAMPEN | MACEN;
  epod_flags[string("descente")] = RAMPEN | MICEN;
  comm_verbose = 1;
  int i;				// initialisation de ppod[], une commodite pour
  for	( i = 0; i < QVAN; i++ )	// traiter les podgets collectivement
	ppod.push_back( &vanne[i] );

  for	( i = 0; i < QMFC; i++ )
	ppod.push_back( &mfc[i] );

  for	( i = 0; i < QTEM; i++ )
	ppod.push_back( &tem[i] );

  ppod.push_back( &fre );
  };

void dump( void ) {	// dump des parametres des podgets (lus du xml)
  unsigned int i;
  for ( i = 0; i < ppod.size(); i++ )
      ppod[i]->dump();
  }
void load_xml( const char * fourpath );	// initialisation par lecture fours.xml
void scan_rec();			// mise a jour reclist selon contenu xml_dir
void autosync( unsigned int crc );	// identifie la recette chargee dans l'automate
int whichspot( int x, int y );		// identifie le hot-spot ( rend index sur ppod[] )
int get_ipod( int i );		// rend l'indice d'un podget dans son tableau specifique
				// en fonction de son indice dans ppod[]
int get_ipod( podget * pp );	// rend l'indice d'un podget dans son tableau specifique
				// en fonction de son pointeur
};

// fonctions utilitaires non-membres --------------------------- //

// fonction de formatage du temps vers un stream
void stream_time( ostream & outstr, time_t * pt, char separator );

// interpretation ip de la forme 192.168.1.80
void txt2ip( const char *txt, unsigned char * IP );

// une fonction qui coupe la string 'strin' en morceaux selon le
// delimiteur 'deli', et range les morceaux non vides dans le vector 'splut'
// qui doit exister. rend le nombre de morceaux.
// ici deli est 1 char
int ssplit1( vector <string> * splut, string strin, char deli );
