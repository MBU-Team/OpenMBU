//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  Functions that implement game-play
//-----------------------------------------------------------------------------

package DemoSceneGame {

//-----------------------------------------------------------------------------

function GameConnection::onClientEnterGame(%this)
{
   commandToClient(%this,'SyncClock',$Sim::Time - $Game::StartTime);
   commandToClient(%this,'SetGameGUI',"SceneGui");

   // Create a new camera object.
   %this.camera = new Camera() {
      dataBlock = Observer;
      gui = "CameraGUI";
   };
   MissionCleanup.add( %this.camera );
   %this.camera.scopeToClient(%this);

   // Create scene camera
   %this.player = new PathCamera() {
      dataBlock = LoopingCam;
      position = Scene::getStartPos();
      gui = "SceneGUI";
   };
   MissionCleanup.add( %this.player );
   %this.player.scopeToClient(%this);

   // Create idle loop camera
   %this.idleCamera = new PathCamera() {
      dataBlock = LoopingCam;
      position = Scene::getStartPos();
      gui = "CameraGUI";
   };
   MissionCleanup.add(%this.idleCamera);
   %this.idleCamera.scopeToClient(%this);
   %this.idleCamera.reset(0);
   %this.idleCamera.followPath("MissionGroup/Paths/CameraLoop");

   // Start up the room gui system
   $Server::Client = %this;
   %this.setControlObject(%this.player);
   SceneGui.setSceneNumber(0);

   // Force detail settings
   $Pref::TS::detailAdjust = 1;
   $Pref::TS::screenError = 10;

   // Start NPC orcs
   //newNPCOrc("MissionGroup/Paths/OrcNPC1",0.75);
   //newNPCOrc("MissionGroup/Paths/OrcNPC2",0.75);
   //newNPCOrc("MissionGroup/Paths/OrcNPC3",0.40);
   startAllBlobs();
}


//-----------------------------------------------------------------------------

function GameConnection::createPlayer(%this, %spawnPoint)
{
}


//-----------------------------------------------------------------------------

function serverCmdSuicide(%client)
{
   if (isObject(%client.player)) {
      %client.player.delete();
      %client.player = 0;
   }
   %client.spawnPlayer();
}

};


//-----------------------------------------------------------------------------
// Scene Class
//-----------------------------------------------------------------------------

$Server::CurrentScene = 0;

// Scene activation is driven from the DemoGui and related scripts.
// (see demo/client/scripts/demoGui.cs)
function Scene::activateScene(%number)
{
   if (isObject($Server::CurrentScene))
      $Server::CurrentScene.close();

   $Server::CurrentScene = "MissionGroup/Scenes".getObject(%number);
   echo("Activating Scene #"@%number);
   $Server::CurrentScene.open();
   return $Server::CurrentScene;
}

function Scene::open(%this)
{
   echo("Scene " @ %this.getName() @ " open");
   %client = $Server::Client;

   // Push any scene specific controls
   if (isObject(%this.gui))
      Canvas.pushDialog(%this.gui);

   // Check for a path reference
   %path = %this.path;
   if (isObject(%path)) {
      if (%path.getId() != %client.player.path) {
         %client.player.reset(0);
         %client.player.followPath(%path);
      }
   }
   else {
      // Check for a path object.
      %path = %this.getId() @ "/path";
      if (isObject(%path)) {
         %client.player.reset(0);
         %client.player.followPath(%path);
      }
      else {
         // Check for a static camera
         %start = %this.getId() @ "/start";
         if (isObject(%start)) {
            echo("Static Camera");
            %client.player.path = 0;
            %client.player.reset(0);
            %client.player.pushNode(%start);
            %client.player.popFront();
         }
      }
   }
}

function Scene::getStartPos()
{
   %scene = "MissionGroup/Scenes".getObject(0);
   %start = %scene.getId() @ "/start";
   if (isObject(%start))
      return %start.getTransform();
   return "0 0 100";
}

function Scene::close(%this)
{
   %client = $Server::Client;

   // Pop any scene specific controls
   if (isObject(%this.gui))
      Canvas.popDialog(%this.gui);

   echo("Scene " @ %this.getName() @ " closed");
}


//-----------------------------------------------------------------------------
// Orc Animation
//-----------------------------------------------------------------------------

function newNPCOrc(%path,%speed)
{
   %orc = AIPlayer::spawnOnPath("NPC",%path);
   %orc.setMoveSpeed(%speed);
   %orc.mountImage(CrossbowImage,0);
   %orc.pushTask("followPath(\""@%path@"\",-1)");
}


function newAnimationOrc()
{
   if (isObject($AnimationOrc))
      $AnimationOrc.clearTasks();
   else {
      $AnimationOrc = AIPlayer::spawnOnPath("Tarkof","MissionGroup/Paths/OrcAnimation");
      $AnimationOrc.setMoveSpeed(0.75);
   }
   $AnimationOrc.pushTask("followPath(\"MissionGroup/Paths/OrcAnimation\",100)");
}

function deleteAnimationOrc()
{
   if (isObject($AnimationOrc)) {
      $AnimationOrc.clearTasks();
      $AnimationOrc.pushTask("followPath(\"MissionGroup/Paths/OrcAnimation\",0)");
      $AnimationOrc.pushTask("done()");
   }
}

function AnimationOrcPlay(%anim)
{
   $AnimationOrc.clearTasks();
   $AnimationOrc.pushTask("animate(\""@%anim@"\")");
}

function newDancingOrc()
{
   if (isObject($DancingOrc))
      $DancingOrc.clearTasks();
   else {
      $DancingOrc = AIPlayer::spawnOnPath("Dance","MissionGroup/Paths/OrcDance");
      $DancingOrc.setMoveSpeed(0.75);
   }
   $DancingOrc.pushTask("followPath(\"MissionGroup/Paths/OrcDance\",100)");
   $DancingOrc.pushTask("animate(\"dance\")");
}

function deleteDancingOrc()
{
   if (isObject($DancingOrc)) {
      $DancingOrc.clearTasks();
      $DancingOrc.pushTask("followPath(\"MissionGroup/Paths/OrcDance\",0)");
      $DancingOrc.pushTask("done()");
   }
}

function newDetailOrc()
{
   if (isObject($DetailOrc)) {
      $DetailOrc.clearTasks();
      $DetailOrc.unmountImage(0);
      $DetailOrc.unmountImage(1);
      $DetailOrc.setArmThread("looknw");
   }
   else {
      $DetailOrc = AIPlayer::spawnOnPath("Detail","MissionGroup/Paths/OrcDetail");
      $DetailOrc.setMoveSpeed(0.75);
      $DetailOrc.setArmThread("looknw");
   }
   $DetailOrc.pushTask("followPath(\"MissionGroup/Paths/OrcDetail\",100)");
}

function deleteDetailOrc()
{
   if (isObject($DetailOrc)) {
      $DetailOrc.clearTasks();
      $DetailOrc.pushTask("followPath(\"MissionGroup/Paths/OrcDetail\",0)");
      $DetailOrc.pushTask("done()");
   }
}

function mountDetailOrc(%object,%slot)
{
   if ($DetailOrc.getMountedImage(%slot) == %object.getId()) {
      $DetailOrc.unmountImage(%slot);
      if (%slot == 0)
         $DetailOrc.setArmThread("looknw");
   }
   else {
      $DetailOrc.mountImage(%object,%slot);
      if (%slot == 0)
         $DetailOrc.setArmThread("look");
   }
}

function newShootingOrc()
{
   if (isObject($ShootingOrc))
      $ShootingOrc.clearTasks();
   else {
      $ShootingOrc = AIPlayer::spawnOnPath("Detail","MissionGroup/Paths/OrcShooting");
      $ShootingOrc.mountImage(CrossbowImage,0);
      $ShootingOrc.setInventory(CrossbowAmmo,500000);
      $ShootingOrc.setMoveSpeed(0.75);
   }
   $ShootingOrc.pushTask("followPath(\"MissionGroup/Paths/OrcShooting\",100)");
   $ShootingOrc.pushTask("aimAt(\"MissionGroup/Scenes/ProjectileScene/target\")");
   $ShootingOrc.pushTask("wait(1)");
   $ShootingOrc.pushTask("fire(true)");
}

function deleteShootingOrc()
{
   if (isObject($ShootingOrc)) {
      $ShootingOrc.clearTasks();
      $ShootingOrc.pushTask("fire(false)");
      $ShootingOrc.pushTask("followPath(\"MissionGroup/Paths/OrcShooting\",0)");
      $ShootingOrc.pushTask("done()");
   }
}


//-----------------------------------------------------------------------------
// Moving Blobs
//-----------------------------------------------------------------------------

datablock PathCameraData(Blob1)
{
   mode = "";
   shapeFile = "~/data/shapes/test/blob1.dts";
};

datablock PathCameraData(Blob2)
{
   mode = "";
   shapeFile = "~/data/shapes/test/blob2.dts";
};

datablock PathCameraData(Floater)
{
   mode = "";
   shapeFile = "~/data/shapes/test/blob1.dts";
};

function startAllBlobs()
{
   startBlob("blob1","MissionGroup/Paths/Blob1Path");
   startBlob("blob2","MissionGroup/Paths/Blob2Path");
   startBlob("floater","MissionGroup/Paths/Floater1","0.15 0.15 0.15");
   startBlob("floater","MissionGroup/Paths/Floater2","0.15 0.15 0.15");
}

function startBlob(%datablock,%path,%scale)
{
   %blob = new PathCamera() {
      dataBlock = %datablock;
      position = Scene::getStartPos();
   };
   MissionCleanup.add(%blob);
   %blob.reset(0);
   %blob.followPath(%path);

   if (%scale !$= "")
      %blob.setScale(%scale);

   return %blob;
}


//-----------------------------------------------------------------------------
// Environment effects
//-----------------------------------------------------------------------------

function startRain()
{
   if (!isObject($Rain))
      $Rain = new Precipitation() {
         datablock = "HeavyRain";
         percentage = "1";
         color1 = "0.700000 0.700000 0.700000 1";
         color2 = "-1.000000 0.000000 0.000000 1.000000";
         color3 = "-1.000000 0.000000 0.000000 1.000000";
         offsetSpeed = "0";
         minVelocity = "0.25";
         maxVelocity = "1.5";
         maxNumDrops = "3000";
         maxRadius = "10";
         locked = "false";
      };
}

function stopRain()
{
   $Rain.delete();
}

function startLightning()
{
   if (!isObject($Lightning))
      $Lightning = new Lightning() {
         position = "350 300 180";
         scale = "250 400 500";
         dataBlock = "LightningStorm";
         strikesPerMinute = "20";
         strikeWidth = "2.5";
         chanceToHitTarget = "100";
         strikeRadius = "50";
         boltStartRadius = "20";
         color = "1.000000 1.000000 1.000000 1.000000";
         fadeColor = "0.100000 0.100000 1.000000 1.000000";
         useFog = "0";
         locked = "false";
      };
}

function stopLightning()
{
   $Lightning.delete();
}


//-----------------------------------------------------------------------------
// Scenes
//-----------------------------------------------------------------------------

function WelcomeScene::open(%this)
{
   Parent::open(%this);
   newAnimationOrc();
   $AnimationOrc.mountImage(SpaceGunImage,0);
   $AnimationOrc.pushTask("wait(0.5)");
   $AnimationOrc.pushTask("animate(\"celwave\")");
}

function Intro::open(%this)
{
   Parent::open(%this);
   AnimationOrcPlay("dance");
   $AnimationOrc.unmountImage(0);
}



//-----------------------------------------------------------------------------
// Path Camera
//-----------------------------------------------------------------------------

datablock PathCameraData(LoopingCam)
{
   mode = "";
};

function PathCameraData::onNode(%this,%camera,%node)
{
   if (%node == %camera.loopNode) {
      %camera.pushPath(%camera.path);
      %camera.loopNode += %camera.path.getCount();
   }
}

//function PathCamera::reset(%this,%speed)
//{
//   %this.path = 0;
//   Parent::reset(%this,%speed);
//}

function PathCamera::followPath(%this,%path)
{
   %this.path = %path.getId();
   if (!(%this.speed = %path.speed))
      %this.speed = 100;
   if (%path.isLooping)
      %this.loopNode = %path.getCount() - 2;
   else
      %this.loopNode = -1;

   %this.pushPath(%path);
   %this.popFront();
}

function PathCamera::pushPath(%this,%path)
{
   for (%i = 0; %i < %path.getCount(); %i++)
      %this.pushNode(%path.getObject(%i));
}

function PathCamera::pushNode(%this,%node)
{
   if (!(%speed = %node.speed))
      %speed = %this.speed;
   if ((%type = %node.type) $= "")
      %type = "Normal";
   if ((%smoothing = %node.smoothing) $= "")
      %smoothing = "Linear";
   %this.pushBack(%node.getTransform(),%speed,%type,%smoothing);
}


//-----------------------------------------------------------------------------

datablock ItemData(Logo)
{
   // An item is used
   category = "Misc";
   shapeFile = "~/data/shapes/logo/torque_logo.dts";
};

datablock ItemData( BoxEffects )
{
   shapeFile = "~/data/shapes/test/newBox2.dts";
   dynamicReflection = false;
};

datablock ItemData( TestSphere )
{
   shapeFile = "~/data/shapes/test/sphere_all.dts";
   dynamicReflection = true;
};

datablock ItemData(SpaceGun)
{
   // Mission editor category
   category = "Weapon";
   className = "Weapon";

   // Basic Item properties
   shapeFile = "~/data/shapes/spaceOrc/gun.dts";
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;
   emap = true;
};

datablock ShapeBaseImageData(SpaceGunImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/spaceOrc/gun.dts";
   emap = true;
   mountPoint = 0;
   eyeOffset = "0.1 0.4 -0.6";
   offset = "0 0.55 .1";
   className = "WeaponImage";
};


