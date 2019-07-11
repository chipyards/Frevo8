
/* fonction reflashable pour adresse IP et fin adresse MAC */
void copy_IP_MAC5( void );

/* fonction pour preparer header ethernet en reponse au packet courant */
void swapMAC( void );

/* fonction pour preparer packet ARP en reponse au packet courant */
void swapARP( void );

/* fonction pour preparer header IP en reponse au packet courant */
void swapIP( void );

/* fonction pour preparer header UDP en reponse au packet courant */
void swapUDP( void );

/* traiter un paquet dans le buffer packet[],
   eventuellement preparer reponse dans le meme buffer
   le code retour indique l'action prise :
	- 0  : rien, packet n'est pas pour nous
	- 1  : requete ARP, reponse standard prete a envoyer
	- 2  : requete icmp, reponse standard prete a envoyer
	- 3  : requete UDP, pour le bon port, echo a elaborer et envoyer
	- 88 : requete UDP, pas pour le bon port
	- 89 : requete UDP, erreur checksum
	- FF : non supporte
 */
unsigned char process_pack( void );


// UDP local server port (in network order)
#define MYUPORT0 0x07	// 1953 = 0x7a1
#define MYUPORT1 0xa1

