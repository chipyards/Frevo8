
/*
PIC 30F, 24F, 24H, 33F : Attention aux unites :
chaque instruction :
- est codee sur 3 bytes
- occupe 2 locations dans l'espace PC (program counter)
- occupe 4 "raw bytes" dans le fichier hex
  (dans le fichier hex les adresses sont des adresses de raw bytes) 

24HJ128GP specific :
- erase block = 1024 PC-units = 1536 bytes = 512 instr.
- chaque bloc represente 2048 raw bytes dans le fichier hex
- adresse PC maxi = 0x15800
- progr. mem = 44032 mots de 24 bits soit 0x15800 PCunits ou 132096 bytes
- fichier hex contient max 176128 raw bytes (plus config)

24FJ64GA002 specific :
- block : idem
- adresse PC maxi = 0xABFC
- progr. mem = 22014 mots de 24 bits soit 0xABFC PCunits ou 66042 bytes
- fichier hex contient max 88056 raw bytes (plus config)
- les mots de config sont juste apres, il faut tailler au plus juste pour
  ne pas les avoir dans le fichier .bin

33FJ32MC302 specific :
- block : idem
- adresse PC maxi = 0x5800
- progr. mem = 11264 mots de 24 bits soit 0x5800 PCunits ou 33792 bytes
- fichier hex contient max 45056 raw bytes (plus config)

*/

#define PICTYPE "33F"

// #define QRAWBYTES (176128)  /* capacite PIC 24HJ128GP en raw bytes */
#define QRAWBYTES (88056)  /* capacite PIC 24FJ64GA002 en raw bytes */
// #define QRAWBYTES (45056)  /* capacite PIC 33FJ32MC302 en raw bytes */

#define QINSTR (QRAWBYTES/4)

#define RAWBLOCK 2048
#define INSBLOCK (RAWBLOCK/4)
#define LOCBLOCK (RAWBLOCK/2)
#define QBLOCKS (QRAWBYTES/RAWBLOCK)

#define ERASED 0xFFFFFF

#define MAX_INTEL_HEX_RECORD_LENGTH 16

typedef struct _INTEL_HEX_RECORD
{
   unsigned char  Length;
   unsigned short Address;
   unsigned char  Type;
   unsigned char  Data[MAX_INTEL_HEX_RECORD_LENGTH];
} INTEL_HEX_RECORD;

void lec_hex_rec( INTEL_HEX_RECORD * prec );

int hex2i ( char ascii ); /* CONVERSION  1 char ASCII hexa -> int */

void lec_hex_file( const char *fnam );

// la fonction lec_hex_rec() :
// copie les donnees dans rawbytes[], met a jour rawblocks[]
void lec_hex_rec( INTEL_HEX_RECORD * prec );
