//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShapeInstance.h"

//-------------------------------------------------------------------------------------
// This file contains the shape instance thread class (defined in tsShapeInstance.h)
// and the tsShapeInstance functions to interface with the thread class.
//-------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
// Thread class
//-------------------------------------------------------------------------------------

// given a position on the thread, choose correct keyframes
// slight difference between one-shot and cyclic sequences -- see comments below for details
void TSThread::selectKeyframes(F32 pos, const TSSequence * seq, S32 * k1, S32 * k2, F32 * kpos)
{
   const TSShape * shape = mShapeInstance->mShape;
   S32 numKF = seq->numKeyframes;
   F32 kf;

   if (seq->isCyclic())
   {
      // cyclic sequence:
      // pos=0 and pos=1 are equivalent, so we don't have a keyframe at pos=1
      // last keyframe corresponds to pos=n/(n-1) up to (not including) pos=1
      // (where n == num keyframes)

      AssertFatal(pos>=0.0f && pos<1.0f,"TSThread::selectKeyframes");

      kf = pos * (F32) (numKF);

      // set keyPos
      if (kpos)
         *kpos = kf - (S32) kf;

      // make sure compiler doing what we want...
      AssertFatal(*kpos>=0.0f && *kpos<1.0f,"TSThread::selectKeyframes");

      S32 kfIdx1 = (S32) kf;

      // following assert could happen if pos1<1 && pos1==1...paradoxically...
      AssertFatal(kfIdx1<=seq->numKeyframes,"TSThread::selectKeyframes");

      S32 kfIdx2 = (kfIdx1==seq->numKeyframes-1) ? 0 : kfIdx1+1;

      if (k1)
         *k1 = kfIdx1;
      if (k2)
         *k2 = kfIdx2;
   }
   else
   {
      // one-shot sequence:
      // pos=0 and pos=1 are now different, so we have a keyframe at pos=1
      // last keyframe corresponds to pos=1
      // rest of the keyframes are equally spaced (so 1/(n-1) pos units long)
      // (where n == num keyframes)

      AssertFatal(pos>=0.0f && pos<=1.0f,"TSThread::selectKeyframes");

      if (pos==1.0f)
      {
         if (kpos)
            *kpos = 0.0f;
         if (k1)
            *k1 = seq->numKeyframes-1;
         if (k2)
            *k2 = seq->numKeyframes-1;
      }
      else
      {
         kf = pos * (F32) (numKF-1);

         // set keyPos
         if (kpos)
            *kpos = kf - (S32) kf;

         S32 kfIdx1 = (S32) kf;

         // following assert could happen if pos1<1 && pos1==1...paradoxically...
         AssertFatal(kfIdx1<seq->numKeyframes,"TSThread::selectKeyFrames: invalid keyframe!");

         S32 kfIdx2 = kfIdx1+1;

         if (k1)
            *k1 = kfIdx1;
         if (k2)
            *k2 = kfIdx2;
      }
   }
}

void TSThread::getGround(F32 t, MatrixF * pMat)
{
   static const QuatF unitRotation(0,0,0,1);
   static const Point3F unitTranslation(0,0,0);

   const QuatF * q1, * q2;
   QuatF rot1,rot2;
   const Point3F * p1, * p2;

   // if N = sequence->numGroundFrames, then there are N+1 positions we
   // interpolate betweeen:  0/N, 1/N ... N/N
   // we need to convert the p passed to us into 2 ground keyframes:

   // the 0.99999f is in case 'p' is exactly 1.0f, which is legal but 'kf'
   // needs to be strictly less than 'sequence->numGroundFrames'
   F32 kf = 0.999999f * t * (F32) sequence->numGroundFrames;

   // get frame number and interp param (kpos)
   S32 frame = (S32)kf;
   F32 kpos = kf - (F32)frame;

   // now point pT1 and pT2 at transforms for keyframes 'frame' and 'frame+1'

   // following a little strange:  first ground keyframe (0/N in comment above) is
   // assumed to be ident. and not found in the list.
   if (frame)
   {
      p1 = &mShapeInstance->mShape->groundTranslations[sequence->firstGroundFrame + frame - 1];
      q1 = &mShapeInstance->mShape->groundRotations[sequence->firstGroundFrame + frame - 1].getQuatF(&rot1);
   }
   else
   {
      p1 = &unitTranslation;
      q1 = &unitRotation;
   }

   // similar to above, ground keyframe number 'frame+1' is actually offset by 'frame'
   p2 = &mShapeInstance->mShape->groundTranslations[sequence->firstGroundFrame + frame];
   q2 = &mShapeInstance->mShape->groundRotations[sequence->firstGroundFrame + frame].getQuatF(&rot2);

   QuatF q;
   Point3F p;
   TSTransform::interpolate(*q1,*q2,kpos,&q);
   TSTransform::interpolate(*p1,*p2,kpos,&p);
   TSTransform::setMatrix(q,p,pMat);
}

void TSThread::setSequence(S32 seq, F32 toPos)
{
   const TSShape * shape = mShapeInstance->mShape;

   AssertFatal(shape && shape->sequences.size()>seq && toPos>=0.0f && toPos<=1.0f,
      "TSThread::setSequence: invalid shape handle, sequence number, or position.");

   mShapeInstance->clearTransition(this);

   sequence = &shape->sequences[seq];
   priority = sequence->priority;
   pos = toPos;
   makePath = sequence->makePath();

   // 1.0f doesn't exist on cyclic sequences
   if (pos>0.9999f && sequence->isCyclic())
      pos = 0.9999f;

   // select keyframes
   selectKeyframes(pos,sequence,&keyNum1,&keyNum2,&keyPos);
}

void TSThread::transitionToSequence(S32 seq, F32 toPos, F32 duration, bool continuePlay)
{
   AssertFatal(duration>=0.0f,"TSThread::transitionToSequence: negative duration not allowed");

   const TSShape * shape = mShapeInstance->mShape;

   // make sure these nodes are smoothly interpolated to new positions...
   // basically, any node we controlled just prior to transition, or at any stage
   // of the transition is interpolated.  If we start to transtion from A to B,
   // but before reaching B we transtion to C, we interpolate all nodes controlled
   // by A, B, or C to their new position.
   if (transitionData.inTransition)
   {
      transitionData.oldRotationNodes.overlap(sequence->rotationMatters);
      transitionData.oldTranslationNodes.overlap(sequence->translationMatters);
      transitionData.oldScaleNodes.overlap(sequence->scaleMatters);
   }
   else
   {
      transitionData.oldRotationNodes = sequence->rotationMatters;
      transitionData.oldTranslationNodes = sequence->translationMatters;
      transitionData.oldScaleNodes = sequence->scaleMatters;
   }

   // set time characteristics of transition
   transitionData.oldSequence = (S32)(sequence-&shape->sequences[0]);
   transitionData.oldPos = pos;
   transitionData.duration = duration;
   transitionData.pos = 0.0f;
   transitionData.direction = timeScale>0.0f ? 1.0f : -1.0f;
   transitionData.targetScale = continuePlay ? 1.0f : 0.0f;

   // in transition...
   transitionData.inTransition = true;

   // set target sequence data
   sequence = &shape->sequences[seq];
   priority = sequence->priority;
   pos = toPos;
   makePath = sequence->makePath();

   // 1.0f doesn't exist on cyclic sequences
   if (pos>0.9999f && sequence->isCyclic())
      pos = 0.9999f;

   // select keyframes
   selectKeyframes(pos,sequence,&keyNum1,&keyNum2,&keyPos);
}

bool TSThread::isInTransition()
{
   return transitionData.inTransition;
}

void TSThread::animateTriggers()
{
   if (!sequence->numTriggers)
      return;

   switch (path.loop)
   {
      case -1 :
         activateTriggers(path.start,0);
         activateTriggers(1,path.end);
         break;
      case  0 :
         activateTriggers(path.start,path.end);
         break;
      case  1 :
         activateTriggers(path.start,1);
         activateTriggers(0,path.end);
         break;
      default:
      {
         if (path.loop>0)
         {
            activateTriggers(path.end,1);
            activateTriggers(0,path.end);
         }
         else
         {
            activateTriggers(path.end,0);
            activateTriggers(1,path.end);
         }
      }
   }
}

void TSThread::activateTriggers(F32 a, F32 b)
{
   S32 i;
   const TSShape * shape = mShapeInstance->mShape;
   S32 firstTrigger = sequence->firstTrigger;
   S32 numTriggers = sequence->numTriggers;

   // first find triggers at position a and b
   // we assume there aren't many triggers, so
   // search is linear
   F32 lastPos = -1.0f;
   S32 aIndex  = numTriggers+firstTrigger; // initialized to handle case where pos past all triggers
   S32 bIndex  = numTriggers+firstTrigger; // initialized to handle case where pos past all triggers
   for (i=firstTrigger; i<numTriggers+firstTrigger; i++)
   {
      // is a between this trigger and previous one...
      if (a>lastPos && a<=shape->triggers[i].pos)
         aIndex = i;
      // is b between this trigger and previous one...
      if (b>lastPos && b<=shape->triggers[i].pos)
         bIndex = i;
      lastPos = shape->triggers[i].pos;
   }

   // activate triggers between aIndex and bIndex (depends on direction)
   if (aIndex<=bIndex)
   {
      for (i=aIndex; i<bIndex; i++)
      {
         U32 state = shape->triggers[i].state;
         bool on = (state & TSShape::Trigger::StateOn)!=0;
         mShapeInstance->setTriggerStateBit(state & TSShape::Trigger::StateMask, on);
      }
   }
   else
   {
      for (i=aIndex-1; i>=bIndex; i--)
      {
         U32 state = shape->triggers[i].state;
         bool on = (state & TSShape::Trigger::StateOn)!=0;
         if (state & TSShape::Trigger::InvertOnReverse)
            on = !on;
         mShapeInstance->setTriggerStateBit(state & TSShape::Trigger::StateMask, on);
      }
   }
}

F32 TSThread::getPos()
{
   return transitionData.inTransition ? transitionData.pos : pos;
}

F32 TSThread::getTime()
{
   return transitionData.inTransition ? transitionData.pos * transitionData.duration : pos * sequence->duration;
}

F32 TSThread::getDuration()
{
   return transitionData.inTransition ? transitionData.duration : sequence->duration;
}

F32 TSThread::getScaledDuration()
{
   return getDuration() / mFabs(timeScale);
}

F32 TSThread::getTimeScale()
{
   return timeScale;
}

void TSThread::setTimeScale(F32 ts)
{
   timeScale = ts;
}

void TSThread::advancePos(F32 delta)
{
   if (mFabs(delta)>0.00001f)
   {
      // make dirty what this thread changes
      U32 dirtyFlags = sequence->dirtyFlags | (transitionData.inTransition ? TSShapeInstance::TransformDirty : 0);
      for (S32 i=0; i<mShapeInstance->getShape()->subShapeFirstNode.size(); i++)
         mShapeInstance->mDirtyFlags[i] |= dirtyFlags;
   }

   if (transitionData.inTransition)
   {
      transitionData.pos += transitionData.direction * delta;
      if (transitionData.pos<0 || transitionData.pos>=1.0f)
      {
         mShapeInstance->clearTransition(this);
         if (transitionData.pos<0.0f)
            // return to old sequence
            mShapeInstance->setSequence(this,transitionData.oldSequence,transitionData.oldPos);
      }
      // re-adjust delta to be correct time-wise
      delta *= transitionData.targetScale * transitionData.duration / sequence->duration;
   }

   // even if we are in a transition, keep playing the sequence

   if (makePath)
   {
      path.start = pos;
      pos += delta;
      if (!sequence->isCyclic())
      {
         pos = mClampF(pos , 0.0f, 1.0f);
         path.loop = 0;
      }
      else
      {
         path.loop = (S32)pos;
         if (pos < 0.0f)
            path.loop--;
         pos -= path.loop;
         // following necessary because of floating point roundoff errors
         if (pos < 0.0f) pos += 1.0f;
         if (pos >= 1.0f) pos -= 1.0f;
      }
      path.end = pos;

      animateTriggers(); // do this automatically...no need for user to call it

      AssertFatal(pos>=0.0f && pos<=1.0f,"TSThread::advancePos (1)");
      AssertFatal(!sequence->isCyclic() || pos<1.0f,"TSThread::advancePos (2)");
   }
   else
   {
      pos += delta;
      if (!sequence->isCyclic())
         pos = mClampF(pos, 0.0f, 1.0f);
      else
      {
         pos -= S32(pos);
         // following necessary because of floating point roundoff errors
         if (pos < 0.0f) pos += 1.0f;
         if (pos >= 1.0f) pos -= 1.0f;
      }
      AssertFatal(pos>=0.0f && pos<=1.0f,"TSThread::advancePos (3)");
      AssertFatal(!sequence->isCyclic() || pos<1.0f,"TSThread::advancePos (4)");
   }

   // select keyframes
   selectKeyframes(pos,sequence,&keyNum1,&keyNum2,&keyPos);
}

void TSThread::advanceTime(F32 delta)
{
   advancePos(timeScale * delta / getDuration());
}

void TSThread::setPos(F32 pos)
{
   advancePos(pos-getPos());
}

void TSThread::setTime(F32 time)
{
   setPos(timeScale * time/getDuration());
}

S32 TSThread::getKeyframeCount()
{
   AssertFatal(!transitionData.inTransition,"TSThread::getKeyframeCount: not while in transition");

   return sequence->numKeyframes + 1;
}

S32 TSThread::getKeyframeNumber()
{
   const TSShape * shape = mShapeInstance->mShape;

   AssertFatal(!transitionData.inTransition,"TSThread::getKeyframeNumber: not while in transition");

   return keyNum1;
}

void TSThread::setKeyframeNumber(S32 kf)
{
   const TSShape * shape = mShapeInstance->mShape;

   AssertFatal(kf>=0 && kf<= sequence->numKeyframes,
      "TSThread::setKeyframeNumber: invalid frame specified.");
   AssertFatal(!transitionData.inTransition,"TSThread::setKeyframeNumber: not while in transition");

   keyNum1 = keyNum2 = kf;
   keyPos = 0;
   pos = 0;
}

TSThread::TSThread(TSShapeInstance * _shapeInst)
{
   timeScale = 1.0f;
   mShapeInstance = _shapeInst;
   transitionData.inTransition = false;
   blendDisabled = false;
   setSequence(0,0.0f);
}

S32 TSThread::operator<(const TSThread & th2) const
{
   if (sequence->isBlend() == th2.sequence->isBlend())
   {
      // both blend or neither blend, sort based on priority only -- higher priority first
      S32 ret = 0; // do it this way to (hopefully) take advantage of 'conditional move' assembly instruction
      if (priority > th2.priority)
         ret = -1;
      if (th2.priority > priority)
         ret = 1;
      return ret;
   }
   else
   {
      // one is blend, the other is not...sort based on blend -- non-blended first
      AssertFatal(!sequence->isBlend() || !th2.sequence->isBlend(),"compareThreads: unequal 'trues'");

      S32 ret = -1; // do it this way to (hopefully) take advantage of 'conditional move' assembly instruction
      if (sequence->isBlend())
         ret = 1;
      return ret;
   }
}


//-------------------------------------------------------------------------------------
// TSShapeInstance Thread Interface -- more implemented in header file
//-------------------------------------------------------------------------------------

TSThread * TSShapeInstance::addThread()
{
   if (mShape->sequences.empty())
      return NULL;

   mThreadList.increment();
   mThreadList.last() = new TSThread(this);
   setDirty(AllDirtyMask);
   return mThreadList.last();
}

TSThread * TSShapeInstance::getThread(S32 threadNumber)
{
   AssertFatal(threadNumber < mThreadList.size() && threadNumber>=0,"TSShapeInstance::getThread: threadNumber out of bounds.");
   return mThreadList[threadNumber];
}

void TSShapeInstance::destroyThread(TSThread * thread)
{
   if (!thread)
      return;

   clearTransition(thread);

   S32 i;
   for (i=0; i<mThreadList.size(); i++)
      if (thread==mThreadList[i])
         break;

   AssertFatal(i<mThreadList.size(),"TSShapeInstance::destroyThread was requested to destroy a thread that this instance doesn't own!");

   delete mThreadList[i];
   mThreadList.erase(i);
   setDirty(AllDirtyMask);
   checkScaleCurrentlyAnimated();
}

U32 TSShapeInstance::threadCount()
{
   return mThreadList.size();
}

void TSShapeInstance::setSequence(TSThread * thread, S32 seq, F32 pos)
{
   if ( (thread->transitionData.inTransition && mTransitionThreads.size()>1) || mTransitionThreads.size()>0)
   {
      // if we have transitions, make sure transforms are up to date...
      animateNodeSubtrees();
   }

   thread->setSequence(seq,pos);
   setDirty(AllDirtyMask);
   mGroundThread = NULL;

   if (mScaleCurrentlyAnimated && !thread->sequence->animatesScale())
      checkScaleCurrentlyAnimated();
   else if (!mScaleCurrentlyAnimated && thread->sequence->animatesScale())
      mScaleCurrentlyAnimated=true;

   updateTransitions();
}

U32 TSShapeInstance::getSequence(TSThread * thread)
{
   AssertFatal( (thread->sequence - mShape->sequences.address())>=0,
      "TSShapeInstance::getSequence: range error A");
   AssertFatal( (thread->sequence - mShape->sequences.address())<mShape->sequences.size(),
      "TSShapeInstance::getSequence: range error B");
   return (U32) (thread->sequence - mShape->sequences.address());
}

void TSShapeInstance::transitionToSequence(TSThread * thread, S32 seq, F32 pos, F32 duration, bool continuePlay)
{
   // make sure all transforms on all detail levels are accurate
   animateNodeSubtrees();

   thread->transitionToSequence(seq,pos,duration,continuePlay);
   setDirty(AllDirtyMask);
   mGroundThread = NULL;

   if (mScaleCurrentlyAnimated && !thread->sequence->animatesScale())
      checkScaleCurrentlyAnimated();
   else if (!mScaleCurrentlyAnimated && thread->sequence->animatesScale())
      mScaleCurrentlyAnimated=true;

   mTransitionRotationNodes.overlap(thread->transitionData.oldRotationNodes);
   mTransitionRotationNodes.overlap(thread->sequence->rotationMatters);

   mTransitionTranslationNodes.overlap(thread->transitionData.oldTranslationNodes);
   mTransitionTranslationNodes.overlap(thread->sequence->translationMatters);

   mTransitionScaleNodes.overlap(thread->transitionData.oldScaleNodes);
   mTransitionScaleNodes.overlap(thread->sequence->scaleMatters);

   // if we aren't already in the list of transition threads, add us now
   S32 i;
   for (i=0; i<mTransitionThreads.size(); i++)
      if (mTransitionThreads[i]==thread)
         break;
   if (i==mTransitionThreads.size())
      mTransitionThreads.push_back(thread);

   updateTransitions();
}

void TSShapeInstance::clearTransition(TSThread * thread)
{
   if (!thread->transitionData.inTransition)
      return;

   // if other transitions are still playing,
   // make sure transforms are up to date
   if (mTransitionThreads.size()>1)
      animateNodeSubtrees();

   // turn off transition...
   thread->transitionData.inTransition = false;

   // remove us from transition list
   S32 i;
   if (mTransitionThreads.size() != 0) {
      for (i=0; i<mTransitionThreads.size(); i++)
	 if (mTransitionThreads[i]==thread)
	    break;
      AssertFatal(i!=mTransitionThreads.size(),"TSShapeInstance::clearTransition");
      mTransitionThreads.erase(i);
   }

   // recompute transitionNodes
   mTransitionRotationNodes.clearAll();
   mTransitionTranslationNodes.clearAll();
   mTransitionScaleNodes.clearAll();
   for (i=0; i<mTransitionThreads.size(); i++)
   {
      mTransitionRotationNodes.overlap(mTransitionThreads[i]->transitionData.oldRotationNodes);
      mTransitionRotationNodes.overlap(mTransitionThreads[i]->sequence->rotationMatters);

      mTransitionTranslationNodes.overlap(mTransitionThreads[i]->transitionData.oldTranslationNodes);
      mTransitionTranslationNodes.overlap(mTransitionThreads[i]->sequence->translationMatters);

      mTransitionScaleNodes.overlap(mTransitionThreads[i]->transitionData.oldScaleNodes);
      mTransitionScaleNodes.overlap(mTransitionThreads[i]->sequence->scaleMatters);
   }

   setDirty(ThreadDirty);

   updateTransitions();
}

void TSShapeInstance::updateTransitions()
{
   if (mTransitionThreads.empty())
      return;

   S32 i;
   mNodeReferenceRotations.setSize(mShape->nodes.size());
   mNodeReferenceTranslations.setSize(mShape->nodes.size());
   for (i=0; i<mShape->nodes.size(); i++)
   {
      if (mTransitionRotationNodes.test(i))
         mNodeReferenceRotations[i].set(smNodeCurrentRotations[i]);
      if (mTransitionTranslationNodes.test(i))
         mNodeReferenceTranslations[i] = smNodeCurrentTranslations[i];
   }

   if (animatesScale())
   {
      if (animatesUniformScale())
      {
         mNodeReferenceUniformScales.setSize(mShape->nodes.size());
         for (i=0; i<mShape->nodes.size(); i++)
         {
            if (mTransitionScaleNodes.test(i))
               mNodeReferenceUniformScales[i] = smNodeCurrentUniformScales[i];
         }
      }
      else if (animatesAlignedScale())
      {
         mNodeReferenceScaleFactors.setSize(mShape->nodes.size());
         for (i=0; i<mShape->nodes.size(); i++)
         {
            if (mTransitionScaleNodes.test(i))
               mNodeReferenceScaleFactors[i] = smNodeCurrentAlignedScales[i];
         }
      }
      else
      {
         mNodeReferenceScaleFactors.setSize(mShape->nodes.size());
         mNodeReferenceArbitraryScaleRots.setSize(mShape->nodes.size());
         for (i=0; i<mShape->nodes.size(); i++)
         {
            if (mTransitionScaleNodes.test(i))
            {
               mNodeReferenceScaleFactors[i] = smNodeCurrentArbitraryScales[i].mScale;
               mNodeReferenceArbitraryScaleRots[i].set(smNodeCurrentArbitraryScales[i].mRotate);
            }
         }
      }
   }

   // reset transition durations to account for new reference transforms
   for (i=0; i<mTransitionThreads.size(); i++)
   {
      TSThread * th = mTransitionThreads[i];
      if (th->transitionData.inTransition)
      {
         th->transitionData.duration *= 1.0f - th->transitionData.pos;
         th->transitionData.pos = 0.0f;
      }
   }
}

void TSShapeInstance::checkScaleCurrentlyAnimated()
{
   mScaleCurrentlyAnimated=true;
   for (S32 i=0; i<mThreadList.size(); i++)
      if (mThreadList[i]->sequence->animatesScale())
         return;
   mScaleCurrentlyAnimated=false;
}

void TSShapeInstance::setBlendEnabled(TSThread * thread, bool blendOn)
{
   thread->blendDisabled = !blendOn;
}

bool TSShapeInstance::getBlendEnabled(TSThread * thread)
{
   return !thread->blendDisabled;
}

F32 TSShapeInstance::getTime(TSThread * thread)
{
   return thread->getTime();
}

F32 TSShapeInstance::getPos(TSThread * thread)
{
   return thread->getPos();
}

void TSShapeInstance::setTime(TSThread * thread, F32 time)
{
   thread->setTime(time);
}

void TSShapeInstance::setPos(TSThread * thread, F32 pos)
{
   thread->setPos(pos);
}

bool TSShapeInstance::isInTransition(TSThread * thread)
{
   return thread->isInTransition();
}

F32 TSShapeInstance::getTimeScale(TSThread * thread)
{
   return thread->getTimeScale();
}

void TSShapeInstance::setTimeScale(TSThread * thread, F32 timeScale)
{
   thread->setTimeScale(timeScale);
}

F32 TSShapeInstance::getDuration(TSThread * thread)
{
   return thread->getDuration();
}

F32 TSShapeInstance::getScaledDuration(TSThread * thread)
{
   return thread->getScaledDuration();
}

S32 TSShapeInstance::getKeyframeCount(TSThread * thread)
{
   return thread->getKeyframeCount();
}

S32 TSShapeInstance::getKeyframeNumber(TSThread * thread)
{
   return thread->getKeyframeNumber();
}

void TSShapeInstance::setKeyframeNumber(TSThread * thread, S32 kf)
{
   thread->setKeyframeNumber(kf);
}


// advance time on a particular thread
void TSShapeInstance::advanceTime(F32 delta, TSThread * thread)
{
   thread->advanceTime(delta);
}

// advance time on all threads
void TSShapeInstance::advanceTime(F32 delta)
{
   for (S32 i=0; i<mThreadList.size(); i++)
      mThreadList[i]->advanceTime(delta);
}

// advance pos on a particular thread
void TSShapeInstance::advancePos(F32 delta, TSThread * thread)
{
   thread->advancePos(delta);
}

// advance pos on all threads
void TSShapeInstance::advancePos(F32 delta)
{
   for (S32 i=0; i<mThreadList.size(); i++)
      mThreadList[i]->advancePos(delta);
}

