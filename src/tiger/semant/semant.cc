#include "tiger/semant/semant.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/env/env.h"

extern EM::ErrorMsg errormsg;

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

namespace {
static TY::TyList *make_formal_tylist(TEnvType tenv, A::FieldList *params) {
  if (params == nullptr) {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(params->head->typ);
  if (ty == nullptr) {
    errormsg.Error(params->head->pos, "undefined type %s",
                   params->head->typ->Name().c_str());
  }

  return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
}

static TY::FieldList *make_fieldlist(TEnvType tenv, A::FieldList *fields) {
  if (fields == nullptr) {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(fields->head->typ);
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}

}  // namespace

namespace A {

TY::Ty *SimpleVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  E::EnvEntry* e = venv->Look(this->sym);

  if (e && e->kind == E::EnvEntry::VAR) {
    // errormsg.Error(this->pos, "test%d", static_cast<E::VarEntry*>(e)->ty->ActualTy()->kind);
    return static_cast<E::VarEntry*>(e)->ty->ActualTy();
  } else {
    errormsg.Error(this->pos, "undefined variable %s", this->sym->Name().c_str());
    return TY::IntTy::Instance();
  }
}

TY::Ty *FieldVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::Var* var = this->var;
  S::Symbol* sym = this->sym;
  TY::Ty* varType = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* actualVarType = varType->ActualTy();

  if (actualVarType->kind != TY::Ty::RECORD) {
    errormsg.Error(this->pos, "not a record type");
    return TY::IntTy::Instance();
  } else {
    TY::FieldList* actualVarTypeFieldList = static_cast<TY::RecordTy*>(actualVarType)->fields;
    TY::Field* field;

    while (actualVarTypeFieldList != nullptr) {
      field = actualVarTypeFieldList->head;

      if (field->name == sym) {
        return field->ty->ActualTy();
      }

      actualVarTypeFieldList = actualVarTypeFieldList->tail;
    }

    errormsg.Error(this->pos, "field %s doesn't exist", sym->Name().c_str());

    return TY::StringTy::Instance();
  }
}

TY::Ty *SubscriptVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                                 int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::Var* var = this->var;
  A::Exp* subscript = this->subscript;
  TY::Ty* varType = var->SemAnalyze(venv, tenv, labelcount);

  if (varType->ActualTy()->kind != TY::Ty::ARRAY) {
    errormsg.Error(this->pos, "array type required");
    return TY::IntTy::Instance();
  } else {
    TY::Ty* expType = subscript->SemAnalyze(venv, tenv, labelcount);

    if (expType->ActualTy()->kind != TY::Ty::INT) {
      errormsg.Error(this->pos, "index type is not int");
      return TY::IntTy::Instance();
    }
  }

  // return varType->ActualTy();
  return TY::IntTy::Instance();
}

TY::Ty *VarExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *temp = this->var->SemAnalyze(venv, tenv, labelcount);

  return temp;
}

TY::Ty *NilExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::NilTy::Instance();
}

TY::Ty *IntExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::IntTy::Instance();
}

TY::Ty *StringExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::StringTy::Instance();
}

TY::Ty *CallExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).
  S::Symbol* name = this->func;
  ExpList *arguments = this->args;
  E::EnvEntry* e = venv->Look(this->func);

  if (e == nullptr || e->kind != E::EnvEntry::FUN) {
    errormsg.Error(this->pos, "undefined function %s", this->func->Name().c_str());
    return TY::VoidTy::Instance();
  } else {
    TY::TyList *formals = static_cast<E::FunEntry*>(e)->formals;
    A::Exp* arg;
    TY::Ty* formal;

    for (;arguments && formals; arguments = arguments->tail, formals = formals->tail) {
      arg = arguments->head;
      formal = formals->head;
      TY::Ty* temp = arg->SemAnalyze(venv, tenv, labelcount);

      if (!temp->IsSameType(formal)) {
        errormsg.Error(arg->pos, "para type mismatch");
      }
    }

    if (arguments != nullptr) {
      errormsg.Error(this->pos, "too many params in function %s", this->func->Name().c_str());
    }

    return static_cast<E::FunEntry*>(e)->result->ActualTy();
  }
}

TY::Ty *OpExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::Oper op = this->oper;
  TY::Ty* left = this->left->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* right = this->right->SemAnalyze(venv, tenv, labelcount);

  if (oper == A::PLUS_OP || oper == A::MINUS_OP || oper == A::TIMES_OP || oper == A::DIVIDE_OP) {
    if (left->ActualTy()->kind != TY::Ty::INT) {
      errormsg.Error(this->left->pos, "integer required");
    }

    if (right->ActualTy()->kind != TY::Ty::INT) {
      errormsg.Error(this->right->pos, "integer required");
    }
  } else {
    if (!left->IsSameType(right)) {
      errormsg.Error(this->left->pos, "same type required");
    }
  }

  return TY::IntTy::Instance();
}

TY::Ty *RecordExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  S::Symbol* name = this->typ;
  TY::Ty* temp = tenv->Look(name)->ActualTy();
  // TY::Ty* temp = nullptr;
  // try
  // { errormsg.Error(this->pos, "tebbbst%s", name->Name().c_str());
  //   temp = tenv->Look(name)->ActualTy();
  // }
  // catch(const std::exception& e)
  // { errormsg.Error(this->pos, "tesaaat%s", name->Name().c_str());
  //   temp = tenv->Look(name);
  // }
  
  // TY::Ty* temp = tenv->Look(name);

  if (!temp) {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TY::IntTy::Instance();
  }

  if (temp->ActualTy()->kind != TY::Ty::RECORD) {
    errormsg.Error(this->pos, "not a record type");
    return temp;
  } else {
    A::EFieldList* f1 = this->fields;
    TY::FieldList* f2 = static_cast<TY::RecordTy*>(temp)->fields;
    A::EField* ftemp1;
    TY::Field* ftemp2;

    while (f1 != nullptr) {
      if (f2 == nullptr) {
        errormsg.Error(this->pos, "Too many efields in %s", this->typ->Name().c_str());
        break;
      }

      ftemp1 = f1->head;
      ftemp2 = f2->head;
      TY::Ty* typetemp = ftemp1->exp->SemAnalyze(venv, tenv, labelcount);

      if (!typetemp->IsSameType(ftemp2->ty)) {
        errormsg.Error(this->pos, "record type unmatched");
      }

      f1 = f1->tail;
      f2 = f2->tail;
    }

    if (f2 != nullptr) {
      errormsg.Error(this->pos, "Too little efields in %s", this->typ->Name().c_str());
    }
  }

  return temp;
}

TY::Ty *SeqExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* exp = TY::VoidTy::Instance();
  A::ExpList* explist;

  for (explist = this->seq; explist; explist = explist->tail) {
    exp = explist->head->SemAnalyze(venv, tenv, labelcount);
  }

  return exp;
}

TY::Ty *AssignExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::Var* v = this->var;

  if (v->kind == A::Var::SIMPLE) {
    E::EnvEntry* e = venv->Look(static_cast<A::SimpleVar*>(v)->sym);

    if (e->readonly == true) {
      errormsg.Error(this->pos, "loop variable can't be assigned");
    }
  }

  A::Exp* exp = this->exp;
  TY::Ty* variableType = v->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* expressionType = exp->SemAnalyze(venv, tenv, labelcount);

  if (!variableType->IsSameType(expressionType)) {
    errormsg.Error(this->pos, "unmatched assign exp");
  }

  return TY::VoidTy::Instance();
}

TY::Ty *IfExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* test = this->test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* thenn = this->then->SemAnalyze(venv, tenv, labelcount);

  if (this->elsee != nullptr) {
    TY::Ty* elsee = this->elsee->SemAnalyze(venv, tenv, labelcount);

    if (!thenn->IsSameType(elsee)) {
      errormsg.Error(this->pos, "then exp and else exp type mismatch");
    }
  } else if (thenn->kind != TY::Ty::VOID) {
    errormsg.Error(this->pos, "if-then exp's body must produce no value");
  }

  return thenn->ActualTy();
}

TY::Ty *WhileExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* test = this->test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* body = this->body->SemAnalyze(venv, tenv, labelcount);

  if (body->kind != TY::Ty::VOID) {
    errormsg.Error(this->pos, "while body must produce no value");
  }

  return TY::VoidTy::Instance();
}

TY::Ty *ForExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  venv->BeginScope();

  TY::Ty* lo = this->lo->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* hi = this->hi->SemAnalyze(venv, tenv, labelcount);

  if (!lo->IsSameType(TY::IntTy::Instance()) || !hi->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(this->lo->pos, "for exp's range type is not integer");
  }

  E::EnvEntry* e = new E::VarEntry(lo);
  e->readonly = true;
  venv->Enter(this->var, e);
  TY::Ty* body = this->body->SemAnalyze(venv, tenv, labelcount);

  if (body->kind != TY::Ty::VOID) {
    errormsg.Error(this->pos, "for body must produce no value");
  }

  venv->EndScope();

  return TY::VoidTy::Instance();
}

TY::Ty *BreakExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

TY::Ty *LetExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  venv->BeginScope();
  tenv->BeginScope();

  TY::Ty* bodyType;
  A::DecList* declarations;

  for (declarations = this->decs; declarations != nullptr; declarations = declarations->tail) {
    switch (declarations->head->kind) {
      case A::Dec::FUNCTION:
        declarations->head->SemAnalyze(venv, tenv, labelcount);
        break;
      case A::Dec::VAR:
        declarations->head->SemAnalyze(venv, tenv, labelcount);
        break;
      case A::Dec::TYPE:
        declarations->head->SemAnalyze(venv, tenv, labelcount);
        break;
    }
  }

  bodyType = this->body->SemAnalyze(venv, tenv, labelcount);

  venv->EndScope();
  tenv->EndScope();

  return bodyType->ActualTy();
}

TY::Ty *ArrayExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* arrayType = tenv->Look(this->typ)->ActualTy();
  // errormsg.Error(this->pos,"test");
  // TY::Ty* arrayType = tenv->Look(this->typ);

  if (arrayType == nullptr) {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TY::IntTy::Instance();
  }

  if (arrayType->kind != TY::Ty::ARRAY) {
    errormsg.Error(this->pos, "not array type");
  }

  TY::Ty* size = this->size->SemAnalyze(venv, tenv, labelcount);
  TY::Ty* init = this->init->SemAnalyze(venv, tenv, labelcount);

  if (size->kind != TY::Ty::INT) {
    errormsg.Error(this->size->pos, "type of size expression should be int");
  }

  if (!init->IsSameType(static_cast<TY::ArrayTy*>(arrayType)->ty)) {
    errormsg.Error(this->size->pos, "type mismatch");
  }

  return arrayType;
}

TY::Ty *VoidExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::FunDecList* functions = this->functions;
  A::FunDec* temp;

  // push functions name, parameters and return types into tenv, venv
  for (; functions; functions = functions->tail) {
    temp = functions->head;

    if (venv->Look(temp->name)) {
      errormsg.Error(temp->pos, "two functions have the same name");
      continue;
    }

    TY::TyList* tylist = make_formal_tylist(tenv, temp->params);

    if (temp->result) {
      TY::Ty* resultType = tenv->Look(temp->result);

      if (!resultType) {
        errormsg.Error(temp->pos, "undefined return type %s", temp->result->Name());
        continue;
      }

      venv->Enter(temp->name, new E::FunEntry(tylist, resultType));
    } else {
      venv->Enter(temp->name, new E::FunEntry(tylist, TY::VoidTy::Instance()));
    }
  }

  // Check body and given type
  for (functions = this->functions; functions; functions = functions->tail) {
    temp = functions->head;
    TY::TyList* tylist = make_formal_tylist(tenv, temp->params);

    venv->BeginScope();

    A::FieldList* f;
    TY::TyList* t;

    for (f = temp->params, t = tylist; f; f = f->tail, t = t->tail) {
      venv->Enter(f->head->name, new E::VarEntry(t->head));
    }

    TY::Ty* body = temp->body->SemAnalyze(venv, tenv, labelcount);
    E::EnvEntry* function = venv->Look(temp->name);

    if (!body->IsSameType(static_cast<E::FunEntry*>(function)->result)) {
      if (static_cast<E::FunEntry*>(function)->result->ActualTy()->kind == TY::Ty::VOID) {
        errormsg.Error(temp->pos, "procedure returns value");
      }

      // errormsg.Error(temp->pos, "procedure returns unexpected type");
    }

    venv->EndScope();
  }
}

void VarDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* e = this->init->SemAnalyze(venv, tenv, labelcount);
  S::Symbol* s = this->typ;

  if (!s) {
    if (e == TY::NilTy::Instance()) {
      errormsg.Error(this->pos, "init should not be nil without type specified");
    }

    venv->Enter(this->var, new E::VarEntry(e));

    return;
  } else {
    TY::Ty* varType = tenv->Look(s);

    if (varType == nullptr) {
      errormsg.Error(this->pos, "undefined type %s", s->Name().c_str());
      return;
    } else {
      if (e == TY::NilTy::Instance() && varType->ActualTy()->kind != TY::Ty::RECORD) {
        errormsg.Error(this->pos, "init should not be nil without type specified");
      } else if (!varType->IsSameType(e) && e != TY::NilTy::Instance()) {
        errormsg.Error(this->pos, "type mismatch");
      }
    }

    venv->Enter(this->var, new E::VarEntry(e));
  }
}

void TypeDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::NameAndTyList* types = this->types;
  A::NameAndTy* head;

  // push type names to tenv
  while (types) {
    head = types->head;
    
    if (tenv->Look(head->name)) {
      errormsg.Error(this->pos, "two types have the same name");
    } else {
      tenv->Enter(head->name, new TY::NameTy(head->name, nullptr));
    }

    types = types->tail;
  }

  // push actual types
  types = this->types;
  
  while (types) {
    head = types->head;
    TY::Ty* type = tenv->Look(head->name);
    static_cast<TY::NameTy*>(type)->ty = head->ty->SemAnalyze(tenv);
    types = types->tail;
  }

  // find wrong declarations in cycles
  types = this->types;

  while (types) {
    head = types->head;
    TY::Ty* init = tenv->Look(head->name);
    TY::Ty* tempTy = init;
    
    while (tempTy->kind == TY::Ty::NAME) {
      tempTy = static_cast<TY::NameTy*>(tempTy)->ty;

      if (tempTy == init) {
        errormsg.Error(this->pos, "illegal type cycle");
        static_cast<TY::NameTy*>(init)->ty = TY::IntTy::Instance();
        break;
      }
    }

    types = types->tail;
  }
}

TY::Ty *NameTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* nameType = tenv->Look(this->name);

  if (nameType == nullptr) {
    errormsg.Error(this->pos, "undefined type %s", this->name->Name().c_str());
  }

  return new TY::NameTy(this->name, nameType);
}

TY::Ty *RecordTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::FieldList* tempFieldList = new TY::FieldList(nullptr, nullptr);
  TY::FieldList* temp = tempFieldList;
  A::FieldList* record = this->record;

  while (record != nullptr) {
    TY::Ty* typetemp = tenv->Look(record->head->typ);

    if (!typetemp) {
      errormsg.Error(this->pos, "undefined type %s", record->head->typ->Name().c_str());
      typetemp = TY::IntTy::Instance();
    }

    TY::Field* field = new TY::Field(record->head->name, typetemp);
    temp->tail = new TY::FieldList(field, nullptr);
    temp = temp->tail;
    record = record->tail;
  }

  TY::FieldList* header = tempFieldList;
  tempFieldList = tempFieldList->tail;
  free(header);

  return new TY::RecordTy(tempFieldList);
}

TY::Ty *ArrayTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::Ty* arrayType = tenv->Look(this->array);

  if (!arrayType) {
    errormsg.Error(this->pos, "undefined type %s", this->array->Name().c_str());
    return new TY::ArrayTy(nullptr);
    // return arrayType;
  }

  return new TY::ArrayTy(arrayType);
}

}  // namespace A

namespace SEM {
void SemAnalyze(A::Exp *root) {
  if (root) root->SemAnalyze(E::BaseVEnv(), E::BaseTEnv(), 0);
}

}  // namespace SEM
