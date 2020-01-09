#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/translate/tree.h"
#include "tiger/util/util.h"

namespace F {

class Frame;
class AccessList;

const int wordsize = 8;

// virtual stack pointer
static TEMP::Temp* rsp = nullptr;

// virtual frame pointer
static TEMP::Temp* fp = nullptr;

// caller saved registers
static TEMP::Temp* rax = nullptr;
static TEMP::Temp* rdi = nullptr;
static TEMP::Temp* rsi = nullptr;
static TEMP::Temp* rdx = nullptr;
static TEMP::Temp* rcx = nullptr;
static TEMP::Temp* r8 = nullptr;
static TEMP::Temp* r9 = nullptr;
static TEMP::Temp* r10 = nullptr;
static TEMP::Temp* r11 = nullptr;

// callee saved registers
static TEMP::Temp* rbp = nullptr;
static TEMP::Temp* rbx = nullptr;
static TEMP::Temp* r12 = nullptr;
static TEMP::Temp* r13 = nullptr;
static TEMP::Temp* r14 = nullptr;
static TEMP::Temp* r15 = nullptr;

// registers helper functions
TEMP::Temp* FP(); // RBP
TEMP::Temp* RBX();
TEMP::Temp* RSP();
TEMP::Temp* RBP();
TEMP::Temp* RAX(); // RV
TEMP::Temp* RDI();
TEMP::Temp* RSI();
TEMP::Temp* RDX();
TEMP::Temp* RCX();
TEMP::Temp* R8();
TEMP::Temp* R9();
TEMP::Temp* R10();
TEMP::Temp* R11();
TEMP::Temp* R12();
TEMP::Temp* R13();
TEMP::Temp* R14();
TEMP::Temp* R15();

class Frame {
  // Base class
  public:
    TEMP::Label* name;
    F::AccessList* formals;
    F::AccessList* locals;
    T::StmList* view_shift;
    int offset;

    Frame(TEMP::Label *name,
        F::AccessList *formals,
        F::AccessList *locals,
        T::StmList *view_shift, int offset)
      : name(name), formals(formals), locals(locals), view_shift(view_shift), offset(offset) {}
};

class Access {
 public:
  enum Kind { INFRAME, INREG };

  Kind kind;

  Access(Kind kind) : kind(kind) {}

  virtual T::Exp* ToExp(T::Exp* framePtr) const = 0;

  // Hints: You may add interface like
  //        `virtual T::Exp* ToExp(T::Exp* framePtr) const = 0`
};

class InFrameAccess : public Access {
 public:
  int offset;

  T::Exp* ToExp(T::Exp* framePtr) const override;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  T::Exp* ToExp(T::Exp* framePtr) const override;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
};

class AccessList {
 public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

/*
 * Fragments
 */

class Frag {
 public:
  enum Kind { STRING, PROC };

  Kind kind;

  Frag(Kind kind) : kind(kind) {}
};

class StringFrag : public Frag {
 public:
  TEMP::Label *label;
  std::string str;

  StringFrag(TEMP::Label *label, std::string str)
      : Frag(STRING), label(label), str(str) {}
};

class ProcFrag : public Frag {
 public:
  T::Stm *body;
  Frame *frame;

  ProcFrag(T::Stm *body, Frame *frame) : Frag(PROC), body(body), frame(frame) {}
};

class FragList {
 public:
  Frag *head;
  FragList *tail;

  FragList(Frag *head, FragList *tail) : head(head), tail(tail) {}
};

// self-written functions
F::Frame* NewFrame(TEMP::Label* name, U::BoolList* escapes);
T::Stm* F_procEntryExit1(F::Frame* frame, T::Stm* stm);
AS::InstrList* F_procEntryExit2(AS::InstrList* body);
AS::Proc* F_procEntryExit3(F::Frame* frame, AS::InstrList* body);
T::CallExp *F_externalCall(std::string s, T::ExpList *args);
F::Access *F_allocLocal(Frame *frame, bool escape);

}  // namespace F

#endif