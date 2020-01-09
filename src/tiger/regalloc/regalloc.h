#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include "tiger/liveness/flowgraph.h"

#include <iostream>
#include <sstream>
#include <set>

namespace RA
{

class Result
{
public:
  TEMP::Map *coloring;
  AS::InstrList *il;
  Result(TEMP::Map *coloring, AS::InstrList *il) : coloring(coloring), il(il) {}
};

Result RegAlloc(F::Frame *f, AS::InstrList *il);

// data structures
// worklist
static G::NodeList<TEMP::Temp> *simplifyWorklist;
static G::NodeList<TEMP::Temp> *spillWorklist;
static G::NodeList<TEMP::Temp> *freezeWorklist;
// clustered nodes set
static G::NodeList<TEMP::Temp> *coloredNodes;
static G::NodeList<TEMP::Temp> *spilledNodes;
static G::NodeList<TEMP::Temp> *coalescedNodes;
static TEMP::TempList *notSpillTemps;
// movelist
static LIVE::MoveList *coalescedMoves;
static LIVE::MoveList *constrainedMoves;
static LIVE::MoveList *frozenMoves;
static LIVE::MoveList *worklistMoves;
static LIVE::MoveList *activeMoves;
// choosable stack
static G::NodeList<TEMP::Temp> *selectStack;
// information binding
static G::Table<TEMP::Temp, int> *colortable;
static G::Table<TEMP::Temp, LIVE::MoveList> *movelisttable;
static G::Table<TEMP::Temp, G::Node<TEMP::Temp>> *aliastable;
static G::Table<TEMP::Temp, int> *degreetable;

static LIVE::LiveGraph *live = new LIVE::LiveGraph();
static std::string *real_registers[16] = {new std::string("none"), new std::string("%rax"), new std::string("%rbx"),
     new std::string("%rcx"), new std::string("%rdx"), new std::string("%rsi"),
     new std::string("%rdi"), new std::string("%rbp"), new std::string("%r8"),
     new std::string("%r9"), new std::string("%r10"), new std::string("%r11"),
     new std::string("%r12"), new std::string("%r13"), new std::string("%r14"),
     new std::string("%r15")};

} // namespace RA

#endif