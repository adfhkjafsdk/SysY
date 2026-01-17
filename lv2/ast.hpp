#ifndef _SYSY_AST_HPP_
#define _SYSY_AST_HPP_

#include <string>
#include <iostream>
#include <memory>

#include "sysy_exceptions.hpp"
#include "mir.hpp"

using std::operator""s;

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void Dump(std::ostream&) const = 0;
	virtual void DumpIR(std::ostream&) const = 0;
	virtual MIRInfo *DumpMIR() const = 0;
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
	MIRInfo *DumpMIR() const override {
		auto tmp = new ProgramInfo;
		tmp -> vars .init(0);
		tmp -> funcs .init(1);
		tmp -> funcs[0] = dynamic_cast<FuncInfo*>(func_def -> DumpMIR());
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
	void DumpIR(std::ostream &out) const override {
		out << "fun @" << ident << "(): " << IR(func_type) << " {\n"
		<< IR(block)
		<< "}\n" ;
	}
	MIRInfo *DumpMIR() const override {
		auto tmp = new FuncInfo;
		tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR());
		tmp -> name = ident;
		tmp -> params .init(0);
		tmp -> block .init(1);
		tmp -> block[0] = dynamic_cast<BlockInfo*>(block -> DumpMIR());
		return tmp; 
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
	MIRInfo *DumpMIR() const override {
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
	void DumpIR(std::ostream &out) const override {
		out << "%entry:\n"
			<< "  " << IR(stmt) << '\n';
	}
	MIRInfo *DumpMIR() const override {
		auto tmp = new BlockInfo;
		tmp -> name = "%entry";
		tmp -> stmt .init(1);
		tmp -> stmt[0] = dynamic_cast<StmtInfo*>(stmt -> DumpMIR());
		return tmp; 
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
	MIRInfo *DumpMIR() const override {
		auto tmp = new StmtInfo;
		tmp -> number = number;
		return tmp;
	}
};

using ASTree = std::unique_ptr<CompUnit>;

#endif