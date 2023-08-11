//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COLLADA_UTILS_H_
#define _COLLADA_UTILS_H_

#ifdef _MSC_VER
#pragma warning(disable : 4786)  // disable warning about long debug symbol names
#pragma warning(disable : 4355)  // disable "'this' : used in base member initializer list" warnings
#endif

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _MQUAT_H_
#include "math/mQuat.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _TSSHAPE_LOADER_H_
#include "ts/loader/tsShapeLoader.h"
#endif
#ifndef _OPTIMIZEDPOLYLIST_H_
#include "collision/optimizedPolyList.h"
#endif
#ifndef TINYXML_INCLUDED
#include "tinyxml.h"
#endif

#include <string>
#include "core/path.h"
#include "dae.h"
#include "dae/daeErrorHandler.h"
#include "dae/domAny.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domMaterial.h"
#include "dom/domGeometry.h"
#include "dom/domMorph.h"
#include "dom/domNode.h"
#include "dom/domCOLLADA.h"
typedef std::string String;


namespace ColladaUtils
{
   MatrixF convertTransform(const MatrixF& m);

   // void collapsePath(const Torque::Path& path);

   // Apply the set of Collada conditioners (suited for loading Collada models into Torque)
   void applyConditioners(domCOLLADA* root);

   const domProfile_COMMON* findEffectCommonProfile(const domEffect* effect);
   const domCommon_color_or_texture_type_complexType* findEffectDiffuse(const domEffect* effect);
   const domFx_sampler2D_common_complexType* getSampler2D(const domEffect* effect);

   // Collada export helper functions, not implemented yet for OpenMBU
   // const char* findTexture(const Torque::Path& diffuseMap); 
   void exportColladaHeader(TiXmlElement* rootNode);
   void exportColladaMaterials(TiXmlElement* rootNode, const OptimizedPolyList& mesh, Vector<String>& matNames, const Torque::Path& colladaFile);
   void exportColladaTriangles(TiXmlElement* meshNode, const OptimizedPolyList& mesh, const String& meshName, const Vector<String>& matNames);
   void exportColladaMesh(TiXmlElement* rootNode, const OptimizedPolyList& mesh, const String& meshName, const Vector<String>& matNames);
   void exportColladaScene(TiXmlElement* rootNode, const String& meshName, const Vector<String>& matNames);

   // Export an OptimizedPolyList to a simple Collada file
   void exportToCollada(const Torque::Path& colladaFile, const OptimizedPolyList& mesh, const String& meshName = "");
};

//-----------------------------------------------------------------------------
// Helper Classes
//
// The Collada DOM uses a different class for each XML element, and there is very
// little class inheritance, even though many elements have the same attributes
// and children. This makes the DOM a bit ugly to work with, and the following
// templates attempt to make this situation a bit nicer by providing a common way
// to access common elements, while retaining the strong typing of the DOM classes.
//-----------------------------------------------------------------------------

/// Convert from the Collada transform types to a Torque MatrixF
template<class T> inline MatrixF vecToMatrixF(const domListOfFloats& vec) { return MatrixF(true); }

/// Collada <translate>: [x_translate, y_translate, z_translate]
template<> inline MatrixF vecToMatrixF<domTranslate>(const domListOfFloats& vec)
{
   MatrixF mat(true);
   mat.setPosition(Point3F(vec[0], vec[1], vec[2]));
   return mat;
}

/// Collada <scale>: [x_scale, y_scale, z_scale]
template<> inline MatrixF vecToMatrixF<domScale>(const domListOfFloats& vec)
{
   MatrixF mat(true);
   mat.scale(Point3F(vec[0], vec[1], vec[2]));
   return mat;
}

/// Collada <rotate>: [rotation_axis, angle_in_degrees]
template<> inline MatrixF vecToMatrixF<domRotate>(const domListOfFloats& vec)
{
   AngAxisF aaxis(Point3F(vec[0], vec[1], vec[2]), -(vec[3] * M_PI) / 180.0f);
   MatrixF mat(true);
   aaxis.setMatrix(&mat);
   return mat;
}

/// Collada <matrix>: same form as TGE (woohoo!)
template<> inline MatrixF vecToMatrixF<domMatrix>(const domListOfFloats& vec)
{
   MatrixF mat;
   for (int i = 0; i < 16; i++)
      mat[i] = vec[i];
   return mat;
}

/// Collada <skew>: [angle_in_degrees, rotation_axis, translation_axis]
/// skew transform code adapted from GMANMatrix4 implementation
template<> inline MatrixF vecToMatrixF<domSkew>(const domListOfFloats& vec)
{
   F32 angle = -(vec[0] * M_PI) / 180.0f;
   Point3F rotAxis(vec[1], vec[2], vec[3]);
   Point3F transAxis(vec[4], vec[5], vec[6]);

   transAxis.normalize();

   Point3F a1 = transAxis * mDot(rotAxis, transAxis);
   Point3F a2 = rotAxis - a1;
   a2.normalize();

   F32 an1 = mDot(rotAxis, a2);
   F32 an2 = mDot(rotAxis, transAxis);

   F32 rx = an1 * mCos(angle) - an2 * mSin(angle);
   F32 ry = an1 * mSin(angle) + an2 * mCos(angle);

   // Check for rotation parallel to translation
   F32 alpha = (an1 == 0) ? 0 : (ry/rx - an2/an1);

   MatrixF mat(true);
   mat(0,0) = a2.x * transAxis.x * alpha + 1.0;
   mat(1,0) = a2.y * transAxis.x * alpha;
   mat(2,0) = a2.z * transAxis.x * alpha;

   mat(0,1) = a2.x * transAxis.y * alpha;
   mat(1,1) = a2.y * transAxis.y * alpha + 1.0;
   mat(2,1) = a2.z * transAxis.y * alpha;

   mat(0,2) = a2.x * transAxis.z * alpha;
   mat(1,2) = a2.y * transAxis.z * alpha;
   mat(2,2) = a2.z * transAxis.z * alpha + 1.0;
   return mat;
}

/// Collada <lookat>: [eye, target, up]
template<> inline MatrixF vecToMatrixF<domLookat>(const domListOfFloats& vec)
{
   Point3F eye(vec[1], vec[1], vec[2]);
   Point3F target(vec[3], vec[4], vec[5]);
   Point3F up(vec[6], vec[7], vec[8]);

   Point3F fwd = target - eye;
   fwd.normalizeSafe();

   Point3F right = mCross(fwd, up);
   right.normalizeSafe();

   up = mCross(right, fwd);
   up.normalizeSafe();

   MatrixF mat(true);
   mat.setColumn(0, right);
   mat.setColumn(1, fwd);
   mat.setColumn(2, up);
   mat.setColumn(3, eye);
   mat.inverse();
   return mat;
}

//-----------------------------------------------------------------------------

/// Try to get a name for the element using the following attributes (in order):
/// name, sid, id, "null"
template<class T> inline const char* _GetNameOrId(const T* element)
{
   return element->getName() ? element->getName() : (element->getId() ? element->getId() : "null");
}

template<> inline const char* _GetNameOrId(const domInstance_geometry* element)
{
   return element->getName() ? element->getName() : (element->getSid() ? element->getSid() : "null");
}

template<> inline const char* _GetNameOrId(const domInstance_controller* element)
{
   return element->getName() ? element->getName() : (element->getSid() ? element->getSid() : "null");
}

//-----------------------------------------------------------------------------
// Collada <source>s are extremely flexible, and thus difficult to access in a nice
// way. This class attempts to provide a clean interface to convert Collada source
// data to the appropriate Torque data structure without losing any of the flexibility
// of the underlying Collada DOM.
//
// Some of the conversions we need to handle are:
// - daeString to const char*
// - daeIDRef to const char*
// - double to F32
// - double to Point2F
// - double to Point3F
// - double to MatrixF
//
// The _SourceReader object is initialized with a list of parameter names that it
// tries to match to <param> elements in the source accessor to figure out how to
// pull values out of the 1D source array. Note that no type checking of any kind
// is done until we actually try to extract values from the source.
class _SourceReader
{
   const domSource* source;      // the wrapped Collada source
   const domAccessor* accessor;  // shortcut to the source accessor
   Vector<U32> offsets;          // offset of each of the desired values to pull from the source array

public:
   _SourceReader() : source(0), accessor(0) {}

   void reset()
   {
      source = 0;
      accessor = 0;
      offsets.clear();
   }

   //------------------------------------------------------
   // Initialize the _SourceReader object
   bool initFromSource(const domSource* src, const char* paramNames[] = 0)
   {
      source = src;
      accessor = source->getTechnique_common()->getAccessor();
      offsets.clear();

      // The source array has groups of values in a 1D stream => need to map the
      // input param names to source params to determine the offset within the
      // group for each desired value
      U32 paramCount = 0;
      while (paramNames && paramNames[paramCount][0]) {
         // lookup the index of the source param that matches the input param
         offsets.push_back(0);
         for (U32 iParam = 0; iParam < accessor->getParam_array().getCount(); iParam++) {
            if (accessor->getParam_array()[iParam]->getName() &&
               dStrcmp(accessor->getParam_array()[iParam]->getName(), paramNames[paramCount]) == 0) {
               offsets.last() = iParam;
               break;
            }
         }
         paramCount++;
      }

      // If no input params were specified, just map the source params directly
      if (!offsets.size()) {
         for (int iParam = 0; iParam < accessor->getParam_array().getCount(); iParam++)
            offsets.push_back(iParam);
      }

      return true;
   }

   //------------------------------------------------------
   // Shortcut to the size of the array (should be the number of destination objects)
   S32 size() const { return accessor ? accessor->getCount() : 0; }

   // Get the number of elements per group in the source
   S32 stride() const { return accessor ? accessor->getStride() : 0; }

   //------------------------------------------------------
   // Get a pointer to the start of a group of values (index advances by stride)
   //template<class T> T getArrayData(int index) const { return 0; }

   const double* getStringArrayData(int index) const
   {
      if ((index >= 0) && (index < size())) {
         if (source->getFloat_array())
            return &source->getFloat_array()->getValue()[index*stride()];
      }
      return 0;
   }

   //------------------------------------------------------
   // Read a single value from the source array
   //template<class T> T getValue(int index) const { return T; }

   const char* getStringValue(int index) const
   {
      if ((index >= 0) && (index < size())) {
         // could be plain strings or IDREFs
         if (source->getName_array())
            return source->getName_array()->getValue()[index*stride()];
         else if (source->getIDREF_array())
            return source->getIDREF_array()->getValue()[index*stride()].getID();
      }
      return "";
   }

   F32 getFloatValue(int index) const
   {
      F32 value(0);
      if (const double* data = getStringArrayData(index))
         return data[offsets[0]];
      return value;
   }

   Point2F getPoint2FValue(int index) const
   {
      Point2F value(0, 0);
      if (const double* data = getStringArrayData(index))
         value.set(data[offsets[0]], data[offsets[1]]);
      return value;
   }

   Point3F getPoint3FValue(int index) const
   {
      Point3F value(1, 0, 0);
      if (const double* data = getStringArrayData(index))
         value.set(data[offsets[0]], data[offsets[1]], data[offsets[2]]);
      return value;
   }

   ColorI getColorIValue(int index) const
   {
      ColorI value(255, 255, 255, 255);
      if (const double* data = getStringArrayData(index))
      {
         value.red = data[offsets[0]] * 255.0;
         value.green = data[offsets[1]] * 255.0;
         value.blue = data[offsets[2]] * 255.0;
         if ( stride() == 4 )
            value.alpha = data[offsets[3]] * 255.0;
      }
      return value;
   }

   MatrixF getMatrixFValue(int index) const
   {
      MatrixF value(true);
      if (const double* data = getStringArrayData(index)) {
         for (int i = 0; i < 16; i++)
            value[i] = data[i];
      }
      return value;
   }
};

//-----------------------------------------------------------------------------
// Collada geometric primitives: Use the BasePrimitive class to access the
// different primitive types in a nice way.
class BasePrimitive
{
public:
   /// Return true if the element is a geometric primitive type
   static bool isPrimitive(const daeElement* element)
   {
      switch (element->getElementType()) {
         case COLLADA_TYPE::TRIANGLES:          case COLLADA_TYPE::POLYLIST:
         case COLLADA_TYPE::POLYGONS:           case COLLADA_TYPE::TRIFANS:
         case COLLADA_TYPE::TRISTRIPS:          case COLLADA_TYPE::CAPSULE:
         case COLLADA_TYPE::CYLINDER:           case COLLADA_TYPE::LINES:
         case COLLADA_TYPE::LINESTRIPS:         case COLLADA_TYPE::PLANE:
         case COLLADA_TYPE::SPLINE:             case COLLADA_TYPE::SPHERE:
         case COLLADA_TYPE::TAPERED_CAPSULE:    case COLLADA_TYPE::TAPERED_CYLINDER:
            return true;
      }
      return false;
   }

   /// Return true if the element is a supported primitive type
   static bool isSupportedPrimitive(const daeElement* element)
   {
      switch (element->getElementType()) {
         case COLLADA_TYPE::TRIANGLES:
         case COLLADA_TYPE::POLYLIST:
         case COLLADA_TYPE::POLYGONS:
            return true;
      }
      return false;
   }

   /// Construct a child class based on the type of Collada element
   static BasePrimitive* get(const daeElement* element);

   /// Methods to be implemented for each supported Collada geometric element
   virtual const char* getElementName() = 0;
   virtual const char* getMaterial() = 0;
   virtual const domInputLocalOffset_Array& getInputs() = 0;

   virtual S32 getStride() const = 0;
   virtual const domListOfUInts *getTriangleData() = 0;
};

/// Template child class for supported Collada primitive elements
template<class T> class ColladaPrimitive : public BasePrimitive
{
   T* primitive;
   domListOfUInts *pTriangleData;
   S32 stride;
public:
   ColladaPrimitive(const daeElement* e) : pTriangleData(0)
   {
      // Cast to geometric primitive element
      primitive = daeSafeCast<T>(const_cast<daeElement*>(e));

      // Determine stride
      stride = 0;
      for (int iInput = 0; iInput < getInputs().getCount(); iInput++) {
         if (getInputs()[iInput]->getOffset() >= stride)
            stride = getInputs()[iInput]->getOffset() + 1;
      }
   }
   ~ColladaPrimitive()
   {
      delete pTriangleData;
   }

   /// Most primitives can use these common implementations
   const char* getElementName() { return primitive->getElementName(); }
   const char* getMaterial() { return primitive->getMaterial(); }
   const domInputLocalOffset_Array& getInputs() { return primitive->getInput_array(); }
   S32 getStride() const { return stride; }

   /// Each supported primitive needs to implement this method (and convert
   /// to triangles if required)
   const domListOfUInts *getTriangleData() { return NULL; }
};

//-----------------------------------------------------------------------------
// <triangles>
template<> inline const domListOfUInts *ColladaPrimitive<domTriangles>::getTriangleData()
{
   // Return the <p> integer list directly
   return &(primitive->getP()->getValue());
}

//-----------------------------------------------------------------------------
// <polygons>
template<> inline const domListOfUInts *ColladaPrimitive<domPolygons>::getTriangleData()
{
   if (!pTriangleData)
   {
      // Convert polygons to triangles
      pTriangleData = new domListOfUInts();

      for (int iPoly = 0; iPoly < primitive->getCount(); iPoly++) {

         domP* P = primitive->getP_array()[iPoly];
         domUint* pSrcData = &(P->getValue()[0]);
         S32 numPoints = P->getValue().getCount() / stride;

         // Use a simple tri-fan (centered at the first point) method of
         // converting the polygon to triangles.
         domUint* v0 = pSrcData;
         pSrcData += stride;
         for (int iTri = 0; iTri < numPoints-2; iTri++) {
            pTriangleData->appendArray(stride, v0);
            pTriangleData->appendArray(stride*2, pSrcData);
            pSrcData += stride;
         }
      }
   }
   return pTriangleData;
}

//-----------------------------------------------------------------------------
// <polylist>
template<> inline const domListOfUInts *ColladaPrimitive<domPolylist>::getTriangleData()
{
   if (!pTriangleData)
   {
      // Convert polygons to triangles
      pTriangleData = new domListOfUInts();

      domUint* pSrcData = &(primitive->getP()->getValue()[0]);
      const domListOfUInts& vcount = primitive->getVcount()->getValue();

      // Check that the P element has the right number of values (this
      // has been seen with certain models exported using COLLADAMax)
      U32 expectedCount = 0;
      for (int iPoly = 0; iPoly < vcount.getCount(); iPoly++)
         expectedCount += vcount[iPoly];
      expectedCount *= stride;

      if (primitive->getP()->getValue().getCount() != expectedCount)
      {
         Con::warnf("<polylist> element found with invalid <p> array. This primitive will be ignored.");
         return pTriangleData;
      }

      for (int iPoly = 0; iPoly < vcount.getCount(); iPoly++) {

         // Use a simple tri-fan (centered at the first point) method of
         // converting the polygon to triangles.
         domUint* v0 = pSrcData;
         pSrcData += stride;
         for (int iTri = 0; iTri < vcount[iPoly]-2; iTri++) {
            pTriangleData->appendArray(stride, v0);
            pTriangleData->appendArray(stride*2, pSrcData);
            pSrcData += stride;
         }
         pSrcData += stride;
      }
   }
   return pTriangleData;
}

//-----------------------------------------------------------------------------

/// Convert a custom parameter string to a particular type
template<typename T> inline T convert(const char* value) { return value; }
template<> inline bool convert(const char* value) { return dAtob(value); }
template<> inline S32 convert(const char* value) { return dAtoi(value); }
template<> inline double convert(const char* value) { return dAtof(value); }
template<> inline F32 convert(const char* value) { return convert<double>(value); }

//-----------------------------------------------------------------------------
/// Collada animation data
typedef Vector<struct AnimData*> AnimChannels;
struct AnimData
{
   _SourceReader  input;
   _SourceReader  output;

   _SourceReader  inTangent;
   _SourceReader  outTangent;

   _SourceReader  interpolation;

   U32 targetValueOffset;        ///< Offset into the target element (for arrays of values)
   U32 targetValueCount;         ///< Number of values animated (from OUTPUT source array)

   /// Get the animation channels for the Collada element (if any)
   static AnimChannels* getAnimChannels(const daeElement* element)
   {
      return element ? (AnimChannels*)const_cast<daeElement*>(element)->getUserData() : 0;
   }

   void parseTargetString(const char* target, int fullCount, const char* elements[]);

   F32 invertParamCubic(F32 param, F32 x0, F32 x1, F32 x2, F32 x3) const;
   void interpValue(F32 t, U32 offset, double* value) const;
   void interpValue(F32 t, U32 offset, const char** value) const;
};

//-----------------------------------------------------------------------------
// Collada allows any element with an SID or ID attribute to be the target of
// an animation channel, which is very flexible, but awkward to work with. Some
// examples of animated values are:
// - single float
// - single int
// - single bool
// - single string
// - list of floats (transform elements or morph weights)
//
// This class provides a generic way to check if an element is animated, and
// to get the value of the element at a given time.
template<class T>
struct AnimatedElement
{
   const daeElement* element;    ///< The Collada element (can be NULL)
   T defaultVal;                 ///< Default value (used when element is NULL)

   AnimatedElement(const daeElement* e=0) : element(e) { }

   /// Check if the element has any animations channels
   bool isAnimated()
   {
      return (AnimData::getAnimChannels(element) != 0);
   }

   /// Check if the element has any animation channels that overlap the interval
   bool isAnimated(F32 start, F32 end)
   {
      const AnimChannels* channels = AnimData::getAnimChannels(element);
      if (channels) {
         for (int iChannel = 0; iChannel < channels->size(); iChannel++) {
            F32 curveStart = (*channels)[iChannel]->input.getFloatValue(0);
            if ((curveStart >= start) && (curveStart <= end))
               return true;
         }
      }
      return false;
   }

   /// Get the value of the element at the specified time
   T getValue(F32 time)
   {
      // If the element is NULL, just use the default (handy for <extra> profiles which
      // may or may not be present in the document)
      T value(defaultVal);
      if (const domAny* param = daeSafeCast<domAny>(const_cast<daeElement*>(element))) {
         // If the element is not animated, just use its current value
         value = convert<T>(param->getValue());
         // Animate the value
         if (const AnimChannels* channels = AnimData::getAnimChannels(element)) {
            for (int iChannel = 0; iChannel < channels->size(); iChannel++) {
               const AnimData* animData = (*channels)[iChannel];
               F32 curveStart = animData->input.getFloatValue(0);
               F32 curveEnd = animData->input.getFloatValue(animData->input.size()-1);
               if ((time >= curveStart) && (time <= curveEnd)) {
                  animData->interpValue(time, 0, &value);
                  break;
               }
            }
         }
      }
      return value;
   }
};

template<class T> struct AnimatedElementList : public AnimatedElement<T>
{
   const daeElement* element;    ///< The Collada element (can be NULL)
   T defaultVal;                 ///< Default value (used when element is NULL)
   
   AnimatedElementList(const daeElement* e=0) : AnimatedElement<T>(e) { }

   // Get the value of the element list at the specified time
   T getValue(F32 time)
   {
      T vec(defaultVal);
      if (element) {
         // Get a copy of the vector
         vec = *(T*)const_cast<daeElement*>(element)->getValuePointer();

         // Animate the vector
         if (const AnimChannels* channels = AnimData::getAnimChannels(element)) {
            for (int iChannel = 0; iChannel < channels->size(); iChannel++) {
               const AnimData* animData = (*channels)[iChannel];
               F32 curveStart = animData->input.getFloatValue(0);
               F32 curveEnd = animData->input.getFloatValue(animData->input.size()-1);
               if ((time >= curveStart) && (time <= curveEnd)) {
                  for (int iValue = 0; iValue < animData->targetValueCount; iValue++)
                     animData->interpValue(time, iValue, &vec[animData->targetValueOffset + iValue]);
                  break;
               }
            }
         }
      }
      return vec;
   }
};

// Strongly typed animated values
typedef AnimatedElement<double> AnimatedFloat;
typedef AnimatedElement<bool> AnimatedBool;
typedef AnimatedElement<S32> AnimatedInt;
typedef AnimatedElement<const char*> AnimatedString;
typedef AnimatedElementList<domListOfFloats> AnimatedFloatList;

#endif // _COLLADA_UTILS_H_