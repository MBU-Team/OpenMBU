/**********************************************************************
 	FILE: omanapi.h

	DESCRIPTION:  Defines an interface to the osnapmanager class

	CREATED BY: John Hutchinson
	HISTORY: May 14, 1997
	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _IOMAN_H
#define _IOMAN_H
// This class provides an interface to the OsnapManager. People who implement osnaps
// need to record hits with the manager. People implementing command modes are responsible 
// for getting the snap preview done and may be responsible for initting and closing point
// sequences.
class OsnapHit;
  
#define IID_IOsnapManager Interface_ID(0x5ba68f3, 0x490c28a2)
class IOsnapManager :  public BaseInterface
{
public:
	virtual BOOL getactive() const =0;
	virtual BOOL getAxisConstraint()=0;
	virtual void RecordHit(OsnapHit* somehit)=0;
	virtual BOOL OKForRelativeSnap()=0;
	virtual BOOL RefPointWasSnapped()=0;
	virtual Point3 GetRefPoint(BOOL top = TRUE)=0;
	virtual BOOL IsHolding()=0;
	virtual OsnapHit &GetHit()=0;
	virtual ViewExp* GetVpt()=0;
	virtual INode* GetNode()=0;
	virtual int GetSnapStrength()=0;
	virtual Matrix3 GetObjectTM()=0;
	virtual TimeValue GetTime()=0;
	virtual void wTranspoint(Point3 *inpt, IPoint3 *outpt)=0;
	virtual void Reset() =0;
	virtual BOOL TestAFlag(int mask)=0;
	virtual Point3 GetCurrentPoint()=0;
};

#endif// _IOMAN_H