//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"

#include "console/ast.h"
#include "core/tAlgorithm.h"
#include "core/resManager.h"

#include "core/findMatch.h"
#include "console/consoleInternal.h"
#include "console/consoleObject.h"
#include "core/fileStream.h"
#include "console/compiler.h"

//--- Information pertaining to this page... ------------------
/// @file
///
/// For specifics on using the consoleDoc functionality, see @ref console_autodoc

ConsoleFunctionGroupBegin(ConsoleDoc, "Console self-documentation functions. These output psuedo C++ suitable for feeeding through Doxygen or another auto documentation tool.");

ConsoleFunction(dumpConsoleClasses, void, 1, 1, "() dumps all declared console classes to the console.")
{
   Namespace::dumpClasses();
}

ConsoleFunction(dumpConsoleFunctions, void, 1, 1, "() dumps all declared console functions to the console.")
{
   Namespace::dumpFunctions();
}

ConsoleFunctionGroupEnd(ConsoleDoc);

/// Helper table to convert type ids to human readable names.
const char *typeNames[] = 
{
      "Script",
      "string",
      "int",
      "float",
      "void",
      "bool",
      "",
      "",
      "unknown_overload"
};

void printClassHeader(const char * className, const char * superClassName, const bool stub)
{
   if(stub) 
   {
      Con::printf("/// Stub class");
      Con::printf("/// ");
      Con::printf("/// @note This is a stub class to ensure a proper class hierarchy. No ");
      Con::printf("///       information was available for this class.");
   }

   // Print out appropriate class header
   if(superClassName)
      Con::printf("class  %s : public %s {", className, superClassName ? superClassName : "");
   else if(!className)
      Con::printf("namespace Global {");
   else
      Con::printf("class  %s {", className);

   if(className)
      Con::printf("  public:");

}

void printClassMethod(const bool isVirtual, const char *retType, const char *methodName, const char* args, const char*usage)
{
   if(usage && usage[0] != ';' && usage[0] != 0)
      Con::printf("   /*! %s */", usage);
   Con::printf("   %s%s %s(%s) {}", isVirtual ? "virtual " : "", retType, methodName, args);
}

void printGroupStart(const char * aName, const char * aDocs)
{
   Con::printf("");
   Con::printf("   /*! @name %s", aName);

   if(aDocs)
   {
      Con::printf("   ");
      Con::printf("   %s", aDocs);
   }

   Con::printf("   @{ */");
}

void printClassMember(const bool isDeprec, const char * aType, const char * aName, const char * aDocs)
{
   Con::printf("   /*!");

   if(aDocs)
   {
      Con::printf("   %s", aDocs);
      Con::printf("   ");
   }

   if(isDeprec)
      Con::printf("   @deprecated This member is deprecated, which means that its value is always undefined.");

   Con::printf("    */");

   Con::printf("   %s %s;", isDeprec ? "deprecated" : aType, aName);
}

void printGroupEnd()
{
   Con::printf("   /// @}");
   Con::printf("");
}

void printClassFooter()
{
   Con::printf("};");
   Con::printf("");
}

void Namespace::printNamespaceEntries(Namespace * g)
{
   static bool inGroup = false;

   // Go through all the entries.
   // Iterate through the methods of the namespace...
   for(Entry *ewalk = g->mEntryList; ewalk; ewalk = ewalk->mNext)
   {
      char buffer[1024]; //< This will bite you in the butt someday.
      int eType = ewalk->mType;
      const char * funcName = ewalk->mFunctionName;

      // If it's a function
      if(eType >= Entry::ScriptFunctionType || eType == Entry::OverloadMarker)
      {
         if(eType==Entry::OverloadMarker) 
         {
            // Deal with crap from the OverloadMarker case.
            // It has no type information so we have to "correct" its type.

            // Find the original
            eType = 8;
            for(Entry *eseek = g->mEntryList; eseek; eseek = eseek->mNext)
            {
               if(!dStrcmp(eseek->mFunctionName, ewalk->cb.mGroupName))
               {
                  eType = eseek->mType;
                  break;
               }
            }
            // And correct the name
            funcName = ewalk->cb.mGroupName;
         }

         // A quick note  - if your usage field starts with a (, then it's auto-integrated into
         // the script docs! Use this HEAVILY!

         // We add some heuristics here as well. If you're of the form:
         // *.methodName(*)
         // then we will also extract parameters.

         const char *use = ewalk->mUsage ? ewalk->mUsage : "";
         const char *bgn = dStrchr(use, '(');
         const char *end = dStrchr(use, ')');
         const char *dot = dStrchr(use, '.');

         if(use[0] == '(')
         {
            if(!end)
               end = use + 1;

            use++;
            
            U32 len = end - use;
            dStrncpy(buffer, use, len);
            buffer[len] = 0;

            printClassMethod(true, typeNames[eType], funcName, buffer, end+1);

            continue; // Skip to next one.
         }

         // We check to see if they're giving a prototype.
         if(dot && bgn && end)       // If there's two parentheses, and a dot...
         if(dot < bgn && bgn < end)  // And they're in the order dot, bgn, end...
         {
            use++;
            U32 len = end - bgn - 1;
            dStrncpy(buffer, bgn+1, len);
            buffer[len] = 0;

            // Then let's do the heuristic-trick
            printClassMethod(true, typeNames[eType], funcName, buffer, end+1);
            continue; // Get to next item.
         }

         // Finally, see if they did it foo(*) style.
         char* func_pos = dStrstr(use, funcName);
         if((func_pos) && (func_pos < bgn) && (end > bgn))
         {
            U32 len = end - bgn - 1;
            dStrncpy(buffer, bgn+1, len);
            buffer[len] = 0;

            printClassMethod(true, typeNames[eType], funcName, buffer, end+1);
            continue;
         }

         // Default...
         printClassMethod(true, typeNames[eType], funcName, "", ewalk->mUsage);
      }
      else if(ewalk->mType == Entry::GroupMarker)
      {
         if(!inGroup)
            printGroupStart(ewalk->cb.mGroupName, ewalk->mUsage);
         else 
            printGroupEnd();

         inGroup = !inGroup;
      }
      else if(ewalk->mFunctionOffset)                 // If it's a builtin function...
      {
         ewalk->mCode->getFunctionArgs(buffer, ewalk->mFunctionOffset);
         printClassMethod(false, typeNames[ewalk->mType], ewalk->mFunctionName, buffer, "");
      }
      else
      {
         Con::printf("   // got an unknown thing?? %d", ewalk->mType );
      }
   }

}

void Namespace::dumpClasses()
{
   Vector<Namespace *> vec;
   trashCache();

   // We use mHashSequence to mark if we have traversed...
   // so mark all as zero to start.
   for(Namespace *walk = mNamespaceList; walk; walk = walk->mNext)
      walk->mHashSequence = 0;

   for(Namespace *walk = mNamespaceList; walk; walk = walk->mNext)
   {
      Vector<Namespace *> stack;

      // Get all the parents of this namespace... (and mark them as we go)
      Namespace *parentWalk = walk;
      while(parentWalk)
      {
         if(parentWalk->mHashSequence != 0)
            break;
         if(parentWalk->mPackage == 0)
         {
            parentWalk->mHashSequence = 1;   // Mark as traversed.
            stack.push_back(parentWalk);
         }
         parentWalk = parentWalk->mParent;
      }

      // Load stack into our results vector.
      while(stack.size())
      {
         vec.push_back(stack[stack.size() - 1]);
         stack.pop_back();
      }
   }

   // Go through previously discovered classes
   U32 i;
   for(i = 0; i < vec.size(); i++)
   {
      const char *className = vec[i]->mName;
      const char *superClassName = vec[i]->mParent ? vec[i]->mParent->mName : NULL;

      // Skip the global namespace, that gets dealt with in dumpFunctions
      if(!className) continue;

      // If we hit a class with no members and no classRep, do clever filtering.
      if(vec[i]->mEntryList == NULL && vec[i]->mClassRep == NULL)
      {
         // Print out a short stub so we get a proper class hierarchy.
         if(superClassName) { // Filter hack; we don't want non-inheriting classes...
            printClassHeader(className,superClassName, true);
            printClassFooter();
         }
         continue;
      }

      // Print the header for the class..
      printClassHeader(className, superClassName, false);

      // Deal with entries.
      printNamespaceEntries(vec[i]);

      // Deal with the classRep (to get members)...
      AbstractClassRep *rep = vec[i]->mClassRep;
      AbstractClassRep::FieldList emptyList;
      AbstractClassRep::FieldList *parentList = &emptyList;
      AbstractClassRep::FieldList *fieldList = &emptyList;

      if(rep)
      {
         // Get information about the parent's fields...
         AbstractClassRep *parentRep = vec[i]->mParent ? vec[i]->mParent->mClassRep : NULL;
         if(parentRep)
            parentList = &(parentRep->mFieldList);

         // Get information about our fields
         fieldList = &(rep->mFieldList);

         // Go through all our fields...
         for(U32 j = 0; j < fieldList->size(); j++)
         {
            switch((*fieldList)[j].type)
            {
            case AbstractClassRep::StartGroupFieldType:
               printGroupStart((*fieldList)[j].pGroupname, (*fieldList)[j].pFieldDocs);
               break;
            case AbstractClassRep::EndGroupFieldType:
               printGroupEnd();
               break;
            default:
            case AbstractClassRep::DepricatedFieldType:
               {
                  bool isDeprecated = ((*fieldList)[j].type == AbstractClassRep::DepricatedFieldType);

                  if(isDeprecated)
                  {
                     printClassMember(
                        true,
                        "<deprecated>",
                        (*fieldList)[j].pFieldname,
                        (*fieldList)[j].pFieldDocs
                        );
                  }
                  else
                  {
                     ConsoleBaseType *cbt = ConsoleBaseType::getType((*fieldList)[j].type);

                     printClassMember(
                        false,
                        cbt ? cbt->getTypeClassName() : "<unknown>",
                        (*fieldList)[j].pFieldname,
                        (*fieldList)[j].pFieldDocs
                        );
                  }
               }
            }
         }
      }

      // Close the class/namespace.
      printClassFooter();
   }
}

void Namespace::dumpFunctions()
{
   // Get the global namespace.
   Namespace* g = find(NULL); //->mParent;

   printClassHeader(NULL,NULL, false);

   while(g) 
   {
      printNamespaceEntries(g);
      g = g->mParent;
   }

   printClassFooter();
}
