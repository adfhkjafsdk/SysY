#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <map>
#include <cassert>

#include "ast.hpp"

static std::string OperatorASM(Operator op, bool imm) {
	static std::string ASM[] = {
		"??", "??", "??", "mul", "div", "rem", "add", "sub",
		"le", "ge", "lt", "gt", "eq", "ne", "and", "or" };
	static std::string ASMi[] = {
		"??", "??", "??", "mul", "div", "rem", "addi", "sub",
		"le", "ge", "lt", "gt", "eq", "ne", "and", "or" };
	return imm ? ASMi[op] : ASM[op];
}

const std::string AVAILABLE[] = {
	"t3", "t4", "t5", "t6",
	"t0", "t1", "t2",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
};
class Registers {
public:
	int cnt;
	std::string allocate() { 
		return AVAILABLE [ ++ cnt ];
	}
	void clear() { cnt = 0; }
} reg;
static std::map<std::string, std::string> varReg;

// std::string RegOfVar(const std::string &var) {
// 	auto it = varReg.find(var);
// 	if(it == varReg.end()) it = varReg.emplace(var, reg.allocate());
// 	return it->second;
// }
std::string ValueToASM(std::ostream &out, ValueInfo *mir) {
	switch(mir->tag) {
		case VT_INT:
			if(mir->i32 == 0) return "zero";
			else {
				std::string crt = reg.allocate();
				out << "  li " << crt << ", " << mir->i32 << '\n';
				return crt;
			}
		case VT_SYMBOL: {
			auto it = varReg.find(*mir->symbol);
			assert(it != varReg.end());
			return it->second;
		}
		case VT_UNDEF:
			return "zero";
	}
}

void ExprToASM(std::ostream &out, ExprInfo *mir, const std::string &buf) {
	switch(mir->op) {
		case OP_POS:
		case OP_NEG:
		case OP_LNOT: break;
		case OP_ADD:
			// if(mir->right->tag == VT_INT) {
			// 	auto tmp = ValueToASM(out, mir->left);
			// 	out << "addi " << buf << ", " << tmp << ", " << mir->right->i32 << '\n'; 
			// }
			// else if(mir->left->tag == VT_INT) {
			// 	auto tmp = ValueToASM(out, mir->right);
			// 	out << "addi " << buf << ", " << tmp << ", " << mir->left->i32 << '\n'; 
			// }
			// else {
			// 	auto left = ValueToASM(out, mir->left), right = ValueToASM(mir->right);
			// 	out << "add " << buf << ", " << left << ", " << right << '\n';
			// }
			// break;
		case OP_SUB:
			break;
	}
}

void StmtToASM(std::ostream &out, StmtInfo *mir) {
	switch(mir->tag) {
		case ST_SYMDEF:{
			auto crt = reg.allocate();
			varReg[* mir->symdef.name] = crt;
			
			// std::string res = ExprToASM(out, mir->symdef.expr);
			// out << OperatorASM() << " " << varReg[* mir->symdef.name] 
			// 	<< ", "	<<
			break;
		} 
		case ST_RETURN:{
			auto val = ValueToASM(out, mir->ret.val);
			out << "  " << "li a0, " << val << '\n';
			out << "  " << "ret\n";
			break;
		}
	}
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