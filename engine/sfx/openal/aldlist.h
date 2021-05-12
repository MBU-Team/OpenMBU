//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef ALDEVICELIST_H
#define ALDEVICELIST_H

#pragma warning(disable: 4786)  //disable warning "identifier was truncated to '255' characters in the browser information"
#include "core/tVector.h"
#include "core/stringTable.h"
#include "sfx/openal/sfxALCaps.h"
#include "LoadOAL.h"

typedef struct
{
	char           strDeviceName[256];
	int				iMajorVersion;
	int				iMinorVersion;
	unsigned int	uiSourceCount;
	int iCapsFlags;
	bool			bSelected;
} ALDEVICEINFO, *LPALDEVICEINFO;

class ALDeviceList
{
private:
	OPENALFNTABLE	ALFunction;
	Vector<ALDEVICEINFO> vDeviceInfo;
	int defaultDeviceIndex;
	int filterIndex;

public:
	ALDeviceList ( const OPENALFNTABLE &oalft );
	~ALDeviceList ();
	int GetNumDevices();
	const char *GetDeviceName(int index);
	void GetDeviceVersion(int index, int *major, int *minor);
	unsigned int GetMaxNumSources(int index);
	bool IsExtensionSupported(int index, SFXALCaps caps);
	int GetDefaultDevice();
	void FilterDevicesMinVer(int major, int minor);
	void FilterDevicesMaxVer(int major, int minor);
	void FilterDevicesExtension(SFXALCaps caps);
	void ResetFilters();
	int GetFirstFilteredDevice();
	int GetNextFilteredDevice();

private:
	unsigned int GetMaxNumSources();
};

#endif // ALDEVICELIST_H
