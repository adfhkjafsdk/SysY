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
			switch(mir->symdef.tag) {
				case SDT_EXPR:
					ExprToIR(out, mir->symdef.expr);
					out << '\n';
					break;
				case SDT_LOAD:
					out << "load " << *mir->symdef.load << '\n';
					break;
				case SDT_ALLOC:
					out << "alloc " << IRType(mir->symdef.alloc) << '\n';
					break;
				case SDT_FUNCALL: {
					out << "call " << *mir->symdef.func.fun << "(";
					auto &params = *mir->symdef.func.para;
					for(std::size_t i = 0; i < params.size(); ++ i) {
						if(i > 0) out << ", ";
						ValueToIR(out, params[i]);
					}
					out << ")\n";
					break;
				}
			}
			break;
		case ST_RETURN:
			out << "  " << "ret ";
			if(mir->ret.val != nullptr) ValueToIR(out, mir->ret.val);
			out << '\n';
			break;
		case ST_STORE:
			out << "  " << "store ";
			if(mir->store.isValue) ValueToIR(out, mir->store.val);
			else assert(0);
			out << ", " << *mir->store.addr << '\n';
			break;
		case ST_BR:
			out << "  " << "br ";
			ValueToIR(out, mir->jump.cond);
			out << ", " << *mir->jump.blkThen << ", " << *mir->jump.blkElse << '\n';
			break;
		case ST_JUMP:
			out << "  " << "jump " << *mir->jump.blkThen << '\n';
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
	out << "fun @" << mir->name << "(";
	for(std::size_t i = 0; i < mir->params.size(); ++ i) {
		if(i > 0) out << ", ";
		out << mir->params[i]->name << ": " << IRType(mir->params[i]->type);
	}
	out << ")";
	if(mir -> ret != nullptr && mir->ret->tag != TT_UNIT) {
		out << ": " << IRType(mir->ret);
	}
	out << " {\n";
	std::cerr << "block size = " << mir->block.size() << '\n';
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