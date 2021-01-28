//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CREATOR_H_
#define _CREATOR_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _GUIARRAYCTRL_H_
#include "gui/core/guiArrayCtrl.h"
#endif

class CreatorTree : public GuiArrayCtrl
{
   typedef GuiArrayCtrl Parent;
   public:

      class Node
      {
         public:
            Node();
            ~Node();

            enum {
               Group       = BIT(0),
               Expanded    = BIT(1),
               Selected    = BIT(2),
               Root        = BIT(3)
            };

            BitSet32             mFlags;
            S32                  mId;
            U32                  mTab;
            Node *               mParent;
            Vector<Node*>        mChildren;
            StringTableEntry     mName;
            StringTableEntry     mValue;

            void expand(bool exp);
            void select(bool sel){mFlags.set(Selected, sel);}

            Node * find(S32 id);

            //
            bool isGroup(){return(mFlags.test(Group));}
            bool isExpanded(){return(mFlags.test(Expanded));}
            bool isSelected(){return(mFlags.test(Selected));}
            bool isRoot(){return(mFlags.test(Root));}
            S32 getId(){return(mId);}
            bool hasChildItem();
            S32 getSelected();

            //
            bool isFirst();
            bool isLast();
      };

      CreatorTree();
      ~CreatorTree();

      //
      S32                        mCurId;
      Node *                     mRoot;
      Vector<Node*>              mNodeList;

      //
      void buildNode(Node * node, U32 tab);
      void build();

      //
      bool addNode(Node * parent, Node * node);
      Node * createNode(const char * name, const char * value, bool group = false, Node * parent = 0);
      Node * findNode(S32 id);
      S32 getSelected(){return(mRoot->getSelected());}

      //
      void expandNode(Node * node, bool expand);
      void selectNode(Node * node, bool select);

      //
      void sort();
      void clear();

      S32                           mTabSize;
      S32                           mMaxWidth;
      S32                           mTxtOffset;

      // GuiControl
      void onMouseDown(const GuiEvent & event);
      void onMouseDragged(const GuiEvent & event);
      void onMouseUp(const GuiEvent & event);
      bool onWake();

      // GuiArrayCtrl
      void onRenderCell(Point2I offset, Point2I cell, bool, bool);

      DECLARE_CONOBJECT(CreatorTree);
};

#endif
