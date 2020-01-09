#include "tiger/regalloc/regalloc.h"

namespace RA {

  static LIVE::MoveList * movelist(G::Node<TEMP::Temp>* n) {
    return movelisttable -> Look(n);
  }
  
  static bool precolor(G::Node<TEMP::Temp>* n) {
    return *(colortable -> Look(n));
  }

  static int degree(G::Node<TEMP::Temp>* n) {
    return *(degreetable -> Look(n));
  }

  static LIVE::MoveList * nodemoves(G::Node<TEMP::Temp>* n) {
    LIVE::MoveList * movelist = movelisttable -> Look(n);
    return LIVE::intersectMoveList(movelist, LIVE::unionMoveList(activeMoves, worklistMoves));
  }

  static bool MoveRelated(G::Node<TEMP::Temp>* n) {
    return nodemoves(n) != nullptr;
  }

  static TEMP::Map * assignregisters(LIVE::LiveGraph * g) {
    TEMP::Map * res = TEMP::Map::Empty();
    G::NodeList<TEMP::Temp>* nodes = (g -> graph) -> Nodes();

    res -> Enter(F::RSP(), new std::string("%rsp"));

    for (; nodes; nodes = nodes -> tail) {
      int * color = colortable -> Look(nodes -> head);
      res -> Enter(nodes -> head -> NodeInfo(), real_registers[*color]);
    }

    return res;
  }

  static TEMP::TempList * replaceTempList(TEMP::TempList * instr, TEMP::Temp * old, TEMP::Temp * new_t) {
    if (instr) {
      if (instr -> head == old) {
        return new TEMP::TempList(new_t, replaceTempList(instr -> tail, old, new_t));
      } else {
        return new TEMP::TempList(instr -> head, replaceTempList(instr -> tail, old, new_t));
      }
    } else {
      return nullptr;
    }
  }

  static G::Node<TEMP::Temp>* alias(G::Node<TEMP::Temp>* n) {
    return aliastable -> Look(n);
  }

  static G::Node<TEMP::Temp>* getalias(G::Node<TEMP::Temp>* t) {
    if (coalescedNodes -> InNodeList(t)) {
      return getalias(alias(t));
    } else {
      return t;
    }
  }

  bool isInTemp(TEMP::TempList * list, TEMP::Temp * temp) {
    for (; list; list = list -> tail) {
      if (list -> head == temp)
        return true;
    }

    return false;
  }

  void RewriteProgram(F::Frame * f, AS::InstrList * pil) {
    std::string fs = TEMP::LabelString(f -> name) + "_framesize";
    notSpillTemps = nullptr;
    AS::InstrList * il = new AS::InstrList(nullptr, nullptr);
    * il = * pil;
    AS::InstrList * instr;
    AS::InstrList * last;
    AS::InstrList * next;
    AS::InstrList * new_instr;
    int off;
    int count = 0;

    while (spilledNodes) {
      G::Node<TEMP::Temp>* cur = spilledNodes -> head;
      spilledNodes = spilledNodes -> tail;
      TEMP::Temp * spilledtemp = cur -> NodeInfo();
      off = f -> offset;
      f -> offset -= 8;
      instr = il;
      last = nullptr;
      count++;

      while (instr) {
        TEMP::Temp * t = nullptr;
        next = instr -> tail;
        TEMP::TempList * def = nullptr, * use = nullptr;

        switch (instr -> head -> kind) {
        case AS::Instr::MOVE:
          def = static_cast<AS::MoveInstr*>(instr -> head) -> dst;
          use = static_cast<AS::MoveInstr*>(instr -> head) -> src;
          break;
        case AS::Instr::OPER:
          def = static_cast<AS::MoveInstr*>(instr -> head) -> dst;
          use = static_cast<AS::MoveInstr*>(instr -> head) -> src;
          break;
        default:
          def = nullptr;
          use = nullptr;
        }

        if (use && isInTemp(use, spilledtemp)) {
          t = TEMP::Temp::NewTemp();
          notSpillTemps = new TEMP::TempList(t, notSpillTemps);
          *use = *replaceTempList(use, spilledtemp, t);
          std::string assem;
          std::stringstream ioss;
          ioss << "# Spillload\nmovq (" + fs + "-0x" << std::hex << -off << ")(%rsp),`d0\n";
          assem = ioss.str();
          AS::OperInstr * os_instr = new AS::OperInstr(assem, new TEMP::TempList(t, nullptr), nullptr, new AS::Targets(nullptr));
          new_instr = new AS::InstrList(os_instr, instr);

          if (last) {
            last -> tail = new_instr;
          } else {
            il = new_instr;
          }
        }

        last = instr;

        if (def && isInTemp(def, spilledtemp)) {
          if (!t) {
            t = TEMP::Temp::NewTemp();

            notSpillTemps = new TEMP::TempList(t, notSpillTemps);
          }

          * def = * replaceTempList(def, spilledtemp, t);

          assert(!isInTemp(def, spilledtemp));

          std::string assem;
          std::stringstream ioss;
          ioss << "# Spillstore\nmovq `s0, (" + fs + "-0x" << std::hex << -off << ")(%rsp) \n";
          assem = ioss.str();
          AS::OperInstr * os_instr = new AS::OperInstr(assem, nullptr, new TEMP::TempList(t, nullptr), new AS::Targets(nullptr));
          instr -> tail = new AS::InstrList(os_instr, next);

          last = instr -> tail;
        }
        instr = next;
      }
    }

    f -> offset += 8;
    * pil = * il;
  }

  static void MakeWorklist() {
    G::NodeList<TEMP::Temp>* nodes = (live -> graph) -> Nodes();

    for (; nodes; nodes = nodes -> tail) {
      G::Node<TEMP::Temp>* node = nodes -> head;
      int degree = node -> Degree();
      int * color = colortable -> Look(nodes -> head);
     
      if ( * color != 0) {
        continue;
      }

      if (degree >= 15) {
        spillWorklist = new G::NodeList<TEMP::Temp>(node, spillWorklist);
      } else if (MoveRelated(node)) {
        freezeWorklist = new G::NodeList<TEMP::Temp>(node, freezeWorklist);
      } else {
        simplifyWorklist = new G::NodeList<TEMP::Temp>(node, simplifyWorklist);
      }
    }
  }

  static void Build() {
    simplifyWorklist = nullptr;
    freezeWorklist = nullptr;
    spillWorklist = nullptr;
    spilledNodes = nullptr;
    coalescedNodes = nullptr;
    coloredNodes = nullptr;
    selectStack = nullptr;
    coalescedMoves = nullptr;
    constrainedMoves = nullptr;
    frozenMoves = nullptr;
    worklistMoves = live -> moves;
    activeMoves = nullptr;

    degreetable = new G::Table < TEMP::Temp, int > ();
    colortable = new G::Table < TEMP::Temp, int > ();
    movelisttable = new G::Table < TEMP::Temp, LIVE::MoveList > ();
    aliastable = new G::Table < TEMP::Temp, G::Node<TEMP::Temp>> ();

    G::NodeList<TEMP::Temp>* nodes = (live -> graph) -> Nodes();

    for (; nodes; nodes = nodes -> tail) {
      TEMP::Temp * temp = nodes -> head -> NodeInfo();
      int * color = new int;
      if (temp == F::RAX())
        *color = 1;
      else if (temp == F::RBX())
        *color = 2;
      else if (temp == F::RCX())
        *color = 3;
      else if (temp == F::RDX())
        *color = 4;
      else if (temp == F::RSI())
        *color = 5;
      else if (temp == F::RDI())
        *color = 6;
      else if (temp == F::RBP())
        *color = 7;
      else if (temp == F::R8())
        *color = 8;
      else if (temp == F::R9())
        *color = 9;
      else if (temp == F::R10())
        *color = 10;
      else if (temp == F::R11())
        *color = 11;
      else if (temp == F::R12())
        *color = 12;
      else if (temp == F::R13())
        *
        color = 13;
      else if (temp == F::R14())
        *color = 14;
      else if (temp == F::R15())
        *color = 15;
      else
        *color = 0;

      colortable -> Enter(nodes -> head, color);

      int * degree = new int;
      * degree = nodes -> head -> Degree();
      degreetable -> Enter(nodes -> head, degree);
      G::Node<TEMP::Temp>* alias = nodes -> head;
      aliastable -> Enter(nodes -> head, alias);
      LIVE::MoveList * list = worklistMoves;
      LIVE::MoveList * movelist = nullptr;

      for (; list; list = list -> tail) {
        if (list -> src == nodes -> head || list -> dst == nodes -> head) {
          movelist = new LIVE::MoveList(list -> src, list -> dst, movelist);
        }
      }

      movelisttable -> Enter(nodes -> head, movelist);
    }
  }

  static void enablemoves(G::NodeList<TEMP::Temp>* nodes) {
    for (; nodes; nodes = nodes -> tail) {
      for (LIVE::MoveList * m = nodemoves(nodes -> head); m; m = m -> tail) {
        if (LIVE::isInMoveList(m -> src, m -> dst, activeMoves)) {
          activeMoves = LIVE::subMoveList(activeMoves, new LIVE::MoveList(m -> src, m -> dst, nullptr));
          worklistMoves = new LIVE::MoveList(m -> src, m -> dst, worklistMoves);
        }
      }
    }
  }

  static void AssignColors() {
    bool okColors[15 + 1];
    spilledNodes = nullptr;

    while (selectStack) {
      okColors[0] = false;
      for (int i = 1; i < 15 + 1; i++) {
        okColors[i] = true;
      }

      G::Node<TEMP::Temp>* n = selectStack -> head;
      selectStack = selectStack -> tail;

      for (G::NodeList<TEMP::Temp>* succs = n -> Succ(); succs; succs = succs -> tail) {
        int * color = colortable -> Look(succs -> head);
        okColors[ * color] = false;
      }

      int i;
      bool realSpill = true;
      for (i = 1; i < 15 + 1; i++) {
        if (okColors[i]) {
          realSpill = false;
          break;
        }
      }

      if (realSpill) {
        spilledNodes = new G::NodeList<TEMP::Temp>(n, spilledNodes);
      } else {
        int * color = colortable -> Look(n);
        * color = i;
      }
    }

    for (G::NodeList<TEMP::Temp>* p = live -> graph -> mynodes; p; p = p -> tail) {
      colortable -> Enter(p -> head, colortable -> Look(getalias(p -> head)));
    }
  }

  static void freezemoves(G::Node<TEMP::Temp>* v) {
    for (LIVE::MoveList * m = nodemoves(v); m; m = m -> tail) {
      G::Node<TEMP::Temp>* x = m -> src;
      G::Node<TEMP::Temp>* y = m -> dst;
      G::Node<TEMP::Temp>* u;

      if (getalias(y) == getalias(v)) {
        u = getalias(x);
      } else {
        u = getalias(y);
      }

      activeMoves = LIVE::subMoveList(activeMoves, new LIVE::MoveList(x, y, nullptr));
      frozenMoves = new LIVE::MoveList(x, y, frozenMoves);

      if (nodemoves(u) == nullptr && degree(u) < 15) {
        freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(u, nullptr));
        simplifyWorklist = new G::NodeList<TEMP::Temp>(u, simplifyWorklist);
      }
    }
  }

  static void SelectSpill() {
    G::Node<TEMP::Temp>* m = nullptr;
    int max = 0;

    for (G::NodeList<TEMP::Temp>* spill = spillWorklist; spill; spill = spill -> tail) {
      TEMP::Temp * t = spill -> head -> NodeInfo();
      if (isInTemp(notSpillTemps, t)) {
        continue;
      }
      if (spill -> head -> Degree() > max) {
        m = spill -> head;
        max = m -> Degree();
      }
    }

    if (m) {
      spillWorklist = G::NodeList<TEMP::Temp>::SubList(spillWorklist, new G::NodeList<TEMP::Temp>(m, nullptr));
      simplifyWorklist = new G::NodeList<TEMP::Temp>(m, simplifyWorklist);
      freezemoves(m);
    } else {
      m = spillWorklist -> head;
      spillWorklist = spillWorklist -> tail;
      simplifyWorklist = new G::NodeList<TEMP::Temp>(m, simplifyWorklist);
      freezemoves(m);
    }
  }

  static void DecrementDegree(G::Node<TEMP::Temp>* n) {
    int * degree = degreetable -> Look(n);
    int d = * degree;
    * degree = * degree - 1;
    int * color = colortable -> Look(n);

    if (d == 15 && * color == 0) {
      enablemoves(new G::NodeList<TEMP::Temp>(n, n -> Adj()));
      spillWorklist = G::NodeList<TEMP::Temp>::SubList(spillWorklist, new G::NodeList<TEMP::Temp>(n, nullptr));
      if (MoveRelated(n)) {
        freezeWorklist = new G::NodeList<TEMP::Temp>(n, freezeWorklist);
      } else {
        simplifyWorklist = new G::NodeList<TEMP::Temp>(n, simplifyWorklist);
      }
    }
  }

  static void AddWorkList(G::Node<TEMP::Temp>* n) {
    if (!precolor(n) && !MoveRelated(n) && degree(n) < 15) {
      freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(n, nullptr));
      simplifyWorklist = new G::NodeList<TEMP::Temp>(n, simplifyWorklist);
    }
  }

  static void Simplify() {
    G::Node<TEMP::Temp>* node = simplifyWorklist -> head;
    simplifyWorklist = simplifyWorklist -> tail;
    selectStack = new G::NodeList<TEMP::Temp>(node, selectStack);
    G::NodeList<TEMP::Temp>* adj = node -> Adj();

    for (; adj; adj = adj -> tail) {
      DecrementDegree(adj -> head);
    }
  }

  static void Combine(G::Node<TEMP::Temp>* v, G::Node<TEMP::Temp>* u) {
    if (freezeWorklist -> InNodeList(u)) {
      freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(u, nullptr));
    } else {
      spillWorklist = G::NodeList<TEMP::Temp>::SubList(spillWorklist, new G::NodeList<TEMP::Temp>(u, nullptr));
    }

    coalescedNodes = new G::NodeList<TEMP::Temp>(u, coalescedNodes);
    aliastable -> Enter(u, v);
    movelisttable -> Enter(v, LIVE::unionMoveList(movelist(v), movelist(u)));

    for (G::NodeList<TEMP::Temp>* adj = u -> Adj(); adj; adj = adj -> tail) {
      G::Node<TEMP::Temp>* t = adj -> head;
      G::Graph<TEMP::Temp>::AddEdge(t, v);
      DecrementDegree(t);
    }
    if (degree(v) >= 15 && freezeWorklist -> InNodeList(v)) {
      freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(v, nullptr));
      spillWorklist = new G::NodeList<TEMP::Temp>(v, spillWorklist);
    }
  }

  static bool george(G::Node<TEMP::Temp>* u, G::Node<TEMP::Temp>* v) {
    bool ok = true;

    for (G::NodeList<TEMP::Temp>* adj = u -> Adj(); adj; adj = adj -> tail) {
      G::Node<TEMP::Temp>* t = adj -> head;
      if (!(t -> Degree() < 15 || precolor(t) || v -> Adj() -> InNodeList(t))) {
        ok = false;
        break;
      }
    }

    return ok;
  }

  static bool briggs(G::NodeList<TEMP::Temp>* nodes) {
    int count = 0;

    for (; nodes; nodes = nodes -> tail) {
      if (nodes -> head -> Degree() >= 15) {
        count = count + 1;
      }
    }
    
    return count < 15;
  }

  static void coalesce() {
    G::Node<TEMP::Temp>* x;
    G::Node<TEMP::Temp>* y;
    G::Node<TEMP::Temp>* v;
    G::Node<TEMP::Temp>* u;
    x = worklistMoves -> src;
    y = worklistMoves -> dst;

    if (precolor(getalias(y))) {
      v = getalias(y);
      u = getalias(x);
    } else {
      v = getalias(x);
      u = getalias(y);
    }
    worklistMoves = worklistMoves -> tail;

    if (v == u) {
      coalescedMoves = new LIVE::MoveList(x, y, coalescedMoves);
      AddWorkList(v);
    } else if (precolor(u) || u -> Adj() -> InNodeList(v)) {
      constrainedMoves = new LIVE::MoveList(x, y, constrainedMoves);
      AddWorkList(v);
      AddWorkList(u);
    } else if ((precolor(v) && george(u, v)) || (!precolor(v) && briggs(G::NodeList<TEMP::Temp>::UnionNodeList(v -> Adj(), u -> Adj())))) {
      coalescedMoves = new LIVE::MoveList(x, y, coalescedMoves);
      Combine(v, u);
      AddWorkList(v);
    } else {
      activeMoves = new LIVE::MoveList(x, y, activeMoves);
    }
  }

  static void Freeze() {
    G::Node<TEMP::Temp>* v = freezeWorklist -> head;
    freezeWorklist = freezeWorklist -> tail;
    simplifyWorklist = new G::NodeList<TEMP::Temp>(v, simplifyWorklist);
    freezemoves(v);
  }

  Result RegAlloc(F::Frame * f, AS::InstrList * il) {
    // TODO: Put your codes here (lab6).
    Result * ret = new Result(nullptr, nullptr);
    bool done = false;

    do {
      done = true;
      G::Graph<AS::Instr>* fg = FG::AssemFlowGraph(il, f);
      live = LIVE::Liveness(fg);
      Build();
      MakeWorklist();

      while (simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist) {
        if (simplifyWorklist) {
          Simplify();
        } else if (worklistMoves) {
          coalesce();
        } else if (freezeWorklist) {
          Freeze();
        } else if (spillWorklist) {
          SelectSpill();
        }
      }

      AssignColors();

      if (spilledNodes) {
        RewriteProgram(f, il);
        done = false;
      }

    } while (!done);

    ret -> coloring = assignregisters(live);
    ret -> il = il;

    return *ret;
  }

} // namespace RA