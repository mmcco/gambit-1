//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Interface to objects representing information sets
//
// This file is part of Gambit
// Copyright (c) 2002, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifndef INFOSET_H
#define INFOSET_H

#ifdef __GNUG__
#pragma interface
#endif   // __GNUG__

#include "math/gvector.h"

#include "efplayer.h"

class Node;

class Lexicon;

class Action   {
  friend class efgGame;
  friend class BehavProfile<double>;
  friend class BehavProfile<gRational>;
  friend class BehavProfile<gNumber>;
  friend class Infoset;
  private:
    int number;
    gText name;
    Infoset *owner;

    Action(int br, const gText &n, Infoset *s)
      : number(br), name(n), owner(s)  { }
    ~Action()   { }

  public:
    const gText &GetName(void) const   { return name; }
    void SetName(const gText &s)       { name = s; }

    int GetNumber(void) const        { return number; }
    Infoset *BelongsTo(void) const   { return owner; }
    bool Precedes(const Node *) const;
};

class Infoset   {
  friend class efgGame;
  friend class EFPlayer;
  friend class BehavProfile<double>;
  friend class BehavProfile<gRational>;
  friend class BehavProfile<gNumber>;
  friend class Lexicon;

  protected:
    efgGame *E;
    int number;
    gText name;
    EFPlayer *player;
    gBlock<Action *> actions;
    gBlock<Node *> members;
    int flag, whichbranch;
    
    Infoset(efgGame *e, int n, EFPlayer *p, int br);
    virtual ~Infoset();  

    virtual void PrintActions(gOutput &f) const;

  public:
    efgGame *Game(void) const   { return E; }

    bool IsChanceInfoset(void) const   { return (player->IsChance()); }

    EFPlayer *GetPlayer(void) const    { return player; }

    void SetName(const gText &s)    { name = s; }
    const gText &GetName(void) const   { return name; }

    virtual Action *InsertAction(int where);
    virtual void RemoveAction(int which);

    void SetActionName(int i, const gText &s)
      { actions[i]->name = s; }
    const gText &GetActionName(int i) const  { return actions[i]->name; }

    const gArray<Action *> &Actions(void) const { return actions; }
    Action *GetAction(const int act) const { return actions[act]; }
    const gList<Action *> ListOfActions(void) const;
    int NumActions(void) const   { return actions.Length(); }

    const gArray<Node *> &Members(void) const   { return members; }
    Node *GetMember(int m) const { return members[m]; }
    const gList<const Node *> ListOfMembers(void) const;
    int NumMembers(void) const   { return members.Length(); }

    int GetNumber(void) const    { return number; }

    bool Precedes(const Node *) const;
};

class ChanceInfoset : public Infoset  {
  friend class efgGame;
  friend class BehavProfile<double>;
  friend class BehavProfile<gRational>;
  friend class BehavProfile<gNumber>;
  friend class PureBehavProfile<double>;
  friend class PureBehavProfile<gRational>;
  friend class PureBehavProfile<gNumber>;

  private:
    gBlock<gNumber> probs;

    ChanceInfoset(efgGame *E, int n, EFPlayer *p, int br);
    virtual ~ChanceInfoset()    { }

    void PrintActions(gOutput &f) const;

  public:
    Action *InsertAction(int where);
    void RemoveAction(int which);

    void SetActionProb(int i, const gNumber &value)  { probs[i] = value; }
    const gNumber &GetActionProb(int i) const   { return probs[i]; }
    const gArray<gNumber> &GetActionProbs(void) const  { return probs; }
};

#endif   //# INFOSET_H
