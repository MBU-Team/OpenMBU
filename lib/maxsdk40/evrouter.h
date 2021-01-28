/*********************************************************************
 *<
	FILE: evrouter.h

	DESCRIPTION: Event router functionality

	CREATED BY:	Tom Hudson

	HISTORY: Created 16 June 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __EVROUTER__
#define __EVROUTER__

#include "evuser.h"

typedef EventUser* PEventUser;
typedef Tab<PEventUser> PEventUserTab;

class EventRouter {
	private:
		PEventUserTab	userTab;
		BOOL			notifyMultiple;
	public:
		EventRouter(BOOL multiple = FALSE)	{ notifyMultiple = multiple; }
		CoreExport void Register(EventUser *user);
		CoreExport void UnRegister(EventUser *user);
		// Process the event.  Returns TRUE if the event was handed off to a user.
		CoreExport BOOL Process();
	};

extern CoreExport EventRouter deleteRouter;
extern CoreExport EventRouter backspaceRouter;
#ifdef _OSNAP
	extern CoreExport EventRouter tabkeyRouter;
#endif

#endif // __EVROUTER__
