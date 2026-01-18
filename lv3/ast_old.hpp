#ifndef _SYSY_AST_HPP_
#define _SYSY_AST_HPP_

#include <string>
#include <iostream>
#include <memory>
#include <utility>

#include "sysy_exceptions.hpp"
#include "mir.hpp"

using std::operator""s;

int cntTmp = 0;

enum ASTOperator {
	OP_POS, OP_NEG, OP_LNOT,
	OP_MUL, OP_DIV, OP_MOD,
	OP_ADD, OP_SUB,
	OP_LE, OP_GE, OP_LT, OP_GT,
	OP_EQ, OP_NEQ,
	OP_LAND,
	OP_LOR
};

std::string GetTmp() {
	++cntTmp;
	return "%" + std::to_string(cntTmp);
}

bool IsUnaryOperator(ASTOperator op) {
	return op == OP_POS || op == OP_NEG || op == OP_LNOT;
}
std::string OperatorStr(ASTOperator op) {
	static std::string STR[] = {
		"\'+\'", "\'-\'", "!", "*", "/", "%", "+", "-",
		"<=", ">=", "<", ">", "==", "!=", "&&", "||" };
	return STR[op];
}
std::string OperatorIR(ASTOperator op) {
	static std::string IR[] = {
		"??", "??", "??", "mul", "div", "mod", "add", "sub",
		"le", "ge", "lt", "gt", "eq", "ne", "and", "or" };
	return IR[op];
}

struct MIRRet {
	MIRInfo *mir;
	int ext;
	MIRRet(MIRInfo *mir, int ext = 0): mir{mir}, ext{ext} {}
};

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	virtual int DumpIR(std::ostream&) const = 0;
	virtual MIRRet DumpMIR() const = 0;
	friend std::ostream &operator<< (std::ostream &stream, const BaseAST &ast) {
		ast.Dump (stream);
		return stream;
	}
};

using PtrAST = std::unique_ptr<BaseAST>;
class IR {
public:
	const PtrAST *data;
	IR(const PtrAST &ast) : data{&ast} {}
	friend std::ostream &operator<< (std::ostream &stream, const IR &ir) {
		(*ir.data) -> DumpIR(stream);
		return stream;
	}
};

class CompUnit: public BaseAST {
public:
	PtrAST func_def;
	void Dump(std::ostream &out) const override {
		out << "CompUnit { " << *func_def << " }";
	}
	int DumpIR(std::ostream &out) const override {
		out << IR(func_def) ;
		return 0;
	}
	MIRRet DumpMIR() const override {
		auto tmp = new ProgramInfo;
		tmp -> vars .init(0);
		tmp -> funcs .init(1);
		tmp -> funcs[0] = dynamic_cast<FuncInfo*>(func_def -> DumpMIR().mir);
		return tmp;
	}
};

class FuncDef: public BaseAST {
public:
	PtrAST func_type;
	std::string ident;
	PtrAST block;
	void Dump(std::ostream &out) const override {
		out << "FuncDef { " << *func_type << ", " << ident << ", " << *block << " }";
	}
	int DumpIR(std::ostream &out) const override {
		out << "fun @" << ident << "(): " << IR(func_type) << " {\n"
		<< IR(block)
		<< "}\n" ;
		return 0;
	}
	MIRRet DumpMIR() const override {
		auto tmp = new FuncInfo;
		tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR().mir);
		tmp -> name = ident;
		tmp -> params .init(0);
		tmp -> block .init(1);
		tmp -> block[0] = dynamic_cast<BlockInfo*>(block -> DumpMIR().mir);
		return tmp; 
	}
};

class FuncType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override {
		out << "FuncType { " << type << " }";
	}
	int DumpIR(std::ostream &out) const override {
		if(type == "int") out << "i32";
		else throw syntax_error("Unknown Type \""s + type + "\"");
		return 0;
	}
	MIRRet DumpMIR() const override {
		auto tmp = new TypeInfo;
		if(type == "int") {
			tmp -> tag = TT_INT32;
		}
		return tmp; 
	}
};

class Block: public BaseAST {
public:
	PtrAST stmt;
	void Dump(std::ostream &out) const override {
		out << "Block { " << *stmt << " }";
	}
	int DumpIR(std::ostream &out) const override {
		out << "%entry:\n"
			<< "  " << IR(stmt) << '\n';
		return 0;	
	}
	MIRRet DumpMIR() const override {
		auto tmp = new BlockInfo;
		tmp -> name = "%entry";
		tmp -> stmt .init(1);
		tmp -> stmt[0] = dynamic_cast<StmtInfo*>(stmt -> DumpMIR().mir);
		return tmp; 
	}
};

class Stmt: public BaseAST {
public:
	int number;
	void Dump(std::ostream &out) const override {
		out << "Stmt { return " << number << " }";
	}
	int DumpIR(std::ostream &out) const override {
		out << "ret " << number ;
		return 0;
	}
	MIRRet DumpMIR() const override {
		auto tmp = new StmtInfo;
		tmp -> number = number;
		return tmp;
	}
};

class Expr: public BaseAST {
public:
	ASTOperator op;
	PtrAST left, right;
	void Dump(std::ostream &out) const override {
		if(IsUnaryOperator(op)) out << OperatorStr(op) << " { " << left << " }";
		else out << OperatorStr(op) << " { " << left << ", " << right << " }";
	}
	int DumpIR(std::ostream &out) const override {
		if(IsUnaryOperator(op)) {
			int vLeft = left -> DumpIR(out);
			if(op == OP_POS) return vLeft;
			else {
				int crt = GetTmp();
				if(op == OP_NEG) out << "%t" << crt << " = sub 0, " << "%t" << vLeft << '\n';
				else if(op == OP_LNOT) out << "%t" << crt << " = sub 1, " << "%t" << vLeft << '\n';
				return crt;
			}
		}
		else {
			int crt = GetTmp(), vLeft = left->DumpIR(out), vRight = right -> DumpIR(out);
			out << "%t" << crt << " = " << OperatorIR(op) << ' ' << "%t" << vLeft << ", %t" << vRight <<'\n';
			return crt;
		}
		return 0;
	}
	MIRRet DumpMIR() const override {
		
	}
};

using ASTree = std::unique_ptr<CompUnit>;

#endif