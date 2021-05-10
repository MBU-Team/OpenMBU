typedef union {
   char              c;
   int               i;
   const char *      s;
   char *            str;
   double            f;
   StmtNode *        stmt;
   ExprNode *        expr;
   SlotAssignNode *  slist;
   VarNode *         var;
   SlotDecl          slot;
   ObjectBlockDecl   odcl;
   ObjectDeclNode *  od;
   AssignDecl        asn;
   IfStmtNode *      ifnode;
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
#define	rwDO	269
#define	rwENDIF	270
#define	rwENDWHILE	271
#define	rwENDFOR	272
#define	rwDEFAULT	273
#define	rwFOR	274
#define	rwDATABLOCK	275
#define	rwSWITCH	276
#define	rwCASE	277
#define	rwSWITCHSTR	278
#define	rwCASEOR	279
#define	rwPACKAGE	280
#define	rwNAMESPACE	281
#define	rwCLASS	282
#define	ILLEGAL_TOKEN	283
#define	CHRCONST	284
#define	INTCONST	285
#define	TTAG	286
#define	VAR	287
#define	IDENT	288
#define	STRATOM	289
#define	TAGATOM	290
#define	FLTCONST	291
#define	opMINUSMINUS	292
#define	opPLUSPLUS	293
#define	STMT_SEP	294
#define	opSHL	295
#define	opSHR	296
#define	opPLASN	297
#define	opMIASN	298
#define	opMLASN	299
#define	opDVASN	300
#define	opMODASN	301
#define	opANDASN	302
#define	opXORASN	303
#define	opORASN	304
#define	opSLASN	305
#define	opSRASN	306
#define	opCAT	307
#define	opEQ	308
#define	opNE	309
#define	opGE	310
#define	opLE	311
#define	opAND	312
#define	opOR	313
#define	opSTREQ	314
#define	opCOLONCOLON	315
#define	opMDASN	316
#define	opNDASN	317
#define	opNTASN	318
#define	opSTRNE	319
#define	UNARY	320


extern YYSTYPE CMDlval;
