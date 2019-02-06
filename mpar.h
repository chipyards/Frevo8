// USB : taille paquets bulk (<= 64)
#define QBULK 32


// taille buffer MPAR
#define QMPAR 26	// une puissance de 2 ? meme pas oblige. doit etre < QBULK
// taille max contenu utile requete UART
#define QUPILOT 30

// constantes poour le bootloader PIC24F/33F =============================

// limite pour eviter auto-ecrasement !
#define MINROMADR 0x01000L

// ecriture en flash PIC24H, 24J, 33F "row" et "block"
#define QROWBYTES   192
#define QROWPCUNITS 128
#define QROWPERBLOCK 8
#define QBLOCKBYTES (QROWBYTES*QROWPERBLOCK)
#define QBLOCKPCUNITS (QROWPCUNITS*QROWPERBLOCK)

// decoupage "row" en "segments" pour s'accomoder
// de la taille limitee des paquets USB et de la RAM du PIC18
// il FAUT : QSEGBYTES <= QMPAR-2 < QBULK
#define QSEGBYTES 24
#define QFSEG (QROWBYTES/QSEGBYTES)

// opcodes communs
#define SYSVER		4
#define SYSNOP		0xF0	// pour resynchroniser liaison async
#define ERR_OP		0xFF
#define ERR_PAR		0xFE
#define ERR_DLY		0xFD

// opcodes pour le bootloader		!! le MSB sert de bit d'adresse !!
#define WR_FSEG	  0x10	// ecriture vers rowbuf d'un segment de data a flasher
#define WR_FLASH  0x11	// copie de rowbuf dans la flash meme
#define ERA_FLASH 0x12	// effacement d'un bloc de la flash
#define RD_FLASH  0x13	// copie de la flash vers rowbuf
#define RD_FSEG	  0x14	// lecture d'un segment de data de rowbuf
#define CRC_FLASH 0x15	// verifie le CRC a partir de MINROMADR
#define EXE_FLASH 0x16	// executer l'appli a partir de MINROMADR si crc ok
#define SW_RESET  0x17	// executer un SW/reset

