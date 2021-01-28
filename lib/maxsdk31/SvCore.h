#pragma once

typedef enum
	{
	SVT_PROCEED,
	SVT_EXISTS,
	SVT_DO_NOT_PROCEED,
	} SvTraverseStatus;

typedef enum
	{
	REFTYPE_CHILD,
	REFTYPE_SUBANIM,
	REFTYPE_PLUGIN,
	} SvReferenceType;

// Filter bits...
static const DWORD SV_FILTER_SELOBJECTS     = (1<<0);
static const DWORD SV_FILTER_OBJECTMODS     = (1<<1);
static const DWORD SV_FILTER_BASEPARAMS     = (1<<2);
static const DWORD SV_FILTER_MATPARAMS      = (1<<3);
static const DWORD SV_FILTER_GEOM           = (1<<4);
static const DWORD SV_FILTER_SHAPES         = (1<<5);
static const DWORD SV_FILTER_LIGHTS         = (1<<6);
static const DWORD SV_FILTER_CAMERAS        = (1<<7);
static const DWORD SV_FILTER_HELPERS        = (1<<8);
static const DWORD SV_FILTER_WARPS          = (1<<9);
static const DWORD SV_FILTER_VISIBLE_OBJS   = (1<<10);
static const DWORD SV_FILTER_CONTROLLERS    = (1<<11);
static const DWORD SV_FILTER_ANIMATEDONLY   = (1<<12);
static const DWORD SV_FILTER_MAPS           = (1<<13);
static const DWORD SV_FILTER_BONES          = (1<<14);

// Schematic view UI colors...
static const int SV_UICLR_WINBK               = 0;
static const int SV_UICLR_NODEBK              = 1;
static const int SV_UICLR_SELNODEBK           = 2;
static const int SV_UICLR_NODE_HIGHLIGHT      = 3;
static const int SV_UICLR_MATERIAL_HIGHLIGHT  = 4;
static const int SV_UICLR_MODIFIER_HIGHLIGHT  = 5;
static const int SV_UICLR_PLUGIN_HIGHLIGHT    = 6;
static const int SV_UICLR_SUBANIM_LINE        = 7;
static const int SV_UICLR_CHILD_LINE          = 8;
static const int SV_UICLR_FRAME               = 9;
static const int SV_UICLR_SELTEXT             = 10;
static const int SV_UICLR_TEXT                = 11;
static const int SV_UICLR_FOCUS               = 12;
static const int SV_UICLR_MARQUIS             = 13;
static const int SV_UICLR_COLLAPSEARROW       = 14;
static const int SV_UICLR_GEOMOBJECT_BK       = 15;
static const int SV_UICLR_LIGHT_BK            = 16;
static const int SV_UICLR_CAMERA_BK           = 17;
static const int SV_UICLR_SHAPE_BK            = 18;
static const int SV_UICLR_HELPER_BK           = 19;
static const int SV_UICLR_SYSTEM_BK           = 20;
static const int SV_UICLR_CONTROLLER_BK       = 21;
static const int SV_UICLR_MODIFIER_BK         = 22;
static const int SV_UICLR_MATERIAL_BK         = 23;
static const int SV_UICLR_MAP_BK              = 24;

static const int SV_UICLR_CACHE_SIZE          = 25;

// Magic value returned from Animatable::SvGetSwatchColor(...)
// to indicate that no swatch is to be drawn...
static const int SV_NO_SWATCH							= 0xFFFFFFFF;

//-------------------------------------------------------------------------
// Bit flags which can be passed to IGraphObjectManager::AddAnimatable(...)
// and Animatable::SvTraverseAnimGraph(....)
//-------------------------------------------------------------------------

// If set, newly created node will be in the hidden state.  If the node
// already exists in the graph, the flag is ignored...
static const DWORD SV_INITIALLY_HIDDEN          = 0x00000001;

// If set, shared instances of an animatable will produce multiple
// graph nodes in the schematic view instead of a single shared graph node...
static const DWORD SV_DUPLICATE_INSTANCES       = 0x00000002;

// If set, the newly created children of the newly created node
// will be in the hidden state.  If the node already exists in the graph,
// the flag is ignored.  Children of this node that already exist in
// the graph will not have their visibility state changed...
static const DWORD SV_INITIALLY_CLOSED          = 0x00000004;

//-------------------------------------------------------------------------
// Flags which can be passed to IGraphObjectManager::PushLevel(...)
//-------------------------------------------------------------------------

// This id, when passed to PushLevel(), indicates that no id is to be associated
// with Animatable being pushed onto the stack...
static const int SV_NO_ID                       = 0x80000000;

class IGraphNode;
class Animatable;
class IGraphObjectManager;

class MultiSelectCallback
	{
	public:
	virtual int Priority() = 0;		// Used for sorting select order.
	virtual void Begin(IGraphObjectManager *gom, bool clear) = 0;
	virtual void Select(IGraphObjectManager *gom, IGraphNode *gNode, bool isSelected) = 0;
	virtual void End(IGraphObjectManager *gom) = 0;
	};

class IGraphRef
	{
	public:
	};

class IGraphNode
	{
	public:

		// Returns the Animatable associated with this node...
	virtual Animatable *GetAnim() = 0;

		// Returns the "primary parent" of this node.  Nodes
		// can have multiple parents (objects referencing
		// this node) so this function is not strictly
		// accurate.  That said, many nodes have the
		// concept of an owner node, which is what this
		// function returns.
	virtual IGraphNode *GetParentNode() = 0;

		// Returns the "owner" of this node.  Some nodes
		// have multiple owners.  When this is the case, this
		// function returns the "first" owner (the object
		// that first added this node to the schematic view)...
	virtual Animatable *GetOwner() = 0;

		// Return the "id" of this node.  When nodes are
		// added to the schematic view (via the
		// IGraphObjectManager::AddAnimatable(...) method),
		// an integer is provided.  This value is is not
		// used internally by the schematic view.  Rather,
		// it is available to implementers of the 
		// Animatable::Sv*() methods to aid in identifying
		// the node.
	virtual int GetID() = 0;
	};

class SvGraphNodeReference
	{
	public:

	IGraphNode *gNode;
	SvTraverseStatus stat;

	SvGraphNodeReference()
		{
		gNode= NULL;
		stat = SVT_DO_NOT_PROCEED;
		}
	};

class IGraphObjectManager
	{
	public:

		// During traversal of the Animatable graph via SvTraverseAnimGraph(...),
		// PushLevel() and PopLevel() should be called appropriately to
		// maintain an ownership stack.  This is required by the schematic view
		// when nodes are added to the graph with the "SV_DUPLICATE_INSTANCES"
		// flag set...
	virtual void PushLevel(Animatable *anim, int id = SV_NO_ID) = 0;
	virtual void PopLevel() = 0;

		// Adds an Animatable to the schematic view...
	virtual SvGraphNodeReference AddAnimatable(Animatable *anim, Animatable *owner, int id, DWORD flags = 0) = 0;

		// Add a reference from "maker" node to "target"...
	virtual IGraphRef *AddReference(IGraphNode *maker, IGraphNode *target, SvReferenceType type) = 0;

		// Pops up the property editor dialog on the
		// selected nodes in the schematic view...
	virtual void SvEditSelectedNodeProperties() = 0;

		// Selects the given node in the material editor.
		// Does nothing if "gNode" does not represent a
		// material or map...
	virtual void SvSelectInMaterialEditor(IGraphNode *gNode) = 0;

		// Selects the given node in the modifier panel.
		// Does nothing if "gNode" does not represent an
		// object...
	virtual void SvSetCurEditObject(IGraphNode *gNode) = 0;

		// Returns true if the given node is current
		// in the modifier panel...
	virtual bool SvIsCurEditObject(IGraphNode *gNode) = 0;

	virtual bool ApplyModifier(IGraphNode *gModNode, IGraphNode *gParentNode) = 0;
	virtual bool DeleteModifier(IGraphNode *gNode) = 0;

		// Invalidates the schematic view window...
	virtual void SvInvalidateView() = 0;

		// Invalidates a node in the schematic view window...
	virtual void SvInvalidateNode(IGraphNode *gNode) = 0;

		// Forces the material editor to update...
	virtual void SvUpdateMaterialEditor() = 0;

		// Forces the modifier panel to update...
	virtual void SvUpdateModifierPanel() = 0;

		// Set, Clear and Test filter flags...
	virtual void SetFilter(DWORD mask) = 0;
	virtual void ClearFilter(DWORD mask) = 0;
	virtual bool TestFilter(DWORD mask) = 0;

		// Get a SV UI color given a color index...
	virtual COLORREF SvGetUIColor(int colorIndex) = 0;
	};
