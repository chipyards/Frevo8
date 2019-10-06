/* compilateur de recette - contient aussi une ebauche du futur GUI */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>

using namespace std;

#include "xmlpe.h"
#include "frevo_dtd.h"
#include "process.h"
#include "dirlist.h"
#include "crc32.h"

extern "C" void gasp( const char *fmt, ... );  /* fatal error handling */


// static storage all in one
four tube;	// exportable !

DTD_four      four::dtd;
DTD_recette recipe::dtd;


// methodes du tube 'four' ------------------------------------- //
void four::load_xml( const char * fourpath )
{
xmlobj fourxml( fourpath, &dtd );
if ( !(fourxml.is) )
   gasp("echec ouverture fichier %s", fourxml.filepath );

// parsing
int ipod;
int status; xelem * elem;
int bonfour = 0;

while ( ( status = fourxml.step() ) != 9 )
  {
  elem = &fourxml.stac.back();
  switch( status )
    {
    case 1 :
    case 3 :
	if ( bonfour == 1 )
	   {
	   // printf("~~~> %s\n", elem->tag.c_str() );
	   if ( elem->tag == string("vanne") )
	      {
	      ipod = atoi(elem->attr[string("n")].c_str());
	      if   ( ( ipod < 0 ) || ( ipod >= QVAN ) )
		   gasp("fichier %s : numero vanne incorrect ligne %d",
                         fourxml.filepath, (fourxml.curlin+1) );
	      vanne[ipod].setbase( elem );	// lecture attributs de base (communs)
	      ssplit1( &vanne[ipod].src0, elem->attr[string("src0")], '+' );
	      ssplit1( &vanne[ipod].src1, elem->attr[string("src1")], '+' );
	      vanne[ipod].pix   = elem->attr[string("pix")];
	      vanne[ipod].gazx  = atoi( elem->attr[string("gazx")].c_str() );
	      vanne[ipod].gazy  = atoi( elem->attr[string("gazy")].c_str() );
	      van_num[vanne[ipod].name] = ipod;
	      break;
              }
	   if ( elem->tag == string("mfc") )
	      {
	      ipod = atoi(elem->attr[string("n")].c_str());
	      if   ( ( ipod < 0 ) || ( ipod >= QMFC ) )
		   gasp("fichier %s : numero mfc incorrect ligne %d",
                         fourxml.filepath, (fourxml.curlin+1) );
	      mfc[ipod].setbase( elem );	// lecture attributs de base (communs)
	      mfc[ipod].gazx  = atoi( elem->attr[string("gazx")].c_str() );
	      mfc[ipod].gazy  = atoi( elem->attr[string("gazy")].c_str() );
	      ssplit1( &mfc[ipod].src, elem->attr[string("src")], '+' );
	      mfc[ipod].fs    = atof( elem->attr[string("fs")].c_str() );
	      mfc[ipod].unit  = elem->attr[string("unit")];
	      mfc_num[mfc[ipod].name] = ipod;
	      break;
	      }
	   if ( elem->tag == string("tem") )
	      {
	      ipod = atoi(elem->attr[string("n")].c_str());
	      if   ( ( ipod < 0 ) || ( ipod >= QTEM ) )
		   gasp("fichier %s : numero tem incorrect ligne %d",
                         fourxml.filepath, (fourxml.curlin+1) );
	      tem[ipod].setbase( elem );	// lecture attributs de base (communs)
	      tem_num[tem[ipod].name] = ipod;
	      int ipre; string strpre = string("temp0");
	      for ( ipre = 0; ipre < QPRE; ipre++ )
		  {
		  strpre[4] = '0' + (char)ipre;
		  if   ( elem->attr.count(strpre) )
		       tem[ipod].preset[ipre] = atoi( elem->attr[strpre].c_str() );
		  }
	      break;
              }
	   if ( elem->tag == string("fre") )
	      {
	      fre.setbase( elem );	// lecture attributs de base (communs)
	      fre.fs     = atof( elem->attr[string("fs")].c_str() );
	      fre.unit   = elem->attr[string("unit")];
	      fre.offset = atof( elem->attr[string("offset")].c_str() );
	      break;
	      }
	   }
	if ( elem->tag == string("four") )
           if ( atoi(elem->attr[string("n")].c_str() ) == ifou )
              {
	      bonfour = 1;
	      nom = elem->attr[string("title")];
	      txt2ip( elem->attr[string("ip")].c_str(), destIP );
	      comm_verbose = atoi(elem->attr[string("comm_verbose")].c_str());
	      comm_log = elem->attr[string("comm_log")];
	      // xml_dir += string( SLASH );   <== marche PO
	      // xml_dir.append( 1, SLASH );   <== marche
	      xml_dir += char(SLASH);
	      xml_dir += elem->attr[string("locdir")];
	      pix_dir += char(SLASH);
	      pix_dir += elem->attr[string("locdir")];
	      plot_dir += char(SLASH);
	      plot_dir += elem->attr[string("locdir")];
	      auto_secu = elem->attr[string("auto_secu")];
              }
	if ( elem->tag == string("frevo") )
	   {
	   xml_dir = elem->attr[string("xmldir")];
	   pix_dir = elem->attr[string("pixdir")];
	   plot_dir = elem->attr[string("plotdir")];
	   xml_ver = elem->attr[string("version")];
	   text_editor = elem->attr[string("text_editor")];
	   }
	break;
    case 2 :
	if ( ( elem->tag == string("four") ) && ( bonfour == 1 ) )
	   bonfour = 2;
	break;
    case -1983 :
	gasp("%s ligne %d : element non supporte", fourxml.filepath, (fourxml.curlin+1) );
	break;
    case -1984 :
	gasp("%s ligne %d : attribut non supporte", fourxml.filepath, (fourxml.curlin+1) );
	break;
    default :
	gasp("%s ligne %d : syntaxe xml %d", fourxml.filepath, (fourxml.curlin+1), status );
    }
  }	// while status
if ( bonfour != 2 )
   gasp("four %d non decrit dans fichier %s", ifou, fourxml.filepath );
// noms predefinis (supplementaires) pour les tems :
tem_num[string("S")] = 0;
tem_num[string("C")] = 1;
tem_num[string("H")] = 2;
}

void four::scan_rec()
{
reclist.clear();

// lire repertoire sur le disque
dirdata recdir( xml_dir.c_str() );
if ( recdir.dd.size() == 0 )
   return;
recdir.tri();

unsigned int i;
recipe tmprec( this );
for ( i = 0; i < recdir.dd.size(); i++ )
    {
    string sname, sext;
    sname = string(recdir.dd[i].name);
    if ( ( recdir.dd[i].type == 'F' ) && ( sname.size() > 4 ) )
       sext.append( sname, sname.size() - 4, 4 );
    // printf("~~> %s <%s>\n", sname.c_str(), sext.c_str() );
    if	( sext == string(".xml") )
	{
	printf("~~> %s\n", sname.c_str() );
	tmprec.filename = sname;
	tmprec.load_xml();
	if ( tmprec.stat > -2 )
	   tmprec.check();
	if ( tmprec.stat > -2 )
	   tmprec.make_pack();
	reclist.push_back( recipe_summary( tmprec ) );
	if   ( reclist.back().stat > -1 )
	     printf("crc %08X\n", reclist.back().crc );
	//      printf("lu : %s %d bytes %08X\n", reclist.back().titre.c_str(), reclist.back().packlen, reclist.back().crc );
	// else printf("lu : %s\n", reclist.back().errmess.c_str() );
	}
    }
}

void four::autosync( unsigned int crc )	// identifie la recette chargee dans l'automate
{
printf("autosync...looking for %04X\n", crc );
unsigned int i;
for ( i = 0; i < reclist.size(); i++ )
    if	( ( reclist[i].crc == crc ) && ( reclist[i].stat > -2 ) )
	{
	printf("found : %s\n", reclist[i].filename.c_str() );
	recette.filename = reclist[i].filename;
	recette.load_xml();
	if ( recette.stat > -2 )
	   recette.check();
	if ( recette.stat > -2 )
	   recette.make_pack();
	if ( recette.stat > 0 )
	   recette.stat = 2;
        }
}

int four::whichspot( int x, int y )	// identifie le hot-spot ( rend index sur ppod[] )
{					// rend -1 si aucun spot
unsigned int retval = 0;
podget * p;
while ( retval < ppod.size() )
   {
   p = ppod[retval];
   if ( ( x >= p->x1 ) && ( x < p->x2 ) &&
        ( y >= p->y1 ) && ( y < p->y2 )
      )
      return(retval);
   retval++;
   }
return(-1);
}

int four::get_ipod( podget * pp )	// rend l'indice d'un podget dans son tableau specifique
{					// en fonction de son pointeur
int ipod;
switch( pp->tipo() )
   {
   // attention tres audacieuse soustraction de pointeurs - le danger
   // tient au fait qu'ils pointent sur des objets de taille differentes !!
   // un casting leve l'ambiguite - on verifie quand meme
   case 'V' :	ipod = (vodget *)pp - vanne;
		if  ( pp != &(vanne[ipod]) )
		    gasp("internal erreur #2008 ipod = %d", ipod );
		break;
   case 'M' :	ipod = (modget *)pp - mfc;
		if  ( pp != &(mfc[ipod]) )
		    gasp("internal erreur #2008 ipod = %d", ipod );
		break;
   case 'T' :	ipod = (todget *)pp - tem;
		if  ( pp != &(tem[ipod]) )
		    gasp("internal erreur #2008 ipod = %d", ipod );
		break;
   default :	ipod = 0;
   }
return( ipod );
}

// surcharge de la precedente
int four::get_ipod( int i )	// rend l'indice d'un podget dans son tableau specifique
{				// en fonction de son indice dans ppod[]
int ipod;
if ( ( i < 0 ) || ( i >= (int)ppod.size() ) )
   gasp("internal erreur #2008 i = %d", i );
ipod = get_ipod( ppod[i] );
return( ipod );
}

// methodes du podget de base ------------------------------------ //

void podget::setbase( xelem * elem )	// lecture attributs de base (communs)
{
name  = elem->attr[string("txt")];
x     = atoi( elem->attr[string("x")].c_str() );
y     = atoi( elem->attr[string("y")].c_str() );
pixx  = atoi( elem->attr[string("pixx")].c_str() );
pixy  = atoi( elem->attr[string("pixy")].c_str() );
x1 = pixx - 20; x2 = pixx + 20;
y1 = pixy - 20; y2 = pixy + 20;
}

// methodes du mfc ou tem 'modget' ------------------------------------ //

/* il s'agit de conversions entre
	- texte (txt) ou unites utilisateur (uiu) toujours en double
	- unites automate (pcu) toujours en 16 bits

types : 'v' volts
	'd' degres ou unites DAC
	'u' unites utilisateurs
	'p' pourcent
 */

int modget::txt2pcu( string s )
{
char * tail; double val;
val = strtod( s.c_str(), &tail );
// val = std::strtod( s.c_str(), &tail );
// printf("txt2pcu %s --> val %g\n", s.c_str(), val );
if ( tail[0] == 0 )	// defaut
   return( uiu2pcu( val, 'd' ) );
if ( tail[0] != ' ' )	// espace obligatoire
   return(-1);
switch( tail[1] )
   {
   case 'd' : return( uiu2pcu( val, 'd' ) );	break;
   case 'v' : return( uiu2pcu( val, 'v' ) );	break;
   case '%' : return( uiu2pcu( val, 'p' ) );	break;
   case 'u' : return( uiu2pcu( val, 'u' ) );	break;
   case 'f' : return( uiu2pcu( val, 'f' ) );	break;
   default  : return( -1 );			break;
   }
return(-1);
}

// DAC full-scale = 0xFFF0 soit  65520
// pour les DAC on calcule sur 12 bits puis on decale a gauche
// pour eviter d'avoir de l'info inutilisee dans les 4 LSBs
// meme chose pour les temperatures.
// les seuls cas ou les LSBs sont utiles sont les types 'd' (pour les increments de rampe) et 'f' (V2F)
// ce calcul rend equiprobable les 4096 codes pour les valeurs d'entree
// prises sur l'intervalle [0,fs[,
// cependant la valeur fs est marginale et necessite un if
int modget::uiu2pcu( double sv, char type='d' )
{
int retval = (int)sv;	// reponse par defaut (utile ??)
switch( type )
   {
   // les 3 cas DAC bases sur une valeur de Full Scale 5.0, 100.0 ou fs
   case 'v' : if   ( sv >= 5.0 )
		   retval = 0xFFF;
	      else retval = (int)floor( sv * ( 4096.0 / 5.0 ) );
	      retval <<= 4;					break;
   case 'p' : if   ( sv >= 100.0 )
		   retval = 0xFFF;
	      else retval = (int)floor( sv * ( 4096.0 / 100.0 ) );
	      retval <<= 4;					break;
   case 'u' :  if   ( sv >= fs )
		   retval = 0xFFF;
	      else retval = (int)floor( sv * ( 4096.0 / fs ) );
	      retval <<= 4;					break;
   // la temperature
   case 'd' : retval = (int)floor( sv * 16.0 );			break;
   // le V/F
   case 'f' : retval = (int)round( sv * (65535.0/fs) );		break;
   }
// printf("uiu2pcu sv = %g, fs = %g, retval = %d\n", sv, fs, retval );
if ( retval < 0 ) retval = 0;
if ( retval > 0xFFFF ) retval = 0xFFFF;
return( retval );
}

// ADC full-scale = 0xFFC0 soit 65472
// ADC 1/2 LSB = 32
// ici type en majuscule pour ADC
double modget::pcu2uiu( int pcuval, char type='d' )
{
double val = (double)pcuval;	// reponse par defaut (utile ??)
switch( type )
   {
   // les DACs 12 bits : on ajoute un demi lsb pour l'exactitude statistique
   // sauf pour zero (on n'ajoute pas) et fs (on ajoute 1 lsb) pour le look
   // ceci ne doit pas porter atteinte a la reversibilite
   case 'v' : if ( pcuval > 0 ) pcuval += 8;		// demi lsb
	      if ( pcuval >= 0xFFF8 ) pcuval = 0x10000;	// pile fs
	      val = ((double)pcuval)*(5.0/65536.0);	break;
   case 'p' : if ( pcuval > 0 ) pcuval += 8;		// demi lsb
	      if ( pcuval >= 0xFFF8 ) pcuval = 0x10000;	// pile fs
	      val = ((double)pcuval)*(100.0/65536.0);	break;
   case 'u' : if ( pcuval > 0 ) pcuval += 8;		// demi lsb
	      if ( pcuval >= 0xFFF8 ) pcuval = 0x10000;	// pile fs
	      val = ((double)pcuval)*(fs/65536.0);	break;
   // la temperature
   case 'd' : val = ((double)pcuval)*(1.0/16.0);	break;
   // le V/F
   case 'f' : val = ((double)pcuval)*(fs/65535.0);	break;
   // les ADCS 10 bits : on ajoute un demi lsb pour l'exactitude statistique
   // sauf pour zero (on n'ajoute pas) et fs (on ajoute 1 lsb) pour le look
   case 'V' : if ( pcuval > 0 ) pcuval += 0x20;		// demi lsb
	      if ( pcuval >= 0xFFE0 ) pcuval = 0x10000;	// pile fs
	      val = ((double)pcuval)*(5.0/65536.0);	break;
   case 'P' : if ( pcuval > 0 ) pcuval += 0x20;		// demi lsb
	      if ( pcuval >= 0xFFE0 ) pcuval = 0x10000;	// pile fs
	      val = ((double)pcuval)*(100.0/65536.0);	break;
   case 'U' : if ( pcuval > 0 ) pcuval += 0x20;		// demi lsb
	      if ( pcuval >= 0xFFE0 ) pcuval = 0x10000;	// pile fs
	      val = ((double)pcuval)*(fs/65536.0);	break;
   }
return( val );
}

void modget::pcu2stream( ostream & obuf, int pcuval, char type='d' )
{
double val = pcu2uiu( pcuval, type );
switch( type )
   {
   case 'v' :
   case 'V' : obuf << fixed << right << setprecision(3) << setw(6) << val << " V    "; break;
   case 'p' :
   case 'P' : obuf << fixed << right << setprecision(1) << setw(6) << val << " %    "; break;
   case 'f' :
   case 'u' :
   case 'U' : obuf << fixed << right << setprecision(2) << setw(6) << val << " "
		            << left  << setw(5) << unit; break;
   case 'd' : obuf << fixed << right << setprecision(1) << setw(6) << val << " °C"; break;
   default  : obuf << left << val;
   }
}


// methodes de la recette "recipe" ------------------------------------- //

void recipe::init()
{
int istep;
// effacer tout, sauf filename !
for ( istep = 0; istep < 256; istep++ )
    step[istep].init();
qstep = 0; packlen = 0;
stat = -1;
}

void recipe::errtxt( const char * txt )
{
ostringstream oserr;
oserr << txt << ", ligne " << errlin;
errmess = oserr.str();
stat = -2;
}

// cette methode utilise le membre filename comme short path
void recipe::load_xml()	// chercher dans le repertoire de recettes du tube
{
string fullpath = ptube->xml_dir + char(SLASH) + filename;
load_xml( fullpath );
}

/* phases du parsing = profondeur de hierarchie = recxml.stac.size()
	0	idle
	1	dans recette
	2	dans step
	3	dans vannes
	3	dans podget MFC, TEM ou FRE
 */

void recipe::load_xml( string fullpath )
{
int istep=0; int ipod; epod * pepod; modget * ppod;

init();

// ouvrir fichier
xmlobj recxml( fullpath.c_str(), &dtd );
if ( !(recxml.is) )
   {
   ostringstream oserr;
   oserr << "echec ouverture fichier " << fullpath;
   errmess = oserr.str();
   stat = -2; return;
   }

// parsing
int status; xelem * elem; string s;
/* status :
   1 nouvel element : fin de start-tag (tous attributs inclus, contenu exclu)
     accessible au sommet de la pile (this->stac)
   2 fin de end-tag de l'element courant
     (encore accessible au sommet de la pile, mais en instance d'etre depile)
   3 fin d'empty element tag (tous attributs inclus) accessible au sommet de la pile
   9 EOF
   <0 si erreur
 */
while ( ( status = recxml.step() ) != 9 )
  {
  elem = &recxml.stac.back(); errlin = recxml.curlin + 1;
  switch( status )
    {
    case 1 :
    case 3 :
	if ( recxml.stac.size() == 1 )
	   {
	   if ( elem->tag != string("recette") )
	      { errtxt("recette attendue"); return; }
           if ( atoi(elem->attr[string("four")].c_str() ) != ptube->ifou )
	      { errtxt("recette n'est pas pour ce tube"); return; }
	   titre = elem->attr[string("titre")];
	   break;
	   }
	if ( recxml.stac.size() == 2 )
	   {
	   if ( elem->tag != string("step") )
	      { errtxt("step attendu"); return; }
	   s = elem->attr[string("n")];
	   if ( s.size() == 0 )
	      { errtxt("step n'a pas de num"); return; }
	   istep = atoi(s.c_str());
	   if ( ( istep <= 0 ) || ( istep > 255 ) )
	      { errtxt("numero de step illegal"); return; }
	   if ( step[istep].existe )
	      { errtxt("numero de step duplique"); return; }
	   step[istep].existe = 1;
	   s = elem->attr[string("deldg")];
	   if ( s.size() )
	      step[istep].deldg = atoi(s.c_str());
	   s = elem->attr[string("duree")];
	   if ( s.size() )
	      step[istep].duree = atoi(s.c_str());
	   s = elem->attr[string("saut")];
	   if ( s.size() )
	      step[istep].stogo = atoi(s.c_str());
	   s = elem->attr[string("secustat")];
	   if ( s.size() )
	      step[istep].secstat = atoi(s.c_str());
	   step[istep].titre = elem->attr[string("titre")];
	   break;
	   }
	if ( recxml.stac.size() == 3 )
	   {
	   pepod = NULL; ppod = NULL; ipod = -1;
	   if   ( elem->tag == string("mfc") )
		{
		s = elem->attr[string("n")];
		if ( s.size() == 0 )
		   { errtxt("mfc n'a pas de nom ou numero"); return; }
		if   ( ( s[0] >= '0' ) && ( s[0] <= '9' ) )
		     ipod = atoi( s.c_str() );
		else if   ( ptube->mfc_num.count(s) )
			  ipod = ptube->mfc_num[s];
		     else ipod = -1;
		if ( ( ipod < 0 ) || ( ipod >= QMFC ) )
		   { errtxt("mfc n'existe pas"); return; }
		pepod = &step[istep].mfc[ipod];
		ppod  = &ptube->mfc[ipod];
		} else
	   if   ( elem->tag == string("tem") )
		{
		s = elem->attr[string("n")];
		if ( s.size() == 0 )
		   { errtxt("tem n'a pas de nom ou numero"); return; }
		if   ( ( s[0] >= '0' ) && ( s[0] <= '9' ) )
		     ipod = atoi( s.c_str() );
		else if   ( ptube->tem_num.count(s) )
			  ipod = ptube->tem_num[s];
		     else ipod = -1;
		if ( ( ipod < 0 ) || ( ipod >= QTEM ) )
		   { errtxt("tem n'existe pas"); return; }
		pepod = &step[istep].tem[ipod];
		ppod  = &ptube->tem[ipod];
		} else
	   if   ( elem->tag == string("fre") )
		{
		pepod = &step[istep].fre;
		ppod  = &ptube->fre;
		}
	   if	( pepod )	// traitement commun aux mfcs, tems et fre
		{
		s = elem->attr[string("SV")];
		if ( s.size() )
		   if ( ( pepod->SV = ppod->txt2pcu(s) ) < 0 )
		      { errtxt("valeur SV illisible"); return; }
		s  = elem->attr[string("SVmi")];
		if ( s.size() == 0 )
		   s = elem->attr[string("SVinc")];	// SVinc est un alias de SVmi pour les rampes
		if ( s.size() )
		   if ( ( pepod->SVmi = ppod->txt2pcu(s) ) < 0 )
		      { errtxt("valeur SVmi illisible"); return; }
		s = elem->attr[string("SVma")];
		if ( s.size() == 0 )
		   s = elem->attr[string("SVdec")];	// SVdec est un alias de SVma pour les rampes
		if ( s.size() )
		   if ( ( pepod->SVma = ppod->txt2pcu(s) ) < 0 )
		      { errtxt("valeur SVma illisible"); return; }
		s = elem->attr[string("check")];
		if ( s.size() == 0 )
		   s = elem->attr[string("rampe")];	// rampe est un alias de check
		if ( s.size() )
		   {
		   if   ( ptube->epod_flags.count(s) )
			pepod->flags = ptube->epod_flags[s];
		   else { errtxt("valeur flags opt illegale"); return; }
		   }
		s = elem->attr[string("saut")];
		if ( s.size() )
		   {
		   pepod->stogo = atoi( s.c_str() );
		   if ( ( pepod->stogo < 0 ) || ( pepod->stogo > 255 ) )
		      { errtxt("saut illegal"); return; }
		   }
		// printf("podget %d %s --> SV=%d flags=%02X stogo=%d\n",
		//	 ipod, ppod->name.c_str(), pepod->SV, pepod->flags, pepod->stogo );
		break;
		}
	   if   ( elem->tag == string("vanne") )
		{
		s = elem->attr[string("n")];
		if ( s.size() == 0 )
		   { errtxt("vanne n'a pas de nom ou numero"); return; }
		if   ( ( s[0] >= '0' ) && ( s[0] <= '9' ) )
		     ipod = atoi( s.c_str() );
		else if   ( ptube->van_num.count(s) )
			  ipod = ptube->van_num[s];
		     else ipod = -1;
		if ( ( ipod < 0 ) || ( ipod >= QVAN ) )
		   { errtxt("vanne n'existe pas"); return; }
		if ( step[istep].vannes < 0 )
		   step[istep].vannes = 0;
		step[istep].vannes |= ( 1 << ipod );
		// printf("step %d, vannes %04X\n", istep, step[istep].vannes );
		break;
		}
	   { errtxt("mfc ou tem ou fre ou vanne attendu"); return; }
	   }
	break;
    case 2 :
	break;
    case -1983 : { errtxt("element non supporte"); return; }
	break;
    case -1984 : { errtxt("attribut non supporte"); return; }
	break;
    default :
	{ errtxt("erreur de syntaxe xml"); return; }
    }
  }	// while status
stat = 0;
}

// verifier la coherence du contenu de la recette
void recipe::check()
{
int istep, ipod, igo;
etape * pstep;

for ( istep = 1; istep < 256; istep ++ )
    {
    pstep = &step[istep];
    if	( pstep->existe )
	{
	igo = pstep->stogo;
	if ( igo > 0 )
	   if	( step[igo].existe == 0 )
		{
		ostringstream oserr;
		oserr << "step " << istep << " : destination du saut n'existe pas";
		errmess = oserr.str(); stat = -2; return;
		}
	if ( igo < 0 )
	   if   ( ( (istep==255)?(1):(step[istep+1].existe==0) ) && ( pstep->duree >= 0 ) )
		{
		ostringstream oserr;
		oserr << "step " << istep << " : n'a pas de successeur et n'est pas en pause";
		errmess = oserr.str(); stat = -2; return;
		}
	for ( ipod = 0; ipod < QMFC; ipod++ )
	    {
	    igo = pstep->mfc[ipod].stogo;
	    if	( ( pstep->mfc[ipod].flags > 0 ) && ( igo < 0 ) )
		{
		ostringstream oserr;
		oserr << "step " << istep << ", mfc " << ipod << " : manque destination du saut";
		errmess = oserr.str(); stat = -2; return;
		}
	    if  ( igo > 0 )
		if ( step[igo].existe == 0 )
		   {
		   ostringstream oserr;
		   oserr << "step " << istep << ", mfc " << ipod << " : destination du saut n'existe pas";
		   errmess = oserr.str(); stat = -2; return;
		   }
	    }
	for ( ipod = 0; ipod < QTEM; ipod++ )
	    {
	    igo = pstep->tem[ipod].stogo;
	    if	( ( pstep->tem[ipod].flags > 0 ) && ( igo < 0 ) )
		{
		ostringstream oserr;
		oserr << "step " << istep << ", tem " << ipod << " : manque destination du saut";
		errmess = oserr.str(); stat = -2; return;
		}
	    if  ( igo > 0 )
		if ( step[igo].existe == 0 )
		   {
		   ostringstream oserr;
		   oserr << "step " << istep << ", tem " << ipod << " : destination du saut n'existe pas";
		   errmess = oserr.str(); stat = -2; return;
		   }
	    }
	igo = pstep->fre.stogo;
	    if	( ( pstep->fre.flags > 0 ) && ( igo < 0 ) )
		{
		ostringstream oserr;
		oserr << "step " << istep << ", fre " << ipod << " : manque destination du saut";
		errmess = oserr.str(); stat = -2; return;
		}
	if  ( igo > 0 )
	    if  ( step[igo].existe == 0 )
		{
		ostringstream oserr;
		oserr << "step " << istep << ", fre : destination du saut n'existe pas";
		errmess = oserr.str(); stat = -2; return;
		}
	}
    }
}

// comprimer la recette en format automate
void recipe::make_pack()
{
int istep, i, ipod; epod * pepod;
int adr = 3; qstep = 0;
for ( istep = 1; istep < 256; istep ++ )
    if	( step[istep].existe )
	{
	pack[adr++] = 0;
	pack[adr++] = istep;
	pack[adr++] = step[istep].deldg;
	if ( step[istep].duree >= 0 )
	   {
	   pack[adr++] = 0x01;
	   pack[adr++] = (unsigned char)step[istep].duree;
	   pack[adr++] = (unsigned char)(step[istep].duree >> 8);
	   }
	if ( step[istep].vannes > 0 )	// 8.1n : si vannes==0, pas besoin de l'encoder dans le pack
	   {
	   pack[adr++] = 0x02;
	   pack[adr++] = (unsigned char)step[istep].vannes;
	   pack[adr++] = (unsigned char)(step[istep].vannes >> 8);
	   }
	if ( step[istep].stogo >= 0 )
	   {
	   pack[adr++] = 0x03;
	   pack[adr++] = (unsigned char)step[istep].stogo;
	   pack[adr++] = 0;	// unused
	   }
	for ( i = 0; i < (QMFC+QTEM+1); i++ )
	    {
	    ipod = i;
	    if   ( ipod < QMFC )
	         pepod = &step[istep].mfc[ipod];
	    else {
	         ipod -= QMFC;
		 if   ( ipod < QTEM )
		      pepod = &step[istep].tem[ipod];
		 else pepod = &step[istep].fre;
		 }
	    if	( pepod->SV >=0 )
		{
		pack[adr++] = (i+1) << 4;
		pack[adr++] = (unsigned char)pepod->SV;
		pack[adr++] = (unsigned char)(pepod->SV >> 8);
		}
	    if	( pepod->SVmi >=0 )
		{
		pack[adr++] = ( (i+1) << 4 ) + 1;
		pack[adr++] = (unsigned char)pepod->SVmi;
		pack[adr++] = (unsigned char)(pepod->SVmi >> 8);
		}
	    if	( pepod->SVma >=0 )
		{
		pack[adr++] = ( (i+1) << 4 ) + 2;
		pack[adr++] = (unsigned char)pepod->SVma;
		pack[adr++] = (unsigned char)(pepod->SVma >> 8);
		}
	    if	( pepod->flags >= 0 )
		{
		pack[adr++] = ( (i+1) << 4 ) + 3;
		pack[adr++] = (unsigned char)pepod->flags;
		pack[adr++] = (unsigned char)pepod->stogo;
		}
	    }	// for i
	qstep++;
	}	// if existe et for istep
pack[0] = qstep;
packlen = adr;
pack[1] = (unsigned char)packlen;
pack[2] = (unsigned char)(packlen >> 8);
crc = icrc32( pack, packlen );
stat = 1;
}

/* dump d'une recette comprimee dans 1 string recipe::dump
	- clusters de 3 bytes
	- premier cluster :
		- nombre de steps (step 0 exclu) sur 1 byte
		- taille recette en bytes, sur 2 bytes
	- autres clusters :
		- 1 byte d'identification :
			- 4 MSBs = index podget
			- 4 LSBs = index du parametre dans le podget
		- 2 bytes de data
   reference : la fonction sauter() de PIC/PROC/recipe.c
   recipe.h et aussi process.h pour la definition des flags MICEN, MACEN
   et RAMPEN et de leurs combinaisons
 */
void recipe::dump_pack()
{
ostringstream odum;
odum << "Tube " << ptube->ifou << ", \"" << ptube->nom << "\", fichier " << filename << endl;
odum << titre << endl;
unsigned int packlen, adr, ipar, ipod, val16;
packlen =  pack[2] << 8;
packlen |= pack[1];
odum << "pack de " << (unsigned int)(pack[0]) << " steps, " << packlen << " bytes, CRC="
     << hex << setfill('0') << setw(8) << crc << endl;
adr = 3;
while ( adr < packlen )
   {
   odum << hex << setfill('0')
	<< setw(2) << (unsigned int)(pack[adr]) << " "
	<< setw(2) << (unsigned int)(pack[adr+1]) << " "
	<< setw(2) << (unsigned int)(pack[adr+2]) << "  " << setfill(' ') << dec;
   ipar = pack[adr] & 0x0F;
   ipod = pack[adr] >> 4;
   val16 = pack[adr+1] | ( pack[adr+2] << 8 );
   if   ( ipod == 0 )	// parametres du step entier
	switch ( ipar )
	   {
	   case 0 : odum << "-------- STEP " << setw(3) << (unsigned int)(pack[adr+1]);
		    if   ( pack[adr+2] )
			 odum << " ------ delai de grace = " << setw(3)
			      << (unsigned int)(pack[adr+2]) << " ---";
		    else odum << " -------------------------------";
		    break;
	   case 1 : odum << "duree " << val16 << " s.";
		    break;
	   case 2 : odum << "vannes " << hex << setfill('0') << setw(4) << val16
			 << setfill(' ') << dec;
		    break;
	   case 3 : odum << "step to go " << (unsigned int)(pack[adr+1]);
		    break;
	   default : gasp("pack recette corrompu");
	   }
   else {
	ipod--;			// vu qu'il y a trois sortes de podget
	if   ( ipod < QMFC )	// on ajuste ipod selon.
	     odum << "MFC " << ipod << " ";
	else {
	     ipod -= QMFC;
	     if   ( ipod < QTEM )
		  odum << "TEM " << ipod << " ";
	     else if   ( ipod == QTEM )
		       odum << "FRE   ";
		  else gasp("pack recette corrompu");
	     }
	switch ( ipar )
	   {
	   case 0 : odum << "SV   "; break;
	   case 1 : odum << "SVmi "; break;
	   case 2 : odum << "SVma "; break;
	   }
	switch ( ipar )
	   {
	   case 0 :
	   case 1 :
	   case 2 : odum << setw(5) << val16 << " --> " << setw(4) << (val16 >> 4);
		    ipod = pack[adr] >> 4;
		    if  ( ( ipod < QMFC ) || ( ipod == QTEM ) )	// eviter TEM
			odum << " (" << setprecision(1) << fixed << (((double)val16)/655.20) << "%)";
		    break;
	   case 3 : if   ( pack[adr+1] & RAMPEN )
			 {
			 if   ( pack[adr+1] & MACEN )
			      odum << "rampe montante, ";
			 else if   ( pack[adr+1] & MICEN )
				   odum << "rampe descendante, ";
			 }
		    else {
			 if   ( pack[adr+1] & MICEN )
			      odum << "min check, ";
			 if   ( pack[adr+1] & MACEN )
			      odum << "max check, ";
			 }
		    odum << "step to go " << (unsigned int)(pack[adr+2]);
		    break;
	   default : gasp("pack recette corrompu");
	   }
	}
   odum << endl;
   adr += 3;
   }
dump = odum.str();
}

// recodage de la recette en xml
void recipe::make_xml( FILE * xfil )
{
int istep, i, vv, cnt; epod * pepod; modget * pemod;
fprintf( xfil, "<recette four=\"%d\" titre=\"%s\" >\n", ptube->ifou, titre.c_str() );
for	( istep = 1; istep < 256; istep ++ )
	{
	if	( step[istep].existe )
		{
		// <step n="26" duree="120" deldg="60" titre="Regu vide" secustat="1" >
		fprintf( xfil, "<step n=\"%d\" ", istep );
		if	( step[istep].deldg > 0 )
			fprintf( xfil, "deldg=\"%d\" ", step[istep].deldg );
		if	( step[istep].duree > 0 )
			fprintf( xfil, "duree=\"%d\" ", step[istep].duree );
		fprintf( xfil, "titre=\"%s\" ", step[istep].titre.c_str() );
		if	( step[istep].secstat >= 0 )
			fprintf( xfil, "secustat=\"%d\" ", step[istep].secstat );
		if	( step[istep].stogo >= 0 )
			fprintf( xfil, "saut=\"%d\" ", step[istep].stogo );
		fprintf( xfil, ">\n" );
		vv = step[istep].vannes; cnt = 0;
		for	( i = 0; i < 16; i++ )
			{
			if	( vv & 1 )
				{
				fprintf( xfil, "<vanne n=\"%s\" /> ", ptube->vanne[i].name.c_str() );
				if	( ++cnt >= 4 )
					{ cnt = 0; fprintf( xfil, "\n" ); }
				}
			vv >>= 1;
			}
		if	(cnt)
			fprintf( xfil, "\n" );
		for	( i = 0; i < QMFC; i++ )
			{
			pepod = &step[istep].mfc[i];
			pemod = &ptube->mfc[i];
			podget2xml( xfil, pepod, pemod, "mfc", 'u' );
			}	// for i
		for	( i = 0; i < QTEM; i++ )
			{
			pepod = &step[istep].tem[i];
			pemod = &ptube->tem[i];
			podget2xml( xfil, pepod, pemod, "tem", 'd' );
			}	// for i
		pepod = &step[istep].fre;
		pemod = &ptube->fre;
		podget2xml( xfil, pepod, pemod, "fre", 'f' );
		fprintf( xfil, "</step>\n" );
		}	// if existe
	} // for istep
fprintf( xfil, "</recette>\n" );
}

// recodage d'un podget en xml
void recipe::podget2xml( FILE * xfil, epod * pepod, modget * pemod, const char * prefix, char type )
{
if	(!( ( pepod->SV >=0 ) || ( pepod->SVmi >=0 ) || ( pepod->SVma >=0 ) || ( pepod->flags >= 0 ) ) )
	return;	// nothing to do
fprintf( xfil, "\t<%s n=\"%s\" ", prefix, pemod->name.c_str() );
if	( pepod->SV >=0 )
	fprintf( xfil, "SV=\"%g %c\" ", pemod->pcu2uiu( pepod->SV, type ), type );
if	( pepod->flags > 0 )
	{
	const char * SVmi_alias = "SVmi";
	const char * SVma_alias = "SVma";
	if	( ( ( pepod->flags & RAMPEN ) == 0 ) && ( pepod->flags & (MICEN|MACEN) ) )
		{
		fprintf( xfil, "check=\"" );
		if	( pepod->flags & MICEN )
			fprintf( xfil, "min" );
		if	( pepod->flags & MACEN )
			fprintf( xfil, "max" );
		fprintf( xfil, "\" " );
		}
	if	( pepod->flags & RAMPEN )
		{
		if	( pepod->flags & MACEN )
			{
			SVmi_alias = "SVinc";
			fprintf( xfil, "rampe=\"montee\" " );
			}
		if	( pepod->flags & MICEN )
			{
			SVma_alias = "SVdec";
			fprintf( xfil, "rampe=\"descente\" " );
			}
		}
	if	( pepod->SVmi >=0 )
		fprintf( xfil, "%s=\"%g %c\" ", SVmi_alias, pemod->pcu2uiu( pepod->SVmi, type ), type );
	if	( pepod->SVma >=0 )
		fprintf( xfil, "%s=\"%g %c\" ", SVma_alias, pemod->pcu2uiu( pepod->SVma, type ), type );
	if	( pepod->stogo >= 0 )
		fprintf( xfil, "saut=\"%d\" ", pepod->stogo );
	}
fprintf( xfil, "/>\n" );
}

// methodes du step "etape" ------------------------------------- //

/* ATTENTION : initialisation recette avant lecture XML
- en general valeur -1 signifie parametre non specifie dans la recette xml donc
  pouvant etre omis dans le pack,
  sa valeur sera determinee par l'automate,
	- soit par heritage du step precedent,
	- soit par application d'une valeur par defaut
- exceptions a la regle ci-dessus :
	- deldg : toujours encode dans le pack, 0 si omis dans le xml
	- vannes  : non specifie dans la recette xml implique valeur zero
	  toutefois ce zero ne sera jamais encode dans le pack, car dans ce cas
	  l'automate mettra toujours zero (depuis Frevo 7.9b).
N.B. ici -1, c'est -1 sur 32 bits soit 0xffffffff, la valeur 0xffff est legale
pour certains parametres (par exemple duree)
 */

void etape::init()
{
int ipod;
for ( ipod = 0; ipod < QMFC; ipod++ )
    mfc[ipod].init();
for ( ipod = 0; ipod < QTEM; ipod++ )
    tem[ipod].init();
fre.init();
existe = 0; deldg = 0; duree = -1; vannes = 0; stogo = -1, secstat = -1;
}

// methodes de l'etat du podget "epod"
void epod::init()
{ SV = -1; SVmi = -1; SVma = -1; flags = -1; stogo = -1;  }


// fonctions utilitaires non-membres --------------------------- //

// fonction de formatage du temps vers un stream
void stream_time( ostream & outstr, time_t * pt, char separator )
{
struct tm *t;
t = localtime( pt );
outstr	<< setfill('0')
	<< setw(4) << (unsigned int)(t->tm_year+1900) << separator
	<< setw(2) << (unsigned int)(t->tm_mon+1) << separator
	<< setw(2) << (unsigned int)(t->tm_mday) << separator
	<< setw(2) << (unsigned int)(t->tm_hour) << 'h'
	<< setw(2) << (unsigned int)(t->tm_min) << 'm'
	<< setw(2) << (unsigned int)(t->tm_sec);
}

// interpretation ip de la forme 192.168.1.80
void txt2ip( const char *txt, unsigned char * IP )
{
int i0=0, i1=0, i2=0, i3=0;
sscanf( txt, "%d.%d.%d.%d", &i0, &i1, &i2, &i3 );
IP[0] = (unsigned char)i0; IP[1] = (unsigned char)i1;
IP[2] = (unsigned char)i2; IP[3] = (unsigned char)i3;
}

// une fonction qui coupe la string 'strin' en morceaux selon le
// delimiteur 'deli', et range les morceaux non vides dans le vector 'splut'
// qui doit exister. rend le nombre de morceaux.
// ici deli est 1 char
int ssplit1( vector <string> * splut, string strin, char deli )
{
unsigned int pos, cnt; string part;

pos = 0; cnt = 0;
while ( pos < strin.size() )
   {
   if   ( strin[pos] == deli )
	{
	if ( part.size() )
	   {
	   splut->push_back( part );
	   part = string("");
	   cnt++;
	   }
	}
   else part += strin[pos];
   pos++;
   }
if ( part.size() )
   {
   splut->push_back( part );
   part = string("");
   cnt++;
   }
return cnt;
}
