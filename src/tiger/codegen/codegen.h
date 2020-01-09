#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

namespace CG {

// caller save register: rax rdi rsi rdx rcx r8 r9 r10 r11
// callee save register: rbx rbp r12 r13 r14 r15 

AS::InstrList* Codegen(F::Frame* f, T::StmList* stmList);

// helper methods for Codegen
static void emit(AS::Instr* instr);
static void saveCalleeReg();
static void restoreCalleReg();
static void munchStatement(T::Stm* stm);
static TEMP::Temp* munchExpression(T::Exp* exp);
static void munchArguments(T::ExpList* explist);

}
#endif