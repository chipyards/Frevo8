
// mise a jour des visibilites des widgets en manuel ou auto
void show_art_manu( int flag, glostru * glo  );

// mise a jour des spinboxes en fonction des valeurs de SV restituees
// par l'automate et du type d'echelle courant
// attention : cause le retour des valeurs vers l'automate, ne pas iterer !!
// void update_SVboxes( glostru * glo );

// mise a jour affichage a chaque seconde
void display_art_status( glostru * glo );

// fonction qui cree une drawing area et son contenu...
GtkWidget * mk_dart( glostru *glo );

// copie de la drawing area
void art_dump( glostru * glo );
