//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/stream.h"
#include "core/zipHeaders.h"

const U32 ZipLocalFileHeader::csm_localFileHeaderSig = 0x04034b50;
const U32 ZipDirFileHeader::csm_dirFileHeaderSig     = 0x02014b50;
const U32 ZipEOCDRecord::csm_eocdRecordSig           = 0x06054b50;

bool
ZipLocalFileHeader::readFromStream(Stream& io_rStream)
{
   AssertFatal(io_rStream.getStatus() == Stream::Ok,
               "Error, stream is closed or has an uncleared error.");
   AssertFatal(io_rStream.hasCapability(Stream::StreamPosition),
               "Must be positionable stream to read zip headers...");

   // Read the initial header fields, marking the initial position...
   //
   U32 initialPosition = io_rStream.getPosition();
   // this is NOT endian-safe code!!!
   //bool success = io_rStream.read(sizeof(m_header), &m_header);
   // this IS.
   bool success = io_rStream.read(&m_header.headerSig);
   if (success == false || m_header.headerSig != csm_localFileHeaderSig)  {
      AssertWarn(0, "Unable to retrieve local file header from stream position...");
      io_rStream.setPosition(initialPosition);
      return false;
   }

   // NOW, read in the rest of the fields.  ENDIAN-SAFE.
   // not checking errors to start with...
   success &= io_rStream.read(&m_header.versionToDecompress);
   success &= io_rStream.read(&m_header.bitFlags);
   success &= io_rStream.read(&m_header.compressionMethod);
   success &= io_rStream.read(&m_header.lastModTime);
   success &= io_rStream.read(&m_header.lastModDate);
   success &= io_rStream.read(&m_header.crc32);
   success &= io_rStream.read(&m_header.compressedSize);
   success &= io_rStream.read(&m_header.uncompressedSize);
   success &= io_rStream.read(&m_header.fileNameLength);
   success &= io_rStream.read(&m_header.extraFieldLength);
   if (success == false)
   {
      AssertWarn(0, "Failed retrieving local file header fields from stream...");
      io_rStream.setPosition(initialPosition);
      return false;
   }


   // Read the variable length file name from the stream...
   //
   AssertFatal(m_header.fileNameLength < (MaxFileNameLength - 1),
               "Filename too long, increase structure size");
   success = io_rStream.read(m_header.fileNameLength, m_pFileName);
   m_pFileName[m_header.fileNameLength] = '\0';
   if (success == false) {
      AssertWarn(0, "Unable to read file name from stream position...");
      io_rStream.setPosition(initialPosition);
      return false;
   }


   // And seek to the end of the header, ignoring the extra field.
   io_rStream.setPosition(initialPosition +
                          (sizeof(m_header)        +
                           m_header.fileNameLength +
                           m_header.extraFieldLength));
   return true;
}


bool
ZipDirFileHeader::readFromStream(Stream& io_rStream)
{
   AssertFatal(io_rStream.getStatus() == Stream::Ok,
               "Error, stream is closed or has an uncleared error.");
   AssertFatal(io_rStream.hasCapability(Stream::StreamPosition),
               "Must be positionable stream to read zip headers...");

   // Read the initial header fields, marking the initial position...
   //
   U32 initialPosition = io_rStream.getPosition();
   // this is NOT endian-safe code!!!
   //bool success = io_rStream.read(sizeof(m_header), &m_header);
   // this IS.
   bool success = io_rStream.read(&m_header.headerSig);
   if (success == false || m_header.headerSig != csm_dirFileHeaderSig)  {
      AssertWarn(0, "Unable to retrieve dir file header from stream position...");
      io_rStream.setPosition(initialPosition);
      return false;
   }

   // NOW, read in the rest of the fields.  ENDIAN-SAFE.
   // not checking errors to start with...
   success &= io_rStream.read(&m_header.versionMadeBy);
   success &= io_rStream.read(&m_header.versionToDecompress);
   success &= io_rStream.read(&m_header.bitFlags);
   success &= io_rStream.read(&m_header.compressionMethod);
   success &= io_rStream.read(&m_header.lastModTime);
   success &= io_rStream.read(&m_header.lastModDate);
   success &= io_rStream.read(&m_header.crc32);
   success &= io_rStream.read(&m_header.compressedSize);
   success &= io_rStream.read(&m_header.uncompressedSize);
   success &= io_rStream.read(&m_header.fileNameLength);
   success &= io_rStream.read(&m_header.extraFieldLength);
   success &= io_rStream.read(&m_header.fileCommentLength);
   success &= io_rStream.read(&m_header.diskNumberStart);
   success &= io_rStream.read(&m_header.internalFileAttributes);
   success &= io_rStream.read(&m_header.externalFileAttributes);
   success &= io_rStream.read(&m_header.relativeOffsetOfLocalHeader);
   if (success == false)
   {
      AssertWarn(0, "Failed retrieving dir file header fields from stream...");
      io_rStream.setPosition(initialPosition);
      return false;
   }

   // Read the variable length file name from the stream...
   //
   AssertFatal(m_header.fileNameLength < (MaxFileNameLength - 1),
               "Filename too long, increase structure size");
   success = io_rStream.read(m_header.fileNameLength, m_pFileName);
   m_pFileName[m_header.fileNameLength] = '\0';
   if (success == false) {
      AssertWarn(0, "Unable to read dir file name from stream position...");
      io_rStream.setPosition(initialPosition);
      return false;
   }


   // And seek to the end of the header, ignoring the extra field.
   io_rStream.setPosition(initialPosition +
                          (sizeof(m_header)        +
                           m_header.fileNameLength +
                           m_header.extraFieldLength));
   return true;
}


bool
ZipEOCDRecord::readFromStream(Stream& io_rStream)
{
   AssertFatal(io_rStream.getStatus() == Stream::Ok,
               "Error, stream is closed or has an uncleared error.");
   AssertFatal(io_rStream.hasCapability(Stream::StreamPosition),
               "Must be positionable stream to read zip headers...");

   // Read the initial header fields, marking the initial position...
   //
   U32 initialPosition = io_rStream.getPosition();
   // this is NOT endian-safe code!!!
   //bool success = io_rStream.read(sizeof(m_record), &m_record);
   // this IS.
   bool success = io_rStream.read(&m_record.eocdSig);
   if (success == false || m_record.eocdSig != csm_eocdRecordSig)
   {
      AssertWarn(0, "Unable to retrieve EOCD header from stream position...");
      io_rStream.setPosition(initialPosition);
      return false;
   }

   // NOW, read in the rest of the fields.  ENDIAN-SAFE.
   // not checking errors to start with...
   success &= io_rStream.read(&m_record.diskNumber);
   success &= io_rStream.read(&m_record.eocdDiskNumber);
   success &= io_rStream.read(&m_record.numCDEntriesDisk);
   success &= io_rStream.read(&m_record.numCDEntriesTotal);
   success &= io_rStream.read(&m_record.cdSize);
   success &= io_rStream.read(&m_record.cdOffset);
   success &= io_rStream.read(&m_record.zipFileCommentLength);
   if (success == false)
   {
      AssertWarn(0, "Failed retrieving EOCD header fields from stream...");
      io_rStream.setPosition(initialPosition);
      return false;
   }

   // And seek to the end of the header, ignoring the extra field.
   io_rStream.setPosition(initialPosition +
                          (sizeof(m_record) + m_record.zipFileCommentLength));
   return true;
}

