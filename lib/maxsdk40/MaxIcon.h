 /**********************************************************************
 *<
	FILE: MaxIcon.h

	DESCRIPTION: Max Icon and Icon Table definitions

	CREATED BY:	Scott Morrison

	HISTORY: Created 15 March, 2000,

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __ICONMAN__
#define __ICONMAN__

// A MaxIcon is an abstract class that represents an icon image for
// toolbar buttons, icons in list boxes, etc.  The  class is based
// on Win32 ImageLists.  MaxIcons must provide an image list and
// index into the list for both large (24x24) and small (16x15) icons.

#include "object.h"
#include "iColorMan.h"

class ICustButton;

class MaxIcon : public InterfaceServer {
public:
    // Get the image list for the size of icons that the user has chose
    virtual HIMAGELIST GetDefaultImageList()  = 0;
    // Get the image list for small icons
    virtual HIMAGELIST GetSmallImageList()  = 0;
    // Get the image list for large icons
    virtual HIMAGELIST GetLargeImageList()  = 0;

    // Get the index into the image list for the small version of this
    // particular icon.
    virtual int GetSmallImageIndex(bool enabledVersion = true,
                                   COLORREF backgroundColor =
                                   GetCustSysColor( COLOR_BTNFACE) ) = 0;
    // Get the index into the image list for the large version of this
    // particular icon.
    virtual int GetLargeImageIndex(bool enabledVersion = true,
                                   COLORREF backgroundColor =
                                   GetCustSysColor( COLOR_BTNFACE) ) = 0;
    // Get the index into the image list for the default version of this
    // particular icon.
            int GetDefaultImageIndex(bool enabledVersion = true,
                                     COLORREF backgroundColor =
                                     GetCustSysColor( COLOR_BTNFACE) );

    // returns true if the icons has an alpha mask that needs to be blended
    // with the background color.
    virtual bool UsesAlphaMask() = 0;
};

// This implementation of MaxIcon is for the icon images that are stored
// as ".bmp" files in MAX's UI directory.  This is used by the macroScript
// facility in MAXSrcipt to specify icons.  See the documentation for
// "macroScript" for the exact meaning of the filename and index.

class MaxBmpFileIcon: public MaxIcon {
public:
    CoreExport MaxBmpFileIcon(TCHAR* pFilePrefix, int index);
	CoreExport MaxBmpFileIcon(SClass_ID sid, Class_ID cid);

    CoreExport HIMAGELIST GetDefaultImageList();
    CoreExport HIMAGELIST GetSmallImageList();
    CoreExport HIMAGELIST GetLargeImageList();

    CoreExport int GetSmallImageIndex(bool enabledVersion = true,
                                      COLORREF backgroundColor =
                                      GetCustSysColor( COLOR_BTNFACE) );

    CoreExport int GetLargeImageIndex(bool enabledVersion = true,
                                      COLORREF backgroundColor =
                                      GetCustSysColor( COLOR_BTNFACE) );

    CoreExport bool UsesAlphaMask();
    CoreExport TSTR& GetFilePrefix() { return mFilePrefix; }
    CoreExport int GetIndex() { return mIndex; }

private:
    int     mIndex;
    TSTR    mFilePrefix;
};

CoreExport HIMAGELIST GetIconManDefaultImageList();
CoreExport HIMAGELIST GetIconManSmallImageList();
CoreExport HIMAGELIST GetIconManLargeImageList();

CoreExport BOOL LoadMAXFileIcon(TCHAR* pFile, HIMAGELIST hImageList, ColorId color, BOOL disabled);

#endif
