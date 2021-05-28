/**********************************************************************
 *<
	FILE: ptrvec.h

	DESCRIPTION:  An variable length array of pointers

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __PTRVEC__H
#define __PTRVEC__H

class PtrVector {
	protected:
		int size;
		int nused;
		void** data;
		PtrVector() { size = nused = 0; data = NULL; }
		UtilExport ~PtrVector();
		UtilExport PtrVector(const PtrVector& v);
		UtilExport PtrVector&	operator=(const PtrVector& v);
	    UtilExport void append(void * ptr , int extra);
		UtilExport void insertAt(void * ptr , int at, int extra);
	    UtilExport void* remove(int i);
	    UtilExport void* removeLast();
		void* operator[](int i) const { return data[i]; }		
		void*& operator[](int i) { return data[i]; }		
	public:
		UtilExport void reshape(int i);  // sets capacity
		UtilExport void setLength(int i);  // sets length, capacity if necessary
		UtilExport void clear();	// deletes the ptr array, but not the objects
		void shrink() { reshape(nused); }
		int length() const { return nused; }
		int capacity() const { return size; }
	};

template <class T> class PtrVec: public PtrVector {
public:	
	PtrVec():PtrVector() {}
	T* operator[](int i) const { return (T*)PtrVector::operator[](i); }		
	T*& operator[](int i) { return (T*&)PtrVector::operator[](i); }				
	PtrVec<T>& operator=(const PtrVec<T>& v) { return (PtrVec<T>&)PtrVector::operator=(v); }
	void append(T *ptr, int extra = 10) { PtrVector::append(ptr,extra); }	
	void insertAt(T* ptr, int at, int extra=10) { PtrVector::insertAt(ptr,at,extra); }	 
	T* remove(int i) { return (T *)PtrVector::remove(i); }		
	T* removeLast() { return (T *)PtrVector::removeLast(); }		
	void deleteAll();  //  deletes all the objects		
	};

template <class T> 
void PtrVec<T>::deleteAll() {
	while (length()) {
		T* p = removeLast();
		delete p;
		}
	}

#endif
