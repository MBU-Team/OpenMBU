/**********************************************************************
 *<
	FILE: expr.h

	DESCRIPTION: expression object include file.

	CREATED BY: Don Brittain

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _EXPR_H_

#define _EXPR_H_

#include "export.h"

#define SCALAR_EXPR		1
#define VECTOR_EXPR		3

#define SCALAR_VAR		SCALAR_EXPR
#define VECTOR_VAR		VECTOR_EXPR

class Expr;

typedef int (*ExprFunc)(Expr *e, float f);

class DllExport Inst {
public:
    ExprFunc	func;
    float  		sVal;
};

class ExprVar {
public:
	TSTR		name;
	int			type;
	int			regNum;
};

MakeTab(float);
MakeTab(Point3);
MakeTab(Inst);
MakeTab(ExprVar);

class Expr {
public:
	Expr()	{ sValStk = vValStk = instStk = nextScalar = nextVector = 0; }
	~Expr()	{ deleteAllVars(); }

	DllExport int		load(char *s);
	DllExport int		eval(float *ans, int sRegCt, float *sRegs, int vRegCt=0, Point3 *vRegs=NULL);
	int					getExprType(void)	{ return exprType; }
	TCHAR *				getExprStr(void)	{ return origStr; }
	TCHAR *				getProgressStr(void){ return progressStr; }
	DllExport int		defVar(int type, TCHAR *name);
	DllExport int		getVarCount(int type);
	DllExport TCHAR *	getVarName(int type, int i);
	DllExport int		getVarRegNum(int type, int i);
	DllExport BOOL		deleteAllVars();
	DllExport BOOL		deleteVar(TCHAR *name);

// pseudo-private: (only to be used by the "instruction" functions
	void		setExprType(int type)	{ exprType = type; }
	void		pushInst(ExprFunc fn, float f) 
					{ if(instStk >= inst.Count()) inst.SetCount(instStk+30); 
					inst[instStk].func = fn; inst[instStk++].sVal = f; }
	void		pushSVal(float f)	{ if(sValStk>=sVal.Count())sVal.SetCount(sValStk+10);sVal[sValStk++]=f; }
	float		popSVal()			{ return sVal[--sValStk]; }
	void		pushVVal(Point3 &v)	{ if(vValStk>=vVal.Count())vVal.SetCount(vValStk+10);vVal[vValStk++]=v; }
	Point3 &	popVVal()			{ return vVal[--vValStk]; }
	int			getSRegCt(void)		{ return sRegCt; }
	float		getSReg(int index)	{ return sRegPtr[index]; }
	int			getVRegCt(void)		{ return vRegCt; }
	Point3 &	getVReg(int index)	{ return vRegPtr[index]; }

	ExprVarTab	vars;			// named variables
private:
	TCHAR *		exprPtr;		// pointer to current str pos during parsing
	TCHAR *		exprStr;		// ptr to original expression string to parse
	TSTR		origStr;		// original expression string that was loaded
	TSTR		progressStr;	// string to hold part of expr successfully parsed
	int			sRegCt;			// actual number of scalar registers passed to "eval"
	float		*sRegPtr;		// pointer to the scalar register array
	int			vRegCt;			// actual number of vector registers passed to "eval"
	Point3		*vRegPtr;		// pointer to the vector register array
	int			exprType;		// expression type: SCALAR_EXPR or VECTOR_EXPR (set by load)

	int			sValStk;		// scalar value stack
	floatTab	sVal;
	int			vValStk;		// vector value stack
	Point3Tab	vVal;
	int			instStk;		// instruction stack
	InstTab		inst;

	int			nextScalar;		// next scalar slot
	int			nextVector;		// next vector slot

	friend		yylex();
	friend		yyerror(char *);
};

#define EXPR_NORMAL			 0
#define EXPR_INST_OVERFLOW	-1	// instruction stack overflow during parsing
#define EXPR_UNKNOWN_TOKEN	-2  // unknown function, const, or reg during parsing
#define EXPR_TOO_MANY_VARS	-3	// value stack overflow
#define EXPR_TOO_MANY_REGS	-4	// register array overflow, or reg number too big
#define EXPR_CANT_EVAL		-5	// function can't be evaluated with given arg
#define EXPR_CANT_PARSE		-6	// expression can't be parsed (syntactically)

#endif // _EXPR_H_
