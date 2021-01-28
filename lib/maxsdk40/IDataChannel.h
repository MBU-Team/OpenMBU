 /**********************************************************************
 
	FILE: IDataChannel.h

	DESCRIPTION:  Intelligent Data Channel API

	CREATED BY: Attila Szabo, Discreet

	HISTORY: [attilas|19.6.2000]


 *>	Copyright (c) 1998-2000, All Rights Reserved.
 **********************************************************************/

#ifndef __IDATACHANNEL__H
#define __IDATACHANNEL__H

#include "maxtypes.h"

// Macros to be used if any of the method below are implemented in a dll
// The dll project that implements the methods needs to define DATACHANNEL_IMP
#ifdef DATACHANNEL_IMP
#define DataChanExport __declspec(dllexport)
#else
#define DataChanExport __declspec(dllimport)
#endif

// forward declaration
class FPInterface;

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// A data channel is a homogeneous collection of objects of a user defined 
// type (data objects). Data channels are uniquely identified by a Class_ID.
//
// Data channels can be associated with any element type of a Max object:
// faces or vertexes of Meshes, etc.
//________________________________________________________________________
class IDataChannel : public InterfaceServer
{
	public:
		// Returns the unique id of the channel
		virtual Class_ID DataChannelID() const =0;

		// Returns the number of data objects in this channel
		virtual ULONG Count() const { return 0; }
		
		// Self destruction
		virtual void DeleteThis() = 0;
};

// interface ID
#define DATACHANNEL_INTERFACE Interface_ID(0x38a718a8, 0x14685b4b)
#define GetDataChannelInterface(obj) ((IDataChannel*)obj->GetInterface(DATACHANNEL_INTERFACE)) 

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Face-data channel interface
// 
// This is an abstraction of a collection of data objects that is 
// associated with faces of Max objects
// Max objects that have face-data channels call the methods of this interface
// when those faces change in some way. The data channels can then react to
// the changes to the faces.
//
// Currently only Meshes support face-data channels.
//________________________________________________________________________
class IFaceDataChannel : public IDataChannel
{
	public:

		//
		// --- face specific operations\events ---
		//
		// These methods are called by the owner of face-data channels
		// when its faces change in some way. It's up to the face-data channel 
		// to do wathever it wants to do on these notification methods.

		// Called when num new faces were created at index at in the
		// object's list of faces. Returns TRUE on success
		// ULONG at - index in the object's array of faces where the new faces 
		// were inserted
		// ULONG num - the number of new faces created
		virtual BOOL FacesCreated( ULONG at, ULONG num ) = 0;
		
		// Called when the owner object has cloned some of its faces and appended
		// then to its list of faces.
		// BitArray& set - bitarray with as many bits as many faces the owner 
		// object has. Bits set to 1 correspond to cloned faces
		virtual BOOL FacesClonedAndAppended( BitArray& set ) = 0;

		// Called when faces were deleted in the owner object. Returns TRUE on success
		// BitArray& set - bitarray with as many bits as many faces the owner 
		// object has. Bits set to 1 correspond to deleted faces
		virtual BOOL FacesDeleted( BitArray& set ) = 0;

		// Called when faces were deleted in the owner object. Returns TRUE on success
		// Allwos for a more efficient deletion of a range of data objects
		// than using a BitArray
		// ULONG from - index in the object's array of faces. Faces starting 
		// from this index were deleted
		// ULONG num - number of faces that were deleted
		virtual BOOL FacesDeleted( ULONG from, ULONG num ) = 0;
		
		// Called when all faces in the owner object are deleted
		virtual void AllFacesDeleted() = 0;

		// Called when a face has been copied from index from in the owner object's
		// array of faces to the face at index to.
		// ULONG from - index of source face
		// ULONG to - index of dest face
		virtual BOOL FaceCopied( ULONG from, ULONG to ) = 0;

		// Called when a new face has been created in the owner object based on
		// data interpolated from other faces
		// ULONG numSrc - the number of faces used in the interpolation
		// ULONG* srcFaces - array of numSrc face indeces in the owner object's 
		// face array. These faces were used when creating the new face
		// float* coeff - array of numSrc coefficients used in the interpolation
		// ULONG targetFace - the index in the owner object's array of faces of the 
		// newly created face
		virtual BOOL FaceInterpolated( ULONG numSrc, 
															ULONG* srcFaces, 
															float* coeff, 
															ULONG targetFace ) = 0;

		//
		// --- geometry pipeline (stack) specific methods ---
		//
		// These methods are called when the owner object is flowing up the 
		// pipeline (stack). They must be implemented to ensure that the 
		// face-data channel flows up the pipeline correctly.
		// The owner object expects the face-data to do exactly what the 
		// names of these methods imply. These can be seen as commands that are
		// given by the owner object to the face-data channel

		// Allocates an empty data-channel 
		virtual IFaceDataChannel* CreateChannel( ) = 0;

		// The data-channel needs to allocate a new instance of itself and fill it 
		// with copies of all data items it stores.
		// This method exist to make it more efficient to clone the whole data-channel
		virtual IFaceDataChannel* CloneChannel( ) = 0;

		// The data-channel needs to append the data objects in the fromChan
		// to itself. 
		virtual BOOL AppendChannel( const IFaceDataChannel* fromChan ) = 0;
};

// interface ID
#define FACEDATACHANNEL_INTERFACE Interface_ID(0x181358d5, 0x3cab1bc9)
#define GetFaceDataChannelInterface(obj) ((IFaceDataChannel*)obj->GetInterface(FACEDATACHANNEL_INTERFACE)) 

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Interface class that allows to execute a callback method (Proc) for all 
// face-data channels of an object.
//
// Developers should derive their own classes from this interface and 
// overwrite the Proc method to call the desired IFaceDataChannel method
// It is up to derived class to interpret the context parameter passed to
// Proc.
//
// --- Usage ---
// Classes that hold face-data channels, can implement a method called
// EnumFaceDataChannels( IFaceDataEnumCallBack& cb, void* pContext)
// This method would be called with a reference to an instance of a class
// derived from IFaceDataEnumCallBack in which Proc was overwritten. The 
// implementation of EnumFaceDataChannels would call cb.Proc for each of 
// the face-data channels of the object
//
// Warning:
// Deleting data channels from within Proc can lead to unexpected behaviour
//________________________________________________________________________
class IFaceDataChannelsEnumCallBack
{
	public:
		virtual BOOL Proc( IFaceDataChannel* pChan, void* pContext ) = 0;
};




#endif 