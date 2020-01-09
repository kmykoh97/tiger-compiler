#ifndef TIGER_ESCAPE_ESCAPE_H_
#define TIGER_ESCAPE_ESCAPE_H_

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/symbol/symbol.h"

namespace ESC {

void FindEscape(A::Exp* exp);
}  // namespace ESC

#endif