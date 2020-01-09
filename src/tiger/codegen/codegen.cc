#include "tiger/codegen/codegen.h"
#include <iostream>
#include <sstream>

namespace CG {

static TEMP::Temp* rbxtemp = nullptr;
static TEMP::Temp* rbptemp = nullptr;
static TEMP::Temp* r12temp = nullptr;
static TEMP::Temp* r13temp = nullptr;
static TEMP::Temp* r14temp = nullptr;
static TEMP::Temp* r15temp = nullptr;
static AS::InstrList* instrlist = nullptr;
static AS::InstrList* instrlisttemp = nullptr;
static std::string fs;

static void emit(AS::Instr* instr) {
  if (instrlist) {
    instrlisttemp->tail = new AS::InstrList(instr, nullptr);
    instrlisttemp = instrlisttemp->tail;
  } else {
    instrlist = new AS::InstrList(instr, nullptr);
    instrlisttemp = instrlist;
  }
}

AS::InstrList* Codegen(F::Frame* f, T::StmList* stmList) {
  // TODO: Put your codes here (lab6).
  // AS::InstrList* list;
  // T::StmList* stmlist;

  // sprintf(fs, "%s_framesize", const_cast<char*>(f->name->Name().c_str()));
  fs = TEMP::LabelString(f->name) + "_framesize";
  instrlist = nullptr;
  saveCalleeReg();

  for (; stmList; stmList = stmList->tail) {
    munchStatement(stmList->head);
  }

  restoreCalleReg();
  // list = instrlist;

  return F::F_procEntryExit2(instrlist);
}

static void saveCalleeReg() {
  rbxtemp = TEMP::Temp::NewTemp();
  rbptemp = TEMP::Temp::NewTemp();
  r12temp = TEMP::Temp::NewTemp();
  r13temp = TEMP::Temp::NewTemp();
  r14temp = TEMP::Temp::NewTemp();
  r15temp = TEMP::Temp::NewTemp();

  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(rbxtemp, nullptr), new TEMP::TempList(F::RBX(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(rbptemp, nullptr), new TEMP::TempList(F::RBP(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(r12temp, nullptr), new TEMP::TempList(F::R12(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(r13temp, nullptr), new TEMP::TempList(F::R13(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(r14temp, nullptr), new TEMP::TempList(F::R14(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(r15temp, nullptr), new TEMP::TempList(F::R15(), nullptr)));
}

static void restoreCalleReg() {
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::RBX(), nullptr), new TEMP::TempList(rbxtemp, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::RBP(), nullptr), new TEMP::TempList(rbptemp, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::R12(), nullptr), new TEMP::TempList(r12temp, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::R13(), nullptr), new TEMP::TempList(r13temp, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::R14(), nullptr), new TEMP::TempList(r14temp, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::R15(), nullptr), new TEMP::TempList(r15temp, nullptr)));
}

static void munchStatement(T::Stm* stm) {
  switch (stm->kind) {
    case T::Stm::MOVE: {
      T::Exp* src = static_cast<T::MoveStm*>(stm)->src;
      T::Exp* dst = static_cast<T::MoveStm*>(stm)->dst;

      if (dst->kind == T::Exp::TEMP) {
        TEMP::Temp* left = munchExpression(src);
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(static_cast<T::TempExp*>(dst)->temp, nullptr), new TEMP::TempList(left, nullptr)));
        return;
      }

      if (dst->kind == T::Exp::MEM) {
        TEMP::Temp* left = munchExpression(src);
        TEMP::Temp* right = munchExpression(static_cast<T::MemExp*>(dst)->exp);
        emit(new AS::MoveInstr("movq `s0,(`s1)", nullptr, new TEMP::TempList(left, new TEMP::TempList(right, nullptr))));
        return;
      }

      assert(0);
    }
    case T::Stm::EXP: {
      munchExpression(static_cast<T::ExpStm*>(stm)->exp);
      return;
    }
    case T::Stm::LABEL: {
      TEMP::Label* label = static_cast<T::LabelStm*>(stm)->label;
      // char* labelc = (char*)malloc(256);
      // sprintf(labelc, "%s", const_cast<char*>(TEMP::LabelString(label).c_str()));
      // emit(new AS::LabelInstr(labelc, label));
      emit(new AS::LabelInstr(label->Name(), label));
      return;
    }
    case T::Stm::JUMP: {
      std::string label = static_cast<T::JumpStm*>(stm)->exp->name->Name();
      // char* labelj = (char*)malloc(256);
      // sprintf(labelj, "jmp %s", const_cast<char*>(TEMP::LabelString(label).c_str()));
      // emit(new AS::OperInstr(labelj, nullptr, nullptr, new AS::Targets(static_cast<T::JumpStm*>(stm)->jumps)));
      emit(new AS::OperInstr("jmp " + label, nullptr, nullptr, new AS::Targets(static_cast<T::JumpStm*>(stm)->jumps)));
      return;
    }
    case T::Stm::CJUMP: {
      char* cjump = (char*)malloc(256);
      std::string op;
      TEMP::Temp* left = munchExpression(static_cast<T::CjumpStm*>(stm)->left);
      TEMP::Temp* right = munchExpression(static_cast<T::CjumpStm*>(stm)->right);

      switch (static_cast<T::CjumpStm*>(stm)->op) {
        case T::EQ_OP:
          op = "je ";
          break;
        case T::NE_OP:
          op = "jne ";
          break;
        case T::LT_OP:
          op = "jl ";
          break;
        case T::LE_OP:
          op = "jle ";
          break;
        case T::GT_OP:
          op = "jg ";
          break;
        case T::GE_OP:
          op = "jge ";
          break;
      }
      emit(new AS::OperInstr("cmp `s0,`s1", nullptr, new TEMP::TempList(right, new TEMP::TempList(left, nullptr)), new AS::Targets(nullptr)));
      // sprintf(cjump, "%s %s", op, const_cast<char*>(TEMP::LabelString(static_cast<T::CjumpStm*>(stm)->true_label).c_str()));
      // emit(new AS::OperInstr(cjump, nullptr, nullptr, new AS::Targets(new TEMP::LabelList(static_cast<T::CjumpStm*>(stm)->true_label, nullptr))));
      emit(new AS::OperInstr(op + TEMP::LabelString(static_cast<T::CjumpStm*>(stm)->true_label), nullptr, nullptr, new AS::Targets(new TEMP::LabelList(static_cast<T::CjumpStm*>(stm)->true_label, nullptr))));
      return;
    }
  }
}

static TEMP::Temp* munchExpression(T::Exp* exp) {
  TEMP::Temp* r = TEMP::Temp::NewTemp();

  switch (exp->kind) {
    case T::Exp::MEM: {
      T::Exp* addr = static_cast<T::MemExp*>(exp)->exp;

      if (addr->kind == T::Exp::BINOP) {
        T::Exp* left = static_cast<T::BinopExp*>(addr)->left;
        T::Exp* right = static_cast<T::BinopExp*>(addr)->right;

        if (left->kind == T::Exp::TEMP && static_cast<T::TempExp*>(left)->temp == F::FP()) {
          if (right->kind != T::Exp::CONST) {
            assert(0);
          }

          int offset = static_cast<T::ConstExp*>(right)->consti;
          // char* instruction = (char*)malloc(256);
          // sprintf(instruction, "movq (%s-%#x)(`s0),`d0", fs, -offset);
          // emit(new AS::OperInstr(instruction, new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RSP(), nullptr), new AS::Targets(nullptr)));
          std::string instr;
          std::stringstream ioss;
          ioss << "movq (" + fs + "-0x" << std::hex << -offset << ")(`s0),`d0";
          instr = ioss.str();
          emit(new AS::OperInstr(instr, new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RSP(), nullptr), new AS::Targets(nullptr)));
          return r;
        } else {
          TEMP::Temp* lefttemp = munchExpression(left);
          TEMP::Temp* righttemp = munchExpression(right);
          emit(new AS::OperInstr("addq `s0,`d0", new TEMP::TempList(righttemp, nullptr), new TEMP::TempList(lefttemp, new TEMP::TempList(righttemp, nullptr)), new AS::Targets(nullptr)));
          emit(new AS::OperInstr("movq (`s0),`d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(righttemp, nullptr), new AS::Targets(nullptr)));
          return r;
        }
      } else { // memory address is not an algorithmetic expression
        TEMP::Temp* rtemp = munchExpression(addr);
        emit(new AS::MoveInstr("movq (`s0),`d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(rtemp, nullptr))); // changed
        return r;
      }
    }
    case T::Exp::BINOP: {
      switch (static_cast<T::BinopExp*>(exp)->op) {
        case T::PLUS_OP: {
          if (static_cast<T::BinopExp*>(exp)->left->kind == T::Exp::TEMP && munchExpression(static_cast<T::BinopExp*>(exp)->left) == F::FP() && static_cast<T::BinopExp*>(exp)->right->kind == T::Exp::CONST) {
            // char* temp = (char*)malloc(256);
            // sprintf(temp, "leaq (%s-%#x)(`s0),`d0", fs, -static_cast<T::ConstExp*>(static_cast<T::BinopExp*>(exp)->right)->consti);
            // emit(new AS::OperInstr(temp, new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RSP(), nullptr), new AS::Targets(nullptr)));
            int rightvalue = static_cast<T::ConstExp*>(static_cast<T::BinopExp*>(exp)->right)->consti;
            std::string instr;
            std::stringstream ioss;
            ioss << "leaq (" + fs + "-0x" << std::hex << -rightvalue << ")(`s0),`d0";
            instr = ioss.str();
            emit(new AS::OperInstr(instr, new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RSP(), nullptr), new AS::Targets(nullptr)));
          } else {
            TEMP::Temp* left = munchExpression(static_cast<T::BinopExp*>(exp)->left);
            TEMP::Temp* right = munchExpression(static_cast<T::BinopExp*>(exp)->right);
            emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(left, nullptr)));
            emit(new AS::OperInstr("addq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(right , new TEMP::TempList(r, nullptr)), new AS::Targets(nullptr)));
          }
          return r;
        }
        case T::MINUS_OP: {
          TEMP::Temp* left = munchExpression(static_cast<T::BinopExp*>(exp)->left);
          TEMP::Temp* right = munchExpression(static_cast<T::BinopExp*>(exp)->right);
          emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(left, nullptr)));
          emit(new AS::OperInstr("subq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(right , new TEMP::TempList(r, nullptr)), new AS::Targets(nullptr)));
          return r;
        }
        case T::MUL_OP: {
          TEMP::Temp* left = munchExpression(static_cast<T::BinopExp*>(exp)->left);
          TEMP::Temp* right = munchExpression(static_cast<T::BinopExp*>(exp)->right);
          emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(left, nullptr)));
          emit(new AS::OperInstr("imulq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(right , nullptr), new AS::Targets(nullptr))); // changed
          return r;
        }
        case T::DIV_OP: {
          TEMP::Temp* left = munchExpression(static_cast<T::BinopExp*>(exp)->left);
          TEMP::Temp* right = munchExpression(static_cast<T::BinopExp*>(exp)->right);
          emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(F::RAX(), nullptr), new TEMP::TempList(left, nullptr)));
          emit(new AS::OperInstr("cltd", new TEMP::TempList(F::RAX(), new TEMP::TempList(F::RDX(), nullptr)), new TEMP::TempList(F::RAX(), nullptr), new AS::Targets(nullptr)));
          emit(new AS::OperInstr("idivq `s0", nullptr, new TEMP::TempList(right, nullptr), new AS::Targets(nullptr))); // changed
          emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RAX(), nullptr)));
          return r;
        }
      }
    }
    case T::Exp::TEMP: {
      TEMP::Temp* temp = static_cast<T::TempExp*>(exp)->temp;

      if (temp == F::FP()) {
        // char* instruction = (char*)malloc(256);
        // temp = TEMP::Temp::NewTemp();
        // sprintf(instruction, "leaq %s(`s0),`d0", fs);
        // emit(new AS::OperInstr(instruction, new TEMP::TempList(temp, nullptr), new TEMP::TempList(F::RSP(), nullptr), new AS::Targets(nullptr)));
        std::string inst = "leaq " + fs + "(`s0),`d0";
        temp = TEMP::Temp::NewTemp();
        emit(new AS::OperInstr(inst, new TEMP::TempList(temp, nullptr), new TEMP::TempList(F::RSP(), nullptr), new AS::Targets(nullptr)));
      }

      return temp;
    }
    case T::Exp::NAME: {
      // char* instruction = (char*)malloc(256);
      // sprintf(instruction, "movq $%s,`d0", const_cast<char*>(TEMP::LabelString(static_cast<T::NameExp*>(exp)->name).c_str()));
      // std::string assem = "leaq " + static_cast<T::NameExp*>(exp)->name->Name() + "(%rip),`d0"; // changed
      // emit(new AS::OperInstr(instruction, new TEMP::TempList(r, nullptr), nullptr, new AS::Targets(nullptr)));
      std::string assem = "leaq " + static_cast<T::NameExp*>(exp)->name->Name() + "(%rip),`d0";
      emit(new AS::OperInstr(assem, new TEMP::TempList(r, nullptr), nullptr, new AS::Targets(nullptr)));
      return r;
    }
    case T::Exp::CONST: {
      // char* instruction = (char*)malloc(256);
      // sprintf(instruction, "movq $%d,`d0", static_cast<T::ConstExp*>(exp)->consti);
      // emit(new AS::OperInstr(instruction, new TEMP::TempList(r, nullptr), nullptr, nullptr)); // changed
      emit(new AS::OperInstr("movq $" + std::to_string(static_cast<T::ConstExp*>(exp)->consti) + " ,`d0", new TEMP::TempList(r, nullptr), nullptr, nullptr));
      return r;
    }
    case T::Exp::CALL: {
      TEMP::Label* funname = static_cast<T::NameExp*>(static_cast<T::CallExp*>(exp)->fun)->name;
      // char* instruction = (char*)malloc(256);
      munchArguments(static_cast<T::CallExp*>(exp)->args);
      // sprintf(instruction, "call %s", const_cast<char*>(TEMP::LabelString(funname).c_str()));
      // emit(new AS::OperInstr(instruction, new TEMP::TempList(F::RAX(), new TEMP::TempList(F::RDI(), new TEMP::TempList(F::RSI(), new TEMP::TempList(F::RDX(), new TEMP::TempList(F::RCX(), new TEMP::TempList(F::R8(), new TEMP::TempList(F::R9(), new TEMP::TempList(F::R10(), new TEMP::TempList(F::R11(), nullptr))))))))), nullptr, new AS::Targets(nullptr)));
      // emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RAX(), nullptr)));
      std::string assem = "call " + TEMP::LabelString(funname);
      emit(new AS::OperInstr(assem, new TEMP::TempList(F::RAX(), new TEMP::TempList(F::RDI(), new TEMP::TempList(F::RSI(), new TEMP::TempList(F::RDX(), new TEMP::TempList(F::RCX(), new TEMP::TempList(F::R8(), new TEMP::TempList(F::R9(), new TEMP::TempList(F::R10(), new TEMP::TempList(F::R11(), nullptr))))))))), nullptr, new AS::Targets(nullptr)));
      emit(new AS::MoveInstr("movq `s0, `d0", new TEMP::TempList(r, nullptr), new TEMP::TempList(F::RAX(), nullptr)));
      return r;
    }
  }
}

static void munchArguments(T::ExpList* explist) {
  int num = 1;
  TEMP::Temp* arguments;

  for (T::ExpList* list = explist; list; list = list->tail, num++) {
    arguments = munchExpression(list->head);

    switch (num) {
      case 1:
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::RDI(), nullptr), new TEMP::TempList(arguments, nullptr)));
        break;
      case 2:
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::RSI(), nullptr), new TEMP::TempList(arguments, nullptr)));
        break;
      case 3:
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::RDX(), nullptr), new TEMP::TempList(arguments, nullptr)));
        break;
      case 4:
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::RCX(), nullptr), new TEMP::TempList(arguments, nullptr)));
        break;
      case 5:
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::R8(), nullptr), new TEMP::TempList(arguments, nullptr)));
        break;
      case 6:
        emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::R9(), nullptr), new TEMP::TempList(arguments, nullptr)));
        break;
      default:
        emit(new AS::OperInstr("pushq `s0", nullptr, new TEMP::TempList(arguments, nullptr), new AS::Targets(nullptr)));
        break;
    }
  }

  return;
}

}  // namespace CG