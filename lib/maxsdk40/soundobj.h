/**********************************************************************
 *<
	FILE:  soundobj.h

	DESCRIPTION:  Sound plug-in object base class

	CREATED BY:  Rolf Berteig

	HISTORY: created 2 July 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _SOUNDOBJ_H_
#define _SOUNDOBJ_H_

#include <vfw.h>

// IWavePaint
// A SoundObject implements this interface to let the TrackBar
// paint a waveform. 

#define I_WAVEPAINT 94

class IWavePaint {
public:
	virtual void PaintWave(HDC hdc, RECT* rect, Interval i) = 0;
	};

class SoundObj : public ReferenceTarget {
	public:
		virtual SClass_ID SuperClassID() {return SClass_ID(SOUNDOBJ_CLASS_ID);}		

		virtual BOOL Play(TimeValue tStart,TimeValue t0,TimeValue t1,TimeValue frameStep)=0;
		virtual void Scrub(TimeValue t0,TimeValue t1)=0;
		virtual TimeValue Stop()=0;
		virtual TimeValue GetTime()=0;
		virtual BOOL Playing()=0;
		virtual void SaveSound(PAVIFILE pfile,TimeValue t0,TimeValue t1)=0;
		virtual void SetMute(BOOL mute)=0;
		virtual BOOL IsMute()=0;
	};


CoreExport SoundObj *NewDefaultSoundObj();

#endif // _SOUNDOBJ_H_
