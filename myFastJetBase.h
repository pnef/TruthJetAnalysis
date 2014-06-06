#ifndef MYFASTJETBASE_H
#define MYFASTJETBASE_H

#include "fastjet/PseudoJet.hh"
#include "fastjet/Selector.hh"



class GhostUserInfo : public fastjet::PseudoJet::UserInfoBase{
 public:
    GhostUserInfo(const bool & is_ghost, const bool &is_pileup, const bool &is_track): _is_ghost(is_ghost), _is_pileup(is_pileup), _is_track(is_track){}
    bool is_ghost()   const { return _is_ghost;}
    bool is_pileup()  const { return _is_pileup;}
    bool is_track()   const { return _is_track;}
 protected:
    bool _is_ghost;  // true is ghost particle
    bool _is_pileup; // true is ghost particle
    bool _is_track;  // true is track 
};


#endif
