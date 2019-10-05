/* visualisation recette en detail */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

#include "modpop2.h"
#include "../../version.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"
#include "../dirlist.h"
#include "rview.h"

/* ---------------- SIMPLE CALL BACKS -------------------- */

/* il faut intercepter le delete event pour que si l'utilisateur
   ferme la fenetre on revienne sans engendrer de destroy signal.
   A cet effet ce callback doit rendre TRUE ( gtk_main_quit() ne le fait pas)
 */
static gint delete_call( GtkWidget *widget,
                  GdkEvent  *event, rviewstru * rtree )
{
rtree->selected = -1;
gtk_main_quit();
return (TRUE);
}


// sauter sur le step de l'iter propose si possible et si confirme
// sauter c'est seulement renseigner rtree->selected et faire gtk_main_quit();
static void jmp_step( GtkTreeIter * piter, rviewstru * rtree )
{
int istep; etape * pstep; ostringstream obuf;
gtk_tree_model_get( (GtkTreeModel *)(rtree->tmod), piter, 1, &istep, -1 );
// printf("jmp %d\n", istep );
pstep = &rtree->prec->step[istep];
if	( pstep->existe )
	{
	obuf << "sauter sur step " << istep << "<b> " << pstep->titre << "</b> ?";
	if ( modpopYN( "CONFIRMATION", obuf.str().c_str(),
		       " OUI ", " NON ", GTK_WINDOW(rtree->wmain) )
	   ) { rtree->selected = istep; gtk_main_quit(); }
	}
}

// bouton jmp
static void jmp_but_call( GtkWidget *widget, rviewstru * rtree )
{
GtkTreeIter iter;
GtkTreeSelection * cursel = gtk_tree_view_get_selection( (GtkTreeView*)rtree->tste );
if   ( gtk_tree_selection_get_selected( cursel, NULL, &iter ) )
     jmp_step( &iter, rtree );
}

// editer un step :
static void edit_step( GtkTreeIter * piter, rviewstru * rtree );

static void edi_but_call( GtkWidget *widget, rviewstru * rtree )
{
GtkTreeIter iter;
GtkTreeSelection * cursel = gtk_tree_view_get_selection( (GtkTreeView*)rtree->tste );
if   ( gtk_tree_selection_get_selected( cursel, NULL, &iter ) )
     edit_step( &iter, rtree );
}

// sauver la recette
static void save_but_call( GtkWidget *widget, rviewstru * rtree )
{
rtree->prec->check();
if	( rtree->prec->stat > -2 )
	rtree->prec->make_pack();

if	( rtree->prec->stat < -1 )
	{ printf( "%s\n", rtree->prec->errmess.c_str() ); }
else	{
	string fullpath = rtree->ptube->xml_dir + char(SLASH) + string("saved_") + rtree->prec->filename;
	FILE * xfil = fopen( fullpath.c_str(), "w" );
	if	( xfil == NULL ) xfil = stdout;
	else	printf("saving %s\n", fullpath.c_str() );
	rtree->prec->make_xml( xfil );
	fclose( xfil );
		{
		fullpath = rtree->ptube->xml_dir + char(SLASH) + string("dumped_") + rtree->prec->filename + string(".txt");
		FILE * dfil = fopen( fullpath.c_str(), "w" );
		if	( dfil == NULL ) dfil = stdout;
		else	printf("saving %s\n", fullpath.c_str() );
		rtree->prec->dump_pack();
		fputs( rtree->prec->dump.c_str(), dfil );
		fclose( dfil );
		}
	fflush(stdout);
	}
}

// double clic
static void double_call( GtkTreeView *curwidg,
			GtkTreePath *path, GtkTreeViewColumn *col, rviewstru * rtree )
{
GtkTreeIter iter;
if   ( gtk_tree_model_get_iter( (GtkTreeModel *)(rtree->tmod), &iter, path ) )
     {
     if ( rtree->flags & JMP )
        jmp_step( &iter, rtree );
     if ( rtree->flags & EDIT )
        edit_step( &iter, rtree );
     }
}


static void qui_but_call( GtkWidget *widget, rviewstru * rtree )
{
rtree->selected = -1; gtk_main_quit();
}

/** ======================== tree call backs ======================= */

// une callback type data_func
void step_data_call( GtkTreeViewColumn * tree_column,	// sert pas !
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     void              * glo )
{
lintype typ; int istep, ipod, ityp;
gchar *text; const char * cellback;

// recuperer les donnees dans les colonnes 0, 1 et 2 du modele
// (pairs of column number and value return locations, terminated by -1)
gtk_tree_model_get( tree_model, iter, 0, &ityp, 1, &istep, 2, &ipod, -1 );
typ = (lintype)ityp;
switch (typ)
   {
   case STEP :	if ( istep >= 100 ) cellback="#FFC080"; else cellback="#C0FFC0";
				    text = g_strdup_printf( "STEP %3d", istep ); break;
   case SPAR :	cellback="#FFFFFF"; text = g_strdup_printf( " " ); break;
   case VANN :	cellback="#FFFFFF"; text = g_strdup_printf( "Vannes" ); break;
   case MFC :	cellback="#FFFFFF"; text = g_strdup_printf( "MFC %d", ipod ); break;
   case TEM :	cellback="#FFFFFF"; text = g_strdup_printf( "Reg. T. %d", ipod ); break;
   case AUX :	cellback="#FFFFFF"; text = g_strdup_printf( "Aux" ); break;
   default :	cellback="#FFFFFF"; text = g_strdup_printf( "-" ); break;
   }

// donner cela comme attribut au renderer
g_object_set( rendy, "markup", text, NULL );
g_object_set( rendy, "cell-background", cellback,
                     "cell-background-set", TRUE, NULL );
g_free( text );
}

// une callback type data_func
void data_data_call( GtkTreeViewColumn * tree_column,	// sert pas !
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     void              * vglo )
{
lintype typ; int istep, ipod, ival, ityp, pflags; epod * pepod; modget * fomod;
char scale; ostringstream obuf;
rviewstru * glo = (rviewstru *)vglo;
const char * cellback="#C0C0C0";
const char * spBl ="<span background=\"#FFFFFF\" foreground=\"#000000\" >";
const char * spBlG="<span background=\"#FFFFFF\" foreground=\"#00AA33\" weight=\"bold\" >";	// vert
const char * spBlR="<span background=\"#FFFFFF\" foreground=\"#BB0000\" weight=\"bold\" >";	// rouge
const char * spBlB="<span background=\"#FFFFFF\" foreground=\"#0000BB\" weight=\"bold\" >";	// bleu
const char * spBlM="<span background=\"#FFFFFF\" foreground=\"#AA00AA\" weight=\"bold\" >";	// magenta
const char * spBlO="<span background=\"#FFFFFF\" foreground=\"#AA7700\" weight=\"bold\" >";	// orange

const char * sp0="</span>";

// recuperer les donnees dans les colonnes 0, 1 et 2 du modele
gtk_tree_model_get( tree_model, iter, 0, &ityp, 1, &istep, 2, &ipod, -1 );
typ = (lintype)ityp;
switch (typ)
   {
   case STEP :	if ( istep >= 100 ) cellback="#FFC080"; else cellback="#C0FFC0";
		obuf << "<b>" << glo->prec->step[istep].titre << "</b>";
		break;
   case SPAR :  obuf << "<tt>"; ival = glo->prec->step[istep].duree;
		if   ( ival <= 0 )
		     obuf << "step en pause";
		else obuf << spBlM << "durée = " << left << setw(5) << ival << sp0;
		obuf << "  " << spBlB << "prochain step = "; ival = glo->prec->step[istep].stogo;
		if ( ival < 0 ) ival = istep + 1;
		obuf << left << setw(3) << ival << sp0;
		ival = glo->prec->step[istep].deldg;
		if ( ival > 0 ) obuf << "  " << spBl << "délai de grâce = "
				     << left << setw(2) << ival << sp0;
		ival = glo->prec->step[istep].secstat;
		if ( ival >= 0 ) obuf << "  " << spBlO << "arm. automate secu = "
				     << left << setw(1) << ival << sp0;
		obuf << "</tt>"; break;
   case VANN :	obuf << "<tt>";
		ival = glo->prec->step[istep].vannes;
		if ( ival > 0 )
		   {
		   for  ( ipod = 0; ipod < QVAN; ipod++ )
			{
			if  ( ival & 1 )
			    obuf << spBl << glo->ptube->vanne[ipod].name << sp0 << " ";
			ival >>= 1;
			}
		   }
		obuf << "</tt>"; break;
   case MFC :
   case TEM :
   case AUX :	switch (typ)
		   {
		   case MFC : pepod = &glo->prec->step[istep].mfc[ipod];
			      fomod = &glo->ptube->mfc[ipod]; scale = glo->scale_type; break;
		   case TEM : pepod = &glo->prec->step[istep].tem[ipod];
			      fomod = &glo->ptube->tem[ipod]; scale = 'd'; break;
		   default  : pepod = &glo->prec->step[istep].fre;
			      fomod = &glo->ptube->fre; scale = 'f'; break;
		   }
		obuf << "<tt>" << left << setw(12) << fomod->name;
		pflags = pepod->flags;
		if ( pflags < 0 ) pflags = 0;
		if ( pepod->SV >= 0 )
		   {
		   obuf << " " << spBl << "SV = ";
		   fomod->pcu2stream( obuf, pepod->SV, scale ); obuf << sp0;
		   }
		if ( ( ( pflags & RAMPEN ) == 0 ) && ( pflags & (MICEN|MACEN) ) )
		   {
		   if	( ( pflags & (MICEN|MACEN) ) == (MICEN|MACEN) )
			obuf << " Seuils";
		   else obuf << " Seuil ";
		   if   ( pflags & MICEN )
			{
			if   ( pepod->SVmi > 0 ) ival = pepod->SVmi;
			else if   ( pepod->SV >= 0 )	// min par defaut de l'automate
				  ival = pepod->SV - (pepod->SV >> TOLSHIFT);
			     else ival = -1;
			if ( ival > 0 )
			   {
			   obuf << " " << spBlG << "Min = ";
			   fomod->pcu2stream( obuf, ival, scale ); obuf << sp0;
			   }
			}
		   if   ( pflags & MACEN )
			{
			if   ( pepod->SVma > 0 ) ival = pepod->SVma;
			else if   ( pepod->SV >= 0 )	// max par defaut de l'automate
				  {
				  ival = pepod->SV + (pepod->SV >> TOLSHIFT);
				  // simulation saturation cf recipe.c de PROC 8.1g
				  if ( ival > 65535 ) ival = 65535;
				  }
			     else ival = -1;
			if ( ival > 0 )
			   {
			   obuf << " " << spBlR << "Max = ";
			   fomod->pcu2stream( obuf, ival, scale ); obuf << sp0;
			   }
			}
		   if	( pepod->stogo >= 0 )
			obuf << " " << spBlB << "Saut vers " << pepod->stogo << sp0;;
		   }
		if ( pflags & RAMPEN )
		   {
		   if	( pflags & MACEN )
			{
			obuf << " Rampe montante " << spBl << "vers "; ival = pepod->SVma;
			if ( ival > 0 )
			   fomod->pcu2stream( obuf, ival, scale );
			obuf << sp0;
			obuf << " " << spBlO << "à "; ival = pepod->SVmi;
			if ( ival > 0 )
			   fomod->pcu2stream( obuf, ival, scale );
			obuf << " par s. "; obuf << sp0;
			}
		   if	( pflags & MICEN )
			{
			obuf << " Rampe descente " << spBl << "vers "; ival = pepod->SVmi;
			if ( ival > 0 )
			   fomod->pcu2stream( obuf, ival, scale );
			obuf << sp0;
			obuf << " " << spBlO << "à "; ival = pepod->SVma;
			if ( ival > 0 )
			   fomod->pcu2stream( obuf, ival, scale );
			obuf << " par s. "; obuf << sp0;
			}
		   if	( pepod->stogo >= 0 )
			obuf << " " << spBlB << "Saut vers " << pepod->stogo << sp0;
		   }
		obuf << "</tt>"; break;
   default :	obuf << "#-#";
   }
// donner cela comme attribut au renderer
g_object_set( rendy, "markup", obuf.str().c_str(), NULL );
g_object_set( rendy, "cell-background", cellback,
                     "cell-background-set", TRUE, NULL );
}


/** ========================= constr. et manip database ===================== */

GtkTreeStore * makestore( rviewstru * glo )
{
GtkTreeStore *mymod;
GtkTreeIter   parent_iter, child_iter;
GtkTreeIter  *curiter;
int istep, ipod;
etape * pstep; epod * pepod;

// 3 colonnes de type int
// type d'element (enum lintype) | numero step | numero podget
mymod = gtk_tree_store_new( 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT );

// boucle des steps
for ( istep = 1; istep < 256; istep++ )
    {
    pstep = &glo->prec->step[istep];
    if	( pstep->existe )
	{
	// un top-level parent
	curiter = &parent_iter;
	gtk_tree_store_append( mymod, curiter, NULL );
	gtk_tree_store_set( mymod, curiter, 0, STEP, 1, istep, -1 );
	// les parametres du step (un child)
	curiter = &child_iter;
	gtk_tree_store_append( mymod, curiter, &parent_iter );
	gtk_tree_store_set( mymod, curiter, 0, SPAR, 1, istep, -1 );
	// les vannes
	curiter = &child_iter;
	gtk_tree_store_append( mymod, curiter, &parent_iter );
	gtk_tree_store_set( mymod, curiter, 0, VANN, 1, istep, -1 );
	// les MFCs
	for ( ipod = 0; ipod < QMFC; ipod++ )
	    {
	    pepod = &pstep->mfc[ipod];
	    if  ( ( pepod->SV >=0 ) || ( pepod->SVmi >=0 ) || ( pepod->SVma >=0 ) )
		{
		// un child
		curiter = &child_iter;
		gtk_tree_store_append( mymod, curiter, &parent_iter );
		gtk_tree_store_set( mymod, curiter, 0, MFC, 1, istep, 2, ipod, -1 );
		}
	    }
	// les regulateurs de temperature
	for ( ipod = 0; ipod < QTEM; ipod++ )
	    {
	    pepod = &pstep->tem[ipod];
	    if  ( ( pepod->SV >=0 ) || ( pepod->SVmi >=0 ) || ( pepod->SVma >=0 ) )
		{
		// un child
		curiter = &child_iter;
		gtk_tree_store_append( mymod, curiter, &parent_iter );
		gtk_tree_store_set( mymod, curiter, 0, TEM, 1, istep, 2, ipod, -1 );
		}
	    }
	// le canal auxiliaire
	pepod = &pstep->fre;
	if  ( ( pepod->SV >=0 ) || ( pepod->SVmi >=0 ) || ( pepod->SVma >=0 ) )
	    {
	    // un child
	    curiter = &child_iter;
	    gtk_tree_store_append( mymod, curiter, &parent_iter );
	    gtk_tree_store_set( mymod, curiter, 0, AUX, 1, istep, -1 );
	    }
	}
    }

return(mymod);
}

// cherche un step dans le tree store, renseigne un iter
// recherche lineaire, un gros foreach...
int findstep( rviewstru * rtree, GtkTreeIter * founditer, int step )
{
gboolean retval;
lintype typ; int istep, ityp;

// Get the first iter in the list
retval = gtk_tree_model_get_iter_first( GTK_TREE_MODEL( rtree->tmod ), founditer );

while ( retval )
    {
    // recuperer les donnees dans les colonnes 0 et 1 du modele
    // (pairs of column number and value return locations, terminated by -1)
    gtk_tree_model_get( GTK_TREE_MODEL( rtree->tmod ), founditer,
                        0, &ityp, 1, &istep, -1 );

    typ = (lintype)ityp;
    // Do something with the data
    // printf("~~> %d %d\n", typ, istep );
    if ( ( typ == STEP ) && ( istep == step ) )
       return TRUE;

    retval = gtk_tree_model_iter_next( GTK_TREE_MODEL( rtree->tmod ), founditer );
    }
return retval;
}


/** =========================== constr. TREE VIEW ==================== */

GtkWidget * maketview( rviewstru * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection *cursel;

curwidg = gtk_tree_view_new();

// une colonne, avec data_func, qui hebergera ausi les triangles
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, " Step " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         step_data_call,
                                         (gpointer)glo, NULL );

gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// une colonne, avec data_func
// (c'est la data_func qui choisira la colonne du model)
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, " Data " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         data_data_call,
                                         (gpointer)glo, NULL );

gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );


// configurer la selection
cursel = gtk_tree_view_get_selection( (GtkTreeView*)curwidg );
gtk_tree_selection_set_mode( cursel, GTK_SELECTION_SINGLE );

// connecter callback pour double-clic
g_signal_connect( curwidg, "row-activated", (GCallback)double_call, (gpointer)glo );

// connecter callback "realize" pour initialisation
// g_signal_connect( curwidg, "realize",
//		  G_CALLBACK( tree_realized_call ), (gpointer)glo );

return(curwidg);
}


/* --------------- main function of this module ------------------ */

// cette fonction ouvre la liste des steps
// - si selstep > 0, selectionne, expande et scrolle le step
// - si flags == jump, rend le step destination choisi par l'utilisateur, ou -1


int view_recette( recipe * prec, char scale_type, int selstep, int flags )
{
rviewstru thertree;
rviewstru * rtree = &thertree;
GtkWidget *curwidg;

rtree->ptube = prec->ptube;
rtree->prec = prec;
rtree->scale_type = scale_type;
rtree->selected = -1;
rtree->flags = flags;
rtree->modcnt = 0;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );
gtk_window_set_modal( GTK_WINDOW(curwidg), TRUE );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), (gpointer)rtree );

char * lbuf;
string fullpath = rtree->ptube->xml_dir + char(SLASH) + prec->filename;
lbuf = g_strdup_printf( "%s [%s]", rtree->ptube->nom.c_str(), fullpath.c_str() );
gtk_window_set_title( GTK_WINDOW (curwidg), lbuf );
g_free( lbuf );
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
rtree->wmain = curwidg;

// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( rtree->wmain ), curwidg );
rtree->vmain = curwidg;

// label
curwidg = gtk_label_new( NULL );
lbuf = g_strdup_printf( "RECETTE : <b>%s</b>", prec->titre.c_str() );
gtk_label_set_markup( GTK_LABEL( curwidg ), lbuf );
g_free( lbuf );
gtk_box_pack_start( GTK_BOX( rtree->vmain ), curwidg, FALSE, TRUE, 0 );
rtree->lmain = curwidg;

// scrolled window avec tree
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(curwidg),
				     GTK_SHADOW_IN);
gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(curwidg),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
gtk_widget_set_usize( curwidg, 900, 710 );
gtk_box_pack_start( GTK_BOX( rtree->vmain ), curwidg, TRUE, TRUE, 0 );
rtree->wlis = curwidg;

rtree->tste = maketview( rtree );
gtk_container_add( GTK_CONTAINER( rtree->wlis ), rtree->tste );

// creer le modele
rtree->tmod = makestore( rtree );

// connecter le modele
gtk_tree_view_set_model( (GtkTreeView*)rtree->tste, GTK_TREE_MODEL( rtree->tmod ) );

// boite a boutons
curwidg = gtk_hbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_box_pack_end ( GTK_BOX( rtree->vmain ), curwidg, FALSE, FALSE, 0);
rtree->hbut = curwidg;

// simple bouton : close
curwidg = gtk_button_new_with_label (" Fermer ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( qui_but_call ), (gpointer)rtree );
gtk_box_pack_end( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
rtree->bqui = curwidg;

if ( flags & EDIT )
   {
   // simple bouton : save
   curwidg = gtk_button_new_with_label (" Sauver recette ");
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( save_but_call ), (gpointer)rtree );
   gtk_box_pack_end( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
   rtree->bsav = curwidg;
   // simple bouton : edit
   curwidg = gtk_button_new_with_label (" Modifier step ");
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( edi_but_call ), (gpointer)rtree );
   gtk_box_pack_end( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
   rtree->bedi = curwidg;
   }

if ( flags & JMP )
   {
   // simple bouton : jump
   curwidg = gtk_button_new_with_label (" Sauter ");
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( jmp_but_call ), (gpointer)rtree );
   gtk_box_pack_start( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
   rtree->bjmp = curwidg;
   }


rtree->wstep = NULL;	// creation differee

gtk_widget_show_all ( rtree->wmain );

GtkTreeSelection * cursel = gtk_tree_view_get_selection( (GtkTreeView*)rtree->tste );

// preselection
if   ( selstep > 0 )
     {
     GtkTreeIter curiter;
     if ( findstep( rtree, &curiter, selstep ) )
	{
	GtkTreePath *path;
	gtk_tree_selection_select_iter( cursel, &curiter );
	path = gtk_tree_model_get_path( (GtkTreeModel*)rtree->tmod, &curiter );
	gtk_tree_view_expand_to_path( (GtkTreeView*)rtree->tste, path );
	gtk_tree_view_scroll_to_cell((GtkTreeView*)rtree->tste, path, NULL,
				      TRUE, 0.3, 0.3 );
	gtk_tree_path_free( path );
	}
     }
else gtk_tree_selection_unselect_all( cursel );


gtk_main();
gtk_widget_destroy( rtree->wmain );
return(rtree->selected);
}

// ============================= editeur de step ==================================== //

static void step2gui( int istep, rviewstru * rtree );
static void gui2step( int istep, rviewstru * rtree );

// callbacks

// aller editer le step precedent dans la recette
static void rew_but_call( GtkWidget *widget, rviewstru * rtree )
{
int istep = rtree->selected;
// sauver modifs eventuelles
gui2step( rtree->selected, rtree );
// trouver nouveau step
while ( --istep > 0 )
   {
   if 	( rtree->prec->step[istep].existe )
	{
	rtree->selected = istep;
	step2gui( rtree->selected, rtree );
	return;
	}
   }
}

// aller editer le step suivant dans la recette
static void ffw_but_call( GtkWidget *widget, rviewstru * rtree )
{
int istep = rtree->selected;
// sauver modifs eventuelles
gui2step( rtree->selected, rtree );
// trouver nouveau step
while ( ++istep <= 255 )
   {
   if 	( rtree->prec->step[istep].existe )
	{
	rtree->selected = istep;
	step2gui( rtree->selected, rtree );
	return;
	}
   }
}

// retourner a la liste de steps
static void ret_but_call( GtkWidget *widget, rviewstru * rtree )
{
// sauver modifs eventuelles
gui2step( rtree->selected, rtree );
// commuter l'affichage
gtk_widget_hide( rtree->wstep );
gtk_widget_show( rtree->wlis );
gtk_widget_hide( rtree->brew );
gtk_widget_hide( rtree->bffw );
gtk_widget_hide( rtree->bret );
gtk_widget_show( rtree->bedi );
// expander le step qui vient d'etre edite
GtkTreeSelection * cursel = gtk_tree_view_get_selection( (GtkTreeView*)rtree->tste );
GtkTreeIter curiter;
if   ( gtk_tree_selection_get_selected( cursel, NULL, &curiter ) )
     {
     GtkTreePath * path = gtk_tree_model_get_path( (GtkTreeModel*)rtree->tmod, &curiter );
     gtk_tree_view_expand_to_path( (GtkTreeView*)rtree->tste, path );
     gtk_tree_view_scroll_to_cell( (GtkTreeView*)rtree->tste, path, NULL,
				   TRUE, 0.3, 0.3 );
     gtk_tree_path_free( path );
     }
}

// bouton "pause forcee"
static void bpau_check_call( GtkWidget *widget, rviewstru * rtree )
{
if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) )
     {
     gtk_widget_set_sensitive( rtree->smin, FALSE );
     gtk_widget_set_sensitive( rtree->ssec, FALSE );
     gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->smin), 0 );
     gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->ssec), 0 );
     }
else {
     gtk_widget_set_sensitive( rtree->smin, TRUE );
     gtk_widget_set_sensitive( rtree->ssec, TRUE );
     }
}

static void ssui_spin_call( GtkWidget *widget, rviewstru * rtree )
{
int ival;
ival = (int) gtk_spin_button_get_value( GTK_SPIN_BUTTON(rtree->ssui) );
if   ( rtree->prec->step[ival].existe )
     gtk_entry_set_text( GTK_ENTRY( rtree->esui ), rtree->prec->step[ival].titre.c_str() );
else if   ( ival == 0 )
          gtk_entry_set_text( GTK_ENTRY( rtree->esui ), "-- repos --" );
     else gtk_entry_set_text( GTK_ENTRY( rtree->esui ), "? à définir ?" );
}

// bouton "nouvelle consigne"
static void bcon_check_call( GtkWidget *widget, modgui * curmod )
{
if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) )
     gtk_widget_show( curmod->scon );
else gtk_widget_hide( curmod->scon );
}

static void modget_flags_call( GtkWidget *widget, modgui * curmod )
{
int i = gtk_combo_box_get_active( GTK_COMBO_BOX(widget) );
switch( i )
   {
   case 1 :	//   case MICEN :		flagi = 1;
	gtk_widget_show( curmod->lmin );
	gtk_widget_show( curmod->smin );
	gtk_widget_hide( curmod->lmax );
	gtk_widget_hide( curmod->smax );
	gtk_widget_hide( curmod->linc );
	gtk_widget_hide( curmod->sinc );
	gtk_widget_show( curmod->ljmp );
	gtk_widget_show( curmod->sjmp );
	gtk_widget_show( curmod->ejmp );
	break;
   case 2 :	//   case MACEN :		flagi = 2;
	gtk_widget_hide( curmod->lmin );
	gtk_widget_hide( curmod->smin );
	gtk_widget_show( curmod->lmax );
	gtk_widget_show( curmod->smax );
	gtk_widget_hide( curmod->linc );
	gtk_widget_hide( curmod->sinc );
	gtk_widget_show( curmod->ljmp );
	gtk_widget_show( curmod->sjmp );
	gtk_widget_show( curmod->ejmp );
	break;
   case 3 :	//   case (MICEN|MACEN) :	flagi = 3;
	gtk_widget_show( curmod->lmin );
	gtk_widget_show( curmod->smin );
	gtk_widget_show( curmod->lmax );
	gtk_widget_show( curmod->smax );
	gtk_widget_hide( curmod->linc );
	gtk_widget_hide( curmod->sinc );
	gtk_widget_show( curmod->ljmp );
	gtk_widget_show( curmod->sjmp );
	gtk_widget_show( curmod->ejmp );
	break;
   case 4 :	//   case (RAMPEN|MACEN) :	flagi = 4;
	gtk_widget_hide( curmod->lmin );
	gtk_widget_hide( curmod->smin );
	gtk_widget_show( curmod->lmax );
	gtk_widget_show( curmod->smax );
	gtk_widget_show( curmod->linc );
	gtk_widget_show( curmod->sinc );
	gtk_widget_show( curmod->ljmp );
	gtk_widget_show( curmod->sjmp );
	gtk_widget_show( curmod->ejmp );
	break;
   case 5 :	//   case (RAMPEN|MICEN) :	flagi = 5;
	gtk_widget_show( curmod->lmin );
	gtk_widget_show( curmod->smin );
	gtk_widget_hide( curmod->lmax );
	gtk_widget_hide( curmod->smax );
	gtk_widget_show( curmod->linc );
	gtk_widget_show( curmod->sinc );
	gtk_widget_show( curmod->ljmp );
	gtk_widget_show( curmod->sjmp );
	gtk_widget_show( curmod->ejmp );
	break;
   default :	//   default :			flagi = 0;
	gtk_widget_hide( curmod->lmin );
	gtk_widget_hide( curmod->smin );
	gtk_widget_hide( curmod->lmax );
	gtk_widget_hide( curmod->smax );
	gtk_widget_hide( curmod->linc );
	gtk_widget_hide( curmod->sinc );
	gtk_widget_hide( curmod->ljmp );
	gtk_widget_hide( curmod->sjmp );
	gtk_widget_hide( curmod->ejmp );
	break;
   }
}

// creation GUI pour un modget (MFC, TEM ou AUX)
// rend le widget 'xmod' du modget (qui fut un expander autrefois, maintenant
// un cadre), ou eventuellement NULL
static GtkWidget * mkmodgui( lintype typ, int i, rviewstru * rtree )
{
GtkWidget *curwidg;
GtkWidget *curlabel;
GtkAdjustment *curadj;
char * lbuf; modget * master; const char * typnam; modgui * curmod;
double fs;
switch( typ )
  {
  case MFC :	master = &(rtree->ptube->mfc[i]);
		typnam = "Débit gaz"; curmod = &(rtree->ymfc[i]); break;
  case TEM :	master = &(rtree->ptube->tem[i]);
		typnam = "Température"; curmod = &(rtree->ytem[i]);  break;
  default  :	master = &(rtree->ptube->fre);
		typnam = "Aux"; curmod = &(rtree->yaux); break;
  }

if   ( master->name == string("--") )
     {
     curmod->xmod = NULL;	// expandeur null <==> pas de modget
     return curmod->xmod;
     }

// le cadre du modget
curlabel = gtk_label_new( NULL );
lbuf = g_strdup_printf( "%s <b>%s</b>", typnam, master->name.c_str() );
gtk_label_set_markup( GTK_LABEL(curlabel), lbuf );
g_free( lbuf );
curwidg = gtk_frame_new( NULL );
gtk_frame_set_label_widget( GTK_FRAME(curwidg), curlabel );
curmod->xmod = curwidg;

// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 0 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( curmod->xmod ), curwidg );
curmod->vmod = curwidg;

// creer boite horizontale
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_box_pack_start( GTK_BOX( curmod->vmod ), curwidg, FALSE, TRUE, 10 );
curmod->hcon = curwidg;

// check bouton
curwidg = gtk_check_button_new_with_label("Nouvelle consigne");
gtk_box_pack_start( GTK_BOX( curmod->hcon ), curwidg, FALSE, FALSE, 10 );
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( bcon_check_call ), (gpointer)curmod );
curmod->bcon = curwidg;

// preparer fs et label pour spinbouton consigne
if   ( typ == TEM )
     {
     fs = 1200.0;
     lbuf = g_strdup_printf( "°C" );
     }
else {
     switch( rtree->scale_type )
	{
	case 'p' : fs = 100.0; lbuf = g_strdup_printf( "%%" );	break;
	case 'u' : fs = master->fs;
		   lbuf = g_strdup_printf( "%s (pleine échelle %g %s)",
		   master->unit.c_str(), fs, master->unit.c_str() );
		   break;
	case 'v' :
	default  : fs = 5.0; lbuf = g_strdup_printf( "V" );
	}
     }

// spin bouton
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0.0, 0.0, fs, fs/50.0, fs/5.0, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );

gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 100, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 4 );
gtk_box_pack_start( GTK_BOX( curmod->hcon ), curwidg, FALSE, FALSE, 0 );
curmod->scon = curwidg;

// label
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg), lbuf );
g_free( lbuf );
gtk_box_pack_start( GTK_BOX( curmod->hcon ), curwidg, FALSE, TRUE, 0 );

// creer boite horizontale
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_box_pack_start( GTK_BOX( curmod->vmod ), curwidg, FALSE, TRUE, 10 );
curmod->hfla = curwidg;

// combo box
curwidg = gtk_combo_box_new_text();
gtk_combo_box_append_text( GTK_COMBO_BOX(curwidg), "Stable" );
gtk_combo_box_append_text( GTK_COMBO_BOX(curwidg), "Détection seuil bas" );
gtk_combo_box_append_text( GTK_COMBO_BOX(curwidg), "Détection seuil haut" );
gtk_combo_box_append_text( GTK_COMBO_BOX(curwidg), "Détection min et max" );
gtk_combo_box_append_text( GTK_COMBO_BOX(curwidg), "Rampe montante" );
gtk_combo_box_append_text( GTK_COMBO_BOX(curwidg), "Rampe descendante" );
gtk_signal_connect( GTK_OBJECT(curwidg), "changed",
                    GTK_SIGNAL_FUNC( modget_flags_call ), (gpointer)curmod );
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, FALSE, 10 );
curmod->cfla = curwidg;

// label de spinbox
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#00AA33\" weight=\"bold\" >Min</span>");
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, TRUE, 0 );
curmod->lmin = curwidg;

// spin bouton
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0.0, 0.0, fs, fs/50.0, fs/5.0, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );

gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 100, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 4 );
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, FALSE, 0 );
curmod->smin = curwidg;

// label de spinbox
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#BB0000\" weight=\"bold\" >Max</span>");
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, TRUE, 0 );
curmod->lmax = curwidg;

// spin bouton
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0.0, 0.0, fs, fs/50.0, fs/5.0, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );

gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 100, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 4 );
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, FALSE, 0 );
curmod->smax = curwidg;

// label de spinbox
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#AA7700\" weight=\"bold\" >var. par seconde</span>");
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, TRUE, 0 );
curmod->linc = curwidg;

// spin bouton
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0.0, 0.0, fs, fs/50.0, fs/5.0, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );

gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 100, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 4 );
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, FALSE, 0 );
curmod->sinc = curwidg;

// label de spinbox
curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#0000BB\" weight=\"bold\" >Saut vers step</span>");
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, TRUE, 0 );
curmod->ljmp = curwidg;

// spin bouton
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0, 0, 255, 1, 1, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );

gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 45, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 0 );
gtk_box_pack_start( GTK_BOX( curmod->hfla ), curwidg, FALSE, FALSE, 0 );
curmod->sjmp = curwidg;

// entree non editable pour l'intitule
curwidg = gtk_entry_new();
gtk_widget_set_usize (curwidg, 100, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_box_pack_end( GTK_BOX( curmod->hfla ), curwidg, TRUE, TRUE, 0 );
curmod->ejmp = curwidg;

return curmod->xmod;
}

// creation GUI step (1 seule fois SVP)
void mkstepgui( rviewstru * rtree )
{
GtkWidget *curwidg;
GtkAdjustment *curadj;
int i; char * lbuf; string * curnam;

// scrolled window
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(curwidg),
				     GTK_SHADOW_IN);
gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(curwidg),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
gtk_widget_set_usize( curwidg, 1100, 600 );
gtk_box_pack_start( GTK_BOX( rtree->vmain ), curwidg, TRUE, TRUE, 0 );
rtree->wstep = curwidg;

// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(rtree->wstep), curwidg );
rtree->vstep = curwidg;

// boite horizontale pour le titre du step
curwidg = gtk_hbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_box_pack_start( GTK_BOX( rtree->vstep ), curwidg, FALSE, FALSE, 0 );
rtree->hstep = curwidg;

// label (texte sera mis plus tard)
curwidg = gtk_label_new( NULL );
gtk_box_pack_start( GTK_BOX( rtree->hstep ), curwidg, FALSE, TRUE, 10 );
rtree->lstep = curwidg;

// entree editable pour l'intitule
curwidg = gtk_entry_new_with_max_length (132);
gtk_widget_set_usize (curwidg, 300, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_box_pack_end( GTK_BOX( rtree->hstep ), curwidg, TRUE, TRUE, 0 );
rtree->estep = curwidg;

// boite horizontale pour les param globaux du step
curwidg = gtk_hbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_box_pack_start( GTK_BOX( rtree->vstep ), curwidg, FALSE, TRUE, 0 );
rtree->hdur = curwidg;

// bouton pause intiale
curwidg = gtk_check_button_new_with_label("Pause forcée");
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 10 );
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( bpau_check_call ), (gpointer)rtree );
rtree->bpau = curwidg;

curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#AA00AA\" weight=\"bold\" >Durée</span>");
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );

// un spin button
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0, 0, (65535/60), 1, 10, 0 );
		//	( value, lower, upper, step, page_inc, page_size )
curwidg = gtk_spin_button_new( curadj, 0, 0 );
// rtree->amin = curadj;
gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 60, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 0 );
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );
rtree->smin = curwidg;

curwidg = gtk_label_new("mn");
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );

// un spin button
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0, 0, 59, 1, 10, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );
// rtree->asec = curadj;
gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), TRUE);
gtk_widget_set_usize( curwidg, 40, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 0 );
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );
rtree->ssec = curwidg;

curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#000000\" weight=\"bold\" >Délai de grâce</span>");
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );

// un spin button
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0, 0, 255, 1, 10, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );
// rtree->addg = curadj;
gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 45, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 0 );
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );
rtree->sddg = curwidg;

curwidg = gtk_label_new( NULL );
gtk_label_set_markup( GTK_LABEL(curwidg),
  "  <span background=\"#FFFFFF\" foreground=\"#0000BB\" weight=\"bold\" >Prochain Step</span>");
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );

// un spin button
curadj = (GtkAdjustment *) gtk_adjustment_new ( 0, 0, 255, 1, 10, 0 );
curwidg = gtk_spin_button_new( curadj, 0, 0 );
gtk_signal_connect( GTK_OBJECT(curadj), "value_changed",
                    GTK_SIGNAL_FUNC(ssui_spin_call), (gpointer)rtree );
gtk_spin_button_set_wrap (GTK_SPIN_BUTTON(curwidg), FALSE);
gtk_widget_set_usize( curwidg, 45, 0 );
gtk_spin_button_set_numeric( GTK_SPIN_BUTTON(curwidg), TRUE );
gtk_spin_button_set_digits( GTK_SPIN_BUTTON(curwidg), 0 );
gtk_box_pack_start( GTK_BOX( rtree->hdur ), curwidg, FALSE, FALSE, 0 );
rtree->ssui = curwidg;

// entree non editable pour l'intitule
curwidg = gtk_entry_new();
gtk_widget_set_usize (curwidg, 150, 0);
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_box_pack_end( GTK_BOX( rtree->hdur ), curwidg, TRUE, TRUE, 0 );
rtree->esui = curwidg;

// notebook
curwidg = gtk_notebook_new();
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
gtk_box_pack_start( GTK_BOX( rtree->vstep ), curwidg, FALSE, TRUE, 0 );
rtree->nstep = curwidg;

// 1er tab : Vannes et aux
// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
gtk_notebook_append_page( GTK_NOTEBOOK(rtree->nstep), curwidg,
			  gtk_label_new(" Vannes et Auxiliaires ") );
rtree->vaux = curwidg;


// un cadre
curwidg = gtk_frame_new( "Vannes"  );
gtk_box_pack_start( GTK_BOX( rtree->vaux ), curwidg, FALSE, FALSE, 10 );
rtree->xvan = curwidg;

// une table
curwidg = gtk_table_new( 2, QVAN/2, TRUE );	// homogeneous !
gtk_container_add( GTK_CONTAINER( rtree->xvan ), curwidg );
rtree->tvan = curwidg;
int j = 0;
for ( i = 0; i < QVAN; i++ )
    {
    curnam = &(rtree->ptube->vanne[i].name);
    if   ( *curnam != string("--") )
	 {
	 lbuf = g_strdup_printf( "%s", curnam->c_str() );
         curwidg = gtk_check_button_new_with_label( lbuf );
         g_free( lbuf );
	 gtk_table_attach_defaults( GTK_TABLE(rtree->tvan), curwidg,
                      j%(QVAN/2), j%(QVAN/2) + 1, j/(QVAN/2), j/(QVAN/2) + 1 );
	 j++;
	 }
    else {
	 // lbuf = g_strdup_printf( "v%d", i );
         // curwidg = gtk_check_button_new_with_label( lbuf );
         // g_free( lbuf );
	 curwidg = NULL;
	 }
    rtree->bvan[i] = curwidg;
    }

// auxiliaires
curwidg = mkmodgui( AUX, 0, rtree );
if ( curwidg )
   gtk_box_pack_start( GTK_BOX( rtree->vaux ), curwidg, FALSE, FALSE, 10 );

// second tab : MFCs
// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
gtk_notebook_append_page( GTK_NOTEBOOK(rtree->nstep), curwidg,
			  gtk_label_new(" Débits de gaz (MFCs) ") );
rtree->vmfc = curwidg;

// des cadres pour les MFCs
for ( i = 0; i < QMFC; i++ )
    {
    curwidg = mkmodgui( MFC, i, rtree );
    if ( curwidg )
       gtk_box_pack_start( GTK_BOX( rtree->vmfc ), curwidg, FALSE, FALSE, 10 );
    }

// 3eme tab : temperatures
// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
gtk_notebook_append_page( GTK_NOTEBOOK(rtree->nstep), curwidg,
			  gtk_label_new(" Régulateurs de température ") );
rtree->vtem = curwidg;

// des cadres pour les TEMs
for ( i = 0; i < QTEM; i++ )
    {
    curwidg = mkmodgui( TEM, i, rtree );
    if ( curwidg )
       gtk_box_pack_start( GTK_BOX( rtree->vtem ), curwidg, FALSE, FALSE, 10 );
    }


// simple bouton dans la barre de boutons commune
   curwidg = gtk_button_new_with_label (" |< ");
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( rew_but_call ), (gpointer)rtree );
   gtk_box_pack_start( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
   rtree->brew = curwidg;

// simple bouton dans la barre de boutons commune
   curwidg = gtk_button_new_with_label (" Retour à la recette ");
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( ret_but_call ), (gpointer)rtree );
   gtk_box_pack_start( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
   rtree->bret = curwidg;

// simple bouton dans la barre de boutons commune
   curwidg = gtk_button_new_with_label (" >| ");
   gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                       GTK_SIGNAL_FUNC( ffw_but_call ), (gpointer)rtree );
   gtk_box_pack_start( GTK_BOX( rtree->hbut ), curwidg, TRUE, TRUE, 0 );
   rtree->bffw = curwidg;

}

// copier les parametres du modget du step de la recette vers le GUI du step
// y compris les visibilites
static void mod2gui( lintype typ, int ipod, int istep, rviewstru * rtree )
{
int ival, flagi, pflags;
epod * pepod; modget * master; modgui * curmod; char scale;

switch (typ)
   {
   case MFC : pepod  = &rtree->prec->step[istep].mfc[ipod];
	      master = &rtree->ptube->mfc[ipod]; scale = rtree->scale_type;
	      curmod = &(rtree->ymfc[ipod]);  break;
   case TEM : pepod  = &rtree->prec->step[istep].tem[ipod];
	      master = &rtree->ptube->tem[ipod]; scale = 'd';
	      curmod = &(rtree->ytem[ipod]);  break;
   default  : pepod  = &rtree->prec->step[istep].fre;
	      master = &rtree->ptube->fre; scale = 'f';
	      curmod = &(rtree->yaux); break;
   }

if ( curmod->xmod == NULL )	// expandeur null <==> pas de modget
   return;

// check button "nouvelle consigne"
// la callback bcon_check_call() de ce bouton assurera la mise a jour des visibilites
ival = pepod->SV;
flagi = ( ival >= 0 )?1:0;

// ici un artifice pour forcer l'appel de la call-back s'il n'y a pas de changement apparent
if ( flagi == ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( curmod->bcon ) )?1:0 ) )
   bcon_check_call( curmod->bcon, curmod );
// ici la mise a jour de l'etat du widget
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curmod->bcon ), flagi );

// valeur SV
if ( flagi )
   gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->scon), master->pcu2uiu( ival, scale ) );

// liste deroulante de type de controle
// la callback modget_flags_call() de cette liste assurera la mise a jour des visibilites
pflags = pepod->flags;
if ( pflags < 0 ) pflags = 0;
switch( pflags )
   {
   case MICEN :			flagi = 1; break;
   case MACEN :			flagi = 2; break;
   case (MICEN|MACEN) :		flagi = 3; break;
   case (RAMPEN|MACEN) :	flagi = 4; break;
   case (RAMPEN|MICEN) :	flagi = 5; break;
   default :			flagi = 0; break;
   }
// ici un artifice pour forcer l'appel de la call-back s'il n'y a pas de changement apparent
if ( flagi == gtk_combo_box_get_active( GTK_COMBO_BOX(curmod->cfla) ) )
   modget_flags_call( curmod->cfla, curmod );
// ici la mise a jour de l'etat du widget
gtk_combo_box_set_active( GTK_COMBO_BOX(curmod->cfla), flagi );

// mise a jour des checks
if ( ( ( pflags & RAMPEN ) == 0 ) && ( pflags & (MICEN|MACEN) ) )
   {
   // valeur SVmin
   if   ( pepod->SVmi > 0 ) ival = pepod->SVmi;
   else	if   ( pepod->SV >= 0 )	// min par defaut de l'automate
	     ival = pepod->SV - (pepod->SV >> TOLSHIFT);
	else ival = -1;
   if ( ival > 0 )
      gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->smin), master->pcu2uiu( ival, scale ) );
   // valeur SVmax
   if   ( pepod->SVma > 0 ) ival = pepod->SVma;
   else	if   ( pepod->SV >= 0 )	// max par defaut de l'automate
	     {
	     ival = pepod->SV + (pepod->SV >> TOLSHIFT);
	     if ( ival > 65535 ) ival = 65535;	// simulation saturation cf recipe.c de PROC 8.1g
	     }
	else ival = -1;
   if ( ival > 0 )
      gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->smax), master->pcu2uiu( ival, scale ) );
   }

// mise a jour des rampes
if ( pflags & RAMPEN )
   {
   if   ( pflags & MICEN )
	{
	ival = pepod->SVmi;
	if ( ival > 0 )
	   gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->smin), master->pcu2uiu( ival, scale ) );
	ival = pepod->SVma;
	if ( ival > 0 )
	   gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->sinc), master->pcu2uiu( ival, scale ) );
	}
   if   ( pflags & MACEN )
	{
	ival = pepod->SVmi;
	if ( ival > 0 )
	   gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->sinc), master->pcu2uiu( ival, scale ) );
	ival = pepod->SVma;
	if ( ival > 0 )
	   gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->smax), master->pcu2uiu( ival, scale ) );
	}
   }

// mise a jour du saut
if ( pflags )
   {
   ival = pepod->stogo;
   if	( ival >= 0 )
	{
	gtk_spin_button_set_value( GTK_SPIN_BUTTON(curmod->sjmp), ival );
	if   ( rtree->prec->step[ival].existe )
	     gtk_entry_set_text( GTK_ENTRY(curmod->ejmp), rtree->prec->step[ival].titre.c_str() );
	else if   ( ival == 0 )
		  gtk_entry_set_text( GTK_ENTRY(curmod->ejmp), "-- repos --" );
	     else gtk_entry_set_text( GTK_ENTRY(curmod->ejmp), "???" );
	}
   }

// le modget est a jour
}

// copier les parametres de la recette vers le GUI du step
// y compris les visibilites
static void step2gui( int istep, rviewstru * rtree )
{
char * lbuf; int i, ival;
etape * pstep = &rtree->prec->step[istep];
if ( ! pstep->existe )
   return;

// tout visible d'abord
gtk_widget_show_all( rtree->vstep );

lbuf = g_strdup_printf( "Edition STEP <b>%d</b> : ", istep );
gtk_label_set_markup( GTK_LABEL(rtree->lstep), lbuf );
g_free( lbuf );

gtk_entry_set_text( GTK_ENTRY( rtree->estep ), pstep->titre.c_str() );

// la duree
ival = pstep->duree;
if   ( ival <= 0 )
     {
     gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( rtree->bpau ), TRUE );
     gtk_widget_set_sensitive( rtree->smin, FALSE );
     gtk_widget_set_sensitive( rtree->ssec, FALSE );
     gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->smin), 0 );
     gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->ssec), 0 );
     }
else {
     gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( rtree->bpau ), FALSE );
     gtk_widget_set_sensitive( rtree->smin, TRUE );
     gtk_widget_set_sensitive( rtree->ssec, TRUE );
     gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->smin), ival / 60 );
     gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->ssec), ival % 60 );
     }

// le delai de grace
ival = pstep->deldg;
if ( ival < 0 ) ival = 0;	// ne devrait jamais arriver
gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->sddg), ival );

// le prochain step
ival = pstep->stogo;
if ( ival < 0 ) ival = istep + 1;
if ( ival > 255 ) ival = 0;
gtk_spin_button_set_value( GTK_SPIN_BUTTON(rtree->ssui), ival );

if   ( rtree->prec->step[ival].existe )
     gtk_entry_set_text( GTK_ENTRY( rtree->esui ), rtree->prec->step[ival].titre.c_str() );
else if   ( ival == 0 )
          gtk_entry_set_text( GTK_ENTRY( rtree->esui ), "-- repos --" );
     else gtk_entry_set_text( GTK_ENTRY( rtree->esui ), "???" );

// Les vannes
ival = pstep->vannes;
if ( ival < 0 ) ival = 0;	// ne devrait jamais arriver
for ( i = 0; i < QVAN; i++ )
    {
    if	( rtree->bvan[i] )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( rtree->bvan[i] ), ( ival & 1 ) );
    ival >>= 1;
    }


// les MFCs
for ( i = 0; i < QMFC; i++ )
    mod2gui( MFC, i, istep, rtree );

// les temperatures
for ( i = 0; i < QTEM; i++ )
    mod2gui( TEM, i, istep, rtree );

// les aux
mod2gui( AUX, 0, istep, rtree );

}

// copier les parametres du GUI vers le modget du step de la recette
static void gui2mod( lintype typ, int ipod, int istep, rviewstru * rtree )
{
int ival, flagi; double dval;
int echomod = 1;	// echo des modifs vers stdout
epod * pepod; modget * master; modgui * curmod; char scale;

switch (typ)
   {
   case MFC : pepod  = &rtree->prec->step[istep].mfc[ipod];
	      master = &rtree->ptube->mfc[ipod]; scale = rtree->scale_type;
	      curmod = &(rtree->ymfc[ipod]);  break;
   case TEM : pepod  = &rtree->prec->step[istep].tem[ipod];
	      master = &rtree->ptube->tem[ipod]; scale = 'd';
	      curmod = &(rtree->ytem[ipod]);  break;
   default  : pepod  = &rtree->prec->step[istep].fre;
	      master = &rtree->ptube->fre; scale = 'f';
	      curmod = &(rtree->yaux); break;
   }

if ( curmod->xmod == NULL )	// expandeur null <==> pas de modget
   return;

// check button "nouvelle consigne"
flagi = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( curmod->bcon ) );
// nouvelle consigne
if   ( flagi )
     {
     dval = gtk_spin_button_get_value( GTK_SPIN_BUTTON(curmod->scon) );
     ival = master->uiu2pcu( dval, scale );
     }
else {
     ival = -1;
     }
if ( ival != pepod->SV )
   {
   if ( echomod )
      printf("* SV %d->%d %.9f->%.9f->%.9f ", pepod->SV, ival,
		master->pcu2uiu( pepod->SV, scale ),
		gtk_spin_button_get_value( GTK_SPIN_BUTTON(curmod->scon) ),
		master->pcu2uiu( ival, scale )
  	    );
   pepod->SV = ival; rtree->modcnt++;
   }

}

// copier les parametres du GUI du step vers la recette en comptant les modifs
// (pas de check pour le moment)
static void gui2step( int istep, rviewstru * rtree )
{
int i, ival; string sbuf;
int echomod = 1;	// echo des modifs vers stdout
etape * pstep = &rtree->prec->step[istep];
if ( ! pstep->existe )
   return;

// le titre
sbuf = string( gtk_entry_get_text( GTK_ENTRY( rtree->estep ) ) );
if ( sbuf != pstep->titre )
   {
   if ( echomod ) printf("* titre ");
   pstep->titre = sbuf; rtree->modcnt++;
   }

// le toggle pause et la duree
if   ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( rtree->bpau ) ) )
     {
     ival = -1;
     }
else {
     ival = 60 * (int) gtk_spin_button_get_value( GTK_SPIN_BUTTON(rtree->smin) );
     ival +=     (int) gtk_spin_button_get_value( GTK_SPIN_BUTTON(rtree->ssec) );
     if ( ival > 0xFFFF ) ival = 0xFFFF;
     }
if ( ival != pstep->duree )
   {
   if ( echomod ) printf("* duree : gui=%d, recette=%d", ival, pstep->duree );
   pstep->duree = ival; rtree->modcnt++;
   }

// le delai de grace
ival = (int) gtk_spin_button_get_value( GTK_SPIN_BUTTON(rtree->sddg) );
if ( ival != pstep->deldg )
   {
   if ( echomod ) printf("* deldg ");
   pstep->deldg = ival; rtree->modcnt++;
   }

// le prochain step
ival = (int) gtk_spin_button_get_value( GTK_SPIN_BUTTON(rtree->ssui) );
if ( ival != pstep->stogo )
   if ( !( ( ival == istep + 1 ) && ( pstep->stogo == -1 ) ) )
      {
      if ( echomod ) printf("* stogo ");
      pstep->stogo = ival; rtree->modcnt++;
      }

// Les vannes
ival = 0;
for ( i = 0; i < QVAN; i++ )
    {
    ival >>= 1;
    if	( rtree->bvan[i] )
	{
	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( rtree->bvan[i] ) ) )
	   ival |= 0x8000;
	}
    }
if ( ival != pstep->vannes )
   {
   if ( echomod ) printf("* vannes : gui=%04x recette=%04x ", ival, pstep->vannes );
   pstep->vannes = ival; rtree->modcnt++;
   }

// les MFCs
for ( i = 0; i < QMFC; i++ )
    gui2mod( MFC, i, istep, rtree );

// les temperatures
for ( i = 0; i < QTEM; i++ )
    gui2mod( TEM, i, istep, rtree );

// les aux
gui2mod( AUX, 0, istep, rtree );

char * lbuf;
if   ( rtree->modcnt )
     lbuf = g_strdup_printf( "RECETTE : <b>%s</b>  <i>%d</i>", rtree->prec->titre.c_str(), rtree->modcnt );
else lbuf = g_strdup_printf( "RECETTE : <b>%s</b>", rtree->prec->titre.c_str() );
gtk_label_set_markup( GTK_LABEL( rtree->lmain ), lbuf );
g_free( lbuf );

if ( ( echomod ) && ( rtree->modcnt ) )
   { printf("\n"); fflush(stdout); }
}

// passer en edition de step :
static void edit_step( GtkTreeIter * piter, rviewstru * rtree )
{
gtk_tree_model_get( (GtkTreeModel *)(rtree->tmod), piter, 1, &rtree->selected, -1 );

// creer le GUI si necessaire
if ( rtree->wstep == NULL )
   mkstepgui( rtree );

// mettre a jour les widgets du step
step2gui( rtree->selected, rtree );

// commuter l'affichage
gtk_widget_hide( rtree->wlis );
gtk_widget_show( rtree->wstep );
gtk_widget_hide( rtree->bedi );
gtk_widget_show( rtree->brew );
gtk_widget_show( rtree->bffw );
gtk_widget_show( rtree->bret );
}
