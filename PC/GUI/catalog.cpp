/* listeur-chooser de recettes */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../version.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"
#include "catalog.h"

#include <string>
// #include <map>
#include <vector>

/* garnissage de la liste scrollable */
static void fill_list( catastru * cat )
{
GtkListStore *mymod; unsigned int j;
GtkTreeIter   iter;

mymod = cat->tmod;

gtk_list_store_clear( mymod );

if ( cat->ptube->reclist.size() == 0 )
   return;

for ( j = 0; j < cat->ptube->reclist.size(); j++ )
    {
    gtk_list_store_append( mymod, &iter );
    gtk_list_store_set( mymod, &iter, 0, j, -1 );
    }

char lbuf[256];
sprintf( lbuf, "Recettes lues dans : %s", cat->ptube->xml_dir.c_str() );
gtk_label_set_text( (GtkLabel *)(cat->lmain), lbuf );
}

/* ---------------- SIMPLE CALL BACKS -------------------- */

/* il faut intercepter le delete event pour que si l'utilisateur
   ferme la fenetre on revienne sans engendrer de destroy signal.
   A cet effet ce callback doit rendre TRUE ( gtk_main_quit() ne le fait pas)
 */
static gint delete_call( GtkWidget *widget,
                  GdkEvent  *event, gpointer   data )
{
gtk_main_quit();
return (TRUE);
}


// rafraichir la liste (recompiler les recettes)
static void raf_but_call( GtkWidget *widget, catastru * cat )
{
cat->ptube->scan_rec();
fill_list( cat );
}


// choisir un repertoire de recettes
static void cwd_but_call( GtkWidget *widget, catastru * cat )
{
GtkWidget *dialog;
//GtkFileFilter *filter;

dialog = gtk_file_chooser_dialog_new ("Choix d'un repertoire de recettes",
				      GTK_WINDOW(cat->wmain),
				      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

//filter = gtk_file_filter_new();
//gtk_file_filter_add_pattern( filter, "*.xml" );

//gtk_file_chooser_set_filter ( GTK_FILE_CHOOSER (dialog), filter );

if ( cat->ptube->xml_dir.size() )
   gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER (dialog),
                                        cat->ptube->xml_dir.c_str() );

if ( gtk_dialog_run( GTK_DIALOG (dialog) ) == GTK_RESPONSE_ACCEPT )
   {
   char *dirpath;
   dirpath = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER (dialog) );
   cat->ptube->xml_dir = string(dirpath);
   g_free( dirpath );

   char lbuf[256];
   sprintf( lbuf, "Recettes lues dans : %s", cat->ptube->xml_dir.c_str() );
   gtk_label_set_text( (GtkLabel *)(cat->lmain), lbuf );
   cat->ptube->scan_rec();
   fill_list( cat );
   }

gtk_widget_destroy (dialog);
}


// charger recette (c'est a dire retourner au superviseur le numero)
static void cha_but_call( GtkWidget *widget, catastru * cat )
{
GtkTreeSelection* cursel;
GtkTreeIter iter; int i;

cursel = gtk_tree_view_get_selection( (GtkTreeView*)cat->tlis );
if   ( gtk_tree_selection_get_selected( cursel, NULL, &iter ) )
     {
     gtk_tree_model_get( (GtkTreeModel *)(cat->tmod), &iter, 0, &i, -1 );
     if   ( cat->ptube->reclist[i].stat >= 0 )
	  {
	  cat->selected = i;
	  gtk_main_quit();
	  }
     }
// ne rien faire s'il n'y a pas une recette valide selectionnee
}

// editer recette (c'est a dire retourner au superviseur le numero + 0x4000 )
static void edi_but_call( GtkWidget *widget, catastru * cat )
{
GtkTreeSelection* cursel;
GtkTreeIter iter; int i;

cursel = gtk_tree_view_get_selection( (GtkTreeView*)cat->tlis );
if   ( gtk_tree_selection_get_selected( cursel, NULL, &iter ) )
     {
     gtk_tree_model_get( (GtkTreeModel *)(cat->tmod), &iter, 0, &i, -1 );
     if   ( cat->ptube->reclist[i].stat >= 0 )
	  {
	  cat->selected = i | 0x4000;
	  gtk_main_quit();
	  }
     }
// ne rien faire s'il n'y a pas une recette valide selectionnee
}

// editer : lance un editeur externe puis donne un dump dans stdout
static void edix_but_call( GtkWidget *widget, catastru * cat )
{
GtkTreeSelection* cursel;
GtkTreeIter iter; int i;

cursel = gtk_tree_view_get_selection( (GtkTreeView*)cat->tlis );
if   ( gtk_tree_selection_get_selected( cursel, NULL, &iter ) )
     {
     gtk_tree_model_get( (GtkTreeModel *)(cat->tmod), &iter, 0, &i, -1 );
     string cmd;
     #ifdef WIN32
     #define SLASH '\\'
     cmd = "c:\\appli\\wscite\\SciTE.exe " + cat->ptube->xml_dir + SLASH
	   + cat->ptube->reclist[i].filename;
     #else
     #define SLASH '/'
     cmd = "kwrite " + cat->ptube->xml_dir + SLASH
	   + cat->ptube->reclist[i].filename;
     #endif
     system( cmd.c_str() );

     /* au retour de l'editeur : un petit dump dans stdout... *
     recipe tmprec( cat->ptube );
     tmprec.filename = cat->ptube->reclist[i].filename;
     tmprec.load_xml();
     if   ( tmprec.stat > -2 )
	  tmprec.check();
     if   ( tmprec.stat > -2 )
	  {
	  tmprec.make_pack();
	  tmprec.dump_pack();
	  cout << tmprec.dump;
	  }
     else cout << tmprec.errmess << endl;
     //*/
     gtk_main_quit();	// pour eviter que la fenetre catalogue reste cachee mais bloquante
     }
}


static void qui_but_call( GtkWidget *widget, catastru * cat )
{
gtk_main_quit();
}


// une call back pour le double-clic sur la liste
// meme effet que le bouton edit
static void double_clic_call( GtkTreeView *curwidg,
				GtkTreePath *path,
				GtkTreeViewColumn *col,
				catastru * cat )
{
GtkTreeIter iter;
if ( gtk_tree_model_get_iter( (GtkTreeModel *)(cat->tmod), &iter, path ) )
   {
   int i;
   gtk_tree_model_get( (GtkTreeModel *)(cat->tmod), &iter, 0, &i, -1);
   if   ( cat->ptube->reclist[i].stat >= 0 )
	{
	cat->selected = i | 0x4000;
	gtk_main_quit();
	}
   }
}

/* ---------------- TREE DATA CALL BACKS -------------------- */

static void file_data_call( GtkTreeViewColumn * tree_column,	// sert pas !
	GtkCellRenderer   * rendy, GtkTreeModel      * tree_model,
	GtkTreeIter       * iter,  gpointer          cat )
{
int i;
// recuperer la donnee dans la colonne 0
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
g_object_set( rendy, "text",
	      ((catastru *)cat)->ptube->reclist[i].filename.c_str(), NULL
	    );
if   ( ((catastru *)cat)->ptube->reclist[i].stat >= 0 )
     g_object_set( rendy, "cell-background", "#C0FFC0",
			  "cell-background-set", TRUE, NULL );
else g_object_set( rendy, "cell-background", "#FFD080",
			  "cell-background-set", TRUE, NULL );
}

static void qstep_data_call( GtkTreeViewColumn * tree_column,
	GtkCellRenderer   * rendy, GtkTreeModel      * tree_model,
	GtkTreeIter       * iter,  gpointer          cat )
{
int i; gchar * text;
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
text = g_strdup_printf( "%d", ((catastru *)cat)->ptube->reclist[i].qstep );
g_object_set( rendy, "text", text, NULL );
}

static void titre_data_call( GtkTreeViewColumn * tree_column,
	GtkCellRenderer   * rendy, GtkTreeModel      * tree_model,
	GtkTreeIter       * iter,  gpointer          cat )
{
int i;
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
if   ( ((catastru *)cat)->ptube->reclist[i].stat >= 0 )
     g_object_set( rendy, "text",
		   ((catastru *)cat)->ptube->reclist[i].titre.c_str(), NULL
		 );
else g_object_set( rendy, "text",
		   ((catastru *)cat)->ptube->reclist[i].errmess.c_str(), NULL
		 );
}

static void bytes_data_call( GtkTreeViewColumn * tree_column,
	GtkCellRenderer   * rendy, GtkTreeModel      * tree_model,
	GtkTreeIter       * iter,  gpointer          cat )
{
int i; gchar * text;
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
text = g_strdup_printf( "%d", ((catastru *)cat)->ptube->reclist[i].packlen );
g_object_set( rendy, "text", text, NULL );
}


/* ------------------ CONSTRUCTION GUI ------------------ */

// creer l'afficheur de liste
static GtkWidget * maketview( catastru * cat )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;

// 1 colonne de type int
cat->tmod = gtk_list_store_new( 1, G_TYPE_INT );

curwidg = gtk_tree_view_new();

// la colonne nom de fichier, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();
gtk_tree_view_column_set_title( curcol, " Fichier " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         file_data_call,
                                         (gpointer)cat, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// la colonne nombre de steps, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();
gtk_tree_view_column_set_title( curcol, "Steps" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         qstep_data_call,
                                         (gpointer)cat, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// la colonne titre, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();
gtk_tree_view_column_set_title( curcol, " Intitule " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         titre_data_call,
                                         (gpointer)cat, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// la colonne nom de fichier, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();
gtk_tree_view_column_set_title( curcol, " Bytes " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         bytes_data_call,
                                         (gpointer)cat, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );


// configurer la selection (no callback)
cursel = gtk_tree_view_get_selection( (GtkTreeView*)curwidg );
gtk_tree_selection_set_mode( cursel, GTK_SELECTION_SINGLE );

// connecter callback pour double-clic
g_signal_connect( curwidg, "row-activated",
		  (GCallback)double_clic_call, (gpointer)cat );

// connecter modele
gtk_tree_view_set_model( (GtkTreeView*)curwidg, GTK_TREE_MODEL( cat->tmod ) );

return(curwidg);
}

// afficher le catalogue dans fenetre modale
// element preselectionne est celui qui a stat == 4
// retourne :
//	index nouvel element selectionne si chargement demande
//	index nouvel element selectionne | 0x4000 si edition demandee
//	-1 si rien a faire
// indexes pointent sur ptube->reclist[]
int catalog_recettes( four *ptube )
{
catastru thecat;
#define cat (&thecat)
GtkWidget *curwidg;

cat->ptube = ptube; cat->selected = -1;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );
gtk_window_set_modal( GTK_WINDOW(curwidg), TRUE );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( delete_call ), NULL );

char lbuf[256];
sprintf( lbuf, "%s (Frevo %d.%d%c)", ptube->nom.c_str(), VERSION, SUBVERS, BETAVER );
gtk_window_set_title( GTK_WINDOW (curwidg), lbuf );
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 8 );
cat->wmain = curwidg;

// creer boite verticale
curwidg = gtk_vbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( cat->wmain ), curwidg );
cat->vmain = curwidg;

// label
curwidg = gtk_label_new ( "Recettes" );
gtk_box_pack_start( GTK_BOX( cat->vmain ), curwidg, FALSE, TRUE, 0 );
cat->lmain = curwidg;

// scrolled window avec liste
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(curwidg),
				     GTK_SHADOW_IN);
gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(curwidg),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
gtk_widget_set_usize( curwidg, 800, 400 );
gtk_box_pack_start( GTK_BOX( cat->vmain ), curwidg, TRUE, TRUE, 0 );
cat->wlis = curwidg;

cat->tlis = maketview( cat );
fill_list( cat );	// remplissage initial
gtk_container_add( GTK_CONTAINER( cat->wlis ), cat->tlis );

// boite a boutons
curwidg = gtk_hbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_box_pack_start ( GTK_BOX( cat->vmain ), curwidg, FALSE, FALSE, 0);
cat->hbut = curwidg;

// simple bouton : refresh
curwidg = gtk_button_new_with_label (" Actualiser ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( raf_but_call ), (gpointer)cat );
gtk_box_pack_start( GTK_BOX( cat->hbut ), curwidg, TRUE, TRUE, 0 );
cat->braf = curwidg;

// simple bouton : chdir
curwidg = gtk_button_new_with_label (" Changer de dir ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( cwd_but_call ), (gpointer)cat );
gtk_box_pack_start( GTK_BOX( cat->hbut ), curwidg, TRUE, TRUE, 0 );
cat->bcwd = curwidg;

// simple bouton : load
curwidg = gtk_button_new_with_label (" Charger ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( cha_but_call ), (gpointer)cat );
gtk_box_pack_start( GTK_BOX( cat->hbut ), curwidg, TRUE, TRUE, 0 );
cat->bcha = curwidg;

// simple bouton : edit
curwidg = gtk_button_new_with_label (" Editer ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( edi_but_call ), (gpointer)cat );
gtk_box_pack_start( GTK_BOX( cat->hbut ), curwidg, TRUE, TRUE, 0 );
cat->bedi = curwidg;

// simple bouton : edit XML
curwidg = gtk_button_new_with_label (" Editer XML ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( edix_but_call ), (gpointer)cat );
gtk_box_pack_start( GTK_BOX( cat->hbut ), curwidg, TRUE, TRUE, 0 );
cat->bedx = curwidg;

// simple bouton : close
curwidg = gtk_button_new_with_label (" Fermer ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( qui_but_call ), (gpointer)cat );
gtk_box_pack_start( GTK_BOX( cat->hbut ), curwidg, TRUE, TRUE, 0 );
cat->bqui = curwidg;

gtk_widget_show_all ( cat->wmain );

GtkTreeSelection * cursel = gtk_tree_view_get_selection( (GtkTreeView*)cat->tlis );
gtk_tree_selection_unselect_all( cursel );

gtk_main();
gtk_widget_destroy( cat->wmain );
return(cat->selected);
}
