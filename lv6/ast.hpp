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

struct MIRRet {
	MIRInfo *mir;
	bool isImm;
	std::string res;
	int imm;
	MIRRet(MIRInfo *mir = nullptr): mir{mir}, isImm{true}, res{""}, imm{0} {}
	MIRRet(MIRInfo *mir, const std::string &res): mir{mir}, isImm{false}, res{res}, imm{0} {}
	MIRRet(MIRInfo *mir, int imm): mir{mir}, isImm{true}, res{""}, imm{imm} {}
};

static int cntTmp = 0;
static int cntBlock = 0;

static std::string GetTmp() {
	++ cntTmp;
	// std::cerr << "GetTmp " << "%" + std::to_string(cntTmp) << '\n';
	return "%" + std::to_string(cntTmp);
}

static std::string NewBlock() {
	++ cntBlock;
	std::cerr << "NewBlock " << cntBlock << '\n';
	return "%block" + std::to_string(cntBlock);
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

// enum ASTTag {
	// AST_CUNIT, AST_FUNCDEF, AST_FUNCTYPE, AST_
// };

enum ASTStmtTag {
	AST_ST_VARDEF, AST_ST_CONSTDEF, AST_ST_EXPR, AST_ST_ASSIGN, AST_ST_BLOCK, 
	AST_ST_IF, AST_ST_ELSE, AST_ST_RETURN
};

struct TypeVal {	// unused
	TypeInfo type;
	int val;
};

struct DomainManager {
	// std::size_t depth;		// current block depth
	std::vector< std::map<std::string, std::size_t > > recVar;
	std::vector< std::map<std::string, int > > recConst;
	std::map<std::string, std::size_t> cnt;
	DomainManager() {
		recVar.emplace_back();
		recConst.emplace_back();
	}
	std::string newVar(const std::string &name) {
		std::cerr << "newVar " << name << '\n'; 
		std::size_t id = cnt[name];
		recVar.back().emplace(name, id);
		++ cnt[name];
		return "@" + name + "_" + std::to_string(id);
	}
	void newConst(const std::string &name, int imm) {
		recConst.back().emplace(name, imm);
	}
	MIRRet find(const std::string &name) {
		for(int i = (int)recVar.size() - 1; i >= 0; -- i) {
			auto it = recVar[i].find(name);
			if(it != recVar[i].end()) 
				return MIRRet(nullptr, "@" + name + "_" + std::to_string(it->second));
			auto it2 = recConst[i].find(name);
			if(it2 != recConst[i].end())
				return MIRRet(nullptr, it2->second);
		}
		assert(0);
		return MIRRet();
		// return newName(name);
	}
	void push() { 
		recVar.emplace_back();
		recConst.emplace_back();
	}
	void pop() {
		recVar.pop_back();
		recConst.pop_back();
	}
};

static DomainManager domainMgr;

// static std::map<std::string, int> constVals;

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
	// virtual ASTTag Type() const = 0;
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
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override;
};

class FuncDef: public BaseAST {
public:
	PtrAST func_type;
	std::string ident;
	PtrAST block;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override;
};


class FuncType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override;
};

class BType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override;
};

class Block: public BaseAST {
public:
	std::vector<PtrAST> stmt;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override;
};

class Expr: public BaseAST {
public:
	Operator op;
	PtrAST left, right;
	Expr() {}
	Expr(Operator op, BaseAST* &&left, BaseAST* &&right): op{op}, left{left}, right{right} {}
	void Dump(std::ostream &out) const override;
	int Calc() const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override;
};

class LVal: public BaseAST {
public:
	std::string ident;
	void Dump(std::ostream &out) const override;
	int Calc() const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override;
};


class Stmt: public BaseAST {
public:
	ASTStmtTag tag;
	PtrAST detail, next;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *) const override;
};

class StmtIf: public BaseAST {
public:
	PtrAST expr, stmt;
	PtrAST match;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *) const override;
	bool tryMatch(Stmt *stmtElse);
};

class StmtVarDef: public BaseAST {
public:
	std::string name;
	SharedAST type;
	PtrAST expr, next;	// if uninitialized, expr is nullptr
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override;
};

class StmtConstDef: public BaseAST {
public:
	std::string name;
	SharedAST type;
	PtrAST expr, next;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *) const override;
};

class StmtAssign: public BaseAST {
public:
	PtrAST lval, expr;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override;
};

class StmtReturn: public BaseAST {
public:
	PtrAST expr;
	void Dump(std::ostream &out) const override;
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override;
};

class Number: public BaseAST {
public:
	int val;
	void Dump(std::ostream &out) const override;
	int Calc() const override;
	MIRRet DumpMIR(std::vector<MIRInfo*>*) const override;
};

using ASTree = std::unique_ptr<CompUnit>;



#endif