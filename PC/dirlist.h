#define NAMLEN 1024

#ifdef WIN32
#define SLASH '\\'
#else
#define SLASH '/'
#endif

// une entree de directory
class direlem {
public :
char name[NAMLEN];	// nom court
char type;		// type parmi D, F, U
};

/* notes sur la methode scan :
	Les entrees . et .. sont filtrees.
	elle laisse tout bien ferme
	en cas d'echec de lecture rend simplement dd.size() = 0;
	en cas d'echec de stat, rend dd[i].type = "U"
 */

// un directory complet
class dirdata {
public :        
vector <direlem> dd;	// les entrees
vector <int> Tricat;	// table d'indirection apres tri */
void scan( const char *dirpath );
void tri();
dirdata() {};
dirdata( const char *dirpath ) { scan( dirpath ); tri(); };
private :
// fonctions pour le tri
int pivoter( int i, int j );
void tri_rapide( int i, int j );
int comparer ( int P, int Q );
};


// fonction rendant le directory courant
char *curdir();
