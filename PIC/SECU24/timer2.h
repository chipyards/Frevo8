// timer 2 en mode compteur - frequencemetre

extern long ticks_cnt;		// temps en ticks de 6.4 us, sur 32 bits
extern unsigned int tacks_cnt;	// comptage pulses 
extern unsigned int frePV;	// resultat mesure

// demarrer frequencemetre sur LM331
void freq_vfin_init( void );

// demarrer frequencemetre sur entree opto
void freq_optin_init( void );

/* tache frequencemetre
   mesure le rapport delta_tacks / delta_ticks
	delta_tacks = compte d'impulsions depuis dernier appel
	delta_ticks = duree depuis dernier appel
   resultat 16 bits dans frePV
 */
void freqmeter( void );
