/* GUI graphique artistique
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>

#include "modpop2.h"
#include "modadj.h"
#include "../../ipilot.h"
#include "../fpilot.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"
#include "../dirlist.h"
#include "glostru.h"
#include "artgui.h"

// mise a jour des visibilites des widgets en manuel ou auto
void show_art_manu( int flag, glostru * glo  )
{
// items du menu vanne
gtk_widget_set_sensitive( glo->mivou, flag );
gtk_widget_set_sensitive( glo->mivfe, flag );
// items du menu temp
gtk_widget_set_sensitive( glo->mite[2], flag );
gtk_widget_set_sensitive( glo->mite[1], flag );
gtk_widget_set_sensitive( glo->mite[0], flag );
gtk_widget_set_sensitive( glo->mitea, flag );
}


// composer une image sur le pixbuf destpix
void put_sprite( glostru * glo, const string &name, int x, int y, int centered )
{
GdkPixbuf * sprite;
if ( glo->pix.count(name) == 0 )
   gasp("graphics file %s.png not found", name.c_str() );
sprite = glo->pix[name];
if ( centered )
   {
   x -= ( gdk_pixbuf_get_width(sprite) / 2 );
   y -= ( gdk_pixbuf_get_height(sprite) / 2 );
   }
gdk_pixbuf_composite( sprite,		// src
	glo->destpix,			// dest
	x, y,				// dest_x,y, 
	gdk_pixbuf_get_width(sprite),	// dest w
	gdk_pixbuf_get_height(sprite),	// dest h
	x, y,				// offset_x,y
	1.0, 1.0,			// scale_x,y
	GDK_INTERP_NEAREST,
	255				// overall_alpha
	);
}

// affichage graphique des gaz dans les tuyaux
// cette fonction accepte des arguments des types vodget * et modget *
template <typename PPOD> void aff_gaz( PPOD pp, glostru * glo,
					vector <string> * srcX )
{
char tipo; int ipod, isrc, x, y; unsigned int iX;
ostringstream obuf;
string ssrc;
// on pourrait tester le type PPOD avec typeid(), mais on prefere
// l'identifier avec la methode virtuelle tipo()
tipo = pp->tipo();
ipod = glo->ptube->get_ipod( pp );

/* petite experience - semble marcher
if ( typeid(PPOD) == typeid(vodget *) )
   printf("youpi V = %c\n", tipo );
if ( typeid(PPOD) == typeid(modget *) )
   printf("youpi M = %c\n", tipo );
*/

// mise a jour du flow de sortie
pp->sortie.debit = 0;	// sortie nulle si pas de source...
for  ( iX = 0; iX < srcX->size(); iX++ )
     {
     ssrc = (*srcX)[iX];
     // printf("%c%d ssrc = %s\n", tipo, ipod, ssrc.c_str() );
     if ( ssrc.size() >= 2 )
	switch( ssrc[0] )
		{
		case 'F' : pp->sortie.debit = 5;
			   pp->sortie.gaz = ssrc[1];
			   break;
		case 'V' : isrc = atoi( ssrc.c_str() + 1 );
			   if ( ( isrc < 0 ) || ( isrc >= QVAN ) )
			      gasp("num src inattendu pour %c %d", tipo, ipod );
			   pp->sortie = glo->ptube->vanne[isrc].sortie;
			   break;
		case 'M' : isrc = atoi( ssrc.c_str() + 1 );
			   if ( ( isrc < 0 ) || ( isrc >= QMFC ) )
			      gasp("num src inattendu pour %c %d", tipo, ipod );
			   pp->sortie = glo->ptube->mfc[isrc].sortie;
			   break;
		default : gasp("src inattendu pour %c %d", tipo, ipod );
		}
     if ( pp->sortie.debit )
	break;	// premiere source prioritaire
     }
// printf("%c%d sortie %c%d\n", tipo, ipod, pp->sortie.gaz, pp->sortie.debit );

// graphisme gaz
x = pp->gazx;
y = pp->gazy;
if   ( ( x ) && ( pp->sortie.gaz > ' ' ) && ( pp->sortie.debit > 0 ) ) 
     {
     obuf.str( string("") );	// vidage de l'ostringstream
     obuf << 'G' << tipo << ipod <<       pp->sortie.gaz
				 << char( pp->sortie.debit + '0' );
     // cas particulier d'une vanne 3 voies "1 entree 2 sortie"
     // le nom du graphisme a un suffixe _0 ou _1 selon position 
     if	( ( tipo == 'V' ) && ( glo->pix.count( obuf.str() ) == 0 ) )
	{
	obuf.str( string("") );	// vidage de l'ostringstream
	obuf << 'G' << tipo << ipod << '_'
	     << ( ( glo->status.vannes & ( 1 << ipod ) ) ? '1' : '0' )
	     << pp->sortie.gaz << char( pp->sortie.debit + '0' );
	}
     // printf("obuf 2 = %s\n", obuf.str().c_str() );
     put_sprite( glo, obuf.str(), x, y, 0 );
     }
}


// mise a jour affichage a chaque seconde
// en fait cette fonction met a jour separement :
//	le pixbuf "destpix" sur lequel elle compose les sprites (vannes et gaz)
//	les pango layouts dans lequels elle copie les textes 
// ces elements seront copies sur la drawing area par la fonction expose_call()
//
void display_art_status( glostru * glo )
{
ostringstream obuf; int ivan, imfc, x, y;

// mise a jour scale_type 
if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( glo->rsca[1]) ) )
     glo->show.scale_type = 'p';
else if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( glo->rsca[2]) ) )
          glo->show.scale_type = 'u';
     else glo->show.scale_type = 'v';

// reconstitution du fond (NE PAS UTILISER gdk_pixbuf_copy qui re-allouerait la memoire !)
// on pourrait le faire avec gdk_pixbuf_composite, mais gdk_pixbuf_copy_area est moins lourd
gdk_pixbuf_copy_area( glo->backpix,		// src
	0, 0,					// src x, y
	gdk_pixbuf_get_width(glo->backpix),	// dest w
	gdk_pixbuf_get_height(glo->backpix),	// dest h
	glo->destpix,				// dest
	0, 0					// dest_x,y, 
	);

// affichage gaz des vannes
for	( ivan = 0; ivan < QVAN; ivan++ )
	{
	vodget * pp = &(glo->ptube->vanne[ivan]);
	if	( glo->status.vannes & ( 1 << ivan ) )
		aff_gaz( pp, glo, &(pp->src1) );
	else	aff_gaz( pp, glo, &(pp->src0) );
	}	// for ivan

// affichage gaz des mfcs
for	( imfc = 0; imfc < QMFC; imfc++ )
	{
	modget * pp = &(glo->ptube->mfc[imfc]);
	aff_gaz( pp, glo, &(pp->src) );
	}	// for imfc

// affichage MFCs : texte puis graphique avec voyants colores
for	( imfc = 0; imfc < QMFC; imfc++ )
	{
	if   ( glo->ptube->mfc[imfc].pang )
	     {
	     obuf.str( string("") );	// vidage de l'ostringstream
	     obuf << "<tt>M" << imfc << " " << glo->ptube->mfc[imfc].name << "</tt>" << endl;
	     obuf << "PV ";
	     /*
	     // conversion ADC : raffinee par l'usage des conversions 'majuscules'
	     val = glo->ptube->mfc[imfc].pcu2uiu( glo->status.mfc[imfc].pv,
						  glo->show.scale_type - ('a'-'A') );
	     switch( glo->show.scale_type )
		{
		case 'v' : obuf << fixed << setprecision(3) << setw(6) << val << " V"; break;
		case 'p' : obuf << fixed << setprecision(1) << setw(6) << val << " %"; break;
		case 'u' : obuf << fixed << setprecision(2) << setw(6) << val << " "
				<< glo->ptube->mfc[imfc].unit; break;
		default  : obuf << val;
		}
	     */
	     // conversion ADC : raffinee par l'usage des conversions 'majuscules'
	     glo->ptube->mfc[imfc].pcu2stream( obuf, glo->status.mfc[imfc].pv,
					       glo->show.scale_type - ('a'-'A') );

	     // cas particulier "vaporizer" : pas de SV pour le carrier
	     if	( glo->ptube->mfc[imfc].name != string("Porteur") )
		{
		obuf << endl << "SV ";
		/*
		val = glo->ptube->mfc[imfc].pcu2uiu( glo->status.mfc[imfc].sv, glo->show.scale_type );

		// if   ( glo->ptube->mfc[imfc].name != string("Porteur") )
		switch( glo->show.scale_type )
		{
		case 'v' : obuf << fixed << setprecision(3) << setw(6) << val << " V"; break;
		case 'p' : obuf << fixed << setprecision(1) << setw(6) << val << " %"; break;
		case 'u' : obuf << fixed << setprecision(2) << setw(6) << val << " "
				<< glo->ptube->mfc[imfc].unit; break;
		default  : obuf << val;
		}
		*/
		glo->ptube->mfc[imfc].pcu2stream( obuf, glo->status.mfc[imfc].sv, glo->show.scale_type );
		}
	     // printf("#obuf = %s\n", obuf.str().c_str() );
	     pango_layout_set_markup( (PangoLayout *)glo->ptube->mfc[imfc].pang,
				      obuf.str().c_str(), -1 );
	     // le -1 est pour length <==> null terminated
	     }
	// mise a jour du graphisme du mfc avec niveaux de 0 a 5 (off, tres bas, bas, ok, haut, tres haut)
	x = glo->ptube->mfc[imfc].pixx;
	y = glo->ptube->mfc[imfc].pixy;
	if   ( x )
	     {
	     int level;
	     int diff = glo->status.mfc[imfc].pv - glo->status.mfc[imfc].sv;
	     if ( glo->ptube->mfc[imfc].sortie.debit == 0 )
		level = 0;	else
	     if ( diff < ( -65472 / 8  ) )	// N.B. : full-scale = 65472 ici
		level = 1;	else
	     if ( diff < ( -65472 / 16 ) )
		level = 2;	else
	     if ( diff <= ( 65472 / 16 ) )
		level = 3;	else
	     if ( diff <= ( 65472 / 8  ) )
		level = 4;	else
	     level = 5;	
 	     obuf.str( string("") );	// vidage de l'ostringstream
	     obuf << "M" << level;
	     // printf("obuf 1 = %s\n", obuf.str().c_str() );
	     put_sprite( glo, obuf.str(), x, y, 1 );
	     }
	}	// for imfc
// affichage vannes : texte puis graphique symbolisant l'etat
for	( ivan = 0; ivan < QVAN; ivan++ )
	{
	// mise a jour des textes
	if   ( glo->ptube->vanne[ivan].pang )
	     {
	     obuf.str( string("") );	// vidage de l'ostringstream - on n'a pas trouve mieux ?
	     obuf << "V" << ivan << " " << glo->ptube->vanne[ivan].name << endl;
	     if   ( glo->status.vannes & ( 1 << ivan ) )
                  obuf << "1";
             else obuf << "0";
	     pango_layout_set_markup( (PangoLayout *)glo->ptube->vanne[ivan].pang,
				      obuf.str().c_str(), -1 );
	     // le -1 est pour length <==> null terminated
	     // printf("obuf = %s\n", obuf.str().c_str() );
	     }
	// mise a jour du graphisme de la vanne
	x = glo->ptube->vanne[ivan].pixx;
	y = glo->ptube->vanne[ivan].pixy;
	if   ( x )
	     {
	     obuf.str( string("") );	// vidage de l'ostringstream
	     obuf << glo->ptube->vanne[ivan].pix;
	     if   ( glo->status.vannes & ( 1 << ivan ) )
                  obuf << '1';
             else obuf << '0';
	     // printf("obuf 1 = %s\n", obuf.str().c_str() );
	     put_sprite( glo, obuf.str(), x, y, 1 );
	     }
	}	// for ivan

// affichage temperatures 
for	( int item = 0; item < QTEM; item++ )
	{
	if   ( glo->ptube->tem[item].pang )
	     {
	     obuf.str( string("") );	// vidage de l'ostringstream
	     obuf << "<tt><span background=\"#666666\" foreground=\"#FFFFFF\"> T" << item;
	     obuf << " " << setw(9) << glo->ptube->tem[item].name << " </span>\n";
	     obuf << "<b><span background=\"#333333\" foreground=\"#FF3300\"> PV ";
	     /*
	     val = glo->ptube->tem[item].pcu2uiu( glo->status.temp[item].pv, 'd' );
	     obuf << fixed << setprecision(1) << setw(6) << val << " °C </span>\n";
	     obuf << "<span background=\"#333333\" foreground=\"#00CC66\"> SV ";
	     val = glo->ptube->tem[item].pcu2uiu( glo->status.temp[item].sv, 'd' );
	     obuf << fixed << setprecision(1) << setw(6) << val << " °C </span></b></tt>";
	     */
	     glo->ptube->tem[item].pcu2stream( obuf, glo->status.temp[item].pv, 'd' );
	     obuf << " </span>\n";
	     obuf << "<span background=\"#333333\" foreground=\"#00CC66\"> SV ";
	     glo->ptube->tem[item].pcu2stream( obuf, glo->status.temp[item].sv, 'd' );
	     obuf << " </span></b></tt>";
	     pango_layout_set_markup( (PangoLayout *)glo->ptube->tem[item].pang,
				      obuf.str().c_str(), -1 );
	     // le -1 est pour length <==> null terminated
	     // printf("obuf = %s\n", obuf.str().c_str() );
	     }
	}	// for item

// affichage analog aux V/F

// affichage Hz brut (sans passer par fonctions de conversion)

if   ( glo->ptube->fre.pang )
     {
     /* mauvaise bidouille qui ignore fs alors que fs est pris en compte par la recette
     #define KF (10000000/33554432.0)
     val = KF * (double)glo->status.frequ;

     // correction d'offset
     val += glo->ptube->fre.offset;
     if ( val < 0.0 ) val = 0.0;

     obuf.str( string("") );	// vidage de l'ostringstream
     if   ( glo->ptube->fre.unit == string("mTorr") )
          {
	  // traitement particulier pour pression Baratron
	  if	( val < 13000 )
		{
		obuf << fixed << setprecision(1) << setw(6) << val << " mT";
		}
	  else	{
		obuf << "> 13000 mT";
		}
	  }
     else {
	  obuf << fixed << setprecision(1) << setw(6) << val;
	  }
     */
     obuf.str( string("") );	// vidage de l'ostringstream
     obuf << glo->ptube->fre.name << endl;
     glo->ptube->fre.pcu2stream( obuf, glo->status.frequ, 'f' );

     pango_layout_set_markup( (PangoLayout *)glo->ptube->fre.pang,
			      obuf.str().c_str(), -1 );
     // printf("obuf = %s\n", obuf.str().c_str() );
     }


gtk_widget_queue_draw( glo->darea );	// rafraichir l'ecran
}

/** ============================ call backs de la drawing-area ================= */

static gboolean expose_call( GtkWidget * widget,
			     GdkEventExpose * event,
			     glostru * glo)
{
// printf("expozed\n");
gdk_draw_pixbuf( widget->window,
		 NULL,
		 glo->destpix,
		 0, 0, 0, 0,	// src and dest xy
		 -1, -1,	// width and height, here from pixbuf
		 GDK_RGB_DITHER_NONE, 0, 0
		);
// affichage vannes
for	( int ivan = 0; ivan < QVAN; ivan++ )
	{
	if   ( glo->ptube->vanne[ivan].pang )	// pango layout
	     {
	     gdk_draw_layout( widget->window,
			widget->style->black_gc,
			glo->ptube->vanne[ivan].x,
			glo->ptube->vanne[ivan].y,
			(PangoLayout *)glo->ptube->vanne[ivan].pang );
	     }
	}
for	( int imfc = 0; imfc < QMFC; imfc++ )
	{
	if   ( glo->ptube->mfc[imfc].pang )	// pango layout
	     {
	     gdk_draw_layout( widget->window,
			widget->style->black_gc,
			glo->ptube->mfc[imfc].x,
			glo->ptube->mfc[imfc].y,
			(PangoLayout *)glo->ptube->mfc[imfc].pang );
	     }
	}
for	( int item = 0; item < QTEM; item++ )
	{
	if   ( glo->ptube->tem[item].pang )	// pango layout
	     {
	     gdk_draw_layout( widget->window,
			widget->style->black_gc,
			glo->ptube->tem[item].x,
			glo->ptube->tem[item].y,
			(PangoLayout *)glo->ptube->tem[item].pang );
	     }
	}
if   ( glo->ptube->fre.pang )	// pango layout
     {
     gdk_draw_layout( widget->window,
		widget->style->black_gc,
		glo->ptube->fre.x,
		glo->ptube->fre.y,
		(PangoLayout *)glo->ptube->fre.pang );
     }
return FALSE;	// MAIS POURQUOI ???
}

static gboolean configure_call( GtkWidget * widgut,
				GdkEventConfigure * event,
				glostru	* glo)
{
printf("drawing area configured\n");
return TRUE;	// MAIS POURQUOI ???
}


static gboolean click_call( GtkWidget		* widget,
			    GdkEventButton	* event,
			    glostru		* glo )
{
int x, y, ipod, vtype; char tipo; podget * pp;
ostringstream obuf;

x = (int)event->x; y = (int)event->y;

glo->ispot = glo->ptube->whichspot( x, y );

// clic gauche : seulement sur les hot-spots
if ( ( glo->ispot >= 0 ) && ( event->button == 1 ) )
   {
   ipod = glo->ptube->get_ipod(glo->ispot);
   pp = glo->ptube->ppod[glo->ispot];
   tipo = pp->tipo();
   printf("clic %c%d spot %d %d:%d \n", tipo, ipod, glo->ispot, x, y );
   switch( tipo )
	{
	case 'V'  : obuf << "<b>Vanne " << ipod << "</b> " << pp->name;
		    vtype = 0;	// n.f., n.o., 3 voies ?
		    if ( ((vodget *)pp)->src0.size() ) vtype |= 1;
		    if ( ((vodget *)pp)->src1.size() ) vtype |= 2;
		    switch ( vtype )
			{
			case 1  : obuf << " <i>(n.o.)</i>";
				gtk_label_set_markup( GTK_LABEL(glo->lpva1), "Fermer" );
				gtk_label_set_markup( GTK_LABEL(glo->lpva0), "Ouvrir" );
				break;
			case 3  : obuf << " <i>(3 v.)</i>";
				gtk_label_set_markup( GTK_LABEL(glo->lpva1), "Position 1" );
				gtk_label_set_markup( GTK_LABEL(glo->lpva0), "Position 0" );
				break;
			default :	// 0 ou 2 : vanne ordinaire 
				gtk_label_set_markup( GTK_LABEL(glo->lpva1), "Ouvrir" );
				gtk_label_set_markup( GTK_LABEL(glo->lpva0), "Fermer" );
				break;
			}
		    gtk_label_set_markup( GTK_LABEL(glo->lpvan), obuf.str().c_str() );
		    gtk_menu_popup( (GtkMenu *)glo->pop_vanne, NULL, NULL, NULL, NULL,
				event->button, event->time );
		    break;
	case 'M'  : obuf << "<b>MFC " << ipod << "</b> " << pp->name;
		    double val, fs;
		    val = glo->ptube->mfc[ipod].pcu2uiu( glo->status.mfc[ipod].sv,
							 glo->show.scale_type );
		    switch( glo->show.scale_type )
			{
			case 'p' : fs = 100.0;				break;
			case 'u' : fs = glo->ptube->mfc[ipod].fs;	break;
			case 'v' :
			default  : fs = 5.0;
			}

		    if	  ( glo->status.flags & MANU )
			  {
			  val = modadj( "Réglage Débit", obuf.str().c_str(),
					GTK_WINDOW(glo->wmain), val, fs );
			  int ival = glo->ptube->mfc[ipod].uiu2pcu( val, glo->show.scale_type );
			  set_dac( ipod, ival );
			  printf("val = %g\n", val );
			  }
		    else  modpop("Réglage Débit", "seulement en mode manuel", GTK_WINDOW(glo->wmain) );
		    break;
	case 'T'  : obuf << "<b>Régulateur de température " << ipod << "</b> " << pp->name;
		    gtk_label_set_markup( GTK_LABEL(glo->lptem), obuf.str().c_str() );
		    gtk_menu_popup( (GtkMenu *)glo->pop_temp, NULL, NULL, NULL, NULL,
				event->button, event->time );
		    break;
	}
   }

// clic droit
if ( event->button == 3 )
   {
   printf("rlic %d:%d spot %d\n", x, y, glo->ispot );
   switch( glo->ispot )
	{
	case -1 :  gtk_menu_popup( (GtkMenu *)glo->pop_fond, NULL, NULL, NULL, NULL,
			event->button, event->time );
		   break;
	}
   }
/* We've handled the event, stop processing */
return TRUE;
}


static gboolean motion_call( GtkWidget		* widget,
			     GdkEventMotion 	* event,
			     glostru		* glo )
{
int x, y;
GdkModifierType state;
GdkCursor* newcur;
static int oldspot = -1;

gdk_window_get_pointer( event->window, &x, &y, &state );
glo->ispot = glo->ptube->whichspot( x, y );

if   ( glo->ispot != oldspot )	// changer curseur seulement si necessaire
     {
     if   ( glo->ispot >= 0 )
	  {
          newcur = gdk_cursor_new( GDK_HAND2 );
	  }
     else newcur = NULL;	// mother windows's cursor...

     gdk_window_set_cursor( widget->window, newcur );
     if ( newcur )
	gdk_cursor_unref(newcur); // eviter fuite memoire minuscule ?
     oldspot = glo->ispot; 
     }

if   ( state & GDK_BUTTON1_MASK )
     {
     printf("drag %3d:%3d spot %d\n", x, y, glo->ispot );
     }
/*
else {
     printf("move %d:%d spot %d\n", x, y, glo->ispot );
     }
*/
/* We've handled it, stop processing */
return TRUE;
}

/** ============================ call backs des menus ================= */

void ouvrir_vanne_call( GtkWidget *widget, glostru * glo )
{
if ( glo->status.flags & MANU )
   set_van( glo->ptube->get_ipod( glo->ispot ) );
}

void fermer_vanne_call( GtkWidget *widget, glostru * glo )
{
if ( glo->status.flags & MANU )
   reset_van( glo->ptube->get_ipod( glo->ispot ) );
}

void temp_preset_call( GtkWidget *widget, glostru * glo )
{
int ipre, item, ival; double val;
for ( ipre = 0; ipre < 3; ipre++ )
    if	( widget == glo->mite[ipre] )
	{
	if  ( glo->status.flags & MANU )
	    for ( item = 0; item < 3; item++ )
		{
		val = (double)glo->ptube->tem[item].preset[ipre];
		ival = glo->ptube->tem[item].uiu2pcu( val, 'd' );
		set_temp( item, ival );
		}
	break;
	}
}

void temp_adj_call( GtkWidget *widget, glostru * glo )
{
int item; double val; int ival;
podget * pp; char tipo; ostringstream obuf;

item = glo->ptube->get_ipod(glo->ispot);
pp = glo->ptube->ppod[glo->ispot];
tipo = pp->tipo();
if	( ( tipo == 'T' ) && ( item >= 0 ) && ( item < 3 ) )
	{
	obuf << "<b>Régulateur " << item << "</b> " << pp->name;
	val = ((todget *)pp)->pcu2uiu( glo->status.temp[item].sv, 'd' );
	val = modadj( "Réglage Température", obuf.str().c_str(),
			GTK_WINDOW(glo->wmain), val, 1200.0 );
	ival = ((todget *)pp)->uiu2pcu( val, 'd' );
	if ( glo->status.flags & MANU )
           set_temp( item, ival );
	}
}

void urg_stop_call( GtkWidget *widget, glostru * glo )
{
set_step(0);
}

void gasp_call( GtkWidget *widget, glostru * glo )
{
printf("Simulated fatal error from spot %d\n", glo->ispot );
gasp("semblant d'%s <b>%s</b>", "erreur", "fatale");
}

void screendump_call( GtkWidget *widget, glostru * glo )
{
printf("Screen dump called from spot %d\n", glo->ispot );
GdkPixbuf * dumpix;

dumpix = gdk_pixbuf_get_from_drawable(  NULL,		// we request creation of the dest pixbuf  GdkPixbuf *dest,
					glo->darea->window,	// GdkDrawable *src,
					NULL,		// take the  map from the drawable GdkColormap *cmap,
					0, 0,		// int src_x, int src_y,
					0, 0,		// int dest_x, int dest_y,
					gdk_pixbuf_get_width(glo->backpix),	// int width,
					gdk_pixbuf_get_height(glo->backpix)	// int height
				     );
if ( dumpix == NULL )
   gasp("echec copie d'ecran");
printf("dumpix ok\n");
if   ( gdk_pixbuf_save( dumpix, "darea.png", "png",
			NULL,	// GError **
			NULL	// end of options list
		      )
     )
     printf("Ok ecriture copie d'ecran darea.png\n");
else printf("oops, echec ecriture copie d'ecran darea.png\n");
}

/** ============================ constr. GUI ======================= */

// fonctions pour creer le menu de fond d'ecran (un fois pour toutes)
GtkWidget * mkmenu_fond( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;

curmenu = gtk_menu_new ();    /* Don't need to show menus */

/* Create the menu items */
curitem = gtk_menu_item_new_with_label("STOP RECETTE");
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( urg_stop_call ), (gpointer)glo );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("copie d'ecran");
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( screendump_call ), (gpointer)glo );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("emulation erreur fatale");
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( gasp_call ), (gpointer)glo );
gtk_widget_show ( curitem );

return curmenu;
}

// fonctions pour creer le menu de vanne (un fois pour toutes)
GtkWidget * mkmenu_vanne( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;
GtkWidget * curwidg;

curmenu = gtk_menu_new ();    /* Don't need to show menus */

// le premier item n'a pas de call-back, nous l'utilisons juste comme titre
// avec la possibilite de le changer dynamiquement
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), "<b>Vanne</b>" );
glo->lpvan = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curwidg = gtk_label_new( NULL );
glo->lpva1 = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ouvrir_vanne_call ), (gpointer)glo );
gtk_widget_show ( curitem );
glo->mivou = curitem;

curwidg = gtk_label_new( NULL );
glo->lpva0 = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( fermer_vanne_call ), (gpointer)glo );
gtk_widget_show ( curitem );
glo->mivfe = curitem;

return curmenu;
}

// fonctions pour creer le menu de temperature (un fois pour toutes)
GtkWidget * mkmenu_temp( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;
GtkWidget * curwidg;

curmenu = gtk_menu_new ();    /* Don't need to show menus */

// le premier item n'a pas de call-back, nous l'utilisons juste comme titre
// avec la possibilite de le changer dynamiquement
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), "<b>Régulateur</b>" );
glo->lptem = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curwidg = gtk_label_new( NULL );
glo->lpte2 = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( temp_preset_call ), (gpointer)glo );
gtk_widget_show ( curitem );
glo->mite[2] = curitem;

curwidg = gtk_label_new( NULL );
glo->lpte1 = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( temp_preset_call ), (gpointer)glo );
gtk_widget_show ( curitem );
glo->mite[1] = curitem;

curwidg = gtk_label_new( NULL );
glo->lpte0 = curwidg;
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( temp_preset_call ), (gpointer)glo );
gtk_widget_show ( curitem );
glo->mite[0] = curitem;

curwidg = gtk_label_new( "autre valeur" );
gtk_widget_show ( curwidg );
curitem = gtk_menu_item_new();
gtk_container_add( GTK_CONTAINER(curitem), curwidg );
gtk_menu_shell_append( GTK_MENU_SHELL(curmenu), curitem );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( temp_adj_call ), (gpointer)glo );
gtk_widget_show ( curitem );
glo->mitea = curitem;

ostringstream obuf;

obuf << glo->ptube->tem[0].preset[2] << "/" << glo->ptube->tem[1].preset[2] << "/"
     << glo->ptube->tem[2].preset[2] << " °C";
gtk_label_set_markup( GTK_LABEL(glo->lpte2), obuf.str().c_str() );
obuf.str( string("") );
obuf << glo->ptube->tem[0].preset[1] << "/" << glo->ptube->tem[1].preset[1] << "/"
     << glo->ptube->tem[2].preset[1] << " °C";
gtk_label_set_markup( GTK_LABEL(glo->lpte1), obuf.str().c_str() );
obuf.str( string("") );
obuf << glo->ptube->tem[0].preset[0] << "/" << glo->ptube->tem[1].preset[0] << "/"
     << glo->ptube->tem[2].preset[0] << " °C";
gtk_label_set_markup( GTK_LABEL(glo->lpte0), obuf.str().c_str() );

return curmenu;
}


/* fonction qui cree une drawing area et son contenu... */
GtkWidget * mk_dart( glostru *glo )
{
GtkWidget *curwidg;
string fullpath;

curwidg = gtk_drawing_area_new ();
/* set a minimum size */
gtk_widget_set_size_request (curwidg, 200, 200);

g_signal_connect( curwidg, "expose_event",
                  G_CALLBACK(expose_call), glo );

g_signal_connect( curwidg, "configure_event",
                  G_CALLBACK(configure_call), glo );

g_signal_connect( curwidg, "motion_notify_event",
		  G_CALLBACK( motion_call ), glo );

g_signal_connect( curwidg, "button_press_event",
		  G_CALLBACK( click_call ), glo );

gtk_widget_set_events ( curwidg, gtk_widget_get_events(curwidg)
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_POINTER_MOTION_MASK
			      | GDK_POINTER_MOTION_HINT_MASK
		      );

glo->darea = curwidg;

// lire les images

dirdata recdir( glo->ptube->pix_dir.c_str() );	// lire repertoire sur le disque
if ( recdir.dd.size() == 0 )
   gasp("graphics files missing in %s", glo->ptube->pix_dir.c_str() );

unsigned int i;

for ( i = 0; i < recdir.dd.size(); i++ )
    {
    string fnam, pixext, pixnam;
    fnam = string(recdir.dd[i].name);
    if	( ( recdir.dd[i].type == 'F' ) && ( fnam.size() > 4 ) )
	{
	pixext.append( fnam, fnam.size() - 4, 4 );
	pixnam.append( fnam, 0, fnam.size() - 4 );
	} 
    // printf("~~> %s <%s>\n", pixnam.c_str(), pixext.c_str() );
    if	( pixext == string(".png") )
	{
	fullpath = glo->ptube->pix_dir + char(SLASH) + fnam;
	glo->pix[pixnam] = gdk_pixbuf_new_from_file( fullpath.c_str(), NULL );
	}
    }

printf("lu %u images dans %s\n", (unsigned int)glo->pix.size(), glo->ptube->pix_dir.c_str() );

// extraire image de fond */

if ( glo->pix.count( string("bak") ) == 0 )
   gasp("graphics file bak.png not found" );
glo->backpix = glo->pix[ string("bak") ];

// ajuster la drawing area aux dimensions voulues
gtk_widget_set_size_request( glo->darea,
			     gdk_pixbuf_get_width(glo->backpix),
			     gdk_pixbuf_get_height(glo->backpix) );

// creation pixbuf par copie
glo->destpix = gdk_pixbuf_copy( glo->backpix );

// creation menus
glo->pop_fond  = mkmenu_fond( glo );
glo->pop_vanne = mkmenu_vanne( glo );
glo->pop_temp  = mkmenu_temp( glo );

glo->show.scale_type = 'v';
glo->ispot = -1;

// preparation de pango-layouts pour les textes
int ivan, imfc, item;

for	( ivan = 0; ivan < QVAN; ivan++ )
	{
	if   ( ( glo->ptube->vanne[ivan].y > 0 ) && ( glo->ptube->vanne[ivan].pang == NULL ) )
	     {
	     glo->ptube->vanne[ivan].pang = gtk_widget_create_pango_layout( glo->darea, NULL );
	     // printf("pango layout created for V%d %s\n", ivan, glo->ptube->vanne[ivan].name.c_str() );
	     }
	}
for	( imfc = 0; imfc < QMFC; imfc++ )
	{
	if   ( ( glo->ptube->mfc[imfc].y > 0 ) && ( glo->ptube->mfc[imfc].pang == NULL ) )
	     {
	     glo->ptube->mfc[imfc].pang = gtk_widget_create_pango_layout( glo->darea, NULL );
	     // printf("pango layout created for M%d %s\n", imfc, glo->ptube->mfc[imfc].name.c_str() );
	     }
	}
for	( item = 0; item < QTEM; item++ )
	{
	if   ( ( glo->ptube->tem[item].y > 0 ) && ( glo->ptube->tem[item].pang == NULL ) )
	     {
	     glo->ptube->tem[item].pang = gtk_widget_create_pango_layout( glo->darea, NULL );
	     // printf("pango layout created for T%d %s\n", item, glo->ptube->tem[item].name.c_str() );
	     }
	}
if   ( ( glo->ptube->fre.y > 0 ) && ( glo->ptube->fre.pang == NULL ) )
     {
     glo->ptube->fre.pang = gtk_widget_create_pango_layout( glo->darea, NULL );
     // printf("pango layout created for FRE %s\n", glo->ptube->fre.name.c_str() );
     }

/* dump des parametres extraits de fours.xml, pour verif */
glo->ptube->dump();
//*/

return( glo->darea );
}

