#include "tiger/escape/escape.h"

namespace ESC {

class EscapeEntry {
 public:
  int depth;
  bool* escape;

  EscapeEntry(int depth, bool* escape) : depth(depth), escape(escape) {}
};

static void transverseExp(S::Table<EscapeEntry>* env, int depth, A::Exp* exp);

void FindEscape(A::Exp* exp) {
  // TODO: Put your codes here (lab6).
  S::Table<EscapeEntry>* env = new S::Table<EscapeEntry>();
  transverseExp(env, 1, exp);
}

static void transverseDec(S::Table<EscapeEntry>* env, int depth, A::Dec* dec);
static void transverseVar(S::Table<EscapeEntry>* env, int depth, A::Var* var);

static void transverseExp(S::Table<EscapeEntry>* env, int depth, A::Exp* exp) {
  switch (exp->kind) {
    case A::Exp::VAR: {
      transverseVar(env, depth, static_cast<A::VarExp*>(exp)->var);
      break;
    }
    case A::Exp::CALL: {
      A::ExpList* args = static_cast<A::CallExp*>(exp)->args;

      for (; args; args = args->tail) {
        transverseExp(env, depth, args->head);
      }

      break;
    }
    case A::Exp::OP: {
      transverseExp(env, depth, static_cast<A::OpExp*>(exp)->left);
      transverseExp(env, depth, static_cast<A::OpExp*>(exp)->right);
      break;
    }
    case A::Exp::RECORD: {
      A::EFieldList* fields = static_cast<A::RecordExp*>(exp)->fields;

      for (; fields; fields = fields->tail) {
        transverseExp(env, depth, fields->head->exp);
      }

      break;
    }
    case A::Exp::SEQ: {
      A::ExpList* seq = static_cast<A::SeqExp*>(exp)->seq;

      for (; seq; seq = seq->tail) {
        transverseExp(env, depth, seq->head);
      }

      break;
    }
    case A::Exp::ASSIGN: {
      transverseVar(env, depth, static_cast<A::AssignExp*>(exp)->var);
      transverseExp(env, depth, static_cast<A::AssignExp*>(exp)->exp);
      break;
    }
    case A::Exp::IF: {
      transverseExp(env, depth, static_cast<A::IfExp*>(exp)->test);
      transverseExp(env, depth, static_cast<A::IfExp*>(exp)->then);

      if (static_cast<A::IfExp*>(exp)->elsee) {
        transverseExp(env, depth, static_cast<A::IfExp*>(exp)->elsee);
      }

      break;
    }
    case A::Exp::WHILE: {
      transverseExp(env, depth, static_cast<A::WhileExp*>(exp)->test);
      transverseExp(env, depth, static_cast<A::WhileExp*>(exp)->body);
      break;
    }
    case A::Exp::FOR: {
      static_cast<A::ForExp*>(exp)->escape = false;
      env->Enter(static_cast<A::ForExp*>(exp)->var, new EscapeEntry(depth, &static_cast<A::ForExp*>(exp)->escape));
      transverseExp(env, depth, static_cast<A::ForExp*>(exp)->lo);
      transverseExp(env, depth, static_cast<A::ForExp*>(exp)->hi);
      transverseExp(env, depth, static_cast<A::ForExp*>(exp)->body);
      break;
    }
    case A::Exp::LET: {
      for (A::DecList* decs = static_cast<A::LetExp*>(exp)->decs; decs; decs = decs->tail) {
        transverseDec(env, depth, decs->head);
      }

      transverseExp(env, depth, static_cast<A::LetExp*>(exp)->body);
      break;
    }
    case A::Exp::ARRAY: {
      transverseExp(env, depth, static_cast<A::ArrayExp*>(exp)->init);
      transverseExp(env, depth, static_cast<A::ArrayExp*>(exp)->size);
      break;
    }
    default: break;
  }
}

static void transverseVar(S::Table<EscapeEntry>* env, int depth, A::Var* var) {
  switch (var->kind) {
    case A::Var::SIMPLE: {
      EscapeEntry* entry = env->Look(static_cast<A::SimpleVar*>(var)->sym);

      if (depth > entry->depth) {
        *(entry->escape) = true;
      }

      break;
    }
    case A::Var::FIELD: {
      transverseVar(env, depth, static_cast<A::FieldVar*>(var)->var);
      break;
    }
    case A::Var::SUBSCRIPT: {
      transverseVar(env, depth, static_cast<A::SubscriptVar*>(var)->var);
      transverseExp(env, depth, static_cast<A::SubscriptVar*>(var)->subscript);
      break;
    }
    default: assert(0);
  }
}

static void transverseDec(S::Table<EscapeEntry>* env, int depth, A::Dec* dec) {
  switch (dec->kind) {
    case A::Dec::VAR: {
      static_cast<A::VarDec*>(dec)->escape = false;
      env->Enter(static_cast<A::VarDec*>(dec)->var, new EscapeEntry(depth, &static_cast<A::VarDec*>(dec)->escape));
      transverseExp(env, depth, static_cast<A::VarDec*>(dec)->init);
      break;
    }
    case A::Dec::FUNCTION: {
      for (A::FunDecList* decs = static_cast<A::FunctionDec*>(dec)->functions; decs; decs = decs->tail) {
        A::FunDec* fundec = decs->head;
        env->BeginScope();
        A::FieldList* formals = fundec->params;

        for (; formals; formals = formals->tail) {
          A::Field* formal = formals->head;
          formal->escape = false;
          env->Enter(formal->name, new EscapeEntry(depth+1, &formal->escape));
        }

        transverseExp(env, depth+1, fundec->body);
        env->EndScope();
      }
      break;
    }
    default: break;
  }
}

}  // namespace ESC