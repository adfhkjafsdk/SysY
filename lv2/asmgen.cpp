#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cassert>

#include "ast.hpp"

std::string ASMType(TypeInfo *mir) {
	switch(mir -> tag) {
		case TT_INT32: return "i32";
		case TT_UNIT: return "unit";
		case TT_ARRAY:
			return "[" + ASMType(mir->array.base) + ", " + std::to_string(mir->array.len) +"]";
		case TT_POINTER:
			return "*" + ASMType(mir->pointer.base);
		case TT_FUNCTION:
			std::string result = "(";
			for(std::size_t i = 0; i < mir->function.params.len; ++ i) {
				if(i != 0) result += ", ";
				result += ASMType(mir -> function.params[i]);
			}
			result += ')';
			if(mir->function.ret != nullptr) {
				result += ": ";
				result += ASMType(mir->function.ret);
			}
			return result;
	}
}

void StmtToASM(std::ostream &out, StmtInfo *mir) {
	out << "  " << "li a0, " << mir->number << '\n';
	out << "  " << "ret\n";
}

void BlockToASM(std::ostream &out, BlockInfo *mir) {
	for(auto stmt: mir->stmt) {
		StmtToASM(out, stmt);
	}
}

void FuncToASM(std::ostream &out, FuncInfo *mir) {
	out << mir->name << ":\n";
	for(auto block: mir->block) {
		BlockToASM(out, block);
	}
}

void ProgramToASM(std::ostream &out, ProgramInfo *mir) {
	out << "  .text\n";
	for(auto func: mir -> funcs) {
		out << "  .globl " << func->name << '\n';
	}
	for(auto func: mir -> funcs) {
		FuncToASM(out, func);
	}
}