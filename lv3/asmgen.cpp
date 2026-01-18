#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cassert>

#include "ast.hpp"

void ExprToASM(std::ostream &out, ExprInfo *mir) {
	
}

void StmtToASM(std::ostream &out, StmtInfo *mir) {
	// out << "  " << "li a0, " << mir-> << '\n';
	// out << "  " << "ret\n";
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