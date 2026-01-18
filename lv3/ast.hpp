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
	bool isImm;
	union{
		std::string *res;
		int imm;
	};
	MIRRet(MIRInfo *mir, const std::string &res = ""): mir{mir} {
		
	}
};

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	virtual MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const = 0;
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
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		auto tmp = new ProgramInfo;
		tmp -> vars .init(0);
		tmp -> funcs .init(1);
		tmp -> funcs[0] = dynamic_cast<FuncInfo*>(func_def -> DumpMIR(buf).mir);
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
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		auto tmp = new FuncInfo;
		tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR(buf).mir);
		tmp -> name = ident;
		tmp -> params .init(0);
		tmp -> block .init(1);
		tmp -> block[0] = dynamic_cast<BlockInfo*>(block -> DumpMIR(buf).mir);
		return tmp; 
	}
};

class FuncType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override {
		out << "FuncType { " << type << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		auto tmp = new TypeInfo;
		if(type == "int") {
			tmp -> tag = TT_INT32;
		}
		buf.emplace_back(tmp);
		return tmp; 
	}
};

class Block: public BaseAST {
public:
	PtrAST stmt;
	void Dump(std::ostream &out) const override {
		out << "Block { " << *stmt << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		auto tmp = new BlockInfo;
		tmp -> name = "%entry";
		tmp -> stmt .init(1);
		tmp -> stmt[0] = dynamic_cast<StmtInfo*>(stmt -> DumpMIR(buf).mir);
		return tmp; 
	}
};

class Expr: public BaseAST {
public:
	Operator op;
	PtrAST left, right;
	void Dump(std::ostream &out) const override {
		if(IsUnaryOperator(op)) out << OperatorStr(op) << " { " << *left << " }";
		else out << OperatorStr(op) << " { " << *left << ", " << *right << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		// if(op == OP_POS) return left->DumpMIR();
		auto tmp = new ExprInfo;
		auto crt = GetTmp();
		if(IsUnaryOperator(op)) {
			auto mirLeft = left -> DumpMIR(buf);
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
			auto mirLeft = left->DumpMIR(buf), mirRight = right -> DumpMIR(buf);
			tmp->op = op;
			tmp->left = new ValueInfo(mirLeft.res);
			tmp->right = new ValueInfo(mirRight.res);
		}
		auto symd = new StmtInfo;
		symd -> tag = ST_SYMDEF;
		symd -> symdef.name = new std::string(crt);
		symd -> symdef.expr = tmp;
		buf.emplace_back(symd);
		return {nullptr, crt};
	}
};

class Stmt: public BaseAST {
public:
	PtrAST exp;
	void Dump(std::ostream &out) const override {
		out << "Stmt { return " << *exp << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		auto tmp = new StmtInfo;
		tmp -> tag = ST_RETURN;
		tmp -> ret.val = new ValueInfo(exp->DumpMIR(buf).res);
		buf.emplace_back(tmp);
		return tmp;
	}
};

class Number: public BaseAST {
public:
	int val;
	void Dump(std::ostream &out) const override {
		out << val ;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> &buf) const override {
		auto tmp = new StmtInfo;
		tmp -> tag = ST_SYMDEF;
		tmp -> symdef.expr = 
		return {nullptr, val};
	}
}

using ASTree = std::unique_ptr<CompUnit>;

#endif