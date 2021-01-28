/**********************************************************************
 *<
	FILE: assert1.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifdef assert
#undef assert
#endif

#define assert( expr ) ( expr || assert1( /*#expr,*/ __LINE__, __FILE__ ) )

#define MaxAssert( expr ) ( (expr) || assert1( __LINE__, __FILE__ ) )

extern int UtilExport assert1( /*char *expr,*/ int line, char *file );

#ifdef _DEBUG
#define DbgAssert( expr ) ( (expr) || assert1( __LINE__, __FILE__ ) )
#define DbgVerify( expr ) ( (expr) || assert1( __LINE__, __FILE__ ) )
#else
#define DbgAssert( expr )
#define DbgVerify( expr ) ( expr )
#endif
