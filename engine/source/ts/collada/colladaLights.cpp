//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2009 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/collada/colladaUtils.h"
#include "ts/collada/colladaAppNode.h"
#include "ts/collada/colladaShapeLoader.h"

//#include "T3D/pointLight.h"
// #include "T3D/spotlight.h"


//-----------------------------------------------------------------------------
// Collada <light> elements are very similar, but are arranged as separate, unrelated
// classes. These template functions are used to provide a simple way to access the
// common elements.
template<class T> static void resolveLightColor(T* light, ColorF& color)
{
   if (light->getColor())
   {
      color.red = light->getColor()->getValue()[0];
      color.green = light->getColor()->getValue()[1];
      color.blue = light->getColor()->getValue()[2];
   }
}

template<class T> static void resolveLightAttenuation(T* light, Point3F& attenuationRatio)
{
   if (light->getConstant_attenuation())
      attenuationRatio.x = light->getConstant_attenuation()->getValue();
   if (light->getLinear_attenuation())
      attenuationRatio.y = light->getLinear_attenuation()->getValue();
   if (light->getQuadratic_attenuation())
      attenuationRatio.z = light->getQuadratic_attenuation()->getValue();
}

//-----------------------------------------------------------------------------
// Recurse through the collada scene to add <light>s to the Torque scene
static void processNodeLights(AppNode* appNode, const MatrixF& offset, SimGroup* group)
{
    Con::errorf("Collada Lighting not implemented!");
   //const domNode* node = dynamic_cast<ColladaAppNode*>(appNode)->getDomNode();

   //for (S32 iLight = 0; iLight < node->getInstance_light_array().getCount(); iLight++) {

   //   domInstance_light* instLight = node->getInstance_light_array()[iLight];
   //   domLight* p_domLight = daeSafeCast<domLight>(instLight->getUrl().getElement());
   //   if (!p_domLight) {
   //      Con::warnf("Failed to find light for URL \"%s\"", instLight->getUrl().getOriginalURI());
   //      continue;
   //   }

   //   String lightName = Sim::getUniqueName(_GetNameOrId(node));
   //   const char* lightType = "";

   //   domLight::domTechnique_common* technique = p_domLight->getTechnique_common();
   //   if (!technique) {
   //      Con::warnf("No <technique_common> for light \"%s\"", lightName.c_str());
   //      continue;
   //   }

   //   LightBase* pLight = 0;
   //   ColorF color(ColorF::WHITE);
   //   Point3F attenuation(0, 1, 1);

   //   if (technique->getAmbient()) {
   //      domLight::domTechnique_common::domAmbient* ambient = technique->getAmbient();
   //      // No explicit support for ambient lights, so use a PointLight instead
   //      lightType = "ambient";
   //      pLight = new PointLight;
   //      resolveLightColor(ambient, color);
   //   }
   //   else if (technique->getDirectional()) {
   //      domLight::domTechnique_common::domDirectional* directional = technique->getDirectional();
   //      // No explicit support for directional lights, so use a SpotLight instead
   //      lightType = "directional";
   //      pLight = new SpotLight;
   //      resolveLightColor(directional, color);
   //   }
   //   else if (technique->getPoint()) {
   //      domLight::domTechnique_common::domPoint* point = technique->getPoint();
   //      lightType = "point";
   //      pLight = new PointLight;
   //      resolveLightColor(point, color);
   //      resolveLightAttenuation(point, attenuation);
   //   }
   //   else if (technique->getSpot()) {
   //      domLight::domTechnique_common::domSpot* spot = technique->getSpot();
   //      lightType = "spot";
   //      pLight = new SpotLight;
   //      resolveLightColor(spot, color);
   //      resolveLightAttenuation(spot, attenuation);
   //   }
   //   else
   //      continue;

   //   Con::printf("Adding <%s> light \"%s\" as a %s", lightType, lightName.c_str(), pLight->getClassName());

   //   MatrixF mat(offset);
   //   mat.mul(appNode->getNodeTransform(TSShapeLoader::DefaultTime));

   //   pLight->setDataField(StringTable->insert("color"), 0,
   //      avar("%f %f %f %f", color.red, color.green, color.blue, color.alpha));
   //   pLight->setDataField(StringTable->insert("attenuationRatio"), 0,
   //      avar("%f %f %f", attenuation.x, attenuation.y, attenuation.z));
   //   pLight->setTransform(mat);

   //   if (!pLight->registerObject(lightName)) {
   //      Con::errorf(ConsoleLogEntry::General, "Failed to register light for \"%s\"", lightName.c_str());
   //      delete pLight;
   //   }

   //   if (group)
   //      group->addObject(pLight);
   //}

   //// Recurse child nodes
   //for (S32 iChild = 0; iChild < appNode->getNumChildNodes(); iChild++)
   //   processNodeLights(appNode->getChildNode(iChild), offset, group);
}

// Load lights from a collada file and add to the scene.
ConsoleFunction(loadColladaLights, bool, 2, 4, "(string filename, [parentGroup], [baseObject])")
{
    Con::errorf("Collada Lighting not implemented!");
   //Torque::Path path(argv[1]);

   //// Optional group to add the lights to. Create if it does not exist, and use
   //// the MissionGroup if not specified.
   //SimGroup* missionGroup = dynamic_cast<SimGroup*>(Sim::findObject("MissionGroup"));
   //SimGroup* group = 0;
   //if ((argc > 2) && (argv[2][0])) {
   //   if (!Sim::findObject(argv[2], group)) {
   //      // Create the group if it could not be found
   //      group = new SimGroup;
   //      if (group->registerObject(argv[2])) {
   //         if (missionGroup)
   //            missionGroup->addObject(group);
   //      }
   //      else {
   //         delete group;
   //         group = 0;
   //      }
   //   }
   //}
   //if (!group)
   //   group = missionGroup;

   //// Optional object to provide the base transform
   //MatrixF offset(true);
   //if (argc > 3) {
   //   SceneObject *obj;
   //   if (Sim::findObject(argv[3], obj))
   //      offset = obj->getTransform();
   //}

   //// Load the Collada file into memory
   //domCOLLADA* root = ColladaShapeLoader::getDomCOLLADA(path);
   //if (!root) {
   //   TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Load complete");
   //   return false;
   //}

   //// Extract the global scale and up_axis from the top level <asset> element,
   //F32 unit = 1.0f;
   //domUpAxisType upAxis = UPAXISTYPE_Z_UP;
   //if (root->getAsset()) {
   //   if (root->getAsset()->getUnit())
   //      unit = root->getAsset()->getUnit()->getMeter();
   //   if (root->getAsset()->getUp_axis())
   //      upAxis = root->getAsset()->getUp_axis()->getValue();
   //}

   //ColladaUtils::getOptions().unit = unit;
   //ColladaUtils::getOptions().upAxis = upAxis;

   //// First grab all of the top-level nodes
   //Vector<ColladaAppNode*> sceneNodes;
   //for (int iSceneLib = 0; iSceneLib < root->getLibrary_visual_scenes_array().getCount(); iSceneLib++) {
   //   const domLibrary_visual_scenes* libScenes = root->getLibrary_visual_scenes_array()[iSceneLib];
   //   for (int iScene = 0; iScene < libScenes->getVisual_scene_array().getCount(); iScene++) {
   //      const domVisual_scene* visualScene = libScenes->getVisual_scene_array()[iScene];
   //      for (int iNode = 0; iNode < visualScene->getNode_array().getCount(); iNode++)
   //         sceneNodes.push_back(new ColladaAppNode(visualScene->getNode_array()[iNode]));
   //   }
   //}

   //// Recurse the scene tree looking for <instance_light>s
   //for (S32 iNode = 0; iNode < sceneNodes.size(); iNode++) {
   //   processNodeLights(sceneNodes[iNode], offset, group);
   //   delete sceneNodes[iNode];
   //}

   //TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Load complete");

   return true;
}
