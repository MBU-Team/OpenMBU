//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "game/badWordFilter.h"

BadWordFilter *gBadWordFilter = NULL;
bool BadWordFilter::filteringEnabled = true;

BadWordFilter::BadWordFilter()
{
   dStrcpy(defaultReplaceStr, "knqwrtlzs");
   filterTables.push_back(new FilterTable);
   curOffset = 0;
}

BadWordFilter::~BadWordFilter()
{
   for(U32 i = 0; i < filterTables.size(); i++)
      delete filterTables[i];
}

void BadWordFilter::create()
{
   Con::addVariable("pref::enableBadWordFilter", TypeBool, &filteringEnabled);
   gBadWordFilter = new BadWordFilter;
   gBadWordFilter->addBadWord("shit");
   gBadWordFilter->addBadWord("fuck");
   gBadWordFilter->addBadWord("cock");
   gBadWordFilter->addBadWord("bitch");
   gBadWordFilter->addBadWord("cunt");
   gBadWordFilter->addBadWord("nigger");
   gBadWordFilter->addBadWord("bastard");
   gBadWordFilter->addBadWord("dick");
   gBadWordFilter->addBadWord("whore");
   gBadWordFilter->addBadWord("goddamn");
   gBadWordFilter->addBadWord("asshole");
}

void BadWordFilter::destroy()
{
   delete gBadWordFilter;
   gBadWordFilter = NULL;
}


U8 BadWordFilter::remapTable[257] = "------------------------------------------------OI---------------ABCDEFGHIJKLMNOPQRSTUVWXYZ------ABCDEFGHIJKLMNOPQRSTUVWXYZ-----C--F--TT--S-C-Z-----------S-C-ZY--CLOY-S-CA---R---UT-UP--IO-----AAAAAAACEEEEIIIIDNOOOOOXOUUUUYDBAAAAAAACEEEEIIIIDNOOOOO-OUUUUYDY";
U8 BadWordFilter::randomJunk[MaxBadwordLength+1] = "REMsg rk34n4ksqow;xnskq;KQoaWnZa";

BadWordFilter::FilterTable::FilterTable()
{
   for(U32 i = 0; i < 26; i++)
      nextState[i] = TerminateNotFound;
}

bool BadWordFilter::addBadWord(const char *cword)
{
   FilterTable *curFilterTable = filterTables[0];
   // prescan the word to see if it has any skip chars
   const U8 *word = (const U8 *) cword;
   const U8 *walk = word;
   if(dStrlen(cword) > MaxBadwordLength)
      return false;
   while(*walk)
   {
      if(remapTable[*walk] == '-')
         return false;
      walk++;
   }
   while(*word)
   {
      U8 remap = remapTable[*word] - 'A';
      U16 state = curFilterTable->nextState[remap];

      if(state < TerminateNotFound)
      {
         // this character is already in the state table...
         curFilterTable = filterTables[state];
      }
      else if(state == TerminateFound)
      {
         // a subset of this word is already in the table...
         // exit out.
         return false;
      }
      else if(state == TerminateNotFound)
      {
         if(word[1])
         {
            curFilterTable->nextState[remap] = filterTables.size();
            filterTables.push_back(new FilterTable);
            curFilterTable = filterTables[filterTables.size() - 1];
         }
         else
            curFilterTable->nextState[remap] = TerminateFound;
      }
      word++;
   }
   return true;
}

bool BadWordFilter::setDefaultReplaceStr(const char *str)
{
   U32 len = dStrlen(str);
   if(len < 2 || len >= sizeof(defaultReplaceStr))
      return false;
   dStrcpy(defaultReplaceStr, str);
   return true;
}

void BadWordFilter::filterString(char *cstring, const char *replaceStr)
{
   if(!replaceStr)
      replaceStr = defaultReplaceStr;
   U8 *string = (U8 *) cstring;
   U8 *starts[MaxBadwordLength];
   U8 *curStart = string;
   U32 replaceLen = dStrlen(replaceStr);
   while(*curStart)
   {
      FilterTable *curFilterTable = filterTables[0];
      S32 index = 0;
      U8 *walk = curStart;
      while(*walk)
      {
         U8 remap = remapTable[*walk];
         if(remap != '-')
         {
            starts[index++] = walk;
            U16 table = curFilterTable->nextState[remap - 'A'];
            if(table < TerminateNotFound)
               curFilterTable = filterTables[table];
            else if(table == TerminateNotFound)
            {
               curStart++;
               break;
            }
            else // terminate found
            {
               for(U32 i = 0; i < index; i++)
               {
                  starts[i][0] = (U8 )replaceStr[curOffset % replaceLen];
                  curOffset += randomJunk[curOffset & (MaxBadwordLength - 1)];
               }
               curStart = walk + 1;
               break;
            }
         }
         walk++;
      }
      if(!*walk)
         curStart++;
   }
}

bool BadWordFilter::containsBadWords(const char *cstring)
{
   U8 *string = (U8 *) cstring;
   U8 *curStart = string;
   while(*curStart)
   {
      FilterTable *curFilterTable = filterTables[0];
      S32 index = 0;
      U8 *walk = curStart;
      while(*walk)
      {
         U8 remap = remapTable[*walk];
         if(remap != '-')
         {
            U16 table = curFilterTable->nextState[remap - 'A'];
            if(table < TerminateNotFound)
               curFilterTable = filterTables[table];
            else if(table == TerminateNotFound)
            {
               curStart++;
               break;
            }
            else // terminate found
               return true;
         }
         walk++;
      }
      if(!*walk)
         curStart++;
   }
   return false;
}

ConsoleFunction(addBadWord, bool, 2, 2, "addBadWord(someReallyNastyWord);")
{
   argc;
   return gBadWordFilter->addBadWord(argv[1]);
}

ConsoleFunction(filterString, const char *, 2, 3, "filterString(baseString,replacementChars);")
{
   const char *replaceStr = NULL;
   if(argc == 3)
      replaceStr = argv[2];

   char *ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1);
   dStrcpy(ret, argv[1]);
   gBadWordFilter->filterString(ret, replaceStr);
   return ret;
}

ConsoleFunction(containsBadWords, bool, 2, 2, "containsBadWords(text);")
{
   argc;
   return gBadWordFilter->containsBadWords(argv[1]);
}