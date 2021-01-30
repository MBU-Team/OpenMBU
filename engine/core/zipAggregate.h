//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ZIPAGGREGATE_H_
#define _ZIPAGGREGATE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

class Stream;
class ZipDirFileHeader;

class ZipAggregate
{
public:
    struct FileEntry {
        enum {
            Uncompressed = 0,
            Compressed = 1 << 0
        };

        const char* pPath;
        const char* pFileName;
        U32 fileOffset;
        U32 fileSize;
        U32 compressedFileSize;
        U32 flags;
    };

    //-------------------------------------- Instance scope members and decls.
private:
    char* m_pZipFileName;
    Vector<FileEntry> m_fileList;

    void enterZipDirRecord(const ZipDirFileHeader& in_rHeader);
    bool createZipDirectory(Stream*);
    void destroyZipDirectory();

    ZipAggregate(const ZipAggregate&);   // disallowed
public:
    ZipAggregate();
    ~ZipAggregate();

    // Opening/Manipulation interface...
public:
    bool openAggregate(const char* in_pFileName);
    void closeAggregate();
    bool refreshAggregate();

    // Entry iteration interface...
public:
    typedef Vector<FileEntry>::const_iterator iterator;

    U32 numEntries() const { return m_fileList.size(); }
    const FileEntry& operator[](const U32 idx) const { return m_fileList[idx]; }
    iterator begin() const { return m_fileList.begin(); }
    iterator end() const { return m_fileList.end(); }
};

#endif //_ZIPAGGREGATE_H_
