#ifndef _SYSY_MIR_HPP_
#define _SYSY_MIR_HPP_

/*
	koopa.h is too "ugly" to use! It is designed for C.
	It's necessary to design a new form suitable for C++.
*/

#include <cstdio>
#include <string>
#include <vector>

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
enum StmtTag { ST_SYMDEF, ST_RETURN };
enum SymbolDefTag { SDT_NUM, SDT_EXPR, SDT_}

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
			data[i] -> clean();
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
	virtual void clean() = 0;
	// virtual void dumpIR(std::ostream&) const = 0;
	// virtual void dumpASM(std::ostream&) const = 0;
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
	void clean() override {
		switch(tag) {
			case TT_INT32:
			case TT_UNIT:
				break;
			case TT_ARRAY:
				array.base -> clean();
				break;
			case TT_POINTER:
				pointer.base -> clean();
				break;
			case TT_FUNCTION:
				function.params.destruct();
				if(function.ret != nullptr) function.ret -> clean();
				break;
		}
	}
};

struct VarInfo: public MIRInfo {
	TypeInfo *type;
	std::string name;
	void clean() override {
		type -> clean();
	}
};

struct ValueInfo: public MIRInfo {
	ValueTag tag;
	union {
		std::string *symbol;
		int i32;
	};
	void clean() override {
		if(tag == VT_SYMBOL) delete symbol;
	}
	ValueInfo(): tag{VT_UNDEF} {}
	ValueInfo(const std::string &sym): tag{VT_SYMBOL} {
		symbol = new std::string(sym);
	}
	ValueInfo(int val): tag{VT_INT}, i32{val} {}
};

struct ExprInfo: public MIRInfo {
	Operator op;
	ValueInfo *left, *right;
	void clean() override {
		left -> clean();
		right -> clean();
	}
};

struct SymbolDefInfo: public MIRInfo {
	bool global;
	std::string name;
	ExprInfo *expr;
	void clean() override {
		expr -> clean();
	}
};

struct StmtInfo: public MIRInfo {
	StmtTag tag;
	union {
		struct {
			std::string *name;
			ExprInfo *expr;
		} symdef;
		struct {
			ValueInfo *val;
		} ret;
	};
	void clean() override {
		switch(tag) {
			case ST_SYMDEF:
				delete symdef.name;
				symdef.expr -> clean();
				break;
			case ST_RETURN:
				ret.val -> clean();
		}
	}
};

struct BlockInfo: public MIRInfo {
	std::string name;
	List<StmtInfo*> stmt;
	void clean() override {
		stmt.destruct();
	}
};

struct FuncInfo: public MIRInfo {
	TypeInfo *ret;		// if no return value, this is nullptr
	std::string name;
	List<VarInfo*> params;
	List<BlockInfo*> block;
	void clean() override {
		if(ret != nullptr) ret -> clean();
		params.destruct();
		block.destruct();
	}
};

struct ProgramInfo: public MIRInfo {
	List<VarInfo*> vars;
	List<FuncInfo*> funcs;
	void clean() override {
		for(auto x : vars) x -> clean();
		for(auto f : funcs) f -> clean();
	}
};

#endif