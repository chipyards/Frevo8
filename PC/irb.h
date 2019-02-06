/*
   IRB = ipilot request block
	- contient demande et reponse
	- peut etre utilise par les fonctions dial (bloquantes)
	- peut etre mis en queue

   La taille de buffer QIPILOT est definie dans ipilot.h
 */
#ifndef IRBLOCK
#define IRBLOCK

// ipilot request block (irb)
typedef struct {
unsigned char txbuf[QIPILOT+4];
int txcnt;
int txgot;	// code de resultat (-1 si pas encore tente, 0 si ok)
unsigned char rxbuf[QIPILOT+4];
int rxcnt;
int rxgot;	// code de resultat
int tenta;	// decompteur de tentatives
} irblock;

#endif
