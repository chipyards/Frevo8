/* GUI avec des widgets texte (spinboxes principalement)
   pour les organes du four
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "modpop2.h"
#include "../../ipilot.h"
#include "../fpilot.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"
#include "glostru.h"
#include "plot.h"
#include "textgui.h"

// mise a jour des visibilites des widgets en manuel ou auto
void show_txt_manu( int flag, glostru * glo  )
{
int i;
if   ( flag )
     {
     for ( i = 0; i < QVAN; i++ )
         if   ( glo->ptube->vanne[i].name[0] == '-' )
	      gtk_widget_hide( glo->svan[i] );
	 else gtk_widget_show( glo->svan[i] );
     for ( i = 0; i < QMFC; i++ )
         if   ( ( glo->ptube->mfc[i].name[0] == '-' ) ||
	        ( glo->ptube->mfc[i].name == string("Porteur") )
	      )
	      gtk_widget_hide( glo->smfc[i] );
	 else gtk_widget_show( glo->smfc[i] );
     for ( i = 0; i < 3; i++ )
         gtk_widget_show( glo->stem[i] );
     }
else {
     for ( i = 0; i < QVAN; i++ )
         gtk_widget_hide( glo->svan[i] );
     for ( i = 0; i < QMFC; i++ )
         gtk_widget_hide( glo->smfc[i] );
     for ( i = 0; i < 3; i++ )
         gtk_widget_hide( glo->stem[i] );
     }
// changement d'echelle interdit en mode manuel
gtk_widget_set_sensitive ( glo->rsca[0] , !flag );
gtk_widget_set_sensitive ( glo->rsca[1] , !flag );
gtk_widget_set_sensitive ( glo->rsca[2] , !flag );
// preset temperature
gtk_widget_set_sensitive ( glo->btpr[0] , flag );
gtk_widget_set_sensitive ( glo->btpr[1] , flag );
gtk_widget_set_sensitive ( glo->btpr[2] , flag );
gtk_widget_set_sensitive ( glo->buni, flag );
}

// mise a jour des spinboxes en fonction des valeurs de SV restituees
// par l'automate et du type d'echelle courant
// attention : cause le retour des valeurs vers l'automate, ne pas iterer !!
void update_spinboxes( glostru * glo )
{
int ipod; double val;
for ( ipod = 0; ipod < QVAN; ipod++ )
    {
    if   ( glo->status.vannes & ( 1 << ipod ) )
         gtk_spin_button_set_value( (GtkSpinButton *)glo->svan[ipod], 1.0 );
    else gtk_spin_button_set_value( (GtkSpinButton *)glo->svan[ipod], 0.0 );
    }
for ( ipod = 0; ipod < QMFC; ipod++ )
    {
    val = glo->ptube->mfc[ipod].pcu2uiu( glo->status.mfc[ipod].sv, glo->show.scale_type );
    switch( glo->show.scale_type )
	{
	// exceptionnel acces direct a un membre d'une structure GTK (GtkAdjustment)
	case 'v' : glo->amfc[ipod]->upper = 5.0;
		 glo->amfc[ipod]->step_increment = 0.05;
		 glo->amfc[ipod]->page_increment = 0.25;
		 break;
	case 'p' : glo->amfc[ipod]->upper = 100.0;
		 glo->amfc[ipod]->step_increment = 1.0;
		 glo->amfc[ipod]->page_increment = 10.0;
		 break;
	case 'u' : glo->amfc[ipod]->upper = glo->ptube->mfc[ipod].fs;
		 glo->amfc[ipod]->step_increment = glo->ptube->mfc[ipod].fs/100.0;
		 glo->amfc[ipod]->page_increment = glo->ptube->mfc[ipod].fs/10.0;
		 break;
	}
    gtk_adjustment_changed(glo->amfc[ipod]);

    gtk_spin_button_set_value( (GtkSpinButton *)glo->smfc[ipod], val );
    }
for ( ipod = 0; ipod < 3; ipod++ )
    {
    val = glo->ptube->tem[ipod].pcu2uiu( glo->status.temp[ipod].sv, 'd' );
    gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[ipod], val );
    }
}

// mise a jour affichage a chaque seconde
void display_txt_status( glostru * glo )
{
int imfc, ivan, item; double val;
char tbuf[256];

/* mise a jour scale_type - attention update_spinboxes en a besoin */
if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( glo->rsca[1]) ) )
     glo->show.scale_type = 'p';
else if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( glo->rsca[2]) ) )
          glo->show.scale_type = 'u';
     else glo->show.scale_type = 'v';


// affichage vannes
for    ( ivan = 0; ivan < QVAN; ivan++ )
       {
       if   ( glo->ptube->vanne[ivan].name[0] == '-' )
	    gtk_entry_set_text( GTK_ENTRY(glo->evan[ivan]), "" );
       else if   ( glo->status.vannes & ( 1 << ivan ) )
                 gtk_entry_set_text( GTK_ENTRY(glo->evan[ivan]), "1" );
            else gtk_entry_set_text( GTK_ENTRY(glo->evan[ivan]), "0" );
       }

// affichage MFCs
for ( imfc = 0; imfc < QMFC; imfc++ )
    {
    if   ( glo->ptube->mfc[imfc].name[0] == '-' )
	 gtk_entry_set_text( GTK_ENTRY(glo->empv[imfc]), "" );
    else {
	 // conversion ADC : raffinee par l'usage des conversions 'majuscules'
	 val = glo->ptube->mfc[imfc].pcu2uiu( glo->status.mfc[imfc].pv, glo->show.scale_type - ('a'-'A') );
	 switch( glo->show.scale_type )
	  {
	  case 'v' : sprintf( tbuf, "PV  %6.4f V", val ); break;
	  case 'p' : sprintf( tbuf, "PV  %5.1f %%", val ); break;
	  case 'u' : sprintf( tbuf, "PV  %6.2f %s", val, glo->ptube->mfc[imfc].unit.c_str() ); break;
	  default: sprintf( tbuf, "PV  %6.4f", val );
	  }
	 gtk_entry_set_text( GTK_ENTRY(glo->empv[imfc]), tbuf );
	 }

    if   ( ( glo->ptube->mfc[imfc].name[0] == '-' ) ||
	   ( glo->ptube->mfc[imfc].name == string("Porteur") )
	 )
	 gtk_entry_set_text( GTK_ENTRY(glo->emsv[imfc]), "" );
    else {
         val = glo->ptube->mfc[imfc].pcu2uiu( glo->status.mfc[imfc].sv, glo->show.scale_type );
         switch( glo->show.scale_type )
	  {
	  case 'v' : sprintf( tbuf, "SV  %6.4f V", val ); break;
	  case 'p' : sprintf( tbuf, "SV  %5.1f %%", val ); break;
	  case 'u' : sprintf( tbuf, "SV  %6.2f %s", val, glo->ptube->mfc[imfc].unit.c_str() ); break;
	  default: sprintf( tbuf, "SV  %6.4f", val );
	  }
         gtk_entry_set_text( GTK_ENTRY(glo->emsv[imfc]), tbuf );
         }
    }

// affichage temperatures

for    ( item = 0; item < QTEM; item++ )
       {
       sprintf( tbuf, "PV  %6.1f °C", glo->ptube->tem[item].pcu2uiu( glo->status.temp[item].pv, 'd' ) );
       gtk_entry_set_text( GTK_ENTRY(glo->etpv[item]), tbuf );

       sprintf( tbuf, "SV  %6.1f °C", glo->ptube->tem[item].pcu2uiu( glo->status.temp[item].sv, 'd' ) );
       gtk_entry_set_text( GTK_ENTRY(glo->etsv[item]), tbuf );
       }

// affichage analog aux V/F

// affichage Hz brut (sans passer par fonctions de conversion)
#define KF (10000000/33554432.0)
val = KF * (double)glo->status.frequ;
sprintf( tbuf, "%7.1f", val );
gtk_entry_set_text( GTK_ENTRY(glo->eaux[0]), tbuf );

// affichage en unites r�lles
val = glo->ptube->fre.pcu2uiu( glo->status.frequ, 'f' );

// correction d'offset
val += glo->ptube->fre.offset;
if ( val < 0.0 ) val = 0.0;

if   ( glo->ptube->fre.unit == string("mTorr") )
     {
     // traitement particulier pour pression Baratron
     if   ( val < 13000 )
          {
	  sprintf( tbuf, "%7.1f", val );
	  gtk_entry_set_text( GTK_ENTRY(glo->eaux[1]), tbuf );
	  val /= 750.0;
	  sprintf( tbuf, "%6.3f", val );
	  gtk_entry_set_text( GTK_ENTRY(glo->eaux[2]), tbuf );
	  }
     else {
	  gtk_entry_set_text( GTK_ENTRY(glo->eaux[1]), "> 13000" );
	  gtk_entry_set_text( GTK_ENTRY(glo->eaux[2]), "> 17.0" );
	  }
     }
else {
     sprintf( tbuf, "%7.1f", val );
     gtk_entry_set_text( GTK_ENTRY(glo->eaux[1]), tbuf );
     gtk_entry_set_text( GTK_ENTRY(glo->eaux[2]), "" );
     }
}

/** ============================ call backs ======================= */

/* action sur vannes : interdite hors mode manuel */
static void van_call( GtkAdjustment *adjet, glostru * glo )
{
int ivan, val;
if ( glo->status.flags & MANU )
   for ( ivan = 0; ivan < QVAN; ivan++ )
       if ( adjet == glo->avan[ivan] )
          {
          val = gtk_spin_button_get_value_as_int((GtkSpinButton*)glo->svan[ivan]);
          if ( val ) set_van( ivan );
          else     reset_van( ivan );
          break;
          }
}

/* action sur MFCs : interdite hors mode manuel */
static void mfc_call( GtkAdjustment *adjet, glostru * glo )
{
int imfc; double val; int ival;
if ( glo->status.flags & MANU )
   for ( imfc = 0; imfc < QMFC; imfc++ )
       if ( adjet == glo->amfc[imfc] )
          {
          val = gtk_spin_button_get_value_as_float((GtkSpinButton*)glo->smfc[imfc]);
	  ival = glo->ptube->mfc[imfc].uiu2pcu( val, glo->show.scale_type );
          set_dac( imfc, ival );
          break;
          }
}

/* action sur regu temperature : interdite hors mode manuel */
static void tem_call( GtkAdjustment *adjet, glostru * glo )
{
int item; double val; int ival;
if ( ( glo->status.flags & MANU ) && ( glo->show.txt_temp ) )
   for ( item = 0; item < 3; item++ )
       if ( adjet == glo->atem[item] )
          {
          val = gtk_spin_button_get_value_as_float((GtkSpinButton*)glo->stem[item]);
	  if ( (val < 0.0 ) || ( val > 1200.0 ) )
	     gasp("valeur TEMP illegale %g", val );
	  ival = glo->ptube->tem[item].uiu2pcu( val, 'd' );
          set_temp( item, ival );
	  if ( (glo->show.uniform_temp ) && ( item == 1 ) ) // copier milieu sur les bouts
	     {
	     gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[0], val );
	     gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[2], val );
	     }
          break;
          }
}

// preset temperature
static void pre_temp_call( GtkWidget *widget, glostru * glo )
{
int ibu;
if ( glo->status.flags & MANU )
   for ( ibu = 0; ibu < 3; ibu++ )
       if ( widget == glo->btpr[ibu] )
          {
	  gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[0], (double)glo->ptube->tem[0].preset[ibu] );
	  gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[1], (double)glo->ptube->tem[1].preset[ibu] );
	  gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[2], (double)glo->ptube->tem[2].preset[ibu] );
          break;
          }
}

static void uniform_call( GtkWidget *widget, glostru * glo )
{
glo->show.uniform_temp = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );
gtk_widget_set_sensitive ( glo->stem[0] , !glo->show.uniform_temp );
gtk_widget_set_sensitive ( glo->stem[2] , !glo->show.uniform_temp );
if ( glo->show.uniform_temp )
   {
   double val;
   val = gtk_spin_button_get_value_as_float((GtkSpinButton *)glo->stem[1] );
   gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[0], val );
   gtk_spin_button_set_value( (GtkSpinButton *)glo->stem[2], val );
   }
}

static void plot_view1_call( GtkWidget *widget, glostru * glo )
{
plot_gui( glo, 0 );
}

static void plot_view2_call( GtkWidget *widget, glostru * glo )
{
plot_gui( glo, 1 );
}

static void arme_secu_call( GtkWidget *widget, glostru * glo )
{
secu_set_param( 0, 1 );
}

/** ============================ constr. GUI ======================= */

/* fonction qui cree un cadre horizontal contenant les
   controles des vannes */
GtkWidget * mk_fvan( glostru *glo )
{
GtkWidget *curwidg;
GtkAdjustment *curadj;
int i; char lbuf[40];

/* creer cadre avec label (devra contenir box si on veut marge */
curwidg = gtk_frame_new ("Vannes");
glo->fvan = curwidg;

/* creer boite verticale */
curwidg = gtk_vbox_new( FALSE, VSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_container_add( GTK_CONTAINER( glo->fvan ), curwidg );
glo->vvaa = curwidg;

/* creer boites horizontales */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_box_pack_start( GTK_BOX(glo->vvaa), curwidg, FALSE, FALSE, 0 );
glo->hvan[1] = curwidg;

curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_box_pack_start( GTK_BOX(glo->vvaa), curwidg, FALSE, FALSE, 0 );
glo->hvan[0] = curwidg;

for ( i = (QVAN-1); i >= 0; i-- )
    {
    /* une petite boite verticale */
    curwidg = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (curwidg), SPACE);
    glo->vvan[i] = curwidg;

    /* une entree non editable */
    curwidg = gtk_entry_new_with_max_length (1);
    gtk_widget_set_usize (curwidg, 36, 0);
    gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
    gtk_entry_set_text( GTK_ENTRY(curwidg), "0" );
    gtk_box_pack_start( GTK_BOX(glo->vvan[i]), curwidg, FALSE, FALSE, 0 );
    glo->evan[i] = curwidg;

    /* un spin button */
    curadj = (GtkAdjustment *)
             gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 1.0, 0.0);
    curwidg = gtk_spin_button_new ( curadj, 0, 0 );
    glo->avan[i] = curadj;
    /* no wrap around limits */
    gtk_spin_button_set_wrap( GTK_SPIN_BUTTON (curwidg), FALSE );
    gtk_widget_set_usize( curwidg, 36, 0 );
    gtk_box_pack_start( GTK_BOX(glo->vvan[i]), curwidg, FALSE, FALSE, 0 );
    glo->svan[i] = curwidg;

    /* un callback */
    gtk_signal_connect( GTK_OBJECT(curadj), "value_changed",
                        GTK_SIGNAL_FUNC(van_call),
                        (gpointer)glo );

    /* un label */
    sprintf( lbuf, "V %d", i );
    curwidg = gtk_label_new ( lbuf );
    gtk_box_pack_start( GTK_BOX(glo->vvan[i]), curwidg, FALSE, TRUE, 0 );
    glo->lvan[i] = curwidg;

    /* encore un label */
    curwidg = gtk_label_new ( glo->ptube->vanne[i].name.c_str() );
    gtk_box_pack_start( GTK_BOX(glo->vvan[i]), curwidg, FALSE, TRUE, 0 );

    gtk_box_pack_start( GTK_BOX(glo->hvan[i>>3]), glo->vvan[i], TRUE, TRUE, HSPACE );
    }
return( glo->fvan );
}

/* fonction qui cree une petite boite verticale contenant les
   radio-buttons pour le choix d'unites et les boutons du plot */
GtkWidget * mk_vmisc( glostru *glo )
{
GtkWidget *curwidg;
curwidg = gtk_vbox_new(FALSE, 0);
gtk_container_set_border_width( GTK_CONTAINER(curwidg), SPACE);
glo->vmisc = curwidg;

curwidg = gtk_hbox_new(FALSE, 0);
gtk_container_set_border_width( GTK_CONTAINER(curwidg), 0);
gtk_box_pack_start( GTK_BOX(glo->vmisc), curwidg, FALSE, FALSE, 0 );
glo->hmisc = curwidg;

curwidg = gtk_vbox_new(FALSE, 0);
gtk_container_set_border_width( GTK_CONTAINER(curwidg), SPACE);
gtk_box_pack_start( GTK_BOX(glo->hmisc), curwidg, FALSE, FALSE, 0 );
glo->vsca = curwidg;

/* trois boutons radio */
curwidg = gtk_radio_button_new_with_label( NULL, "Unités réelles");
gtk_box_pack_start( GTK_BOX(glo->vsca), curwidg, FALSE, FALSE, 0 );
glo->rsca[2] = curwidg;

curwidg = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(glo->rsca[2]),
							"% pleine échelle");
gtk_box_pack_start( GTK_BOX(glo->vsca), curwidg, FALSE, FALSE, 0 );
glo->rsca[1] = curwidg;

curwidg = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(glo->rsca[2]),
							"Volts");
gtk_box_pack_start( GTK_BOX(glo->vsca), curwidg, FALSE, FALSE, 0 );
glo->rsca[0] = curwidg;

/* un bouton simple */
curwidg = gtk_button_new_with_label("log\n-> txt");
gtk_box_pack_end( GTK_BOX(glo->hmisc), curwidg, FALSE, FALSE, 0 );
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( plot_view2_call ), (gpointer)glo );
glo->bpl2 = curwidg;

/* un bouton simple */
curwidg = gtk_button_new_with_label("log\nrésumé");
gtk_box_pack_end( GTK_BOX(glo->hmisc), curwidg, FALSE, FALSE, 0 );
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( plot_view1_call ), (gpointer)glo );
glo->bpl1 = curwidg;


if ( glo->show.auto_secu )
   {
   char lbuf[40];
   // un cadre
   snprintf( lbuf, 40, "Automate sécurité" );
   curwidg = gtk_frame_new( lbuf );
   gtk_box_pack_start( GTK_BOX(glo->vmisc), curwidg, FALSE, FALSE, 0 );
   glo->fsec = curwidg;
   // une boite horizontale
   curwidg = gtk_hbox_new( FALSE, 0 ); /* spacing ENTRE objets */
   gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
   gtk_container_add( GTK_CONTAINER( glo->fsec ), curwidg );
   glo->hsec = curwidg;
   /* une entree non editable */
   curwidg = gtk_entry_new();
   gtk_widget_set_usize (curwidg, 250, 0);
   gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
   gtk_entry_set_text( GTK_ENTRY(curwidg), "0" );
   gtk_box_pack_start( GTK_BOX(glo->hsec), curwidg, FALSE, FALSE, 0 );
   glo->esec = curwidg;
   /* un bouton simple */
   curwidg = gtk_button_new_with_label("armer");
   gtk_box_pack_start( GTK_BOX(glo->hsec), curwidg, FALSE, FALSE, 0 );
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( arme_secu_call ), (gpointer)glo );
   glo->bsec = curwidg;
   }

return( glo->vmisc );
}

/* fonction qui cree un cadre horizontal contenant les
   controles des MFCs */
GtkWidget * mk_fmfc( glostru *glo )
{
GtkWidget *curwidg;
GtkAdjustment *curadj;
int i; char lbuf[40];

/* creer cadre avec label (devra contenir box si on veut marge */
curwidg = gtk_frame_new ("Régulateurs de débit");
glo->fmfc = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_container_add( GTK_CONTAINER( glo->fmfc ), curwidg );
glo->hmfc = curwidg;

for ( i = (QMFC-1); i >= 0; i-- )
    {
    /* une petite boite verticale */
    curwidg = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width( GTK_CONTAINER(curwidg), SPACE);
    glo->vmfc[i] = curwidg;

    /* entree non editable */
    curwidg = gtk_entry_new_with_max_length (20);
    gtk_widget_set_usize (curwidg, 120, 0);
    gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
    gtk_entry_set_text( GTK_ENTRY(curwidg), "PV" );
    gtk_box_pack_start( GTK_BOX(glo->vmfc[i]), curwidg, FALSE, FALSE, 0 );
    glo->empv[i] = curwidg;

    /* entree non editable */
    curwidg = gtk_entry_new_with_max_length (20);
    gtk_widget_set_usize (curwidg, 120, 0);
    gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
    gtk_entry_set_text( GTK_ENTRY(curwidg), "SV" );
    gtk_box_pack_start( GTK_BOX(glo->vmfc[i]), curwidg, FALSE, FALSE, 0 );
    glo->emsv[i] = curwidg;

    /* un spin button - clic gauche = 0.001, clic milieu 0.25 */
    curadj = (GtkAdjustment *)
             gtk_adjustment_new ( 0.0, 0.0, 5.0, 0.05, 0.25, 0.0 );
		//	( value, lower, upper, step, page_inc, page_size )
    /* un peu d'acceleration : non merci */
    curwidg = gtk_spin_button_new( curadj, 0, 0 );
    glo->amfc[i] = curadj;
    /* no wrap around limits */
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
    gtk_widget_set_usize( curwidg, 100, 0 );
    gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
    gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 4 );
    gtk_box_pack_start( GTK_BOX(glo->vmfc[i]), curwidg, FALSE, TRUE, 0 );
    glo->smfc[i] = curwidg;

    /* un callback */
    gtk_signal_connect( GTK_OBJECT(curadj), "value_changed",
                        GTK_SIGNAL_FUNC(mfc_call),
                        (gpointer)glo );

    /* un label */
    sprintf( lbuf, "MFC %d", i );
    curwidg = gtk_label_new ( lbuf );
    gtk_box_pack_start( GTK_BOX(glo->vmfc[i]), curwidg, FALSE, TRUE, 0);
    glo->lmfc[i] = curwidg;

    /* encore un label */
    curwidg = gtk_label_new ( glo->ptube->mfc[i].name.c_str() );
    gtk_box_pack_start( GTK_BOX(glo->vmfc[i]), curwidg, FALSE, TRUE, 0 );

    gtk_box_pack_start( GTK_BOX(glo->hmfc), glo->vmfc[i], TRUE, TRUE, HSPACE);
    }

/* une petite boite verticale pour le choix d'unite, l'automate secu etc... */
curwidg = mk_vmisc( glo );
gtk_box_pack_start( GTK_BOX(glo->hmfc), curwidg, TRUE, FALSE, HSPACE);

return( glo->fmfc );
}

/* fonction qui cree un cadre horizontal contenant les
   controles des regulateurs de temperature */
GtkWidget * mk_ftem( glostru *glo )
{
GtkWidget *curwidg;
GtkAdjustment *curadj;
int i; char lbuf[40];

/* creer cadre avec label (devra contenir box si on veut marge */
curwidg = gtk_frame_new ("Régulateurs de température");
glo->ftem = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_container_add( GTK_CONTAINER( glo->ftem ), curwidg );
glo->htem = curwidg;

for ( i = 0; i < 3; i++ )
    {
    /* une petite boite verticale */
    curwidg = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width( GTK_CONTAINER(curwidg), SPACE);
    glo->vtem[i] = curwidg;

    /* entree non editable */
    curwidg = gtk_entry_new_with_max_length (20);
    gtk_widget_set_usize (curwidg, 120, 0);
    gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
    gtk_entry_set_text( GTK_ENTRY(curwidg), "PV" );
    gtk_box_pack_start( GTK_BOX(glo->vtem[i]), curwidg, FALSE, FALSE, 0 );
    glo->etpv[i] = curwidg;

    /* entree non editable */
    curwidg = gtk_entry_new_with_max_length (20);
    gtk_widget_set_usize (curwidg, 120, 0);
    gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
    gtk_entry_set_text( GTK_ENTRY(curwidg), "SV" );
    gtk_box_pack_start( GTK_BOX(glo->vtem[i]), curwidg, FALSE, FALSE, 0 );
    glo->etsv[i] = curwidg;

    /* un spin button */
    curadj = (GtkAdjustment *)
             gtk_adjustment_new ( 600.0, 15.0, 1150.0, 10.0, 100.0, 0.0 );
		//	( value, lower, upper, step, page_inc, page_size )
    /* un peu d'acceleration : non merci */
    curwidg = gtk_spin_button_new( curadj, 0, 0 );
    glo->atem[i] = curadj;
    /* no wrap around limits */
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
    gtk_widget_set_usize( curwidg, 100, 0 );
    gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
    gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 0 );
    gtk_box_pack_start( GTK_BOX(glo->vtem[i]), curwidg, FALSE, TRUE, 0 );
    glo->stem[i] = curwidg;

    /* un callback */
    gtk_signal_connect( GTK_OBJECT(curadj), "value_changed",
                        GTK_SIGNAL_FUNC(tem_call),
                        (gpointer)glo );

    /* un label */
    curwidg = gtk_label_new ( glo->ptube->tem[i].name.c_str() );
    gtk_box_pack_start( GTK_BOX(glo->vtem[i]), curwidg, FALSE, TRUE, 0);
    glo->ltem[i] = curwidg;

    gtk_box_pack_start( GTK_BOX(glo->htem), glo->vtem[i], TRUE, TRUE, HSPACE);
    }

/* une petite boite verticale */
curwidg = gtk_vbox_new(FALSE, 0);
gtk_container_set_border_width( GTK_CONTAINER(curwidg), SPACE);
gtk_box_pack_start( GTK_BOX(glo->htem), curwidg, TRUE, TRUE, HSPACE);
glo->vtpr = curwidg;

/* simples boutons */
sprintf( lbuf, " %d/%d/%d °C ",
         glo->ptube->tem[0].preset[2], glo->ptube->tem[1].preset[2], glo->ptube->tem[2].preset[2]);
curwidg = gtk_button_new_with_label (lbuf);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pre_temp_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->vtpr ), curwidg, FALSE, FALSE, 0 );
glo->btpr[2] = curwidg;

sprintf( lbuf, " %d/%d/%d °C ",
         glo->ptube->tem[0].preset[1], glo->ptube->tem[1].preset[1], glo->ptube->tem[2].preset[1]);
curwidg = gtk_button_new_with_label (lbuf);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pre_temp_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->vtpr ), curwidg, FALSE, FALSE, 0 );
glo->btpr[1] = curwidg;

sprintf( lbuf, " %d/%d/%d °C ",
         glo->ptube->tem[0].preset[0], glo->ptube->tem[1].preset[0], glo->ptube->tem[2].preset[0]);
curwidg = gtk_button_new_with_label (lbuf);
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pre_temp_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->vtpr ), curwidg, FALSE, FALSE, 0 );
glo->btpr[0] = curwidg;

/* toggle bouton */
curwidg = gtk_check_button_new_with_label ("Temp. uniforme");
gtk_signal_connect (GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( uniform_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->vtpr ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->show.uniform_temp = 0;
glo->buni = curwidg;

return( glo->ftem );
}


/* fonction qui cree un cadre horizontal contenant les
   affichages des auxiliaires */
GtkWidget * mk_faux( glostru *glo )
{
GtkWidget *curwidg;
int i;
// textes pour labels
const char * auxname[3];

/* preparer les labels */
auxname[0] = "Hz";
auxname[1] = glo->ptube->fre.unit.c_str();
if   ( glo->ptube->fre.unit == string("mTorr") )
     auxname[2] = "mbar";
else auxname[2] = "";

/* creer cadre avec label (devra contenir box si on veut marge */
curwidg = gtk_frame_new ( glo->ptube->fre.name.c_str() );
glo->faux = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, HSPACE ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), SPACE);
gtk_container_add( GTK_CONTAINER( glo->faux ), curwidg );
glo->haux = curwidg;

for ( i = 0; i < 3; i++ )
    {
    /* une petite boite verticale */
    curwidg = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width( GTK_CONTAINER(curwidg), SPACE);
    glo->vaux[i] = curwidg;

    /* entree non editable */
    curwidg = gtk_entry_new_with_max_length (20);
    gtk_widget_set_usize (curwidg, 100, 0);
    gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
    gtk_entry_set_text( GTK_ENTRY(curwidg), "0.000" );
    gtk_box_pack_start( GTK_BOX(glo->vaux[i]), curwidg, FALSE, FALSE, 0 );
    glo->eaux[i] = curwidg;

    /* encore un label */
    curwidg = gtk_label_new ( auxname[i] );
    gtk_box_pack_start( GTK_BOX(glo->vaux[i]), curwidg, FALSE, TRUE, 0 );

    gtk_box_pack_start( GTK_BOX(glo->haux), glo->vaux[i], TRUE, TRUE, HSPACE);
    }
return( glo->faux );
}

