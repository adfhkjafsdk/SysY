#ifndef _SYSY_AST_HPP_
#define _SYSY_AST_HPP_

#include <string>
#include <iostream>
#include <memory>

#include "sysy_exceptions.hpp"

using std::operator""s;

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	virtual void DumpIR(std::ostream&) const = 0;
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
	void DumpIR(std::ostream &out) const override {
		out << IR(func_def) ;
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
	void DumpIR(std::ostream &out) const override {
		out << "fun @" << ident << "(): " << IR(func_type) << " {\n"
		<< IR(block)
		<< "}\n" ;
	}
};

class FuncType: public BaseAST {
	public:
	std::string type;
	void Dump(std::ostream &out) const override {
		out << "FuncType { " << type << " }";
	}
	void DumpIR(std::ostream &out) const override {
		if(type == "int") out << "i32";
		else throw syntax_error("Unknown Type \""s + type + "\"");
	}
};

class Block: public BaseAST {
public:
	PtrAST stmt;
	void Dump(std::ostream &out) const override {
		out << "Block { " << *stmt << " }";
	}
	void DumpIR(std::ostream &out) const override {
		out << "%entry:\n"
			<< "  " << IR(stmt) << '\n';
	}
};

class Stmt: public BaseAST {
public:
	int number;
	void Dump(std::ostream &out) const override {
		out << "Stmt { return " << number << " }";
	}
	void DumpIR(std::ostream &out) const override {
		out << "ret " << number ;
	}
};

using ASTree = std::unique_ptr<CompUnit>;

#endif