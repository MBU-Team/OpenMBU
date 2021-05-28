/*********************************************************************
 *<
	FILE: evuser.h

	DESCRIPTION: Event user functionality

	CREATED BY:	Tom Hudson

	HISTORY: Created 16 June 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

/**********************************************************************
How to use:

This is a set of classes which form a generic notification system.  To
use:

* Create an EventUser object.

* Register the EventUser object with the appropriate router.

* The EventRouter will call your EventUser's Notify() method when
the event occurs.

* When you're done with the EventUser object, call the EventRouter's
UnRegister() method.  This will delete the EventUser from the router's
notification system.

* If your code is part of a window proc, call the router's Register
and UnRegister methods when the window receives WM_ACTIVATE messages.
This will properly uncouple the notification system when the window is
deactivated.

**********************************************************************/

#ifndef __EVUSER__
#define __EVUSER__

class EventUser {
	public:
		virtual void Notify()=0;
	};

#endif // __EVUSER__
