//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/tVector.h"
#include "core/stream.h"

#include "core/fileStream.h"
#include "core/zipSubStream.h"
#include "core/zipAggregate.h"
#include "core/zipHeaders.h"
#include "core/resizeStream.h"
#include "core/memstream.h"
#include "core/frameAllocator.h"

#include "core/resManager.h"
#include "core/findMatch.h"

#include "console/console.h"
#include "console/consoleTypes.h"

#include "util/safeDelete.h"

ResManager* ResourceManager = NULL;

bool gAllowExternalWrite = false;

char* ResManager::smExcludedDirectories = ".svn;CVS";

//------------------------------------------------------------------------------
ResourceObject::ResourceObject()
{
    next = NULL;
    prev = NULL;
    lockCount = 0;
    mInstance = NULL;
}

void ResourceObject::destruct()
{
    // If the resource was not loaded because of an error, the resource
    // pointer will be NULL
    SAFE_DELETE(mInstance);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

ResManager::ResManager()
{
    echoFileNames = 0;
    primaryPath[0] = 0;
    writeablePath[0] = 0;
    pathList = NULL;
    resourceList.nextResource = NULL;
    resourceList.next = NULL;
    resourceList.prev = NULL;
    timeoutList.nextResource = NULL;
    timeoutList.next = NULL;
    timeoutList.prev = NULL;
    registeredList = NULL;
    mLoggingMissingFiles = false;
}

void ResManager::fileIsMissing(const char* fileName)
{
    if (mLoggingMissingFiles)
    {
        char* name = dStrdup(fileName);
        //      Con::printf("> Missing file: %s", fileName);
        mMissingFileList.push_back(name);
    }
}

void ResManager::setMissingFileLogging(bool logging)
{
    mLoggingMissingFiles = logging;
    if (!mLoggingMissingFiles)
        clearMissingFileList();
}

void ResManager::clearMissingFileList()
{
    while (mMissingFileList.size())
    {
        dFree(mMissingFileList[0]);
        mMissingFileList.pop_front();
    }
    mMissingFileList.clear();
}

bool ResManager::getMissingFileList(Vector<char*>& list)
{
    if (!mMissingFileList.size())
        return false;

    for (U32 i = 0; i < mMissingFileList.size(); i++)
    {
        for (U32 j = 0; j < list.size(); j++)
        {
            if (!dStrcmp(list[j], mMissingFileList[i]))
            {
                dFree(mMissingFileList[i]);
                mMissingFileList[i] = NULL;
                break;
            }
        }
        if (mMissingFileList[i])
            list.push_back(mMissingFileList[i]);
    }

    mMissingFileList.clear();

    return true;
}

void ResourceObject::getFileTimes(FileTime* createTime, FileTime* modifyTime)
{
    char buffer[1024];
    dSprintf(buffer, sizeof(buffer), "%s/%s/%s",
        Platform::getWorkingDirectory(), path, name);
    Platform::getFileTimes(buffer, createTime, modifyTime);
}

const char* ResourceObject::getFullPath()
{
    char buffer[1024];
    dSprintf(buffer, sizeof(buffer), "%s/%s",
        path, name);
    return StringTable->insert(buffer);
}

//------------------------------------------------------------------------------

ResManager::~ResManager()
{
    purge();
    // volume list should be gone.

    if (pathList)
        dFree(pathList);

    for (ResourceObject* walk = resourceList.nextResource; walk;
        walk = walk->nextResource)
        walk->destruct();

    while (resourceList.nextResource)
        freeResource(resourceList.nextResource);

    while (registeredList)
    {
        RegisteredExtension* temp = registeredList->next;
        delete registeredList;
        registeredList = temp;
    }
}

#ifdef TORQUE_DEBUG
void ResManager::dumpLoadedResources()
{
    ResourceObject* walk = resourceList.nextResource;
    while (walk != NULL)
    {
        if (walk->mInstance != NULL)
        {
            Con::errorf("LoadedRes: %s/%s (%d)", walk->path, walk->name,
                walk->lockCount);
        }
        walk = walk->nextResource;
    }
}

ConsoleFunction(dumpResourceStats, void, 1, 1, "Dump information about resources. Debug only!")
{
    ResourceManager->dumpLoadedResources();
}

#endif

//------------------------------------------------------------------------------

void ResManager::create()
{
    AssertFatal(ResourceManager == NULL,
        "ResourceManager::create: manager already exists.");
    ResourceManager = new ResManager;

    Con::addVariable("Pref::ResourceManager::excludedDirectories", TypeString, &smExcludedDirectories);
}


//------------------------------------------------------------------------------

void ResManager::destroy()
{
    AssertFatal(ResourceManager != NULL,
        "ResourceManager::destroy: manager does not exist.");
    delete ResourceManager;
    ResourceManager = NULL;
}

//------------------------------------------------------------------------------

void ResManager::setFileNameEcho(bool on)
{
    echoFileNames = on;
}

//------------------------------------------------------------------------------

bool ResManager::isValidWriteFileName(const char* fn)
{
    if (!gAllowExternalWrite)
    {
        // all files must be based off the VFS
        if (fn[0] == '/' || dStrchr(fn, ':'))
            return false;
    }

    if (!writeablePath[0])
        return true;

    // get the path to the file
    const char* path = dStrrchr(fn, '/');
    if (!path)
        path = fn;
    else
    {
        if (!dStrchr(path, '.'))
            return false;
    }

    // now loop through the writeable path.
    const char* start = writeablePath;
    S32 pathLen = path - fn;
    for (;;)
    {
        const char* end = dStrchr(writeablePath, ';');
        if (!end)
            end = writeablePath + dStrlen(writeablePath);

        if (end - start == pathLen && !dStrnicmp(start, path, pathLen))
            return true;
        if (end[0])
            start = end + 1;
        else
            break;
    }
    return false;
}

void ResManager::setWriteablePath(const char* path)
{
    dStrcpy(writeablePath, path);
}

//------------------------------------------------------------------------------

static const char* buildPath(StringTableEntry path, StringTableEntry file)
{
    static char buf[1024];
    if (path)
        dSprintf(buf, sizeof(buf), "%s/%s", path, file);
    else
        dStrcpy(buf, file);
    return buf;
}

//------------------------------------------------------------------------------

void ResManager::getPaths(const char* fullPath, StringTableEntry& path,
    StringTableEntry& fileName)
{
    static char buf[1024];
    char* ptr = (char*)dStrrchr(fullPath, '/');
    if (!ptr)
    {
        path = NULL;
        fileName = StringTable->insert(fullPath);
    }
    else
    {
        S32 len = ptr - fullPath;
        dStrncpy(buf, fullPath, len);
        buf[len] = 0;
        fileName = StringTable->insert(ptr + 1);
        path = StringTable->insert(buf);
    }
}

//------------------------------------------------------------------------------

bool ResManager::scanZip(ResourceObject* zipObject)
{
    // now open the volume and add all its resources to the dictionary
    ZipAggregate zipAggregate;
    if (zipAggregate.
        openAggregate(buildPath(zipObject->zipPath, zipObject->zipName)) ==
        false)
    {
        Con::errorf("Error opening zip (%s/%s), need to handle this better...",
            zipObject->zipPath, zipObject->zipName);
        return false;
    }
    ZipAggregate::iterator itr;
    for (itr = zipAggregate.begin(); itr != zipAggregate.end(); itr++)
    {
        const ZipAggregate::FileEntry& rEntry = *itr;
        ResourceObject* ro =
            createZipResource(rEntry.pPath, rEntry.pFileName,
                zipObject->zipPath,
                zipObject->zipName);

        ro->flags = ResourceObject::VolumeBlock;
        ro->fileSize = rEntry.fileSize;
        ro->compressedFileSize = rEntry.compressedFileSize;
        ro->fileOffset = rEntry.fileOffset;

        dictionary.pushBehind(ro, ResourceObject::File);
    }
    zipAggregate.closeAggregate();

    return true;
}

//------------------------------------------------------------------------------

void ResManager::searchPath(const char* path)
{
    AssertFatal(path != NULL, "No path to dump?");

    Vector < Platform::FileInfo > fileInfoVec;
    Platform::dumpPath(path, fileInfoVec);

    for (U32 i = 0; i < fileInfoVec.size(); i++)
    {
        Platform::FileInfo& rInfo = fileInfoVec[i];

        // Create a resource for this file...
        //
        ResourceObject* ro = createResource(rInfo.pFullPath, rInfo.pFileName);
        dictionary.pushBehind(ro, ResourceObject::File);

        ro->flags = ResourceObject::File;
        ro->fileOffset = 0;
        ro->fileSize = rInfo.fileSize;
        ro->compressedFileSize = rInfo.fileSize;

        // see if it's a zip
        const char* extension = dStrrchr(ro->name, '.');
        if (extension && !dStricmp(extension, ".zip"))
        {
            // Copy the path and files names to the zips resource object
            ro->zipName = rInfo.pFileName;
            ro->zipPath = rInfo.pFullPath;
            scanZip(ro);
        }
    }
}


//------------------------------------------------------------------------------

bool ResManager::setModZip(const char* path)
{
    // Get the path and add .zip to the end of the dir
    const char* ext = ".zip";
    char* modPath = new char[dStrlen(path) + dStrlen(ext) + 1]; // make enough room.
    dStrcpy(modPath, path);
    dStrcat(modPath, ext);

    // Now we have to go through the root and look for our zipped up mod
    // this is unfortunately necessary because there is no means to get
    // a individual files properties -- we can only do it in one
    // big dump
    Vector < Platform::FileInfo > pathInfo;
    Platform::dumpPath(Platform::getWorkingDirectory(), pathInfo);
    for (U32 i = 0; i < pathInfo.size(); i++)
    {
        Platform::FileInfo& file = pathInfo[i];

        if (!dStricmp(file.pFileName, modPath))
        {
            // Setup the resource to the zip file itself
            ResourceObject* zip = createResource(NULL, file.pFileName);
            dictionary.pushBehind(zip, ResourceObject::File);
            zip->flags = ResourceObject::File;
            zip->fileOffset = 0;
            zip->fileSize = file.fileSize;
            zip->compressedFileSize = file.fileSize;
            zip->zipName = file.pFileName;
            zip->zipPath = NULL;

            // Setup the resource for the zip contents
            // ..now open the volume and add all its resources to the dictionary
            ZipAggregate zipAggregate;
            if (zipAggregate.openAggregate(zip->zipName) == false)
            {
                delete[] modPath;
                return false;
            }

            ZipAggregate::iterator itr;
            for (itr = zipAggregate.begin(); itr != zipAggregate.end(); itr++)
            {
                const ZipAggregate::FileEntry& rEntry = *itr;

                ResourceObject* ro = createZipResource(rEntry.pPath, rEntry.pFileName, zip->zipPath, zip->zipName);

                ro->flags = ResourceObject::VolumeBlock;
                ro->fileSize = rEntry.fileSize;
                ro->compressedFileSize = rEntry.compressedFileSize;
                ro->fileOffset = rEntry.fileOffset;
                dictionary.pushBehind(ro, ResourceObject::File);
            }
            zipAggregate.closeAggregate();

            // Break from the loop since we got our one file
            delete[] modPath;
            return true;
        }
    }

    delete[] modPath;
    return false;
}

//------------------------------------------------------------------------------

void ResManager::initExcludedDirectories()
{
    // Set up our excluded directories.
    Platform::clearExcludedDirectories();

    // ignored is a semi-colon delimited list of names.
    char* working = dStrdup(smExcludedDirectories);
    char* temp = dStrtok(working, ";");
    while (temp)
    {
        Platform::addExcludedDirectory(temp);
        temp = dStrtok(NULL, ";");
    }

    dFree(working);
}

void ResManager::setModPaths(U32 numPaths, const char** paths)
{
    // detach all the files.
    for (ResourceObject* pwalk = resourceList.nextResource; pwalk;
        pwalk = pwalk->nextResource)
        pwalk->flags = ResourceObject::Added;

    U32 pathLen = 0;

    // Set up exclusions.
    initExcludedDirectories();

    // Make sure invalid paths are not processed
    Vector<const char*> validPaths;

    // Determine if the mod paths are valid
    for (U32 i = 0; i < numPaths; i++)
    {
        if (!Platform::isSubDirectory(Platform::getWorkingDirectory(), paths[i]) || Platform::isExcludedDirectory(paths[i]))
        {
            if (!setModZip(paths[i]))
            {
                Con::errorf("setModPaths: invalid mod path directory name: '%s'", paths[i]);
                continue;
            }
        }
        pathLen += (dStrlen(paths[i]) + 1);

        // Load zip first so that local files override
        setModZip(paths[i]);
        searchPath(paths[i]);

        // Copy this path to the validPaths list
        validPaths.push_back(paths[i]);
    }

    Platform::clearExcludedDirectories();

    if (!pathLen)
        return;

    // Build the internal path list string
    pathList = (char*)dRealloc(pathList, pathLen);
    dStrcpy(pathList, validPaths[0]);
    U32 strlen;
    for (U32 i = 1; i < validPaths.size(); i++)
    {
        strlen = dStrlen(pathList);
        dSprintf(pathList + strlen, pathLen - strlen, ";%s", validPaths[i]);
    }

    // Unlink all 'added' that aren't loaded.
    ResourceObject* rwalk = resourceList.nextResource, * rtemp;
    while (rwalk != NULL)
    {
        if ((rwalk->flags & ResourceObject::Added) && !rwalk->mInstance)
        {
            rwalk->unlink();
            dictionary.remove(rwalk);
            rtemp = rwalk->nextResource;
            freeResource(rwalk);
            rwalk = rtemp;
        }
        else
            rwalk = rwalk->nextResource;
    }
}

ConsoleFunction(setModPaths, void, 2, 2, "(string paths)"
    "Set the mod paths the resource manager is using. These are semicolon delimited.")
{
    char buf[512];
    dStrncpy(buf, argv[1], sizeof(buf) - 1);
    buf[511] = '\0';

    Vector<char*> paths;
    char* temp = dStrtok(buf, ";");
    while (temp)
    {
        if (temp[0])
            paths.push_back(temp);
        temp = dStrtok(NULL, ";");
    }

    ResourceManager->setModPaths(paths.size(), (const char**)paths.address());
}

//------------------------------------------------------------------------------

const char* ResManager::getModPaths()
{
    return ((const char*)pathList);
}

ConsoleFunction(getModPaths, const char*, 1, 1, "Return the mod paths the resource manager is using.")
{
    return(ResourceManager->getModPaths());
}

//------------------------------------------------------------------------------

S32 ResManager::getSize(const char* fileName)
{
    ResourceObject* ro = find(fileName);
    if (!ro)
        return 0;
    else
    {
        if (ro->flags & ResourceObject::Memory)
            return ro->mMemStream->getStreamSize();
        return ro->fileSize;
    }
}

//------------------------------------------------------------------------------

const char* ResManager::getFullPath(const char* fileName, char* path, U32 pathlen)
{
    AssertFatal(fileName, "ResourceManager::getFullPath: fileName is NULL");
    AssertFatal(path, "ResourceManager::getFullPath: path is NULL");
    ResourceObject* obj = find(fileName);
    if (!obj)
        dStrcpy(path, fileName);
    else
        dSprintf(path, pathlen, "%s/%s", obj->path, obj->name);
    return path;
}

//------------------------------------------------------------------------------

const char* ResManager::getPathOf(const char* fileName)
{
    AssertFatal(fileName, "ResourceManager::getPathOf: fileName is NULL");
    ResourceObject* obj = find(fileName);
    if (!obj)
        return NULL;
    else
        return obj->path;
}

//------------------------------------------------------------------------------

const char* ResManager::getModPathOf(const char* fileName)
{
    AssertFatal(fileName, "ResourceManager::getModPathOf: fileName is NULL");

    if (!pathList)
        return NULL;

    ResourceObject* obj = find(fileName);
    if (!obj)
        return NULL;

    char buffer[256];
    char* base;
    const char* list = pathList;
    do
    {
        base = buffer;
        *base = 0;
        while (*list && *list != ';')
        {
            *base++ = *list++;
        }
        if (*list == ';')
            ++list;

        *base = 0;

        if (dStrncmp(buffer, obj->path, (base - buffer)) == 0)
            return StringTable->insert(buffer);
    } while (*list);

    return NULL;
}

//------------------------------------------------------------------------------

const char* ResManager::getBasePath()
{
    if (!pathList)
        return NULL;
    const char* base = dStrrchr(pathList, ';');
    return base ? (base + 1) : pathList;
}


//------------------------------------------------------------------------------

void ResManager::registerExtension(const char* name, RESOURCE_CREATE_FN create_fn)
{
    AssertFatal(!getCreateFunction(name),
        "ResourceManager::registerExtension: file extension already registered.");

    const char* extension = dStrrchr(name, '.');
    AssertFatal(extension,
        "ResourceManager::registerExtension: file has no extension.");

    RegisteredExtension* add = new RegisteredExtension;
    add->mExtension = StringTable->insert(extension);
    add->mCreateFn = create_fn;
    add->next = registeredList;
    registeredList = add;
}

//------------------------------------------------------------------------------

RESOURCE_CREATE_FN ResManager::getCreateFunction(const char* name)
{
    const char* s = dStrrchr(name, '.');
    if (!s)
        return (NULL);

    RegisteredExtension* itr = registeredList;
    while (itr)
    {
        if (dStricmp(s, itr->mExtension) == 0)
            return (itr->mCreateFn);
        itr = itr->next;
    }
    return (NULL);
}


//------------------------------------------------------------------------------

void ResManager::unlock(ResourceObject* obj)
{
    if (!obj)
        return;

    AssertFatal(obj->lockCount > 0,
        "ResourceManager::unlock: lock count is zero.");

    //set the timeout to the max requested

    if (--obj->lockCount == 0)
        obj->linkAfter(&timeoutList);
}

//------------------------------------------------------------------------------
// gets the crc of the file, ignores the stream type

bool ResManager::getCrc(const char* fileName, U32& crcVal,
    const U32 crcInitialVal)
{
    ResourceObject* obj = find(fileName);
    if (!obj)
        return (false);

    // check if in a volume
    if (obj->flags & (ResourceObject::VolumeBlock | ResourceObject::File))
    {
        // can't crc locked resources...
        if (obj->lockCount)
            return false;

        // get rid of the resource
        // have to make sure user can't have it sitting around in the resource cache

        obj->unlink();
        obj->destruct();

        Stream* stream = openStream(obj);

        U32 waterMark = 0xFFFFFFFF;

        U8* buffer;
        U32 maxSize = FrameAllocator::getHighWaterMark() - FrameAllocator::getWaterMark();
        if (maxSize < obj->fileSize)
            buffer = new U8[obj->fileSize];
        else
        {
            waterMark = FrameAllocator::getWaterMark();
            buffer = (U8*)FrameAllocator::alloc(obj->fileSize);
        }

        stream->read(obj->fileSize, buffer);

        // get the crc value
        crcVal = calculateCRC(buffer, obj->fileSize, crcInitialVal);
        if (waterMark == 0xFFFFFFFF)
            delete[]buffer;
        else
            FrameAllocator::setWaterMark(waterMark);

        closeStream(stream);
        return (true);
    }

    return (false);
}

//------------------------------------------------------------------------------

ResourceObject* ResManager::load(const char* fileName, bool computeCRC)
{
    // if filename is not known, exit now
    ResourceObject* obj = find(fileName);
    if (!obj)
        return NULL;

    // if no one has a lock on this, but it's loaded and it needs to
    // be CRC'd, delete it and reload it.
    if (!obj->lockCount && computeCRC && obj->mInstance)
        obj->destruct();

    obj->lockCount++;
    obj->unlink();      // remove from purge list

    if (!obj->mInstance)
    {
        obj->mInstance = loadInstance(obj, computeCRC);
        if (!obj->mInstance)
        {
            obj->lockCount--;
            return NULL;
        }
    }
    return obj;
}

//------------------------------------------------------------------------------

ResourceInstance* ResManager::loadInstance(const char* fileName, bool computeCRC)
{
    // if filename is not known, exit now
    ResourceObject* obj = find(fileName);
    if (!obj)
        return NULL;

    return loadInstance(obj, computeCRC);
}

//------------------------------------------------------------------------------

static const char* alwaysCRCList = ".ter.dif.dts";
ResourceObject* curResourceObj = NULL;

ResourceInstance* ResManager::loadInstance(ResourceObject* obj, bool computeCRC)
{
    Stream* stream = openStream(obj);
    if (!stream)
        return NULL;

    if (!computeCRC)
    {
        const char* x = dStrrchr(obj->name, '.');
        if (x && dStrstr(alwaysCRCList, x))
            computeCRC = true;
    }

    if (computeCRC)
        obj->crc = calculateCRCStream(stream, InvalidCRC);
    else
        obj->crc = InvalidCRC;

    curResourceObj = obj;
    RESOURCE_CREATE_FN createFunction = ResourceManager->getCreateFunction(obj->name);

    if (!createFunction)
    {
        AssertWarn(false, "ResourceObject::construct: NULL resource create function.");
        Con::errorf("ResourceObject::construct: NULL resource create function for '%s'.", obj->name);
        return NULL;
    }

    ResourceInstance* ret = createFunction(*stream);
    if (ret)
        ret->mSourceResource = obj;
    closeStream(stream);
    return ret;
}

//------------------------------------------------------------------------------

Stream* ResManager::openStream(const char* fileName)
{
    ResourceObject* obj = find(fileName);
    if (!obj)
        return NULL;
    return openStream(obj);
}

//------------------------------------------------------------------------------

Stream* ResManager::openStream(ResourceObject* obj)
{
    // if filename is not known, exit now
    if (!obj)
        return NULL;

    if (echoFileNames)
        Con::printf("FILE ACCESS: %s/%s", obj->path, obj->name);

    // used for openStream stream access
    FileStream* diskStream = NULL;

    // if disk file
    if (obj->flags & (ResourceObject::File))
    {
        diskStream = new FileStream;
        if (!diskStream->open(buildPath(obj->path, obj->name), FileStream::Read))
        {
            AssertISV(false, avar("ResManager::openStream - failed to open stream for resource '%s'!", buildPath(obj->path, obj->name)));
            delete diskStream;
            return NULL;
        }
        obj->fileSize = diskStream->getStreamSize();
        return diskStream;
    }

    // if zip file

    if (obj->flags & ResourceObject::VolumeBlock)
    {
        diskStream = new FileStream;
        diskStream->open(buildPath(obj->zipPath, obj->zipName),
            FileStream::Read);

        diskStream->setPosition(obj->fileOffset);

        ZipLocalFileHeader zlfHeader;
        if (zlfHeader.readFromStream(*diskStream) == false)
        {
            Con::errorf("ResourceManager::loadStream: '%s' Not in the zip! (%s/%s)",
                obj->name, obj->zipPath, obj->zipName);
            diskStream->close();
            return NULL;
        }

        if (zlfHeader.m_header.compressionMethod == ZipLocalFileHeader::Stored
            || obj->fileSize == 0)
        {
            // Just read straight from the stream...
            ResizeFilterStream* strm = new ResizeFilterStream;
            strm->attachStream(diskStream);
            strm->setStreamOffset(diskStream->getPosition(), obj->fileSize);
            return strm;
        }
        else
        {
            if (zlfHeader.m_header.compressionMethod ==
                ZipLocalFileHeader::Deflated)
            {
                ZipSubRStream* zipStream = new ZipSubRStream;
                zipStream->attachStream(diskStream);
                zipStream->setUncompressedSize(obj->fileSize);
                return zipStream;
            }
            else
            {
                AssertFatal(false, avar("ResourceManager::loadStream: '%s' Compressed inappropriately in the zip! (%s/%s)",
                    obj->name, obj->zipPath, obj->zipName));
                diskStream->close();
                return NULL;
            }
        }
    }

    // If memory file
    if (obj->flags & ResourceObject::Memory)
    {
        MemSubStream* memStream = new MemSubStream;
        memStream->attachStream(obj->mMemStream);
        return memStream;
    }

    // unknown type
    return NULL;
}

//------------------------------------------------------------------------------

void ResManager::closeStream(Stream* stream)
{
    FilterStream* subStream = dynamic_cast <FilterStream*>(stream);
    if (subStream)
    {
        stream = subStream->getStream();
        subStream->detachStream();
        delete subStream;
    }
    delete stream;
}


//------------------------------------------------------------------------------

ResourceObject* ResManager::find(const char* fileName)
{
    if (!fileName)
        return NULL;
    StringTableEntry path, file;
    getPaths(fileName, path, file);
    ResourceObject* ret = dictionary.find(path, file);
    if (!ret)
    {
        if (path && file && !dStrcmp(path, "mem"))
        {
            // Opening memory file
            ret = createResource(path, file);
            dictionary.pushBehind(ret, ResourceObject::Memory);
            
            ret->flags = ResourceObject::Memory;
            ret->mMemStream = new ResizableMemStream();

            return ret;
        }

        // Potentially dangerous behavior to have in shipping version but *very* useful
        // in a production environment
#ifndef TORQUE_SHIPPING
      // If we couldn't find the file in the resource list (generated
      // by setting the modPaths) then try to load it directly
        if (Platform::isFile(fileName))
        {
            ret = createResource(path, file);
            dictionary.pushBehind(ret, ResourceObject::File);

            ret->flags = ResourceObject::File;
            ret->fileOffset = 0;

            S32 fileSize = Platform::getFileSize(fileName);
            ret->fileSize = fileSize;
            ret->compressedFileSize = fileSize;

            return ret;
        }
#endif

        fileIsMissing(fileName);
    }
    return ret;
}

//------------------------------------------------------------------------------

ResourceObject* ResManager::find(const char* fileName, U32 flags)
{
    if (!fileName)
        return NULL;
    StringTableEntry path, file;
    getPaths(fileName, path, file);
    return dictionary.find(path, file, flags);
}


//------------------------------------------------------------------------------
// Add resource constructed outside the manager

bool ResManager::add(const char* name, ResourceInstance* addInstance,
    bool extraLock)
{
    StringTableEntry path, file;
    getPaths(name, path, file);

    ResourceObject* obj = dictionary.find(path, file);
    if (obj && obj->mInstance)
        // Resource already exists?
        return false;

    if (!obj)
        obj = createResource(path, file);

    dictionary.pushBehind(obj,
        ResourceObject::File | ResourceObject::VolumeBlock | ResourceObject::Memory);
    obj->mInstance = addInstance;
    addInstance->mSourceResource = obj;
    obj->lockCount = extraLock ? 2 : 1;
    unlock(obj);
    return true;
}

//------------------------------------------------------------------------------

void ResManager::purge()
{
    bool found;
    do
    {
        ResourceObject* obj = timeoutList.getNext();
        found = false;
        while (obj)
        {
            ResourceObject* temp = obj;
            obj = obj->next;
            temp->unlink();
            temp->destruct();
            found = true;
            if (temp->flags & ResourceObject::Added)
                freeResource(temp);
        }
    } while (found);
}

ConsoleFunction(purgeResources, void, 1, 1, "Purge resources from the resource manager.")
{
    ResourceManager->purge();
}

//------------------------------------------------------------------------------

void ResManager::purge(ResourceObject* obj)
{
    AssertFatal(obj->lockCount == 0,
        "ResourceManager::purge: handle lock count is not ZERO.") obj->
        unlink();
    obj->destruct();
}

//------------------------------------------------------------------------------
// serialize sorts a list of files by .zip and position within the zip
// it allows an aggregate (material list, etc) to find the preferred
// loading order for a set of files.
//------------------------------------------------------------------------------

struct ResourceObjectIndex
{
    ResourceObject* ro;
    const char* fileName;

    static S32 QSORT_CALLBACK compare(const void* s1, const void* s2)
    {
        const ResourceObjectIndex* r1 = (ResourceObjectIndex*)s1;
        const ResourceObjectIndex* r2 = (ResourceObjectIndex*)s2;

        if (r1->ro->path != r2->ro->path)
            return r1->ro->path - r2->ro->path;
        if (r1->ro->name != r2->ro->name)
            return r1->ro->name - r2->ro->name;
        return r1->ro->fileOffset - r2->ro->fileOffset;
    }
};

//------------------------------------------------------------------------------

void ResManager::serialize(VectorPtr < const char*>& filenames)
{
    Vector < ResourceObjectIndex > sortVector;

    sortVector.reserve(filenames.size());

    U32 i;
    for (i = 0; i < filenames.size(); i++)
    {
        ResourceObjectIndex roi;
        roi.ro = find(filenames[i]);
        roi.fileName = filenames[i];
        sortVector.push_back(roi);
    }

    dQsort((void*)&sortVector[0], sortVector.size(),
        sizeof(ResourceObjectIndex), ResourceObjectIndex::compare);
    for (i = 0; i < filenames.size(); i++)
        filenames[i] = sortVector[i].fileName;
}

//------------------------------------------------------------------------------

ResourceObject* ResManager::findMatch(const char* expression, const char** fn,
    ResourceObject* start)
{
    if (!start)
        start = resourceList.nextResource;
    else
        start = start->nextResource;
    while (start)
    {
        const char* fname = buildPath(start->path, start->name);
        if (FindMatch::isMatch(expression, fname, false))
        {
            *fn = fname;
            return start;
        }
        start = start->nextResource;
    }
    return NULL;
}

S32 ResManager::findMatches(FindMatch* pFM)
{
    static char buffer[16384];
    S32 bufl = 0;
    ResourceObject* walk;
    for (walk = resourceList.nextResource; walk && !pFM->isFull(); walk = walk->nextResource)
    {
        const char* fpath =
            buildPath(walk->path, walk->name);
        if (bufl + dStrlen(fpath) >= 16380)
            return pFM->numMatches();
        dStrcpy(buffer + bufl, fpath);
        if (pFM->findMatch(buffer + bufl))
            bufl += dStrlen(fpath) + 1;
    }
    return (pFM->numMatches());
}

//------------------------------------------------------------------------------

bool ResManager::findFile(const char* name)
{
    return (bool)find(name);
}

//------------------------------------------------------------------------------

ResourceObject* ResManager::createResource(StringTableEntry path, StringTableEntry file)
{
    ResourceObject* newRO = dictionary.find(path, file);
    if (newRO)
        return newRO;

    newRO = new ResourceObject;
    newRO->path = path;
    newRO->name = file;
    newRO->lockCount = 0;
    newRO->mInstance = NULL;
    newRO->flags = ResourceObject::Added;
    newRO->next = newRO->prev = NULL;
    newRO->nextResource = resourceList.nextResource;
    resourceList.nextResource = newRO;
    newRO->prevResource = &resourceList;
    if (newRO->nextResource)
        newRO->nextResource->prevResource = newRO;
    dictionary.insert(newRO, path, file);
    newRO->fileSize = newRO->fileOffset = newRO->compressedFileSize = 0;
    newRO->zipPath = NULL;
    newRO->zipName = NULL;
    newRO->crc = InvalidCRC;

    return newRO;
}

//------------------------------------------------------------------------------

ResourceObject* ResManager::createZipResource(StringTableEntry path, StringTableEntry file,
    StringTableEntry zipPath,
    StringTableEntry zipName)
{
    ResourceObject* newRO = dictionary.find(path, file, zipPath, zipName);
    if (newRO)
        return newRO;

    newRO = new ResourceObject;
    newRO->path = path;
    newRO->name = file;
    newRO->lockCount = 0;
    newRO->mInstance = NULL;
    newRO->flags = ResourceObject::Added;
    newRO->next = newRO->prev = NULL;
    newRO->nextResource = resourceList.nextResource;
    resourceList.nextResource = newRO;
    newRO->prevResource = &resourceList;
    if (newRO->nextResource)
        newRO->nextResource->prevResource = newRO;
    dictionary.insert(newRO, path, file);
    newRO->fileSize = newRO->fileOffset = newRO->compressedFileSize = 0;
    newRO->zipPath = zipPath;
    newRO->zipName = zipName;
    newRO->crc = InvalidCRC;

    return newRO;
}

//------------------------------------------------------------------------------

void ResManager::freeResource(ResourceObject* ro)
{
    ro->destruct();
    ro->unlink();

    //   if((ro->flags & ResourceObject::File) && ro->lockedData)
    //      delete[] ro->lockedData;

    if (ro->prevResource)
        ro->prevResource->nextResource = ro->nextResource;
    if (ro->nextResource)
        ro->nextResource->prevResource = ro->prevResource;
    dictionary.remove(ro);
    delete ro;
}

//------------------------------------------------------------------------------

bool ResManager::openFileForWrite(Stream*& stream, const char* fileName, U32 accessMode, bool forceMemory)
{
    if (!isValidWriteFileName(fileName))
        return false;

    // tag it on to the first directory
    char path[1024];
    dStrcpy(path, fileName);
    char* file = dStrrchr(path, '/');
    if (!file)
        return false;      // don't allow storing files in root
    *file++ = 0;

    if (path && file && (forceMemory || !dStrnicmp(path, "mem", 3) && path[3] == '/' || path[3] == 0))
    {
        // Opening memory file
        ResourceObject* ret = createResource(StringTable->insert(path), StringTable->insert(file));
        dictionary.pushBehind(ret, ResourceObject::Memory);
        
        ret->flags = ResourceObject::Memory;
        ret->mMemStream = new ResizableMemStream();

        MemSubStream* memStream = new MemSubStream;
        memStream->attachStream(ret->mMemStream);

        stream = memStream;
        return true;
    }

    if (!Platform::createPath(fileName))   // create directory tree
        return false;

    FileStream* fs = new FileStream();
    if (!fs->open(fileName, (FileStream::AccessMode)accessMode))
    {
        delete fs;
        return false;
    }
    stream = fs;

    // create a resource for the file.
    ResourceObject* ro = createResource(StringTable->insert(path), StringTable->insert(file));
    ro->flags = ResourceObject::File;
    ro->fileOffset = 0;
    ro->fileSize = 0;
    ro->compressedFileSize = 0;
    return true;
}

//------------------------------------------------------------------------------
// reload - reload updated resource
//------------------------------------------------------------------------------
ResourceObject* ResManager::reload(const char* fileName, bool computeCRC)
{
    if (!fileName)
        return NULL;

    StringTableEntry path, file;
    getPaths(fileName, path, file);
    ResourceObject* obj = dictionary.find(path, file);

    // If we found the resource it is already loaded.
    // Check the crc to see if it needs reloading
    if (obj)
    {
        bool reload = true;

        // If we have a valid crc check against the new crc
        if (obj->crc != InvalidCRC)
        {
            FileStream file;
            file.open(fileName, FileStream::Read);

            U32 newCRC = calculateCRCStream(&file, InvalidCRC);
            file.close();

            if (newCRC != obj->crc)
                reload = true;
        }

        if (reload)
            obj->destruct();
    }
    // If not then try to find it outside of the dictionary
    else
        obj = find(fileName);

    if (!obj)
        return NULL;

    // if no one has a lock on this, but it's loaded and it needs to
    // be CRC'd, delete it and reload it.
    if (!obj->lockCount && computeCRC && obj->mInstance)
        obj->destruct();

    obj->lockCount++;
    obj->unlink();      // remove from purge list
    if (!obj->mInstance)
    {
        obj->mInstance = loadInstance(obj, computeCRC);
        if (!obj->mInstance)
        {
            obj->lockCount--;
            return NULL;
        }
    }
    return obj;
}

//------------------------------------------------------------------------------
// Start resource traversal - wipe internal data for a new traversal
//------------------------------------------------------------------------------
void ResManager::startResourceTraverse()
{
    mTraverseHashIndex = 0;
    mTraverseCurObj = NULL;
}

//------------------------------------------------------------------------------
// Get next resource in resource traversal
//------------------------------------------------------------------------------
ResourceObject* ResManager::getNextResource()
{
    if (mTraverseCurObj)
    {
        if (mTraverseCurObj->nextEntry)
        {
            mTraverseCurObj = mTraverseCurObj->nextEntry;
            return mTraverseCurObj;
        }
        else
        {
            ++mTraverseHashIndex;
        }
    }

    U32 hashTableSize = 0;
    ResourceObject** hashTable = NULL;
    dictionary.getHash(&hashTable, &hashTableSize);

    for (U32 i = mTraverseHashIndex; i < hashTableSize; i++)
    {
        ResourceObject* obj;
        if (obj = hashTable[i])
        {
            mTraverseHashIndex = i;
            mTraverseCurObj = obj;
            return mTraverseCurObj;
        }
    }

    return NULL;  // end of traversal
}

