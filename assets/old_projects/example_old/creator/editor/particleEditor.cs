// The Particle Editor!
// Edits both emitters and their particles in realtime in game.
// 
// Open the particle editor to spawn a test emitter in front of the player.
// Edit the sliders, check boxes, and text fields and see the results in
// realtime.  Switch between emitters and particles with the buttons in the
// top left corner.  When in particle mode, the only particles available will
// be those assigned to the current emitter to avoid confusion.  In the top
// right corner, there is a button marked "Drop Emitter", which will spawn the
// test emitter in front of the player again, and a button marked "Restart
// Emitter", which will play the particle animation again.

//TODO: animTexName on Particles (max 50)

function toggleParticleEditor(%val)
{
   if (!%val)
      return;

   if ($ParticleEditor::isOpen)
   {
      Canvas.popDialog(ParticleEditor);
      $ParticleEditor::isOpen = false;
      return;
   }

   if (!isObject(ParticleEditor))
   {
      exec("common/editor/ParticleEditor.gui");
      ParticleEditor.initEditor();
   }
   ParticleEditor.startup();

   Canvas.pushDialog(ParticleEditor);
   $ParticleEditor::isOpen = true;
}


function ParticleEditor::startup(%this)
{
   $ParticleEditor::activeEditor.updateControls();
   if (!isObject($ParticleEditor::emitterNode))
      %this.resetEmitterNode();
}

function ParticleEditor::initEditor(%this)
{
   echo("Initializing ParticleEmitterData and ParticleData DataBlocks...");

   %count = DatablockGroup.getCount();
   %emitterCount = 0;
   %particleCount = 0;

   PEE_EmitterSelector.clear();
   PEE_EmitterParticleSelector1.clear();
   PEE_EmitterParticleSelector2.clear();
   PEE_EmitterParticleSelector3.clear();
   PEE_EmitterParticleSelector4.clear();
   PEP_ParticleSelector.clear();
   for (%i = 0; %i < %count; %i++)
   {
      %obj = DatablockGroup.getObject(%i);
      if (%obj.getClassName() $= "ParticleEmitterData")
      {
         PEE_EmitterSelector.add(%obj.getName(), %emitterCount);
         %emitterCount++;
      }
      if (%obj.getClassName() $= "ParticleData")
      {
         PEE_EmitterParticleSelector1.add(%obj.getName(), %particleCount);
         PEE_EmitterParticleSelector2.add(%obj.getName(), %particleCount);
         PEE_EmitterParticleSelector3.add(%obj.getName(), %particleCount);
         PEE_EmitterParticleSelector4.add(%obj.getName(), %particleCount);
         %particleCount++;
      }
   }
   PEE_EmitterParticleSelector2.add("", %particleCount); //insert a blank space
   PEE_EmitterParticleSelector3.add("", %particleCount); //insert a blank space
   PEE_EmitterParticleSelector4.add("", %particleCount); //insert a blank space

   echo("Found" SPC %emitterCount SPC "emitters and" SPC %particleCount SPC "particles.");

   PEE_EmitterSelector.sort();
   PEE_EmitterParticleSelector1.sort();
   PEE_EmitterParticleSelector2.sort();
   PEE_EmitterParticleSelector3.sort();
   PEE_EmitterParticleSelector4.sort();

   PEE_EmitterSelector.setSelected(0);

   %this.openEmitterPane();
 }

function PE_EmitterEditor::updateControls(%this)
{
   %id = PEE_EmitterSelector.getSelected();
   %data = PEE_EmitterSelector.getTextById(%id);

   PEE_ejectionPeriodMS.setValue(   %data.ejectionPeriodMS);
   PEE_periodVarianceMS.setValue(   %data.periodVarianceMS);
   PEE_ejectionVelocity.setValue(   %data.ejectionVelocity);
   PEE_velocityVariance.setValue(   %data.velocityVariance);
   PEE_ejectionOffset.setValue(     %data.ejectionOffset);
   PEE_lifetimeMS.setValue(         %data.lifetimeMS);
   PEE_lifetimeVarianceMS.setValue( %data.lifetimeVarianceMS);
   PEE_thetaMin.setValue(           %data.thetaMin);
   PEE_thetaMax.setValue(           %data.thetaMax);
   PEE_phiReferenceVel.setValue(    %data.phiReferenceVel);
   PEE_phiVariance.setValue(        %data.phiVariance);
   PEE_overrideAdvance.setValue(    %data.overrideAdvance);
   PEE_orientParticles.setValue(    %data.orientParticles);
   PEE_orientOnVelocity.setValue(   %data.orientOnVelocity);
   PEE_useEmitterSizes.setValue(    %data.useEmitterSizes);
   PEE_useEmitterColors.setValue(   %data.useEmitterColors);

   PEE_EmitterParticleSelector1.setText(getField(%data.particles, 0));
   PEE_EmitterParticleSelector2.setText(getField(%data.particles, 1));
   PEE_EmitterParticleSelector3.setText(getField(%data.particles, 2));
   PEE_EmitterParticleSelector4.setText(getField(%data.particles, 3));

   $ParticleEditor::currEmitter = %data;
}

function PE_ParticleEditor::updateControls(%this)
{
   %id = PEP_ParticleSelector.getSelected();
   %data = PEP_ParticleSelector.getTextById(%id);

   PEP_dragCoefficient.setValue(     %data.dragCoefficient);
   PEP_windCoefficient.setValue(     %data.windCoefficient);
   PEP_gravityCoefficient.setValue(  %data.gravityCoefficient);
   PEP_inheritedVelFactor.setValue(  %data.inheritedVelFactor);
   PEP_constantAcceleration.setValue(%data.constantAcceleration);
   PEP_lifetimeMS.setValue(          %data.lifetimeMS);
   PEP_lifetimeVarianceMS.setValue(  %data.lifetimeVarianceMS);
   PEP_spinSpeed.setValue(           %data.spinSpeed);
   PEP_spinRandomMin.setValue(       %data.spinRandomMin);
   PEP_framesPerSec.setValue(        %data.framesPerSec);
   PEP_spinRandomMax.setValue(       %data.spinRandomMax);
   PEP_useInvAlpha.setValue(         %data.useInvAlpha);
   PEP_animateTexture.setValue(      %data.animateTexture);
   PEP_times0.setText(               %data.times[0]);
   PEP_times1.setText(               %data.times[1]);
   PEP_times2.setText(               %data.times[2]);
   PEP_times3.setText(               %data.times[3]);
   PEP_sizes0.setText(               %data.sizes[0]);
   PEP_sizes1.setText(               %data.sizes[1]);
   PEP_sizes2.setText(               %data.sizes[2]);
   PEP_sizes3.setText(               %data.sizes[3]);
   PEP_colors0.setText(              %data.colors[0]);
   PEP_colors1.setText(              %data.colors[1]);
   PEP_colors2.setText(              %data.colors[2]);
   PEP_colors3.setText(              %data.colors[3]);
   PEP_textureName.setText(          %data.textureName);

   $ParticleEditor::currParticle =   %data;
}

function ParticleEditor::openEmitterPane(%this)
{
   PE_Window.setText("Particle Editor - Emitters");
   PE_EmitterEditor.setVisible(true);
   PE_EmitterButton.setActive(false);
   PE_ParticleEditor.setVisible(false);
   PE_ParticleButton.setActive(true);
   PE_EmitterEditor.updateControls();
   $ParticleEditor::activeEditor = PE_EmitterEditor;
}

function ParticleEditor::openParticlePane(%this)
{
   PE_Window.setText("Particle Editor - Particles");
   PE_EmitterEditor.setVisible(false);
   PE_EmitterButton.setActive(true);
   PE_ParticleEditor.setVisible(true);
   PE_ParticleButton.setActive(false);

   PEP_ParticleSelector.clear();
   PEP_ParticleSelector.add(PEE_EmitterParticleSelector1.getText(), 0);
   %count = 1;
   if (PEE_EmitterParticleSelector2.getText() !$= "")
   {
      PEP_ParticleSelector.add(PEE_EmitterParticleSelector2.getText(), %count);
      %count++;
   }
   if (PEE_EmitterParticleSelector3.getText() !$= "")
   {
      PEP_ParticleSelector.add(PEE_EmitterParticleSelector3.getText(), %count);
      %count++;
   }
   if (PEE_EmitterParticleSelector4.getText() !$= "")
      PEP_ParticleSelector.add(PEE_EmitterParticleSelector4.getText(), %count);

   PEP_ParticleSelector.sort();
   PEP_ParticleSelector.setSelected(0);   //select the PEE_EmitterParticleSelector1 particle

   PE_ParticleEditor.updateControls();
   $ParticleEditor::activeEditor = PE_ParticleEditor;
}

function PE_EmitterEditor::updateEmitter(%this)
{
   $ParticleEditor::currEmitter.ejectionPeriodMS = PEE_ejectionPeriodMS.getValue();
   $ParticleEditor::currEmitter.periodVarianceMS = PEE_periodVarianceMS.getValue();
   if ($ParticleEditor::currEmitter.periodVarianceMS >= $ParticleEditor::currEmitter.ejectionPeriodMS)
   {
      $ParticleEditor::currEmitter.periodVarianceMS = $ParticleEditor::currEmitter.ejectionPeriodMS - 1;
      PEE_periodVarianceMS.setValue($ParticleEditor::currEmitter.periodVarianceMS);
   }
   $ParticleEditor::currEmitter.ejectionVelocity   = PEE_ejectionVelocity.getValue();
   $ParticleEditor::currEmitter.velocityVariance   = PEE_velocityVariance.getValue();
   if ($ParticleEditor::currEmitter.velocityVariance >= $ParticleEditor::currEmitter.ejectionVelocity)
   {
      $ParticleEditor::currEmitter.velocityVariance = $ParticleEditor::currEmitter.ejectionVelocity - 0.01;
      if ($ParticleEditor::currEmitter.velocityVariance < 0)
         $ParticleEditor::currEmitter.velocityVariance = 0;
      PEE_velocityVariance.setValue($ParticleEditor::currEmitter.velocityVariance);
   }
   $ParticleEditor::currEmitter.ejectionOffset     = PEE_ejectionOffset.getValue();
   $ParticleEditor::currEmitter.lifetimeMS         = PEE_lifetimeMS.getValue();
   $ParticleEditor::currEmitter.lifetimeVarianceMS = PEE_lifetimeVarianceMS.getValue();
   if ($ParticleEditor::currEmitter.lifetimeMS == 0)
   {
      $ParticleEditor::currEmitter.lifetimeVarianceMS = 0;
      PEE_lifetimeVarianceMS.setValue($ParticleEditor::currEmitter.lifetimeVarianceMS);
   }
   else if ($ParticleEditor::currEmitter.lifetimeVarianceMS >= $ParticleEditor::currEmitter.lifetimeMS)
   {
      $ParticleEditor::currEmitter.lifetimeVarianceMS = $ParticleEditor::currEmitter.lifetimeMS - 1;
      PEE_lifetimeVarianceMS.setValue($ParticleEditor::currEmitter.lifetimeVarianceMS);
   }
   $ParticleEditor::currEmitter.thetaMin           = PEE_thetaMin.getValue();
   $ParticleEditor::currEmitter.thetaMax           = PEE_thetaMax.getValue();
   $ParticleEditor::currEmitter.phiReferenceVel    = PEE_phiReferenceVel.getValue();
   $ParticleEditor::currEmitter.phiVariance        = PEE_phiVariance.getValue();
   $ParticleEditor::currEmitter.overrideAdvance    = PEE_overrideAdvance.getValue();
   $ParticleEditor::currEmitter.orientParticles    = PEE_orientParticles.getValue();
   $ParticleEditor::currEmitter.orientOnVelocity   = PEE_orientOnVelocity.getValue();
   $ParticleEditor::currEmitter.useEmitterSizes    = PEE_useEmitterSizes.getValue();
   $ParticleEditor::currEmitter.useEmitterColors   = PEE_useEmitterColors.getValue();
   $ParticleEditor::currEmitter.particles          = PEE_EmitterParticleSelector1.getText();

   if (PEE_EmitterParticleSelector2.getText() !$= "")
      $ParticleEditor::currEmitter.particles = $ParticleEditor::currEmitter.particles TAB PEE_EmitterParticleSelector2.getText();
   if (PEE_EmitterParticleSelector3.getText() !$= "")
      $ParticleEditor::currEmitter.particles = $ParticleEditor::currEmitter.particles TAB PEE_EmitterParticleSelector3.getText();
   if (PEE_EmitterParticleSelector4.getText() !$= "")
      $ParticleEditor::currEmitter.particles = $ParticleEditor::currEmitter.particles TAB PEE_EmitterParticleSelector4.getText();

   $ParticleEditor::currEmitter.reload();                        
}

function PE_ParticleEditor::updateParticle(%this)
{
   $ParticleEditor::currParticle.dragCoefficient      = PEP_dragCoefficient.getValue();
   $ParticleEditor::currParticle.windCoefficient      = PEP_windCoefficient.getValue();
   $ParticleEditor::currParticle.gravityCoefficient   = PEP_gravityCoefficient.getValue();
   $ParticleEditor::currParticle.inheritedVelFactor   = PEP_inheritedVelFactor.getValue();
   $ParticleEditor::currParticle.constantAcceleration = PEP_constantAcceleration.getValue();
   $ParticleEditor::currParticle.lifetimeMS           = PEP_lifetimeMS.getValue();
   $ParticleEditor::currParticle.lifetimeVarianceMS   = PEP_lifetimeVarianceMS.getValue();
   if ($ParticleEditor::currParticle.lifetimeVarianceMS >= $ParticleEditor::currParticle.lifetimeMS)
   {
      $ParticleEditor::currParticle.lifetimeVarianceMS = $ParticleEditor::currParticle.lifetimeMS - 1;
      PEP_lifetimeVarianceMS.setValue($ParticleEditor::currParticle.lifetimeVarianceMS);
   }
   $ParticleEditor::currParticle.spinSpeed            = PEP_spinSpeed.getValue();
   $ParticleEditor::currParticle.spinRandomMin        = PEP_spinRandomMin.getValue();
   $ParticleEditor::currParticle.spinRandomMax        = PEP_spinRandomMax.getValue();
   $ParticleEditor::currParticle.framesPerSec         = PEP_framesPerSec.getValue();
   $ParticleEditor::currParticle.useInvAlpha          = PEP_useInvAlpha.getValue();
   $ParticleEditor::currParticle.animateTexture       = PEP_animateTexture.getValue();

   $ParticleEditor::currParticle.times[0]             = PEP_times0.getValue();
   $ParticleEditor::currParticle.times[1]             = PEP_times1.getValue();
   $ParticleEditor::currParticle.times[2]             = PEP_times2.getValue();
   $ParticleEditor::currParticle.times[3]             = PEP_times3.getValue();
   $ParticleEditor::currParticle.sizes[0]             = PEP_sizes0.getValue();
   $ParticleEditor::currParticle.sizes[1]             = PEP_sizes1.getValue();
   $ParticleEditor::currParticle.sizes[2]             = PEP_sizes2.getValue();
   $ParticleEditor::currParticle.sizes[3]             = PEP_sizes3.getValue();
   $ParticleEditor::currParticle.colors[0]            = PEP_colors0.getValue();
   $ParticleEditor::currParticle.colors[1]            = PEP_colors1.getValue();
   $ParticleEditor::currParticle.colors[2]            = PEP_colors2.getValue();
   $ParticleEditor::currParticle.colors[3]            = PEP_colors3.getValue();

   $ParticleEditor::currParticle.textureName          = PEP_textureName.getValue();

   $ParticleEditor::currParticle.reload();                        
}

function PE_EmitterEditor::onNewEmitter(%this)
{
   ParticleEditor.updateEmitterNode();
   PE_EmitterEditor.updateControls();
}

function PE_ParticleEditor::onNewParticle(%this)
{
   PE_ParticleEditor.updateControls();
}

function ParticleEditor::resetEmitterNode(%this)
{
   %tform = ServerConnection.getControlObject().getEyeTransform();
   %vec = VectorNormalize(ServerConnection.getControlObject().getForwardVector());
   %vec = VectorScale(%vec, 4);
   %tform = setWord(%tform, 0, getWord(%tform, 0) + getWord(%vec, 0));
   %tform = setWord(%tform, 1, getWord(%tform, 1) + getWord(%vec, 1));
   %tform = setWord(%tform, 2, getWord(%tform, 2) + getWord(%vec, 2));

   if (!isObject($ParticleEditor::emitterNode))
   {
      if (!isObject(TestEmitterNodeData))
      {
         datablock ParticleEmitterNodeData(TestEmitterNodeData)
         {
            timeMultiple = 1;
         };
      }

      $ParticleEditor::emitterNode = new ParticleEmitterNode()
      {
         emitter = PEE_EmitterSelector.getText();
         velocity = 1;
         position = getWords(%tform, 0, 2);
         rotation = getWords(%tform, 3, 6);
         datablock = TestEmitterNodeData;
      };
      //grab the client-side emitter node so we can reload the emitter datablock
      $ParticleEditor::clientEmitterNode = $ParticleEditor::emitterNode+1;
   }
   else
   {
      $ParticleEditor::clientEmitterNode.setTransform(%tform);
      ParticleEditor.updateEmitterNode();
   }
}

function ParticleEditor::updateEmitterNode()
{
   $ParticleEditor::clientEmitterNode.setEmitterDataBlock(PEE_EmitterSelector.getText().getId());
}

function PE_EmitterEditor::save(%this)
{
   %mod = $currentMod;
   if (%mod $= "")
   {
      warn("Warning: No mod detected, saving in common.");
      %mod = "common";
   }
   %filename = %mod @ "/" @ $ParticleEditor::currEmitter @ ".cs";
   $ParticleEditor::currEmitter.save(%filename);
}

function PE_ParticleEditor::save(%this)
{
   %mod = $currentMod;
   if (%mod $= "")
   {
      warn("Warning: No mod detected, saving in common.");
      %mod = "common";
   }
   %filename = %mod @ "/" @ $ParticleEditor::currParticle @ ".cs";
   $ParticleEditor::currParticle.save(%filename);
}

