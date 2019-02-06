/* listeur-chooser de recettes */

typedef struct
{
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   lmain;
GtkWidget *   wlis;	// fenetre scrollable
GtkListStore *  tmod;	// modele pour la liste
GtkWidget *     tlis;	// liste des recettes
GtkWidget *   hbut;
GtkWidget *     braf;
GtkWidget *     bcwd;
GtkWidget *     bcha;
GtkWidget *     bedi;
GtkWidget *     bedx;
GtkWidget *     bqui;

four *ptube;
int selected;
} catastru;

// afficher le catalogue dans fenetre modale
// element preselectionne est celui qui a stat == 4
// retourne index nouvel element selectionne si
// chargement demande, ou -1 si rien a faire
// indexes pointent sur ptube->reclist[]
int catalog_recettes( four *ptube );
