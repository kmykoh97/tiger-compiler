#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"

namespace LIVE {

class MoveList {
 public:
  G::Node<TEMP::Temp>*src, *dst;
  MoveList* tail;

  MoveList(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst, MoveList* tail)
      : src(src), dst(dst), tail(tail) {}
};

class LiveGraph {
 public:
  G::Graph<TEMP::Temp>* graph;
  MoveList* moves;
};

LiveGraph *Liveness(G::Graph<AS::Instr>* flowgraph);

// self-defined helper functions
bool isInTempList(TEMP::Temp* temp, TEMP::TempList* templist);
LIVE::MoveList* subMoveList(LIVE::MoveList* left, LIVE::MoveList* right);
LIVE::MoveList* intersectMoveList(LIVE::MoveList* left, LIVE::MoveList* right);
LIVE::MoveList* unionMoveList(LIVE::MoveList* left, LIVE::MoveList* right);
bool isInMoveList(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst, LIVE::MoveList* templist);
TEMP::TempList* unionTempList(TEMP::TempList* left, TEMP::TempList* right);
TEMP::TempList* subTempList(TEMP::TempList* left, TEMP::TempList* right);
bool isTempEqual(TEMP::Temp* t1, TEMP::Temp* t2);
bool isInMoveList(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst, LIVE::MoveList* movelist);

}  // namespace LIVE

#endif