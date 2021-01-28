//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "math/mRandom.h"

MRandomLCG gRandGen;
U32 gRandGenSeed = 1376312589;

void MRandomLCG::setGlobalRandSeed(U32 seed)
{
	if (Game->isJournalReading())
		Game->journalRead(&gRandGenSeed);
	else
	{
		gRandGenSeed = seed;
		if (Game->isJournalWriting())
			Game->journalWrite(gRandGenSeed);
	}

	//now actually set the seed
	gRandGen.setSeed(gRandGenSeed);
}

static U32 msSeed = 1376312589;

inline U32 generateSeed()
{
   // A very, VERY crude LCG but good enough to generate
   // a nice range of seed values
   msSeed = (msSeed * 0x015a4e35L) + 1;
   msSeed = (msSeed>>16)&0x7fff;
   return (msSeed);
}

//--------------------------------------
void MRandomGenerator::setSeed()
{
   setSeed(generateSeed());
}


//--------------------------------------
const S32 MRandomLCG::msQuotient  = S32_MAX / 16807L;
const S32 MRandomLCG::msRemainder = S32_MAX % 16807L;


//--------------------------------------
MRandomLCG::MRandomLCG()
{
   setSeed(generateSeed());
}

MRandomLCG::MRandomLCG(S32 s)
{
   setSeed(s);
}


//--------------------------------------
void MRandomLCG::setSeed(S32 s)
{
   mSeed = s;
}


//--------------------------------------
U32 MRandomLCG::randI()
{
   if ( mSeed <= msQuotient )
      mSeed = (mSeed * 16807L) % S32_MAX;
   else
   {
      S32 high_part = mSeed / msQuotient;
      S32 low_part  = mSeed % msQuotient;

      S32 test = (16807L * low_part) - (msRemainder * high_part);

      if ( test > 0 )
         mSeed = test;
      else
         mSeed = test + S32_MAX;

   }
   return mSeed;
}



//--------------------------------------
MRandomR250::MRandomR250()
{
   setSeed(generateSeed());
}

MRandomR250::MRandomR250(S32 s)
{
   setSeed(s);
}


//--------------------------------------
void MRandomR250::setSeed(S32 s)
{
   mSeed = s;
   MRandomLCG lcg( s );
   mIndex = 0;

   S32 j;
   for (j = 0; j < 250; j++)        // fill r250 buffer with bit values
      mBuffer[j] = lcg.randI();

   for (j = 0; j < 250; j++)        // set some MSBs to 1
      if ( lcg.randI() > 0x40000000L )
         mBuffer[j] |= 0x80000000L;


   U32 msb  = 0x80000000;           // turn on diagonal bit
   U32 mask = 0xffffffff;           // turn off the leftmost bits

   for (j = 0; j < 32; j++)
   {
      S32 k = 7 * j + 3;            // select a word to operate on
      mBuffer[k] &= mask;           // turn off bits left of the diagonal
      mBuffer[k] |= msb;            // turn on the diagonal bit
      mask >>= 1;
      msb  >>= 1;
   }
}


//--------------------------------------
U32 MRandomR250::randI()
{
   S32 j;

   // wrap pointer around
   if ( mIndex >= 147 ) j = mIndex - 147;
   else                 j = mIndex + 103;

   U32 new_rand = mBuffer[ mIndex ] ^ mBuffer[ j ];
   mBuffer[ mIndex ] = new_rand;

   // increment pointer for next time
   if ( mIndex >= 249 ) mIndex = 0;
   else                 mIndex++;

   return new_rand >> 1;
}


