//-----------------------------------------------------------------------------
// GarageGames Library
// Copyright (c) GarageGames, All Rights Reserved
//-----------------------------------------------------------------------------
#pragma once
#include <string>
#include <platform/types.h>
typedef std::string String;

namespace Torque
{

//-----------------------------------------------------------------------------

/// FileSystem filename representation.
/// Filenames has the following form: "root:path/file.ext"
/// @ingroup UtilString
class Path
{
public:
   enum Separator
   {
#if defined(TORQUE_OS_WIN32) || defined(TORQUE_OS_XENON)
      OsSeparator = '\\'
#else
      OsSeparator = '/'
#endif
   };

   Path()
      :  mIsDirtyFileName( true ),
         mIsDirtyPath( true )
   {
   }

   Path( const char *file )
      :  mIsDirtyFileName( true ),
         mIsDirtyPath( true )
   {
      _split(file);
   }

   Path( const String &file )
      :  mIsDirtyFileName( true ),
         mIsDirtyPath( true )
   {
      _split(file);
   }

   Path& operator = ( const String &file ) { _split(file); mIsDirtyPath = mIsDirtyFileName = true; return *this; }
   operator String() const { return getFullPath(); }

   bool operator == (const Path& path) const { return getFullPath() == (path.getFullPath()); }
   bool operator != (const Path& path) const { return !(*this == path); }

   bool isEmpty() const { return getFullPath().empty(); }

   /// Join two path or file name components together.
   static String Join(const String&, char,const String&);

   /// Replace all '\' with '/'
   static String CleanSeparators( String path );

   /// Remove "." and ".." relative paths.
   static String CompressPath( String path );

   /// Take two paths and return the relative path between them.
   static Path MakeRelativePath( const Path &makeRelative, const Path &relativeTo, U32 mode = 0 );
   
   const String& getRoot() const { return mRoot; }
   const String& getPath() const { return mPath; }
   const String& getFileName() const { return mFile; }
   const String& getExtension() const { return mExt; }

   const String& getFullFileName() const;
   const String& getFullPath() const;
   
   const String& setRoot(const String &s);
   const String& setPath(const String &s);
   const String& setFileName(const String &s);
   const String& setExtension(const String &s);
   
   U32 getDirectoryCount() const;
   String getDirectory(U32) const;
   
   bool isDirectory() const;
   bool isRelative() const;
   bool isAbsolute() const;
   
   /// Appends the argument's path component to the object's
   /// path component. The object's root, filename and
   /// extension are unaffected.
   bool appendPath(const Path &path);

private:
   String   mRoot;
   String   mPath;
   String   mFile;
   String   mExt;
   
   mutable String   mFullFileName;
   mutable String   mFullPath;

   mutable bool  mIsDirtyFileName;
   mutable bool  mIsDirtyPath;
   
   void _split(String name);
   String _join() const;
};

/// Convert file/path name to use platform standard path separator
///@ingroup VolumeSystem
String PathToPlatform(String file);

/// Convert file/path name to use OS standard path separator
///@ingroup VolumeSystem
String PathToOS(String file);

} // Namespace

