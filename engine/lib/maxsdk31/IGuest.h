/**********************************************************************
 *<
	FILE: IGuest.h

	DESCRIPTION: Declares Host/Guest protocol

	CREATED BY:	John Hutchinson

	HISTORY: Created April 24, 1999

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#pragma once

class IGeomImp;
class IHost;

class IGuest 
{
public:
	virtual void rsvp(IHost* host, IGeomImp* return_envelope, Matrix3& tm) = 0; 
};


class IHost
{
public:
	virtual void accomodate(IGeomImp* guestrep, Matrix3 &tm, HitRecord *rec = NULL) = 0;
	virtual bool locate(INode *host, Control *c, Matrix3 &oldP, Matrix3 &newP) = 0;
};
