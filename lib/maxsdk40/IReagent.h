/**********************************************************************
 *<
	FILE: IReagent.h

	DESCRIPTION: Declares a class of objects which react

	CREATED BY:	John Hutchinson

	HISTORY: Created January 9, 1999

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#pragma once

#include <IAggregation.h>
class IAggregation;
class IGeomImp;
class IValence;
class ValenceDescIterator;
class IHost;

class IReagent 
{
public:
	//completes the construction process
	virtual void InitializeSubstrate(INode* n) = 0;//completes the construction process

	virtual IValence* OpenValence() = 0;
	virtual void OpenValenceTM(TimeValue t, Matrix3& tm)= 0;
	virtual int OpenValenceIndex() = 0;
	virtual void CloseValence(IValence* val, bool done = false)= 0;
	virtual int NumValenceTypes() = 0;
	virtual Class_ID GetValenceClassID(int which) = 0;
	//create a valence given a class descriptor and a node(used for its location)
	//may create a new instance of this type
	virtual IValence* PrepValence(ClassDesc* cd, INode* me, INode* loc) = 0;

	//pass back the chemistry/geometry of the reagent viz-a-viz the current reaction
	//Thus this excludes any pending reaction results.
	//Currently we assumes one reaction at a time. Could pass a valence to identify which.
	virtual IGeomImp* ReactionSite() = 0;
};


class IValence
{
public:
	enum bondtype {
		eStrongBond,//absorb the node
			eWeakBond,//don't absorb the node
	eTransientBond,//temporary but affect permanaent change to object stack
	eTopicalBond};//unused but coneptually a weaker type of transient bonds which just copy node properties

	//set up a controller and possibly record subobj hit data from the host
	virtual Control* Locate(INode* host, INode *loc, Control* c, Matrix3& oldP, Matrix3& newP) = 0;//may clone the controller
	virtual commitlevels Occupy(Object* obj) = 0;
	virtual Object* Occupant()=0;
	virtual bool Bind(TimeValue t, IGeomImp &complex_geom, Matrix3& tm, Interval iv, IHost* attentivehost = NULL, bool expedite = false) = 0;
	virtual commitlevels Validate(TimeValue t, int action,  commitlevels current, Matrix3* tm) = 0;
	virtual bondtype BondStrength()=0;
	virtual void SetOwner(Object* owner)=0;
	virtual void SetMaterialBase(int matidx) = 0;
};

/*
class ValenceDescIterator
{
public:
	ValenceDescIterator();
	~ValenceDescIterator();

	Class_ID First();
	virtual bool IsDone();
	Class_ID Next();
private:
	int m_current;
};
*/
//mechanism to test compatibility needs to be done at the class descriptor level. 
//Since this is what gets enumerated at the point prio to the valence getting created.
