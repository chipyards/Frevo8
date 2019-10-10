/* visualisation recette en detail */

// type de ligne dans le treeview, interprete par step_data_call() et data_data_call()
typedef enum {
STEP, SPAR, INNR, VANN, MFC, TEM, AUX
} lintype;

// structure de gui pour un modget (mfc ou tem)
typedef struct
{
GtkWidget * xmod;	// frame
GtkWidget *   vmod;
GtkWidget *     hcon;	// consigne
GtkWidget *       bcon;
GtkWidget *       scon;
GtkWidget *     hfla;	// flags = checks et rampes
GtkWidget *       cfla;	// combo pour les flags
GtkWidget *       lmin;
GtkWidget *       smin;
GtkWidget *       lmax;
GtkWidget *       smax;
GtkWidget *       linc;
GtkWidget *       sinc;
GtkWidget *       ljmp;
GtkWidget *       sjmp; // step to go
GtkWidget *       ejmp;	// son intitule
} modgui;


typedef struct
{
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   lmain;
GtkWidget *   wlis;	// fenetre scrollable
GtkWidget *     tste;	// arbre des steps
GtkWidget *   hbut;
GtkWidget *     bedi;
GtkWidget *     brew;
GtkWidget *     bffw;
GtkWidget *     bret;
GtkWidget *     bsav;
GtkWidget *     bjmp;
GtkWidget *     bqui;

GtkWidget *   wstep;	// fenetre scrollable
GtkWidget *   nstep;	// notebook
GtkWidget *     vstep;	// GUI edition step
GtkWidget *       hstep;
GtkWidget *         lstep;	// numero (non editable)
GtkWidget *         estep;	// intitule
GtkWidget *       hdur;
GtkWidget *         bpau;	// pause initiale
GtkWidget *         smin;	// duree
GtkWidget *         ssec;
GtkWidget *         sddg;	// delai de grace
GtkWidget *         ssui;	// step suivant
GtkWidget *         esui;	// son intitule

GtkWidget *       xvan;		// vannes
GtkWidget *         tvan;
GtkWidget *           bvan[QVAN];
GtkWidget *     vmfc;	// GUI edition MFCs
modgui            ymfc[QMFC];		// MFCs
GtkWidget *     vtem;	// GUI edition MFCs
modgui            ytem[QTEM];		// temperatures
GtkWidget *     vaux;	// GUI edition MFCs
modgui            yaux;		// auxiliaires


GtkTreeStore * tmod;	// modele pour l'arbre
four * ptube;
recipe * prec;
char scale_type;	// lettres p, u, v (cf methodes pcu2uiu et uiu2pcu de process.cpp)

int selected;		// step selectionne pour jump, step en cours d'edition
int modcnt;		// compteur de modifications de la recette en cours d'edition
int flags;		// role de le fenetre de recette : JMP ou EDIT
} rviewstru;

// afficher la recette dans fenetre modale

// flags
#define JMP 1
#define EDIT 2

int view_recette( recipe * prec, char scale_type, int selstep, int flags );
