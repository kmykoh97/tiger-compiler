#include "straightline/slp.h"

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int count1 = this->stm1->MaxArgs();
	int count2 = this->stm2->MaxArgs();

	return count1 > count2 ? count1 : count2;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return this->stm2->Interp(this->stm1->Interp(t));
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return this->exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable* result = this->exp->Interp(t);

	return result->t->Update(this->id, result->i);
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int count1 = this->exps->NumExps();  
	int count2 = this->exps->MaxArgs();

	return count1 > count2 ? count1 : count2;
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return this->exps->Interp(t) -> t;
}

int A::IdExp::MaxArgs() const {
  return 0;
}

IntAndTable *A::IdExp::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return t->makeIntAndTable(t->Lookup(this->id));
}

int A::NumExp::MaxArgs() const {
  return 0;
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return t->makeIntAndTable(this->num);
}

int A::OpExp::MaxArgs() const {
  int count1 = this->left->MaxArgs();
	int count2 = this->right->MaxArgs();

	return count1 > count2 ? count1 : count2;
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable* temp1 = this->left->Interp(t);
  IntAndTable* temp2 = this->right->Interp(temp1->t);

  switch (this->oper) {
    case PLUS:
      return temp2->t->makeIntAndTable(temp1->i + temp2->i);
    case MINUS:
      return temp2->t->makeIntAndTable(temp1->i - temp2->i);
    case TIMES:
      return temp2->t->makeIntAndTable(temp1->i * temp2->i);
    case DIV:
      return temp2->t->makeIntAndTable(temp1->i / temp2->i);
  }
}

int A::EseqExp::MaxArgs() const {
  int count1 = this->stm->MaxArgs();
	int count2 = this->exp->MaxArgs();

	return count1 > count2 ? count1 : count2;
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return this->exp->Interp(this->stm->Interp(t));
}

int A::PairExpList::MaxArgs() const {
  int count1 = this->head->MaxArgs();
	int count2 = this->tail->MaxArgs();

	return count1 > count2 ? count1 : count2;
}

int A::PairExpList::NumExps() const {
  return 1 + this->tail->NumExps();
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable* result = this->head->Interp(t);
  std::cout << result->i << " ";

  return this->tail->Interp(result->t);
}

int A::LastExpList::MaxArgs() const {
  return this->last->MaxArgs();
}

int A::LastExpList::NumExps() const {
  return 1;
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable* result = this->last->Interp(t);
	std::cout << result->i << "\n";

  return result;
}

int Table::Lookup(std::string key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(std::string key, int value) const {
  return new Table(key, value, this);
}

IntAndTable* Table::makeIntAndTable(int i) {
  return new IntAndTable(i, this);
}

}  // namespace A
