#include "tiger/liveness/liveness.h"

namespace LIVE {

bool isInTempList(TEMP::Temp* temp, TEMP::TempList* templist) {
  TEMP::TempList* test = templist;
  for (; templist; templist = templist->tail) {
    if (templist->head == temp) {
      return true;
    }
  }

  return false;
}

static TEMP::Temp* findintemplist(TEMP::TempList* l, TEMP::Temp* e) {
  for (; l; l = l->tail) {
    if (l->head == e) {
      return e;
    }
  }

  return nullptr;
}

LIVE::MoveList* subMoveList(LIVE::MoveList* left, LIVE::MoveList* right) {
  LIVE::MoveList* current = nullptr;

  for (; left; left = left->tail) {
    if (!isInMoveList(left->src, left->dst, right)) {
      current = new LIVE::MoveList(left->src, left->dst, current);
    }
  }

  return current;
} 

LIVE::MoveList* intersectMoveList(LIVE::MoveList* left, LIVE::MoveList* right) {
  LIVE::MoveList* current = nullptr;

  for (; left; left = left->tail) {
    if (isInMoveList(left->src, left->dst, right)) {
      current = new LIVE::MoveList(left->src, left->dst, current);
    }
  }

  return current;
}

LIVE::MoveList* unionMoveList(LIVE::MoveList* left, LIVE::MoveList* right) {
  LIVE::MoveList* current = left;

  for (; right; right = right->tail) {
    if (!isInMoveList(right->src, right->dst, left)) {
      current = new LIVE::MoveList(right->src, right->dst, current);
    }
  }

  return current;
}

bool isInMoveList(G::Node<TEMP::Temp>* src, G::Node<TEMP::Temp>* dst, LIVE::MoveList* templist) {
  for (; templist; templist = templist->tail) {
    if (templist->src == src && templist->dst == dst) {
      return true;
    }
  }

  return false;
}

TEMP::TempList* unionTempList(TEMP::TempList* left, TEMP::TempList* right) {
  TEMP::TempList* templist = left;

  // if (left) {
  //   templist = new TEMP::TempList(*left);
  // } else {
  //   templist = left;
  // }

  for (; right; right = right->tail) {
    if (!isInTempList(right->head, templist)) {
      if (right->head == nullptr) continue;
      templist = new TEMP::TempList(right->head, templist);
    }
  }

  return templist;
}

TEMP::TempList* subTempList(TEMP::TempList* left, TEMP::TempList* right) {
  TEMP::TempList* templist = nullptr;

  for (; left; left = left->tail) {
    if (!isInTempList(left->head, right)) {
      if (left->head == nullptr) continue;
      templist = new TEMP::TempList(left->head, templist);
    }
  }

  return templist;
}

bool isTempEqual(TEMP::TempList* t1, TEMP::TempList* t2) {
  TEMP::TempList* temp = t1;

  for (; temp; temp = temp->tail) {
    if (temp->head == nullptr) continue;
    if (!isInTempList(temp->head, t2)) {
      return false;
    }
  }

  temp = t2;

  for (; temp; temp = temp->tail) {
    if (temp->head == nullptr) continue;
    if (!isInTempList(temp->head, t1)) {
      return false;
    }
  }

  return true;
}

G::NodeList<AS::Instr>* rnodes(G::Graph<AS::Instr>* g) {
  if (!g->mynodes) return nullptr;
  G::NodeList<AS::Instr>* nodes = g->Nodes();
  G::NodeList<AS::Instr>* rnodes = new G::NodeList<AS::Instr>(nodes->head, nullptr);

  for (nodes = nodes->tail; nodes; nodes = nodes->tail) {
    rnodes = new G::NodeList<AS::Instr>(nodes->head, rnodes);
  }

  return rnodes;
}

G::Table<AS::Instr, TEMP::TempList>* buildlivemap(G::Graph<AS::Instr>* flow) {
  bool dirty;
  G::Table<AS::Instr, TEMP::TempList>* livein = new G::Table<AS::Instr, TEMP::TempList>();
  G::Table<AS::Instr, TEMP::TempList>* liveout = new G::Table<AS::Instr, TEMP::TempList>();
  G::NodeList<AS::Instr>* rnodeshead = rnodes(flow);
  G::NodeList<AS::Instr>* flownodes = nullptr;

  do {
    dirty = false;

    for (flownodes = rnodeshead; flownodes; flownodes = flownodes->tail) {
      G::Node<AS::Instr>* flownode = flownodes->head;
      TEMP::TempList* ins = livein->Look(flownode);
      TEMP::TempList* outs = liveout->Look(flownode);
      TEMP::TempList* uses = FG::Use(flownode);
      TEMP::TempList* defs = FG::Def(flownode);

      for (; uses; uses = uses->tail) {
        if (!findintemplist(ins, uses->head)) {
          ins = new TEMP::TempList(uses->head, ins);
          dirty = true;
        }
      }

      for (; outs; outs = outs->tail) {
        TEMP::Temp* out = outs->head;

        if (!findintemplist(defs, out) && !findintemplist(ins, out)) {
          ins = new TEMP::TempList(out, ins);
          dirty = true;
        }
      }

      outs = liveout->Look(flownode);
      G::NodeList<AS::Instr>* succs = flownode->Succ();

      for (; succs; succs = succs->tail) {
        G::Node<AS::Instr>* succ = succs->head;
        TEMP::TempList* succins = livein->Look(succ);

        for (; succins; succins = succins->tail) {
          TEMP::Temp* succin = succins->head;

          if (!findintemplist(outs, succin)) {
            outs = new TEMP::TempList(succin, outs);
            dirty = true;
          }
        }
      }

      livein->Enter(flownode, ins);
      liveout->Enter(flownode, outs);
    }
  } while (dirty);

  return liveout;
}

LiveGraph* Liveness(G::Graph<AS::Instr>* flowgraph) {
  // TODO: Put your codes here (lab6).
  LiveGraph* livegraph = new LiveGraph();
  G::Table<AS::Instr, TEMP::TempList>* in = new G::Table<AS::Instr, TEMP::TempList>();
  G::Table<AS::Instr, TEMP::TempList>* out = new G::Table<AS::Instr, TEMP::TempList>();

  // init in, out sets
  G::NodeList<AS::Instr>* flownodes = flowgraph->Nodes();
  G::Node<AS::Instr>* flownode = nullptr;
  bool fixed_point = false;

  for (; flownodes; flownodes = flownodes->tail) {
    flownode = flownodes->head;
    // in->Enter(flownode, new TEMP::TempList(TEMP::Temp::NewTemp(), nullptr));
    // out->Enter(flownode, new TEMP::TempList(TEMP::Temp::NewTemp(), nullptr));
    in->Enter(flownode, (TEMP::TempList*)malloc(sizeof(TEMP::TempList)));
    out->Enter(flownode, (TEMP::TempList*)malloc(sizeof(TEMP::TempList)));
  }

  // start to fill in, out sets
  while (!fixed_point) {
    fixed_point = true;
    flownodes = flowgraph->Nodes();

    for (; flownodes; flownodes = flownodes->tail) {
      flownode = flownodes->head;
      TEMP::TempList* temp1 = in->Look(flownode);
      TEMP::TempList* temp2 = out->Look(flownode);
      TEMP::TempList* intemp = nullptr;
      TEMP::TempList* outtemp = nullptr;
      /* pseudocode:
			in[n] = use[n] and (out[n] – def[n])
 			out[n] = (for s in succ[n])
			            Union(in[s])
			*/
      G::NodeList<AS::Instr>* successor = flownode->Succ();

      // if (!successor) {
      //   goto jump;
      // }

      for (; successor; successor = successor->tail) {
        TEMP::TempList* tempinsuccessor = in->Look(successor->head);
        outtemp = unionTempList(outtemp, tempinsuccessor);
      }

      intemp = unionTempList(FG::Use(flownode), subTempList(outtemp, FG::Def(flownode)));

      if (!isTempEqual(temp1, intemp) || !isTempEqual(temp2, outtemp)) {
        fixed_point = false;
      }
      // jump: ;
      // update intemp, outtemp
      in->Enter(flownode, intemp);
      out->Enter(flownode, outtemp);
      // jump: ;
    }
  }

  // construct interference graph
  livegraph->graph = new G::Graph<TEMP::Temp>();
  livegraph->moves = nullptr;

  // add registers nodes to the graph
  TAB::Table<TEMP::Temp, G::Node<TEMP::Temp>>* tempTable = new TAB::Table<TEMP::Temp, G::Node<TEMP::Temp>>();
  TEMP::TempList* hardregister = new TEMP::TempList(F::RAX(), new TEMP::TempList(F::RBX(), new TEMP::TempList(F::RCX(), new TEMP::TempList(F::RDX(), new TEMP::TempList(F::RSI(), new TEMP::TempList(F::RDI(), new TEMP::TempList(F::R8(), new TEMP::TempList(F::R9(), new TEMP::TempList(F::R10(), new TEMP::TempList(F::R11(), new TEMP::TempList(F::R12(), new TEMP::TempList(F::R13(), new TEMP::TempList(F::R14(), new TEMP::TempList(F::R15(), new TEMP::TempList(F::RBP(), nullptr)))))))))))))));

  // add nodes of registers into the table
  for (TEMP::TempList* list = hardregister; list; list = list->tail) {
    G::Node<TEMP::Temp>* tnode = livegraph->graph->NewNode(list->head);
    tempTable->Enter(list->head, tnode);
  }

  // first, add edges between hard registers
  for (TEMP::TempList* i = hardregister; i; i = i->tail) {
    for (TEMP::TempList* j = i->tail; j; j = j->tail) {
      G::Node<TEMP::Temp>* nodea = tempTable->Look(i->head);
      G::Node<TEMP::Temp>* nodeb = tempTable->Look(j->head);
      G::Graph<TEMP::Temp>::AddEdge(nodea, nodeb);
      G::Graph<TEMP::Temp>::AddEdge(nodeb, nodea);
    }
  }

  // add nodes of temp registers into the table
  flownodes = flowgraph->Nodes();

  for (; flownodes; flownodes = flownodes->tail) {
    flownode = flownodes->head;
    TEMP::TempList* outn = out->Look(flownode);
    TEMP::TempList* def = FG::Def(flownode);

    if (!(def && def->head)) {
      continue; // skip the rest of for loop
    }

    TEMP::TempList* temps = (TEMP::TempList*)malloc(sizeof(TEMP::TempList));
    temps = unionTempList(outn, def);

    for (; temps; temps = temps->tail) {
      if (temps->head == F::RSP()) {
        continue;
      }

      // if temps->head not in table, add it
      if (!tempTable->Look(temps->head)) {
        G::Node<TEMP::Temp>* tenode = livegraph->graph->NewNode(temps->head);
        tempTable->Enter(temps->head, tenode);
      }
    }
  }

  // add edges between temp registers node
  for (flownodes = flowgraph->Nodes(); flownodes; flownodes = flownodes->tail) {
    flownode = flownodes->head;
    TEMP::TempList* outn;
    TEMP::TempList* def = FG::Def(flownode);

    if (!(def && def->head)) {
      continue; // skip the rest of for loop
    }

    // check: if move, suspend, else add edge
    if (!FG::IsMove(flownode)) {
      for (; def; def = def->tail) {
        if (def->head == F::RSP()) {
          continue; // not counted!
        }

        G::Node<TEMP::Temp>* defnode = tempTable->Look(def->head);
        outn = out->Look(flownode);

        for (; outn; outn = outn->tail) {
          if (outn->head == F::RSP()) {
            continue;
          }

          G::Node<TEMP::Temp>* outnode = tempTable->Look(outn->head);

          // If outnode and defnode not linked yet
          if (!outnode->Adj()->InNodeList(defnode)) {
            G::Graph<TEMP::Temp>::AddEdge(defnode, outnode);
            G::Graph<TEMP::Temp>::AddEdge(outnode, defnode);
          }
        }
      }
    } else { // if move
      for(; def; def = def->tail) {
        if (def->head == F::RSP()) {
          continue;
        }

        G::Node<TEMP::Temp>* defnode = tempTable->Look(def->head);

        for (outn = out->Look(flownode); outn; outn = outn->tail) {
          if (outn->head == F::RSP()) {
            continue;
          }

          G::Node<TEMP::Temp>* outnode = tempTable->Look(outn->head);

          if (!(outnode->Adj()->InNodeList(defnode)) && !(isInTempList(outn->head, FG::Use(flownode)))) {
            G::Graph<TEMP::Temp>::AddEdge(defnode, outnode);
            G::Graph<TEMP::Temp>::AddEdge(outnode, defnode);
          }
        }

        // add to movelist for coalesce
        for (TEMP::TempList* addmove = FG::Use(flownode); addmove; addmove = addmove->tail) {
          if (addmove->head == F::RSP()) {
            continue;
          }

          G::Node<TEMP::Temp>* unode = tempTable->Look(addmove->head);

          if (!isInMoveList(unode, defnode, livegraph->moves)) {
            livegraph->moves = new LIVE::MoveList(unode, defnode, livegraph->moves);
          }
        }
      }
    }
  }

  return livegraph;

  // return LiveGraph();
  // G::Table<AS::Instr, TEMP::TempList>* livemap = buildlivemap(flowgraph);

  // return livemap;

}

}  // namespace LIVE