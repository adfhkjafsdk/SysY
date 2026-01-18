#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cassert>

#include "ast.hpp"

std::string IRType(TypeInfo *mir) {
	switch(mir -> tag) {
		case TT_INT32: return "i32";
		case TT_UNIT: return "unit";
		case TT_ARRAY:
			return "[" + IRType(mir->array.base) + ", " + std::to_string(mir->array.len) +"]";
		case TT_POINTER:
			return "*" + IRType(mir->pointer.base);
		case TT_FUNCTION:
			std::string result = "(";
			for(std::size_t i = 0; i < mir->function.params.len; ++ i) {
				if(i != 0) result += ", ";
				result += IRType(mir -> function.params[i]);
			}
			result += ')';
			if(mir->function.ret != nullptr) {
				result += ": ";
				result += IRType(mir->function.ret);
			}
			return result;
	}
}

void StmtToIR(std::ostream &out, StmtInfo *mir) {
	out << "  " << "ret " << mir->number << '\n';
}

void BlockToIR(std::ostream &out, BlockInfo *mir) {
	out << mir->name << ":\n";
	for(auto stmt: mir->stmt) {
		StmtToIR(out, stmt);
	}
}

void FuncToIR(std::ostream &out, FuncInfo *mir) {
	out << "fun @" << mir->name << "( )";
	if(mir -> ret != nullptr) {
		out << ": " << IRType(mir->ret);
	}
	out << " {\n";
	for(auto block: mir->block) {
		BlockToIR(out, block);
	}
	out << "}\n";
}

void ProgramToIR(std::ostream &out, ProgramInfo *mir) {
	out << "  .text\n";
	for(auto func: mir -> funcs) {
		out << "  .globl " << func->name << '\n';
	}
	for(auto func: mir -> funcs) {
		FuncToIR(out, func);
	}
}