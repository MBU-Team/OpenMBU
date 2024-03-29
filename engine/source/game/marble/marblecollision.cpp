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
    if (poly.vertexCount == 0)
        return true;

    Point3F lastVert = Marble::polyList.mVertexList[Marble::polyList.mIndexList[poly.vertexStart + poly.vertexCount - 1]];

    for (int i = 0; i < poly.vertexCount; i++)
    {
        Point3F& v = Marble::polyList.mVertexList[Marble::polyList.mIndexList[i + poly.vertexStart]];
        PlaneF p(v + poly.plane, v, lastVert);
        lastVert = v;
        if (p.distToPlane(point) < 0.0f)
            return false;
    }
    return true;
}

bool Marble::pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir)
{
    if (poly.vertexCount == 0)
        return true;

    Point3F lastVert = Marble::polyList.mVertexList[Marble::polyList.mIndexList[poly.vertexStart + poly.vertexCount - 1]];
    
    for (int i = 0; i < poly.vertexCount; i++)
    {
        Point3F& v = Marble::polyList.mVertexList[Marble::polyList.mIndexList[i + poly.vertexStart]];
        PlaneF p(v + upDir, v, lastVert);
        lastVert = v;
        if (p.distToPlane(point) < -0.003f)
            return false;
    }
    return true;
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

        sgLastCollisionMask = collisionMask;
        sgResetFindObjects = false;

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

	Point3D deltaPosition = velocity * deltaT;
	Point3D finalPosition = position + deltaPosition;

	// If there is a collision mask
	if (collisionMask != 0)
	{
        // Create a Bounding Box and expand it to include the final position
        Box3F box(mObjBox.min + position - 0.5f, mObjBox.max + position + 0.5f);

		if (deltaPosition.x >= 0.0)
            box.max.x += deltaPosition.x;
		else
            box.min.x += deltaPosition.x;

		if (deltaPosition.y >= 0.0)
            box.max.y += deltaPosition.y;
		else
            box.min.y += deltaPosition.y;

		if (deltaPosition.z >= 0.0)
			box.max.z += deltaPosition.z;
		else
            box.min.z += deltaPosition.z;

        findObjectsAndPolys(collisionMask, box, testPIs);
	}
	
	F64 finalT = deltaT;
	F64 marbleCollisionTime = finalT;
	Point3F marbleCollisionNormal(0.0f, 0.0f, 1.0f);

    Point3D lastContactPos;

    ConcretePolyList::Poly* contactPoly;

	// Marble on Marble collision
	if ((collisionMask & PlayerObjectType) != 0)
	{
        Point3F nextPos = position + deltaPosition;
        
        for (S32 i = 0; i < marbles.size(); i++)
        {
            Marble* other = marbles[i];

            Point3F otherPos = other->getPosition();

            F32 time;
            if (MathUtils::capsuleSphereNearestOverlap(position, nextPos, mRadius, otherPos, other->mRadius, time))
            {
                time *= deltaT;

                if (time >= finalT)
                {
                    Point3D vel = finalPosition - otherPos;
                    m_point3D_normalize(vel);

                    Point3D velD = other->getVelocityD();
                    F64 newVelLen = (mVelocity.x - velD.x) * vel.x
                                  + (mVelocity.y - velD.y) * vel.y
                                  + (mVelocity.z - velD.z) * vel.z;

                    if (newVelLen < 0.0)
                    {
                        finalT = time;
                        marbleCollisionTime = time;

                        Point3F posDiff = (nextPos - position) * time;
                        Point3F p = posDiff + position;
                        finalPosition = p;
                        marbleCollisionNormal = p - otherPos;
                        m_point3F_normalize(marbleCollisionNormal);

                        contactPoly = NULL;

                        lastContactPos = p - marbleCollisionNormal * mRadius;
                    }
                } 
            }  
        }
	}
    
    // Marble on Platform collision
    if (!polyList.mPolyList.empty())
    {
        ConcretePolyList::Poly* poly;

        for (S32 index = 0; index < polyList.mPolyList.size(); index++)
        {
            poly = &polyList.mPolyList[index];

            PlaneD polyPlane = poly->plane;

            // If we're going the wrong direction or not going to touch the plane, ignore...
            if (mDot(polyPlane, velocityDir) > -0.001 || mDot(polyPlane, finalPosition) + polyPlane.d > radius)
                continue;

            // Time until collision with the plane
            F64 collisionTime = (radius - (mDot(polyPlane, position) + polyPlane.d)) / mDot(polyPlane, velocity);

            // Are we going to touch the plane during this time step?
            if (collisionTime >= 0.0 && finalT >= collisionTime)
            {
                U32 lastVertIndex = polyList.mIndexList[poly->vertexCount - 1 + poly->vertexStart];
                Point3F lastVert = polyList.mVertexList[lastVertIndex];

                Point3D collisionPos = velocity * collisionTime + position;

                U32 i;
                for (i = 0; i < poly->vertexCount; i++)
                {
                    Point3F thisVert = polyList.mVertexList[polyList.mIndexList[i + poly->vertexStart]];
                    if (thisVert != lastVert)
                    {
                        PlaneD edgePlane(thisVert + polyPlane, thisVert, lastVert);
                        lastVert = thisVert;

                        // if we are on the far side of the edge
                        if (mDot(edgePlane, collisionPos) + edgePlane.d < 0.0)
                            break;
                    }
                }

                bool isOnEdge = i != poly->vertexCount;

                // If we're inside the poly, just get the position
                if (!isOnEdge)
                {
                    finalT = collisionTime;
                    finalPosition = collisionPos;
                    lastContactPos = polyPlane.project(collisionPos);
                    contactPoly = poly;
                    continue;
                }
            }

            // We *might* be colliding with an edge

            Point3F lastVert = polyList.mVertexList[polyList.mIndexList[poly->vertexCount - 1 + poly->vertexStart]];

            if (poly->vertexCount == 0)
                continue;

            F64 radSq = radius * radius;

            for (S32 iter = 0; iter < poly->vertexCount; iter++)
            {
                Point3D thisVert = polyList.mVertexList[polyList.mIndexList[iter + poly->vertexStart]];

                Point3D vertDiff = lastVert - thisVert;
                Point3D posDiff = position - thisVert;
                
                Point3D velRejection = mCross(vertDiff, velocity);
                Point3D posRejection = mCross(vertDiff, posDiff);

                // Build a quadratic equation to solve for the collision time
                F64 a = velRejection.lenSquared();
                F64 halfB = mDot(posRejection, velRejection);
                F64 b = halfB + halfB;

                F64 discriminant = b * b - (posRejection.lenSquared() - vertDiff.lenSquared() * radSq) * (a * 4.0);

                // If it's not quadratic or has no solution, ignore this edge.
                if (a == 0.0 || discriminant < 0.0)
                {
                    lastVert = thisVert;
                    continue;
                }

                F64 oneOverTwoA = 0.5 / a;
                F64 discriminantSqrt = mSqrtD(discriminant);

                // Solve using the quadratic formula
                F64 edgeCollisionTime = (discriminantSqrt - b) * oneOverTwoA;
                F64 edgeCollisionTime2 = (-b - discriminantSqrt) * oneOverTwoA;

                // Make sure the 2 times are in ascending order
                if (edgeCollisionTime2 < edgeCollisionTime)
                {
                    F64 temp = edgeCollisionTime2;
                    edgeCollisionTime2 = edgeCollisionTime;
                    edgeCollisionTime = temp;
                }

                // If the collision doesn't happen on this time step, ignore this edge.
                if (edgeCollisionTime2 <= 0.0001 || finalT <= edgeCollisionTime)
                {
                    lastVert = thisVert;
                    continue;
                }

                // Check if the collision hasn't already happened
                if (edgeCollisionTime >= 0.0)
                {
                    F64 edgeLen = vertDiff.len();

                    Point3D relativeCollisionPos = velocity * edgeCollisionTime + position - thisVert;

                    F64 distanceAlongEdge = mDot(relativeCollisionPos, vertDiff) / edgeLen;

                    // If the collision happens outside the boundaries of the edge, ignore this edge.
                    if (-radius > distanceAlongEdge || edgeLen + radius < distanceAlongEdge)
                    {
                        lastVert = thisVert;
                        continue;
                    }

                    // If the collision is within the edge, resolve the collision and continue.
                    if (distanceAlongEdge >= 0.0 && distanceAlongEdge <= edgeLen)
                    {
                        finalT = edgeCollisionTime;
                        finalPosition = velocity * edgeCollisionTime + position;

                        lastContactPos = vertDiff * (distanceAlongEdge / edgeLen) + thisVert;
                        contactPoly = poly;

                        lastVert = thisVert;
                        continue;
                    }
                }

                // This is what happens when we collide with a corner

                F64 speedSq = velocity.lenSquared();

				// Build a quadratic equation to solve for the collision time
                Point3D posVertDiff = position - thisVert;
                F64 halfCornerB = mDot(posVertDiff, velocity);
                F64 cornerB = halfCornerB + halfCornerB;

                F64 fourA = speedSq * 4.0;

                F64 cornerDiscriminant = cornerB * cornerB - (posVertDiff.lenSquared() - radSq) * fourA;

                // If it's quadratic and has a solution ...
                if (speedSq != 0.0 && cornerDiscriminant >= 0.0)
                {
                    F64 oneOver2A = 0.5 / speedSq;
                    F64 cornerDiscriminantSqrt = mSqrtD(cornerDiscriminant);
					
					// Solve using the quadratic formula
					F64 cornerCollisionTime = (cornerDiscriminantSqrt - cornerB) * oneOver2A;
                    F64 cornerCollisionTime2 = (-cornerB - cornerDiscriminantSqrt) * oneOver2A;
					
					// Make sure the 2 times are in ascending order
                    if (cornerCollisionTime2 < cornerCollisionTime)
                    {
                        F64 temp = cornerCollisionTime2;
                        cornerCollisionTime2 = cornerCollisionTime;
                        cornerCollisionTime = temp;
                    }

					// If the collision doesn't happen on this time step, ignore this corner
                    if (cornerCollisionTime2 > 0.0001 && finalT > cornerCollisionTime)
                    {
						// Adjust to make sure very small negative times are counted as zero
                        if (cornerCollisionTime <= 0.0 && cornerCollisionTime > -0.0001)
                            cornerCollisionTime = 0.0;

						// Check if the collision hasn't already happened
                        if (cornerCollisionTime >= 0.0)
                        {
                            // Resolve it and continue
                            finalT = cornerCollisionTime;
                            contactPoly = poly;
                            finalPosition = velocity * cornerCollisionTime + position;
                            lastContactPos = thisVert;
                        }
                    }
                }
				
				// We still need to check the other corner ...
				// Build one last quadratic equation to solve for the collision time
                Point3D lastVertDiff = position - lastVert;
                F64 lastCornerHalfB = mDot(lastVertDiff, velocity);
                F64 lastCornerB = lastCornerHalfB + lastCornerHalfB;
                F64 lastCornerDiscriminant = lastCornerB * lastCornerB - (lastVertDiff.lenSquared() - radSq) * fourA;

				// If it's not quadratic or has no solution, then skip this corner
                if (speedSq == 0.0 || lastCornerDiscriminant < 0.0)
                {
                    lastVert = thisVert;
                    continue;
                }

                F64 lastCornerOneOver2A = 0.5 / speedSq;
                F64 lastCornerDiscriminantSqrt = mSqrtD(lastCornerDiscriminant);
				
				// Solve using the quadratic formula
                F64 lastCornerCollisionTime = (lastCornerDiscriminantSqrt - lastCornerB) * lastCornerOneOver2A;
                F64 lastCornerCollisionTime2 = (-lastCornerB - lastCornerDiscriminantSqrt) * lastCornerOneOver2A;
				
				// Make sure the 2 times are in ascending order
                if (lastCornerCollisionTime2 < lastCornerCollisionTime)
                {
                    F64 temp = lastCornerCollisionTime2;
                    lastCornerCollisionTime2 = lastCornerCollisionTime;
                    lastCornerCollisionTime = temp;
                }

				// If the collision doesn't happen on this time step, ignore this corner
                if (lastCornerCollisionTime2 <= 0.0001 || finalT <= lastCornerCollisionTime)
                {
                    lastVert = thisVert;
                    continue;
                }

				// Adjust to make sure very small negative times are counted as zero
                if (lastCornerCollisionTime <= 0.0 && lastCornerCollisionTime > -0.0001)
                    lastCornerCollisionTime = 0.0;

				// Check if the collision hasn't already happened
                if (lastCornerCollisionTime < 0.0)
                {
                    lastVert = thisVert;
                    continue;
                }

				// Resolve it and continue
                finalT = lastCornerCollisionTime;
                finalPosition = velocity * lastCornerCollisionTime + position;
                lastContactPos = lastVert;
                contactPoly = poly;

                lastVert = thisVert;
            }
        }
    }

    position = finalPosition;

    bool contacted = false;
    if (deltaT > finalT)
    {
        Material* material;

        // Did we collide with a poly as opposed to a marble?
        if (marbleCollisionTime > finalT && contactPoly != NULL && contactPoly->material != -1)
        {
            material = contactPoly->object->getMaterial(contactPoly->material);
            if ((contactPoly->object->getTypeMask() & ShapeBaseObjectType) != 0)
            {
                Point3F objVelocity = contactPoly->object->getVelocity();
                queueCollision((ShapeBase*)contactPoly->object, mVelocity - objVelocity, contactPoly->material);
            }
        }

        mLastContact.position = lastContactPos;

        // Did we collide with a marble?
        if (marbleCollisionTime <= finalT)
            mLastContact.normal = marbleCollisionNormal;
        else
        {
            // or a poly?
            mLastContact.normal = finalPosition - lastContactPos;
            mLastContact.normal.normalize();
        }

        if (material == NULL)
        {
            mLastContact.friction = 1.0f;
            mLastContact.restitution = 1.0f;
            mLastContact.force = 0.0f;
        } else
        {
            mLastContact.friction = material->friction;
            mLastContact.restitution = material->restitution;
            mLastContact.force = material->force;
        }
        
        contacted = true;
    }

    deltaT = finalT;
    return contacted;
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

				dMemcpy(marbleContact->surfaceVelocity, otherMarble->getVelocityD(), sizeof(marbleContact->surfaceVelocity));

				marbleContact->material = 0;
				marbleContact->object = otherMarble;
				marbleContact->position = *pos + normDist;
				marbleContact->normal = -normDist;
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

            //if (mPhysics == MBG)
            //{
                Point3D finalContact = contactVert;
            //}

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
                                if (mPhysics == MBG || mPhysics == MBGSlopes)
                                    finalContact = vertPlane.project(contactVert);
                                else
								    contactVert = vertPlane.project(contactVert);
								break;
							}
                            if (mPhysics == MBG || mPhysics == MBGSlopes)
                                finalContact = lastVertex;
                            else
							    contactVert = lastVertex;
						}
						else {
                            if (mPhysics == MBG || mPhysics == MBGSlopes)
							    finalContact = vertex;
                            else
                                contactVert = vertex;
						}
					}
					lastVertex = vertex;
				}
			}
            
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
            Point3D delta;
            if (mPhysics == MBG || mPhysics == MBGSlopes)
                delta = *pos - finalContact;
            else
			    delta = *pos - contactVert;

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
            if (mPhysics == MBG || mPhysics == MBGSlopes)
                contact.position = finalContact;
            else
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
    Box3F box;
    box.min = mPosition + mObjBox.min - 0.5f;
    box.max = mPosition + mObjBox.max + 0.5f;

    Point3F deltaVelocity = mVelocity * dt;
    if (deltaVelocity.x >= 0.0f)
        box.max.x += deltaVelocity.x;
    else
        box.min.x += deltaVelocity.x;

    if (deltaVelocity.y >= 0.0f)
        box.max.y += deltaVelocity.y;
    else
        box.min.y += deltaVelocity.y;

    if (deltaVelocity.z >= 0.0f)
        box.max.z += deltaVelocity.z;
    else
        box.min.z += deltaVelocity.z;

    if (!pitrVec.empty())
    {
        for (S32 i = 0; i < pitrVec.size(); i++)
        {
            PathedInterior* it = pitrVec[i];

            Box3F itBox = it->getExtrudedBox();
            if (itBox.isOverlapped(box))
            {
                bool isContacting = false;
                for (S32 j = 0; j < mContacts.size(); j++)
                {
                    if (mContacts[j].object != it)
                        continue;

                    isContacting = true;
                    break;
                }

                if (!isContacting)
                {
                    Point3F vel = mVelocity - it->getVelocity();
                    Point3F boxCenter;
                    itBox.getCenter(&boxCenter);

                    Point3F diff = itBox.max - boxCenter;
                    SphereF sphere(boxCenter, diff.len());
                    polyList.clear();
                    it->buildPolyList(&polyList, itBox, sphere);

                    Point3D position = mPosition;
                    testMove(vel, position, dt, mRadius, 0, false);
                }
            }
        }
    }
}

void Marble::resetObjectsAndPolys(U32 collisionMask, const Box3F& testBox)
{
    sgLastCollisionBox.min.set(0, 0, 0);
    sgLastCollisionBox.max.set(0, 0, 0);

    sgResetFindObjects = true;
    sgCountCalls = 0;

    if (smPathItrVec.empty())
        findObjectsAndPolys(collisionMask, testBox, false);
}
