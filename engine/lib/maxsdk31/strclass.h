/**********************************************************************
 *<
	FILE: strclass.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __STRCLASS__H
#define __STRCLASS__H


//-----------------------------------------------------------------------
// CStr: Simple char string class
//-----------------------------------------------------------------------
class CStr {
	char *buf;
	public:
		UtilExport CStr(); 
		UtilExport CStr(const char *cs);
		UtilExport CStr(const wchar_t *wcstr);
		UtilExport CStr(const CStr& ws);
		UtilExport ~CStr(); 
		UtilExport char *data();
		UtilExport const char *data() const;
		UtilExport operator char *();

		// realloc to nchars (padding with blanks)
		UtilExport void Resize(int nchars);

		UtilExport int Length() const;
		int length() const { return Length(); }
		BOOL isNull() { return Length()==0?1:0; }

		UtilExport CStr & operator=(const CStr& cs);
		UtilExport CStr & operator=(const wchar_t *wcstr);
		UtilExport CStr & operator=(const char *cs);

		// Concatenation operators.
		UtilExport CStr operator+(const CStr& cs) const;
		UtilExport CStr& operator+=(const CStr& cs); 
		CStr& Append(const CStr& cs)  { return ((*this) += cs); }
		CStr& append(const CStr& cs)  { return ((*this) += cs); }
		UtilExport CStr& remove(int pos);	// remove all chars from pos to end
		UtilExport CStr& remove(int pos, int N);	// remove N chars from pos to end

		// Substring operator
		UtilExport CStr Substr(int start, int nchars) const;
		UtilExport char& operator[](int i);
		UtilExport const char& operator[](int i) const;

		// Char search:(return -1 if not found)
		UtilExport int first(char c) const;
		UtilExport int last(char c) const;

		// Comparison
		UtilExport int operator==(const CStr &cs) const;
		UtilExport int operator<(const CStr &cs) const;
		UtilExport int operator<=(const CStr &ws) const;
		UtilExport int operator>(const CStr &ws) const;
		UtilExport int operator>=(const CStr &ws) const;

		UtilExport void toUpper();
		UtilExport void toLower();

		UtilExport int printf(const char *format, ...);
	};


//-----------------------------------------------------------------------
// WStr: Simple Wide char string class
//-----------------------------------------------------------------------
class WStr {
	wchar_t *buf;
	public:
		UtilExport WStr();
		UtilExport WStr(const char *cs);
		UtilExport WStr(const wchar_t *wcstr);
		UtilExport WStr(const WStr& ws);
		UtilExport ~WStr();
		UtilExport 	wchar_t *data();
		UtilExport 	const wchar_t *data() const;
		UtilExport operator wchar_t *();

		// realloc to nchars (padding with blanks)
		UtilExport void Resize(int nchars);
		UtilExport int Length() const;
		int length() const { return Length(); }
		BOOL isNull() { return Length()==0?1:0; }

		UtilExport WStr & operator=(const WStr& ws);
		UtilExport WStr & operator=(const wchar_t *wcstr);
		UtilExport WStr & operator=(const char *cstr);

		// Concatenation operators.
		UtilExport WStr operator+(const WStr& ws) const; 
		UtilExport WStr & operator+=(const WStr& ws); 
		WStr& Append(const WStr& ws) { return ((*this) += ws); }
		WStr& append(const WStr& ws)  { return ((*this) += ws); }
		UtilExport WStr& remove(int pos);	// remove chars from pos to end
		UtilExport WStr& remove(int pos, int N);	// remove N chars from pos to end

		// Substring operator
		UtilExport WStr Substr(int start, int nchars) const;
		wchar_t& operator[](int i) {return buf[i];}
		const wchar_t& operator[](int i) const {return buf[i];}

		// Char search:(return -1 if not found)
		UtilExport int first(wchar_t c) const;
		UtilExport int last(wchar_t c) const;

		// Comparison
		UtilExport int operator==(const WStr &ws) const;
		UtilExport int operator<(const WStr &ws) const;
		UtilExport int operator<=(const WStr &ws) const;
		UtilExport int operator>(const WStr &ws) const;
		UtilExport int operator>=(const WStr &ws) const;

		UtilExport void toUpper();
		UtilExport void toLower();
		UtilExport int printf(const wchar_t *format, ...);
	};					



#ifdef _UNICODE
#define TSTR WStr
#else
#define TSTR CStr
#endif

//--FilterList----------------------------------------------------------------------
// A class whose sole purpose is for buildingup a  filter list to passing to
// GetSaveFileName and GetOpenFileName.  It automatically puts in the imbedded nulls
// and two terminating nulls.
//	 Example:
//
//	FilterList filterList;
//	filterList.Append( _T("Jaguar files(*.jag)"));
//	filterList.Append( _T("*.jag"));
//	ofn.lpstrFilter  = filterList;
//	GetSaveFileName(&ofn)
//----------------------------------------------------------------------------------

class FilterList {
    Tab<TCHAR> buf;
	public:
		UtilExport FilterList();
		UtilExport void Append(TCHAR *name);
		UtilExport operator TCHAR *(); 
	};


/*------------------------------------------------ 
	Split filename "name" into 
	p  path
	f  filename
	e  extension 
-------------------------------------------------*/

UtilExport void SplitFilename(TSTR& name,TSTR* p, TSTR* f, TSTR* e);

/*--------------------------------------------------
Split filename "name" into 
	p  path
	f  filename.ext
-------------------------------------------------*/

UtilExport void SplitPathFile(TSTR& name,TSTR* p, TSTR* f);


/*--------------------------------------------------
Check to see if s matches the pattern in ptrn
-------------------------------------------------*/

UtilExport BOOL MatchPattern(TSTR &s, TSTR &ptrn, BOOL ignoreCase=TRUE);


//-------------------------------------------------------------------------
// A Case Sensitive "smart" alphanumeric compare that sorts things so that
// numerical suffices come out in numerical order.
//-------------------------------------------------------------------------
UtilExport int MaxAlphaNumComp(TCHAR *a, TCHAR *b);


//-------------------------------------------------------------------------
// A Case Insensitive "smart" alphanumeric compare that sorts things so that
// numerical suffices come out in numerical order.
//-------------------------------------------------------------------------
UtilExport int MaxAlphaNumCompI(TCHAR *a, TCHAR *b);

#endif
