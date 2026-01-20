#ifndef _SYSY_AST_HPP_
#define _SYSY_AST_HPP_

#include <string>
#include <iostream>
#include <memory>
#include <utility>
#include <map>
#include <cassert>

#include "sysy_exceptions.hpp"
#include "mir.hpp"

using std::operator""s;

static int cntTmp = 0;

static std::string GetTmp() {
	++cntTmp;
	// std::cerr << "GetTmp " << "%" + std::to_string(cntTmp) << '\n';
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

enum ASTStmtTag {
	AST_ST_VARDEF, AST_ST_CONSTDEF, AST_ST_ASSIGN, AST_ST_RETURN
};

struct TypeVal {	// unused
	TypeInfo type;
	int val;
};

static std::map<std::string, int> constVals;

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
	// std::cerr << "will genValue {" << std::boolalpha << src.isImm <<", " << src.res << ", " << src.imm << " }\n";
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
	virtual int Calc() const { return 0; }	// if the node is an expr, return the result
	friend std::ostream &operator<< (std::ostream &stream, const BaseAST &ast) {
		ast.Dump (stream);
		return stream;
	}
};

using PtrAST = std::unique_ptr<BaseAST>;
using SharedAST = std::shared_ptr<BaseAST>;

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

class BType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override {
		out << "BType { " << type << " }";
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
		if(stmt != nullptr) {
			stmt -> DumpMIR(&statements);
		}
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
	int Calc() const override {
		switch(op) {
			case OP_POS:  return  left->Calc();
			case OP_NEG:  return -left->Calc();
			case OP_LNOT: return !left->Calc();
			case OP_MUL:  return left->Calc() *  right->Calc();
			case OP_DIV:  return left->Calc() /  right->Calc();
			case OP_MOD:  return left->Calc() %  right->Calc();
			case OP_ADD:  return left->Calc() +  right->Calc();
			case OP_SUB:  return left->Calc() -  right->Calc();
			case OP_LE:   return left->Calc() <= right->Calc();
			case OP_GE:   return left->Calc() >= right->Calc();
			case OP_LT:   return left->Calc() <  right->Calc();
			case OP_GT:   return left->Calc() >  right->Calc();
			case OP_EQ:   return left->Calc() == right->Calc();
			case OP_NEQ:  return left->Calc() != right->Calc();
			case OP_LAND: return left->Calc() && right->Calc();
			case OP_LOR:  return left->Calc() || right->Calc();
		}
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
				tmpLeft->symdef.tag = SDT_EXPR;
				tmpLeft->symdef.name = new std::string(GetTmp());
				tmpLeft->symdef.expr = new ExprInfo(OP_NEQ, genValue(mirLeft), new ValueInfo(0));
				buf->emplace_back(tmpLeft);
				
				auto tmpRight = new StmtInfo;
				tmpRight->tag = ST_SYMDEF;
				tmpRight->symdef.tag = SDT_EXPR;
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
		symd -> symdef.tag = SDT_EXPR;
		symd -> symdef.name = new std::string(crt);
		symd -> symdef.expr = tmp;
		buf -> emplace_back(symd);
		if(op == OP_LOR) {
			auto toBool = new StmtInfo;
			toBool->tag = ST_SYMDEF;
			toBool->symdef.tag = SDT_EXPR;
			toBool->symdef.name = new std::string(GetTmp());
			toBool->symdef.expr = new ExprInfo(OP_NEQ, new ValueInfo(crt), new ValueInfo(0));
			buf->emplace_back(toBool);
			crt = *toBool->symdef.name;
		}
		return MIRRet(nullptr, crt);
	}
};

class LVal: public BaseAST {
public:
	std::string ident;
	void Dump(std::ostream &out) const override {
		out << "LVal { " << ident << " }";
	}
	int Calc() const override {
		assert(constVals.find(ident) != constVals.end());
		// std::cerr << "Calc const " << ident << " = " << constVals[ident] << '\n';
		return constVals[ident];
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		auto it = constVals.find(ident);
		if(it != constVals.end())
			return MIRRet(nullptr, it->second);
		else {
			// assert(0);
			auto tmp = new StmtInfo;
			tmp->tag = ST_SYMDEF;
			tmp->symdef.tag = SDT_LOAD;
			tmp->symdef.name = new std::string(GetTmp());
			tmp->symdef.load = new std::string("@" + ident);
			buf -> emplace_back(tmp);
			return MIRRet(nullptr, *tmp->symdef.name);
		}
	}
};

class Stmt: public BaseAST {
public:
	ASTStmtTag tag;
	PtrAST detail, next;
	// PtrAST exp;
	void Dump(std::ostream &out) const override {
		// out << "Stmt { return " << *exp << " }";
		out << "Stmt { " << *detail << " }";
		if(next != nullptr) out << ", " << *next;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		detail->DumpMIR(buf);
		if(next) next->DumpMIR(buf);
		return MIRRet();
	}
};

class StmtVarDef: public BaseAST {
public:
	std::string name;
	SharedAST type;
	PtrAST expr, next;	// if uninitialized, expr is nullptr
	void Dump(std::ostream &out) const override {
		if(expr == nullptr) out << "StmtVarDef { " << *type << ", " << name << ", " << "[NOTHING]" << " }";
		else out << "StmtVarDef { " << *type << ", " << name << ", " << *expr << " }";
		if(next != nullptr) out << ", " << *next;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		// TODO: Check if the type are matched. If not, report the error.
		auto tmp = new StmtInfo;
		tmp->tag = ST_SYMDEF;
		tmp->symdef.tag = SDT_ALLOC;
		tmp->symdef.name = new std::string("@" + name);
		tmp->symdef.alloc = dynamic_cast<TypeInfo*>(type -> DumpMIR(nullptr).mir);
		buf -> emplace_back(tmp);
		
		if(expr != nullptr){
			auto res = expr->DumpMIR(buf);
			tmp = new StmtInfo;
			tmp->tag = ST_STORE;
			tmp->store.isValue = true;
			tmp->store.val = genValue(res);
			tmp->store.addr = new std::string("@" + name);
			buf -> emplace_back(tmp);
		}

		if(next) next->DumpMIR(buf);
		return MIRRet();
	}
};

class StmtConstDef: public BaseAST {
public:
	std::string name;
	SharedAST type;
	PtrAST expr, next;
	void Dump(std::ostream &out) const override {
		out << "StmtConstDef { " << *type << ", " << name << ", " << *expr << " }";
		if(next != nullptr) out << ", " << *next;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *) const override {
		constVals[name] = expr -> Calc();
		// TODO: check if type is matched with the result of expr
		if(next != nullptr) next->DumpMIR(nullptr);
		return MIRRet();
	}
};

class StmtAssign: public BaseAST {
public:
	PtrAST lval, expr;
	void Dump(std::ostream &out) const override {
		out << "StmtAssign { " << *lval << ", " << *expr << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		auto res = expr->DumpMIR(buf);
		auto tmp = new StmtInfo;
		tmp -> tag = ST_STORE;
		tmp -> store.isValue = true;
		tmp -> store.val = genValue(res);
		tmp -> store.addr = new std::string("@" + dynamic_cast<LVal*>(lval.get()) -> ident);
		buf -> emplace_back(tmp);
		return MIRRet();
	}
};

class StmtReturn: public BaseAST {
public:
	PtrAST expr;
	void Dump(std::ostream &out) const override {
		out << "StmtReturn { " << *expr << " }";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		auto tmp = new StmtInfo;
		tmp -> tag = ST_RETURN;
		tmp -> ret.val = genValue(expr->DumpMIR(buf));
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
	int Calc() const override { return val; }
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override {
		return MIRRet(nullptr, val);
	}
};

using ASTree = std::unique_ptr<CompUnit>;

#endif