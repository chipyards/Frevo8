/* interpretation des fichier "plot" (voir spec plotdat.txt)
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>

using namespace std;

#include "modpop2.h"
#include "../../ipilot.h"
#include "../fpilot.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"
#include "glostru.h"
#include "plot.h"

// devra disparaitre
#define KF (10000000/33554432.0)

// pre-scan du log, extraction recette et liste steps
void runlog::pre_scan( string fullpath )
{
FILE * plofil;
unsigned int ifou, sec;
steplog curstep;
time_t block_time, cur_time = 0;
status_full sf;
unsigned char plobuf[4+4+(60*30)];
char c;

// lire la recette en memoire
recette.filename = fullpath;	// pour info
recette.load_xml( fullpath );
if ( recette.stat )
   {
   if	( recette.stat == -2 )
	{ errmess = "recette : " + recette.errmess; return;  }
   else { errmess = "recette : erreur inconnue "; return;  }
   }
printf("lu recette <%s>\n", recette.titre.c_str() );
// faire le scan du log

printf("ouverture lecture %s\n", fullpath.c_str() );
plofil = fopen( fullpath.c_str(), "rb" );

if ( plofil == NULL )
   { errmess = "erreur ouverture fichier"; return;  }

// sauter la recette jusqu'au delimiteur 0x7F
do {
   c = getc( plofil );
   if ( c == EOF ) 
      { errmess = "fin de fichier prematuree"; return;  }
   if ( c == 0 ) 
      { errmess = "caractere incorrect dans zone recette"; return;  }
   } while ( c != 0x7F );

curstep.istep = -1;
// boucle des blocs de 60s
while	( fread( plobuf, 1, sizeof(plobuf), plofil ) )
	{						// afficher bloc de 60 s
	ifou = ((unsigned int *)plobuf)[0];
	if  ( ifou != (unsigned int)recette.ptube->ifou )
	    { errmess = "numero de four incorrect"; return;  }
	block_time = ((time_t *)(plobuf+4))[0] - 60;	// date debut = date fin - 60
	if  ( start_time == 0 )
	    { start_time = block_time; cur_time = start_time;  }
	if  ( cur_time != block_time )
	    {
	    int diff = (int)cur_time - (int)block_time;
	    if   ( ( diff >= -2 ) && ( diff <= 2 ) )
		 {
		 printf("warning : time skew %d\n", diff );
		 cur_time = block_time;
		 }
	    else {
		 struct tm *t;
		 t = localtime( &cur_time );
		 printf( "cur_time %02d:%02d:%02d, ", t->tm_hour, t->tm_min, t->tm_sec );
		 t = localtime( &block_time );
		 printf( "blk_time %02d:%02d:%02d\n", t->tm_hour, t->tm_min, t->tm_sec );
		 printf("stop : time skew : count %d should be %d\n", (int)cur_time, (int)block_time );
		 break;
		 }
	    }
	for ( sec = 0; sec < 60; sec++ )
	    {
	    unpack_status( &sf, plobuf + 8 + 30*sec );
	    if  ( curstep.istep != sf.step )
		{
		// terminer et sauver le step en cours
		if ( curstep.istep >= 0 )
		   {
		   curstep.duree = (unsigned int)( cur_time - curstep.start_time );
		   // printf("fin step %d, duree %d\n", curstep.istep, curstep.duree );
		   steps.push_back( curstep );
		   }
		// commencer nouveau step
		curstep.istep = sf.step;
		curstep.start_time = cur_time;
		}
	    cur_time++;
	    }
	}
// terminer et sauver le dernier step
if ( curstep.istep >= 0 )
   {
   curstep.duree = (unsigned int)( cur_time - curstep.start_time );
   // printf("fin step %d, duree %d\n", curstep.istep, curstep.duree );
   steps.push_back( curstep );
   }
end_time = cur_time;
fclose( plofil );
}

void runlog::dump_resume()
{
ostringstream odum; unsigned int i;
odum << recette.ptube->nom << endl;
// odum << "fichier " << recette.filename << endl;
odum << "Recette \"" << recette.titre << "\"" << endl;
odum << "debut log "; stream_time( odum, &start_time, '_' );
odum << ", fin log "; stream_time( odum, &end_time, '_' ); odum << endl;
for ( i = 0; i < steps.size(); i++ )
    {
    odum << "step " << setfill(' ') << setw(3) << steps[i].istep << " " << setw(5) << steps[i].duree << "s ";
    odum << " \"" << recette.step[steps[i].istep].titre << "\"";
    odum << endl;  
    }
resume = odum.str();
}

void plot_gui( glostru * glo, int detail )
{
GtkWidget *dialog;

dialog = gtk_file_chooser_dialog_new( "Ouvrir Plot",
		GTK_WINDOW(glo->wmain), GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL );
gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(dialog), glo->ptube->plot_dir.c_str() );

if ( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
   {
   char *plofilname = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER (dialog) );
   runlog log(glo->ptube);
   log.pre_scan( string( plofilname ) );
   if   ( log.errmess.size() > 0 )
	{
        modpop( "Erreur sur lecture log",
		( log.recette.filename + "\n" + log.errmess ).c_str(),
		GTK_WINDOW(glo->wmain)
	      );
	}
   else {
	log.dump_resume();
	// printf( log.resume.c_str() );
	if   ( detail )
             plot_text_view( glo, plofilname, log.resume.c_str() );
	else modpop( "Résumé du log",
		     ("<tt>--------------------------------------------------------------------------\n" +
		      log.resume + "</tt>"
		     ).c_str(),
		     GTK_WINDOW(glo->wmain)
		   );
	}
   g_free( plofilname );
   }
gtk_widget_destroy (dialog);
}

// on peut appeler cette fonction si aucun resume n'est disponible, passer une chaine vide
void plot_text_view( glostru * glo, const char *plofilname, const char * resume )
{
   char plotxtname[1024]; int namelen;
   FILE * plofil; FILE * plotxt;
   unsigned int ifou;
   struct tm *t; status_full sf;
   int sec, ivan, imfc, item, oldvan;
   unsigned char plobuf[4+4+(60*30)];
   char c;

   namelen = strlen(plofilname);
   if ( namelen > 1019 ) namelen = 1019;
   strncpy( plotxtname, plofilname, namelen );
   if ( namelen > 4 ) namelen -= 4;
   strcpy( plotxtname + namelen, ".txt" ); 

   printf("ouverture lecture %s\n", plofilname );
   plofil = fopen( plofilname, "rb" );

   printf("ouverture ecriture %s\n", plotxtname );
   plotxt = fopen( plotxtname, "wb" );

   if ( ( plofil != NULL ) && ( plotxt != NULL ) )
      {
      // copier le resume
      fputs( resume, plotxt );
      // identifier et traiter separement le cas du fichier sans ou avec recette
      if ( fread( plobuf, 4, 1, plofil ) )
	 {
	 if   ( plobuf[1] == 0 )		// cas sans recette (ancienne version)
	      {
	      fprintf( plotxt, "<!-- pas de copie de la recette dans ce fichier -->\n");
	      fseek( plofil, 0L, SEEK_SET );		// rembobiner au debut
	      }
	 else {
	      fseek( plofil, 0L, SEEK_SET );		// rembobiner au debut
	      do {			// lire la recette jusqu'au delimiteur 0x7F
		 c = getc( plofil );
		 if ( c == EOF )	// fin de fichier prematuree
		    {
		    fprintf( plotxt, "\nfin de fichier prematuree\n");
		    break;
		    }
		 if ( c == 0x7F )	// fin normale de la recette
		    {
		    fprintf( plotxt, "\n<!-- fin de la recette -->\n");
		    break;
		    }
		 putc( c, plotxt );
		 } while ( c != EOF );
	      }
	 }
      while ( fread( plobuf, 1, sizeof(plobuf), plofil ) )
	    {						// afficher bloc de 60 s
	    fprintf( plotxt, "step  chrono   press  ");
	    for ( imfc = QMFC-1; imfc >= 0 ; imfc-- )
	        fprintf( plotxt, "%-13s", glo->ptube->mfc[imfc].name.c_str() );
	    ifou = ((unsigned int *)plobuf)[0];
	    if  ( ifou != (unsigned int)glo->ptube->ifou )
		{
		fprintf( plotxt, "\nnumero de four incorrect %u, abandon de la conversion\n", ifou );
		break;
		}
	    t = localtime( (time_t *)(plobuf+4) );
	    fprintf( plotxt, "#%u %02d-%02d-%4d %02d:%02d:%02d", ifou,
	             t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec );
	    fprintf( plotxt, "\n");
	    oldvan = 0xFFFF;
	    for ( sec = 0; sec < 60; sec++ )
		{
		unpack_status( &sf, plobuf + 8 + 30*sec );
		fprintf( plotxt, "%3d", sf.step );
		fprintf( plotxt, ( sf.flags & PAUSE )?"P":" ");	
		fprintf( plotxt, ( sf.flags & MANU )?"M":" ");	
		fprintf( plotxt, ( sf.flags & ROOT )?"R":" ");
		fprintf( plotxt, "%4ds ", sf.chrono );
		fprintf( plotxt, "%7.1fHz ", KF * (double)sf.frequ );
		for ( imfc = QMFC-1; imfc >= 0 ; imfc-- )
	            fprintf( plotxt, "%5.3f->%5.3f ",
			     glo->ptube->mfc[imfc].pcu2uiu( sf.mfc[imfc].sv, 'v' ),
			     glo->ptube->mfc[imfc].pcu2uiu( sf.mfc[imfc].pv, 'V' )  );
		for ( item = QTEM-1; item >= 0 ; item-- )
	            fprintf( plotxt, "%6.1f->%6.1f ",
			     glo->ptube->tem[item].pcu2uiu( sf.temp[item].sv, 'd' ),
			     glo->ptube->tem[item].pcu2uiu( sf.temp[item].pv, 'd' )  );
		if  ( sf.vannes != oldvan )
		    {
		    fprintf( plotxt, "/");
		    for ( ivan = QVAN-1; ivan >= 0; ivan-- )
			{
			if  ( sf.vannes & ( 1 << ivan ) )
			    fprintf( plotxt, "%s/", glo->ptube->vanne[ivan].name.c_str() );
			}
		    oldvan = sf.vannes;
		    fprintf( plotxt, "/");
		    }
		fprintf( plotxt, "\n");
		}
	    }
      fprintf( plotxt, "fin des donnees\n");
      fclose( plofil ); fclose( plotxt );
      printf("_fini\n");

      string cmd;
      #ifdef WIN32
      cmd = "c:\\appli\\wscite\\SciTE.exe " + string(plotxtname);
      #else
      cmd = "kwrite " + string(plotxtname);
      #endif
      system( cmd.c_str() );
      }

}
