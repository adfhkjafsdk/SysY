#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cassert>

#include "mir.hpp"

static std::string OperatorIR(Operator op) {
	static std::string IR[] = {
		"??", "??", "??", "mul", "div", "mod", "add", "sub",
		"le", "ge", "lt", "gt", "eq", "ne", "and", "or" };
	return IR[op];
}

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

void ValueToIR(std::ostream &out, ValueInfo *mir) {
	if(mir->tag == VT_UNDEF) out << "undef";
	else if(mir->tag == VT_INT) out << mir->i32;
	else if(mir->tag == VT_SYMBOL) out << *(mir->symbol);
}

void ExprToIR(std::ostream &out, ExprInfo *mir) {
	out << OperatorIR(mir->op) << " ";
	ValueToIR(out, mir->left);
	out << ", ";;
	ValueToIR(out, mir->right);
}

void StmtToIR(std::ostream &out, StmtInfo *mir) {
	switch(mir->tag) {
		case ST_SYMDEF:
			out << "  " << *mir->symdef.name << " = ";
			ExprToIR(out, mir->symdef.expr);
			out << '\n';
			break;
		case ST_RETURN:
			out << "  " << "ret ";
			ValueToIR(out, mir->ret.val);
			out << '\n';
			break;
	}
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
	for(auto func: mir -> funcs) {
		FuncToIR(out, func);
	}
}