/**********************************************************************
 *<
	FILE: jagtypes.h

	DESCRIPTION:  Typedefs for general jaguar types.

	CREATED BY: Rolf Berteig

	HISTORY: created 19 November 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __JAGTYPES__
#define __JAGTYPES__

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef uchar UBYTE;
typedef unsigned short USHORT;
typedef unsigned short UWORD;
														 
struct Color24 { 
	uchar r,g,b;
	};

struct Color48 { 
	UWORD r,g,b;
	};

struct Color64 { 
	UWORD r,g,b,a;
	};

//-- Pixel storage classes used by BitmapManager ----------------------------------------------------

typedef struct {
   BYTE r,g,b;
} BMM_Color_24;

typedef struct {
   BYTE r,g,b,a;
} BMM_Color_32;

typedef struct {
   WORD r,g,b;
} BMM_Color_48;

typedef struct {
   WORD r,g,b,a;
} BMM_Color_64;

/* Time:
 */
typedef int TimeValue;

#define TIME_TICKSPERSEC	4800

#define TicksToSec( ticks ) ((float)(ticks)/(float)TIME_TICKSPERSEC)
#define SecToTicks( secs ) ((TimeValue)(secs*TIME_TICKSPERSEC))
#define TicksSecToTime( ticks, secs ) ( (TimeValue)(ticks)+SecToTicks(secs) )
#define TimeToTicksSec( time, ticks, secs ) { (ticks) = (time)%TIME_TICKSPERSEC; (secs) = (time)/TIME_TICKSPERSEC ; } 

#define TIME_PosInfinity	TimeValue(0x7fffffff)
#define TIME_NegInfinity 	TimeValue(0x80000000)


//-----------------------------------------------------
// Class_ID
//-----------------------------------------------------
class Class_ID {
	ULONG a,b;
	public:
		Class_ID() { a = b = 0xffffffff; }
		Class_ID(const Class_ID& cid) { a = cid.a; b = cid.b;	}
		Class_ID(ulong aa, ulong bb) { a = aa; b = bb; }
		ULONG PartA() { return a; }
		ULONG PartB() { return b; }
		void SetPartA( ulong aa ) { a = aa; } //-- Added 11/21/96 GG
		void SetPartB( ulong bb ) { b = bb; }
		int operator==(const Class_ID& cid) const { return (a==cid.a&&b==cid.b); }
		int operator!=(const Class_ID& cid) const { return (a!=cid.a||b!=cid.b); }
		Class_ID& operator=(const Class_ID& cid)  { a=cid.a; b = cid.b; return (*this); }
	};

// SuperClass ID
typedef ulong SClass_ID;  


// Types used by ISave, ILoad, AppSave, AppLoad 
typedef enum {IO_OK=0, IO_END=1, IO_ERROR=2} IOResult; 
typedef enum {NEW_CHUNK=0, CONTAINER_CHUNK=1, DATA_CHUNK=2} ChunkType;
typedef enum {IOTYPE_MAX=0, IOTYPE_MATLIB=1} FileIOType; 

#ifdef DESIGN_VER
enum TesselationType
{
    kTriangles,
    kQuadrilaterals,
    kTriStrips
};
#endif

#endif // __JAGTYPES__

