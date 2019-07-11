
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

