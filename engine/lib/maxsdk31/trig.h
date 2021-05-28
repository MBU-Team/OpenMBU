/**********************************************************************
 *<
	FILE: trig.h

	DESCRIPTION:  Useful trigonometry macros

	CREATED BY: Rolf Berteig

	HISTORY: created 19 November 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __TRIG__
#define __TRIG__

#define PI  ((float)3.1415926535)
#define TWOPI ((float)6.283185307)
#define HALFPI ((float)1.570796326794895)

#define DEG_TO_RAD (PI/(float)180.0)
#define RAD_TO_DEG ((float)180.0/PI)
#define DegToRad(deg) (((float)deg)*DEG_TO_RAD)
#define RadToDeg(rad) (((float)rad)*RAD_TO_DEG)


#endif // __TRIG_H__
