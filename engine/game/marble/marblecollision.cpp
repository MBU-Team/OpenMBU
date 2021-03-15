//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

#include "materials/material.h"
#include "math/mathUtils.h"

//----------------------------------------------------------------------------

static Box3F sgLastCollisionBox;
static U32 sgLastCollisionMask;
static bool sgResetFindObjects;
static U32 sgCountCalls;

void Marble::clearObjectsAndPolys()
{
    sgResetFindObjects = true;
	sgLastCollisionBox.min.set(0, 0, 0);
	sgLastCollisionBox.max.set(0, 0, 0);
}

bool Marble::pointWithinPoly(const ConcretePolyList::Poly& poly, const Point3F& point)
{
    // TODO: Implement pointWithinPoly
    return false;
}

bool Marble::pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir)
{
    // TODO: Implement pointWithinPolyZ
    return false;
}

void Marble::findObjectsAndPolys(U32 collisionMask, const Box3F& testBox, bool testPIs)
{
    if (collisionMask != sgLastCollisionMask || !sgLastCollisionBox.isContained(testBox) || sgResetFindObjects || !smPathItrVec.empty())
    {
        ++sgCountCalls;
		if (sgResetFindObjects || !smPathItrVec.empty())
		{
			sgLastCollisionBox.min = testBox.min - 0.5f;
			sgLastCollisionBox.max = testBox.max + 0.5f;
		} else
		{
			sgLastCollisionBox.min.setMin(testBox.min - 0.5f);
		    sgLastCollisionBox.max.setMax(testBox.max + 0.5f);
		}

		Point3D pos = (sgLastCollisionBox.max + sgLastCollisionBox.min) * 0.5f;
		Point3F test = sgLastCollisionBox.max - sgLastCollisionBox.min;
		SphereF sphere(pos, test.len() * 0.5f);
		
		static SimpleQueryList sql;
		sql.mList.clear();
		mContainer->findObjects(sgLastCollisionBox, collisionMask, SimpleQueryList::insertionCallback, &sql);
		polyList.clear();
		marbles.clear();

		for (S32 i = 0; i < sql.mList.size(); i++)
		{
		    SceneObject* obj = sql.mList[i];

		    if ((sql.mList[i]->getTypeMask() & PlayerObjectType) == 0)
		    {
				if (testPIs || !dynamic_cast<PathedInterior*>(obj))
				    obj->buildPolyList(&polyList, sgLastCollisionBox, sphere);
		    } else if (obj != this)
		    {
		        marbles.push_back(reinterpret_cast<Marble*>(obj));
		    }
		}
    }
}

bool Marble::testMove(Point3D velocity, Point3D& position, F64& deltaT, F64 radius, U32 collisionMask, bool testPIs)
{
	F64 velLen = velocity.len();
    if (velocity.len() < 0.001)
	    return false;

	Point3D velocityDir = velocity * (1.0 / velLen);

	Point3D deltaVelocity = velocity * deltaT;
	Point3D finalPosition = position + deltaVelocity;

	// If there is a collision mask
	if (collisionMask != 0)
	{
		// TODO: Finish Implementing testMove

		if (deltaVelocity.x >= 0.0)
		{
			// TODO: Finish Implementing testMove
		} else
		{
			// TODO: Finish Implementing testMove
		}

		if (deltaVelocity.y >= 0.0)
		{
			// TODO: Finish Implementing testMove
		}
		else
		{
			// TODO: Finish Implementing testMove
		}

		if (deltaVelocity.z >= 0.0)
		{
			// TODO: Finish Implementing testMove
		}
		else
		{
			// TODO: Finish Implementing testMove
		}

		// TODO: Finish Implementing testMove
	}
	
	F64 finalT = deltaT;
	F64 marbleCollisionTime = finalT;
	Point3F marbleCollisionNormal(0.0f, 0.0f, 1.0f);

	// Marble on Marble collision
	if ((collisionMask & PlayerObjectType) != 0)
	{
		// TODO: Finish Implementing testMove
	}

	S32 iter = 0;
	// Marble on Platform collision
	if (!polyList.mPolyList.empty())
	{
		// TODO: Finish Implementing testMove
	}

	// TODO: Finish Implementing testMove
	
    position = finalPosition;

	// TODO: Finish Implementing testMove

    return false;
}

void Marble::findContacts(U32 contactMask, const Point3D* inPos, const F32* inRad)
{
    mContacts.clear();

    static Vector<Marble::MaterialCollision> materialCollisions;
    materialCollisions.clear();

    F32 rad;
    Box3F objBox;
    if (inRad != NULL && inPos != NULL)
    {
        rad = *inRad;

        objBox.min = Point3F(-rad, -rad, -rad);
        objBox.max = Point3F(rad, rad, rad);
    } else
    {
        // If either is null, both should be null
        inRad = NULL;
        inPos = NULL;
    }

    const Point3D* pos = inPos;
    if (inPos == NULL)
        pos = &mPosition;
		
    if (inRad == NULL)
    {
        rad = mRadius;
        objBox = mObjBox;
    } else
    {
      rad = *inRad;  
    }
    
    if (contactMask != 0)
    {
        Box3F extrudedMarble;
        extrudedMarble.min = objBox.min + *pos - 0.0001f;
        extrudedMarble.max = objBox.max + *pos + 0.0001f;

        findObjectsAndPolys(contactMask, extrudedMarble, true);
    }

    if ((contactMask & PlayerObjectType) != 0)
    {
        for (S32 i = 0; i < marbles.size(); i++)
        {
            Marble* otherMarble = marbles[i];

			Point3F otherDist = otherMarble->getPosition() - *pos;

			F32 otherRadius = otherMarble->mRadius + rad;
			if (otherRadius * otherRadius * 1.01f > mDot(otherDist, otherDist))
			{
			    Point3F normDist = otherDist;
				m_point3F_normalize(normDist);

				mContacts.increment();
				Contact* marbleContact = &mContacts[mContacts.size() - 1];

				memcpy(marbleContact->surfaceVelocity, otherMarble->getVelocityD(), sizeof(marbleContact->surfaceVelocity));

				marbleContact->material = 0;
				marbleContact->object = otherMarble;
				marbleContact->position = *pos + normDist;
				marbleContact->normal = -*pos;
				marbleContact->contactDistance = rad;
				marbleContact->friction = 1.0f;
				marbleContact->restitution = 1.0f;
				marbleContact->force = 0.0f;

				queueCollision(otherMarble, mVelocity - otherMarble->getVelocity(), 0);
			}
        }
    }
    
	for (int i = 0; i < polyList.mPolyList.size(); i++)
	{
		ConcretePolyList::Poly* poly = &polyList.mPolyList[i];
		PlaneD plane(poly->plane);
		F64 distance = plane.distToPlane(*pos);
		if (mFabsD(distance) <= (F64)rad + 0.0001) {
			Point3D lastVertex(polyList.mVertexList[polyList.mIndexList[poly->vertexStart + poly->vertexCount - 1]]);

			Point3D contactVert = plane.project(*pos);
			F64 separation = mSqrtD(rad * rad - distance * distance);

			for (int j = 0; j < poly->vertexCount; j++) {
				Point3D vertex = polyList.mVertexList[polyList.mIndexList[poly->vertexStart + j]];
				if (vertex != lastVertex) {
					PlaneD vertPlane(vertex + plane, vertex, lastVertex);
					F64 vertDistance = vertPlane.distToPlane(contactVert);
					if (vertDistance < 0.0) {
						if (vertDistance < -(separation + 0.0001))
							goto superbreak;

						if (PlaneD(vertPlane + vertex, vertex, vertex + plane).distToPlane(contactVert) >= 0.0) {
							if (PlaneD(lastVertex - vertPlane, lastVertex, lastVertex + plane).distToPlane(contactVert) >= 0.0) {
								contactVert = vertPlane.project(contactVert);
								break;
							}
							contactVert = lastVertex;
						}
						else {
							contactVert = vertex;
						}
					}
					lastVertex = vertex;
				}
			}

			// Todo: Implement getMaterial in ShapeBase, TerrainBlock, and InteriorInstance
			Material* matProp = poly->object->getMaterial(poly->material);

			PathedInterior* hitPI = dynamic_cast<PathedInterior*>(poly->object);

			Point3D surfaceVelocity;
			if (hitPI != nullptr) {
				surfaceVelocity = hitPI->getVelocity();
			}
			else {
				surfaceVelocity = Point3D(0, 0,  0);
			}

			U32 materialId = poly->material;
			Point3D delta = *pos - contactVert;
			F64 contactDistance = delta.len();
			if ((F64)rad + 0.0001 < contactDistance) {
				continue;
			}

			Point3D normal(plane.x, plane.y, plane.z);
			if (contactDistance != 0.0) {
				normal = delta * (1.0 / contactDistance);
			}
			F32 force = 0.0;
			F32 friction = 1.0;
			F32 restitution = 1.0;
			if (matProp != nullptr) {
				friction = matProp->friction;
				restitution = matProp->restitution;
				force = matProp->force;
			}

			Marble::Contact contact{};

			contact.restitution = restitution;
			contact.normal = normal;
			contact.position = contactVert;
			contact.surfaceVelocity = surfaceVelocity;
			contact.object = poly->object;
			contact.contactDistance = contactDistance;
			contact.friction = friction;
			contact.force = force;
			contact.material = materialId;

			mContacts.push_back(contact);

			GameBase* gb = dynamic_cast<GameBase*>(poly->object);
			U32 objTypeMask = 0;
			if (gb != nullptr) {
				objTypeMask = gb->getTypeMask();
			}
			
			if ((objTypeMask & ShapeBaseObjectType) != 0) {
				U32 netIndex = gb->getNetIndex();

				bool found = false;
				for (int j = 0; j < materialCollisions.size(); j++) {
					if (materialCollisions[j].ghostIndex == netIndex && materialCollisions[j].materialId == materialId) {
						found = true;
						break;
					}
				}

				if (!found) {

					Marble::MaterialCollision coll{};
					coll.ghostIndex = netIndex;
					coll.materialId = materialId;
					coll.object = NULL;
					materialCollisions.push_back(coll);
					Point3F offset(0, 0, 0);
					queueCollision(reinterpret_cast<ShapeBase*>(gb), offset, materialId);
				}
			}
		}
	superbreak:
		(void)0;
	}
}

void Marble::computeFirstPlatformIntersect(F64& dt, Vector<PathedInterior*>& pitrVec)
{
    // TODO: Implement computeFirstPlatformIntersect
}

void Marble::resetObjectsAndPolys(U32 collisionMask, const Box3F& testBox)
{
    sgLastCollisionBox.min.set(0, 0, 0);
    sgLastCollisionBox.max.set(0, 0, 0);

    sgResetFindObjects = 1;
    sgCountCalls = 0;

    if (smPathItrVec.empty())
        findObjectsAndPolys(collisionMask, testBox, false);
}
