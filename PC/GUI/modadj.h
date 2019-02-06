#ifdef  __cplusplus
extern "C" {
#endif

/* popup dialog "modal" pour info ou exception
   il bloque tout le reste.
 */

double modadj( const char *title, const char *txt, GtkWindow *parent,
		double start, double fs );

#ifdef  __cplusplus
}
#endif
