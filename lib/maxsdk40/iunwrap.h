//-------------------------------------------------------------
// Access to UVW Unwrap
//


#ifndef __IUNWRAP__H
#define __IUNWRAP__H

#include "iFnPub.h"


#define UNWRAP_CLASSID	Class_ID(0x02df2e3a,0x72ba4e1f)

// Flags
#define CONTROL_FIT			(1<<0)
#define CONTROL_CENTER		(1<<1)
#define CONTROL_ASPECT		(1<<2)
#define CONTROL_UNIFORM		(1<<3)
#define CONTROL_HOLD		(1<<4)
#define CONTROL_INIT		(1<<5)
#define CONTROL_OP			(CONTROL_FIT|CONTROL_CENTER|CONTROL_ASPECT|CONTROL_UNIFORM)
#define CONTROL_INITPARAMS	(1<<10)

#define IS_MESH		1
#define IS_PATCH	2
#define IS_NURBS	3
#define IS_MNMESH	4




#define FLAG_DEAD		1
#define FLAG_HIDDEN		2
#define FLAG_FROZEN		4
//#define FLAG_QUAD		8
#define FLAG_SELECTED	16
#define FLAG_CURVEDMAPPING	32
#define FLAG_INTERIOR	64


#define ID_TOOL_MOVE	0x0100
#define ID_TOOL_ROTATE	0x0110
#define ID_TOOL_SCALE	0x0120
#define ID_TOOL_PAN		0x0130
#define ID_TOOL_ZOOM    0x0140
#define ID_TOOL_PICKMAP	0x0160
#define ID_TOOL_ZOOMREG 0x0170
#define ID_TOOL_UVW		0x0200
#define ID_TOOL_PROP	0x0210 
#define ID_TOOL_SHOWMAP	0x0220
#define ID_TOOL_UPDATE	0x0230
#define ID_TOOL_ZOOMEXT	0x0240
#define ID_TOOL_BREAK	0x0250
#define ID_TOOL_WELD	0x0260
#define ID_TOOL_WELD_SEL 0x0270
#define ID_TOOL_HIDE	 0x0280
#define ID_TOOL_UNHIDE	 0x0290
#define ID_TOOL_FREEZE	 0x0300
#define ID_TOOL_UNFREEZE	 0x0310
#define ID_TOOL_TEXTURE_COMBO 0x0320
#define ID_TOOL_SNAP 0x0330
#define ID_TOOL_LOCKSELECTED 0x0340
#define ID_TOOL_MIRROR 0x0350
#define ID_TOOL_FILTER_SELECTEDFACES 0x0360
#define ID_TOOL_FILTER_MATID 0x0370
#define ID_TOOL_INCSELECTED 0x0380
#define ID_TOOL_FALLOFF 0x0390
#define ID_TOOL_FALLOFF_SPACE 0x0400
#define ID_TOOL_FLIP 0x0410







class IUnwrapMod;

//***************************************************************
//Function Publishing System stuff   
//****************************************************************
#define UNWRAP_CLASSID	Class_ID(0x02df2e3a,0x72ba4e1f)
#define UNWRAP_INTERFACE Interface_ID(0x53b3409b, 0x18ff7ab8)

#define GetIUnwrapInterface(cd) \
			(IUnwrapMod *)(cd)->GetInterface(UNWRAP_INTERFACE)

enum {  unwrap_planarmap,unwrap_save,unwrap_load, unwrap_reset, unwrap_edit,
		unwrap_setMapChannel,unwrap_getMapChannel,
		unwrap_setProjectionType,unwrap_getProjectionType,
		unwrap_setVC,unwrap_getVC,
		unwrap_move,unwrap_moveh,unwrap_movev,
		unwrap_rotate,
		unwrap_scale,unwrap_scaleh,unwrap_scalev,
		unwrap_mirrorh,unwrap_mirrorv,
		unwrap_expandsel, unwrap_contractsel,
		unwrap_setFalloffType,unwrap_getFalloffType,
		unwrap_setFalloffSpace,unwrap_getFalloffSpace,
		unwrap_setFalloffDist,unwrap_getFalloffDist,
		unwrap_breakselected,
		unwrap_weldselected, unwrap_weld,
		unwrap_updatemap,unwrap_displaymap,unwrap_ismapdisplayed,
		unwrap_setuvspace, unwrap_getuvspace,
		unwrap_options,
		unwrap_lock,
		unwrap_hide, unwrap_unhide,
		unwrap_freeze, unwrap_thaw,
		unwrap_filterselected,
		unwrap_pan,unwrap_zoom, unwrap_zoomregion, unwrap_fit, unwrap_fitselected,
		unwrap_snap,
		unwrap_getcurrentmap,unwrap_setcurrentmap, unwrap_numbermaps,
		unwrap_getlinecolor,unwrap_setlinecolor,
		unwrap_getselectioncolor,unwrap_setselectioncolor,
		unwrap_getrenderwidth,unwrap_setrenderwidth,
		unwrap_getrenderheight,unwrap_setrenderheight,
		unwrap_getusebitmapres,unwrap_setusebitmapres,
		unwrap_getweldtheshold,unwrap_setweldtheshold,

		unwrap_getconstantupdate,unwrap_setconstantupdate,
		unwrap_getshowselectedvertices,unwrap_setshowselectedvertices,
		unwrap_getmidpixelsnap,unwrap_setmidpixelsnap,

		unwrap_getmatid,unwrap_setmatid, unwrap_numbermatids,
		unwrap_getselectedverts, unwrap_selectverts,
		unwrap_isvertexselected,

		unwrap_moveselectedvertices,
		unwrap_rotateselectedverticesc,
		unwrap_rotateselectedvertices,
		unwrap_scaleselectedverticesc,
		unwrap_scaleselectedvertices,
		unwrap_getvertexposition,
		unwrap_numbervertices,

		unwrap_movex, unwrap_movey, unwrap_movez,

		unwrap_getselectedpolygons, unwrap_selectpolygons, unwrap_ispolygonselected,
		unwrap_numberpolygons,

		unwrap_detachedgeverts,
		unwrap_fliph,unwrap_flipv ,

		unwrap_setlockaspect,unwrap_getlockaspect,

		unwrap_setmapscale,unwrap_getmapscale,
		unwrap_getselectionfromface,

		unwrap_forceupdate,
		unwrap_zoomtogizmo,

		unwrap_setvertexposition,
		unwrap_addvertex,
		unwrap_markasdead,

		unwrap_numberpointsinface,
		unwrap_getvertexindexfromface,
		unwrap_gethandleindexfromface,
		unwrap_getinteriorindexfromface,

		unwrap_getvertexgindexfromface,
		unwrap_gethandlegindexfromface,
		unwrap_getinteriorgindexfromface,


		unwrap_addpointtoface,
		unwrap_addpointtohandle,
		unwrap_addpointtointerior,

		unwrap_setfacevertexindex,
		unwrap_setfacehandleindex,
		unwrap_setfaceinteriorindex,

		unwrap_updateview,

		unwrap_getfaceselfromstack,



		};
//****************************************************************


class IUnwrapMod :  public Modifier, public FPMixinInterface 
	{
	public:

		//Function Publishing System
		//Function Map For Mixin Interface
		//*************************************************
		BEGIN_FUNCTION_MAP
			VFN_0(unwrap_planarmap, fnPlanarMap);
			VFN_0(unwrap_save, fnSave);
			VFN_0(unwrap_load, fnLoad);
			VFN_0(unwrap_reset, fnReset);
			VFN_0(unwrap_edit, fnEdit);
			VFN_1(unwrap_setMapChannel, fnSetMapChannel,TYPE_INT);
			FN_0(unwrap_getMapChannel, TYPE_INT, fnGetMapChannel);
			VFN_1(unwrap_setProjectionType, fnSetProjectionType,TYPE_INT);
			FN_0(unwrap_getProjectionType, TYPE_INT, fnGetProjectionType);
			VFN_1(unwrap_setVC, fnSetVC,TYPE_BOOL);
			FN_0(unwrap_getVC, TYPE_BOOL, fnGetVC);

			VFN_0(unwrap_move, fnMove);
			VFN_0(unwrap_moveh, fnMoveH);
			VFN_0(unwrap_movev, fnMoveV);

			VFN_0(unwrap_rotate, fnRotate);

			VFN_0(unwrap_scale, fnScale);
			VFN_0(unwrap_scaleh, fnScaleH);
			VFN_0(unwrap_scalev, fnScaleV);

			VFN_0(unwrap_mirrorh, fnMirrorH);
			VFN_0(unwrap_mirrorv, fnMirrorV);
			VFN_0(unwrap_expandsel, fnExpandSelection);
			VFN_0(unwrap_contractsel, fnContractSelection);
			VFN_1(unwrap_setFalloffType, fnSetFalloffType,TYPE_INT);
			FN_0(unwrap_getFalloffType, TYPE_INT, fnGetFalloffType);
			VFN_1(unwrap_setFalloffSpace, fnSetFalloffSpace,TYPE_INT);
			FN_0(unwrap_getFalloffSpace, TYPE_INT, fnGetFalloffSpace);
			VFN_1(unwrap_setFalloffDist, fnSetFalloffDist,TYPE_FLOAT);
			FN_0(unwrap_getFalloffDist, TYPE_FLOAT, fnGetFalloffDist);
			VFN_0(unwrap_breakselected, fnBreakSelected);
			VFN_0(unwrap_weld, fnWeld);
			VFN_0(unwrap_weldselected, fnWeldSelected);
			VFN_0(unwrap_updatemap, fnUpdatemap);
			VFN_1(unwrap_displaymap, fnDisplaymap, TYPE_BOOL);
			FN_0(unwrap_ismapdisplayed, TYPE_BOOL, fnIsMapDisplayed);
			VFN_1(unwrap_setuvspace, fnSetUVSpace,TYPE_INT);
			FN_0(unwrap_getuvspace, TYPE_INT, fnGetUVSpace);
			VFN_0(unwrap_options, fnOptions);
			VFN_0(unwrap_lock, fnLock);
			VFN_0(unwrap_hide, fnHide);
			VFN_0(unwrap_unhide, fnUnhide);
			VFN_0(unwrap_freeze, fnFreeze);
			VFN_0(unwrap_thaw, fnThaw);
			VFN_0(unwrap_filterselected, fnFilterSelected);

			VFN_0(unwrap_pan, fnPan);
			VFN_0(unwrap_zoom, fnZoom);
			VFN_0(unwrap_zoomregion, fnZoomRegion);
			VFN_0(unwrap_fit, fnFit);
			VFN_0(unwrap_fitselected, fnFitSelected);

			VFN_0(unwrap_snap, fnSnap);

			FN_0(unwrap_getcurrentmap,TYPE_INT, fnGetCurrentMap);
			VFN_1(unwrap_setcurrentmap, fnSetCurrentMap,TYPE_INT);
			FN_0(unwrap_numbermaps,TYPE_INT, fnNumberMaps);

			FN_0(unwrap_getlinecolor,TYPE_POINT3, fnGetLineColor);
			VFN_1(unwrap_setlinecolor, fnSetLineColor,TYPE_POINT3);
			FN_0(unwrap_getselectioncolor,TYPE_POINT3, fnGetSelColor);
			VFN_1(unwrap_setselectioncolor, fnSetSelColor,TYPE_POINT3);

			FN_0(unwrap_getrenderwidth,TYPE_INT, fnGetRenderWidth);
			VFN_1(unwrap_setrenderwidth, fnSetRenderWidth,TYPE_INT);
			FN_0(unwrap_getrenderheight,TYPE_INT, fnGetRenderHeight);
			VFN_1(unwrap_setrenderheight, fnSetRenderHeight,TYPE_INT);

			FN_0(unwrap_getusebitmapres,TYPE_BOOL, fnGetUseBitmapRes);
			VFN_1(unwrap_setusebitmapres, fnSetUseBitmapRes,TYPE_BOOL);

			FN_0(unwrap_getweldtheshold,TYPE_FLOAT, fnGetWeldThresold);
			VFN_1(unwrap_setweldtheshold, fnSetWeldThreshold,TYPE_FLOAT);


			FN_0(unwrap_getconstantupdate,TYPE_BOOL, fnGetConstantUpdate);
			VFN_1(unwrap_setconstantupdate, fnSetConstantUpdate,TYPE_BOOL);

			FN_0(unwrap_getshowselectedvertices,TYPE_BOOL, fnGetShowSelectedVertices);
			VFN_1(unwrap_setshowselectedvertices, fnSetShowSelectedVertices,TYPE_BOOL);

			FN_0(unwrap_getmidpixelsnap,TYPE_BOOL, fnGetMidPixelSnape);
			VFN_1(unwrap_setmidpixelsnap, fnSetMidPixelSnape,TYPE_BOOL);


			FN_0(unwrap_getmatid,TYPE_INT, fnGetMatID);
			VFN_1(unwrap_setmatid, fnSetMatID,TYPE_INT);
			FN_0(unwrap_numbermatids,TYPE_INT, fnNumberMatIDs);

			FN_0(unwrap_getselectedverts,TYPE_BITARRAY, fnGetSelectedVerts);
			VFN_1(unwrap_selectverts, fnSelectVerts,TYPE_BITARRAY);
			FN_1(unwrap_isvertexselected,TYPE_BOOL, fnIsVertexSelected,TYPE_INT);

			VFN_1(unwrap_moveselectedvertices, fnMoveSelectedVertices,TYPE_POINT3);
			VFN_1(unwrap_rotateselectedverticesc, fnRotateSelectedVertices,TYPE_FLOAT);
			VFN_2(unwrap_rotateselectedvertices, fnRotateSelectedVertices,TYPE_FLOAT, TYPE_POINT3);
			VFN_2(unwrap_scaleselectedverticesc, fnScaleSelectedVertices,TYPE_FLOAT, TYPE_INT);
			VFN_3(unwrap_scaleselectedvertices, fnScaleSelectedVertices,TYPE_FLOAT, TYPE_INT,TYPE_POINT3);

			FN_2(unwrap_getvertexposition,TYPE_POINT3, fnGetVertexPosition, TYPE_TIMEVALUE, TYPE_INT);
			FN_0(unwrap_numbervertices,TYPE_INT, fnNumberVertices);

			VFN_1(unwrap_movex, fnMoveX,TYPE_FLOAT);
			VFN_1(unwrap_movey, fnMoveY,TYPE_FLOAT);
			VFN_1(unwrap_movez, fnMoveZ,TYPE_FLOAT);

			FN_0(unwrap_getselectedpolygons,TYPE_BITARRAY, fnGetSelectedPolygons);
			VFN_1(unwrap_selectpolygons, fnSelectPolygons,TYPE_BITARRAY);
			FN_1(unwrap_ispolygonselected,TYPE_BOOL, fnIsPolygonSelected,TYPE_INT);
			FN_0(unwrap_numberpolygons,TYPE_INT, fnNumberPolygons);
			VFN_0(unwrap_detachedgeverts, fnDetachEdgeVerts);
			VFN_0(unwrap_fliph, fnFlipH);
			VFN_0(unwrap_flipv, fnFlipV);
			
			VFN_1(unwrap_setlockaspect, fnSetLockAspect,TYPE_BOOL);
			FN_0(unwrap_getlockaspect,TYPE_BOOL, fnGetLockAspect);

			VFN_1(unwrap_setmapscale, fnSetMapScale,TYPE_FLOAT);
			FN_0(unwrap_getmapscale,TYPE_FLOAT, fnGetMapScale);

			VFN_0(unwrap_getselectionfromface, fnGetSelectionFromFace);

			VFN_1(unwrap_forceupdate, fnForceUpdate,TYPE_BOOL);

			VFN_1(unwrap_zoomtogizmo, fnZoomToGizmo,TYPE_BOOL);

			VFN_3(unwrap_setvertexposition, fnSetVertexPosition,TYPE_TIMEVALUE,TYPE_INT,TYPE_POINT3);
			VFN_1(unwrap_markasdead, fnMarkAsDead,TYPE_INT);

			FN_1(unwrap_numberpointsinface,TYPE_INT,fnNumberPointsInFace,TYPE_INT);
			FN_2(unwrap_getvertexindexfromface,TYPE_INT,fnGetVertexIndexFromFace,TYPE_INT,TYPE_INT);
			FN_2(unwrap_gethandleindexfromface,TYPE_INT,fnGetHandleIndexFromFace,TYPE_INT,TYPE_INT);
			FN_2(unwrap_getinteriorindexfromface,TYPE_INT,fnGetInteriorIndexFromFace,TYPE_INT,TYPE_INT);
			FN_2(unwrap_getvertexgindexfromface,TYPE_INT,fnGetVertexGIndexFromFace,TYPE_INT,TYPE_INT);
			FN_2(unwrap_gethandlegindexfromface,TYPE_INT,fnGetHandleGIndexFromFace,TYPE_INT,TYPE_INT);
			FN_2(unwrap_getinteriorgindexfromface,TYPE_INT,fnGetInteriorGIndexFromFace,TYPE_INT,TYPE_INT);
			
			VFN_4(unwrap_addpointtoface,fnAddPoint,TYPE_POINT3,TYPE_INT,TYPE_INT,TYPE_BOOL);
			VFN_4(unwrap_addpointtohandle,fnAddHandle,TYPE_POINT3,TYPE_INT,TYPE_INT,TYPE_BOOL);
			VFN_4(unwrap_addpointtointerior,fnAddInterior,TYPE_POINT3,TYPE_INT,TYPE_INT,TYPE_BOOL);

			VFN_3(unwrap_setfacevertexindex,fnSetFaceVertexIndex,TYPE_INT,TYPE_INT,TYPE_INT);
			VFN_3(unwrap_setfacehandleindex,fnSetFaceHandleIndex,TYPE_INT,TYPE_INT,TYPE_INT);
			VFN_3(unwrap_setfaceinteriorindex,fnSetFaceInteriorIndex,TYPE_INT,TYPE_INT,TYPE_INT);

			VFN_0(unwrap_updateview,fnUpdateViews);

			VFN_0(unwrap_getfaceselfromstack,fnGetFaceSelFromStack);


			
		END_FUNCTION_MAP

		FPInterfaceDesc* GetDesc();    // <-- must implement 

		virtual void	fnPlanarMap()=0;
		virtual void	fnSave()=0;
		virtual void	fnLoad()=0;
		virtual void	fnReset()=0;
		virtual void	fnEdit()=0;

		virtual void	fnSetMapChannel(int channel)=0;
		virtual int		fnGetMapChannel()=0;

		virtual void	fnSetProjectionType(int proj)=0;
		virtual int		fnGetProjectionType()=0;

		virtual void	fnSetVC(BOOL vc)=0;
		virtual BOOL	fnGetVC()=0;

		virtual void	fnMove()=0;
		virtual void	fnMoveH()=0;
		virtual void	fnMoveV()=0;

		virtual void	fnRotate()=0;

		virtual void	fnScale()=0;
		virtual void	fnScaleH()=0;
		virtual void	fnScaleV()=0;

		virtual void	fnMirrorH()=0;
		virtual void	fnMirrorV()=0;

		virtual void	fnExpandSelection()=0;
		virtual void	fnContractSelection()=0;
		

		virtual void	fnSetFalloffType(int falloff)=0;
		virtual int		fnGetFalloffType()=0;
		virtual void	fnSetFalloffSpace(int space)=0;
		virtual int		fnGetFalloffSpace()=0;
		virtual void	fnSetFalloffDist(float dist)=0;
		virtual float	fnGetFalloffDist()=0;

		virtual void	fnBreakSelected()=0;
		virtual void	fnWeld()=0;
		virtual void	fnWeldSelected()=0;

		virtual void	fnUpdatemap()=0;
		virtual void	fnDisplaymap(BOOL update)=0;
		virtual BOOL	fnIsMapDisplayed()=0;

		virtual void	fnSetUVSpace(int space)=0;
		virtual int		fnGetUVSpace()=0;
		virtual void	fnOptions()=0;

		virtual void	fnLock()=0;
		virtual void	fnHide()=0;
		virtual void	fnUnhide()=0;

		virtual void	fnFreeze()=0;
		virtual void	fnThaw()=0;
		virtual void	fnFilterSelected()=0;

		virtual void	fnPan()=0;
		virtual void	fnZoom()=0;
		virtual void	fnZoomRegion()=0;
		virtual void	fnFit()=0;
		virtual void	fnFitSelected()=0;

		virtual void	fnSnap()=0;


		virtual int		fnGetCurrentMap()=0;
		virtual void	fnSetCurrentMap(int map)=0;
		virtual int		fnNumberMaps()=0;

		virtual Point3*	fnGetLineColor()=0;
		virtual void	fnSetLineColor(Point3 color)=0;

		virtual Point3*	fnGetSelColor()=0;
		virtual void	fnSetSelColor(Point3 color)=0;



		virtual void	fnSetRenderWidth(int dist)=0;
		virtual int		fnGetRenderWidth()=0;
		virtual void	fnSetRenderHeight(int dist)=0;
		virtual int		fnGetRenderHeight()=0;
		
		virtual void	fnSetWeldThreshold(float dist)=0;
		virtual float	fnGetWeldThresold()=0;

		virtual void	fnSetUseBitmapRes(BOOL useBitmapRes)=0;
		virtual BOOL	fnGetUseBitmapRes()=0;

		
		virtual BOOL	fnGetConstantUpdate()=0;
		virtual void	fnSetConstantUpdate(BOOL constantUpdates)=0;

		virtual BOOL	fnGetShowSelectedVertices()=0;
		virtual void	fnSetShowSelectedVertices(BOOL show)=0;

		virtual BOOL	fnGetMidPixelSnape()=0;
		virtual void	fnSetMidPixelSnape(BOOL midPixel)=0;

		virtual int		fnGetMatID()=0;
		virtual void	fnSetMatID(int matid)=0;
		virtual int		fnNumberMatIDs()=0;

		virtual BitArray* fnGetSelectedVerts()=0;
		virtual void fnSelectVerts(BitArray *sel)=0;
		virtual BOOL fnIsVertexSelected(int index)=0;

		virtual void fnMoveSelectedVertices(Point3 offset)=0;
		virtual void fnRotateSelectedVertices(float angle)=0;
		virtual void fnRotateSelectedVertices(float angle, Point3 axis)=0;
		virtual void fnScaleSelectedVertices(float scale,int dir)=0;
		virtual void fnScaleSelectedVertices(float scale,int dir,Point3 axis)=0;
		virtual Point3* fnGetVertexPosition(TimeValue t,  int index)=0;
		virtual int fnNumberVertices()=0;

		virtual void fnMoveX(float p)=0;
		virtual void fnMoveY(float p)=0;
		virtual void fnMoveZ(float p)=0;

		virtual BitArray* fnGetSelectedPolygons()=0;
		virtual void fnSelectPolygons(BitArray *sel)=0;
		virtual BOOL fnIsPolygonSelected(int index)=0;
		virtual int fnNumberPolygons()=0;

		virtual void fnDetachEdgeVerts()=0;

		virtual void fnFlipH()=0;
		virtual void fnFlipV()=0;

		virtual BOOL	fnGetLockAspect()=0;
		virtual void	fnSetLockAspect(BOOL a)=0;

		virtual float	fnGetMapScale()=0;
		virtual void	fnSetMapScale(float sc)=0;

		virtual void	fnGetSelectionFromFace()=0;
		virtual void	fnForceUpdate(BOOL update)= 0;

		virtual void	fnZoomToGizmo(BOOL all)= 0;

		virtual void	fnSetVertexPosition(TimeValue t, int index, Point3 pos) = 0;
		virtual void	fnMarkAsDead(int index) = 0;

		virtual int		fnNumberPointsInFace(int index)=0;
		virtual int		fnGetVertexIndexFromFace(int index,int vertexIndex)=0;
		virtual int		fnGetHandleIndexFromFace(int index,int vertexIndex)=0;
		virtual int		fnGetInteriorIndexFromFace(int index,int vertexIndex)=0;
		virtual int		fnGetVertexGIndexFromFace(int index,int vertexIndex)=0;
		virtual int		fnGetHandleGIndexFromFace(int index,int vertexIndex)=0;
		virtual int		fnGetInteriorGIndexFromFace(int index,int vertexIndex)=0;

		virtual void	fnAddPoint(Point3 pos, int fIndex,int ithV, BOOL sel)=0;
		virtual void	fnAddHandle(Point3 pos, int fIndex,int ithV, BOOL sel)=0;
		virtual void	fnAddInterior(Point3 pos, int fIndex,int ithV, BOOL sel)=0;

		virtual void	fnSetFaceVertexIndex(int fIndex,int ithV, int vIndex)=0;
		virtual void	fnSetFaceHandleIndex(int fIndex,int ithV, int vIndex)=0;
		virtual void	fnSetFaceInteriorIndex(int fIndex,int ithV, int vIndex)=0;

		virtual void	fnUpdateViews()=0;

		virtual void	fnGetFaceSelFromStack()=0;



	};


#endif // __IUWNRAP__H
