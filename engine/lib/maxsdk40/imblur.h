/**********************************************************************
 *<
	FILE: imblur.h

	DESCRIPTION:  Defines Interface for Image Motion Blur.  This interface is
		implemented in the Effect plugin MotionBlur.dlv, which must be present to use it.
		This interface is does NOT support scripting, only direct calling.

	CREATED BY: Dan Silva

	HISTORY: created 7 March 2000

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

//------------------------------------------------------------------
// Usage example
//		ClassDesc2* mbcd = GET_MBLUR_CD;
//		if (mbcd) {
//			IMBOps* imb = GetIMBInterface(mbcd);
//			imb->ApplyMotionBlur(bm, &imbcb, 1.2f);
//			}
//------------------------------------------------------------------


#ifndef __IMBLUR__ 

#define __IMBLUR__

#include "iFnPub.h"
#include "iparamb2.h"

// Class ID for the motion blur Effect
#define MBLUR_CLASS_ID Class_ID(0x86c92d3, 0x601af384) 

// Interface ID for the motion blur function interface ( IMBOps) 
#define IMB_INTERFACE Interface_ID(0x2A3764C1,0x9C96F51)

// Use this to get ClassDesc2 that implements interface.
#define GET_MBLUR_CD (ClassDesc2*)GetCOREInterface()->GetDllDir().ClassDir().FindClass(RENDER_EFFECT_CLASS_ID, MBLUR_CLASS_ID)

// Use this to get IMBOps interface from the ClassDesc2.
#define GetIMBInterface(cd) (IMBOps *)(cd)->GetInterface(IMB_INTERFACE)

// Flags values
#define IMB_TRANSP  1  // controls whether motion blur works through transparency.  Setting it to 0 saves memory, runs faster.
 
class IMBOps: public FPStaticInterface {
	public: 
	virtual ULONG ChannelsRequired(ULONG flags=0)=0;
	virtual int ApplyMotionBlur(Bitmap *bm, CheckAbortCallback *progCallback=NULL,  
		float duration=1.0f,  ULONG flags=IMB_TRANSP, Bitmap *extraBM=NULL)=0;
	};



#endif // __IMBLUR__
