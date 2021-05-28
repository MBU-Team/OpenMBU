/**********************************************************************
 *<
	FILE: tab.h

	DESCRIPTION:  Defines Tab Class

	CREATED BY: Dan Silva

	HISTORY: created 13 September 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

/*-------------------------------------------------------------------------------

 A Generic "Table" class.      
  
  (DSilva 9-13-94)

  This is a type-safe variable length array which also supports list-like
  operations of insertion, appending and deleting.  Two instance variables
  are maintained: "nalloc" is the number elements allocated in the
  array; "count" is the number actual used. (count<=nalloc).
  Allocation is performed automatically when Insert or Append operations
  are performed.  It can also be done manually by calling Resize or Shrink.
  Note: Delete does not resize the storage: to do this call Shrink().  
  If you are going to do a sequence of Appends, it's more efficient to 
  first call Resize to make room for them.  Beware of using the Addr 
  function: it returns a pointer which may be invalid after subsequent 
  Insert, Append, Delete, Resize, or Shrink operations.  
  
  
  The implementation minimizes the storage of empty Tables: they are
  represented by a single NULL pointer.  Also, the major part of the
  code is generic, shared by different Tabs for different types of elements.

------------------------------------------------------------------------------*/

#ifndef __TAB__ 

#define __TAB__

#include <malloc.h>
#include <stdlib.h>

typedef int CNT;

typedef struct {											
	CNT count;
	CNT nalloc;
	} TabHdr;

////////////////////////////////////////////////////////////////////////////////
// Functions for internal use only: Clients should never call these.
//
UtilExport int TBMakeSize(TabHdr** pth, int num, int elsize); 
UtilExport int TBInsertAt(TabHdr** pth,int at, int num, void *el, int elsize, int extra); 
UtilExport int TBCopy(TabHdr** pth,int at, int num, void *el, int elsize); 
UtilExport int TBDelete(TabHdr** pth,int starting, int num, int elsize);
UtilExport void TBSetCount(TabHdr** pth,int n, int elsize);
UtilExport void zfree(void**p);
////////////////////////////////////////////////////////////////////////////////

#define NoExport
		
template <class T> class NoExport TabHd {
	public:
		CNT count;
		CNT nalloc;
		T data[100];
		TabHd() { count = 0; nalloc = 0; }
	};


// Type of function to pass to Sort.
// Note: Sort just uses the C lib qsort function. If we restricted
// all Tab elements to have well defined <,>,== then we wouldn't need
// this callback function.
typedef int( __cdecl *CompareFnc) ( const void *elem1, const void *elem2 );



template <class T> class NoExport Tab {
private:
 	TabHd<T> *th;
 	/*
 	struct TabHd {
		CNT count;
		CNT nalloc;
		T data[1];
		} *th;
	*/
public:
	Tab() { th = 0; }
	// Copy constructor
	Tab(const Tab& tb) {  
		th = 0;
		TBCopy((TabHdr** )&th,0, tb.Count(), &tb.th->data, sizeof(T)); 
		}
	// Assignment operator
	Tab& operator=(const Tab& tb) {
		TBCopy((TabHdr** )&th,0, tb.Count(), &tb.th->data, sizeof(T)); 
		return *this;
		}
	
	~Tab() { zfree((void**)&th); }  // destructor

	int Count() const { if (th) return(th->count); return 0; }  // return number of entries being used

	void ZeroCount() { if (th) th->count=0; }
	void SetCount(int n) { TBSetCount((TabHdr **)&th, n, sizeof(T)); }

	T& operator[](const int i) const {       // access ith entry.
		assert(th&&(i<th->count)); return(th->data[i]); 
		}
	T* Addr(const int i) const {             // use with caution  
		assert(th&&(i<th->count)); return(&th->data[i]); 
		}
//	void *MemAddr() {	return((void *)th);	}
//	long MemSize() { return(th? (2*sizeof(CNT)+th->nalloc*sizeof(T)): 0);}

	// Insert "num" elements position "at" 
	int Insert(int at, int num, T *el) {
		return(TBInsertAt((TabHdr**)&th, at, num, (void *)el, sizeof(T),0));
		}
	// Append "num" elements position on end of array" 
	// If need to enlarge the array, allocate "allocExtra" extra slots
	int Append(int num, T *el, int allocExtra=0) {
		return(TBInsertAt((TabHdr**)&th,th?th->count:0,num,	(void *)el,sizeof(T),allocExtra)); 
		}
	// List-type delete of "num" elements starting with "start" 
	int Delete(int start,int num) { 
		return(TBDelete((TabHdr**)&th,start,num,sizeof(T)));
		} 
	// Change number of allocated items to num
	int Resize(int num) { 
		return(TBMakeSize((TabHdr**)&th,num, sizeof(T)));
		}	
	// Reallocate so there is no wasted space (nalloc = count)
	void Shrink() {
		TBMakeSize((TabHdr**)&th, th?th->count:0, sizeof(T)); 
		}

	void Sort(CompareFnc cmp) {
		if (th) {
			qsort(th->data,th->count,sizeof(T),cmp);
			}
		}																		
	};

#ifndef __tab_name2
#define __tab_name2(a,b) a##b
#endif

#define MakeTab(TYPE) typedef Tab<TYPE> __tab_name2(TYPE,Tab); 															


UtilExport void TabStartRecording();
UtilExport void TabStopRecording();
UtilExport void TabPrintAllocs();
UtilExport void TabAssertAllocNum(int i);

#endif
