#include "interiorMapRes.h"

#include "gfx/gBitmap.h"
#include "console/consoleTypes.h"
#include "console/console.h"
#include "collision/convexBrush.h"
#include "core/fileStream.h"

InteriorMapResource::InteriorMapResource()
{
    mBrushFormat = Unknown;
    mTexGensCalced = false;
    mNextBrushID = 0;
    mWorldSpawn = NULL;
    mBrushScale = 1.0f;
}

InteriorMapResource::~InteriorMapResource()
{
}

bool InteriorMapResource::load(const char* filename)
{
    // Open our file and read it into a buffer
    FileStream* pStream = new FileStream;

    if (pStream->open(filename, FileStream::Read) == false)
    {
        Con::errorf("Unable to open %s", filename);

        delete pStream;
        return false;
    }

    read(*pStream);

    // Done with actual file
    pStream->close();
    delete pStream;

    return true;
}

bool InteriorMapResource::read(Stream& stream)
{
    // Parse that sucker
    Tokenizer toker;
    toker.openFile(&stream);

    parseMap(&toker);

    for (U32 i = 0; i < mBrushes.size(); i++)
        mBrushes[i]->processBrush();

    // Assign IDs
    for (U32 i = 0; i < mBrushes.size(); i++)
        mBrushes[i]->mID = getNextBrushID();

    for (U32 i = 0; i < mBrushes.size(); i++)
    {
        if (mBrushes[i]->mStatus != ConvexBrush::Good)
            Con::errorf(mBrushes[i]->mDebugInfo);
    }

    return true;
}

bool InteriorMapResource::write(Stream& stream)
{
    char buffer[10240];

    dSprintf(buffer, 10240, "// This map has been written by the Torque Constructor\r\n");
    stream.write(dStrlen(buffer), buffer);

    dSprintf(buffer, 10240, "// For more information see http://www.garagegames.com\r\n");
    stream.write(dStrlen(buffer), buffer);

    stream.write(2, "\r\n");

    // Start the worldspawn dump
    dSprintf(buffer, 10240, "{\r\n");
    stream.write(dStrlen(buffer), buffer);

    // Write out the properties of worldspawn
    if (mWorldSpawn)
    {
        for (U32 i = 0; i < mWorldSpawn->properties.size(); i++)
        {
            dSprintf(buffer, 10240, "   \"%s\" \"%s\"\r\n", mWorldSpawn->properties[i].name, mWorldSpawn->properties[i].value);
            stream.write(dStrlen(buffer), buffer);
        }
    }
    else // Write out some default properties
    {
        dSprintf(buffer, 10240, "   \"classname\" \"worldspawn\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"detail_number\" \"0\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"min_pixels\" \"250\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"geometry_scale\" \"32.0\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"light_geometry_scale\" \"32.0\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"ambient_color\" \"0 0 0\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"emergency_ambient_color\" \"0 0 0\"\r\n");
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   \"mapversion\" \"220\"\r\n");
        stream.write(dStrlen(buffer), buffer);
    }

    // Dump the structural brushes into the worldspawn
    for (U32 i = 0; i < mBrushes.size(); i++)
    {
        if (mBrushes[i]->mType != Structural)
            continue;

        // Don't write any deleted brushes
        if (mBrushes[i]->mStatus == ConvexBrush::Deleted)
            continue;

        writeBrush(i, stream);
    }

    // Finish up the worldspawn dump
    dSprintf(buffer, 10240, "}\r\n");
    stream.write(dStrlen(buffer), buffer);

    // Dump the rest of the entities
    for (U32 i = 0; i < mEntities.size(); i++)
    {
        if (dStricmp("worldspawn", mEntities[i]->classname) == 0)
            continue;

        // Begin the entity dump
        dSprintf(buffer, 10240, "{\r\n");
        stream.write(dStrlen(buffer), buffer);

        // Write out the entity properties
        for (U32 j = 0; j < mEntities[i]->properties.size(); j++)
        {
            dSprintf(buffer, 10240, "   \"%s\" \"%s\"\r\n", mEntities[i]->properties[j].name, mEntities[i]->properties[j].value);
            stream.write(dStrlen(buffer), buffer);
        }

        // Dump any brushes that belong to this entity
        for (U32 j = 0; j < mBrushes.size(); j++)
        {
            // We already dumped the Structural brushes with worldspawn
            if (mBrushes[j]->mType == Structural)
                continue;

            // Skip any that don't belong to this entity
            if (mBrushes[j]->mOwner != mEntities[i])
                continue;

            // Don't write any deleted brushes
            if (mBrushes[j]->mStatus == ConvexBrush::Deleted)
                continue;

            writeBrush(j, stream);
        }

        // Finish the entity dump
        dSprintf(buffer, 10240, "}\r\n");
        stream.write(dStrlen(buffer), buffer);
    }

    return true;
}

bool InteriorMapResource::writeBrush(U32 brushIndex, Stream& stream)
{
    char buffer[10240];

    if (mBrushes[brushIndex]->mFaces.mPolyList.size() > 3)
    {
        dSprintf(buffer, 10240, "\r\n   // Brush %d\r\n", brushIndex);
        stream.write(dStrlen(buffer), buffer);

        dSprintf(buffer, 10240, "   {\r\n");
        stream.write(dStrlen(buffer), buffer);

        for (U32 j = 0; j < mBrushes[brushIndex]->mFaces.mPolyList.size(); j++)
        {
            // MDFFIX: Really shouldn't rely on pre-calced points..should generate them from plane
            if (mBrushes[brushIndex]->mFaces.mPolyList[j].vertexCount > 2)
            {
                // MDFFIX: Only testing first 3 verts...should really loop until find a valid triangle
                U32 i0 = mBrushes[brushIndex]->mFaces.mIndexList[mBrushes[brushIndex]->mFaces.mPolyList[j].vertexStart];
                U32 i1 = mBrushes[brushIndex]->mFaces.mIndexList[mBrushes[brushIndex]->mFaces.mPolyList[j].vertexStart + 1];
                U32 i2 = mBrushes[brushIndex]->mFaces.mIndexList[mBrushes[brushIndex]->mFaces.mPolyList[j].vertexStart + 2];

                // Snag the points
                Point3F p0 = mBrushes[brushIndex]->mFaces.mVertexList[i0];
                Point3F p1 = mBrushes[brushIndex]->mFaces.mVertexList[i1];
                Point3F p2 = mBrushes[brushIndex]->mFaces.mVertexList[i2];

                // Multiply by the brush transform
                Point3F tp0, tp1, tp2;
                MatrixF mat = mBrushes[brushIndex]->mTransform;
                mat.mulP(p0, &tp0);
                mat.mulP(p1, &tp1);
                mat.mulP(p2, &tp2);

                // Scale it by our transform scale
                tp0.convolve(mBrushes[brushIndex]->mScale);
                tp1.convolve(mBrushes[brushIndex]->mScale);
                tp2.convolve(mBrushes[brushIndex]->mScale);

                // Now scale it up by the brush scale
                tp0 *= mBrushScale;
                tp1 *= mBrushScale;
                tp2 *= mBrushScale;

                F32 rot = mBrushes[brushIndex]->mTexInfos[j].rot;
                F32 texScale[2];
                F32 texDiv[2];
                PlaneF texGens[2];
                texScale[0] = mBrushes[brushIndex]->mTexInfos[j].scale[0];
                texScale[1] = mBrushes[brushIndex]->mTexInfos[j].scale[1];
                texDiv[0] = mBrushes[brushIndex]->mTexInfos[j].texDiv[0];
                texDiv[1] = mBrushes[brushIndex]->mTexInfos[j].texDiv[1];
                texGens[0] = mBrushes[brushIndex]->mTexInfos[j].texGens[0];
                texGens[1] = mBrushes[brushIndex]->mTexInfos[j].texGens[1];

                //Con::warnf("preconditioned texGens[0](%g, %g, %g, %g) texGens[1](%g, %g, %g, %g)",
                //            texGens[0].x, texGens[0].y, texGens[0].z, texGens[0].d,
                //            texGens[1].x, texGens[1].y, texGens[1].z, texGens[1].d);

                // Divide out the brushscale
                texGens[0] /= mBrushScale;
                texGens[1] /= mBrushScale;

                // Multiply by our texture sizes
                texGens[0] *= texDiv[0];
                texGens[0].d *= texDiv[0];
                texGens[1] *= texDiv[1];
                texGens[1].d *= texDiv[1];

                // Multiply by texgen scale
                texGens[0] *= texScale[0];
                texGens[1] *= texScale[1];

                // Normalize axii and d
                F32 mag[2];
                mag[0] = 1.0f / texGens[0].len();
                mag[1] = 1.0f / texGens[1].len();

                texGens[0].normalize();
                texGens[1].normalize();

                texGens[0].d *= mag[0];
                texGens[1].d *= mag[1];

                //Con::warnf("conditioned texGens[0](%g, %g, %g, %g) texGens[1](%g, %g, %g, %g)",
                //            texGens[0].x, texGens[0].y, texGens[0].z, texGens[0].d,
                //            texGens[1].x, texGens[1].y, texGens[1].z, texGens[1].d);

                if (validatePlane(p0, p1, p2))
                {
                    // ( -2 4 -158 ) ( -2 -252 -158 ) ( 254 4 -158 ) concrete_1 [ 1.00000 0.00000 0.00000 1.00000 ]  [ 0.00000 -1.00000 0.00000 -2.00000 ]  0  2.00000 -2.00000

                    // First the verts specifying the plane
                    dSprintf(buffer, 10240, "      ( %g %g %g ) ( %g %g %g ) ( %g %g %g )", tp0.x, tp0.y, tp0.z, tp1.x, tp1.y, tp1.z, tp2.x, tp2.y, tp2.z);
                    stream.write(dStrlen(buffer), buffer);

                    // Then the texture
                    dSprintf(buffer, 10240, " %s", mMaterials[mBrushes[brushIndex]->mFaces.mPolyList[j].material]);
                    stream.write(dStrlen(buffer), buffer);

                    // Now the texgens
                    dSprintf(buffer, 10240, " [ %g %g %g %g ]", texGens[0].x, texGens[0].y, texGens[0].z, texGens[0].d);
                    stream.write(dStrlen(buffer), buffer);

                    dSprintf(buffer, 10240, " [ %g %g %g %g ]", texGens[1].x, texGens[1].y, texGens[1].z, texGens[1].d);
                    stream.write(dStrlen(buffer), buffer);

                    // And last the rotation and scaling
                    dSprintf(buffer, 10240, " %g %g %g\r\n", rot, texScale[0], texScale[1]);
                    stream.write(dStrlen(buffer), buffer);
                }
            }
        }

        dSprintf(buffer, 10240, "   }\r\n");
        stream.write(dStrlen(buffer), buffer);
    }
    return true;
}

bool InteriorMapResource::parseMap(Tokenizer* toker)
{
    // Find a worldspawn entity and then find a brush
    toker->reset();

    while (toker->advanceToken(true))
    {
        // Search for the beginning of an entity
        while (toker->getToken()[0] != '{')
        {
            if (!toker->advanceToken(true))
                break;
        }

        // Check to see if we are at the end of the file or at
        // the beginning of an entity
        if (toker->getToken()[0] == '{')
        {
            // Found the start of an entity...parse it
            parseEntity(toker);
        }

        // Unable to support the newest Quake 3 format
        if (mBrushFormat == QuakeNew)
        {
            Con::errorf("The latest Quake 3 format is incompatible with this version of map2dif. Please use an older format.");
            return false;
        }

        // parseEntity *should* return us at the end of the entity - i.e }
        // If it doesn't then loop through till we find the end
        while (toker->getToken()[0] != '}')
        {
            if (!toker->advanceToken(true))
                break;
        }
    }

    //for (U32 i = 0; i < mEntities.size(); i++)
    //{
    //   Con::errorf("Entity[%d] %s", i, mEntities[i]->classname);
    //   for (U32 j = 0; j < mEntities[i]->properties.size(); j++)
    //      Con::printf("   %s %s", mEntities[i]->properties[j].name, mEntities[i]->properties[j].value);
    //}

    // If we don't have a worldspawn entity then create one
    if (!mWorldSpawn)
    {
        // Add a worldspawn entity
        mWorldSpawn = new Entity;
        mEntities.push_back(mWorldSpawn);

        mWorldSpawn->classname = StringTable->insert("worldspawn");

        // Fill in the default properties
        InteriorMapResource::Property prop;

        prop.name = StringTable->insert("classname");
        prop.value = StringTable->insert("worldspawn");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("detail_number");
        prop.value = StringTable->insert("0");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("min_pixels");
        prop.value = StringTable->insert("250");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("geometry_scale");
        prop.value = StringTable->insert("1.0");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("light_geometry_scale");
        prop.value = StringTable->insert("32.0");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("ambient_color");
        prop.value = StringTable->insert("0 0 0");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("emergency_ambient_color");
        prop.value = StringTable->insert("0 0 0");
        mWorldSpawn->properties.push_back(prop);

        prop.name = StringTable->insert("mapversion");
        prop.value = StringTable->insert("220");
        mWorldSpawn->properties.push_back(prop);
    }

    return true;
}

bool InteriorMapResource::parsePatch(Tokenizer* toker)
{
    // For now we aren't parsing patches so just skip to the end of the entity
    U32 bcnt = 1;

    while (bcnt > 0)
    {
        toker->advanceToken(true);

        if (toker->getToken()[0] == '{')
            bcnt++;
        if (toker->getToken()[0] == '}')
            bcnt--;
    }

    return true;
}

bool InteriorMapResource::parseBrush(Tokenizer* toker, BrushType type)
{
    mBrushes.increment();
    mBrushes.last() = new ConvexBrush;
    mBrushes.last()->mType = type;
    mBrushes.last()->mOwner = mEntities.last();

    U32 bcnt = 1;

    while (bcnt > 0)
    {
        toker->advanceToken(true);

        if (toker->getToken()[0] == '{')
            bcnt++;
        if (toker->getToken()[0] == '}')
            bcnt--;

        // Look for the beginning of a brush
        if (toker->getToken()[0] == '(')
            parsePlane(toker);

        // Unable to support the newest Quake 3 format
        if (mBrushFormat == QuakeNew)
            return false;
    }

    return true;
}

bool InteriorMapResource::parseEntity(Tokenizer* toker)
{
    // In the .map format entities have this format:
    // {
    //    "property1 name" "property1 value"
    //    "property2 name" "property2 value"
    //    ...
    //    { // brush1 (optional - has to have at least 4 planes to be valid)
    //       <brush plane1>
    //       <brush plane2>
    //       <brush plane3>
    //       <brush plane4>
    //       ...
    //    }
    //    { // brush2 (optional - has to have at least 4 planes to be valid)
    //       <brush plane1>
    //       <brush plane2>
    //       <brush plane3>
    //       <brush plane4>
    //       ...
    //    }
    //    ...
    //    { // patch1 (optional - bezier patches in the q3 format)
    //       patchDef2
    //       {
    //          <texture/shader>
    //          ( dimensions )
    //          (
    //             ( <control point1> )
    //             ( <control point2> )
    //             ...
    //          )
    //       }
    //    }
    //    { // patch2 (optional - bezier patches in the q3 format)
    //       patchDef2
    //       {
    //          <texture/shader>
    //          ( dimensions )
    //          (
    //             ( <control point1> )
    //             ( <control point2> )
    //             ...
    //          )
    //       }
    //    }
    //    ...
    // }

    // Sanity check
    if (toker->getToken()[0] != '{')
    {
        Con::errorf("InteriorMapResource::parseEntity() not at the beginning of an entity - token is %s line %d", toker->getToken(), toker->getCurrentLine());
        return false;
    }

    // Add an entity to the entities list
    mEntities.increment();
    mEntities.last() = new Entity;

    Entity* ent = mEntities.last();
    ent->classname = StringTable->insert("unknown");

    // We are at brace level 1
    U32 bcnt = 1;

    // Loop through till we hit the ending brace
    while (bcnt > 0)
    {
        toker->advanceToken(true);

        if (toker->endOfFile())
            return true;

        //const char* tok = toker->getToken();

        if (toker->getToken()[0] == '{')
            bcnt++;
        if (toker->getToken()[0] == '}')
            bcnt--;

        // Properties are on the first brace level
        if (bcnt == 1 && toker->getToken()[0] != '}')
        {
            ent->properties.increment();
            ent->properties.last().name = StringTable->insert(toker->getToken());

            toker->advanceToken(false);

            ent->properties.last().value = StringTable->insert(toker->getToken());

            // Store the classname if we find it (just makes it easier to look up entities
            if (dStricmp(ent->properties.last().name, "classname") == 0)
                ent->classname = ent->properties.last().value;
        }
        else if (bcnt == 2 && toker->getToken()[0] == '{')  // We have found either a brush or a patch
        {
            // Determine whether this is a patch or a brush
            int sub = -1;

            toker->advanceToken(true);

            if (toker->tokenICmp("patchDef2"))
                sub = 0;
            else if (toker->tokenICmp("brushDef") || toker->getToken()[0] == '(')
                sub = 1;
            else
                Con::errorf("InteriorMapResource::parseEntity() unknown brush entity - token is %s line %d", toker->getToken(), toker->getCurrentLine());

            // Regress to the beginning of the subobject
            toker->regressToken(true);

            // Sanity check
            if (toker->getToken()[0] != '{')
                Con::errorf("InteriorMapResource::parseEntity() failed to return to the beginning of a subobject - token is %s line %d", toker->getToken(), toker->getCurrentLine());
            else
            {
                if (sub == 0)
                    parsePatch(toker);
                else if (dStricmp(ent->classname, "worldspawn") == 0)
                    parseBrush(toker, Structural);
                else if (dStricmp(ent->classname, "detail") == 0)
                    parseBrush(toker, Detail);
                else if (dStricmp(ent->classname, "collision") == 0)
                    parseBrush(toker, Collision);
                else if (dStricmp(ent->classname, "portal") == 0)
                    parseBrush(toker, Portal);
                else if (dStricmp(ent->classname, "trigger") == 0)
                    parseBrush(toker, Trigger);
                else
                    Con::errorf("InteriorMapResource::parseEntity() unknown entity %s with brushes", ent->classname);

                // Unable to support the newest Quake 3 format
                if (mBrushFormat == QuakeNew)
                    return false;

                // Loop to the end of the subobject
                while (toker->getToken()[0] != '}' && !toker->endOfFile())
                    toker->advanceToken(true);

                // Should return at the end of the subobject
                if (toker->getToken()[0] == '}')
                    bcnt--;
                else
                {
                    Con::errorf("InteriorMapResource::parseEntity() failure to parse to the end of a subobject");
                    Con::errorf("InteriorMapResource::parseEntity() entity is %s token is %s line %d", ent->classname, toker->getToken(), toker->getCurrentLine());
                }
            }
        }
    }

    // See if this entiry is our worldspawn
    if (dStricmp(ent->classname, "worldspawn") == 0)
    {
        mWorldSpawn = ent;

        // See if we have a brush scale
        char* scale = mWorldSpawn->getValue("geometry_scale");
        if (scale)
            mBrushScale = dAtof(scale);
    }

    // Sanity check
    if (toker->getToken()[0] != '}')
    {
        Con::errorf("InteriorMapResource::parseEntity() not at the end of an entity - token is %s line %d", toker->getToken(), toker->getCurrentLine());
        return false;
    }

    return true;
}

bool InteriorMapResource::validatePlane(Point3F k, Point3F j, Point3F l)
{
    F32 ax = k.x - j.x;
    F32 ay = k.y - j.y;
    F32 az = k.z - j.z;
    F32 bx = l.x - j.x;
    F32 by = l.y - j.y;
    F32 bz = l.z - j.z;
    F32 x = ay * bz - az * by;
    F32 y = az * bx - ax * bz;
    F32 z = ax * by - ay * bx;
    F32 squared = x * x + y * y + z * z;

    if (squared == 0.0f)
        return false;
    else
        return true;
}

bool InteriorMapResource::parsePlane(Tokenizer* toker)
{
    // We start at the first (
    F32 x, y, z;
    Point3F p0, p1, p2;

    // All of the formats share this in common
    toker->advanceToken(false);
    x = dAtof(toker->getToken());
    toker->advanceToken(false);
    y = dAtof(toker->getToken());
    toker->advanceToken(false);
    z = dAtof(toker->getToken());

    p0 = Point3F(x, y, z);

    // Brings us to end of the first ()
    toker->advanceToken(false);

    // Brings us to the beginning of the second ()
    toker->advanceToken(true);

    toker->advanceToken(false);
    x = dAtof(toker->getToken());
    toker->advanceToken(false);
    y = dAtof(toker->getToken());
    toker->advanceToken(false);
    z = dAtof(toker->getToken());

    p1 = Point3F(x, y, z);

    // Brings us to end of the second ()
    toker->advanceToken(false);

    // Brings us to the beginning of the third ()
    toker->advanceToken(true);

    toker->advanceToken(false);
    x = dAtof(toker->getToken());
    toker->advanceToken(false);
    y = dAtof(toker->getToken());
    toker->advanceToken(false);
    z = dAtof(toker->getToken());

    p2 = Point3F(x, y, z);

    // Brings us to end of the third ()
    toker->advanceToken(false);

    // So now we have enough to calculate our plane
    PlaneF normal;

    // Check to make sure that none of the points are the same
    if (!validatePlane(p0, p1, p2))
    {
        normal.set(1.0f, 0.0f, 0.0f);
        mBrushes.last()->mStatus = ConvexBrush::Malformed;
        mBrushes.last()->mDebugInfo = StringTable->insert("This brush was exported improperly with badly formed plane points. Please validate the brushes in your modelling tool before export.");
    }
    else
        normal.set(p0, p1, p2);

    // Here is where things start to split up by format
    // If we find a ( here then the map is in the newest
    // Quake 3 format. Otherwise we are in Quake/Valve format
    // which further splits

    // This the data that needs to be returned
    U32 texIndex;
    PlaneF texGens[2];
    F32 scale[2] = { 1.0f, 1.0f };

    // If we don't know what format we are using see if it is
    // the newest Quake 3 format.
    if (mBrushFormat == Unknown)
    {
        toker->advanceToken(false);

        if (toker->getToken()[0] == '(')
            mBrushFormat = QuakeNew;

        // Move back to the beginning of the texgen fields
        toker->regressToken(true);
    }

    if (mBrushFormat == QuakeNew)
    {
        // The newest Quake 3 format is not supported at this time
        // b/c its texgens are incompatible with map2dif's
        return false;

        //parseQuakeNew(toker, texIndex, texGens);
    }
    else
        parseQuakeValve(toker, normal, texIndex, texGens, scale);

    // Now that we have our data add it to the brush
    mBrushes.last()->addFace(normal, texGens, scale, texIndex);

    // ( 64 -64 -64 ) ( -64 -64 -64 ) ( -64 -64 64 ) WALL_PANEL01 [ 1 0 0 64 ] [ 0 0 -1 64 ] 0 1 1 
    //Con::warnf("( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s [ %g %g %g %g ] [ %g %g %g %g ] 0 %g %g",
    //               p0.x, p0.y, p0.z,
    //               p1.x, p1.y, p1.z,
    //               p2.x, p2.y, p2.z,
    //               mMaterials[texIndex],
    //               texGens[0].x, texGens[0].y, texGens[0].z, texGens[0].d,
    //               texGens[1].x, texGens[1].y, texGens[1].z, texGens[1].d,
    //               scale[0], scale[1]);

    return true;
}

bool InteriorMapResource::parseQuakeValve(Tokenizer* toker, VectorF normal, U32& tdx, PlaneF* texGens, F32* scale)
{
    // Brings us to the texture name
    toker->advanceToken(false);

    tdx = addTexture((char*)toker->getToken());

    // Ok here is where things start to differ between the formats
    // If we haven't determined the format yet then check for a '['
    if (mBrushFormat == Unknown)
    {
        toker->advanceToken(false);

        if (toker->getToken()[0] == '[')
            mBrushFormat = Valve220;
        else
            mBrushFormat = QuakeOld;

        // Move back to the beginning of the texgen fields
        toker->regressToken(true);
    }

    if (mBrushFormat == Valve220)
        parseValve220TexGens(toker, texGens, scale);
    else if (mBrushFormat == QuakeOld)
        parseQuakeTexGens(toker, normal, texGens, scale);

    return true;
}

bool InteriorMapResource::parseQuakeNew(Tokenizer* toker, U32& tdx, PlaneF* texGens, F32* scale)
{
    // Brings us to the beginning of the outer ()
    toker->advanceToken(false);

    // Brings us to the beginning of the next ()
    toker->advanceToken(false);

    // MDFFIX:: figure out what these are
    toker->advanceToken(false);
    toker->advanceToken(false);
    toker->advanceToken(false);

    // Brings us to the end of ()
    toker->advanceToken(false);

    // Brings us to the beginning of the next ()
    toker->advanceToken(false);

    // MDFFIX:: figure out what these are
    toker->advanceToken(false);
    toker->advanceToken(false);
    toker->advanceToken(false);

    // Brings us to the end of ()
    toker->advanceToken(false);

    // Brings us to the end of the outer ()
    toker->advanceToken(false);

    // Brings us to the texture
    toker->advanceToken(false);

    tdx = addTexture((char*)toker->getToken());

    // MDFFIX:: figure out what these are
    toker->advanceToken(false);
    toker->advanceToken(false);
    toker->advanceToken(false);

    return true;
}

bool InteriorMapResource::parseValve220TexGens(Tokenizer* toker, PlaneF* texGens, F32* scale)
{
    // Brings us to the opening of the first []
    toker->advanceToken(false);

    toker->advanceToken(false);
    texGens[0].x = dAtof(toker->getToken());
    toker->advanceToken(false);
    texGens[0].y = dAtof(toker->getToken());
    toker->advanceToken(false);
    texGens[0].z = dAtof(toker->getToken());
    toker->advanceToken(false);
    texGens[0].d = dAtof(toker->getToken());

    // Takes us to the close of the first []
    toker->advanceToken(false);
    // Brings us to the opening of the second []
    toker->advanceToken(false);

    toker->advanceToken(false);
    texGens[1].x = dAtof(toker->getToken());
    toker->advanceToken(false);
    texGens[1].y = dAtof(toker->getToken());
    toker->advanceToken(false);
    texGens[1].z = dAtof(toker->getToken());
    toker->advanceToken(false);
    texGens[1].d = dAtof(toker->getToken());

    // Takes us to the close of the second []
    toker->advanceToken(false);

    // Skip rotation
    toker->advanceToken(false);

    // Scale
    toker->advanceToken(false);
    scale[0] = dAtof(toker->getToken());
    toker->advanceToken(false);
    scale[1] = dAtof(toker->getToken());

    texGens[0].x /= scale[0];
    texGens[0].y /= scale[0];
    texGens[0].z /= scale[0];

    texGens[1].x /= scale[1];
    texGens[1].y /= scale[1];
    texGens[1].z /= scale[1];

    return true;
}

F32	baseaxis[18][3] =
{
   {0,0,1}, {1,0,0}, {0,-1,0},			// floor
   {0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
   {1,0,0}, {0,1,0}, {0,0,-1},			// west wall
   {-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
   {0,1,0}, {1,0,0}, {0,0,-1},			// south wall
   {0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};

void textureAxisFromPlane(const VectorF& normal, F32* xv, F32* yv)
{
    int		bestaxis;
    F32		dot, best;
    int		i;

    best = 0;
    bestaxis = 0;

    for (i = 0; i < 6; i++) {
        dot = mDot(normal, VectorF(baseaxis[i * 3][0], baseaxis[i * 3][1], baseaxis[i * 3][2]));
        if (dot > best)
        {
            best = dot;
            bestaxis = i;
        }
    }

    xv[0] = baseaxis[bestaxis * 3 + 1][0];
    xv[1] = baseaxis[bestaxis * 3 + 1][1];
    xv[2] = baseaxis[bestaxis * 3 + 1][2];
    yv[0] = baseaxis[bestaxis * 3 + 2][0];
    yv[1] = baseaxis[bestaxis * 3 + 2][1];
    yv[2] = baseaxis[bestaxis * 3 + 2][2];
}

void quakeTextureVecs(const VectorF& normal,
    F32 offsetX,
    F32 offsetY,
    F32 rotate,
    F32 scaleX,
    F32 scaleY,
    PlaneF* u,
    PlaneF* v)
{
    F32   vecs[2][3];
    int   sv, tv;
    F32   ang, sinv, cosv;
    F32   ns, nt;
    int   i, j;

    textureAxisFromPlane(normal, vecs[0], vecs[1]);

    ang = rotate / 180 * M_PI;
    sinv = sin(ang);
    cosv = cos(ang);

    if (vecs[0][0] != 0.0)
        sv = 0;
    else if (vecs[0][1] != 0.0)
        sv = 1;
    else
        sv = 2;

    if (vecs[1][0] != 0.0)
        tv = 0;
    else if (vecs[1][1] != 0.0)
        tv = 1;
    else
        tv = 2;

    for (i = 0; i < 2; i++) {
        ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
        nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
        vecs[i][sv] = ns;
        vecs[i][tv] = nt;
    }

    u->x = vecs[0][0] / scaleX;
    u->y = vecs[0][1] / scaleX;
    u->z = vecs[0][2] / scaleX;
    u->d = offsetX;

    v->x = vecs[1][0] / scaleY;
    v->y = vecs[1][1] / scaleY;
    v->z = vecs[1][2] / scaleY;
    v->d = offsetY;
}

bool InteriorMapResource::parseQuakeTexGens(Tokenizer* toker, VectorF normal, PlaneF* texGens, F32* scale)
{
    toker->advanceToken(false);
    F32 shiftU = dAtof(toker->getToken());
    toker->advanceToken(false);
    F32 shiftV = dAtof(toker->getToken());

    toker->advanceToken(false);
    F32 rot = dAtof(toker->getToken());

    toker->advanceToken(false);
    scale[0] = dAtof(toker->getToken());
    toker->advanceToken(false);
    scale[1] = dAtof(toker->getToken());

    // Make sure the normal is normalized
    normal.normalize();

    quakeTextureVecs(normal, shiftU, shiftV, rot, scale[0], scale[1], &texGens[0], &texGens[1]);

    return true;
}

U32 InteriorMapResource::addTexture(char* texture)
{
    S32 idx = getTextureIndex(texture);

    if (idx == -1)
    {
        idx = mMaterials.size();
        mMaterials.increment();
        mMaterials.last() = StringTable->insert(texture);
    }

    return idx;
}

S32 InteriorMapResource::getTextureIndex(char* texture)
{
    S32 idx = -1;

    for (U32 i = 0; i < mMaterials.size(); i++)
    {
        if (dStricmp(texture, mMaterials[i]) == 0)
        {
            idx = i;
            break;
        }
    }

    return idx;
}

char* InteriorMapResource::getTextureName(U32 texIdx)
{
    if (mMaterials.size() == 0 || texIdx > mMaterials.size() - 1)
    {
        Con::warnf("InteriorMapResource::getTextureName(): texture index is outside of range");

        return "error";
    }

    return (char*)mMaterials[texIdx];
}

ResourceInstance* constructInteriorMAP(Stream& stream)
{
    InteriorMapResource* pResource = new InteriorMapResource;

    if (pResource->read(stream) == true)
        return pResource;
    else
    {
        delete pResource;
        return NULL;
    }
}
