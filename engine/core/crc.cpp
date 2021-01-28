//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/stream.h"

//-----------------------------------------------------------------------------
// simple crc function - generates lookup table on first call

static U32 crcTable[256];
static bool crcTableValid;

static void calculateCRCTable()
{
   U32 val;

   for(S32 i = 0; i < 256; i++)
   {
      val = i;
      for(S32 j = 0; j < 8; j++)
      {
         if(val & 0x01)
            val = 0xedb88320 ^ (val >> 1);
         else
            val = val >> 1;
      }
      crcTable[i] = val;
   }

   crcTableValid = true;
}


//-----------------------------------------------------------------------------

U32 calculateCRC(const void * buffer, S32 len, U32 crcVal )
{
   // check if need to generate the crc table
   if(!crcTableValid)
      calculateCRCTable();

   // now calculate the crc
   char * buf = (char*)buffer;
   for(S32 i = 0; i < len; i++)
      crcVal = crcTable[(crcVal ^ buf[i]) & 0xff] ^ (crcVal >> 8);
   return(crcVal);
}

U32 calculateCRCStream(Stream *stream, U32 crcVal )
{
   // check if need to generate the crc table
   if(!crcTableValid)
      calculateCRCTable();

   // now calculate the crc
   stream->setPosition(0);
   S32 len = stream->getStreamSize();
   U8 buf[4096];

   S32 segCount = (len + 4095) / 4096;

   for(S32 j = 0; j < segCount; j++)
   {
      S32 slen = getMin(4096, len - (j * 4096));
      stream->read(slen, buf);
      crcVal = calculateCRC(buf, slen, crcVal);
   }
   stream->setPosition(0);
   return(crcVal);
}
