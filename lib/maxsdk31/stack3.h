#ifndef _STACK3_H_ 

#define _STACK3_H_

#include "matrix3.h"

#define STACK_DEPTH		32		// default stack depth

class Matrix3Stack {
public:
	DllExport Matrix3Stack();
	DllExport Matrix3Stack(int depth);
	DllExport ~Matrix3Stack();

	BOOL		replace(const Matrix3 &m)
					{ stk[index] = m; return TRUE; }
	BOOL		push(const Matrix3 &m)
					{ stk[index++] = m; return index < maxDepth; }
	BOOL		dup(void)
					{ stk[index+1] = stk[index]; return ++index < maxDepth; }
	BOOL		concat(const Matrix3 &m)
					{ stk[index] = m * stk[index]; return TRUE; }
	Matrix3	&	get(void)
					{ return stk[index]; }
	Matrix3 &	pop(void)
					{ return stk[index--]; }
	BOOL		remove(void)
					{ return --index >= 0; }
	BOOL		reset(void)
					{ index = 0; stk[0].IdentityMatrix(); return TRUE; }

private:
	int			maxDepth;
	int			index;
	Matrix3 *	stk;
};

	
#endif // _STACK3_H_
