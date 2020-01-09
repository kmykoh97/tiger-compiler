#include "tiger/liveness/flowgraph.h"

namespace FG {

TEMP::TempList* Def(G::Node<AS::Instr>* n) {
  // TODO: Put your codes here (lab6).
  AS::Instr* instr = n->NodeInfo();

  switch (instr->kind) {
    case AS::Instr::LABEL:
      return nullptr;
    case AS::Instr::MOVE:
      return static_cast<AS::MoveInstr*>(instr)->dst;
    case AS::Instr::OPER:
      return static_cast<AS::OperInstr*>(instr)->dst;
  }

  return nullptr;
}

TEMP::TempList* Use(G::Node<AS::Instr>* n) {
  // TODO: Put your codes here (lab6).
  AS::Instr* instr = n->NodeInfo();

  switch (instr->kind) {
    case AS::Instr::LABEL:
      return nullptr;
    case AS::Instr::MOVE:
      return static_cast<AS::MoveInstr*>(instr)->src;
    case AS::Instr::OPER:
      return static_cast<AS::OperInstr*>(instr)->src;
  }

  return nullptr;
}

bool IsMove(G::Node<AS::Instr>* n) {
  // TODO: Put your codes here (lab6).
  AS::Instr* instr = n->NodeInfo();

  return (instr->kind == AS::Instr::MOVE);
}

// build control-flow graph
G::Graph<AS::Instr>* AssemFlowGraph(AS::InstrList* il, F::Frame* f) {
  // TODO: Put your codes here (lab6).
  G::Graph<AS::Instr>* flowgraph = new G::Graph<AS::Instr>();
  TAB::Table<TEMP::Label, G::Node<AS::Instr>>* labelmap = new TAB::Table<TEMP::Label, G::Node<AS::Instr>>();
  AS::InstrList* instrlist;
  AS::Instr* instr;
  G::Node<AS::Instr>* previous = nullptr;
  G::Node<AS::Instr>* current = nullptr;

  for (instrlist = il; instrlist; instrlist = instrlist->tail) {
    instr = instrlist->head;
    current = flowgraph->NewNode(instr);

    if (previous) {
      flowgraph->AddEdge(previous, current);
    }

    // add labels to map some control-flow column
    if (instr->kind == AS::Instr::LABEL) {
      labelmap->Enter(static_cast<AS::LabelInstr*>(instr)->label, current);
    }

    // if jmp, do not add edges between nodes
    if (instr->kind == AS::Instr::OPER) {
      std::string i = (static_cast<AS::OperInstr*>(instr)->assem).substr(0, 3);

      if (i == "jmp") {
        previous = nullptr;
        continue;
      }
    }

    previous = current;
  }

  G::NodeList<AS::Instr>* currentFlowGraph = flowgraph->Nodes();
  G::Node<AS::Instr>* target = nullptr;

  for (; currentFlowGraph; currentFlowGraph = currentFlowGraph->tail) {
    current = currentFlowGraph->head;
    instr = current->NodeInfo();

    if (instr->kind == AS::Instr::OPER && (static_cast<AS::OperInstr*>(instr)->jumps && static_cast<AS::OperInstr*>(instr)->jumps->labels)) {
      TEMP::LabelList* labels = static_cast<AS::OperInstr*>(instr)->jumps->labels;

      for (; labels; labels = labels->tail) {
        target = labelmap->Look(labels->head);

        if (target) {
          flowgraph->AddEdge(current, target);
        } else {
          assert(0); // for testing only
        }
      }
    }
  }

  return flowgraph;
}

}  // namespace FG
