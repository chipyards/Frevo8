/* Frevo 7 terminal de maintenance pour four Frevo

   pseudo global storage ( allocated in main()  )
*/

#define QVAN 16
#define QMFC 4

// constantes pour espacement general des widgets
#define VSPACE 4
#define SPACE  3
#define HSPACE 5

// options d'affichage regroupees ici
typedef struct
{
int txt;	// autorise txt gui selon textgui.cpp
int scroll;	// autorise fenetre graphique avec scrollbars pour petit ecran
int oldjmp;	// autorise bouton jmp avec spinbox
int txt_temp;
int txt_aux;
int uniform_temp;
char scale_type;	// lettres p, u, v (cf methodes pcu2uiu et uiu2pcu de process.cpp)
int auto_secu;		// presence d'un automate de securite
} display_flags;


/*   JLN's GTK widget naming chart :
   w : window
   f : frame
   h : hbox
   v : vbox
   b : button
   l : label
   e : entry
   s : spin adj
   m : menu
   o : option
 */

typedef struct
{
GtkWidget * wmain;
GtkWidget * wscro;
GtkWidget * vmain;

// --------------------------- debut textgui.cpp
GtkWidget *   fvan;
GtkWidget *   vvaa;
GtkWidget *    hvan[2];
GtkWidget *     vvan[QVAN];
GtkAdjustment * avan[QVAN];
GtkWidget *     svan[QVAN];
GtkWidget *     evan[QVAN];
GtkWidget *     lvan[QVAN];

GtkWidget *   fmfc;
GtkWidget *   hmfc;
GtkWidget *     vmfc[QMFC];
GtkAdjustment * amfc[QMFC];
GtkWidget *     smfc[QMFC];
GtkWidget *     emsv[QMFC];
GtkWidget *     empv[QMFC];
GtkWidget *     lmfc[QMFC];

GtkWidget *   ftem;
GtkWidget *   htem;
GtkWidget *     vtem[3];
GtkAdjustment * atem[3];
GtkWidget *     stem[3];
GtkWidget *     etsv[3];
GtkWidget *     etpv[3];
GtkWidget *     ltem[3];
GtkWidget *     vtpr;
GtkWidget *       btpr[3];
GtkWidget *       buni;


GtkWidget *   faux;
GtkWidget *   haux;
GtkWidget *     vaux[QMFC];
GtkWidget *     eaux[QMFC];
GtkWidget *     laux[QMFC];

// boite aussi utilisee par artgui

GtkWidget *   vmisc;
GtkWidget *     hmisc;
GtkWidget *       vsca;
GtkWidget *         rsca[3];
GtkWidget *       bpl1;
GtkWidget *       bpl2;
GtkWidget *   fsec;
GtkWidget *     hsec;
GtkWidget *       esec;
GtkWidget *       bsec;

// --------------------------- fin textgui.cpp

// --------------------------- debut artgui.cpp
GtkWidget *   darea;
GdkPixbuf * backpix;
GdkPixbuf * destpix;
map <string, GdkPixbuf *> pix;
GtkWidget *   hbar;
GtkWidget *   vbar;
GtkWidget *   pop_fond;
GtkWidget *   pop_vanne;
GtkWidget *       lpvan;
GtkWidget *       lpva0;
GtkWidget *       lpva1;
GtkWidget *     mivou;
GtkWidget *     mivfe;
GtkWidget *   pop_temp;
GtkWidget *       lptem;
GtkWidget *       lpte0;
GtkWidget *       lpte1;
GtkWidget *       lpte2;
GtkWidget *     mite[3];
GtkWidget *     mitea;

int ispot;

// --------------------------- fin artgui.cpp

GtkWidget *   hste;
GtkWidget *     esta;
GtkWidget *     este;
GtkWidget *   hbut;
GtkWidget *     bman;
GtkWidget *     baut;
GtkWidget *     bpau;
GtkWidget *     bcon;
GtkWidget *     bsta;
GtkWidget *     bsto;
GtkWidget *     bsau;
GtkWidget *     bfas;
GtkWidget *     bqui;
GtkWidget *   frec;
GtkWidget *   hrec;
GtkWidget *     bcho;
GtkWidget *     ere1;
GtkWidget *     ere2;
GtkWidget *     brvi;
// GtkWidget *     brec;

int         idle_id;	// id pour la fonction idle du timeout

four *ptube;
display_flags show;

// inclure fpilot.h d'abord
status_full status;
int old_flags;

} glostru;

