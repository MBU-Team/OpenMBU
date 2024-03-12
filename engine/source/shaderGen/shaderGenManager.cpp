//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "shaderGen/shaderGenManager.h"
#include "core/fileStream.h"
#include "core/memstream.h"
#include <core/resManager.h>

Vector<TrackedAutoGenShader> ShaderGenManager::_mTrackedShaders;
Stream* ShaderGenManager::_mOpenStream = NULL;
Stream* ShaderGenManager::_mOpenReadStream = NULL;

//------------------------------------------------------------------------------

Stream* ShaderGenManager::openNewShaderStream(const char* fileName, bool shaderInMemory)
{
    AssertFatal(_mOpenStream == NULL, "Stream still open! Close that shit!");

#if defined(_XBOX)
    shaderInMemory = true;
#endif

    _mTrackedShaders.increment();
    _mTrackedShaders.last().shaderInMemory = shaderInMemory;
    _mTrackedShaders.last().fileName = StringTable->insert(fileName);

    if (shaderInMemory)
    {
        _mTrackedShaders.last().shaderData = new char[DEFAULT_BUFFER_SIZE];
        _mTrackedShaders.last().shaderDataSize = DEFAULT_BUFFER_SIZE;
        MemStream* ms = new MemStream(DEFAULT_BUFFER_SIZE, _mTrackedShaders.last().shaderData);
        _mOpenStream = ms;
    }
    else
    {
        FileStream* fs = new FileStream();

        if (fs->open(fileName, FileStream::Write))
        {
            _mOpenStream = fs;
        }
        else
        {
            delete fs;

            _mOpenStream = NULL;
        }
    }

    return _mOpenStream;
}

//------------------------------------------------------------------------------

void ShaderGenManager::closeNewShaderStream(Stream* s)
{
    AssertFatal(s == _mOpenStream, "This stream was not opened by the ShaderGenManager");

    if (_mTrackedShaders.last().shaderInMemory)
    {
        // Trim the memory
        U32 pos = s->getPosition();
        dRealloc(_mTrackedShaders.last().shaderData, pos);
        _mTrackedShaders.last().shaderDataSize = pos;
    }

    delete s;
    _mOpenStream = NULL;
}

//------------------------------------------------------------------------------

Stream* ShaderGenManager::readShaderStream(const char* fileName)
{
    AssertFatal(_mOpenReadStream == NULL, "Stream still open! Close that shit!");

    for (Vector<TrackedAutoGenShader>::iterator i = _mTrackedShaders.begin();
        i != _mTrackedShaders.end(); i++)
    {
        if (dStrcmp((*i).fileName, fileName) == 0)
        {
            if ((*i).shaderInMemory)
            {
                MemStream* ms = new MemStream((*i).shaderDataSize, (*i).shaderData);
                _mOpenReadStream = ms;
                break;
            }
            else
            {
                _mOpenReadStream = ResourceManager->openStream(fileName);
                break;
            }
        }
    }

    return _mOpenReadStream;
}

//------------------------------------------------------------------------------

void ShaderGenManager::closeShaderStream(Stream* s)
{
    AssertFatal(s == _mOpenReadStream, "This stream was not opened by the ShaderGenManager");
    delete _mOpenReadStream;
    _mOpenReadStream = NULL;
}