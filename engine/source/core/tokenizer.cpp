//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/tokenizer.h"
#include "platform/platform.h"
#include "core/fileStream.h"
#include "core/resManager.h"

Tokenizer::Tokenizer()
{
    dMemset(mFileName, 0, sizeof(mFileName));

    mpBuffer = NULL;
    mBufferSize = 0;

    mCurrPos = 0;
    mCurrLine = 0;
    mPrevPos = 0;
    mStartPos = 0;

    dMemset(mCurrTokenBuffer, 0, sizeof(mCurrTokenBuffer));
    mTokenIsCurrent = false;
}

Tokenizer::~Tokenizer()
{
    dMemset(mFileName, 0, sizeof(mFileName));

    delete[] mpBuffer;
    mpBuffer = NULL;
    mBufferSize = 0;

    mCurrPos = 0;
    mCurrLine = 0;
    mPrevPos = 0;
    mStartPos = 0;

    dMemset(mCurrTokenBuffer, 0, sizeof(mCurrTokenBuffer));
    mTokenIsCurrent = false;
}

bool Tokenizer::openFile(const char* pFileName)
{
    AssertFatal(mFileName[0] == '\0', "Reuse of Tokenizers not allowed!");

    Stream* pStream = ResourceManager->openStream(pFileName);
    //FileStream* pStream = new FileStream;
    //if (pStream->open(pFileName, FileStream::Read) == false)
    if (!pStream || pStream->getStatus() != Stream::Ok)
    {
        delete pStream;
        return false;
    }
    dStrcpy(mFileName, pFileName);

    mBufferSize = pStream->getStreamSize();
    mpBuffer = new char[mBufferSize];
    pStream->read(mBufferSize, mpBuffer);
    //pStream->close();
    delete pStream;

    // Not really necessary, but couldn't hurt...
    mCurrPos = 0;
    mCurrLine = 0;

    dMemset(mCurrTokenBuffer, 0, sizeof(mCurrTokenBuffer));
    mTokenIsCurrent = false;

    return true;
}

bool Tokenizer::openFile(Stream* pStream)
{
    mBufferSize = pStream->getStreamSize();
    mpBuffer = new char[mBufferSize];
    pStream->read(mBufferSize, mpBuffer);
    //pStream->close();
    //delete pStream;

    // Not really necessary, but couldn't hurt...
    mCurrPos = 0;
    mCurrLine = 0;

    dMemset(mCurrTokenBuffer, 0, sizeof(mCurrTokenBuffer));
    mTokenIsCurrent = false;

    return true;
}

bool Tokenizer::reset()
{
    mCurrPos = 0;
    mCurrLine = 0;
    mPrevPos = 0;
    mStartPos = 0;

    dMemset(mCurrTokenBuffer, 0, sizeof(mCurrTokenBuffer));
    mTokenIsCurrent = false;

    return true;
}

bool Tokenizer::setCurrentPos(U32 pos)
{
    mCurrPos = pos;
    mTokenIsCurrent = false;

    return advanceToken(true);
}

bool Tokenizer::advanceToken(const bool crossLine, const bool assertAvail)
{
    if (mTokenIsCurrent == true)
    {
        AssertFatal(mCurrTokenBuffer[0] != '\0', "No token, but marked as current?");
        mTokenIsCurrent = false;
        return true;
    }

    U32 currPosition = 0;
    mCurrTokenBuffer[0] = '\0';

    // Store the beginning of the previous advance
    // and the beginning of the current advance
    mPrevPos = mStartPos;
    mStartPos = mCurrPos;

    while (mCurrPos < mBufferSize)
    {
        char c = mpBuffer[mCurrPos];

        bool cont = true;
        switch (c)
        {
        case ' ':
        case '\t':
            if (currPosition == 0)
            {
                // Token hasn't started yet...
                mCurrPos++;
            }
            else
            {
                // End of token
                mCurrPos++;
                cont = false;
            }
            break;

        case '\r':
        case '\n':
            if (crossLine == true)
            {
                if (currPosition == 0)
                {
                    // Haven't started getting token, but we're crossing lines...
                    while (mCurrPos < mBufferSize && (mpBuffer[mCurrPos] == '\r' || mpBuffer[mCurrPos] == '\n'))
                        mCurrPos++;

                    mCurrLine++;
                }
                else
                {
                    // Getting token, stop here, leave pointer at newline...
                    cont = false;
                }
            }
            else
            {
                cont = false;
                break;
            }
            break;

        default:
            if (c == '\"')
            {
                // Quoted token
                //AssertISV(currPosition == 0,
                //          avar("Error, quotes MUST be at start of token.  Error: (%s: %d)",
                //               getFileName(), getCurrentLine()));

                U32 startLine = getCurrentLine();
                mCurrPos++;

                while (mpBuffer[mCurrPos] != '\"') {
                    AssertISV(mCurrPos < mBufferSize,
                        avar("End of file before quote closed.  Quote started: (%s: %d)",
                            getFileName(), startLine));
                    AssertISV((mpBuffer[mCurrPos] != '\n' && mpBuffer[mCurrPos] != '\r'),
                        avar("End of line reached before end of quote.  Quote started: (%s: %d)",
                            getFileName(), startLine));

                    mCurrTokenBuffer[currPosition++] = mpBuffer[mCurrPos++];
                }

                mCurrPos++;
                cont = false;
            }
            else if (c == '/' && mpBuffer[mCurrPos + 1] == '/')
            {
                // Line quote...
                if (currPosition == 0)
                {
                    // continue to end of line, then let crossLine determine on the next pass
                    while (mCurrPos < mBufferSize && (mpBuffer[mCurrPos] != '\n' && mpBuffer[mCurrPos] != '\r'))
                        mCurrPos++;
                }
                else
                {
                    // This is the end of the token.  Continue to EOL
                    while (mCurrPos < mBufferSize && (mpBuffer[mCurrPos] != '\n' && mpBuffer[mCurrPos] != '\r'))
                        mCurrPos++;
                    cont = false;
                }
            }
            else
            {
                mCurrTokenBuffer[currPosition++] = c;
                mCurrPos++;
            }
            break;
        }

        if (cont == false)
            break;
    }

    mCurrTokenBuffer[currPosition] = '\0';

    if (assertAvail == true)
        AssertISV(currPosition != 0, avar("Error parsing: %s at or around line: %d", getFileName(), getCurrentLine()));

    if (mCurrPos == mBufferSize)
        return false;

    return currPosition != 0;
}

bool Tokenizer::regressToken(const bool crossLine)
{
    AssertFatal(mTokenIsCurrent == false && mCurrPos != 0,
        "Error, token already regressed, or no token has been taken yet...");

    return setCurrentPos(mPrevPos);
}

bool Tokenizer::tokenAvailable()
{
    // Note: this implies that when advanceToken(false) fails, it must cap the
    //        token buffer.
    //
    return mCurrTokenBuffer[0] != '\0';
}

const char* Tokenizer::getToken() const
{
    return mCurrTokenBuffer;
}

bool Tokenizer::tokenICmp(const char* pCmp) const
{
    return dStricmp(mCurrTokenBuffer, pCmp) == 0;
}

bool Tokenizer::findToken(U32 start, const char* pCmp)
{
    // Move to the start
    setCurrentPos(start);

    // Loop through the file and see if the token exists
    while (advanceToken(true))
    {
        if (tokenICmp(pCmp))
            return true;
    }

    return false;
}

bool Tokenizer::findToken(const char* pCmp)
{
    return findToken(0, pCmp);
}

bool Tokenizer::endOfFile()
{
    if (mCurrPos < mBufferSize)
        return false;
    else
        return true;
}
