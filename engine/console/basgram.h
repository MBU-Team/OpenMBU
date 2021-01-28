typedef union {
	char c;
	int i;
	const char *s;
   char *str;
	double f;
	StmtNode *stmt;
	ExprNode *expr;
   SlotAssignNode *slist;
   VarNode *var;
   SlotDecl slot;
   ObjectBlockDecl odcl;
   ObjectDeclNode *od;
   AssignDecl asn;
   IfStmtNode *ifnode;
} YYSTYPE;
#define	rwDEFINE	258
#define	rwENDDEF	259
#define	rwDECLARE	260
#define	rwBREAK	261
#define	rwELSE	262
#define	rwCONTINUE	263
#define	rwGLOBAL	264
#define	rwIF	265
#define	rwNIL	266
#define	rwRETURN	267
#define	rwWHILE	268
#define	rwENDIF	269
#define	rwENDWHILE	270
#define	rwENDFOR	271
#define	rwDEFAULT	272
#define	rwFOR	273
#define	rwDATABLOCK	274
#define	rwSWITCH	275
#define	rwCASE	276
#define	rwSWITCHSTR	277
#define	rwCASEOR	278
#define	rwPACKAGE	279
#define	ILLEGAL_TOKEN	280
#define	CHRCONST	281
#define	INTCONST	282
#define	TTAG	283
#define	VAR	284
#define	IDENT	285
#define	STRATOM	286
#define	TAGATOM	287
#define	FLTCONST	288
#define	opMINUSMINUS	289
#define	opPLUSPLUS	290
#define	STMT_SEP	291
#define	opSHL	292
#define	opSHR	293
#define	opPLASN	294
#define	opMIASN	295
#define	opMLASN	296
#define	opDVASN	297
#define	opMODASN	298
#define	opANDASN	299
#define	opXORASN	300
#define	opORASN	301
#define	opSLASN	302
#define	opSRASN	303
#define	opCAT	304
#define	opEQ	305
#define	opNE	306
#define	opGE	307
#define	opLE	308
#define	opAND	309
#define	opOR	310
#define	opSTREQ	311
#define	opCOLONCOLON	312
#define	opMDASN	313
#define	opNDASN	314
#define	opNTASN	315
#define	opSTRNE	316
#define	UNARY	317
#define	rwTHEN	318
#define	rwEND	319
#define	rwBEGIN	320
#define	rwCFOR	321
#define	rwTO	322
#define	rwSTEP	323


extern YYSTYPE BASlval;
