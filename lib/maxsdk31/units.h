/**********************************************************************
 *<
	FILE: units.h

	DESCRIPTION: Include file for real-world units support

	CREATED BY: Tom Hudson

	HISTORY:

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef _UNITS_H_

#define _UNITS_H_

// The unit types we support
#define UNITS_INCHES		0
#define UNITS_FEET			1
#define UNITS_MILES			2
#define UNITS_MILLIMETERS	3
#define UNITS_CENTIMETERS	4
#define UNITS_METERS		5
#define UNITS_KILOMETERS	6

#define MAX_UNIT_TYPES 7

// The unit display types
#define UNITDISP_GENERIC	0
#define UNITDISP_METRIC		1
#define UNITDISP_US			2
#define UNITDISP_CUSTOM		3

#define MAX_UNITDISP_TYPES 4

// Metric display options
#define UNIT_METRIC_DISP_MM	0
#define UNIT_METRIC_DISP_CM	1
#define UNIT_METRIC_DISP_M	2
#define UNIT_METRIC_DISP_KM	3

#define MAX_METRIC_DISP_TYPES 4

// US display options
#define UNIT_US_DISP_FRAC_IN	0
#define UNIT_US_DISP_DEC_IN		1
#define UNIT_US_DISP_FRAC_FT	2
#define UNIT_US_DISP_DEC_FT		3
#define UNIT_US_DISP_FT_FRAC_IN	4
#define UNIT_US_DISP_FT_DEC_IN	5

#define MAX_US_DISP_TYPES 6

// US display options
#define UNIT_FRAC_1_1	0
#define UNIT_FRAC_1_2	1
#define UNIT_FRAC_1_4	2
#define UNIT_FRAC_1_8	3
#define UNIT_FRAC_1_10	4
#define UNIT_FRAC_1_16	5
#define UNIT_FRAC_1_32	6
#define UNIT_FRAC_1_64	7
#define UNIT_FRAC_1_100	8

#define MAX_FRAC_TYPES 9

// Units designator types
#define UNIT_DESIG_MM		0
#define UNIT_DESIG_CM		1
#define UNIT_DESIG_M		2
#define UNIT_DESIG_KM		3
#define UNIT_DESIG_IN		4
#define UNIT_DESIG_FT		5
#define UNIT_DESIG_CUSTOM	6

#define UNIT_DESIG_TYPES	 7

// Display information structure

typedef struct {
	int dispType;				// Display type	(UNITDISP_GENERIC, UNITDISP_METRIC, UNITDISP_US, UNITDISP_CUSTOM)
	int metricDisp;				// Metric display option
	int usDisp;					// US display option
	int usFrac;					// US fraction option
	TSTR customName;			// Custom unit name
	float customValue;			// Custom unit value
	int customUnit;				// Custom unit reference
	} DispInfo;

// Get the master scale in terms of the specified unit type
// i.e. GetMasterScale(UNITS_INCHES) gives number of inches per unit
// Returns -1.0 if invalid unit type supplied
double CoreExport GetMasterScale(int type);

void CoreExport GetMasterUnitInfo(int *type,float *scale);
int CoreExport SetMasterUnitInfo(int type,float scale);
void CoreExport GetUnitDisplayInfo(DispInfo *info);
int CoreExport SetUnitDisplayInfo(DispInfo *info);
int CoreExport GetUnitDisplayType();
int CoreExport SetUnitDisplayType(int type);
BOOL CoreExport IsValidUnitCharacter(int ch);	// Returns TRUE if character valid for unit type

// The US unit defaults
#define US_UNIT_DEFAULT_FEET 0
#define US_UNIT_DEFAULT_INCHES 1

// Get/set the default units for US entry fields -- Inches or feet
CoreExport void SetUSDefaultUnit(int type);
CoreExport int GetUSDefaultUnit();

TCHAR CoreExport *GetFirstUnitName();
TCHAR CoreExport *GetNextUnitName();
TCHAR CoreExport *GetFirstMetricDisp();
TCHAR CoreExport *GetNextMetricDisp();
TCHAR CoreExport *GetFirstUSDisp();
TCHAR CoreExport *GetNextUSDisp();
TCHAR CoreExport *GetFirstFraction();
TCHAR CoreExport *GetNextFraction();
TCHAR CoreExport *FormatUniverseValue(float value);
float CoreExport DecodeUniverseValue(TCHAR *string, BOOL *valid = NULL);


//
// Time units
//

// Ways to display time
enum TimeDisp {
	DISPTIME_FRAMES,
	DISPTIME_SMPTE,
	DISPTIME_FRAMETICKS,
	DISPTIME_TIMETICKS
	};

// Formats a time value into a string based on the current frame rate, etc.
void CoreExport TimeToString(TimeValue t,TSTR &string);
BOOL CoreExport StringToTime(TSTR string,TimeValue &t);

CoreExport int GetFrameRate();
CoreExport void SetFrameRate(int rate);
CoreExport int GetTicksPerFrame();
CoreExport void SetTicksPerFrame(int ticks);
CoreExport TimeDisp GetTimeDisplayMode();
CoreExport void SetTimeDisplayMode(TimeDisp m);
CoreExport int LegalFrameRate(int r);

#ifdef DESIGN_VER
TCHAR CoreExport *FormatUniverseValue(TCHAR *value);
#endif

#endif // _UNITS_H_
