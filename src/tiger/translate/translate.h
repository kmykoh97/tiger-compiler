#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include "tiger/absyn/absyn.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/types.h"

/* Forward Declarations */
namespace A {
  class Exp;
} // namespace A

namespace TR {

  class Level;

  class Access {
    public:
      TR::Level * level;
    F::Access * access;

    Access(Level * level, F::Access * access): level(level), access(access) {}
    static Access * AllocLocal(TR::Level * level, bool escape);
  };

  class AccessList {
    public:
      Access * head;
    AccessList * tail;

    AccessList(Access * head, AccessList * tail): head(head), tail(tail) {}
  };

  class Level {
    public:
      F::Frame * frame;
    Level * parent;

    Level(F::Frame * frame, Level * parent): frame(frame), parent(parent) {}
    AccessList * Formals(Level * level) {
      return nullptr;
    }

    static Level * NewLevel(Level * parent, TEMP::Label * name, U::BoolList * formals);
  };

  class PatchList {
    public:
      TEMP::Label ** head;
    PatchList * tail;

    PatchList(TEMP::Label ** head, PatchList * tail): head(head), tail(tail) {}
  };

  class Cx {
    public:
      PatchList * trues;
    PatchList * falses;
    T::Stm * stm;

    Cx(PatchList * trues, PatchList * falses, T::Stm * stm): trues(trues), falses(falses), stm(stm) {}
  };

  class Exp {
    public:
      enum Kind {
        EX,
        NX,
        CX
      };

    Kind kind;

    Exp(Kind kind): kind(kind) {}

    virtual T::Exp * UnEx() const = 0;
    virtual T::Stm * UnNx() const = 0;
    virtual Cx UnCx() const = 0;
  };

  class ExpAndTy {
    public:
      TR::Exp * exp;
    TY::Ty * ty;

    ExpAndTy(TR::Exp * exp, TY::Ty * ty): exp(exp), ty(ty) {}
  };

  class ExExp: public Exp {
    public: T::Exp * exp;

    ExExp(T::Exp * exp): Exp(EX),
    exp(exp) {}

    T::Exp * UnEx() const override {
      return this -> exp;
    }
    T::Stm * UnNx() const override {
      return new T::ExpStm(this -> exp);
    }
    Cx UnCx() const override {
      Cx * uncx = new TR::Cx(nullptr, nullptr, nullptr);
      uncx -> stm = new T::CjumpStm(T::NE_OP, this -> exp, new T::ConstExp(0), nullptr, nullptr);
      uncx -> trues = new PatchList( & (((T::CjumpStm * ) uncx -> stm) -> true_label), nullptr);
      uncx -> falses = new PatchList( & (((T::CjumpStm * ) uncx -> stm) -> false_label), nullptr);
      return *uncx;
    }
  };

  class NxExp: public Exp {
    public: T::Stm * stm;

    NxExp(T::Stm * stm): Exp(NX),
    stm(stm) {}

    T::Exp * UnEx() const override {
      return new T::EseqExp(this -> stm, new T::ConstExp(0));
    }
    T::Stm * UnNx() const override {
      return this -> stm;
    }
    Cx UnCx() const override {}
  };

  void do_patch(PatchList * tList, TEMP::Label * label);

  class CxExp: public Exp {
    public: Cx cx;

    CxExp(struct Cx cx): Exp(CX),
    cx(cx) {}
    CxExp(PatchList * trues, PatchList * falses, T::Stm * stm): Exp(CX),
    cx(trues, falses, stm) {}

    T::Exp * UnEx() const override {
      TEMP::Temp * r = TEMP::Temp::NewTemp();
      TEMP::Label * t = TEMP::NewLabel();
      TEMP::Label * f = TEMP::NewLabel();
      do_patch(this -> cx.trues, t);
      do_patch(this -> cx.falses, f);
      return new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(1)),
        new T::EseqExp(this -> cx.stm,
          new T::EseqExp(new T::LabelStm(f),
            new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(0)),
              new T::EseqExp(new T::LabelStm(t), new T::TempExp(r))))));
    }

    T::Stm * UnNx() const override {
      return new T::ExpStm(this -> UnEx());
    }
    Cx UnCx() const override {
      return this -> cx;
    }
  };

  Level * Outermost();

  F::FragList * TranslateProgram(A::Exp * );

} // namespace TR

#endif