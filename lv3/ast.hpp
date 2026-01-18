#ifndef _SYSY_AST_HPP_
#define _SYSY_AST_HPP_

#include <string>
#include <iostream>
#include <memory>
#include <utility>

#include "sysy_exceptions.hpp"
#include "mir.hpp"

using std::operator""s;

static int cntTmp = 0;

static std::string GetTmp() {
	++cntTmp;
	std::cerr << "GetTmp " << "%" + std::to_string(cntTmp) << '\n';
	return "%" + std::to_string(cntTmp);
}

static bool IsUnaryOperator(Operator op) {
	return op == OP_POS || op == OP_NEG || op == OP_LNOT;
}
static std::string OperatorStr(Operator op) {
	static std::string STR[] = {
		"\'+\'", "\'-\'", "!", "*", "/", "%", "+", "-",
		"<=", ">=", "<", ">", "==", "!=", "&&", "||" };
	return STR[op];
}

struct MIRRet {
	MIRInfo *mir;
	bool isImm;
	std::string res;
	int imm;
	MIRRet(MIRInfo *mir = nullptr): mir{mir}, isImm{true}, res{""}, imm{0} {}
	MIRRet(MIRInfo *mir, const std::string &res): mir{mir}, isImm{false}, res{res}, imm{0} {}
	MIRRet(MIRInfo *mir, int imm): mir{mir}, isImm{true}, res{""}, imm{imm} {}
};

static auto genValue = [](const MIRRet &src) {
	std::cerr << "will genValue {" << std::boolalpha << src.isImm <<", " << src.res << ", " << src.imm << " }\n";
	if(src.isImm) return new ValueInfo(src.imm);
	else return new ValueInfo(src.res);
};

/*
  ## On the DumpMIR function
  
  If the AST Node may generate multiple lines of MIR, use DumpMIR(&buf);
  otherwise, use DumpMIR(nullptr).mir .
*/

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	virtual MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const = 0;
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
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override {
		auto tmp = new ProgramInfo;
		tmp -> vars .init(0);
		tmp -> funcs .init(1);
		tmp -> funcs[0] = dynamic_cast<FuncInfo*>(func_def -> DumpMIR(nullptr).mir);
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
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override {
		auto tmp = new FuncInfo;
		tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR(nullptr).mir);
		tmp -> name = ident;
		tmp -> params .init(0);
		tmp -> block .init(1);
		tmp -> block[0] = dynamic_cast<BlockInfo*>(block -> DumpMIR(nullptr).mir);
		return tmp; 
	}
};

class FuncType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override {
		out << "FuncType { " << type << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override {
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
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override {
		std::vector<MIRInfo*> statements;
		auto tmp = new BlockInfo;
		tmp -> name = "%entry";
		stmt -> DumpMIR(&statements);
		tmp -> stmt .init(statements.size());
		for(std::size_t i = 0; i < statements.size(); ++ i)
			tmp -> stmt[i] = dynamic_cast<StmtInfo*>(statements[i]);
		return tmp;
	}
};

class Expr: public BaseAST {
public:
	Operator op;
	PtrAST left, right;
	Expr() {}
	Expr(Operator op, BaseAST* &&left, BaseAST* &&right): op{op}, left{left}, right{right} {}
	void Dump(std::ostream &out) const override {
		if(IsUnaryOperator(op)) out << OperatorStr(op) << " { " << *left << " }";
		else out << OperatorStr(op) << " { " << *left << ", " << *right << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		if(op == OP_POS) return left->DumpMIR(buf);
		auto tmp = new ExprInfo;
		auto crt = GetTmp();
		if(IsUnaryOperator(op)) {
			auto mirLeft = left -> DumpMIR(buf);
			// if(op == OP_POS) {
			// 	tmp->op = OP_ADD;
			// 	tmp->left = new ValueInfo(0);
			// 	tmp->right = genValue(mirLeft);
			// }
			// else
			if(op == OP_NEG) {
				tmp->op = OP_SUB;
				tmp->left = new ValueInfo(0);
				tmp->right = genValue(mirLeft);
			}
			else if(op == OP_LNOT) {
				tmp->op = OP_EQ;
				tmp->left = new ValueInfo(0);
				tmp->right = genValue(mirLeft);
			}
		}
		else {
			// auto genToBool
			auto mirLeft = left->DumpMIR(buf), mirRight = right -> DumpMIR(buf);
			tmp->op = op;
			if(op == OP_LAND) {
				auto tmpLeft = new StmtInfo;
				tmpLeft->tag = ST_SYMDEF;
				tmpLeft->symdef.name = new std::string(GetTmp());
				tmpLeft->symdef.expr = new ExprInfo(OP_NEQ, genValue(mirLeft), new ValueInfo(0));
				buf->emplace_back(tmpLeft);
				
				auto tmpRight = new StmtInfo;
				tmpRight->tag = ST_SYMDEF;
				tmpRight->symdef.name = new std::string(GetTmp());
				tmpRight->symdef.expr = new ExprInfo(OP_NEQ, genValue(mirRight), new ValueInfo(0));
				buf->emplace_back(tmpRight);

				tmp->left = new ValueInfo(* tmpLeft->symdef.name);
				tmp->right = new ValueInfo(* tmpRight->symdef.name);
			}
			else {
				tmp->left = genValue(mirLeft);
				tmp->right = genValue(mirRight);
			}
		}
		auto symd = new StmtInfo;
		symd -> tag = ST_SYMDEF;
		symd -> symdef.name = new std::string(crt);
		symd -> symdef.expr = tmp;
		buf -> emplace_back(symd);
		if(op == OP_LOR) {
			auto toBool = new StmtInfo;
			toBool->tag = ST_SYMDEF;
			toBool->symdef.name = new std::string(GetTmp());
			toBool->symdef.expr = new ExprInfo(OP_NEQ, new ValueInfo(crt), new ValueInfo(0));
			buf->emplace_back(toBool);
			crt = *toBool->symdef.name;
		}
		return MIRRet(nullptr, crt);
	}
};

class Stmt: public BaseAST {
public:
	PtrAST exp;
	void Dump(std::ostream &out) const override {
		out << "Stmt { return " << *exp << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		auto tmp = new StmtInfo;
		tmp -> tag = ST_RETURN;
		tmp -> ret.val = genValue(exp->DumpMIR(buf));
		buf -> emplace_back(tmp);
		return MIRRet();
	}
};

class Number: public BaseAST {
public:
	int val;
	void Dump(std::ostream &out) const override {
		out << val ;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override {
		return MIRRet(nullptr, val);
	}
};

using ASTree = std::unique_ptr<CompUnit>;

#endif