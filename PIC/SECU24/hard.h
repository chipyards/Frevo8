
// clock
#define FOSC_KHZ 20000U		// 
#define FCY_KHZ (FOSC_KHZ/2U)	// = MIPs

// fonction tempo solide (toute en asm)(lib libpic30 linkee automatiquement)
#include <libpic30.h>
#define __delay_ms(d) __delay32((unsigned long)((d)*(unsigned long)FCY_KHZ))

// bits


// remappable pins : UART 2
  // TX output : field et valeur
  #define UTX_PIN  _RP2R	// field associe a RP2
  #define U2TXSEL 5		// val for a RPORx
  // RX input : valeur pour le field _U2RXR
  #define URX_PIN 4		// RP4

// remappable pin : timer 2
  // ext clock input : valeur pour le field _T2CKR
  #define VFI_PIN 12		// VFIN = RP12 (LM331)
  #define OPT_PIN 13		// OPTIN = RP13 (6N136)
  // Digital-Not-Analog
  #define VFI_DNA _PCFG12	// RP12 = AN12
  #define OPT_DNA _PCFG11	// RP13 = AN11

// relais
#define REL1_BIT LATAbits.LATA0
#define REL2_BIT LATAbits.LATA1
#define REL1_TRI TRISAbits.TRISA0
#define REL2_TRI TRISAbits.TRISA1

// flow-switch
#define FSW_BIT	PORTAbits.RA4
#define FSW_TRI	TRISAbits.TRISA4
