//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "platform/profiler.h"
#include "platform/platformMutex.h"


#ifdef TORQUE_MULTITHREAD
void* gMemMutex = NULL;
#endif

//-------------------------------------- Make sure we don't have the define set
#ifdef new
#undef new
#endif

enum MemConstants : U32 {
    Allocated = BIT(0),
    Array = BIT(1),
    DebugFlag = BIT(2),
    AllocatedGuard = 0xCEDEFEDE,
    FreeGuard = 0x5555FFFF,
    MaxAllocationAmount = 0xFFFFFFFF,
    TreeNodeAllocCount = 2048,
};

enum RedBlackTokens {
    Red = 0,
    Black = 1
};

static U32 MinPageSize = 8 * 1024 * 1024;

#if !defined(TORQUE_SHIPPING) && defined(TORQUE_DEBUG_GUARD)
#define LOG_PAGE_ALLOCS
#endif

U32 gNewNewTotal = 0;
U32 gImageAlloc = 0;
//---------------------------------------------------------------------------

namespace Memory
{

    ConsoleFunctionGroupBegin(Memory, "Memory manager utility functions.");

    struct FreeHeader;

    /// Red/Black Tree Node - used to store queues of free blocks
    struct TreeNode
    {
        U32 size;
        TreeNode* parent;
        TreeNode* left;
        TreeNode* right;
        U32 color;
        FreeHeader* queueHead;
        FreeHeader* queueTail;
        U32 unused;
    };

    struct Header
    {
        // doubly linked list of allocated and free blocks -
        // contiguous in memory.
#ifdef TORQUE_DEBUG_GUARD
        U32 preguard[4];
#endif
        Header* next;
        Header* prev;
        dsize_t size;
        U32 flags;
#ifdef TORQUE_DEBUG_GUARD
#ifdef TORQUE_ENABLE_PROFILE_PATH
        U32 unused[5];
#else
        U32 unused[4];
#endif
        U32 postguard[4];
#endif
    };

    struct AllocatedHeader
    {
#ifdef TORQUE_DEBUG_GUARD
        U32 preguard[4];
#endif
        Header* next;
        Header* prev;
        dsize_t size;
        U32 flags;

#ifdef TORQUE_DEBUG_GUARD
        // an allocated header will only have this stuff if TORQUE_DEBUG_GUARD
        U32 line;
        U32 allocNum;
        const char* fileName;
#ifdef TORQUE_ENABLE_PROFILE_PATH
        const char* profilePath;
#endif
        U32 realSize;
        U32 postguard[4];
#endif
    };

    struct FreeHeader
    {
#ifdef TORQUE_DEBUG_GUARD
        U32 preguard[4];
#endif
        Header* next;
        Header* prev;
        dsize_t size;
        U32 flags;

        // since a free header has at least one cache line (16 bytes)
        // we can tag some more stuff on:

        FreeHeader* nextQueue;  // of the same size
        FreeHeader* prevQueue;  // doubly linked
        TreeNode* treeNode; // which tree node we're coming off of.
        U32 guard;
#ifdef TORQUE_DEBUG_GUARD
#ifdef TORQUE_ENABLE_PROFILE_PATH
        U32 unused;
#endif
        U32 postguard[4];
#endif
    };

    struct PageRecord
    {
        dsize_t allocSize;
        PageRecord* prevPage;
        Header* headerList;  // if headerList is NULL, this is a treeNode page
        void* basePtr;
        U32 unused[4];  // even out the record to 32 bytes...
                        // so if we're on a 32-byte cache-line comp, the tree nodes
                        // will cache better
    };

    PageRecord* gPageList = NULL;
    TreeNode nil;
    TreeNode* NIL = &nil;
    TreeNode* gFreeTreeRoot = &nil;
    TreeNode* gTreeFreeList = NULL;

    U32  gInsertCount = 0;
    U32  gRemoveCount = 0;
    U32  gBreakAlloc = 0xFFFFFFFF;
    U32  gCurrAlloc = 0;
    char gLogFilename[256] = "memlog.txt";
    bool gEnableLogging = false;
    bool gNeverLogLeaks = 0;
    bool gAlwaysLogLeaks = 0;
    U32 gBytesAllocated = 0;
    U32 gPageBytesAllocated = 0;

#ifdef TORQUE_DEBUG_GUARD
    static void checkGuard(Header* header, bool alloc)
    {
        U32 guardVal = alloc ? AllocatedGuard : FreeGuard;
        for (U32 i = 0; i < 4; i++)
            if (header->preguard[i] != guardVal || header->postguard[i] != guardVal)
                Platform::debugBreak();
    }
#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void setGuard(Header* header, bool alloc)
    {
        U32 guardVal = alloc ? AllocatedGuard : FreeGuard;
        for (U32 i = 0; i < 4; i++)
            header->preguard[i] = header->postguard[i] = guardVal;
    }
#endif // !defined(TORQUE_DISABLE_MEMORY_MANAGER)
#endif

    static void memoryError()
    {
        // free all the pages
        PageRecord* walk = gPageList;
        while (walk) {
            PageRecord* prev = walk->prevPage;
            dRealFree(walk);
            walk = prev;
        }
        AssertFatal(false, "Error allocating memory! Shutting down.");
        Platform::AlertOK("Torque Memory Error", "Error allocating memory. Shutting down.\n");
        Platform::forceShutdown(-1);
    }

    PageRecord* allocPage(dsize_t pageSize)
    {
        pageSize += sizeof(PageRecord);
        void* base = dRealMalloc(pageSize);
        if (base == NULL)
            memoryError();

        PageRecord* rec = (PageRecord*)base;
        rec->basePtr = (void*)(rec + 1);
        rec->allocSize = pageSize;
        rec->prevPage = gPageList;
        gPageList = rec;
        rec->headerList = NULL;
        return rec;
    }

    TreeNode* allocTreeNode()
    {
        if (!gTreeFreeList)
        {
            PageRecord* newPage = allocPage(TreeNodeAllocCount * sizeof(TreeNode));
            TreeNode* walk = (TreeNode*)newPage->basePtr;
            U32 i;
            gTreeFreeList = walk;
            for (i = 0; i < TreeNodeAllocCount - 1; i++, walk++)
                walk->parent = walk + 1;
            walk->parent = NULL;
        }
        TreeNode* ret = gTreeFreeList;
        gTreeFreeList = ret->parent;
        return ret;
    }

    void freeTreeNode(TreeNode* tn)
    {
        tn->parent = gTreeFreeList;
        gTreeFreeList = tn;
    }


    static U32 validateTreeRecurse(TreeNode* tree)
    {
        if (tree == NIL)
            return 1;
        // check my left tree
        int lcount, rcount, nc = 0;

        if (tree->color == Red)
        {
            if (tree->left->color == Red || tree->right->color == Red)
                Platform::debugBreak();
        }
        else
            nc = 1;

        FreeHeader* walk = tree->queueHead;
        if (!walk)
            Platform::debugBreak();
        FreeHeader* prev = NULL;
        while (walk)
        {
            if (walk->prevQueue != prev)
                Platform::debugBreak();
            if (walk->treeNode != tree)
                Platform::debugBreak();
            if (walk->size != tree->size)
                Platform::debugBreak();
            if (!walk->nextQueue && walk != tree->queueTail)
                Platform::debugBreak();
            prev = walk;
            walk = walk->nextQueue;
        }

        lcount = validateTreeRecurse(tree->left);
        rcount = validateTreeRecurse(tree->right);
        if (lcount != rcount)
            Platform::debugBreak();
        return lcount + nc;
    }

    static void validateParentageRecurse(TreeNode* tree)
    {
        if (tree->left != NIL)
        {
            if (tree->left->parent != tree)
                Platform::debugBreak();

            if (tree->left->size > tree->size)
                Platform::debugBreak();
            validateParentageRecurse(tree->left);
        }
        if (tree->right != NIL)
        {
            if (tree->right->parent != tree)
                Platform::debugBreak();

            if (tree->right->size < tree->size)
                Platform::debugBreak();
            validateParentageRecurse(tree->right);
        }
    }

    static void validateParentage()
    {
        if (gFreeTreeRoot == NIL)
            return;
        validateParentageRecurse(gFreeTreeRoot);
    }

    static void validateTree()
    {
        if (gFreeTreeRoot == NIL)
            return;
        validateParentageRecurse(gFreeTreeRoot);
        validateTreeRecurse(gFreeTreeRoot);
    }

    void validate()
    {

#ifdef TORQUE_MULTITHREAD
        if (!gMemMutex)
            gMemMutex = Mutex::createMutex();

        Mutex::lockMutex(gMemMutex);
#endif


        // first validate the free tree:
        validateTree();
        // now validate all blocks:
        for (PageRecord* list = gPageList; list; list = list->prevPage)
        {
            Header* prev = NULL;
            for (Header* walk = list->headerList; walk; walk = walk->next)
            {
#ifdef TORQUE_DEBUG_GUARD
                checkGuard(walk, walk->flags & Allocated);
#endif
                if (walk->prev != prev)
                    Platform::debugBreak();
                prev = walk;
                if (walk->next && ((const char*)(walk->next) != (const char*)(walk)+sizeof(Header) + walk->size))
                    Platform::debugBreak();
            }
        }

#ifdef TORQUE_MULTITHREAD
        Mutex::unlockMutex(gMemMutex);
#endif
    }

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void rotateLeft(TreeNode* hdr)
    {
        TreeNode* temp = hdr->right;
        hdr->right = temp->left;
        if (temp->left != NIL)
            temp->left->parent = hdr;
        temp->parent = hdr->parent;
        if (temp->parent == NIL)
            gFreeTreeRoot = temp;
        else if (hdr == hdr->parent->left)
            hdr->parent->left = temp;
        else
            hdr->parent->right = temp;
        temp->left = hdr;
        hdr->parent = temp;
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void rotateRight(TreeNode* hdr)
    {
        TreeNode* temp = hdr->left;
        hdr->left = temp->right;
        if (temp->right != NIL)
            temp->right->parent = hdr;
        temp->parent = hdr->parent;
        if (temp->parent == NIL)
            gFreeTreeRoot = temp;
        else if (hdr == hdr->parent->left)
            hdr->parent->left = temp;
        else
            hdr->parent->right = temp;
        temp->right = hdr;
        hdr->parent = temp;
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void treeInsert(FreeHeader* fhdr)
    {
#ifdef TORQUE_DEBUG_GUARD
        checkGuard((Header*)fhdr, true); // check to see that it's got allocated guards
        setGuard((Header*)fhdr, false);
#endif
        //gInsertCount++;

        TreeNode* newParent = NIL;
        TreeNode* walk = gFreeTreeRoot;
        while (walk != NIL)
        {
            newParent = walk;
            if (fhdr->size < walk->size)
                walk = walk->left;
            else if (fhdr->size > walk->size)
                walk = walk->right;
            else // tag it on the end of the queue...
            {
                // insert it on this header...
                walk->queueTail->nextQueue = fhdr;
                fhdr->prevQueue = walk->queueTail;
                walk->queueTail = fhdr;
                fhdr->nextQueue = NULL;
                fhdr->treeNode = walk;
                return;
            }
        }
        TreeNode* hdr = allocTreeNode();
        hdr->size = fhdr->size;
        hdr->queueHead = hdr->queueTail = fhdr;
        fhdr->nextQueue = fhdr->prevQueue = NULL;
        fhdr->treeNode = hdr;

        hdr->left = NIL;
        hdr->right = NIL;

        hdr->parent = newParent;

        if (newParent == NIL)
            gFreeTreeRoot = hdr;
        else if (hdr->size < newParent->size)
            newParent->left = hdr;
        else
            newParent->right = hdr;

        // do red/black rotations
        hdr->color = Red;
        while (hdr != gFreeTreeRoot && (hdr->parent->color == Red))
        {
            TreeNode* parent = hdr->parent;
            TreeNode* pparent = hdr->parent->parent;

            if (parent == pparent->left)
            {
                TreeNode* temp = pparent->right;
                if (temp->color == Red)
                {
                    parent->color = Black;
                    temp->color = Black;
                    pparent->color = Red;
                    hdr = pparent;
                }
                else
                {
                    if (hdr == parent->right)
                    {
                        hdr = parent;
                        rotateLeft(hdr);
                        parent = hdr->parent;
                        pparent = hdr->parent->parent;
                    }
                    parent->color = Black;
                    pparent->color = Red;
                    rotateRight(pparent);
                }
            }
            else
            {
                TreeNode* temp = pparent->left;
                if (temp->color == Red)
                {
                    parent->color = Black;
                    temp->color = Black;
                    pparent->color = Red;
                    hdr = pparent;
                }
                else
                {
                    if (hdr == parent->left)
                    {
                        hdr = parent;
                        rotateRight(hdr);
                        parent = hdr->parent;
                        pparent = hdr->parent->parent;
                    }
                    parent->color = Black;
                    pparent->color = Red;
                    rotateLeft(pparent);
                }
            }
        }
        gFreeTreeRoot->color = Black;
        //validateTree();
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void treeRemove(FreeHeader* hdr)
    {
#ifdef TORQUE_DEBUG_GUARD
        checkGuard((Header*)hdr, false);
        setGuard((Header*)hdr, true);
#endif
        //validateTree();
        //gRemoveCount++;

        FreeHeader* prev = hdr->prevQueue;
        FreeHeader* next = hdr->nextQueue;

        if (prev)
            prev->nextQueue = next;
        else
            hdr->treeNode->queueHead = next;

        if (next)
            next->prevQueue = prev;
        else
            hdr->treeNode->queueTail = prev;

        if (prev || next)
            return;

        TreeNode* z = hdr->treeNode;

        nil.color = Black;

        TreeNode* y, * x;
        if (z->left == NIL || z->right == NIL)
            y = z;
        else
        {
            y = z->right;
            while (y->left != NIL)
                y = y->left;
        }
        if (y->left != NIL)
            x = y->left;
        else
            x = y->right;

        x->parent = y->parent;
        if (y->parent == NIL)
            gFreeTreeRoot = x;
        else if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;

        U32 yColor = y->color;
        if (y != z)
        {
            // copy y's important fields into z (since we're going to free y)
            if (z->parent->left == z)
                z->parent->left = y;
            else if (z->parent->right == z)
                z->parent->right = y;
            y->left = z->left;
            y->right = z->right;
            if (y->left != NIL)
                y->left->parent = y;
            if (y->right != NIL)
                y->right->parent = y;
            y->parent = z->parent;
            if (z->parent == NIL)
                gFreeTreeRoot = y;
            y->color = z->color;
            if (x->parent == z)
                x->parent = y;
            //validateTree();
        }
        freeTreeNode(z);

        if (yColor == Black)
        {
            while (x != gFreeTreeRoot && x->color == Black)
            {
                TreeNode* w;
                if (x == x->parent->left)
                {
                    w = x->parent->right;
                    if (w->color == Red)
                    {
                        w->color = Black;
                        x->parent->color = Red;
                        rotateLeft(x->parent);
                        w = x->parent->right;
                    }
                    if (w->left->color == Black && w->right->color == Black)
                    {
                        w->color = Red;
                        x = x->parent;
                    }
                    else
                    {
                        if (w->right->color == Black)
                        {
                            w->left->color = Black;
                            rotateRight(w);
                            w = x->parent->right;
                        }
                        w->color = x->parent->color;
                        x->parent->color = Black;
                        w->right->color = Black;
                        rotateLeft(x->parent);
                        x = gFreeTreeRoot;
                    }
                }
                else
                {
                    w = x->parent->left;
                    if (w->color == Red)
                    {
                        w->color = Black;
                        x->parent->color = Red;
                        rotateRight(x->parent);
                        w = x->parent->left;
                    }
                    if (w->left->color == Black && w->right->color == Black)
                    {
                        w->color = Red;
                        x = x->parent;
                    }
                    else
                    {
                        if (w->left->color == Black)
                        {
                            w->right->color = Black;
                            rotateLeft(w);
                            w = x->parent->left;
                        }
                        w->color = x->parent->color;
                        x->parent->color = Black;
                        w->left->color = Black;
                        rotateRight(x->parent);
                        x = gFreeTreeRoot;
                    }
                }
            }
            x->color = Black;
        }
        //validateTree();
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static FreeHeader* treeFindSmallestGreaterThan(dsize_t size)
    {
        TreeNode* bestMatch = NIL;
        TreeNode* walk = gFreeTreeRoot;
        while (walk != NIL)
        {
            if (size == walk->size)
                return walk->queueHead;
            else if (size > walk->size)
                walk = walk->right;
            else // size < walk->size
            {
                bestMatch = walk;
                walk = walk->left;
            }
        }
        //validateTree();
        if (bestMatch != NIL)
            return bestMatch->queueHead;

        return NULL;
    }
#endif

    static void check()
    {
        //for (S32 i = 0; i < mNumUsedHeaders; i++) {
        //   MemoryHeader* probe = &mHeaderBase[i];
        //   if (probe->flags & Allocated && probe->ptr->header != probe)
        //      Platform::debugBreak();
        //}
    }

    struct MemDumpLog
    {
        U32 size;
        U32 count;
        U32 depthTotal;
        U32 maxDepth;
        U32 minDepth;
    };

    void logDumpTraverse(MemDumpLog* sizes, TreeNode* header, U32 depth)
    {
        if (header == NIL)
            return;
        MemDumpLog* mySize = sizes;
        while (mySize->size < header->size)
            mySize++;

        U32 cnt = 0;
        for (FreeHeader* walk = header->queueHead; walk; walk = walk->nextQueue)
            cnt++;
        mySize->count += cnt;
        mySize->depthTotal += depth * cnt;
        mySize->maxDepth = depth > mySize->maxDepth ? depth : mySize->maxDepth;
        mySize->minDepth = depth < mySize->minDepth ? depth : mySize->minDepth;
        logDumpTraverse(sizes, header->left, depth + 1);
        logDumpTraverse(sizes, header->right, depth + 1);
    }

#ifdef TORQUE_DEBUG
    ConsoleFunction(ValidateMemory, void, 1, 1, "ValidateMemory();")
    {
        argc; argv;
        validate();
    }
#endif

    ConsoleFunction(FreeMemoryDump, void, 1, 1, "FreeMemoryDump();")
    {
        argc; argv;
        U32 startSize = 16;
        MemDumpLog memSizes[20];
        U32 i;
        for (i = 0; i < 20; i++)
        {
            memSizes[i].size = startSize << i;
            memSizes[i].count = 0;
            memSizes[i].depthTotal = 0;
            memSizes[i].maxDepth = 0;
            memSizes[i].minDepth = 1000;
        }
        memSizes[19].size = MaxAllocationAmount;
        logDumpTraverse(memSizes, gFreeTreeRoot, 1);
        MemDumpLog fullMem;
        fullMem.count = 0;
        fullMem.depthTotal = 0;
        fullMem.maxDepth = 0;
        fullMem.minDepth = 1000;

        for (i = 0; i < 20; i++)
        {
            if (memSizes[i].count)
                Con::printf("Size: %d - Free blocks: %d  Max Depth: %d  Min Depth: %d  Average Depth: %f",
                    memSizes[i].size, memSizes[i].count, memSizes[i].maxDepth, memSizes[i].minDepth,
                    F32(memSizes[i].depthTotal) / F32(memSizes[i].count));

            fullMem.count += memSizes[i].count;
            fullMem.depthTotal += memSizes[i].depthTotal;
            fullMem.maxDepth = memSizes[i].maxDepth > fullMem.maxDepth ? memSizes[i].maxDepth : fullMem.maxDepth;
            fullMem.minDepth = memSizes[i].minDepth < fullMem.minDepth ? memSizes[i].minDepth : fullMem.minDepth;
        }
        Con::printf("Total free blocks: %d  Max Depth: %d  Min Depth: %d  Average Depth: %f",
            fullMem.count, fullMem.maxDepth, fullMem.minDepth, F32(fullMem.depthTotal) / F32(fullMem.count));
    }

#ifdef TORQUE_DEBUG_GUARD

    void flagCurrentAllocs()
    {
#ifdef TORQUE_ENABLE_PROFILE_PATH
        if (gProfiler && !gProfiler->isEnabled())
        {
            gProfiler->enable(true);
            // warm it up
            gProfiler->dumpToConsole();
        }
#endif
        PageRecord* walk;
        for (walk = gPageList; walk; walk = walk->prevPage) {
            for (Header* probe = walk->headerList; probe; probe = probe->next)
                if (probe->flags & Allocated) {
                    AllocatedHeader* pah = (AllocatedHeader*)probe;
                    pah->flags |= DebugFlag;
                }
        }
    }

    ConsoleFunction(FlagCurrentAllocs, void, 1, 1, "FlagCurrentAllocs();")
    {
        argc; argv;
        flagCurrentAllocs();
    }

    void dumpUnflaggedAllocs(const char* filename)
    {
        FileStream fws;
        bool useFile = filename && *filename;;
        if (useFile)
            useFile = fws.open(filename, FileStream::Write);
        char buffer[1024];

        PageRecord* walk;
        for (walk = gPageList; walk; walk = walk->prevPage)
        {
            for (Header* probe = walk->headerList; probe; probe = probe->next)
            {
                if (probe->flags & Allocated)
                {
                    AllocatedHeader* pah = (AllocatedHeader*)probe;
                    if (!(pah->flags & DebugFlag))
                    {
                        // If you want to extract further information from an unflagged
                        // memory allocation, do the following:
                        // U8 *foo = (U8 *)pah;
                        // foo += sizeof(Header);
                        // FooObject *obj = (FooObject *)foo;
                        dSprintf(buffer, 1023, "%s   %d   %d   %d",
                            pah->fileName != NULL ? pah->fileName : "Undetermined",
                            pah->line, pah->realSize, pah->allocNum);
                        if (useFile)
                        {
                            fws.write(dStrlen(buffer), buffer);
                            fws.write(2, "\r\n");
                        }
                        else
                            Con::errorf("%s", buffer);
#ifdef TORQUE_ENABLE_PROFILE_PATH
                        static char line[4096];
                        dSprintf(line, sizeof(line), "   %s\r\nreal size=%d",
                            pah->profilePath ? pah->profilePath : "unknown",
                            pah->realSize);

                        if (useFile)
                        {
                            fws.write(dStrlen(line), line);
                            fws.write(2, "\r\n");
                        }
                        else
                            Con::errorf(line);
#endif
                    }
                }
            }
        }

        if (useFile)
            fws.close();
    }

    ConsoleFunction(DumpUnflaggedAllocs, void, 1, 2, "DumpUnflaggedAllocs(filename [optional, if not specified dumps to console]);")
    {
        argc;
        dumpUnflaggedAllocs(argc == 2 ? argv[1] : NULL);
    }

    static void initLog()
    {
        static const char* sInitString = " --- INIT MEMORY LOG (ACTION): (FILE) (LINE) (SIZE) (ALLOCNUMBER) ---\r\n";

        FileStream fws;
        fws.open(gLogFilename, FileStream::Write);
        fws.write(dStrlen(sInitString), sInitString);
        fws.close();
    }

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void logAlloc(const AllocatedHeader* hdr, S32 memSize)
    {
        FileStream fws;
        fws.open(gLogFilename, FileStream::ReadWrite);
        fws.setPosition(fws.getStreamSize());

        char buffer[1024];
        dSprintf(buffer, 1023, "alloc: %s %d %d %d\r\n",
            hdr->fileName != NULL ? hdr->fileName : "Undetermined",
            hdr->line, memSize, hdr->allocNum);
        fws.write(dStrlen(buffer), buffer);
        fws.close();
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void logRealloc(const AllocatedHeader* hdr, S32 memSize)
    {
        FileStream fws;
        fws.open(gLogFilename, FileStream::ReadWrite);
        fws.setPosition(fws.getStreamSize());

        char buffer[1024];
        dSprintf(buffer, 1023, "realloc: %s %d %d %d\r\n",
            hdr->fileName != NULL ? hdr->fileName : "Undetermined",
            hdr->line, memSize, hdr->allocNum);
        fws.write(dStrlen(buffer), buffer);
        fws.close();
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void logFree(const AllocatedHeader* hdr)
    {
        FileStream fws;
        fws.open(gLogFilename, FileStream::ReadWrite);
        fws.setPosition(fws.getStreamSize());

        char buffer[1024];
        dSprintf(buffer, 1023, "free:  %s %d %d\r\n",
            hdr->fileName != NULL ? hdr->fileName : "Undetermined",
            hdr->line, hdr->allocNum);
        fws.write(dStrlen(buffer), buffer);
        fws.close();
    }
#endif // !defined(TORQUE_DISABLE_MEMORY_MANAGER)

#endif

    void enableLogging(const char* fileName)
    {
        dStrcpy(gLogFilename, fileName);
        gEnableLogging = true;
    }

    void disableLogging()
    {
        gLogFilename[0] = '\0';
        gEnableLogging = false;
    }

//    // CodeReview - this is never called so commented out to save warning.
//    // Do we want to re-enable it?  Might be nice to get leak tracking on
//    // exit...or maybe that is just a problematical feature we shouldn't
//    // worry about.
//    static void shutdown()
//    {
//#ifdef TORQUE_MULTITHREAD
//        Mutex::destroyMutex(gMemMutex);
//#endif
//
//        // write out leaks and such
//
//        const U32 maxNumLeaks = 1024;
//        U32 numLeaks = 0;
//
//        PageRecord* walk;
//#ifdef TORQUE_DEBUG_GUARD
//        AllocatedHeader* pLeaks[maxNumLeaks];
//        for (walk = gPageList; walk; walk = walk->prevPage)
//            for (Header* probe = walk->headerList; probe; probe = probe->next)
//                if ((probe->flags & Allocated) && ((AllocatedHeader*)probe)->fileName != NULL)
//                    pLeaks[numLeaks++] = (AllocatedHeader*)probe;
//
//        if (numLeaks && !gNeverLogLeaks) {
//            if (gAlwaysLogLeaks || Platform::AlertOKCancel("Memory Status", "Memory leaks detected.  Write to memoryLeaks.log?") == true) {
//                char buffer[1024];
//                FileStream logFile;
//                logFile.open("memoryLeaks.log", FileStream::Write);
//
//                for (U32 i = 0; i < numLeaks; i++) {
//                    dSprintf(buffer, 1023, "Leak in %s: %d (%d)\r\n", pLeaks[i]->fileName, pLeaks[i]->line, pLeaks[i]->allocNum);
//                    logFile.write(dStrlen(buffer), buffer);
//                }
//                logFile.close();
//            }
//        }
//#endif
//
//        // then free all the memory pages
//
//        walk = gPageList;
//        while (walk) {
//            PageRecord* prev = walk->prevPage;
//            dRealFree(walk);
//            walk = prev;
//        }
//    }

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static Header* allocMemPage(dsize_t pageSize)
    {
        pageSize += sizeof(Header);
        if (pageSize < MinPageSize)
            pageSize = MinPageSize;
        PageRecord* base = allocPage(pageSize);

        Header* rec = (Header*)base->basePtr;
        base->headerList = rec;

        rec->size = pageSize - sizeof(Header);
        rec->next = NULL;
        rec->prev = NULL;
        rec->flags = 0;
#ifdef TORQUE_DEBUG_GUARD
        setGuard(rec, true);
#endif
#ifdef LOG_PAGE_ALLOCS
        gPageBytesAllocated += pageSize;
        // total bytes allocated so far will be 0 when TORQUE_DEBUG_GUARD is disabled, so convert that into more meaningful string
        const U32 StrSize = 256;
        char strBytesAllocated[StrSize];
        if (gBytesAllocated > 0)
            dSprintf(strBytesAllocated, sizeof(strBytesAllocated), "%i", gBytesAllocated);
        else
            dStrncpy(strBytesAllocated, "unknown - enable TORQUE_DEBUG_GUARD", StrSize);

#ifndef TORQUE_MULTITHREAD // May deadlock.
        // NOTE: This code may be called within Con::_printf, and if that is the case
        // this will infinitly recurse. This is the reason for the code in Con::_printf
        // that sets Con::active to false. -patw
        if (Con::isActive())
            Con::errorf("PlatformMemory: allocating new page, total bytes allocated so far: %s (total bytes in all pages=%i)", strBytesAllocated, gPageBytesAllocated);
#endif
#endif
        return rec;
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void checkUnusedAlloc(FreeHeader* header, U32 size)
    {
        //validate();
        if (header->size >= size + sizeof(Header) + 16)
        {
            U8* basePtr = (U8*)header;
            basePtr += sizeof(Header);
            FreeHeader* newHeader = (FreeHeader*)(basePtr + size);
            newHeader->next = header->next;
            newHeader->prev = (Header*)header;
            header->next = (Header*)newHeader;
            if (newHeader->next)
                newHeader->next->prev = (Header*)newHeader;
            newHeader->flags = 0;
            newHeader->size = header->size - size - sizeof(Header);
            header->size = size;
#ifdef TORQUE_DEBUG_GUARD
            setGuard((Header*)newHeader, true);
#endif
            treeInsert(newHeader);
        }
    }
#endif

#if defined(TORQUE_MULTITHREAD) && !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static bool gReentrantGuard = false;
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void* alloc(dsize_t size, bool array, const char* fileName, const U32 line)
    {
        //   These should be true, but seem to simply give false errors for people?
        //   AssertFatal(sizeof(Header) == sizeof(AllocatedHeader), "Memory manager header structure size mismatch (Header<->AllocatedHeader)");
        //   AssertFatal(sizeof(Header) == sizeof(FreeHeader),      "Memory manager header structure size mismatch (Header<->FreeHeader)");

#ifdef TORQUE_MULTITHREAD
        if (!gMemMutex && !gReentrantGuard)
        {
            gReentrantGuard = true;
            gMemMutex = Mutex::createMutex();
            gReentrantGuard = false;
        }

        if (!gReentrantGuard)
            Mutex::lockMutex(gMemMutex);
#endif

        fileName, line;
        AssertFatal(size < MaxAllocationAmount, "Size error.");
        //validate();
        if (size == 0)
        {
            size = 1;
        }

#ifndef TORQUE_ENABLE_PROFILE_PATH
        // Note: will cause crash if profile path is on
        PROFILE_START(MemoryAlloc);
#endif

#ifdef TORQUE_DEBUG_GUARD
        // if we're guarding, round up to the nearest DWORD
        size = ((size + 3) & ~0x3);
#else
        // round up size to nearest 16 byte boundary (cache lines and all...)
        size = ((size + 15) & ~0xF);
#endif
        FreeHeader* header = treeFindSmallestGreaterThan(size);
        if (header)
            treeRemove(header);
        else
            header = (FreeHeader*)allocMemPage(size);

        // ok, see if there's enough room in the block to make another block
        // for this to happen it has to have enough room for a header
        // and 16 more bytes.

        U8* basePtr = (U8*)header;
        basePtr += sizeof(Header);

        checkUnusedAlloc(header, size);

        AllocatedHeader* retHeader = (AllocatedHeader*)header;
        retHeader->flags = array ? (Allocated | Array) : Allocated;
#ifdef TORQUE_DEBUG_GUARD
        retHeader->line = line;
        retHeader->fileName = fileName;
        retHeader->allocNum = gCurrAlloc;
        retHeader->realSize = size;
#ifdef TORQUE_ENABLE_PROFILE_PATH
        retHeader->profilePath = gProfiler ? gProfiler->getProfilePath() : "pre";
#endif
        gBytesAllocated += size;
        //static U32 skip = 0;
        //if ((++skip % 1000) == 0)
        //   Con::printf("new=%i, newnew=%i, imagenew=%i",gBytesAllocated,gNewNewTotal,gImageAlloc);
        if (gEnableLogging)
            logAlloc(retHeader, size);
#endif
        if (gCurrAlloc == gBreakAlloc && gBreakAlloc != 0xFFFFFFFF)
            Platform::debugBreak();
        gCurrAlloc++;
#ifndef TORQUE_ENABLE_PROFILE_PATH
        PROFILE_END();
#endif
        //validate();

#ifdef TORQUE_DEBUG
   // fill the block with the fill value.  although this is done in free(), that won't fill
   // newly allocated MM memory (which hasn't been freed yet).  We use a different fill value
   // to diffentiate filled freed memory from filled new memory; this may aid debugging.
#ifndef TORQUE_ENABLE_PROFILE_PATH
        PROFILE_START(stompMem1);
#endif
        dMemset(basePtr, 0xCF, size);
#ifndef TORQUE_ENABLE_PROFILE_PATH
        PROFILE_END();
#endif
#endif

#ifdef TORQUE_MULTITHREAD
        if (!gReentrantGuard)
            Mutex::unlockMutex(gMemMutex);
#endif

        return basePtr;
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void free(void* mem, bool array)
    {
        //validate();
        if (!mem)
            return;

#ifdef TORQUE_MULTITHREAD
        if (!gMemMutex)
            gMemMutex = Mutex::createMutex();

        Mutex::lockMutex(gMemMutex);
#endif

        PROFILE_START(MemoryFree);
        AllocatedHeader* hdr = ((AllocatedHeader*)mem) - 1;

        AssertFatal(hdr->flags & Allocated, avar("Not an allocated block!"));
        AssertFatal(((bool)((hdr->flags & Array) == Array)) == array, avar("Array alloc mismatch. "));

#ifdef TORQUE_DEBUG_GUARD
        gBytesAllocated -= hdr->realSize;
        if (gEnableLogging)
            logFree(hdr);
#endif

        hdr->flags = 0;

        // fill the block with the fill value

#ifdef TORQUE_DEBUG
#ifndef TORQUE_ENABLE_PROFILE_PATH
        PROFILE_START(stompMem2);
#endif
        dMemset(mem, 0xCE, hdr->size);
#ifndef TORQUE_ENABLE_PROFILE_PATH
        PROFILE_END();
#endif
#endif

        // see if we can merge hdr with the block after it.

        Header* next = hdr->next;
        if (next && next->flags == 0)
        {
            treeRemove((FreeHeader*)next);
            hdr->size += next->size + sizeof(Header);
            hdr->next = next->next;
            if (next->next)
                next->next->prev = (Header*)hdr;
        }

        // see if we can merge hdr with the block before it.
        Header* prev = hdr->prev;

        if (prev && prev->flags == 0)
        {
            treeRemove((FreeHeader*)prev);
            prev->size += hdr->size + sizeof(Header);
            prev->next = hdr->next;
            if (hdr->next)
                hdr->next->prev = prev;

            hdr = (AllocatedHeader*)prev;
        }

        // throw this puppy into the tree!
        treeInsert((FreeHeader*)hdr);
        PROFILE_END();

        //   validate();

#ifdef TORQUE_MULTITHREAD
        Mutex::unlockMutex(gMemMutex);
#endif
    }
#endif

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
    static void* realloc(void* mem, dsize_t size)
    {
        //validate();
        if (!size) {
            free(mem, false);
            return NULL;
        }
        if (!mem)
            return alloc(size, false, NULL, 0);

#ifdef TORQUE_MULTITHREAD
        if (!gMemMutex)
            gMemMutex = Mutex::createMutex();

        Mutex::lockMutex(gMemMutex);
#endif

        AllocatedHeader* hdr = ((AllocatedHeader*)mem) - 1;

        AssertFatal((hdr->flags & Allocated) == Allocated, "Bad block flags.");

        size = (size + 0xF) & ~0xF;

        U32 oldSize = hdr->size;
        if (oldSize == size)
        {
#ifdef TORQUE_MULTITHREAD
            Mutex::unlockMutex(gMemMutex);
#endif
            return mem;
        }
        PROFILE_START(MemoryRealloc);

        FreeHeader* next = (FreeHeader*)hdr->next;

#ifdef TORQUE_DEBUG_GUARD
        // adjust header size and allocated bytes size
        hdr->realSize += size - oldSize;
        gBytesAllocated += size - oldSize;
        if (gEnableLogging)
            logRealloc(hdr, size);
#endif
        if (next && !(next->flags & Allocated) && next->size + hdr->size + sizeof(Header) >= size)
        {
            // we can merge with the next dude.
            treeRemove(next);
            hdr->size += sizeof(Header) + next->size;
            hdr->next = next->next;
            if (next->next)
                next->next->prev = (Header*)hdr;

            checkUnusedAlloc((FreeHeader*)hdr, size);
            //validate();
            PROFILE_END();
#ifdef TORQUE_MULTITHREAD
            Mutex::unlockMutex(gMemMutex);
#endif
            return mem;
        }
        else if (size < oldSize)
        {
            checkUnusedAlloc((FreeHeader*)hdr, size);
            PROFILE_END();
#ifdef TORQUE_MULTITHREAD
            Mutex::unlockMutex(gMemMutex);
#endif
            return mem;
        }
#ifdef TORQUE_DEBUG_GUARD
        // undo above adjustment because we're going though alloc instead
        hdr->realSize -= size - oldSize;
        gBytesAllocated -= size - oldSize;
#endif
        void* ret = alloc(size, false, NULL, 0);
        dMemcpy(ret, mem, oldSize);
        free(mem, false);
        PROFILE_END();
#ifdef TORQUE_MULTITHREAD
        Mutex::unlockMutex(gMemMutex);
#endif
        return ret;
    }
#endif

    dsize_t getMemoryUsed()
    {
        U32 size = 0;

        PageRecord* walk;
        for (walk = gPageList; walk; walk = walk->prevPage) {
            for (Header* probe = walk->headerList; probe; probe = probe->next)
                if (probe->flags & Allocated) {
                    size += probe->size;
                }
        }

        return size;
    }

#ifdef TORQUE_DEBUG_GUARD
    ConsoleFunction(dumpMemSnapshot, void, 2, 2, "(string fileName) Dump a snapshot of current memory to a file.")
    {
        argc;
        FileStream fws;
        fws.open(argv[1], FileStream::Write);
        char buffer[1024];

        PageRecord* walk;
        for (walk = gPageList; walk; walk = walk->prevPage) {
            for (Header* probe = walk->headerList; probe; probe = probe->next)
                if (probe->flags & Allocated) {
                    AllocatedHeader* pah = (AllocatedHeader*)probe;
                    dSprintf(buffer, 1023, "%s\t%d\t%d\t%d\r\n",
                        pah->fileName != NULL ? pah->fileName : "Undetermined",
                        pah->line, pah->realSize, pah->allocNum);
                    fws.write(dStrlen(buffer), buffer);
                }
        }

        fws.close();
    }
#endif

    dsize_t getMemoryAllocated()
    {
        return 0;
    }

    void setBreakAlloc(U32 breakAlloc)
    {
        gBreakAlloc = breakAlloc;
    }

    ConsoleFunctionGroupEnd(Memory);

} // namespace Memory

void setMinimumAllocUnit(U32 allocUnit)
{
    AssertFatal(isPow2(allocUnit) && allocUnit > (2 << 20),
        "Error, allocunit must be a power of two, and greater than 2 megs");

    MinPageSize = allocUnit;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <stdlib.h>

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)

// Manage our own memory, add overloaded memory operators and functions

void* FN_CDECL operator new(dsize_t size, const char* fileName, const U32 line)
{
    return Memory::alloc(size, false, fileName, line);
}

void* FN_CDECL operator new[](dsize_t size, const char* fileName, const U32 line)
{
    return Memory::alloc(size, true, fileName, line);
}

void* FN_CDECL operator new(dsize_t size)
{
    return Memory::alloc(size, false, NULL, 0);
}

void* FN_CDECL operator new[](dsize_t size)
{
    return Memory::alloc(size, true, NULL, 0);
}

void FN_CDECL operator delete(void* mem)
{
    Memory::free(mem, false);
}

void FN_CDECL operator delete[](void* mem)
{
    Memory::free(mem, true);
}

void* dMalloc_r(dsize_t in_size, const char* fileName, const dsize_t line)
{
    return Memory::alloc(in_size, false, fileName, line);
}

void dFree(void* in_pFree)
{
    Memory::free(in_pFree, false);
}

void* dRealloc(void* in_pResize, dsize_t in_size)
{
    return Memory::realloc(in_pResize, in_size);
}

#else

// Don't manage our own memory

void* dMalloc_r(dsize_t in_size, const char* fileName, const dsize_t line)
{
    return malloc(in_size);
}

void dFree(void* in_pFree)
{
    free(in_pFree);
}

void* dRealloc(void* in_pResize, dsize_t in_size)
{
    return realloc(in_pResize, in_size);
}

#endif // #else
