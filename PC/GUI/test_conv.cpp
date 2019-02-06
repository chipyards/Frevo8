// test des methodes de conversion pcu <--> uiu de process.cpp
/* N.B. process.o a besoin de xmlpb.o dirlist.o crc32.o
g++ -Wall -Wno-uninitialized -c -O3 ../xmlpb.cpp
g++ -Wall -c -O3 ../dirlist.cpp
gcc -Wall -c -O3 ../crc32.c
g++ -Wall -c -O3 ../process.cpp
g++ -Wall -c -O3 test_conv.cpp
g++ -o test_conv.exe xmlpb.o dirlist.o crc32.o process.o test_conv.o
*/

/* extrait de la classe 
class modget : public podget {
public :
double fs;
string unit;
modget() : fs(1.0), unit(""), gazx(0), gazy(0) {};
// methodes de conversion
int txt2pcu( string s );
int uiu2pcu( double sv, char type );
double pcu2uiu( int pcuval, char type );
void pcu2stream( ostream & obuf, int pcuval, char type );
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include "../../version.h"
#include "../xmlpb.h"
#include "../frevo_dtd.h"
#include "../process.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

extern "C" void gasp( char *fmt, ... )  /* fatal error handling */
{
  va_list  argptr;
  fprintf( stderr, "\nSTOP : " );
  va_start( argptr, fmt );
  vfprintf( stderr, fmt, argptr );
  va_end( argptr );
  fprintf( stderr, "\n" );
  exit(1);
}

int verbose = 0;

void excheckDAC( double fs, char typ )
{
modget moo;
moo.fs = fs;
// on etudie la sequence pcu --> uiu --> pcu --> uiu
//			  i0      d1      i2      d3
int i0, i2;
double d1, d3;
int good = 0;
int bad = 0;
cout <<  "test exhaustif type " << typ << " fs=" << fs << "\n";
for ( i0 = 0; i0 <= 0xFFF0; i0 += 0x10 )
    {
    i0 &= 0xFFF0;
    d1 = moo.pcu2uiu( i0, typ );
    i2 = moo.uiu2pcu( d1, typ );
    d3 = moo.pcu2uiu( i2, typ );
    if  ( i2 != i0 )
	{
	cout << "err #1 " << i0 << " " << d1 << " " << i2 << " " << d3 << "\n";
	bad++;
	}
    else
    if  ( d3 != d1 )
	{
	cout << "err #2 " << i0 << " " << d1 << " " << i2 << " " << d3 << "\n";
	bad++;
	}
    else good++;
    if ( verbose ) cout << i0 << " " << d1 << " " << i2 << " " << d3 << "\n";
    }
cout << good << " good, " << bad << " bad\n";
}

void excheckADC( double fs, char typ )
{
char ADCtyp;
switch ( typ )
   {
   case 'v' : ADCtyp = 'V'; break;
   case 'p' : ADCtyp = 'P'; break;
   case 'u' : ADCtyp = 'U'; break;
   default : ADCtyp = typ;
   }

modget moo;
moo.fs = fs;
// on etudie la sequence pcu --> uiu --> pcu --> uiu
//			  i0      d1      i2      d3
int i0, i2;
double d1, d3;
int good = 0;
int bad = 0;
cout <<  "test exhaustif type " << ADCtyp << " fs=" << fs << "\n";
for ( i0 = 0; i0 <= 0xFFC0; i0 += 0x40 )
    {
    i0 &= 0xFFC0;
    d1 = moo.pcu2uiu( i0, ADCtyp );
    i2 = moo.uiu2pcu( d1, typ );
    i2 &= 0xFFC0;
    d3 = moo.pcu2uiu( i2, ADCtyp );
    if  ( i2 != i0 )
	{
	cout << "err #1 " << i0 << " " << d1 << " " << i2 << " " << d3 << "\n";
	bad++;
	}
    else
    if  ( d3 != d1 )
	{
	cout << "err #2 " << i0 << " " << d1 << " " << i2 << " " << d3 << "\n";
	bad++;
	}
    else good++;
    if ( verbose ) cout << i0 << " " << d1 << " " << i2 << " " << d3 << "\n";
    }
cout << good << " good, " << bad << " bad\n";
}

int main( int argc, char ** argv )
{
excheckDAC( 1.0, 'v' );
excheckDAC( 1.0, 'p' );
excheckDAC( 1.0, 'u' );
excheckDAC( 20.0, 'u' );
excheckDAC( 50.0, 'u' );
excheckDAC( 0.1, 'u' );
excheckDAC( 1.0, 'd' );
excheckDAC( 1.0, 'f' );
excheckDAC( 19530.96, 'f' );
excheckDAC( 0.8754217, 'u' );
excheckDAC( 1.23456789, 'u' );

excheckADC( 1.0, 'v' );
excheckADC( 1.0, 'p' );
excheckADC( 1.0, 'u' );
excheckADC( 100.0, 'u' );
excheckADC( 50.0, 'u' );
excheckADC( 0.2, 'u' );
excheckADC( 0.1, 'u' );

verbose = 1;
excheckDAC( 300.0, 'u' );


    modget moo;
    moo.fs = 300.0;
    int i0, i2;
    double d1, d3;
    d1 = 50.0;
    i2 = moo.uiu2pcu( d1, 'u' );
    i2 &= 0xFFF0;
    d3 = moo.pcu2uiu( i2, 'u' );
    cout << "test ponctuel fs = 300.0 " << d1 << " " << i2 << " " << d3 << "\n";

    string s = string( "50 u" );
    i2 = moo.txt2pcu( s );
    d3 = moo.pcu2uiu( i2, 'u' );
    cout << "test ponctuel fs = 300.0 \"" << s << "\" " << i2 << " " << d3 << "\n";

return 0;
}
