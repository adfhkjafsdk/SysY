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
	return "block" + std::to_string(cntBlock);
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
		std::cerr << "newName " << name << '\n'; 
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
		auto blkEntry = new BlockInfo;
		blkEntry -> name = NewBlock();
		blkEntry -> stmt = std::vector<StmtInfo*>();
		// blkEntry -> stmt = std::vector<StmtInfo*>(1, new StmtInfo);
		// blkEntry -> stmt[0] -> tag = ST_JUMP;
		// blkEntry -> stmt[0] -> jump.blkThen = entry;
		
		std::vector<MIRInfo*> buf(1, blkEntry);
		block -> DumpMIR(&buf);

		auto tmp = new FuncInfo;
		tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR(nullptr).mir);
		tmp -> name = ident;
		tmp -> params = std::vector<VarInfo*>();
		tmp -> block = std::vector<BlockInfo*>(1, blkEntry);
		tmp -> block.resize(buf.size() + 1);
		for(std::size_t i = 0; i < buf.size(); ++ i)
			tmp -> block[i + 1] = dynamic_cast<BlockInfo*>(buf[i]);
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
	bool isNew;
	PtrAST stmt;
	void Dump(std::ostream &out) const override {
		out << "Block { " ;
		if(stmt != nullptr) out << *stmt;
		out << " }";
	}
	// void DumpBlocks(std::vector<MIRInfo*> *buf) const override {
	// }
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {		// Dump Block to vector<BlockInfo*>
		// This requires a BlockInfo created before calling.

		assert(buf != nullptr);
		assert(!buf -> empty());
		std::string ret = "";

		// If the block is created inside an empty parent block, just inherit it.
		// if( buf->empty() || ! dynamic_cast<BlockInfo*>(buf->at(0))->stmt.empty() ) {
		// if(buf->empty()) {
		// 	auto tmp = new BlockInfo;
		// 	tmp -> name = NewBlock();
		// 	tmp -> stmt = std::vector<StmtInfo*>();
		// 	buf -> emplace_back(tmp);
		// 	ret = tmp->name;
		// }

		domainMgr.push();
		if(stmt != nullptr) {
			stmt -> DumpMIR(buf);
		}
		domainMgr.pop();
		return MIRRet(nullptr, ret);
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
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {		// Dump Expr to vector<StmtInfo*>
		if(op == OP_POS) return left->DumpMIR(buf);
		auto tmp = new ExprInfo;
		auto crt = GetTmp();
		if(IsUnaryOperator(op)) {
			auto mirLeft = left -> DumpMIR(buf);
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
		auto found = domainMgr.find(ident);
		assert(found.isImm);
		return found.imm;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {		// Dump LVal to vector<StmtInfo*>
		auto found = domainMgr.find(ident);
		if(found.isImm) return found;
		else {
			auto tmp = new StmtInfo;
			tmp->tag = ST_SYMDEF;
			tmp->symdef.tag = SDT_LOAD;
			tmp->symdef.name = new std::string(GetTmp());
			tmp->symdef.load = new std::string(found.res);
			buf -> emplace_back(tmp);
			return MIRRet(nullptr, *tmp->symdef.name);
		}
	}
};



class StmtIf: public BaseAST {
public:
	PtrAST expr, stmt;
	void Dump(std::ostream &out) const override {
		out << "If { " << *expr << ", " << *stmt << " }"; 
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *) const override {
		assert(0);
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
		auto varName = domainMgr.newVar(name);
		auto tmp = new StmtInfo;
		tmp->tag = ST_SYMDEF;
		tmp->symdef.tag = SDT_ALLOC;
		tmp->symdef.name = new std::string(varName);
		tmp->symdef.alloc = dynamic_cast<TypeInfo*>(type -> DumpMIR(nullptr).mir);
		buf -> emplace_back(tmp);
		
		if(expr != nullptr){
			auto res = expr->DumpMIR(buf);
			tmp = new StmtInfo;
			tmp->tag = ST_STORE;
			tmp->store.isValue = true;
			tmp->store.val = genValue(res);
			tmp->store.addr = new std::string(varName);
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
		domainMgr.newConst(name, expr -> Calc());
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
		assert( ! domainMgr.find(dynamic_cast<LVal*>(lval.get()) -> ident).isImm );
		tmp -> tag = ST_STORE;
		tmp -> store.isValue = true;
		tmp -> store.val = genValue(res);
		tmp -> store.addr = new std::string(domainMgr.find(dynamic_cast<LVal*>(lval.get()) -> ident).res);
		buf -> emplace_back(tmp);
		return MIRRet();
	}
};

class StmtReturn: public BaseAST {
public:
	PtrAST expr;
	void Dump(std::ostream &out) const override {
		if(expr != nullptr) out << "StmtReturn { " << *expr << " }";
		else out << "StmtReturn {}";
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {
		auto tmp = new StmtInfo;
		tmp -> tag = ST_RETURN;
		tmp -> ret.val = genValue(expr->DumpMIR(buf));
		buf -> emplace_back(tmp);
		return MIRRet();
	}
};

class Stmt: public BaseAST {
public:
	ASTStmtTag tag;
	PtrAST detail, next;
	// PtrAST exp;
	void Dump(std::ostream &out) const override {
		// out << "Stmt { return " << *exp << " }";
		out << "Stmt";
		if(tag == AST_ST_ELSE) out << " (ELSE)";
		if(detail != nullptr)
			out << " { " << *detail << " }";
		else
			out << " {}";
		if(next != nullptr) out << ", " << *next;
	}
	MIRRet DumpMIR(std::vector<MIRInfo*> *buf) const override {		// Dump Stmt to vector<BlockInfo*>
		BaseAST *realNext = next.get();
		auto crtBlock = dynamic_cast<BlockInfo*>(buf->back());
		std::vector<MIRInfo*> stmtTmp;
		if(detail != nullptr) {
			switch(tag) {
				case AST_ST_ASSIGN:
				case AST_ST_CONSTDEF:
				case AST_ST_VARDEF:
				case AST_ST_EXPR: {
					detail->DumpMIR(&stmtTmp);
					crtBlock->stmt.reserve(crtBlock->stmt.size() + stmtTmp.size());
					for(auto s: stmtTmp)
						crtBlock->stmt.emplace_back(dynamic_cast<StmtInfo*>(s));
					break;
				}
				case AST_ST_RETURN: {
					detail->DumpMIR(&stmtTmp);
					crtBlock->stmt.reserve(crtBlock->stmt.size() + stmtTmp.size());
					for(auto s: stmtTmp)
						crtBlock->stmt.emplace_back(dynamic_cast<StmtInfo*>(s));
					if(realNext != nullptr) {
						std::cerr << "[Warning] Line(s) after return are ignored.\n";
						realNext = nullptr;
					}
					break;
				}
				case AST_ST_BLOCK:
					detail->DumpMIR(buf);
					break;
				case AST_ST_IF: {
					// std::size_t crt = buf.size() - 1;
					StmtIf *realDetail = dynamic_cast<StmtIf*>(detail.get());
					auto cond = realDetail -> expr -> DumpMIR(&stmtTmp).res;
					crtBlock->stmt.reserve(crtBlock->stmt.size() + stmtTmp.size());
					for(auto s: stmtTmp)
						crtBlock->stmt.emplace_back(dynamic_cast<StmtInfo*>(s));

					auto stmtBr = new StmtInfo;
					stmtBr -> tag = ST_BR;
					stmtBr -> jump.cond = new std::string(cond);

					BlockInfo *blkThen = new BlockInfo, *blkElse = nullptr;
					blkThen->name = NewBlock();
					blkThen->stmt = std::vector<StmtInfo*>();
					buf->emplace_back(blkThen);

					domainMgr.push();
					realDetail -> stmt -> DumpMIR(buf);
					domainMgr.pop();
					stmtBr -> jump.blkThen = new std::string(blkThen->name);
					stmtBr -> jump.blkElse = nullptr;

					if(next != nullptr && dynamic_cast<Stmt*>(realNext)->tag == AST_ST_ELSE) {
						blkElse = new BlockInfo;
						blkElse->name = NewBlock();
						blkElse->stmt = std::vector<StmtInfo*>();
						buf->emplace_back(blkElse);
						
						domainMgr.push();
						dynamic_cast<Stmt*>(realNext) -> detail -> DumpMIR(buf);
						domainMgr.pop();
						stmtBr -> jump.blkElse = new std::string(blkElse->name);
						realNext = dynamic_cast<Stmt*>(realNext) -> next.get();
					}	// TODO: test this!
					crtBlock -> stmt.emplace_back(stmtBr);

					// Create a new block for realNext, and add jump-stmt to blkThen and blkElse // DONE
					auto blkNext = new BlockInfo;
					blkNext->name = NewBlock();
					blkNext->stmt = std::vector<StmtInfo*>();
					auto genJump = [&]() {
						auto stmt = new StmtInfo;
						stmt->tag = ST_JUMP;
						stmt->jump.blkThen = new std::string(blkNext->name);
						return stmt;
					};
					blkThen -> stmt.emplace_back(genJump());
					if(blkElse != nullptr) blkElse -> stmt.emplace_back(genJump());
					break;

				}
				case AST_ST_ELSE: {
					std::cerr << "\'Else\' without an \'if\'.\n";
					assert(0);
					break;
				}
			}
		}
		if(realNext != nullptr) realNext->DumpMIR(buf);
		return MIRRet();		// This should return the entry Block name of these statements.	// No, there may be not an entire block
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