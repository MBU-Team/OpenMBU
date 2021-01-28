#ifndef __EXCLLIST__H
#define __EXCLLIST__H

#include <ioapi.h>

#ifndef NT_INCLUDE
#define NT_INCLUDE			  1
#define NT_AFFECT_ILLUM		  2
#define NT_AFFECT_SHADOWCAST  4
#endif

typedef INode* INodePtr;

class ExclList : public InterfaceServer {
	ULONG flags;
	Tab<ULONG> handles;
	public:
		ExclList() { flags = 0; }
		void SetFlag(ULONG f, BOOL b=1) { if (b) flags|=f; else flags &= ~f; }
		BOOL TestFlag(ULONG f) const { return (flags&f)?1:0; }
		int Count() const { return handles.Count(); }
		CoreExport INode* operator[] (const int i);
		CoreExport void Set(int i, INode *node);
		CoreExport ExclList& operator=(const ExclList& e);
		CoreExport ExclList& operator=(const NameTab& n);
		CoreExport int FindNode(INode *node);
		CoreExport int AddNode(INode *node);
		CoreExport void RemoveNode(INode *node);
		CoreExport void RemoveNode(int i);
		CoreExport void SetSize(int num);     
		void SetCount(int num) { SetSize(num); } 
		CoreExport IOResult Load(ILoad *iload);
		CoreExport IOResult Save(ISave *isave);
		CoreExport void OnMerge(IMergeManager* imm);
	};	

#endif
