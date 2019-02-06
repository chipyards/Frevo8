
// mise a jour des visibilites des widgets en manuel ou auto
void show_txt_manu( int flag, glostru * glo  );

// mise a jour des spinboxes en fonction des valeurs de SV restituees
// par l'automate et du type d'echelle courant
// attention : cause le retour des valeurs vers l'automate, ne pas iterer !!
void update_spinboxes( glostru * glo );

// mise a jour affichage a chaque seconde
void display_txt_status( glostru * glo );

/* fonction qui cree un cadre horizontal contenant les
   controles des vannes */
GtkWidget * mk_fvan( glostru *glo );


/* fonction qui cree un cadre horizontal contenant les
   controles des MFCs */
GtkWidget * mk_fmfc( glostru *glo );


/* fonction qui cree un cadre horizontal contenant les
   controles des regulateurs de temperature */
GtkWidget * mk_ftem( glostru *glo );


/* fonction qui cree un cadre horizontal contenant les
   affichages des auxiliaires */
GtkWidget * mk_faux( glostru *glo );

/* fonction qui cree une petite boite verticale contenant les
   boutons pour le choix d'unites, l'automate secu, etc... */
GtkWidget * mk_vmisc( glostru *glo );

