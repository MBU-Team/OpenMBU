//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PROFILER_H_
#define _PROFILER_H_

#include "core/torqueConfig.h"

#ifdef TORQUE_ENABLE_PROFILER

struct ProfilerData;
struct ProfilerRootData;
/// The Profiler is used to see how long a specific chunk of code takes to execute.
/// All values outputted by the profiler are percentages of the time that it takes
/// to run entire main loop.
///
/// First, you must #define TORQUE_ENABLE_PROFILER in profiler.h in order to
/// active it.  Examples of script use:
/// @code
/// //enables or disables profiling.  Data is only gathered when the profiler is enabled.
/// profilerEnable(bool enable);
/// profilerReset();                                        //resets the data gathered by the profiler
/// profilerDump();                                         //dumps all profiler data to the console
/// profilerDumpToFile(string filename);                    //dumps all profiler data to a given file
/// profilerMarkerEnable((string markerName, bool enable);  //enables or disables a given profile tag
/// @endcode
///
/// The C++ code side of the profiler uses pairs of PROFILE_START() and PROFILE_END().
///
/// When using these macros, make sure there is a PROFILE_END() for every PROFILE_START
/// and a PROFILE_START() for every PROFILE_END().  It is fine to nest these macros, however,
/// you must make sure that no matter what execution path the code takes, the PROFILE macros
/// will be balanced.
///
/// The profiler can be used to locate areas of code that are slow or should be considered for
/// optimization.  Since it tracks the relative time of execution of that code to the execution
/// of the main loop, it is possible to benchmark any given code to see if changes made will
/// actually improve performance.
///
/// Here are some examples:
/// @code
/// PROFILE_START(TerrainRender);
/// //some code here
/// PROFILE_START(TerrainRenderGridSquare);
/// //some code here
/// PROFILE_END();
/// //possibly some code here
/// PROFILE_END();
/// @endcode
class Profiler
{
    enum {
        MaxStackDepth = 256,
        DumpFileNameLength = 256
    };
    U32 mCurrentHash;

    ProfilerData* mCurrentProfilerData;
    ProfilerData* mProfileList;
    ProfilerData* mRootProfilerData;

    bool mEnabled;
    S32 mStackDepth;
    bool mNextEnable;
    U32 mMaxStackDepth;
    bool mDumpToConsole;
    bool mDumpToFile;
    char mDumpFileName[DumpFileNameLength];
    void dump();
    void validate();
public:
    Profiler();
    ~Profiler();

    /// Reset the data in the profiler
    void reset();
    /// Dumps the profile to console
    void dumpToConsole();
    /// Dumps the profile data to a file
    /// @param fileName filename to dump data to
    void dumpToFile(const char* fileName);
    /// Enable profiling
    void enable(bool enabled);
    bool isEnabled() { return mNextEnable; }
    /// Helper function for macro definition PROFILE_START
    void hashPush(ProfilerRootData* data);
    /// Helper function for macro definition PROFILE_END
    void hashPop();
    /// Enable a profiler marker
    void enableMarker(const char* marker, bool enabled);
#ifdef TORQUE_ENABLE_PROFILE_PATH
    /// Get current profile path
    const char* getProfilePath();
    /// Construct profile path of given profiler data
    const char* constructProfilePath(ProfilerData* pd);
#endif
};

extern Profiler* gProfiler;

struct ProfilerRootData
{
    const char* mName;
    U32 mNameHash;
    ProfilerData* mFirstProfilerData;
    ProfilerRootData* mNextRoot;
    F64 mTotalTime;
    F64 mSubTime;
    U32 mTotalInvokeCount;
    bool mEnabled;

    static ProfilerRootData* sRootList;

    ProfilerRootData(const char* name);
};

struct ProfilerData
{
    ProfilerRootData* mRoot; ///< link to root node.
    ProfilerData* mNextForRoot; ///< links together all ProfilerData's for a particular root
    ProfilerData* mNextProfilerData; ///< links all the profilerDatas
    ProfilerData* mNextHash;
    ProfilerData* mParent;
    ProfilerData* mNextSibling;
    ProfilerData* mFirstChild;
    enum {
        HashTableSize = 32,
    };
    ProfilerData* mChildHash[HashTableSize];
    ProfilerData* mLastSeenProfiler;

    U32 mHash;
    U32 mSubDepth;
    U32 mInvokeCount;
    U32 mStartTime[2];
    F64 mTotalTime;
    F64 mSubTime;
#ifdef TORQUE_ENABLE_PROFILE_PATH
    const char* mPath;
#endif
};


#define PROFILE_START(name) \
static ProfilerRootData pdata##name##obj (#name); \
if(gProfiler) gProfiler->hashPush(& pdata##name##obj )

#define PROFILE_END() if(gProfiler) gProfiler->hashPop()

#define PROFILE_END_NAMED(name) if(gProfiler) gProfiler->hashPop(& pdata##name##obj)

class ScopedProfiler {
public:
    ScopedProfiler(ProfilerRootData* data) {
        if (gProfiler) gProfiler->hashPush(data);
   }
    ~ScopedProfiler() {
        if (gProfiler) gProfiler->hashPop();
    }
};

#define PROFILE_SCOPE(name) \
   static ProfilerRootData pdata##name##obj (#name); \
   ScopedProfiler scopedProfiler##name##obj(&pdata##name##obj);

#else
#define PROFILE_START(x)
#define PROFILE_END()
#define PROFILE_SCOPE(x)
#define PROFILE_END_NAMED(x)
#endif

#endif
