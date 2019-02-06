/* interpretation des fichier "plot" (voir spec plotdat.txt)
 */

class steplog {
public:
int istep;		// numero de step dans la recette
time_t start_time;	// date absolue du debut du step
unsigned int duree;	// duree dans le log en secondes
};

class runlog {
public:
recipe recette;
string errmess;
string resume;	// resultat de dump_resume() 
vector <steplog> steps;
time_t start_time;	// date absolue du debut du log
time_t end_time;	// date absolue de la fin du log
// constructeur (la recette a ABSOLUMENT besoin de savoir a quel four elle appartient)
runlog( four * montube  ) : recette(montube), start_time(0) {};
// methodes
void pre_scan( string fullpath );
void dump_resume();
private :
runlog() : recette( (four *)NULL ) {};	// constructeur volontairement rendu inutilisable
};


void plot_gui( glostru * glo, int detail );

void plot_text_view( glostru * glo, const char *plofilname, const char * resume );
