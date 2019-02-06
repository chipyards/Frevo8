/* Frevo 7 derive de Mambo 5 derive de Samba 4
Definition des opcodes pour pilotage et flashage automate process SBC18b
- messages de 1 a 32 bytes, little endian
- requetes :
	- premier byte = opcode
		- MSB = 0 ==> opcode long de 7 bits
		- MSB = 1 ==> opcode court 3 bits suivis d'une adresse 
- lecture subsequente
	- premier byte = status
		- copie dernier opcode <==> tout OK
		- Fx = codes erreur
	- bytes suivants eventuels : valides seulement si status ok
 */

#define QIPILOT 32	// taille maxi checksum inclus

// opcodes longs					data aller	data retour (apres status)
//							chksum exclus	chksum exclus
//							+2 pour rxcnt	+1 pour txcnt	<== pour le firmware
#define SYSLED	0	// sys led			1 byte
#define SYSTIM	2	// system time in seconds	0 byte		4 bytes
#define SYSDBG	3	// system debug...		variable	variable
	// PIC RAM i/o : [SYSDBG]['i'][ADRL][ADRH][cnt]
	//               [SYSDBG]['o'][ADRL][ADRH][D0][D1]... ROOT reserved
#define MAXUPLOAD 27
#define MAXDNLOAD 30

#define SYSVER	4	// system version		0 byte		4 bytes, 5 bytes a partir de Frevo 7.9

#define RS485T	6	// RS485 emission (si ROOT)	variable	idem aller
#define RS485R	7	// RS485 reception (si ROOT)	variable	idem aller

#define ONEW_Q	8	// 1-wire transaction RstTxRx	variable	0 byte
	// [ONEW_Q][onew_rcnt][D0][D1]...
#define ONEW_R	9	// 1-wire retrieve reply	1 byte		variable
	// [ONEW_R][onew_rcnt]
#define ONEWRST	0x0A	// 1-wire reset			0 byte		0 byte
#define ONEWBT	0x0B	// 1-wire bit tx		1 byte		0 byte
#define ONEWBR	0x0C	// 1-wire bit rx		0 byte		1 byte

#define ONEWT	0x0D	// 1-wire emission		variable	idem aller
#define ONEWQ	0x0E	// 1-wire query			1 byte		0 byte
#define ONEWR	0x0F	// 1-wire recup. reception	variable	variable

// specifique mambo5 / ASIX
#define ARDREG	0x10	// lecture registre		1 byte		1 byte
#define AWRREG	0x11	// ecriture registre		2 bytes
#define	ARDRAM	0x12	// lecture RAM 8 bytes		2 bytes		8 bytes
#define AWRRAM	0x13	// ecriture RAM 8 bytes		10 bytes
#define ARDMII	0x14	// lecture MII reg		1 byte		2 bytes
#define AWRMII	0x15	// ecriture MII reg		3 bytes

#define LCDTXT	0x1F	// affichage ascii sur LCD	15 bytes

// opcodes UDP pour passerelle SBC18a
#define UE	0x21	// echo pour appli Mambo 7
#define UW	0x22	// transmettre requete i2c write
#define UR 	0x23	// transmettre requete i2c read
#define UURW 	0x24	// requete UART bidirectionnelle

// reponses UDP
#define UL	0x28	// refus 'lock'
#define UN	0x29	// obtenu i2c Nak
#define UA	0x2A	// obtenu i2c Ack

// specifiques process
#define ALLOFF	1	// vannes off			0 byte
#define PSTEP	0x40	// go to step			1 byte		1 byte : step
#define PMANU	0x41	// manual control (PAUSE etc)	1 byte
#define PCRC	0x42	// query recipe CRC		1 byte : autor	7 bytes : crc status, crc, taille recette pack.
#define PCHRON	0x43	// force chrono			2 byte		0 bytes
#define PFULL	0x4E	// query full process status	0 byte		30 bytes : step, vannes, ETA, status, 8 triplets.

#define PAUSE	1	// bit de PMANU
#define MANU	2	//  "      "
#define ROOT	0x20	//  "      "

#define CRC_CALC	1	// bit de crc status :	calcul en cours
#define CRC_READY	2	//			calcul initial fait
#define CRC_AUTOR	4	//			autorisation d'execution
#define CRC_POLY 0xEDB88320	// polynome de zlib, zip et ethernet

// opcodes courts
#define DAC	0x80	// 3 LSBs = adresse DAC		2 bytes 
#define VAON	0xA0	// 4 LSBs = adresse vanne	0 byte
#define VAOFF	0xB0	// "  "       "       "		0 byte
#define TCONS	0xC0	// 2 LSBs = adr. reg temp	2 bytes 
// obsoletes (cf PMANU
// #define ADC	0x90	// 3 LSBs = adresse ADC		0 byte		4 bytes : PV, SV
// #define TVAL	0xD0	// 2 LSBs = adr. reg temp	0 byte		4 bytes : PV, SV

// bootloader PIC18
// noter ici des codes dont la distance est >= 2
#define FLASHR	0x70	// lire 8 bytes data flash	2 bytes		8 bytes
#define FLASHW	0x76	// ecrire 8 bytes data		10 bytes (2 adr, 8 data)
#define FERASE	0x79	// effacer 64 bytes flash	2 bytes  (2 adr)

// codes erreurs
#define SOUNK	0xF0	// opcode inconnu
#define ILLERR	0xF1	// requete illegale
#define SFERRA	0xFA	// tentative ecriture sur adresse protegee
#define CHKERR	0xFC	// erreur checksum (somme doit etre 0x69 opcode et check byte compris)
#define NAKI2C	0xFD	// erreur sur I2C secondaire (DACs)
#define LENERR	0xFE	// erreur longueur message
#define XOFF	0xFF	// lecture prematuree

// init checksum
#define MYRSUM	0x69
#define MYTSUM	(-MYRSUM)

// opcodes pour l'automate securite
#define SET_PARA	0xE0	// 3 bytes : index, param sur 16 bits
#define GET_SSTA	0xE7	// 7 bytes : step, 3 mesures sur 16 bits
