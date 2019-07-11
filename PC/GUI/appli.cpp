/* appli Frevo 8
   terminal I2C / UDP pour debug automate Frevo 7.9 a 8
   valeurs analog unifiees sur 16 bits unsigned left-justif.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <locale.h>

#include "modpop2.h"
#include "../../ipilot.h"
#include "../fpilot.h"
#include "../../version.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"
#include "../dirlist.h"
#include "glostru.h"
#include "catalog.h"
#include "rview.h"
#include "../UDP/dialu.h"
#include "textgui.h"
#include "artgui.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>

/** ============================ CONFIG ======================= */

// static storage all in one pour le moment dans process.cpp
extern four tube;

// pour exception gasp() de modpop2.c
// TRES BIZARRE ceci cree bien le storage de la variable et l'exporte pour
// un programme C (a savoir modpop2.c) !! marche ok avec :
//	- vc6 (une var. ordinaire n'est pas vue par modpop2.obj)
//
extern "C" { GtkWindow * global_main_window; };

/** ============================ fichier PLOT ====================== */

/*
- a chaque seconde, la reponse de l'automate a la commande PFULL (30 bytes)
  est stockee a l'etat brut dans un buffer circulaire (ou 30 zeros si echec dialogue)
  (fonction get_status() de fpilot.c)
- toutes les 60s, le fichier plot a condition qu'il existe est ouvert
  pour y ecrire 1808 bytes (106 kbyte/h, 2.5 Mbyte/j, 907 Mbyte/an) :
	- le numero du four (4 bytes !)
	- le temps unix (4 bytes) a la cloture du bloc
	- les 60 paquets de 30 bytes
  puis le fichier est referme.
  c'est la presence d'un string logname dans la recette courante qui valide cette action
*/


// creation du fichier plot
void plot_new( glostru * glo )
{
time_t it;
time( &it );

// creation du nom de fichier
ostringstream osfnam;
osfnam << "run_"; stream_time( osfnam, &it, '_' ); osfnam << ".bin";
glo->ptube->recette.logname = osfnam.str();
// ecriture du header
string plotfnam = glo->ptube->plot_dir + char(SLASH) + glo->ptube->recette.logname;
ofstream fiplo( plotfnam.c_str() );
if ( !fiplo )
   {
   glo->ptube->recette.logname = "";
   return;
   }
// nom de la recette
string recfnam = glo->ptube->xml_dir + char(SLASH) + glo->ptube->recette.filename;
fiplo << "<!-- recette " << recfnam <<  " -->\n";
printf("plot new : %s, recette %s\n", glo->ptube->recette.logname.c_str(), glo->ptube->recette.filename.c_str() );
// date derniere modif
struct stat statbuf;
if ( stat( recfnam.c_str(), &statbuf ) == 0 )
   {
   fiplo << "<!-- modifiee le ";
   stream_time( fiplo, &statbuf.st_mtime, '/' );
   fiplo << " -->\n";
   }
// texte source de la recette
ifstream firec( recfnam.c_str() );
char lbuf[132];
if ( firec )
   {
   while ( firec.getline( lbuf, 132 ) )
     {
     fiplo << lbuf << endl;
     }
   }
fiplo.put( (char)0x7F );	// delimiteur de fin de recette
fiplo.close();
glo->status.qirbring = 0;	// initialiser le compteur de minutes
}

// arret log
void plot_stop( glostru * glo )
{
glo->ptube->recette.logname = "";
}

// mise a jour fichier plot a chaque 60 secondes
void plot_status( glostru * glo )
{
if ( glo->status.qirbring != 60 )		// une fois par minute
   return;
if ( glo->ptube->recette.stat != 4 )		// requerir recette correctement verifiee
   return;
if ( glo->ptube->recette.logname.size() < 1 )	// requerir logname non vide
   return;

string plotfnam = glo->ptube->plot_dir + char(SLASH) + glo->ptube->recette.logname;
FILE * fiplo = fopen( plotfnam.c_str(), "ab" );
if ( fiplo != NULL )
   {
   int wcnt = 0; time_t t;
   wcnt += fwrite( &glo->ptube->ifou, sizeof(int), 1, fiplo );
   time( &t );
   wcnt += fwrite( &t, sizeof( time_t ), 1, fiplo );
   wcnt += fwrite( glo->status.irbring, 1, sizeof( glo->status.irbring ), fiplo );
   fclose( fiplo );
   }
}

/** ============================ AFFICHAGE ======================= */

// mise a jour des visibilites des widgets en manuel ou auto
void show_manu( int flag, glostru * glo  )
{
if   ( flag )
     {
     gtk_widget_hide( glo->bman );
     gtk_widget_show( glo->baut );
     }
else {
     gtk_widget_show( glo->bman );
     gtk_widget_hide( glo->baut );
     }
gtk_widget_set_sensitive ( glo->bsta, !flag );
gtk_widget_set_sensitive ( glo->bsto, !flag );
gtk_widget_set_sensitive ( glo->bpau, !flag );
gtk_widget_set_sensitive ( glo->bcon, !flag );

if   ( glo->show.txt )
     show_txt_manu( flag, glo );
else show_art_manu( flag, glo );
}

void show_pause( int flag, glostru * glo )
{
if   ( flag )
     {
     gtk_widget_hide( glo->bpau );
     gtk_widget_show( glo->bcon );
     }
else {
     gtk_widget_show( glo->bpau );
     gtk_widget_hide( glo->bcon );
     }
}

void show_run( int flag, glostru * glo )
{
if   ( flag )
     {
     gtk_widget_hide( glo->bsta );
     gtk_widget_show( glo->bsto );
     }
else {
     gtk_widget_show( glo->bsta );
     gtk_widget_hide( glo->bsto );
     }
}


// mise a jour affichage a chaque seconde
void display_status( glostru * glo )
{
char tbuf[256]; GdkColor rose;
const char * step_op_text; int mn, sec, dmn, dsec;
static int old_step = 0;	// pour faire plot si au demarrage l'automate est sur un step != 0

// cas de la perte de communication
if ( glo->status.step < 0 )
   {
   rose.red = 0xFFFF; rose.green = 0x0000; rose.blue  = 0x0000;
   gtk_widget_modify_base( glo->esta, GTK_STATE_NORMAL, &rose );
   gtk_entry_set_text( GTK_ENTRY(glo->esta), "connexion perdue" );
   return;
   }

// mise a jour txt gui
if   ( glo->show.txt )
     display_txt_status( glo );
else display_art_status( glo );

/* gerer changement manuel<-->auto
   ceci n'est pas fait par les callbacks des boutons auto et manu
   car le changement peut aussi venir aussi de l'automate (redemarrage terminal
   lorsque l'automate est deja en manuel)
*/
if   ( glo->status.flags != glo->old_flags )
     {
     if   ( glo->status.flags & MANU )		// mettre a jour spin boxes si on vient de passer en manuel
	  {
	  show_manu( 1, glo );
	  if ( glo->show.txt )
	     update_spinboxes( glo );
	  }
     else {
	  show_manu( 0, glo );
	  }
     if   ( glo->status.flags & PAUSE )		// mettre a jour boutons pause
	  { show_pause( 1, glo ); }
     else { show_pause( 0, glo ); }
     glo->old_flags = glo->status.flags;
     }

// actions a ne faire qu'au changement de step
if   ( glo->status.step != old_step )
     {
     if ( glo->status.step != 0 )
	{
	show_run( 1, glo );	// boutons start-stop
	if (
	   ( ( glo->status.step == 1 ) || ( old_step == 0 ) ) &&
	   ( glo->ptube->recette.stat == 4 )
	   )
	   plot_new( glo );	// nouveau fichier log
	if ( ( glo->ptube->magic_step > 0 ) && ( glo->status.step == glo->ptube->magic_step ) )
	   secu_set_param( 0, 1 );	// auto-armement automate secu
	}
     if ( glo->status.step == 0 )
	{
	show_run( 0, glo );	// boutons start-stop
	plot_stop( glo );	// zero fichier log
	}
     old_step = glo->status.step;
     }

sec = glo->status.chrono;
mn  = sec / 60;  sec = sec % 60;
dsec = glo->ptube->recette.step[glo->status.step].duree;
dmn = dsec / 60; dsec = dsec % 60;

if   ( glo->status.vannes & END_SIG )
     step_op_text = "FIN";
else if   ( glo->status.step )
	  {
          if   ( glo->status.flags & ( PAUSE | MANU ) )
               step_op_text = "Pause";
          else step_op_text = "Run";
	  }
     else step_op_text = "Repos";

if   ( ( dsec >= 0 ) && ( glo->status.step ) )
     sprintf( tbuf, "step %d - %d:%02d [%d:%02d] %s",
		     glo->status.step, mn, sec, dmn, dsec, step_op_text );
else sprintf( tbuf, "step %d - %d:%02d [-:-] %s",
		     glo->status.step, mn, sec, step_op_text );

gtk_entry_set_text( GTK_ENTRY(glo->esta), tbuf );

if   ( glo->status.step > 0 )
     {
     if   ( glo->status.flags & MANU  )
          {
	  rose.red = 0xC000; rose.green = 0xFF00; rose.blue  = 0xFFFF; // manuel
	  }
     else if   ( glo->status.flags & PAUSE  )
	       {
	       rose.red = 0xFF00; rose.green = 0xD000; rose.blue  = 0x8000; // pause
	       }
	  else {
	       rose.red = 0x8000; rose.green = 0xFF00; rose.blue  = 0x8000; // run
	       }
     }
else if   ( glo->status.flags & MANU  )
          {
	  rose.red = 0xC000; rose.green = 0xA000; rose.blue  = 0xFFFF;	// manuel
	  }
     else {
	  rose.red = 0xFFFF; rose.green = 0xFFFF; rose.blue  = 0x8000;	// repos
	  }

gtk_widget_modify_base( glo->esta, GTK_STATE_NORMAL, &rose );

if   ( ( glo->status.step ) &&
       ( glo->ptube->recette.stat == 4 ) &&
       ( glo->ptube->recette.step[glo->status.step].existe )
     )
     gtk_entry_set_text( GTK_ENTRY(glo->este),
			 glo->ptube->recette.step[glo->status.step].titre.c_str() );
else gtk_entry_set_text( GTK_ENTRY(glo->este), "" );

// affichage recette
if   ( glo->ptube->recette.stat >= 2 )
     {
     gtk_entry_set_text( GTK_ENTRY(glo->ere1), glo->ptube->recette.filename.c_str() );
     gtk_widget_show( glo->brvi );
     }
else {
     gtk_entry_set_text( GTK_ENTRY(glo->ere1), "" );
     gtk_widget_hide( glo->brvi );
     }

switch( glo->ptube->recette.stat )
   {
   case 2 : gtk_entry_set_text( GTK_ENTRY(glo->ere2), "Transmission" );
	    rose.red = 0xFF00; rose.green = 0xD000; rose.blue  = 0x8000; break;
   case 3 : gtk_entry_set_text( GTK_ENTRY(glo->ere2), "Verification" );
	    rose.red = 0xFF00; rose.green = 0xFF00; rose.blue  = 0x8000; break;
   case 4 : gtk_entry_set_text( GTK_ENTRY(glo->ere2), glo->ptube->recette.titre.c_str() );
	    rose.red = 0x8000; rose.green = 0xFF00; rose.blue  = 0x8000; break;
   default : gtk_entry_set_text( GTK_ENTRY(glo->ere2), "~ aucune recette connue ~" );
	    rose.red = 0xC000; rose.green = 0xC000; rose.blue  = 0xC000; break;
   } // switch recette.stat
gtk_widget_modify_base( glo->ere2, GTK_STATE_NORMAL, &rose );
}

// mise a jour affichage a chaque seconde
void display_sec_status( glostru * glo, int * sbuf )
{
char tbuf[256];
if	( sbuf[0] >= 0 )
	{
	double freq;
	#define KF (10000000.0/33554432.0)
	freq = KF * (double)sbuf[1];
	if	( ( sbuf[0] > 0 ) && ( sbuf[2] < 100 ) )
		sprintf( tbuf, "stat=%d, freq=%7.1fHz, t=%d", sbuf[0], freq, sbuf[2] );
	else if	( sbuf[3] > 0 )
		sprintf( tbuf, "stat=%d, freq=%7.1fHz, flow sw=%d", sbuf[0], freq, sbuf[3] );
	else	sprintf( tbuf, "stat=%d, freq=%7.1fHz", sbuf[0], freq );
	}
else	sprintf( tbuf, "?!?" );
gtk_entry_set_text( GTK_ENTRY(glo->esec), tbuf );
}

/** ============================ call backs ======================= */

gint close_X_event_call( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   data )
{
// if	( confirmed )
	gtk_main_quit();
return (TRUE);
}

void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_main_quit();
}

int idle_call( glostru * glo )
{
static int tinks = 0;		// compteur en dixiemes de secondes
static int tonks = 7;		// compteur en secondes pour verif crc lente
static time_t old_t = 0;
time_t it;
int secbuf[4];

switch( ++tinks )
   {
   case 1 :
     {				// scan principal
     get_status( &glo->status );
     display_status( glo );
     plot_status( glo );	// sortie vers fichier pour courbes
     } break;
   case 3 :
     if ( glo->show.auto_secu )
	{				// scan automate secu
	secu_get_status( secbuf );
	display_sec_status( glo, secbuf );
	} break;
   case 5 :
     {
     if ( ( ++tonks >= 10 ) ||	// scan longue periode
	  ( glo->ptube->recette.stat == 2 ) ||	// plus rapide dans ces 2 cas
	  ( glo->ptube->recette.stat == 3 )
	)
	{
	get_crc( &glo->status );
	tonks = 0;
	switch( glo->ptube->recette.stat ) // eventuellement on traite recette
		{
		case 2 :	// c'est choose_call() ou ptube->autosync() qui l'a mis a 2
			if ( ( glo->status.crc_stat & CRC_READY ) &&
			     ( glo->status.packlen == glo->ptube->recette.packlen ) &&
			     ( glo->status.crc == glo->ptube->recette.crc )
			   )
			   {
			   set_crc_autor();
			   glo->ptube->recette.stat = 3;
			   }
			break;
		case 3 :
			if ( glo->status.crc_stat & CRC_AUTOR )
			   {
			   glo->ptube->recette.stat = 4;
			   if ( glo->status.step != 0 )		// recette deja en cours d'execution
			      plot_new( glo );			// alors commencer un plot
			   }
			break;
		case 4 :		// detecter defaillance automate
			if ( !( ( glo->status.crc_stat & CRC_READY ) &&
				( glo->status.packlen == glo->ptube->recette.packlen ) &&
				( glo->status.crc == glo->ptube->recette.crc )
			      )
			   )
			   glo->ptube->recette.init();	// met stat a -1
			break;
		case -1 :
			if ( glo->status.crc_stat & CRC_READY )
			   glo->ptube->autosync( glo->status.crc );
		} // switch stat
	} // if <condition scan>
     } break;
   default :
   if ( tinks > 6 )	// re-synchronisation
      {
      time ( &it );
      if ( it > old_t )
	 {
	 old_t = it;
	 tinks = 0;
	 }
      } break;
   } // switch tinks
return( -1 );
}

void manual_call( GtkWidget *widget, glostru * glo )
{
set_manu( ( glo->status.flags & PAUSE ) | MANU );
show_manu( 1, glo );
}

void auto_call( GtkWidget *widget, glostru * glo )
{
set_manu( glo->status.flags & PAUSE );
show_manu( 0, glo );
}

void pause_call( GtkWidget *widget, glostru * glo )
{
set_manu( PAUSE );
// show_pause( 1, glo ); sera fait par display_status()
}

void cont_call( GtkWidget *widget, glostru * glo )
{
set_manu( 0 );
// show_pause( 0, glo ); sera fait par display_status()
}

void start_call( GtkWidget *widget, glostru * glo )
{
if   ( glo->ptube->recette.stat != 4 )
     {
     if ( modpopYN( "ATTENTION",
                  "La recette de l'automate n'est pas connue du superviseur\nvoulez-vous la lancer \"en aveugle\" ?",
                  " OUI ", " NON ", GTK_WINDOW(glo->wmain) )
        ) set_step(1);
     }
else set_step(1);
}

void stop_call( GtkWidget *widget, glostru * glo )
{
set_step(0);
}

void saut_call( GtkWidget *widget, glostru * glo )
{
int i;
i = view_recette( &glo->ptube->recette, glo->show.scale_type, glo->status.step, JMP );
if ( i > 0 )
   set_step( i );
}

void fast_call( GtkWidget *widget, glostru * glo )
{
set_chron( glo->status.chrono + 10 );
}

void choose_call( GtkWidget *widget, glostru * glo )
{
int i = catalog_recettes( glo->ptube );
if ( i < 0 )
   return;

int editflag = i & 0x4000;
i &= (~0x4000);

if   ( editflag )
     {
     recipe tmprec( glo->ptube );

     // compilons la recette en detail (N.B. methode load_xml appelle methode init)
     if ( (unsigned int)i >= glo->ptube->reclist.size() )
	return;
     tmprec.filename = glo->ptube->reclist[i].filename;
     tmprec.load_xml();
     if ( tmprec.stat < -1 )
	{
	modpop( "ATTENTION erreur compilation recette",
		tmprec.errmess.c_str(), GTK_WINDOW(glo->wmain) );
	return;
	}
     view_recette( &tmprec, glo->show.scale_type, 0, EDIT );
     }
else {
     if ( glo->status.step > 0 )
	{
	modpop( "ATTENTION chargement refuse",
		"une recette est en cours d'execution", GTK_WINDOW(glo->wmain) );
	return;
	}

     // compilons la recette en detail (N.B. methode load_xml appelle methode init)
     if ( (unsigned int)i >= glo->ptube->reclist.size() )
	return;
     glo->ptube->recette.filename = glo->ptube->reclist[i].filename;
     glo->ptube->recette.load_xml();
     if ( glo->ptube->recette.stat > -2 )
	glo->ptube->recette.check();
     if ( glo->ptube->recette.stat > -2 )
	glo->ptube->recette.make_pack();
     if ( glo->ptube->recette.stat < -1 )
	{
	modpop( "ATTENTION erreur compilation recette",
		glo->ptube->recette.errmess.c_str(), GTK_WINDOW(glo->wmain) );
	return;
	}

     printf("about to upload %d bytes @ 300\n", glo->ptube->recette.packlen );
     if   ( glo->ptube->recette.packlen <= (3*256) )
	  upload( 0x300, glo->ptube->recette.pack, glo->ptube->recette.packlen );
     else gasp("recette trop grosse %d", glo->ptube->recette.packlen );
     glo->ptube->recette.stat = 2;
     }
}

void detail_call( GtkWidget *widget, glostru * glo )
{
view_recette( &glo->ptube->recette, glo->show.scale_type, glo->status.step, 0 );
}

/** ============================ constr. GUI ======================= */

/* fonction qui cree une boite horizontale contenant le status du step courant
 */
GtkWidget * mk_hste( glostru *glo )
{
GtkWidget *curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
glo->hste = curwidg;

/* entree non editable */
curwidg = gtk_entry_new_with_max_length (40);
gtk_widget_set_usize (curwidg, 200, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX(glo->hste), curwidg, FALSE, FALSE, 0 );
glo->esta = curwidg;

/* entree non editable */
curwidg = gtk_entry_new_with_max_length (100);
// gtk_widget_set_usize (curwidg, 150, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX(glo->hste), curwidg, TRUE, TRUE, 0 );
glo->este = curwidg;

return( glo->hste );
}

/* fonction qui cree un cadre horizontal contenant la recette
 */
GtkWidget * mk_frec( glostru *glo )
{
GtkWidget *curwidg;

/* creer cadre avec label (devra contenir box si on veut marge */
curwidg = gtk_frame_new ("Recette en cours");
glo->frec = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_container_add( GTK_CONTAINER( glo->frec ), curwidg );
glo->hrec = curwidg;

/* super magic bouton */
curwidg = gtk_button_new_with_label(" Catalogue ");
// curwidg = gtk_button_new_from_stock( GTK_STOCK_OPEN );
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( choose_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hrec ), curwidg, FALSE, FALSE, 0 );
glo->bcho = curwidg;

/* entree non editable */
curwidg = gtk_entry_new_with_max_length (100);
// gtk_widget_set_usize (curwidg, 100, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX(glo->hrec), curwidg, TRUE, TRUE, 0 );
glo->ere1 = curwidg;

/* entree non editable */
curwidg = gtk_entry_new_with_max_length (100);
// gtk_widget_set_usize (curwidg, 120, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX(glo->hrec), curwidg, TRUE, TRUE, 0 );
glo->ere2 = curwidg;

/* hyper magic bouton */
curwidg = gtk_button_new_with_label(" Détail ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( detail_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hrec ), curwidg, FALSE, FALSE, 0 );
glo->brvi = curwidg;

return( glo->frec );
}


/* fonction qui cree une boite horizontale contenant les boutons
 */
GtkWidget * mk_hbut( glostru *glo )
{
GtkWidget *curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Manuel ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( manual_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->status.flags = 0;
glo->bman = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Auto ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( auto_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->baut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Pause ");
// curwidg = gtk_button_new_from_stock( GTK_STOCK_MEDIA_PAUSE );
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pause_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bpau = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Cont ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( cont_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bcon = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Start ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( start_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bsta = curwidg;
//*/

/* simple bouton */
curwidg = gtk_button_new_with_label (" Stop ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( stop_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bsto = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Sauter ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( saut_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bsau = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" >> ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( fast_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bfas = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bqui = curwidg;

return( glo->hbut );
}

void mk_main_text_GUI( glostru *glo )
{
GtkWidget *curwidg;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

// pour garder le controle de la situation (bouton X du bandeau de la fenetre)
g_signal_connect( GTK_OBJECT(curwidg), "delete_event",
		  G_CALLBACK( close_X_event_call ), NULL );
// juste pour le cas ou on aurait un gasp() qui destroy abruptment la fenetre principale
g_signal_connect( GTK_OBJECT(curwidg), "destroy",
		  G_CALLBACK( gtk_main_quit ), NULL );

char lbuf[256];
sprintf( lbuf, "%s (Frevo %d.%d%c)", glo->ptube->nom.c_str(), VERSION, SUBVERS, BETAVER );
gtk_window_set_title( GTK_WINDOW (curwidg), lbuf );
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), SPACE );
glo->wmain = curwidg;

/* creer boite verticale */
curwidg = gtk_vbox_new( FALSE, VSPACE ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

     curwidg = mk_fvan( glo );
     gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );

     curwidg = mk_fmfc( glo );
     gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );

     curwidg = mk_ftem( glo );
     gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );

     curwidg = mk_faux( glo );
     gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );

/* creer boite horizontale pour le step */
curwidg = mk_hste( glo );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );


/* creer boite horizontale pour les boutons */
curwidg = mk_hbut( glo );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );


/* creer cadre horizontal pour la recette */
curwidg = mk_frec( glo );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );

gtk_widget_show_all ( glo->wmain );
}


void mk_main_art_GUI( glostru *glo )
{
GtkWidget *curwidg;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

// pour garder le controle de la situation (bouton X du bandeau de la fenetre)
g_signal_connect( GTK_OBJECT(curwidg), "delete_event",
		  G_CALLBACK( close_X_event_call ), NULL );
// juste pour le cas ou on aurait un gasp() qui destroy abruptment la fenetre principale
g_signal_connect( GTK_OBJECT(curwidg), "destroy",
		  G_CALLBACK( gtk_main_quit ), NULL );

char lbuf[256];
sprintf( lbuf, "%s (Frevo %d.%d%c)", glo->ptube->nom.c_str(), VERSION, SUBVERS, BETAVER );
gtk_window_set_title( GTK_WINDOW (curwidg), lbuf );
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), SPACE );
glo->wmain = curwidg;

/* creer boite verticale principale */
curwidg = gtk_vbox_new( FALSE, VSPACE ); /* spacing ENTRE objets */
glo->vmain = curwidg;

if   ( glo->show.scroll )
     {
     gtk_widget_set_size_request( glo->wmain, 780, 580 );
     /* creer scrolled window. */
     curwidg = gtk_scrolled_window_new( NULL, NULL );
     gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 0 );
     gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg ),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
     gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
     glo->wscro = curwidg;
     /* y placer la boite verticale */
     gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( glo->wscro ), glo->vmain );
     }
else gtk_container_add( GTK_CONTAINER( glo->wmain ), glo->vmain );

/* placer la drawing area */
curwidg = mk_dart( glo );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );

/* creer boite horizontale pour step, boutons, recette, unites */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
glo->hbar = curwidg;

/* creer boite verticale pour step, boutons, recette */
curwidg = gtk_vbox_new( FALSE, VSPACE ); /* spacing ENTRE objets */
gtk_box_pack_start( GTK_BOX( glo->hbar ), curwidg, TRUE, TRUE, 0 );
glo->vbar = curwidg;

/* creer boite horizontale pour le step */
curwidg = mk_hste( glo );
gtk_box_pack_start( GTK_BOX( glo->vbar ), curwidg, FALSE, FALSE, 0 );

/* creer boite horizontale pour les boutons */
curwidg = mk_hbut( glo );
gtk_box_pack_start( GTK_BOX( glo->vbar ), curwidg, FALSE, FALSE, 0 );

/* creer cadre horizontal pour la recette */
curwidg = mk_frec( glo );
gtk_box_pack_start( GTK_BOX( glo->vbar ), curwidg, FALSE, FALSE, 0 );

/* petite boite verticale : radio-buttons pour le choix d'unites, automate secu */
curwidg = mk_vmisc( glo );
gtk_box_pack_start( GTK_BOX( glo->hbar ), curwidg, TRUE, FALSE, 0 );

gtk_widget_show_all ( glo->wmain );
}

void usage( char * moi )
{
gasp("usage : %s &lt;fours.xml> &lt;numero_de_four> {-t|-s}", moi );
}

int main( int argc, char *argv[] )
{
glostru theglo;
#define glo (&theglo)

gtk_init(&argc,&argv);

// problemo : gtk_init peut d'autorite configurer les libs en french,
// alors certaines fonctions de glibc remplacent le point decimal par 1 virgule !!
setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init

glo->ptube = &tube;

if   	( argc < 3 )
	usage( argv[0] );

// lire les donnees de config
glo->ptube->ifou = atoi( argv[2] );
glo->ptube->load_xml( argv[1] );
glo->ptube->scan_rec();

// options
glo->show.txt = 0; glo->show.scroll = 0;

for	( int iopt = 3; iopt < argc; ++iopt )
	{
	if	( argv[iopt][0] == '-' )
		{
		switch	( argv[iopt][1] )
			{
			case 't' : glo->show.txt = 1; break;	// option d'affichage irrevocable
			case 's' : glo->show.scroll = 1; break;	// option d'affichage irrevocable
			default : usage( argv[0] );
			}
		}
	else	usage( argv[0] );
	}

glo->show.txt_temp = 0;
if ( glo->ptube->tem[0].preset[0] != 0 )
   glo->show.txt_temp = 1;

glo->show.txt_aux = 0;
if ( glo->ptube->fre.name != string("--") )
   glo->show.txt_aux = 1;

glo->show.auto_secu = 0;
if ( glo->ptube->auto_secu.size() > 1 )
   {
   if ( glo->ptube->auto_secu[0] == 'm' )
      glo->ptube->magic_step = atoi( glo->ptube->auto_secu.c_str() + 1 );
   printf("Automate securite = %s, magic step %d\n", glo->ptube->auto_secu.c_str(), glo->ptube->magic_step );
   glo->show.auto_secu = 1;
   }

dialugue_set_IP( glo->ptube->destIP );

init_pilot();

fpilot_set_verbose( glo->ptube->comm_verbose );

if ( glo->ptube->comm_log.size() )
   {
   FILE * f;
   f = fopen( glo->ptube->comm_log.c_str(), "w" );
   if ( f == NULL )
      gasp("echec ouverture fichier log %s", glo->ptube->comm_log.c_str() );
   printf("log communication dans %s, verbose %02X\n",
	  glo->ptube->comm_log.c_str(), glo->ptube->comm_verbose );
   fpilot_set_log( f );
   }


// construire le GUI

if   ( glo->show.txt )
     {
     mk_main_text_GUI( glo );
     if ( !glo->show.txt_temp ) gtk_widget_hide( glo->ftem );
     if ( !glo->show.txt_aux )  gtk_widget_hide( glo->faux );
     }
else mk_main_art_GUI( glo );

global_main_window = GTK_WINDOW(glo->wmain);	// pour gasp() de modpop2.c

show_manu( 0, glo );
show_pause( 0, glo );
glo->old_flags = 0;

// action

// brancher la fonction idle, qui doit retourner TRUE
glo->idle_id = g_timeout_add( 100, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

gtk_main(); // on va rester dans cette fonction jusqu'a ce qu'une callback appelle gtk_main_quit();
g_source_remove( glo->idle_id );

end_pilot();
return(0);
}
