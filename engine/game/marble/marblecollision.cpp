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
    // TODO: Cleanup Decompile

    unsigned int v4; // ecx
    unsigned int v5; // eax
    Point3F* v6; // eax
    Point3F* v7; // eax
    PlaneF p; // [esp+Ch] [ebp-34h] BYREF
    Point3F k; // [esp+1Ch] [ebp-24h] BYREF
    Point3F v2; // [esp+28h] [ebp-18h] BYREF
    Point3F v1; // [esp+34h] [ebp-Ch] BYREF
    unsigned int i; // [esp+48h] [ebp+8h]

    v4 = poly.vertexCount;
    v5 = poly.vertexStart;
    i = 0;
    v6 = &Marble::polyList.mVertexList[Marble::polyList.mIndexList[v4 - 1 + v5]];
    v2.x = v6->x;
    v2.y = v6->y;
    v2.z = v6->z;
    if (!v4)
        return true;
    while (true)
    {
        v7 = &Marble::polyList.mVertexList[Marble::polyList.mIndexList[i + poly.vertexStart]];
        v1.x = v7->x;
        v1.y = v7->y;
        v1.z = v7->z;
        k.x = poly.plane.x + v1.x;
        k.y = v1.y + poly.plane.y;
        k.z = poly.plane.z + v1.z;
        p.set(k, v1, v2);
        v2 = v1;
        if (p.distToPlane(point) < 0.0f)
            break;
        if (++i >= poly.vertexCount)
            return true;
    }
    return false;
}

bool Marble::pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir)
{
    // TODO: Cleanup Decompile

    unsigned int v5; // ecx
    unsigned int v6; // eax
    Point3F* v7; // eax
    Point3F* v8; // eax
    PlaneF p; // [esp+Ch] [ebp-34h] BYREF
    Point3F k; // [esp+1Ch] [ebp-24h] BYREF
    Point3F v2; // [esp+28h] [ebp-18h] BYREF
    Point3F v1; // [esp+34h] [ebp-Ch] BYREF
    unsigned int i; // [esp+48h] [ebp+8h]

    v5 = poly.vertexCount;
    v6 = poly.vertexStart;
    i = 0;
    v7 = &Marble::polyList.mVertexList[Marble::polyList.mIndexList[v5 - 1 + v6]];
    v2.x = v7->x;
    v2.y = v7->y;
    v2.z = v7->z;
    if (!v5)
        return true;
    while (true)
    {
        v8 = &Marble::polyList.mVertexList[Marble::polyList.mIndexList[i + poly.vertexStart]];
        v1.x = v8->x;
        v1.y = v8->y;
        v1.z = v8->z;
        k.x = upDir.x + v1.x;
        k.y = v1.y + upDir.y;
        k.z = upDir.z + v1.z;
        p.set(k, v1, v2);
        v2 = v1;
        if (p.distToPlane(point) < -0.003f)
            break;
        if (++i >= poly.vertexCount)
            return true;
    }
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

	Point3D deltaVelocity = velocity * deltaT;
	Point3D finalPosition = position + deltaVelocity;

	// If there is a collision mask
	if (collisionMask != 0)
	{
        Box3F box(mObjBox.min + position - 0.5f, mObjBox.max + position + 0.5f);

		if (deltaVelocity.x >= 0.0)
            box.max.x += deltaVelocity.x;
		else
            box.min.x += deltaVelocity.x;

		if (deltaVelocity.y >= 0.0)
            box.max.y += deltaVelocity.y;
		else
            box.min.y += deltaVelocity.y;

		if (deltaVelocity.z >= 0.0)
			box.max.z += deltaVelocity.z;
		else
            box.min.z += deltaVelocity.z;

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
        Point3F nextPos = position + deltaVelocity;
        
        for (S32 i = 0; i < marbles.size(); i++)
        {
            Marble* other = marbles[i];

            Point3F otherPos = other->getPosition();

            F32 time;
            if (MathUtils::capsuleSphereNearestOverlap(position, nextPos, mRadius, otherPos, other->mRadius, time));
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

	S32 iter = 0;
	// Marble on Platform collision
	if (!polyList.mPolyList.empty())
	{
        ConcretePolyList::Poly* poly;
        S32 index = 0;
        while(true)
        {
            poly = &polyList.mPolyList[index];

            PlaneD plane = poly->plane;

            if (plane.y * velocityDir.y + plane.x * velocityDir.x + plane.z * velocityDir.z > -0.001 ||
                plane.y * finalPosition.y + plane.x * finalPosition.x + plane.z * finalPosition.z + plane.d > radius)
            {
                goto CONTINUE_FIRST_LOOP;
            }

            F64 timeVar = (radius - (plane.x * position.x + plane.y * position.y + plane.z * position.z + plane.d))
                                / (plane.y * velocity.y + plane.x * velocity.x + plane.z * velocity.z);

            if (timeVar < 0.0 || finalT < timeVar)
                break;

            U32 vertIndex = polyList.mIndexList[poly->vertexCount - 1 + poly->vertexStart];
            Point3F vert = polyList.mVertexList[vertIndex];

            Point3D velAddition = velocity * timeVar + position;

            bool hasVertices = poly->vertexCount != 0;

            if (hasVertices)
            {
                U32 i = 0;
                do
                {
                    Point3F newVert = polyList.mVertexList[polyList.mIndexList[i + poly->vertexStart]];
                    if (newVert != vert)
                    {
                        PlaneD pl(Point3D(newVert + plane), newVert, vert);
                        vert = newVert;
                        if (pl.x * velAddition.x + pl.y * velAddition.y + pl.z * velAddition.z + pl.d < 0.0)
                            break;
                    }

                    i++;
                } while(i < poly->vertexCount);

                hasVertices = i != poly->vertexCount;
            }

            if (hasVertices)
                break;

            finalT = timeVar;
            finalPosition = velAddition;
            lastContactPos = plane.project(velAddition);
            contactPoly = poly;

CONTINUE_FIRST_LOOP:
            index++;
            if (index >= polyList.mPolyList.size())
                goto superbreak;
        }

        Point3F nextVert = polyList.mVertexList[polyList.mIndexList[poly->vertexCount - 1 + poly->vertexStart]];

        if (poly->vertexCount == 0)
            goto CONTINUE_FIRST_LOOP;

        F64 lastZ;

        Point3D theVert;

        F64 radSq = radius * radius;
        S32 iter = 0;
        while (true)
        {
            theVert = polyList.mVertexList[polyList.mIndexList[iter + poly->vertexStart]];

            Point3D vertDiff = nextVert - theVert;

            Point3D posDiff = position - theVert;

            Point3D theVelocity;
            mCross(vertDiff, velocity, &theVelocity);

            Point3D thePosThing;
            mCross(vertDiff, posDiff, &thePosThing);

            F64 theVelLen = theVelocity.lenSquared();
            F64 dotVel = mDot(thePosThing, theVelocity);
            F64 dotVelSq = dotVel + dotVel;

            F64 someNum = dotVelSq * dotVelSq - (thePosThing.lenSquared() - vertDiff.lenSquared() * radSq)
                        * (theVelLen * 4.0);

            if (theVelLen == 0.0 || someNum < 0.0)
                goto noedgeint;

            F64 what = 0.5 / theVelLen;
            F64 someNumSqrt = mSqrtD(someNum);

            F64 t1 = (someNumSqrt - dotVelSq) * what;
            F64 t2 = (-dotVelSq - someNumSqrt) * what;
            if (t2 < t1)
            {
                F64 temp = t2;
                t2 = t1;
                t1 = temp;
            }

            if (t2 <= 0.0001 || finalT <= t1)
                goto noedgeint;
                
            if (t1 < 0.0)
                goto LABEL_52;

            F64 vertDiffLen = vertDiff.len();
            
            Point3D velTimePosVert = velocity * t1 + position - theVert;

            F64 theVar = mDot(velTimePosVert, vertDiff) / vertDiffLen;

            if (-radius > theVar || vertDiffLen + radius < theVar)
                goto noedgeint;

            if (theVar < 0.0)
                break;
            if (theVar > vertDiffLen)
                goto LABEL_52;

            finalT = t1;
            finalPosition = velocity * t1 + position;

            lastContactPos = vertDiff * (theVar / vertDiffLen) + theVert;
            contactPoly = poly;

noedgeint:
            bool notGoBack = ++iter < poly->vertexCount;

            nextVert = theVert;

            if (!notGoBack)
                goto CONTINUE_FIRST_LOOP;
        }
LABEL_52:
        F64 wow = velocity.y * velocity.y + velocity.x * velocity.x + velocity.z * velocity.z;

	    Point3D posVertDiff = position - theVert;
        F64 posVertDiffDot = mDot(posVertDiff, velocity);
        F64 posVertDiffDotSq = posVertDiffDot + posVertDiffDot;

        F64 tx = wow * 4.0;

        F64 var12 = posVertDiffDotSq * posVertDiffDotSq - (posVertDiff.lenSquared() - radSq) * tx;

        if (wow != 0.0 && var12 >= 0.0)
        {
            F64 var13 = 0.5 / wow;
            F64 var12Sqrt = mSqrtD(var12);
            F64 var14 = -posVertDiffDotSq - var12Sqrt;
            F64 var1 = (var12Sqrt - posVertDiffDotSq) * var13;
            F64 var2 = var14 * var13;
            if (var2 < var1)
            {
                F64 temp = var2;
                var2 = var1;
                var1 = temp;
            }
            if (var2 > 0.0001 && finalT > var1)
            {
                if (var1 <= 0.0 && var1 > -0.0001)
                    var1 = 0.0;

                if (var1 >= 0.0)
                {
                    finalT = var1;
                    contactPoly = poly;
                    finalPosition = velocity * var1 + position;
                    lastContactPos = theVert;
                }
            }
        }

        Point3D var3 = position - nextVert;
        F64 var4 = mDot(var3, velocity);
        F64 var5 = var4 + var4;
        F64 var6 = var5 * var5 - (var3.lenSquared() - radSq) * tx;
        if (wow == 0.0 || var6 < 0.0)
            goto noedgeint;

        F64 var7 = 0.5 / wow;
        F64 var8 = mSqrtD(var6);
        F64 var9 = -var5 - var8;
        F64 var10 = (var8 - var5) * var7;
        F64 var11 = var9 * var7;
        if (var11 < var10)
        {
            F64 temp = var11;
            var11 = var10;
            var10 = temp;
        }

        if (var11 <= 0.0001 || finalT <= var10)
            goto noedgeint;
        if (var10 <= 0.0 && var10 > -0.0001)
            var10 = 0.0;
        if (var10 < 0.0)
            goto noedgeint;

        finalT = var10;
        finalPosition = velocity * var10 + position;
        lastContactPos = nextVert;
        contactPoly = poly;

        goto noedgeint;
	}
superbreak:

    position = finalPosition;

    bool contacted = false;
    if (deltaT > finalT)
    {
        Material* material;

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
        if (marbleCollisionTime <= finalT)
            mLastContact.normal = marbleCollisionNormal;
        else
            mLastContact.normal = finalPosition - lastContactPos;

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

				memcpy(marbleContact->surfaceVelocity, otherMarble->getVelocityD(), sizeof(marbleContact->surfaceVelocity));

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
    Box3F box;
    box.min = mPosition + mObjBox.min - 0.5f;
    box.max = mPosition + mObjBox.max + 0.5f;

    Point3F deltaVelocity = mVelocity * dt;
    if (deltaVelocity.x > 0.0f)
        box.max.x += deltaVelocity.x;
    else
        box.min.x += deltaVelocity.x;

    if (deltaVelocity.y > 0.0f)
        box.max.y += deltaVelocity.y;
    else
        box.min.y += deltaVelocity.y;

    if (deltaVelocity.z > 0.0f)
        box.max.z += deltaVelocity.z;
    else
        box.min.z += deltaVelocity.z;

    if (!pitrVec.empty())
    {
        S32 i = 0;
        do
        {
            PathedInterior* it = pitrVec[i];
            Box3F itBox = it->getExtrudedBox();
            if (itBox.isOverlapped(box))
            {
                if (!mContacts.empty())
                {
                    S32 j = 0;
                    while (mContacts[j].object != it)
                    {
                        j++;
                        if (j >= mContacts.size())
                            goto LABEL_16;
                    }
                } else
                {
LABEL_16:
                    Point3F vel = it->getVelocity();
                    Point3F boxCenter;
                    itBox.getCenter(&boxCenter);

                    Point3F diff = itBox.max - boxCenter;
                    SphereF sphere(boxCenter, diff.len());
                    polyList.clear();
                    it->buildPolyList(&polyList, itBox, sphere);
                    Point3D position = mPosition;
                    testMove(mVelocity, position, dt, mRadius, 0, false);
                }
            }
            i++;
        } while(i < pitrVec.size());
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
