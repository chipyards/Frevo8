
#ifdef  __cplusplus
extern "C" {
#endif

// fonctions permettant d'acceder a l'objet tube depuis le 'C'

void bridge_initfour( const char * xmlpath, int ifou );

unsigned char * bridge_get_destIP();

/*=========================== CLI ========================= */

void iproc_ui();

#ifdef  __cplusplus
}
#endif
