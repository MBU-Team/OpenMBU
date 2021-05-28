/*	
 *		iMSZip.h - Public interface to MAXScript Zip Package Manager
 *
 *			Copyright © Autodesk, Inc, 2000.  John Wainwright.
 *
 */

#ifndef _H_IMSZIP
#define _H_IMSZIP

class IMSZipMgr;

#define MSZIP_MGR_INTERFACE   Interface_ID(0x26492c82, 0x10fc5210)
inline IMSZipMgr* GetMSZipMgr() { return (IMSZipMgr*)GetCOREInterface(MSZIP_MGR_INTERFACE); }

// class IMSZipMgr
//    MAXScript Zip Package manager interface 
class IMSZipMgr : public FPStaticInterface 
{
public:
	// function IDs 
	enum { fileInPackage,
		   unloadPackage,
		}; 

	virtual BOOL FileInPackage(TCHAR* file_name, TSTR& extract_dir)=0;	// unload & run zip package, return extract-to directory
	virtual BOOL UnloadPackage(TCHAR* file_name, TSTR& extract_dir, 
							   TSTR& dropFile, MacroEntry*& dropScript)=0; // just unload the package, ignore any drop or run commands,
																	    //   return extract-to directory and any primary drop file
																		//   if the primary dopFile is a .ds, compile it in context of
																		//   package and return MAcroEntry* for it in 'dropScript'
}; 

#endif
