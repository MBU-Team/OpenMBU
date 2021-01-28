/**********************************************************************
 *<
	FILE: toneop.h

	DESCRIPTION: Definitions for tone operator. Tone operators are used
	             to map high dynamic range images into RGB. Usually they
	             are used with physical energy values.

	CREATED BY: Cleve Ard

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/


#ifndef __TONEOP_H__
#define __TONEOP_H__

#include <imtl.h>
#include <render.h>

// The tone operator uses the standard Special Effects
// parameter dialog class for its UI.
typedef SFXParamDlg ToneOpParamDlg;

/*=====================================================================
 * Tone Operator Interface class
 *===================================================================*/

class ToneOperator : public SpecialFX {
public:

	ToneOperator() { SetAFlag(A_TONEOP_PROCESS_BG); }

	// Standard methods from ReferenceMaker and Animatable

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
		PartID& partID,  RefMessage message) {return REF_SUCCEED;}
	SClass_ID SuperClassID() { return TONE_OPERATOR_CLASS_ID; }
	
	// Saves and loads name. These should be called at the start of
	// a plug-in's save and load methods.
	IOResult Save(ISave *isave) { return SpecialFX::Save(isave); }
	IOResult Load(ILoad *iload) { return SpecialFX::Load(iload); }

	virtual void SetActive(
		bool		active,
		TimeValue	t
	) {
		if (active ^ (TestAFlag(A_TONEOP_DISABLED) == 0)) {
			if (active) {
				ClearAFlag(A_TONEOP_DISABLED);
			}
			else {
				SetAFlag(A_TONEOP_DISABLED);
			}
			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}
	}

	bool GetProcessBackground() { return TestAFlag(A_TONEOP_PROCESS_BG) != 0; }
	void SetProcessBackground(bool active) {
		if (active ^ (TestAFlag(A_TONEOP_PROCESS_BG) != 0)) {
			if (active) {
				SetAFlag(A_TONEOP_PROCESS_BG);
			}
			else {
				ClearAFlag(A_TONEOP_PROCESS_BG);
			}
			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}
	}

	// UI Access

	// Put up a modal dialog that lets the user edit the plug-ins parameters.
	virtual ToneOpParamDlg *CreateParamDialog(IRendParams *ip) { return NULL; }

	// Implement this if you are using the ParamMap2 AUTO_UI system and the 
	// atmosphere has secondary dialogs that don't have the effect as their 'thing'.
	// Called once for each secondary dialog for you to install the correct thing.
	// Return TRUE if you process the dialog, false otherwise.
	virtual BOOL SetDlgThing(ToneOpParamDlg* dlg) { return FALSE; }

	// Render access

	// Does this tone operator really map physical values to RGB. This method
	// is provided so shaders can determine whether the shading calculations
	// are in physical or RGB space.
	virtual bool IsPhysicalSpace() const { return true; }

	// This method is called once per frame when the renderer begins. This
	// gives the atmospheric or rendering effect the chance to cache any
	// values it uses internally so they don't have to be computed on
	// every pixel. But, this method should not be used to perform any very
	// long tasks, such as sampling the environment to calculate a
	// mapping histogram. This would be the likely method that caches the
	// frames physical scaling value.
	virtual void Update(TimeValue t, Interval& valid) { }

	// This method is called for the operator to do any work it needs
	// to do prior to rendering. Rendering using the RenderMapsContext
	// uses the identity tone operator.
	virtual bool BuildMaps(TimeValue t, RenderMapsContext& rmc)
		{ return true; }

	// This method is called during subrenders to give the tone operator
	// a chance to sample the image with full dynamic range. If your operator
	// needs to sample the image, you can set a flag so you know when you are
	// sampling.
	virtual void SubRenderSample(float energy[3]) { }

	// Map an scaled energy value into RGB. The first version of the
	// method converts a color value and the second converts a monochrome
	// value. The converted color value is stored in <i>energy</i>.
	// The converted monochrome value is returned.
	// This method assumes that Update has been called to cache the
	// various values needed by the tone operator.
	// By using a float array to pass in color values, we can use the same
	// routine to handle the various classes used to store color information,
	// for example, Color, AColor and Point3. The red, green and blue
	// components are stored in that order in the array.
	virtual void ScaledToRGB(float energy[3]) = 0;
	virtual float ScaledToRGB(float energy) = 0;

	// Get and Set the Physical value that is scaled to 1.
	virtual float GetPhysicalUnit(
		TimeValue	t,
		Interval&	valid = Interval(0,0)
	) const = 0;
	virtual void SetPhysicalUnit(
		float		value,
		TimeValue	t
	) = 0;

	// Scale physical values so they can be used in the renderer. The
	// first version of the method converts a color value and the second
	// converts a monochrome value. The converted color value is stored
	// in <i>energy</i>. The converted monochrome value is returned.
	// This method assumes that Update has been called to cache the
	// various values needed by the tone operator.
	// By using a float array to pass in color values, we can use the same
	// routine to handle the various classes used to store color information,
	// for example, Color, AColor and Point3. The red, green and blue
	// components are stored in that order in the array.
	virtual void ScalePhysical(float energy[3]) const = 0;
	virtual float ScalePhysical(float energy) const = 0;

	// Scale RGB values, just supplied to invert ScalePhysical. The first
	// version of the method converts a color value and the second
	// converts a monochrome value. The converted color value is stored
	// in <i>energy</i>. The converted monochrome value is returned.
	// This method assumes that Update has been called to cache the
	// various values needed by the tone operator.
	// By using a float array to pass in color values, we can use the same
	// routine to handle the various classes used to store color information,
	// for example, Color, AColor and Point3. The red, green and blue
	// components are stored in that order in the array.
	virtual void ScaleRGB(float color[3]) const = 0;
	virtual float ScaleRGB(float color) const = 0;
};


// Does this tone operator really map physical values to RGB. This method
// is provided so shaders can determine whether the shading calculations
// are in physical or RGB space.
inline bool ShadeContext::IsPhysicalSpace() const
	{ return globContext != NULL && globContext->pToneOp != NULL
		&& globContext->pToneOp->IsPhysicalSpace( ); }

// Map an scaled energy value into RGB. The first version of the
// method converts a color value and the second converts a monochrome
// value. The converted color value is stored in <i>energy</i>.
// The converted monochrome value is returned.
inline float ShadeContext::ScaledToRGB( float energy ) const
	{ return globContext != NULL && globContext->pToneOp != NULL
		? energy : globContext->pToneOp->ScaledToRGB( energy ); }

// Map an energy value int out.c into RGB. The converted value is stored in
// out.c.
inline void ShadeContext::ScaledToRGB( )
	{ ScaledToRGB( out.c ); }

// Scale physical values so they can be used in the renderer. The
// first version of the method converts a color value and the second
// converts a monochrome value. The converted color value is stored
// in <i>energy</i>. The converted monochrome value is returned.
inline float ShadeContext::ScalePhysical(float energy) const
	{ return globContext != NULL && globContext->pToneOp != NULL
		? energy : globContext->pToneOp->ScalePhysical( energy ); }

// Scale RGB values, just supplied to invert ScalePhysical. The first
// version of the method converts a color value and the second
// converts a monochrome value. The converted color value is stored
// in <i>energy</i>. The converted monochrome value is returned.
inline float ShadeContext::ScaleRGB(float energy) const
	{ return globContext != NULL && globContext->pToneOp != NULL
		? energy : globContext->pToneOp->ScaleRGB( energy ); }


/*=====================================================================
 * Tone Operator Core Interface class
 *===================================================================*/

// This class is used to get access to the tone operator and
// its UI.
#define TONE_OPERATOR_INTERFACE	Interface_ID(0x1563269c, 0x7ec41d84)

class ToneOperatorInterface : public FPStaticInterface {
public:
	typedef void (*ToneChangeCallback)(
		ToneOperator*  newOp,
		ToneOperator*  oldOp,
		void*          param
	);

	// Get and Set the tone operator in the scene
	virtual ToneOperator* GetToneOperator() const = 0;
	virtual void SetToneOperator(ToneOperator* op) = 0;

	virtual void RegisterToneOperatorChangeNotification(
		ToneChangeCallback   callback,
		void*                param
	) = 0;
	virtual void UnRegisterToneOperatorChangeNotification(
		ToneChangeCallback   callback,
		void*                param
	) = 0;
};


#endif
