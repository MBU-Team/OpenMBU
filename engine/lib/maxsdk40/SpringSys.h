/**********************************************************************
 *<
	FILE:			SpringSys.h

	DESCRIPTION:	Public header files for the Spring System created by Adam Felt.
					To use this interface you will need to link to "SpringSys.lib"

					This spring system is a multi-point spring-based dynamic system.
					In order to your the sytem you must derive from SpringSysClient and implement
					Tab<Matrix3>	SpringSysClient::GetForceMatrices(TimeValue t)
					You are responsible for gathering the spring constraint forces at any given time.
					You initialize the 

	CREATED BY:		Adam Felt

	HISTORY: 

 *>	Copyright (c) 1999-2000, All Rights Reserved.
 **********************************************************************/

#include "max.h"

#ifndef AF_SPRINGSYS
#define AF_SPRINGSYS

#ifndef SpringSysExport
#	ifdef BLD_SPRING_SYS
#		define SpringSysExport __declspec( dllexport )
#	else
#		define SpringSysExport __declspec( dllimport )
#	endif
#endif

//A Controlled Particle Class
//Used to constrain a point to an object
class SSConstraintPoint  
{						
	public:
		int index;		//this bone index is used to identify the bone.  
						//Usually refers to a reference index, or paramblock index...
		Point3 pos;		//The control nodes stored position
		Point3 vel;		//The control nodes stored velocity

		SSConstraintPoint() {index = -1; pos = Point3(0.0f, 0.0f, 0.0f); vel = Point3(0.0f, 0.0f, 0.0f);}
		SSConstraintPoint(int id) {index = id; pos = Point3(0.0f, 0.0f, 0.0f); vel = Point3(0,0,0);}
		~SSConstraintPoint() {}
		SSConstraintPoint& operator=(const SSConstraintPoint& from)
		{
			index = from.index;
			pos = from.pos;
			vel = from.vel;
			
			return *this;
		}

		SSConstraintPoint Copy(const SSConstraintPoint from)
		{
			index = from.index;
			pos = from.pos;
			vel = from.vel;
			
			return *this;
		}
		
		int GetIndex() { return index; }
		void SetIndex(int id) { index = id; }
		Point3 GetPos() { return pos; }
		void SetPos(Point3 p) { pos = p; }
		Point3 GetVel() { return vel; }
		void SetVel(Point3 v) { vel = v; }
};

//Class used to store a particles state
class SSParticleCache
{
	public:
		Point3 pos;
		Point3 vel;
		Tab<SSConstraintPoint> bone;

		SSParticleCache() {pos = Point3(0,0,0); vel = Point3(0,0,0); bone.Init(); }
};

//A Spring Class
//Stores the parameters for binding an object to any force with a controllable spring
class SSSpring : public BaseInterfaceServer
{
	private:
		float tension;     
		float dampening;
		Point3 length;		//rest length
		SSConstraintPoint bone;       
		//AFParticle *p2;   //for use later to apply the reactive force

	public:

		SSSpring() 
		{
			bone = NULL;
			length = Point3(0.0f, 0.0f, 0.0f);
			tension = 1.0f;
			dampening = 0.5f;
		}

		SSSpring(SSConstraintPoint *b, Point3 l, float t=2.0f, float d=1.0f)
		{
			bone = *b;
			length = l;
			tension = t;
			dampening = d;
		}

		SSSpring& operator=(const SSSpring& from)
		{
			tension = from.tension;
			dampening = from.dampening;
			length = from.length;
			bone = from.bone;

			return *this;
		}

		SSSpring Copy(const SSSpring from)
		{
			tension = from.tension;
			dampening = from.dampening;
			length = from.length;
			bone = from.bone;

			return *this;
		}

		float GetTension() {return tension;}
		void SetTension(float t) {tension = t;}
		float GetDampening() {return dampening;}
		void SetDampening(float d) {dampening = d;}
		Point3 GetLength() {return length;}
		void SetLength(Point3 len) {length = len;}

		SSConstraintPoint* GetPointConstraint() { return &bone;}
		void SetPointConstraint(SSConstraintPoint b) { bone = b; }
		void SetPointConstraint(int id, Point3 pos, Point3 vel) 
		{
			bone.SetIndex(id);
			bone.SetPos(pos);
			bone.SetVel(vel);
		}

};

class SSParticle
{
	friend class SpringSys;

	private:
		float mass;		//the particle's mass
		float drag;		//the particle's drag coefficient
		Point3 pos;		//particle position
		Point3 vel;		//particle velocity
		Point3 force;   //Force accumulator
		Tab<SSSpring> springs;

	public:
		SSParticle() 
		{
			mass = 300.0f; 
			drag = 1.0f; 
			pos = vel = force = Point3(0.0f,0.0f,0.0f); 
			springs.ZeroCount();
		}
		~SSParticle(){}

		SSParticle& operator=(const SSParticle& from)
		{
			mass = from.mass;
			drag = from.drag;
			pos = from.pos;
			vel = from.vel;
			force = from.force;
			springs.ZeroCount();
			springs.Shrink();
			for (int i=0;i<from.springs.Count();i++)
			{
				SSSpring spring = from.springs[i];
				springs.Append(1, &spring);
			}
			return *this;
		}

		SSParticle Copy(const SSParticle from)
		{
			SSSpring spring;
			mass = from.mass;
			drag = from.drag;
			pos = from.pos;
			vel = from.vel;
			force = from.force;
			springs.ZeroCount();
			for (int i=0;i<from.springs.Count();i++)
			{
				spring.Copy(from.springs[i]);
				springs.Append(1, &spring);
			}
			return *this;
		}

		float GetMass() {return mass;}
		void SetMass(float m) {mass = m;}
		float GetDrag() {return drag;}
		void SetDrag(float d) {drag = d;}
		Point3 GetPos() {return pos;}
		void SetPos(Point3 p) {pos = p;}
		Point3 GetVel() {return vel;}
		void SetVel(Point3 v) {vel = v;}
		Point3 GetForce() {return force;}
		void SetForce(Point3 f) {force = f;}
		Tab<SSSpring>* GetSprings() { return &springs;}
		SSSpring* GetSpring(int i) {if (i>=0 && i<springs.Count()) return &(springs[i]);
									else return NULL; }
		void SetSpring(int i, SSSpring spring) {if (i>=0 && i<springs.Count()) 
													springs[i] = spring; }
		void SetSprings(Tab<SSSpring> sTab) 
		{ 
			//springs = NULL;
			springs.ZeroCount(); 
			for (int i = 0;i<sTab.Count();i++)
			{
				springs.Append(1, &(sTab[i]));
			} 
		}
		
		BOOL AddSpring(SSConstraintPoint *bone, Point3 length, float tension=2.0f, float dampening = 1.0f)
		{
			for (int i=0;i<springs.Count();i++)
			{
				//if it already exists in the list make it stronger
				if ( springs[i].GetPointConstraint()->GetIndex() == bone->GetIndex() )
				{ springs[i].SetTension( springs[i].GetTension()*2 ); return false; }
			}
			//if it doesn't exist, add a new one
			SSSpring *spring = new SSSpring(bone, length, tension, dampening);
			springs.Append(1, spring);
			return true;
		}

		void DeleteSpring(int index) 
		{		
			if ( index == 0 ) return;
			for (int i=0;i<springs.Count();i++)
			{
				if ( (springs[i].GetPointConstraint()->GetIndex()) == index)
					springs.Delete(i--, 1);
				else if (springs[i].GetPointConstraint()->GetIndex() > index)
					springs[i].GetPointConstraint()->SetIndex(springs[i].GetPointConstraint()->GetIndex()-1);
			}
		}

};

class SpringSysClient 
{
	public:
		virtual Tab<Matrix3>	GetForceMatrices(TimeValue t)=0;
		virtual Point3			GetDynamicsForces(TimeValue t, Point3 pos, Point3 vel) {return Point3(0,0,0);}
};


class SpringSys : public BaseInterfaceServer
{
	private:
		float referenceTime;
		float lastTime;
		bool isValid;
		
		SSParticleCache frameCache;
		Tab<Point3> pos_cache;
		Tab<Point3> vel_cache;

		Tab<Point3> initPosTab;
		Tab<SSParticle> parts;		//particles

		SpringSysClient* client;

	public:
		
		SpringSys() 
		{
			client = NULL; 
			referenceTime = lastTime = 0.0f; 
			SetParticleCount(1);
			isValid = false; 
		}

		SpringSys(SpringSysClient* c, int count) 
		{ 
			client = c; 
			referenceTime = lastTime = 0.0f; 
			SetParticleCount(count);
			isValid = false; 
		}

		~SpringSys() {}
		SpringSysExport SpringSys& operator=(const SpringSys& from);

		SpringSysExport SpringSys Copy(const SpringSys* from);

		void SetReferenceTime (float t) { referenceTime = t; }
		float GetReferenceTime () { return referenceTime; }
		Tab<SSParticle>* GetParticles() { return &parts;}
		SSParticle* GetParticle(int i) { if (i >=0 && i< parts.Count())	return &(parts[i]);
											else return NULL; }
		SpringSysExport void SetParticleCount(int count);
		SpringSysExport void SetInitialPosition (Point3 p, int partIndex);
		SpringSysExport void SetInitialVelocity (Point3 p, int partIndex);
		SpringSysExport void SetInitialBoneStates(Tab<Matrix3> boneTMs);
		SpringSysExport void Invalidate ();
		SpringSysExport void Solve (int time, float TimeDelta);
		SpringSysExport void GetPosition (Point3& p, int index);

		SpringSysExport IOResult Load(ILoad *iload);
		SpringSysExport IOResult Save(ISave *isave);

	protected:
		float GetTime() { return lastTime; }
		void SetTime(float t) { lastTime = t; }

		//force functions
		SpringSysExport void Clear_Forces(int index);
		SpringSysExport void Compute_Forces(TimeValue t, int index);
		SpringSysExport void ApplyDrag(int index);
		SpringSysExport void ApplyUnaryForces(TimeValue t, int index); 
		SpringSysExport void ComputeControlledParticleForce(Matrix3 tm, int vertIndex, int springIndex);
		SpringSysExport void ApplySpring(TimeValue t, int index);

		//Solver functions
		SpringSysExport void UpdateParticleState(TimeValue t, Tab<Matrix3> tmArray, int pIndex, TimeValue Delta);
		SpringSysExport void ComputeDerivative(int index, Point3 &pos, Point3 &vel);
		SpringSysExport void GetParticleState(int index, Point3 &pos, Point3 &vel);
		SpringSysExport void SetParticleState(int index, Point3 pos, Point3 vel); 
		SpringSysExport void ScaleVectors(Point3 &pos, Point3 &vel, float delta);
		SpringSysExport void AddVectors(Point3 pos1, Point3 vel1, Point3 &pos, Point3 &vel);
};

#endif //AF_SPRINGSYS
