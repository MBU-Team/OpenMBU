//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
/// \file lang.h
/// \brief Header for language support
//////////////////////////////////////////////////////////////////////////

#include "console/simBase.h"
#include "core/tVector.h"

#ifndef _LANG_H_
#define _LANG_H_

#define LANG_INVALID_ID		0xffffffff		///!< Invalid ID. Used for returning failure

//////////////////////////////////////////////////////////////////////////
/// \brief Class for working with language files
//////////////////////////////////////////////////////////////////////////
class LangFile
{
protected:
    Vector<UTF8*> mStringTable;
    UTF8* mLangName;
    UTF8* mLangFile;

    void freeTable();

public:
    LangFile(const UTF8* langName = NULL);
    virtual ~LangFile();

    bool load(const UTF8* filename);
    bool save(const UTF8* filename);

    bool load(Stream* s);
    bool save(Stream* s);

    const UTF8* getString(U32 id);
    U32 addString(const UTF8* str);

    // [tom, 4/22/2005] setString() added to help the language compiler a bit
    void setString(U32 id, const UTF8* str);

    void setLangName(const UTF8* newName);
    const UTF8* getLangName(void) { return mLangName; }
    const UTF8* getLangFile(void) { return mLangFile; }

    void setLangFile(const UTF8* langFile);
    bool activateLanguage(void);
    void deactivateLanguage(void);

    bool isLoaded(void) { return mStringTable.size() > 0; }

    S32 getNumStrings(void) { return mStringTable.size(); }
};

//////////////////////////////////////////////////////////////////////////
/// \brief Language file table
//////////////////////////////////////////////////////////////////////////
class LangTable : public SimObject
{
    typedef SimObject Parent;

protected:
    Vector<LangFile*> mLangTable;
    S32 mDefaultLang;
    S32 mCurrentLang;

public:
    DECLARE_CONOBJECT(LangTable);

    LangTable();
    virtual ~LangTable();

    S32 addLanguage(LangFile* lang, const UTF8* name = NULL);
    S32 addLanguage(const UTF8* filename, const UTF8* name = NULL);

    void setDefaultLanguage(S32 langid);
    void setCurrentLanguage(S32 langid);
    S32 getCurrentLanguage(void) { return mCurrentLang; }

    const UTF8* getLangName(const S32 langid) const
    {
        if (langid < 0 || langid >= mLangTable.size())
            return NULL;
        return mLangTable[langid]->getLangName();
    }

    const S32 getNumLang(void) const { return mLangTable.size(); }

    const UTF8* getString(const U32 id) const;
    const U32 getStringLength(const U32 id) const;
};

extern UTF8* sanitiseVarName(const UTF8* varName, UTF8* buffer, U32 bufsize);
extern UTF8* getCurrentModVarName(UTF8* buffer, U32 bufsize);
extern const LangTable* getCurrentModLangTable();
extern const LangTable* getModLangTable(const UTF8* mod);

#endif // _LANG_H_
