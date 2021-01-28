//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "max2dtsExporter/maxUtil.h"

#pragma pack(push,8)
#include <decomp.h>
#include <dummy.h>
#include <ISkin.h>
#include <modstack.h>
#pragma pack(pop)

#include "Core/tVector.h"

// Special for biped stuff -- class id in no header, so we just def it here
// NOTE/WARNING:  This could change with Character Studio updates
#define BipedObjectClassID Class_ID(37157,0)

Point3F & Point3ToPoint3F(Point3 & p3, Point3F & p3f)
{
    p3f.x = p3.x;
    p3f.y = p3.y;
    p3f.z = p3.z;
    return p3f;
}

void convertToTransform(Matrix3 & mat, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale)
{
    AffineParts parts;
    decomp_affine(mat,&parts);
    Point3ToPoint3F(parts.t,trans);
    rot.set(QuatF(parts.q[0],parts.q[1],parts.q[2],parts.q[3]));
    srot.set(QuatF(parts.u[0],parts.u[1],parts.u[2],parts.u[3]));
    Point3ToPoint3F(parts.k,scale);
}

MatrixF & convertToMatrixF(Matrix3 & mat3,MatrixF &matf)
{
    matf.identity();
    Point3F x,y,z,p;
    Point3 x3 = mat3 * Point3(1.0f,0.0f,0.0f);
    Point3 y3 = mat3 * Point3(0.0f,1.0f,0.0f);
    Point3 z3 = mat3 * Point3(0.0f,0.0f,1.0f);
    Point3 p3 = mat3 * Point3(0.0f,0.0f,0.0f);
    x = Point3ToPoint3F(x3,x);
    y = Point3ToPoint3F(y3,y);
    z = Point3ToPoint3F(z3,z);
    p = Point3ToPoint3F(p3,p);
    x -= p;
    y -= p;
    z -= p;
    matf.setColumn(0,x);
    matf.setColumn(1,y);
    matf.setColumn(2,z);
    matf.setColumn(3,p);
    return matf;
}

void getNodeTransform(INode *pNode, S32 time, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale)
{
    Matrix3 nodeMat = pNode->GetNodeTM( time );
    convertToTransform(nodeMat,rot,trans,srot,scale);
}

TriObject * getTriObject( INode *pNode, S32 time, S32 multiResVerts, bool & deleteIt)
{
   TriObject * tri = NULL;
   IParamBlock * paramBlock = NULL;

   // if we're a multiRes object, dial us down
   if (multiResVerts>0)
   {
      Animatable * obj = (Animatable*)pNode->GetObjectRef();

      for (S32 i=0; i<obj->NumSubs(); i++)
      {
         if (!dStrcmp(obj->SubAnimName(i),"MultiRes"))
         {
            paramBlock = (IParamBlock*)obj->SubAnim(i)->SubAnim(0);

            Interval range = pNode->GetTimeRange(TIMERANGE_ALL|TIMERANGE_CHILDNODES|TIMERANGE_CHILDANIMS);
            paramBlock->SetValue(0,time,multiResVerts);
            break;
         }
      }
   }

   // if the object can't convert to a tri-mesh, eval world state to
   // get an object that can:
   const ObjectState &os = pNode->EvalWorldState(time);

   if ( os.obj->CanConvertToType(triObjectClassID) )
      tri = (TriObject *)os.obj->ConvertToType( time, triObjectClassID );

   deleteIt = (tri && (tri != os.obj));

   return tri;
}

F32 findVolume(INode * pNode, S32 & polyCount)
{
   bool deleteTri;
   TriObject * tri = getTriObject(pNode,DEFAULT_TIME,-1,deleteTri);
   if (!tri)
   {
      polyCount=0;
      return 0.0f;
   }

   Mesh & mesh = tri->mesh;
   polyCount = mesh.getNumFaces();
   Box3F bounds;
   bounds.min.set( 10E30f, 10E30f, 10E30f);
   bounds.max.set(-10E30f,-10E30f,-10E30f);
   for (S32 i=0; i<mesh.numVerts; i++)
   {
      Point3F v = Point3ToPoint3F(mesh.verts[i],v);
      bounds.min.setMin(v);
      bounds.max.setMax(v);
   }
   if (deleteTri)
      delete tri;
   return (bounds.max.x-bounds.min.x)*(bounds.max.y-bounds.min.y)*(bounds.max.z-bounds.min.z);
}

void findSkinData(INode * pNode, ISkin **skin, ISkinContextData ** skinData)
{
   // till proven otherwise...
   *skin = NULL;
   *skinData = NULL;

   // Get object from node. Abort if no object.
   Object* obj = pNode->GetObjectRef();
   if (!obj)
      return;

   Modifier * mod = NULL;

   // Is derived object ?
   S32 i;
   while (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
   {
      IDerivedObject* dobj = (IDerivedObject*) obj;
      // Iterate over all entries of the modifier stack.
      for (i=0;i<dobj->NumModifiers();i++)
         if (dobj->GetModifier(i)->ClassID() == SKIN_CLASSID)
            break;

      if (i!=dobj->NumModifiers())
      {
         mod = dobj->GetModifier(i);
         break;
      }
      obj = dobj->GetObjRef();
   }
   if (!mod)
      return;
   *skin = (ISkin*) mod->GetInterface(I_SKIN);
   if (!*skin)
      return;
   *skinData = (*skin)->GetContextInterface(pNode);
   if (!*skinData)
      *skin=NULL; // return both or neither
}

bool hasSkin(INode * pNode)
{
   ISkin * skin;
   ISkinContextData * skinData;
   findSkinData(pNode,&skin,&skinData);
   return skin != NULL;
}

bool hasMesh(INode * pNode)
{
    ObjectState os = pNode->EvalWorldState(0);
    return( os.obj->CanConvertToType(triObjectClassID) && !(os.obj->ClassID() == BipedObjectClassID) );
}

void zapScale(Matrix3 & mat)
{
    AffineParts parts;
    decomp_affine(mat,&parts);

    // now put the matrix back together again without the scale:
    // mat = mat.rot * mat.pos
    mat.IdentityMatrix();
    mat.PreTranslate(parts.t);
    PreRotateMatrix(mat,parts.q);
}

Matrix3 & getLocalNodeMatrix(INode *pNode, INode *parent, S32 time, Matrix3 & matrix, AffineParts & a10, AffineParts & a20)
{
    // Here's the story:  the default transforms have no scale.  In order to account for scale, the
    // scale at the default time is folded into the object offset (which is multiplied into the points
    // before exporting).  Because of this, the local transform at a given time must take into account
    // the scale of the parent and child node at time 0 in addition to the current time.  In particular,
    // the world transform at a given time is WT(time) = inverse(Tscale(0)) * T(time)
    // Note: above formula is max style.  Torque style would have the order reveresed. :(

    // in order to avoid recomputing matrix at default time over and over, we assume that the first request
    // for the matrix will be at the default time, and thereafter, we will pass that matrix in and reuse it...
    Matrix3 m1 = pNode->GetNodeTM(time);
    Matrix3 m2 = parent ? parent->GetNodeTM(time) : Matrix3(true);
    if (time==DEFAULT_TIME)
    {
      decomp_affine(m1,&a10);
      decomp_affine(m2,&a20);
    }

    // build the inverse scale matrices
    Matrix3 stretchRot10,stretchRot20;
    Point3 sfactor10, sfactor20;
    Matrix3 invScale10, invScale20;
    a10.u.MakeMatrix(stretchRot10);
    a20.u.MakeMatrix(stretchRot20);
    sfactor10 = Point3(a10.f/a10.k.x,a10.f/a10.k.y,a10.f/a10.k.z);
    sfactor20 = Point3(a20.f/a20.k.x,a20.f/a20.k.y,a20.f/a20.k.z);
    invScale10 = Inverse(stretchRot10) * ScaleMatrix(sfactor10) * stretchRot10;
    invScale20 = Inverse(stretchRot20) * ScaleMatrix(sfactor20) * stretchRot20;

    // build world transforms
    m1 = invScale10 * m1;
    m2 = invScale20 * m2;

    // build local transform
    matrix = m1 * Inverse(m2);
    return matrix;
}

void getLocalNodeTransform(INode *pNode, INode *parent, AffineParts & child0, AffineParts & parent0, S32 time, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale)
{
    Matrix3 local;
    getLocalNodeMatrix(pNode,parent,time,local,parent0,child0);
    convertToTransform(local,rot,trans,srot,scale);
}

void getBlendNodeTransform(INode *pNode, INode *parent, AffineParts & child0, AffineParts & parent0, S32 time, S32 referenceTime, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale)
{
    Matrix3 m1;
    Matrix3 m2;

    getLocalNodeMatrix(pNode, parent, referenceTime, m1, child0, parent0);
    getLocalNodeMatrix(pNode, parent,          time, m2, child0, parent0);
    m1 = Inverse(m1);
    m2 *= m1;
    convertToTransform(m2, rot,trans,srot,scale);
}

void computeObjectOffset(INode * pNode, Matrix3 & mat)
{
   // compute the object offset transform...
   // this will be applied to all the verts of the meshes...
   // we grab this straight from the node, but we also
   // multiply in any scaling on the node transform because
   // the scaling will be stripped out when making a tsshape

   Matrix3 nodeMat = pNode->GetNodeTM(DEFAULT_TIME);
   zapScale(nodeMat);
   nodeMat = Inverse(nodeMat);

   mat = pNode->GetObjTMAfterWSM(DEFAULT_TIME);
   mat *= nodeMat;
}

void getDeltaTransform(INode * pNode, S32 time1, S32 time2, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale)
{
   Matrix3 m1 = pNode->GetNodeTM(time2);
   Matrix3 m2 = pNode->GetNodeTM(time1);
   m2 = Inverse(m2);
   m1 *= m2;
   convertToTransform(m1,rot,trans,srot,scale);
}

bool isVis(INode * pNode, S32 time)
{
   return pNode->GetVisibility(time) > 0.0000001f;
}

F32 getVisValue(INode * pNode, S32 time)
{
   return pNode->GetVisibility(time);
}

bool animatesVis(INode * pNode, const Interval & range, bool & error)
{
   // running error...exit if already encountered an error
   if (error)
      return false;

   F32 startVis = getVisValue(pNode,range.Start());
   for (S32 i=range.Start()+1; i<=range.End(); i++)
      if (mFabs(startVis-getVisValue(pNode,i))>0.01f)
         return true;
   return false;
}

bool animatesFrame(INode * pNode, const Interval & range, bool & error)
{
   // running error...exit if already encountered an error
   if (error)
      return false;

   bool deleteIt;
   TriObject * tri = getTriObject(pNode,range.Start(),-1,deleteIt);
   if (!tri)
   {
      error = true;
      return false;
   }

   Interval ivalid;
   ivalid = tri->ChannelValidity(range.Start(), GEOM_CHAN_NUM);

   if (deleteIt)
      tri->DeleteMe();

   return (ivalid.Start() > range.Start() || ivalid.End() < range.End());
}

bool animatesMatFrame(INode * pNode, const Interval & range, bool & error)
{
   // running error...exit if already encountered an error
   if (error)
      return false;


   bool deleteIt;
   TriObject * tri = getTriObject(pNode,range.Start(),-1,deleteIt);
   if (!tri)
   {
       error = true;
       return false;
   }

   Interval ivalid;
   ivalid = tri->ChannelValidity(range.Start(), TEXMAP_CHAN_NUM);

   if (deleteIt)
      tri->DeleteMe();

   return (ivalid.Start() > range.Start() || ivalid.End() < range.End());
}

bool isdigit(char ch)
{
   return (ch>='0' && ch<='9');
}

//----------------------------------------------------------------------------
// If 's' ends in a number, chopNum returns
// a pointer to first digit in this number.
// If not, then returns pointer to '\0'
// at end of name or first of any trailing
// spaces.
char * chopNum(char * s)
{
   if (s==NULL)
       return NULL;

   char * p = s + dStrlen(s);

   if (p==s)
       return s;
   p--;

   // trim spaces from the end
   while (p!=s && *p==' ')
      p--;

   // back up until we reach a non-digit
   // gotta be better way than this...
   if (isdigit(*p))
      do
      {
         if (p--==s)
            return p+1;
      } while (isdigit(*p));

   // allow negative numbers
   if (*p=='-')
      p--;

   // trim spaces separating name and number
   while (*p==' ')
   {
      p--;
      if (p==s)
         return p;
   }

   // return first space if there was one,
   // o.w. return first digit
   return p+1;
}

//----------------------------------------------------------------------------
// separates nodes names into base name
// and detail number
char * chopTrailingNumber(const char * fullName, S32 & size)
{
    if (!fullName)
    {
        size = -1;
        return NULL;
    }

    char * buffer = dStrdup(fullName);
    char * p = chopNum(buffer);
    if (*p=='\0')
    {
        size = -1;
        return buffer;
    }

    size = dAtoi(p);
    *p='\0'; // terminate string
    return buffer;
}

//----------------------------------------------------------------------------
// returns a list of detail levels of this shape...grabs meshes off shape
// and finds meshes on root level with same name but different trailing
// detail number (i.e., head32 on shape, head16 at root, leads to detail
// levels 32 and 16).
void findDetails(INode * base, INode * root, Vector<S32> & details)
{
   S32 i,j,k;

   // run through all the descendents of this node
   // and look for numbers indicating detail levels
   Vector<INode*> nodeStack;
   nodeStack.push_back(base);
   while (!nodeStack.empty())
   {
      INode * node = nodeStack.last();
      nodeStack.pop_back();
      if (hasMesh(node))
      {
         // have a mesh...a little obscure below:
         // add # of this mesh + any mesh w/ same
         // name on the root level to details vector
         S32 detailSize;
         char * baseName = chopTrailingNumber(node->GetName(),detailSize);
         INode * meshNode = node;
         for (j=-1;j<root->NumberOfChildren();j++)
         {
            if (j>=0)
               meshNode = root->GetChildNode(j);
            if (!hasMesh(meshNode))
               continue;
            char * meshName = chopTrailingNumber(meshNode->GetName(),detailSize);
            if (!dStricmp(baseName,meshName))
            {
               for (k=0; k<details.size(); k++)
                  if (detailSize==details[k])
                     break;
               if (k==details.size())
                  details.push_back(detailSize);
            }
            dFree(meshName);
         }
         dFree(baseName);
      }
      for (j=0; j<node->NumberOfChildren();j++)
         nodeStack.push_back(node->GetChildNode(j));
   }
}

//----------------------------------------------------------------------------
// find selected subtrees (nodes on root level only) and embed in subtree of
// the style the exporter looks for:
// Root
// |
// |-base
//      |-start
//      |     |
//      |     |-<selected node 1>
//      |     |         .
//      |     |         .
//      |     |-<selected node N>
//      |
//      |-detail 1
//      |    .
//      |    .
//      |-detail N
void embedSubtree(Interface * ip)
{
   S32 i,j,k;

   // make dummy node named baseName
   TSTR baseName("base");
   ip->MakeNameUnique(baseName);
   INode * base = ip->CreateObjectNode(new DummyObject);
   base->SetName(baseName);

   // make dummy node named startName
   TSTR startName("start");
   ip->MakeNameUnique(startName);
   INode * start = ip->CreateObjectNode(new DummyObject);
   start->SetName(startName);

   // link later to former
   base->AttachChild(start);

   Vector<S32> details;

   // loop through selection set, link to start (only if child of root)
   S32 count = ip->GetSelNodeCount();
   INode * root = ip->GetRootNode();
   for (i=0; i<count; i++)
   {
      INode * node = ip->GetSelNode(i);
      if (!node->GetParentNode()->IsRootNode())
         continue;
      start->AttachChild(node);

      findDetails(node,root,details);
   }

   // now create detail markers
   for (i=0; i<details.size();i++)
   {
      char detailName[20];
      INode * detailNode = ip->CreateObjectNode(new DummyObject);
      dSprintf(detailName,20,"detail%i",details[i]);
      detailNode->SetName(detailName);
      base->AttachChild(detailNode);
   }
}

//----------------------------------------------------------------------------
void registerDetails(Interface * ip)
{
   S32 i,j,k;

   // loop through selection set, only care about nodes off root
   S32 count = ip->GetSelNodeCount();
   INode * root = ip->GetRootNode();
   for (i=0; i<count; i++)
   {
      INode * node = ip->GetSelNode(i);
      if (!node->GetParentNode()->IsRootNode())
         continue;

      Vector<S32> details;

      // search branches of tree for meshes and find their details
      for (j=0; j<node->NumberOfChildren(); j++)
      {
         INode * child = node->GetChildNode(j);
         if (child->NumberOfChildren()>0)
            findDetails(child,root,details);
      }

      // go through children of base node and cull detail numbers
      // that don't need to be added (because detail marker already
      // present)
      for (j=0; j<node->NumberOfChildren(); j++)
      {
         INode * child = node->GetChildNode(j);
         if (child->NumberOfChildren()==0)
         {
            // look for #
            S32 detailSize;
            char * baseName = chopTrailingNumber(child->GetName(),detailSize);
            dFree(baseName);
            for (k=0;k<details.size();k++)
            {
               if (details[k]==detailSize)
               {
                  // found it
                  details.erase(k);
                  break;
               }
            }
         }
      }

      // items left in details list are unique -- add markers
      for (j=0;j<details.size();j++)
      {
         char detailName[20];
         INode * detailNode = ip->CreateObjectNode(new DummyObject);
         dSprintf(detailName,20,"detail%i",details[j]);
         detailNode->SetName(detailName);
         node->AttachChild(detailNode);
      }
   }
}

//----------------------------------------------------------------------------
void renumberNodes(Interface * ip, S32 newSize)
{
   S32 i, count = ip->GetSelNodeCount();
   char newName[128];
   for (i=0; i<count; i++)
   {
      INode * node = ip->GetSelNode(i);
      S32 oldSize;
      char * baseName = chopTrailingNumber(node->GetName(),oldSize);
      dSprintf(newName,128,"%s%i",baseName,newSize);
      node->SetName(newName);
      dFree(baseName);
   }
}

// this version of doDot used by m_pointInPoly
inline float doDot(F32 v1x, F32 v1y, F32 v2x, F32 v2y, F32 px, F32 py)
{
   return (v1x-px) * (v1y-v2y) +
          (v1y-py) * (v2x-v1x);
}

//------------------------------------------------------------------------------------
// pointInPoly returns true if point is inside poly defined by verts on plane w/
// normal "normal" -- based on m_pointInTriangle.
//------------------------------------------------------------------------------------
bool pointInPoly(const Point3F & point,
                 const Point3F & normal,
                 const Point3F * verts,
                 U32 n)
{
   F32 thisDot, lastDot=0;
   U32 i;

   // we can ignore one of the dimensions because all points are on the same plane...
   if (mFabs(normal.y)>mFabs(normal.x)&&mFabs(normal.y)>mFabs(normal.z))
   {
      // drop y coord
      thisDot = doDot(verts[n-1].x,verts[n-1].z,verts[0].x,verts[0].z,point.x,point.z);
      if (thisDot*lastDot<0)
         return false;
      lastDot = thisDot;

      for (i=0;i<n-1;i++)
      {
         thisDot = doDot(verts[i].x,verts[i].z,verts[i+1].x,verts[i+1].z,point.x,point.z);
         if (thisDot*lastDot<0)
            return false; // different sign, point outside one of the edges
         lastDot = thisDot;
      }
   }
   else if (mFabs(normal.x)>mFabs(normal.y)&&mFabs(normal.x)>mFabs(normal.z))
   {
      // drop x coord
      thisDot = doDot(verts[n-1].y,verts[n-1].z,verts[0].y,verts[0].z,point.y,point.z);
      if (thisDot*lastDot<0)
         return false;
      lastDot = thisDot;

      for (i=0;i<n-1;i++)
      {
         thisDot = doDot(verts[i].y,verts[i].z,verts[i+1].y,verts[i+1].z,point.y,point.z);
         if (thisDot*lastDot<0)
            return false; // different sign, point outside one of the edges
         lastDot = thisDot;
      }
   }
   else
   {
      // drop z coord
      thisDot = doDot(verts[n-1].x,verts[n-1].y,verts[0].x,verts[0].y,point.x,point.y);
      if (thisDot*lastDot<0)
         return false;
      lastDot = thisDot;

      for (i=0;i<n-1;i++)
      {
         thisDot = doDot(verts[i].x,verts[i].y,verts[i+1].x,verts[i+1].y,point.x,point.y);
         if (thisDot*lastDot<0)
            return false; // different sign, point outside one of the edges
         lastDot = thisDot;
      }
   }

   return true;
}









// deal with some unresolved externals...
#include "platform/event.h"
#include "core/bitStream.h"
void GamePostEvent(struct Event const &) {}
void GameHandleInfoPacket(struct NetAddress const *, BitStream *){}
void GameHandleDataPacket(S32, BitStream *){}
void GameConnectionAccepted(S32, BitStream *){}
void GameConnectionRejected(S32, BitStream *){}
void GameConnectionDisconnected(S32, BitStream *){}
void GameConnectionRequested(struct NetAddress const *, BitStream *){}
void GameConnectionEstablished(S32){}
void GameHandleNotify(S32,bool){}
void GameConnectionTimedOut(S32){}
void GameDeactivate(bool) {}
void GameReactivate() {}
S32 GameMain(S32,char const * *){ return 0; }
U32 getTickCount() { return 0; }
void (*terrMipBlit)(U16 *dest, U32 destStride, U32 squareSize, const U8 *sourcePtr, U32 sourceStep, U32 sourceRowAdd);
bool gEditingMission;

class SimGroup;
SimGroup * gDataBlockGroup;
SimGroup * gActionMapGroup;
SimGroup * gClientGroup;
SimGroup * gGuiGroup;
SimGroup * gGuiDataGroup;
SimGroup * gTCPGroup;
class SimSet;
SimSet * gActiveActionMapSet;
SimSet * gGhostAlwaysSet;
SimSet * gLightSet;

//------------------------------------------------------
// These routines aren't currently used, but could prove
// useful at a later time...
//------------------------------------------------------
/*

MatrixF & getNodeMatrix(INode *pNode, S32 time, MatrixF & matrix)
{
    Matrix3 nodeMat = pNode->GetNodeTM( time );
    convertToMatrixF(nodeMat,matrix);
    return matrix;
}

MatrixF & getLocalNodeMatrix(INode *pNode, INode *parent, S32 time, MatrixF & matrix)
{
    if (!parent)
        return getNodeMatrix(pNode,time,matrix);

    MatrixF m1,m2;
    getNodeMatrix(pNode,time,m1);
    getNodeMatrix(parent,time ,m2);
    m2.inverse();
    matrix.mul(m1,m2);
    return matrix;
}

*/
