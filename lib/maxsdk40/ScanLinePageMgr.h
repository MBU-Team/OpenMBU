//***************************************************************************
// ScanLinePageMgr.h
// A scanline based memory-to-disk pager template
// Christer Janson
// Discreet, A division of Autodesk, Inc.
// San Francisco, CA - November 15, 1999

#ifndef _SCANLINEPAGEMGR__H_
#define _SCANLINEPAGEMGR__H_

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#define SCANLINE_PAGE_INMEMORY	0x01
#define SCANLINE_PAGE_NEEDWRITE	0x02
#define SCANLINE_PAGE_HASDISKSTORAGE	0x04
#define SCANLINE_PAGE_LOCKED		0x08
#define SCANLINE_PAGE_VALIDDISK	0x10

template <class T> class ScanLinePage {
public:
	ScanLinePage(int width, int numScanLines, int firstScanLine, int pageSize, int fileIndex)
		{
		mFlags = 0;
		mLockCount = 0;
		mWidth = width;
		mNumScanLines = numScanLines;
		mFirstScanLine = firstScanLine;
		mFileIndex = fileIndex;
		mPageSize = pageSize;

		mData = NULL;
		ClearInMemory();
		ClearValidDisk();
		SetHasDiskStorage();
		ClearNeedWrite();
		}

	ScanLinePage(int width, int height, bool* allocOk)
		{
		mFlags = 0;
		mLockCount = 0;
		mWidth = width;
		mNumScanLines = height;
		mFirstScanLine = 0;
		mFileIndex = 0;
		mPageSize = 0;

		mData = (T*)malloc(width*height*sizeof(T));
		if (mData) {
			*allocOk = true;
			memset(mData, 0, width*height*sizeof(T));
			}
		else {
			*allocOk = false;
			}
		SetInMemory();
		ClearHasDiskStorage();
		ClearNeedWrite();
		ClearValidDisk();
		}


	~ScanLinePage()
		{
		if (mData) {
			free(mData);
			}
		}

	void PreparePage(int fileHandle)
		{
		// If it's allocated, it's already prepared
		if (!mData) {
			mData = (T*)malloc(mPageSize);
			SetInMemory();
			if (GetValidDisk()) {
				ReadPage(fileHandle);
				}
			else {
				memset(mData, 0, mPageSize);
				}
			}
		}

	void FlushPage(int fileHandle)
		{
		if (GetHasDiskStorage() && !IsPageLocked()) {
			if (GetInMemory()) {
				if (GetNeedWrite()) {
					WritePage(fileHandle);
					}
				free(mData);
				mData = NULL;
				ClearInMemory();
				}
			}
		}

	T* GetBuffer(int scanline, bool needWrite)
		{
		if (needWrite) {
			SetNeedWrite();
			}
		return &mData[(scanline-mFirstScanLine)*mWidth];
		}

	T* GetBuffer()
		{
		if (GetInMemory())
			return mData;
		else
			return NULL;
		}

	void	LockPage()		{ mLockCount++; }
	void	UnlockPage()	{ mLockCount--; }
	bool	IsPageLocked()	{ return (mLockCount > 0) ? true : false; }

private:
	void	SetInMemory()	{ mFlags |= SCANLINE_PAGE_INMEMORY; }
	void	ClearInMemory()	{ mFlags &= ~SCANLINE_PAGE_INMEMORY; }
	bool	GetInMemory()	{ return (mFlags & SCANLINE_PAGE_INMEMORY) ? true : false; }

	void	SetNeedWrite()	{ mFlags |= SCANLINE_PAGE_NEEDWRITE; }
	void	ClearNeedWrite(){ mFlags &= ~SCANLINE_PAGE_NEEDWRITE; }
	bool	GetNeedWrite()	{ return (mFlags & SCANLINE_PAGE_NEEDWRITE) ? true : false; }

	void	SetHasDiskStorage()		{ mFlags |= SCANLINE_PAGE_HASDISKSTORAGE; }
	void	ClearHasDiskStorage()	{ mFlags &= ~SCANLINE_PAGE_HASDISKSTORAGE; }
	bool	GetHasDiskStorage()		{ return (mFlags & SCANLINE_PAGE_HASDISKSTORAGE) ? true : false; }

	void	SetValidDisk()	{ mFlags |= SCANLINE_PAGE_VALIDDISK; }
	void	ClearValidDisk(){ mFlags &= ~SCANLINE_PAGE_VALIDDISK; }
	bool	GetValidDisk()	{ return (mFlags & SCANLINE_PAGE_VALIDDISK) ? true : false; }

	int		GetFileIndex()	{ return mFileIndex; }
	int		GetPageSize()	{ return mPageSize; }

	void	ReadPage(int fileHandle)
		{
		int fileIndex = GetFileIndex();
		int pageSize = GetPageSize();
		_lseek(fileHandle, fileIndex, SEEK_SET);
		_read(fileHandle, mData, pageSize);
		ClearNeedWrite();
		SetInMemory();
		}

	void	WritePage(int fileHandle)
		{
		int fileIndex = GetFileIndex();
		int pageSize = GetPageSize();
		_lseek(fileHandle, fileIndex, SEEK_SET);
		_write(fileHandle, mData, pageSize);
		ClearNeedWrite();
		SetValidDisk();
		}

private:
	T*				mData;
	int				mFileIndex;
	int				mFirstScanLine;
	int				mNumScanLines;
	int				mWidth;
	int				mPageSize;
	int				mLockCount;
	BYTE			mFlags;
	};

template <class T> class ScanLinePageMgr {
public:
	ScanLinePageMgr()
		{
		mFileHandle = -1;
		mScanLinesPerPage = 0;
		mNumPages = 0;
		mWidth = 0;
		mFilename = NULL;
		mIsPaged = 0;

		SYSTEM_INFO	si;
		GetSystemInfo(&si);
		mNumPagesInMemory = 2*si.dwNumberOfProcessors;
		pMru = new int[mNumPagesInMemory];

		InitializeCriticalSection(&cs);
		for (int p=0; p<mNumPagesInMemory; p++) {
			pMru[p] = -1;
			}
		}

	bool Initialize(int width, int height, int memThreshold, int pageSize)
		{
		mWidth = width;
		int	totalSize = width * height * sizeof(T);
		if (memThreshold && (totalSize > memThreshold)) {
			mIsPaged = true;

			char baseName[_MAX_PATH];
			char fn[_MAX_PATH];

			// TBD: Need a system page directory
			strcpy(baseName, GetCOREInterface()->GetDir(APP_AUTOBACK_DIR));
			if (strlen(baseName) && baseName[strlen(baseName)-1] != '\\') {
				strcat(baseName, "\\");
				}

			// Generate a unique filename
			bool bGotUniqueName = false;
			int tmpIndex = 0;
			while (!bGotUniqueName) {
				sprintf(fn, "%s3dsmax_pager%d.tmp", baseName, tmpIndex);
				if (_access(fn, 0) !=0) {
					bGotUniqueName = true;
					}
				tmpIndex++;
				}

			mFilename = (char*)malloc(strlen(fn)+1);
			if (!mFilename) {
				return false;
				}

			strcpy(mFilename, fn);

			// These temporary files are deleted by the system when the last handle is closed.
			mFileHandle = _open(mFilename, _O_CREAT | O_TRUNC | _O_TEMPORARY | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
			if (mFileHandle == -1) {
				return false;
				}

			if (_chsize(mFileHandle, totalSize) == -1) {
				return false;
				}

			int scanLineSize = width*sizeof(T);
			if (pageSize < scanLineSize) {
				pageSize = scanLineSize;
				}

			// By rounding up the pageSize we get it aligned to the actual scanLine boundaries. 
			mScanLinesPerPage = pageSize/scanLineSize;
			pageSize = mScanLinesPerPage*scanLineSize;

			mNumPages = height/mScanLinesPerPage;
			if (mNumPages*mScanLinesPerPage < height)
				mNumPages++;

			for (int i=0; i<mNumPages; i++) {
				int fileIndex = i*pageSize;
				ScanLinePage<T>* page = new ScanLinePage<T>(width, mScanLinesPerPage, mScanLinesPerPage*i, pageSize, fileIndex);
				if (!page) {
					_close(mFileHandle);
					return false;
					}
				mPageTable.Append(1, &page, mNumPages);
				}
			}
		else {
			mIsPaged = false;
			mFilename = NULL;

			bool allocOk = false;
			ScanLinePage<T>* page = new ScanLinePage<T>(width, height, &allocOk);
			if (!page) {
				return false;
				}
			if (!allocOk) {
				delete page;
				return false;
				}
			mPageTable.Append(1, &page, 0);
			}
		return true;
		}

	~ScanLinePageMgr()
		{
		for (int i=0; i<mPageTable.Count(); i++) {
			ScanLinePage<T>* page = mPageTable[i];
			delete page;
			}
		mPageTable.ZeroCount();
		mPageTable.Shrink();

		if (mIsPaged) {
			_close(mFileHandle);
			free(mFilename);
			}

		delete [] pMru;

		DeleteCriticalSection(&cs);
		}

	// Returns a pointer to the storage is the map is not paged.
	T* GetBuffer()
		{
		if (!mIsPaged)
			{
			return mPageTable[0]->GetBuffer();
			}
		return NULL;
		}

	const T* OpenScanLine(int scanline)
		{
		if (!mIsPaged) {
			return mPageTable[0]->GetBuffer(scanline, false);
			}
		else {
			int activePage = GetPageIndex(scanline);
			EnterCriticalSection(&cs);

			AddPageToMRU(activePage);

			FlushUnusedPages();
			ScanLinePage<T>* page = GetPage(activePage);
			page->LockPage();
			page->PreparePage(GetFileHandle());
			LeaveCriticalSection(&cs);

			return page->GetBuffer(scanline, false);
			}
		}

	void CloseScanLine(int scanline)
		{
		if (mIsPaged) {
			int activePage = GetPageIndex(scanline);
			EnterCriticalSection(&cs);
			ScanLinePage<T>* page = GetPage(activePage);
			page->UnlockPage();
			LeaveCriticalSection(&cs);
			}
		}

	bool PutScanLine(int scanline, const T* buf)
		{
		if (!mIsPaged) {
			memcpy(mPageTable[0]->GetBuffer(scanline, true), buf, mWidth*sizeof(T));
			}
		else {
			int activePage = GetPageIndex(scanline);
			EnterCriticalSection(&cs);

			AddPageToMRU(activePage);

			FlushUnusedPages();
			ScanLinePage<T>* page = GetPage(activePage);
			page->LockPage();
			page->PreparePage(GetFileHandle());
			memcpy(page->GetBuffer(scanline, true), buf, mWidth*sizeof(T));
			page->UnlockPage();
			LeaveCriticalSection(&cs);
			}

		return true;
		}

	T GetPixel(int x, int y)
		{
		T	col = (T)0;
		const T* buffer = OpenScanLine(y);

		if (buffer) {
			col = buffer[x];
			}

		CloseScanLine(y);

		return col;
		}

	void PutPixel(int x, int y, T color)
		{
		if (!mIsPaged) {
			T* buffer = mPageTable[0]->GetBuffer(y, true);
			buffer[x] = color;
			}
		else {
			int activePage = GetPageIndex(y);
			EnterCriticalSection(&cs);

			AddPageToMRU(activePage);

			FlushUnusedPages();
			ScanLinePage<T>* page = GetPage(activePage);
			page->LockPage();
			page->PreparePage(GetFileHandle());

			T* buffer = page->GetBuffer(y, true);
			buffer[x] = color;

			page->UnlockPage();
			LeaveCriticalSection(&cs);
			}
		}

private:
	int		GetPageIndex(int scanline)
		{
		int pageId = mIsPaged ? (int)((float)scanline / (float)mScanLinesPerPage) : 0;
		return pageId;
		}

	void	AddPageToMRU(int page)
		{
		// speed up multiple access as it's most likely to be on top
		if (pMru[0] == page)
			return;

		int nListPos = -1;
		// See if page is in the MRU
		for (int p=0; p<mNumPagesInMemory; p++) {
			if (page == pMru[p]) {
				nListPos = p;
				break;
				}
			}

		if (nListPos != -1) {
			// Page is in the MRU list, move it to the top
			for (int p=nListPos; p>0; p--) {
				pMru[p] = pMru[p-1];
				}
			pMru[0] = page;
			}
		else {
			// Page is not in the MRU list, put it at the top of the list
			for (int p=(mNumPagesInMemory-1); p>0; p--) {
				pMru[p] = pMru[p-1];
				}
			pMru[0] = page;
			}
		}

	ScanLinePage<T>*	GetPage(int pageIndex)		{ return mPageTable[pageIndex]; }
	void	FlushUnusedPages()
		{
		int numPages = GetNumPages();

		for (int i=0; i<numPages; i++) {
			bool	bInMru = false;
			for (int p=0; p<mNumPagesInMemory; p++) {
				if (i == pMru[p]) {
					bInMru = true;
					break;
					}
				}
			if (!bInMru) {
				mPageTable[i]->FlushPage(GetFileHandle());
				}
			}
		}

	int		GetFileHandle()				{ return mFileHandle; }
	int		GetNumPages()				{ return mNumPages; }

private:
	Tab <ScanLinePage<T>*>	mPageTable;
	int				mFileHandle;
	int				mScanLinesPerPage;
	int				mNumPages;
	int				mWidth;
	char*			mFilename;
	bool			mIsPaged;
	int				mNumPagesInMemory;
	int*			pMru;
	CRITICAL_SECTION cs;
	};

#endif