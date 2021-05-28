/**********************************************************************
 *<
	FILE: ILayer.h

	DESCRIPTION: Declaration of the ILayer interface

	CREATED BY:	Peter Sauerbrei

	HISTORY: Created 19 October 1998

 *>	Copyright (c) 1998-99, All Rights Reserved.
 **********************************************************************/

#pragma once
#ifndef __ILAYER_H__
#define __ILAYER_H__
#include <maxtypes.h>

class LayerProperty : public ReferenceTarget
{
private: 
	int m_id;
	TSTR m_name;

public:
	LayerProperty() : m_id(-1), m_name("") {}
	LayerProperty(const TSTR & name, int id) : m_id(id), m_name(name) {}
	virtual ~LayerProperty() {}

	// child methods
	virtual void SetProperty(const int d) = 0;
	virtual void SetProperty(const float d) = 0;
	virtual void SetProperty(const Point3 & d) = 0;
	virtual void SetProperty(const TSTR & d) = 0;
	virtual void SetProperty(void * d) = 0;
	virtual bool GetProperty(int & i) const = 0;
	virtual bool GetProperty(float & f) const = 0;
	virtual bool GetProperty(Point3 & p) const = 0;
	virtual bool GetProperty(TSTR & n) const = 0;
	virtual bool GetProperty(void * v) const = 0;

	// local methods
	int GetID() const { return m_id; }
	void SetID(int id) { m_id = id; }
	TSTR GetName() const { return m_name; }
	void SetName(const TSTR & name) { m_name = name; }
};

class ILayer : public ReferenceTarget
{
public:
	static const SClass_ID kLayerSuperClassID;

	// from Animatable
	SClass_ID SuperClassID() { return kLayerSuperClassID; }

	// from ILayerRecord
	virtual bool AddToLayer(INode * rtarg) = 0;
	virtual bool DeleteFromLayer(INode * rtarg) = 0;
	virtual void SetName(const TSTR & name) = 0;
	virtual TSTR GetName() const = 0;		// user must delete the string
	virtual void SetWireColor(DWORD newcol) = 0;
	virtual DWORD GetWireColor() const = 0;
	virtual void Hide(bool onOff) = 0;
	virtual bool IsHidden() const = 0;
	virtual void Freeze(bool onOff) = 0;
	virtual bool IsFrozen() const = 0;
	virtual void SetRenderable(bool onOff) = 0;
	virtual bool Renderable() const = 0;
	virtual void XRayMtl(bool onOff) = 0;
	virtual bool HasXRayMtl() const = 0;
	virtual void IgnoreExtents(bool onOff) = 0;
	virtual bool GetIgnoreExtents() const = 0;
	virtual void BoxMode(bool onOff) = 0;
	virtual bool GetBoxMode() const = 0;
	virtual void AllEdges(bool onOff) = 0;
	virtual bool GetAllEdges() const = 0;
	virtual void VertTicks(bool onOff) = 0;
	virtual bool GetVertTicks() const = 0;
	virtual void BackCull(bool onOff) = 0;
	virtual bool GetBackCull() const = 0;
	virtual void SetCVertMode(bool onOff) = 0;
	virtual bool GetCVertMode() const = 0;
	virtual void SetShadeCVerts(bool onOff) = 0;
	virtual bool GetShadeCVerts() const = 0;
	virtual void SetCastShadows(bool onOff) = 0;
	virtual bool CastShadows() const = 0;
	virtual void SetRcvShadows(bool onOff) = 0;
	virtual bool RcvShadows() const = 0;
	virtual void SetMotBlur(int kind) = 0;
	virtual int MotBlur() const = 0;
	virtual int GetRenderFlags() const = 0;
	virtual void SetRenderFlags(int flags) = 0;
	virtual int GetDisplayFlags() const = 0;
	virtual int AddProperty(LayerProperty & lprop) = 0;
	virtual int SetProperty(LayerProperty & lprop) = 0;
	virtual int GetProperty(LayerProperty & lprop) const = 0;
	virtual bool Used(void) const = 0;
	virtual bool GetFlag(int mask) const = 0;
	virtual bool GetFlag2(int mask) const = 0;
	virtual void UpdateSelectionSet(void) = 0;
#ifdef DESIGN_VER
	virtual void SetInheritVisibility(bool onOff) = 0;
	virtual bool GetInheritVisibility() const = 0;
	virtual void Trajectory(bool onOff, bool temp = false) = 0;
	virtual bool GetTrajectory() const = 0;
	virtual void SetDisplayByLayer(BOOL onOff, INode *) = 0;
	virtual void SetRenderByLayer(BOOL onOff, INode *) = 0;
	virtual void SetMotionByLayer(BOOL onOff, INode *) = 0;
	virtual void SelectObjects(void) = 0;
	virtual void SetVisibility(TimeValue t, float vis) = 0;
	virtual float GetVisibility(TimeValue t,Interval *valid=NULL) = 0;
	virtual float GetVisibility(TimeValue t,View & view, Interval *valid=NULL) = 0;
	virtual float GetImageBlurMultiplier(TimeValue t) = 0;
	virtual void  SetImageBlurMultiplier(TimeValue t, float m) = 0;
	virtual bool GetMotBlurOnOff(TimeValue t) = 0;
	virtual void  SetMotBlurOnOff(TimeValue t, bool m) = 0;
	virtual bool IsHiddenByVisControl() = 0;
	virtual float GetLocalVisibility(TimeValue t,Interval *valid) = 0;
#endif
};

#endif