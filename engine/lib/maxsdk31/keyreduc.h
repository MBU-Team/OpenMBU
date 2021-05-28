/**********************************************************************
 *<
	FILE: keyreduc.h

	DESCRIPTION:  Key reduction

	CREATED BY: Rolf Berteig

	HISTORY: created 9/30/95

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __KEYREDUC__
#define __KEYREDUC__

#define DEFULAT_KEYREDUCE_THRESHOLD		(0.5f)

// Values returned from Progress
#define KEYREDUCE_ABORT		-1		// Stops processing and undoes any key reduction
#define KEYREDUCE_STOP		0		// Stops processing, but keeps any reduction done so far
#define KEYREDUCE_CONTINUE	1		// Keeps going.

// A callback so progress can be made during key reduction
class KeyReduceStatus {
	public:
		// Called once before reduction starts. 'total' is the number
		// reduction canidate keys.
		virtual void Init(int total)=0;

		// Called every now and again. 'p' is the number of keys
		// processed. So % done is p/total * 100.		
		virtual int Progress(int p)=0;
	};

// Attempts to delete keys that lie within the given time range.
// The controller will be sampled within the range in 'step' size
// increments. After the key reduction, the controller's values
// at each step are gauranteed to be withen 'threshold' distance
// from their original values.
//
CoreExport int ApplyKeyReduction(
		Control *cont,Interval range,float thresh,TimeValue step,
		KeyReduceStatus *status);


#endif  //__KEYREDUC__

