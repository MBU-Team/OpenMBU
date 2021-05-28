/**********************************************************************
 *<
	FILE: gbuf.h : GBuffer manager.

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

#ifndef __GBUF__H
#define __GBUF__H

#define NUMGBCHAN 14

// GBuffer channels (number of bytes in parenthesis)
#define GB_Z       			0  	// (4)  Z-Buffer depth, float
#define GB_MTL_ID  			1  	// (1)  ID assigned to mtl via mtl editor
#define GB_NODE_ID 			2  	// (2)  ID assigned to node via properties
#define GB_UV       		3 	// (8)  UV coordinates - Point2 
#define GB_NORMAL   		4 	// (4)  Normal vector in view space, compressed 
#define GB_REALPIX  		5 	// (4)  Non clamped colors in "RealPixel" format 
#define GB_COVERAGE 		6 	// (1)  Pixel coverage  
#define GB_BG 	     		7 	// (3)  RGB color of what's behind layer 
#define GB_NODE_RENDER_ID 	8 	// (2)  Node render index, word
#define GB_COLOR		 	9 	// (3)  Color (RGB)
#define GB_TRANSP		 	10 	// (3)  Transparency (RGB)
#define GB_VELOC		 	11 	// (8)  Velocity (Point2)
#define GB_WEIGHT		 	12 	// (3)  Weight of layers contribution to pixel color
#define GB_MASK			 	13 	// (2)  Sub pixal coverage mask

CoreExport int GBDataSize(int i);

CoreExport TCHAR *GBChannelName(int i);

// Recognized channel bits 

#define BMM_CHAN_NONE     0
#define BMM_CHAN_Z        (1<<GB_Z) 		//  Z-buffer depth, float 
#define BMM_CHAN_MTL_ID   (1<<GB_MTL_ID) 	//  ID assigned to mtl via mtl editor 
#define BMM_CHAN_NODE_ID  (1<<GB_NODE_ID) 	//  ID assigned to node via properties 
#define BMM_CHAN_UV       (1<<GB_UV) 		//  UV coordinates - Point2 
#define BMM_CHAN_NORMAL   (1<<GB_NORMAL) 	//  Normal vector in view space, compressed 
#define BMM_CHAN_REALPIX  (1<<GB_REALPIX) 	//  Non clamped colors in "RealPixel" format 
#define BMM_CHAN_COVERAGE (1<<GB_COVERAGE) 	//  Pixel coverage of front surface 
#define BMM_CHAN_BG 	  (1<<GB_BG) 		//  RGB color of what's behind front object 
#define BMM_CHAN_NODE_RENDER_ID (1<<GB_NODE_RENDER_ID) //  node render index 
#define BMM_CHAN_COLOR    (1<<GB_COLOR) 	//  Color (Color24) 
#define BMM_CHAN_TRANSP   (1<<GB_TRANSP) 	//  Transparency (Color24) 
#define BMM_CHAN_VELOC    (1<<GB_VELOC) 	//  Velocity ( Point2 ) 
#define BMM_CHAN_WEIGHT   (1<<GB_WEIGHT) 	//  Weight ( Color24 ) 
#define BMM_CHAN_MASK	  (1<<GB_MASK)   	//  Subpixel mask ( word ) 

// Recognized types of channels
#define BMM_CHAN_TYPE_UNKNOWN 0 
#define BMM_CHAN_TYPE_8   2 // 1 byte per pixel
#define BMM_CHAN_TYPE_16  3 // 1 word per pixel
#define BMM_CHAN_TYPE_24  8 // 3 bytes per pixel
#define BMM_CHAN_TYPE_32  4 // 2 words per pixel
#define BMM_CHAN_TYPE_48  5 // 3 words per pixel
#define BMM_CHAN_TYPE_64  6 // 4 words per pixel
#define BMM_CHAN_TYPE_96  7 // 6 words per pixel

struct GBufData {
	float z;
	UBYTE mtl_id;
	UWORD node_id;
	Point2 uv;
	DWORD normal;
	RealPixel realpix;
	UBYTE coverage;
	UWORD rend_id;
	Color24 color;
	Color24 transp;
	Color24 weight;
	Point2 veloc;
	UWORD mask;
	};
	

//------------------------------------gbuf.h ----------------------------------------------

class GBufReader  : public InterfaceServer {
	public:
	virtual int  StartLine(int y)=0;      	// -1 = no data for line, or x of first non-empty pixel
	virtual BOOL StartPixel(int x)=0;     	// -1 = eol,; 0 = empty; 1= has data  ( Automatically starts first layer)
	virtual BOOL StartPixel(int x,int y)=0; // -1 = eol,; 0 = empty; 1= has data ( includes StartLine)
	virtual BOOL StartNextLayer()=0;		// 0 = no more layers ( Do not call before first layer )
	virtual	int NextPixel()=0;       		// -1 = eol,; 0 = empty; 1 = has data
	// Read a data element: chan is one of { GB_Z, GB_MTL_ID, GB_NODE_ID, ... etc }
	virtual BOOL ReadChannelData(int chan, void *data)=0; // 1= has data;  0= didnt have data
	virtual BOOL ReadAllData(GBufData *data)=0; // 1= has data;  0= didnt have data
	virtual BOOL ModifyChannelData(int chan, void *data)=0;  // 1=success   0=fail
	virtual BOOL ModifyAllData(GBufData *data)=0; // 1= success;  0= fail
	virtual void DeleteThis()=0;
	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
	};	


// This assumes pixels are created in increasing order of x.
class GBufWriter : public InterfaceServer {
	public:
	virtual void StartLine(int y)=0;		 // 1 = success  0 = fail ( Must call before every line)
	virtual void StartPixel(int x)=0;        // 1 = success  0 = fail  ( Must call to start each pixel )
	virtual void StartNextLayer()=0;		 // 1 = success  0 = fail  ( Must call before first layer)
	// Write a data element: chan is one of { GB_Z, GB_MTL_ID, GB_NODE_ID, ... etc }
	virtual BOOL WriteChannelData(int chan, void *data)=0;  // 1=success   0=fail
	virtual BOOL WriteAllData(GBufData *data)=0; // 1= success;  0= fail
	virtual BOOL EndLine()=0;                 // 1=success   0=fail ( Just call after every line)
	virtual void DeleteThis()=0;
	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
	};	

class GBuffer : public InterfaceServer {
	public:
	virtual void SetRasterSize(int ww, int hh)=0;  	  // when create bitmap or size changes.
	virtual int Width()=0;
	virtual int Height()=0;
	virtual int InitBuffer()=0;    					   // call before writing buffer.
	virtual ULONG CreateChannels(ULONG channelMask)=0; // create specified channels
	virtual void  DeleteChannels(ULONG channelMask)=0; // delete specified channels 
	virtual ULONG ChannelsPresent()=0;
	virtual void  *GetChannel( ULONG channelID, ULONG& chanType)=0; 
	virtual GBufReader *CreateReader()=0;
	virtual void DestroyReader(GBufReader *pRdr)=0;
	virtual GBufWriter *CreateWriter()=0;
	virtual void DestroyWriter(GBufWriter *pRdr)=0;
	virtual BOOL IsDefaultGBuffer() { return FALSE; }
	virtual void DeleteThis()=0;
	virtual void Copy(GBuffer *gbfrom)=0;
	virtual	void CopyScale(GBuffer *gbfrom, int cw=-1, int ch=-1)=0;
	virtual void Position(int srcy, int trgy, int srcx, int trgx, int trgw, int trgh )=0;

	// for file writing
	virtual int  NumberLayerRecords(int y)=0;  
	virtual int  GetLayerChannel(int y, int ichan, char *data)=0; // ichan = -1 creates array of x values

	// for file reading
	virtual int CreateLayerRecords(int y, int num)=0;  
	virtual int SetLayerChannel(int y, int ichan, char *data)=0; // ichan = -1 gets array of x values
	
	// This scans the entire image and updates the minimum and maximum values
	virtual void UpdateChannelMinMax()=0;

	// Get Channel limits  ( currently only valid for z,uv,veloc)
	virtual BOOL GetChannelMin(int chan, void *data)=0;
	virtual BOOL GetChannelMax(int chan, void *data)=0;
	
	// names indexed by NodeRenderId
	virtual NameTab &NodeRenderIDNameTab()=0;

	// Array of Image blur multipliers indexed by NodeRenderID
	virtual Tab<float> &ImageBlurMultiplierTab()=0;

	// Generic expansion function
	// JH 11/17/00 Now deriving from InterfaceServer. Extesibility via GetInterface()
	//virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 

	};

CoreExport void SetMaximumGBufferLayerDepth(int m);
CoreExport int GetMaximumGBufferLayerDepth();

CoreExport GBuffer *NewDefaultGBuffer();

#endif// __GBUF__H
