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
		"xx", "xx", "slt", "sgt", "xor", "xor", "and", "or" };
	static std::string ASMi[] = {
		"??", "??", "??", "mul", "div", "rem", "addi", "sub",
		"xx", "xx", "slt", "sgt", "xori", "xori", "andi", "ori" };
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
std::string ValueToReg(std::ostream &out, ValueInfo *mir) {
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

std::string SymdefExprToASM(std::ostream &out, ExprInfo *mir) {
	std::string buf;
	bool opCompare = (mir->op == OP_EQ || mir->op == OP_NEQ);
	switch(mir->op) {
		case OP_POS:
		case OP_NEG:
		case OP_LNOT: break;
		case OP_ADD:
		case OP_LAND:
		case OP_LOR:
		case OP_EQ:
		case OP_NEQ:{
			if(mir->right->tag == VT_INT) {
				auto tmp = ValueToReg(out, mir->left);
				if(opCompare && mir->right->i32 == 0) buf = tmp;
				else {
					buf = reg.allocate();
					out << "  " << OperatorASM(mir->op, true) << ' ' << buf << ", " << tmp << ", " << mir->right->i32 << '\n'; 
				}
			}
			else if(mir->left->tag == VT_INT) {
				buf = reg.allocate();
				auto tmp = ValueToReg(out, mir->right);
				if(opCompare && mir->left->i32 == 0) buf = tmp;
				else {
					buf = reg.allocate();
					out << "  " << OperatorASM(mir->op, true) << ' ' << buf << ", " << tmp << ", " << mir->left->i32 << '\n'; 
				}
				
			}
			else {
				auto left = ValueToReg(out, mir->left), right = ValueToReg(out, mir->right);
				buf = left;
				out << "  " << OperatorASM(mir->op, false) << ' ' << buf << ", " << left << ", " << right << '\n';
			}
			std::string oldBuf = buf;
			if(buf == "zero") buf = reg.allocate();
			if(mir->op == OP_EQ) out << "  " << "seqz " << buf << ", " << oldBuf << '\n';
			else if(mir->op == OP_NEQ) out << "  " << "snez " << buf << ", " << oldBuf << '\n';
			break;
		}
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_LT:
		case OP_GT: {
			auto left = ValueToReg(out, mir->left), right = ValueToReg(out, mir->right);
			buf = reg.allocate();
			out << "  " << OperatorASM(mir->op, false) << ' ' << buf << ", " << left << ", " << right << '\n';
			break;
		}
		case OP_LE:
		case OP_GE: {
			auto left = ValueToReg(out, mir->left), right = ValueToReg(out, mir->right);
			buf = reg.allocate();
			out << "  " << (mir->op == OP_LE ? "sgt" : "slt") << ' ' << buf << ", " << left << ", " << right << '\n';
			out << "  " << "seqz " << buf << ", " << buf << '\n';
			break;
		}
	}
	return buf;
}

void StmtToASM(std::ostream &out, StmtInfo *mir) {
	switch(mir->tag) {
		case ST_SYMDEF:{
			auto crt = SymdefExprToASM(out, mir->symdef.expr);
			varReg[* mir->symdef.name] = crt;
			// out << OperatorASM() << " " << varReg[* mir->symdef.name] 
			// 	<< ", "	<<
			break;
		} 
		case ST_RETURN:{
			if(mir->ret.val->tag == VT_SYMBOL) {
				auto val = ValueToReg(out, mir->ret.val);
				out << "  " << "mv a0, " << val << '\n';
			}
			else {
				out << "  " << "li a0, " << mir->ret.val->i32 << '\n';
			}
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