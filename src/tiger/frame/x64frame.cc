#include "tiger/frame/frame.h"

#include <string>

namespace F {

TEMP::Temp* FP() {
  if (!rbp) {
    rbp = TEMP::Temp::NewTemp();
  }

  return rbp;
}

TEMP::Temp* RBX() {
  if (!rbx) {
    rbx = TEMP::Temp::NewTemp();
  }

  return rbx;
}

TEMP::Temp* RSP() {
  if (!rsp) {
    rsp = TEMP::Temp::NewTemp();
  }

  return rsp;
}

TEMP::Temp* RBP() {
  if (!rbp) {
    rbp = TEMP::Temp::NewTemp();
  }

  return rbp;
}

TEMP::Temp* RAX() {
  if (!rax) {
    rax = TEMP::Temp::NewTemp();
  }

  return rax;
}

TEMP::Temp* RDI() {
  if (!rdi) {
    rdi = TEMP::Temp::NewTemp();
  }

  return rdi;
}

TEMP::Temp* RSI() {
  if (!rsi) {
    rsi = TEMP::Temp::NewTemp();
  }

  return rsi;
}

TEMP::Temp* RDX() {
  if (!rdx) {
    rdx = TEMP::Temp::NewTemp();
  }

  return rdx;
}

TEMP::Temp* RCX() {
  if (!rcx) {
    rcx = TEMP::Temp::NewTemp();
  }

  return rcx;
}

TEMP::Temp* R8() {
  if (!r8) {
    r8 = TEMP::Temp::NewTemp();
  }

  return r8;
}

TEMP::Temp* R9() {
  if (!r9) {
    r9 = TEMP::Temp::NewTemp();
  }

  return r9;
}

TEMP::Temp* R10() {
  if (!r10) {
    r10 = TEMP::Temp::NewTemp();
  }

  return r10;
}

TEMP::Temp* R11() {
  if (!r11) {
    r11 = TEMP::Temp::NewTemp();
  }

  return r11;
}

TEMP::Temp* R12() {
  if (!r12) {
    r12 = TEMP::Temp::NewTemp();
  }

  return r12;
}

TEMP::Temp* R13() {
  if (!r13) {
    r13 = TEMP::Temp::NewTemp();
  }

  return r13;
}

TEMP::Temp* R14() {
  if (!r14) {
    r14 = TEMP::Temp::NewTemp();
  }

  return r14;
}

TEMP::Temp* R15() {
  if (!r15) {
    r15 = TEMP::Temp::NewTemp();
  }

  return r15;
}

class X64Frame : public Frame {
  // TODO: Put your codes here (lab6).
  public:
    X64Frame(TEMP::Label *name,
        F::AccessList *formals,
        F::AccessList *locals,
        T::StmList *view_shift, int offset)
      : Frame(name, formals, locals, view_shift, offset) {}
};

T::Exp* InFrameAccess::ToExp(T::Exp* framePtr) const {
  return new T::MemExp(new T::BinopExp(T::PLUS_OP, framePtr, new T::ConstExp(this->offset)));
}

T::Exp* InRegAccess::ToExp(T::Exp* framePtr) const {
  return new T::TempExp(this->reg);
}

T::Stm* F_procEntryExit1(F::Frame* frame, T::Stm* stm) {
  // T::StmList* list;
  // T::SeqStm *result = new T::SeqStm(stm, new T::ExpStm(new T::ConstExp(0)));

  // for (list = frame->view_shift; list; list = list->tail) {
  //   result = new T::SeqStm(list->head, result);
  // }

  // return result;
  T::StmList *iter = frame->view_shift;
  T::SeqStm *res = new T::SeqStm(stm, new T::ExpStm(new T::ConstExp(0)));
  while (iter && iter->head)
  {
    res = new T::SeqStm(iter->head, res);
    iter = iter->tail;
  }
  return res;
}

AS::InstrList* F_procEntryExit2(AS::InstrList* body) {
  static TEMP::TempList* temp = nullptr;

  if (!temp) {
    temp = new TEMP::TempList(F::RSP(), new TEMP::TempList(F::RAX(), nullptr));
  }

  return AS::InstrList::Splice(body, new AS::InstrList(new AS::OperInstr("", nullptr, temp, new AS::Targets(nullptr)), nullptr));
}

AS::Proc* F_procEntryExit3(F::Frame* frame, AS::InstrList* body) {
  char* prologue = (char*)malloc(256);
  char* epilogue = (char*)malloc(256);
  char* fs = (char*)malloc(20);
  sprintf(fs, "%s_framesize", const_cast<char*>(TEMP::LabelString(frame->name).c_str()));
	sprintf(prologue, ".set %s,%#x\nsubq $%#x,%%rsp\n", fs, -frame->offset, -frame->offset);
	sprintf(epilogue, "addq $%#x,%%rsp\nret\n\n", -frame->offset);

	return new AS::Proc(prologue, body, epilogue);
}

Frame* NewFrame(TEMP::Label* name, U::BoolList* escapes) {
  F::Frame* newframe = new X64Frame(name, nullptr, nullptr, nullptr, -8);
  newframe->name = name;
  F::AccessList* formals = new F::AccessList(nullptr, nullptr);
  T::StmList* view_shift = new T::StmList(nullptr, nullptr);
  F::AccessList* formalstemp = formals;
  T::StmList* view_shifttemp = view_shift;
  newframe->offset = -8;
  bool escape;
  TEMP::Temp* temp;
  int lastarg = wordsize;
  int numarg = 0;

  for (; escapes; escapes = escapes->tail, ++numarg) {
    escape = escapes->head;

    if (escape) {
      switch (numarg) {
        case 0:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::BinopExp(T::PLUS_OP, new T::TempExp(F::FP()), new T::ConstExp(newframe->offset))), new T::TempExp(F::RDI())), nullptr);
          newframe->offset -= wordsize;
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(newframe->offset), nullptr);
          formalstemp = formalstemp->tail;
          view_shifttemp = view_shifttemp->tail;
          break;
        case 1:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::BinopExp(T::PLUS_OP, new T::TempExp(F::FP()), new T::ConstExp(newframe->offset))), new T::TempExp(F::RSI())), nullptr);
          newframe->offset -= wordsize;
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(newframe->offset), nullptr);
          formalstemp = formalstemp->tail;
          view_shifttemp = view_shifttemp->tail;
          break;
        case 2:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::BinopExp(T::PLUS_OP, new T::TempExp(F::FP()), new T::ConstExp(newframe->offset))), new T::TempExp(F::RDX())), nullptr);
          newframe->offset -= wordsize;
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(newframe->offset), nullptr);
          formalstemp = formalstemp->tail;
          view_shifttemp = view_shifttemp->tail;
          break;
        case 3:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::BinopExp(T::PLUS_OP, new T::TempExp(F::FP()), new T::ConstExp(newframe->offset))), new T::TempExp(F::RCX())), nullptr);
          newframe->offset -= wordsize;
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(newframe->offset), nullptr);
          formalstemp = formalstemp->tail;
          view_shifttemp = view_shifttemp->tail;
          break;
        case 4:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::BinopExp(T::PLUS_OP, new T::TempExp(F::FP()), new T::ConstExp(newframe->offset))), new T::TempExp(F::R8())), nullptr);
          newframe->offset -= wordsize;
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(newframe->offset), nullptr);
          formalstemp = formalstemp->tail;
          view_shifttemp = view_shifttemp->tail;
          break;
        case 5:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::MemExp(new T::BinopExp(T::PLUS_OP, new T::TempExp(F::FP()), new T::ConstExp(newframe->offset))), new T::TempExp(F::R9())), nullptr);
          newframe->offset -= wordsize;
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(newframe->offset), nullptr);
          formalstemp = formalstemp->tail;
          view_shifttemp = view_shifttemp->tail;
          break;
        default:
          formalstemp->tail = new F::AccessList(new F::InFrameAccess(lastarg), nullptr);
          formalstemp = formalstemp->tail;
          lastarg += wordsize;
      }
    } else {
      temp = TEMP::Temp::NewTemp();

      switch (numarg) {
        case 0:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(F::RDI())), nullptr);
          view_shifttemp = view_shifttemp->tail;
          formalstemp->tail = new F::AccessList(new F::InRegAccess(temp), nullptr);
          formalstemp = formalstemp->tail;
          break;
        case 1:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(F::RSI())), nullptr);
          view_shifttemp = view_shifttemp->tail;
          formalstemp->tail = new F::AccessList(new F::InRegAccess(temp), nullptr);
          formalstemp = formalstemp->tail;
          break;
        case 2:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(F::RDX())), nullptr);
          view_shifttemp = view_shifttemp->tail;
          formalstemp->tail = new F::AccessList(new F::InRegAccess(temp), nullptr);
          formalstemp = formalstemp->tail;
          break;
        case 3:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(F::RCX())), nullptr);
          view_shifttemp = view_shifttemp->tail;
          formalstemp->tail = new F::AccessList(new F::InRegAccess(temp), nullptr);
          formalstemp = formalstemp->tail;
          break;
        case 4:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(F::R8())), nullptr);
          view_shifttemp = view_shifttemp->tail;
          formalstemp->tail = new F::AccessList(new F::InRegAccess(temp), nullptr);
          formalstemp = formalstemp->tail;
          break;
        case 5:
          view_shifttemp->tail = new T::StmList(new T::MoveStm(new T::TempExp(temp), new T::TempExp(F::R9())), nullptr);
          view_shifttemp = view_shifttemp->tail;
          formalstemp->tail = new F::AccessList(new F::InRegAccess(temp), nullptr);
          formalstemp = formalstemp->tail;
          break;
        default:
          // should be passed on frame 
          ;
      }
    }
  }

  formals = formals->tail;
  view_shift = view_shift->tail;
  newframe->formals = formals;
  newframe->locals = nullptr;
  newframe->view_shift = view_shift;

  return newframe;
}

F::Access *F_allocLocal(Frame *frame, bool escape)
{
  F::Access *local;
  if (escape)
  {
    local = new F::InFrameAccess(frame->offset);
    frame->offset -= wordsize;
  }
  else
  {
    local = new F::InRegAccess(TEMP::Temp::NewTemp());
  }
  return local;
}

T::CallExp *F_externalCall(std::string s, T::ExpList *args)
{
  return new T::CallExp(new T::NameExp(TEMP::NamedLabel(s)), args);
}

}  // namespace F