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

std::string GetTmp() {
	++cntTmp;
	return "%" + std::to_string(cntTmp);
}

bool IsUnaryOperator(Operator op) {
	return op == OP_POS || op == OP_NEG || op == OP_LNOT;
}
std::string OperatorStr(Operator op) {
	static std::string STR[] = {
		"\'+\'", "\'-\'", "!", "*", "/", "%", "+", "-",
		"<=", ">=", "<", ">", "==", "!=", "&&", "||" };
	return STR[op];
}
std::string OperatorIR(Operator op) {
	static std::string IR[] = {
		"??", "??", "??", "mul", "div", "mod", "add", "sub",
		"le", "ge", "lt", "gt", "eq", "ne", "and", "or" };
	return IR[op];
}

struct MIRRet {
	MIRInfo *mir;
	std::string res;
	MIRRet(MIRInfo *mir, const std::string &res = ""): mir{mir}, res{res} {}
};

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	virtual MIRRet DumpMIR() const = 0;
	friend std::ostream &operator<< (std::ostream &stream, const BaseAST &ast) {
		ast.Dump (stream);
		return stream;
	}
};

using PtrAST = std::unique_ptr<BaseAST>;

class CompUnit: public BaseAST {
public:
	PtrAST func_def;
	void Dump(std::ostream &out) const override {
		out << "CompUnit { " << *func_def << " }";
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
	MIRRet DumpMIR() const override {
		auto tmp = new BlockInfo;
		tmp -> name = "%entry";
		tmp -> stmt .init(1);
		tmp -> stmt[0] = dynamic_cast<StmtInfo*>(stmt -> DumpMIR().mir);
		return tmp; 
	}
};

class Expr: public BaseAST {
public:
	Operator op;
	PtrAST left, right;
	void Dump(std::ostream &out) const override {
		if(IsUnaryOperator(op)) out << OperatorStr(op) << " { " << left << " }";
		else out << OperatorStr(op) << " { " << left << ", " << right << " }";
	}
	MIRRet DumpMIRTo(std::vector<ExprInfo*> &buf) const {
		// if(op == OP_POS) return left->DumpMIR();
		auto tmp = new ExprInfo;
		auto crt = GetTmp();
		if(IsUnaryOperator(op)) {
			auto mirLeft = left -> DumpMIRTo(buf);
			if(op == OP_POS) {
				tmp->op = OP_ADD;
				tmp->left = new ValueInfo(0);
				tmp->right = new ValueInfo(mirLeft.res);
			}
			else if(op == OP_NEG) {
				tmp->op = OP_SUB;
				tmp->left = new ValueInfo(0);
				tmp->right = new ValueInfo(mirLeft.res);
			}
			else if(op == OP_LNOT) {
				tmp->op = OP_SUB;
				tmp->left = new ValueInfo(1);
				tmp->right = new ValueInfo(mirLeft.res);
			}
		}
		else {
			auto mirLeft = left->DumpMIRTo(buf), mirRight = right -> DumpMIRTo(buf);
			tmp->op = op;
			tmp->left = new ValueInfo(mirLeft.res);
			tmp->right = new ValueInfo(mirRight.res);
		}
		buf.emplace_back(tmp);
		return {tmp, crt};
	}
	MIRRet DumpMIR() const override {
		assert(0);
		return {nullptr, ""};
	}
};

class Stmt: public BaseAST {
public:
	PtrAST exp;
	void Dump(std::ostream &out) const override {
		out << "Stmt { return " << *exp << " }";
	}
	MIRRet DumpMIRTo(std::vector<)
	MIRRet DumpMIR() const override {
		auto tmp = new StmtInfo;
		tmp -> tag = ST_RETURN;
		return tmp;
	}
};

using ASTree = std::unique_ptr<CompUnit>;

#endif