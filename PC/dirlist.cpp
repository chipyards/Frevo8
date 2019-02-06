/* dirlist.cpp : fonctions pour lecture directory, en 2 styles
   - VC6/Win32     (MSDOS_STYLE)
   - Unix/Solaris
   adapte en C++ a partir du code C utilise dans VV11
   lui-meme derive de celui utilise dans VV16
	- malloc elimine
	- mais on a garde tous les traitements string en style C
	- BUG FIX 2008-01 : FindClose pour eviter fuite memoire avec Win2k (vv11c)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include <sys\stat.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <vector>
using namespace std;
#include "dirlist.h"

// #define DEBOG


/* systeme de tri rapide ============================================= */

/* la fonction pivoter traite les elements de Tricat de i a j inclus.
 * le premier element dit pivot P est separe, les autres sont tries en
 * deux classes, superieurs et inferieurs au pivot.
 * Les inferieurs sont places de i a l-1, le pivot a l,
 * les superieurs de l+1 a j.
 * la fonction rend l.
 */
 
int dirdata::pivoter( int i, int j )
{
int P, Ptemp;
int k, l;
P = Tricat[i];

for ( k = i+1; ( (k < j) && ( comparer(Tricat[k], P ) <= 0 ) ); k++ )
    ;
for ( l = j;                ( comparer(Tricat[l], P ) >  0 );   l-- )
    ;

/* k et l delimitent l'intervalle qui reste a trier
 * on reduit cet intervalle par croissance de k et decroissance de l
 * si k >= l, il ne reste rien a faire
 */
while ( k < l )
    {
    Ptemp = Tricat[k];
    Tricat[k] = Tricat[l];
    Tricat[l] = Ptemp;
    for ( k++ ; ( comparer(Tricat[k], P ) <= 0 ); k++ )
        ;
    for ( l-- ; ( comparer(Tricat[l], P ) >  0 ); l-- )
        ;
    }
Ptemp = Tricat[i];
Tricat[i]  = Tricat[l];
Tricat[l]  = Ptemp;
return( l );   
}

/* la fonction tri_rapide traite les elements de T de i a j inclus.
 * elle les trie sur place dans l'ordre croissant.
 */

void dirdata::tri_rapide( int i, int j )
{
int l;
    
if  ( j > i )
    {
    l = pivoter( i, j );
    tri_rapide( i,   l-1 );
    tri_rapide( l+1,   j );
    }
}

int dirdata::comparer ( int P, int Q )
{
if (  ( dd[P].type == 'F' ) && ( dd[Q].type == 'D' ) )
   return(1);
if (  ( dd[P].type == 'D' ) && ( dd[Q].type == 'F' ) )
   return(-1);
return( strcmp( dd[P].name, dd[Q].name )  ); 
}

/* fonction assurant le tri d'un directory 
   alloue le tableau d'indirection da->tricat
 */

void dirdata::tri()
{
unsigned int i;

if ( Tricat.size() != dd.size() )
   {
   Tricat.resize( dd.size() );
   #ifdef DEBOG
      printf("Tricat resized to %d\n", dd.size() );
   #endif
   } 
for ( i = 0; i < dd.size(); i++ )
    Tricat[i] = i;

tri_rapide( 0, dd.size()-1 );      
}

/* systeme de lecture directory (non recursif a ce niveau ) =========
   Les entrees . et .. sont filtrees.
   les noms de directory n'ont pas de SLASH a la fin
 */

#ifdef WIN32
/* cette fonction lit les infos d'un dir 
   Elle est basee sur les fonctions FindFirstFile etc derivees
   de MSDOS (Win32 API). 
   elle laisse tout bien ferme 
 */
  
void dirdata::scan( const char *dirpath )
{
WIN32_FIND_DATA fdata;
HANDLE hand;
char lbuf[MAX_PATH];
int first;
direlem curelem;

sprintf( lbuf, "%s%c*.*", dirpath, SLASH );

#ifdef DEBOG
   printf("enter FindFirstFile w %s\n", lbuf );
#endif

/* boucle de lecture des noms */
dd.clear(); first = 1; hand = INVALID_HANDLE_VALUE;

while (1)
     {
     if   ( first ) /* lecture du premier element hors boucle */
          {
          hand = FindFirstFile( lbuf, &fdata );
          if ( hand == INVALID_HANDLE_VALUE )
             { return; }
          first = 0;
          }
     else if ( FindNextFile( hand, &fdata ) == 0 ) 
             break;
     // renseigner nouvel element :
     strncpy( curelem.name, fdata.cFileName, NAMLEN-1 );
     curelem.name[NAMLEN-1] = 0;
     curelem.type =
        ( fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )?('D'):('F');
     // introduire nouvel element dans la base de donnees :
     if ( 
        ( strcmp( curelem.name, "."  ) != 0 ) &&
        ( strcmp( curelem.name, ".." ) != 0 )
        )
	{
	dd.push_back( curelem );
	#ifdef DEBOG
           printf("inserted %s (%c)\n", dd.back().name, dd.back().type );
	#endif
	}
     }	// while(1)
FindClose( hand );
}

#else
/* cette fonction lit les infos d'un dir 
   Style Unix/Solaris
   elle laisse tout bien ferme 
 */
  
void dirdata::scan( const char *dirpath )
{
DIR *ledir;
struct dirent * lentree;
struct stat statbuf;
char lbuf[NAMLEN];
direlem curelem;

strcpy( lbuf, dirpath );
if ( lbuf[strlen(dirpath)-1] == SLASH )
   lbuf[strlen(dirpath)-1] = 0;         /* enlever slash a la fin */

dd.clear();

#ifdef DEBOG
printf("ready to open dir %s\n", lbuf );
#endif
ledir = opendir( lbuf );
if   ( ledir == NULL )
     return;
#ifdef DEBOG
printf("succeeded to open dir %s\n", lbuf );
#endif

/* boucle de lecture des noms */

while (  ( lentree = readdir( ledir ) )  !=  NULL  )
     {

     #ifdef DEBOG
     printf("%6d -- %s\n", dd.size(), lentree->d_name );
     #endif

     // renseigner nouvel element :
     strncpy( curelem.name, lentree->d_name, NAMLEN-1 );
     curelem.name[NAMLEN-1] = 0;

     // introduire nouvel element dans la base de donnees :
     if ( 
        ( strcmp( curelem.name, "."  ) != 0 ) &&
        ( strcmp( curelem.name, ".." ) != 0 )
        )
 	{
	dd.push_back( curelem );
	#ifdef DEBOG
           printf("inserted %s\n", dd.back().name );
	#endif
	}
     
     }
#ifdef DEBOG
printf("ready to close dir, seen %d element\n", dd.size() );
#endif
closedir( ledir ); free( lentree );

if ( dd.size() == 0 ) return;

/* boucle de lecture des types */     

unsigned int i;
for ( i = 0; i < dd.size(); i++ )
    {
    sprintf( lbuf, "%s%c%s", dirpath, SLASH, dd[i].name );
    if   ( stat( lbuf, &statbuf ) ) 
         dd[i].type = 'U';
    else switch( statbuf.st_mode & S_IFMT )
             {
             case  S_IFDIR : dd[i].type = 'D'; break;
             case  S_IFREG : dd[i].type = 'F'; break;
             default :       dd[i].type = 'U';
             }
    }
}
#endif


/* fonction rendant le directory courant */

char *curdir()
{
static char curdirbuf[NAMLEN];
#ifdef WIN32
GetCurrentDirectory( NAMLEN, curdirbuf );
#else
getcwd( curdirbuf, NAMLEN );
#endif
return( curdirbuf );
}

/*
void piqueconf()
{
printf("system's max file name length = %d\n",
        pathconf( ".", _PC_NAME_MAX )  );
printf("system's max file path length (.) = %d\n",
        pathconf( ".", _PC_PATH_MAX )  );
printf("system's max file path length (/) = %d\n",
        pathconf( "/", _PC_PATH_MAX )  );
printf("system's stat buffer = %d\n", sizeof( struct stat ) );
}
*/

