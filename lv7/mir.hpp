#ifndef _SYSY_MIR_HPP_
#define _SYSY_MIR_HPP_

/*
	koopa.h is too "ugly" to use! It is designed for C.
	It's necessary to design a new form suitable for C++.
*/

#include <cstdio>
#include <string>
#include <vector>
#include <cassert>

enum Operator {
	OP_POS, OP_NEG, OP_LNOT,
	OP_MUL, OP_DIV, OP_MOD,
	OP_ADD, OP_SUB,
	OP_LE, OP_GE, OP_LT, OP_GT,
	OP_EQ, OP_NEQ,
	OP_LAND,
	OP_LOR
};

enum TypeTag { TT_INT32, TT_UNIT, TT_ARRAY, TT_POINTER, TT_FUNCTION };
enum ValueTag { VT_SYMBOL, VT_INT, VT_UNDEF };
enum StmtTag { ST_SYMDEF, ST_RETURN, ST_STORE, ST_BR, ST_JUMP };
enum SymbolDefTag { SDT_EXPR, SDT_LOAD, SDT_ALLOC };
enum InitializerTag { IT_UNDEF, IT_NUM, IT_ZERO, IT_AGGR };

template <typename T>
struct List {	// This is similar to std::span in C++20
	T *data;
	std::size_t len;
	void init(std::size_t _l) {
		len = _l;
		if(len) data = new T[len];
		else data = nullptr;
	}
	void destruct() {
		if(!len) return ;
		for(std::size_t i = 0; i < len; ++ i) {
			delete data[i];
		}
		delete[] data;
	}
	using iterator = T*;
	using const_iterator = const T*;
	T &operator[] (std::size_t i) { return data[i]; }
	const T &operator[] (std::size_t i) const { return data[i]; }
	iterator begin() { return data; }
	iterator end() { return data + len; }
	const_iterator cbegin() const { return data; }
	const_iterator cend() const { return data + len; }
};

class MIRInfo{
public:
	virtual ~MIRInfo() = default;
};

struct TypeInfo: public MIRInfo {
	TypeTag tag;
	union {
		struct {
			TypeInfo *base;
			std::size_t len;
		} array;
		struct {
			TypeInfo *base;
		} pointer;
		struct {
			List<TypeInfo*> params;
			TypeInfo *ret;	// If there's no return, this is nullptr.
		} function;
	};
	~TypeInfo() override {
		switch(tag) {
			case TT_INT32:
			case TT_UNIT:
				break;
			case TT_ARRAY:
				delete array.base;
				break;
			case TT_POINTER:
				delete pointer.base;
				break;
			case TT_FUNCTION:
				function.params.destruct();
				if(function.ret != nullptr) delete function.ret;
				break;
		}
	}
};

struct VarInfo: public MIRInfo {
	TypeInfo *type;
	std::string name;
	~ VarInfo() override {
		delete type;
	}
};

struct ValueInfo: public MIRInfo {
	ValueTag tag;
	union {
		std::string *symbol;
		int i32;
	};
	ValueInfo(): tag{VT_UNDEF} {}
	ValueInfo(const std::string &sym): tag{VT_SYMBOL} {
		symbol = new std::string(sym);
	}
	ValueInfo(int val): tag{VT_INT}, i32{val} { }
	~ValueInfo() override {
		if(tag == VT_SYMBOL) delete symbol;
	}
};

struct ExprInfo: public MIRInfo {
	Operator op;
	ValueInfo *left, *right;
	ExprInfo() {}
	ExprInfo(Operator op, ValueInfo *left, ValueInfo *right): op{op}, left{left}, right{right} {}
	~ExprInfo() override {
		delete left;
		delete right;
	}
};

struct InitializerInfo: public MIRInfo {
	InitializerTag tag;
	int num;
	std::vector<InitializerInfo*> aggr;
};

struct StmtInfo: public MIRInfo {
	StmtTag tag;
	union {
		struct {
			SymbolDefTag tag;
			std::string *name;
			union {
				ExprInfo *expr;
				std::string *load;
				TypeInfo *alloc;
			};
		} symdef;
		struct {
			ValueInfo *val;
		} ret;
		struct {
			bool isValue;
			union {
				ValueInfo *val;
				InitializerInfo *init;
			};
			std::string *addr;
		} store;
		struct {
			ValueInfo *cond;
			std::string *blkThen, *blkElse;
		} jump;
	};
	~StmtInfo() override {
		switch(tag) {
			case ST_SYMDEF:
				delete symdef.name;
				switch(symdef.tag) {
					case SDT_EXPR:
						delete symdef.expr;
						break;
					case SDT_LOAD:
						delete symdef.load;
						break;
					case SDT_ALLOC:
						delete symdef.alloc;
						break;
				}
				break;
			case ST_RETURN:
				delete ret.val;
				break;
			case ST_STORE:
				if(store.isValue) delete store.val;
				else delete store.init;
				delete store.addr;
				break;
			case ST_JUMP:
				delete jump.blkThen;
				break;
			case ST_BR:
				delete jump.cond;
				delete jump.blkThen;
				delete jump.blkElse;
				break;
		}
	}
};

struct BlockInfo: public MIRInfo {
	std::string name;
	std::vector<StmtInfo*> stmt;
	bool closed() const {
		if(stmt.empty()) return false;
		auto tag = stmt.back() -> tag;
		return tag == ST_JUMP || tag == ST_BR || tag == ST_RETURN;
	}
	~BlockInfo() override {
		for(auto i: stmt) delete i;
	}
};

struct FuncInfo: public MIRInfo {
	TypeInfo *ret;		// if without return value, this is nullptr
	std::string name;
	std::vector<VarInfo*> params;
	std::vector<BlockInfo*> block;
	~FuncInfo() override {
		if(ret != nullptr) delete ret;
		for(auto i: params) delete i;
		for(auto i: block) delete i;
	}
};

struct ProgramInfo: public MIRInfo {
	List<VarInfo*> vars;
	List<FuncInfo*> funcs;
	~ProgramInfo() override {
		vars.destruct();
		funcs.destruct();
	}
};

#endif