/* popup dialog "modal" pour ajustement valeur analog
   il bloque tout le reste.
   derive de modpop2 

 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include "modadj.h"

/* il faut intercepter le delete event pour que si l'utilisateur
   ferme la fenetre on revienne de la fonction modpop() sans engendrer
   de destroy signal. A cet effet ce callback doit rendre TRUE 
   (ce que la fonction gtk_main_quit() ne fait pas)
 */  
   
static gint delete_call( GtkWidget *widget,
                  GdkEvent  *event,
                  gpointer   data )
 {
 gtk_main_quit();
 return (TRUE); 
 }
 
double modadj(  const char *title, const char *txt, GtkWindow *parent,
		double start, double fs )
{
GtkWidget * curwin;
GtkWidget * curbox;
GtkWidget * curwidg;
GtkWidget * curspin;
GtkAdjustment * curadj;
int digits;

curwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
gtk_window_set_modal( GTK_WINDOW(curwin), TRUE );
gtk_window_set_position( GTK_WINDOW(curwin), GTK_WIN_POS_MOUSE );
gtk_window_set_transient_for(  GTK_WINDOW(curwin), parent );
gtk_window_set_type_hint( GTK_WINDOW(curwin), GDK_WINDOW_TYPE_HINT_DIALOG );

gtk_window_set_title( GTK_WINDOW(curwin), title );
gtk_container_set_border_width( GTK_CONTAINER(curwin), 20 );

gtk_signal_connect( GTK_OBJECT(curwin), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), NULL );

/* creer boite verticale */
curbox = gtk_vbox_new( FALSE, 5 );
gtk_container_add( GTK_CONTAINER( curwin ), curbox );
gtk_widget_show(curbox);
/* placer le message (PANGO MARKUP) */                   
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), txt );
gtk_label_set_line_wrap( GTK_LABEL(curwidg), TRUE );
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, TRUE, TRUE, 0 );
gtk_widget_show( curwidg );

/* un spin button - clic gauche = fs/20, clic milieu fs/5 */
curadj = (GtkAdjustment *) gtk_adjustment_new ( start, 0.0, fs, fs/20, fs/5, 0.0 );
		//	( value, lower, upper, step, page_inc, page_size )
/* un peu d'acceleration : non merci */
curwidg = gtk_spin_button_new( curadj, 0, 0 );

/* no wrap around limits */
gtk_spin_button_set_wrap( GTK_SPIN_BUTTON(curwidg), FALSE );
gtk_widget_set_usize( curwidg, 100, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
     if ( fs > 600.0 ) digits = 0;
else if ( fs > 60.0 ) digits = 1;
else if ( fs > 6.0 ) digits = 2;
else digits = 3;
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), digits );
gtk_box_pack_start( GTK_BOX(curbox), curwidg, FALSE, TRUE, 0 );
gtk_widget_show( curwidg );
curspin = curwidg;

/* un callback : non, merci */
// gtk_signal_connect( GTK_OBJECT(curadj), "value_changed",
//                     GTK_SIGNAL_FUNC( adj_call ), (gpointer)glo );

/* le bouton ok */
curwidg = gtk_button_new_with_label (" Ok ");
gtk_box_pack_start( GTK_BOX( curbox ), curwidg, FALSE, FALSE, 0);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );
gtk_widget_show(curwidg);
gtk_widget_show( curwin );

/* on est venu ici alors qu'on est deja dans 1 boucle gtk_main
   alors donc on en imbrique une autre. Le prochain appel a 
   gtk_main_quit() fera sortir de cell-ci (innermost)
 */  
gtk_main();
start = gtk_spin_button_get_value_as_float( (GtkSpinButton*)curspin );
gtk_widget_destroy( curwin );
return(start);
}
