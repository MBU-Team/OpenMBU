/*****************************************************************************
 *<
	FILE: meshacc.h

	DESCRIPTION:  A class for getting at the private snap data of the mesh class

	CREATED BY: John Hutchinson		

	HISTORY: created 1/2/97

 *>	Copyright (c) 1994, All Rights Reserved.
 *****************************************************************************/
#ifndef _MESHACC_H_

#define _MESHACC_H_
#include "export.h"
#include "mesh.h"

class MeshAccess {
private:
	Mesh *mymesh;
public:
	MeshAccess(Mesh *somemesh){mymesh = somemesh;}
	DllExport int BuildSnapData(GraphicsWindow *gw,int verts,int edges);
	char* GetsnapV(){
		return mymesh->snapV;
	}
	char* GetsnapF(){
		return mymesh->snapF;
	}
};

#endif
