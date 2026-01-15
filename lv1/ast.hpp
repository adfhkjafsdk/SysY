#ifndef _SYSY_AST_HPP_
#define _SYSY_AST_HPP_

#include <string>
#include <iostream>
#include <memory>

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	friend std::ostream &operator<< (std::ostream &stream, const BaseAST &ast) {
		ast.Dump (stream);
		return stream;
	}
};

using PtrAST = std::unique_ptr<BaseAST>;
using ASTree = PtrAST;

class CompUnit: public BaseAST {
public:
	PtrAST func_def;
	void Dump(std::ostream &out) const override {
		out << "CompUnit { " << *func_def << " }";
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
};

class FuncType: public BaseAST {
public:
	std::string type;
	void Dump(std::ostream &out) const override {
		out << "FuncType { " << type << " }";
	}
};

class Block: public BaseAST {
public:
	PtrAST stmt;
	void Dump(std::ostream &out) const override {
		out << "Block { " << *stmt << " }";
	}
};

class Stmt: public BaseAST {
public:
	int number;
	void Dump(std::ostream &out) const override {
		out << "Stmt { return " << number << " }";
	}
};

#endif