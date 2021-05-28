/**********************************************************************
 *<
	FILE: TCBGraph.h

	DESCRIPTION:  UI gadget to display TCB graph

	CREATED BY: Rolf Berteig

	HISTORY: created 2/12/96

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _TCBGRAPH_
#define _TCBGRAPH_

// The class name of the UI gadget
#define TCBGRAPHCLASS _T("TCBGraph")

// Send this message to the graph control with lParam
// pointing to a TCBGraphParams structure to set the
// garph parameters
#define WM_SETTCBGRAPHPARAMS	(WM_USER + 0xbb34)

class TCBGraphParams {
	public:
		float tens, cont, bias, easeFrom, easeTo;
	};

CoreExport void InitTCBGraph(HINSTANCE hInst);

#endif //_TCBGRAPH_
