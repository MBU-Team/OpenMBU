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

#include "udmIA64.h"
// Win64 Cleanup: Shuler
// Until MSVC++ 7.0 complete support for the
// Unified Data Model, we need this header file.

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

typedef struct {
   float r,g,b,a;

   operator float*() { return &r; }
   operator const float*() const { return &r; }

   static WORD clipColor(float c)
      { return c <= 0.0f ? 0 : c >= 1.0f ? 65535 : int(c * 65535.0); }
} BMM_Color_fl;

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
		// less operator - allows for ordering Class_IDs (used by stl maps for example) 
		bool operator<(const Class_ID& rhs) const
		{
			if ( a < rhs.a || ( a == rhs.a && b < rhs.b ) )
				return true;

			return false;
		}
	};

// SuperClass ID
typedef ULONG_PTR SClass_ID;  

// new ID for interfaces  R4 JBW 2.16.00
class Interface_ID {
	ULONG a,b;
	public:
		Interface_ID() { a = b = 0xffffffff; }
		Interface_ID(const Interface_ID& iid) { a = iid.a; b = iid.b;	}
		Interface_ID(ulong aa, ulong bb) { a = aa; b = bb; }
		ULONG PartA() { return a; }
		ULONG PartB() { return b; }
		void SetPartA( ulong aa ) { a = aa; }
		void SetPartB( ulong bb ) { b = bb; }
		int operator==(const Interface_ID& iid) const { return (a==iid.a&&b==iid.b); }
		int operator!=(const Interface_ID& iid) const { return (a!=iid.a||b!=iid.b); }
		Interface_ID& operator=(const Interface_ID& iid)  { a=iid.a; b = iid.b; return (*this); }
		// less operator - allows for ordering Class_IDs (used by stl maps for example) 
		bool operator<(const Interface_ID& rhs) const
		{
			if ( a < rhs.a || ( a == rhs.a && b < rhs.b ) )
				return true;

			return false;
		}
	};


// Types used by ISave, ILoad, AppSave, AppLoad 
typedef enum {IO_OK=0, IO_END=1, IO_ERROR=2} IOResult; 
typedef enum {NEW_CHUNK=0, CONTAINER_CHUNK=1, DATA_CHUNK=2} ChunkType;
typedef enum {IOTYPE_MAX=0, IOTYPE_MATLIB=1} FileIOType; 


// Lock / licensing types  000817  --prs.

enum ProductVersionType { PRODUCT_VERSION_DEVEL, PRODUCT_VERSION_TRIAL,
						  PRODUCT_VERSION_ORDINARY, PRODUCT_VERSION_EDU,
						  PRODUCT_VERSION_NFR };

enum LockBehaviorType { LICENSE_BEHAVIOR_PERMANENT, LICENSE_BEHAVIOR_EXTENDABLE,
                        LICENSE_BEHAVIOR_NONEXTENDABLE };



#endif // __JAGTYPES__

