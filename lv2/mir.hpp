#ifndef _SYSY_MIR_HPP_
#define _SYSY_MIR_HPP_

/*
	koopa.h is too "ugly" to use! It is designed for C.
	It's necessary to design a new form suitable for C++.
*/

#include <cstdio>
#include <string>
#include <vector>

enum TypeTag {
	TT_INT32,
	TT_UNIT,
	TT_ARRAY,
	TT_POINTER,
	TT_FUNCTION
};

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

struct StmtInfo: public MIRInfo {
	int number;
	void clean() override {}
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