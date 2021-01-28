//-------------------------------------------------------------
// Access to Point Cache
//


#ifndef __IPOINTCACHE__H
#define __IPOINTCACHE__H

#include "iFnPub.h"

#define POINTCACHE_CLASS_ID	Class_ID(0x21d07ae1, 0x48d30bec)
#define POINTCACHEWSM_CLASS_ID	Class_ID(0x21d07ae1, 0x48d30bed)

#define PARTICLECACHE_CLASS_ID	Class_ID(0x21d07ae1, 0x48d30bee)


enum { pointcache_params };

//enums for various parameters
//note pb_record_file is no longer used and legacy
//pb_disable_mods is no longer used and legacy
//pb_fastcache is fast caching scheme but right now can potential cause crashes
// or extremely slow conditions are systems that spawn particles
enum { 
	pb_time,pb_start_time,pb_end_time,pb_samples, pb_disable_mods, pb_cache_file, pb_record_file,pb_relative,pb_strength,pb_fastcache
};




class IPointCache;
class IPointCacheWSM;
class IParticleCache;

//***************************************************************
//Function Publishing System stuff   
//****************************************************************
#define POINTCACHE_INTERFACE Interface_ID(0x53b4409b, 0x18ee7cc8)

#define GetIPointCacheInterface(cd) \
			(IPointCache *)(cd)->GetInterface(POINTCACHE_INTERFACE)

#define POINTCACHEWSM_INTERFACE Interface_ID(0x53b4409b, 0x18ee7cc9)

#define GetIPointCacheWSMInterface(cd) \
			(IPointCacheWSM *)(cd)->GetInterface(POINTCACHEWSM_INTERFACE)

#define PARTICLECACHE_INTERFACE Interface_ID(0x53b4409b, 0x18ee7cd0)

#define GetIParticleCacheInterface(cd) \
			(IParticleCache *)(cd)->GetInterface(PARTICLECACHE_INTERFACE)

enum {  pointcache_record, pointcache_setcache, pointcache_enablemods,pointcache_disablemods


		};
//****************************************************************


class IPointCache :  public FPMixinInterface 
	{
	public:

		//Function Publishing System
		//Function Map For Mixin Interface
		//*************************************************
		BEGIN_FUNCTION_MAP
			VFN_0(pointcache_record, fnRecord);
			VFN_0(pointcache_setcache, fnSetCache);
			VFN_0(pointcache_enablemods, fnEnableMods);
			VFN_0(pointcache_disablemods, fnDisableMods);

		END_FUNCTION_MAP

		FPInterfaceDesc* GetDesc();    // <-- must implement 

		virtual void	fnRecord()=0;
		virtual void	fnSetCache()=0;
		virtual void	fnEnableMods()=0;
		virtual void	fnDisableMods()=0;

	};


class IPointCacheWSM :  public FPMixinInterface 
	{
	public:

		//Function Publishing System
		//Function Map For Mixin Interface
		//*************************************************
		BEGIN_FUNCTION_MAP
			VFN_0(pointcache_record, fnRecord);
			VFN_0(pointcache_setcache, fnSetCache);
			VFN_0(pointcache_enablemods, fnEnableMods);
			VFN_0(pointcache_disablemods, fnDisableMods);

		END_FUNCTION_MAP

		FPInterfaceDesc* GetDesc();    // <-- must implement 

		virtual void	fnRecord()=0;
		virtual void	fnSetCache()=0;
		virtual void	fnEnableMods()=0;
		virtual void	fnDisableMods()=0;

	};

class IParticleCache :  public FPMixinInterface 
	{
	public:

		//Function Publishing System
		//Function Map For Mixin Interface
		//*************************************************
		BEGIN_FUNCTION_MAP
			VFN_0(pointcache_record, fnRecord);
			VFN_0(pointcache_setcache, fnSetCache);

		END_FUNCTION_MAP

		FPInterfaceDesc* GetDesc();    // <-- must implement 

		virtual void	fnRecord()=0;
		virtual void	fnSetCache()=0;

	};


#endif // __IPOINTCACHE__H
