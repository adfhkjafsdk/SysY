#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <set>
#include <map>
#include <queue>
#include <cassert>

#include "ast.hpp"

extern void StmtToIR(std::ostream &out, StmtInfo *mir) ;

static constexpr int I12_MIN = -2048, I12_MAX = 2047;
static constexpr std::size_t PTR_SIZE = 4;

static std::string OperatorASM(Operator op, bool imm) {
	static std::string ASM[] = {
		"??", "??", "??", "mul", "div", "rem", "add", "sub",
		"xx", "xx", "slt", "sgt", "xor", "xor", "and", "or" };
	static std::string ASMi[] = {
		"??", "??", "??", "mul", "div", "rem", "addi", "sub",
		"xx", "xx", "slt", "sgt", "xori", "xori", "andi", "ori" };
	return imm ? ASMi[op] : ASM[op];
}

const std::set<std::string> AVAILABLE = {
	"t3", "t4", "t5", "t6",
	"t0", "t1", "t2",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
};
class RegisterManager {
public:
	std::priority_queue<std::string> q;		// t6~t0, a7~a0
	RegisterManager() {
		for(const auto &name: AVAILABLE) q.emplace(name);
	}
	std::string allocate() {
		auto ret = q.top();
		q.pop();
		return ret;
	}
	void free(const std::string &str) {
		if(AVAILABLE.find(str) != AVAILABLE.end())
			q.emplace(str);
	} 
	void clear() {
		for(const auto &name: AVAILABLE) q.emplace(name);
	}
} regMgr;

class StackManager {
public:
	std::size_t size;
	std::map<std::string, std::size_t> stackAddr;
	std::size_t getAddr(const std::string &ident) const {
		auto it = stackAddr.find(ident);
		assert(it != stackAddr.end());
		return it -> second;
	}
	void clear() {
		size = 0;
		stackAddr.clear();
	}
} stackMgr;

static std::string crtFuncName;

static std::map<std::string, std::string> varReg;

static bool isImm12(int val) {
	return I12_MIN <= val && val <= I12_MAX;
}

std::string BlockId(const std::string &str) {
	assert(str.substr(0,6) == "%block");
	return crtFuncName + "_b" + str.substr(6);
}

std::size_t SizeOfType(const TypeInfo *type) {
	switch(type->tag) {
		case TT_INT32: return 4;
		case TT_UNIT: return 0;
		case TT_ARRAY: return type->array.len * SizeOfType(type->array.base);
		case TT_POINTER: return PTR_SIZE;
		case TT_FUNCTION: return PTR_SIZE;
	}
}

std::string ValueToReg(std::ostream &out, ValueInfo *mir, std::string target = "") {
	std::string crt = target;
	switch(mir->tag) {
		case VT_INT:
			if(mir->i32 == 0 && crt.empty()) return "zero";
			else {
				if(crt.empty()) crt = regMgr.allocate();
				out << "  li " << crt << ", " << mir->i32 << '\n';
				return crt;
			}
		case VT_SYMBOL: {
			auto name = *mir->symbol;
			auto it = varReg.find(name);
			if(it != varReg.end()) return it->second;
			else {
				if(crt.empty()) crt = regMgr.allocate();
				if(name[0] == '%') out << "  lw " << crt << ", " << stackMgr.getAddr(name) << "(sp)\n";
				else if(name.substr(0, 2) == "@p" && name.substr(name.size() - 5) == "_para") {
					// Parameter initializer
					std::size_t id = (std::size_t) std::strtol(name.substr(2, name.size() - 7).c_str(), nullptr, 10);
					if(id < 8u)
						out << "  mv " << crt << ", " << "a" << id << '\n';
					else {
						// TODO: support long address
						out << "  lw " << crt << ", " << stackMgr.size + (id - 8u) * 4u << "(sp)\n";
					}
				}
				else {
					out << "  li " << crt << ", " << stackMgr.getAddr(name) << '\n';
					// TOFIX: this should be the real address
				}
				return crt;
			}
		}
		case VT_UNDEF: {
			if(crt.empty()) return "zero";
			else {
				out << "  li " << crt << ", 0\n";
				return crt;
			}
		}
	}
}

std::string SymdefExprToASM(std::ostream &out, ExprInfo *mir) {
	std::string buf;
	bool opCompare = (mir->op == OP_EQ || mir->op == OP_NEQ);
	switch(mir->op) {
		case OP_POS:
		case OP_NEG:
		case OP_LNOT: break;
		case OP_ADD:
		case OP_LAND:
		case OP_LOR:
		case OP_EQ:
		case OP_NEQ:{
			if(mir->right->tag == VT_INT && isImm12(mir->right->i32)) { 
				auto tmp = ValueToReg(out, mir->left);
				if(opCompare && mir->right->i32 == 0) buf = tmp;
				else {
					buf = regMgr.allocate();
					out << "  " << OperatorASM(mir->op, true) << ' ' << buf << ", " << tmp << ", " << mir->right->i32 << '\n'; 
					regMgr.free(tmp);
				}
			}
			else if(mir->left->tag == VT_INT && isImm12(mir->left->i32)) {
				auto tmp = ValueToReg(out, mir->right);
				if(opCompare && mir->left->i32 == 0) buf = tmp;
				else {
					buf = regMgr.allocate();
					out << "  " << OperatorASM(mir->op, true) << ' ' << buf << ", " << tmp << ", " << mir->left->i32 << '\n'; 
					regMgr.free(tmp);
				}
			}
			else {
				auto left = ValueToReg(out, mir->left), right = ValueToReg(out, mir->right);
				buf = left;
				out << "  " << OperatorASM(mir->op, false) << ' ' << buf << ", " << left << ", " << right << '\n';
				regMgr.free(right);
			}
			std::string oldBuf = buf;
			if(buf == "zero") buf = regMgr.allocate();
			if(mir->op == OP_EQ) out << "  " << "seqz " << buf << ", " << oldBuf << '\n';
			else if(mir->op == OP_NEQ) out << "  " << "snez " << buf << ", " << oldBuf << '\n';
			if(buf != oldBuf) regMgr.free(oldBuf);
			break;
		}
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_LT:
		case OP_GT: {
			auto left = ValueToReg(out, mir->left), right = ValueToReg(out, mir->right);
			buf = regMgr.allocate();
			out << "  " << OperatorASM(mir->op, false) << ' ' << buf << ", " << left << ", " << right << '\n';
			regMgr.free(left);
			regMgr.free(right);
			break;
		}
		case OP_LE:
		case OP_GE: {
			auto left = ValueToReg(out, mir->left), right = ValueToReg(out, mir->right);
			buf = regMgr.allocate();
			out << "  " << (mir->op == OP_LE ? "sgt" : "slt") << ' ' << buf << ", " << left << ", " << right << '\n';
			out << "  " << "seqz " << buf << ", " << buf << '\n';
			regMgr.free(left);
			regMgr.free(right);
			break;
		}
	}
	return buf;
}

void StmtToASM(std::ostream &out, StmtInfo *mir) {
	switch(mir->tag) {
		case ST_SYMDEF: {
			std::size_t addr = stackMgr.getAddr(* mir->symdef.name);
			switch(mir->symdef.tag){
				case SDT_EXPR: {
					auto crt = SymdefExprToASM(out, mir->symdef.expr);
					// varReg[*mir->symdef.name] = crt;
					out << "  sw " << crt << ", " << addr << "(sp)\n";
					regMgr.free(crt);
					break;
				}
				case SDT_LOAD: {
					auto buf = regMgr.allocate();
					auto addrSrc = stackMgr.getAddr(* mir->symdef.load);
					out << "  lw " << buf << ", " << addrSrc << "(sp)\n";
					out << "  sw " << buf << ", " << addr << "(sp)\n";
					regMgr.free(buf);
					break;
				}
				case SDT_ALLOC:
					out << "  #  " << "value of " << *mir->symdef.name << " is " << addr <<'\n';
					break;
				case SDT_FUNCALL: {
					auto &params = *mir->symdef.func.para;
					for(std::size_t i = 0; i < params.size(); ++ i) {
						if(i < 8u) ValueToReg(out, params[i], "a" + std::to_string(i));
						else {
							auto buf = ValueToReg(out, params[i]);
							out << "  sw " << buf << ", " << (i-8u)*4u << "(sp)\n";
						}
					}
					out << "  call " << mir->symdef.func.fun -> substr(1) << '\n';
					out << "  sw " << "a0" << ", " << addr << "(sp)\n";
					break;
				}
			}
			break;
		}
		case ST_STORE: {
			assert(mir -> store.isValue);
			std::size_t addr = stackMgr.getAddr(* mir->store.addr);
			std::string src = ValueToReg(out, mir->store.val);
			out << "  sw " << src << ", " << addr << "(sp)\n";
			// TODO: support long address on every occurance of 'lw' and 'sw'
			regMgr.free(src);
			break;
		}
		case ST_RETURN: {
			if(mir->ret.val != nullptr){
				if(mir->ret.val->tag == VT_SYMBOL) {
					auto val = ValueToReg(out, mir->ret.val);
					out << "  " << "mv a0, " << val << '\n';
					regMgr.free(val);
				}
				else {
					out << "  " << "li a0, " << mir->ret.val->i32 << '\n';
				}
			}
			out << "  " << "j " << crtFuncName << "_epilogue\n";
			// 'ret' should be after the epilogue, so output it in FuncToASM, instead of here
			break;
		}
		case ST_BR: {
			auto cond = ValueToReg(out, mir->jump.cond);
			out << "  " << "bnez " << cond << ", " 
						<< BlockId(*mir->jump.blkThen) << "\n";
			out << "  " << "j " << BlockId(*mir->jump.blkElse) << '\n';
			regMgr.free(cond);
			break;
		}
		case ST_JUMP: {
			out << "  " << "j " << BlockId(*mir->jump.blkThen) << '\n';
			break;
		}
	}
}

void BlockToASM(std::ostream &out, BlockInfo *mir) {
	out << BlockId(mir->name) << ":\n";
	for(auto stmt: mir->stmt) {
		out << "  #";
		StmtToIR(out, stmt);
		StmtToASM(out, stmt);
		out << '\n';
	}
}

void FuncToASM(std::ostream &out, FuncInfo *mir) {
	std::size_t stackSize = 0, maxParam = 0;
	bool isLeaf = false;
	stackMgr.clear();
	for(auto block: mir->block) 
		for(auto stmt: block->stmt) 
			if(stmt->tag == ST_SYMDEF && stmt->symdef.tag == SDT_FUNCALL) {
				isLeaf = false;
				maxParam = std::max(maxParam, stmt->symdef.func.para -> size() );
			}
	if(maxParam > 8)
		stackSize += 4u * (maxParam - 8);

	for(auto block: mir->block) 
		for(auto stmt: block->stmt) 
			if(stmt->tag == ST_SYMDEF && stmt->symdef.tag != SDT_ALLOC) {
				// Stack for results of instruction
				stackMgr.stackAddr[* stmt->symdef.name] = stackSize;
				stackSize += 4;
			}
	for(auto block: mir->block)
		for(auto stmt: block->stmt)
			if(stmt->tag == ST_SYMDEF && stmt->symdef.tag == SDT_ALLOC) {
				// Stack for alloc
				stackMgr.stackAddr[* stmt->symdef.name] = stackSize;
				stackSize += SizeOfType(stmt->symdef.alloc);
			}
	if(!isLeaf) {
		stackMgr.stackAddr["_ra"] = stackSize;
		stackSize += 4;		// for storing ra
	}
	stackSize = ((stackSize + 15) >> 4) << 4;	// aligning
	stackMgr.size = stackSize;

	crtFuncName = mir->name;
	out << "  .text\n";
	out << "  .globl " << crtFuncName << '\n';
	out << mir->name << ":\n";
	out << "  # prologue of " << mir->name << '\n';
	if(isImm12(-int(stackSize))) {
		out << "  addi sp, sp, " << -int(stackSize) << '\n';
	}
	else {
		out << "  li a0, " << -int(stackSize) << '\n';
		out << "  add sp, sp, a0\n";
	}
	if(!isLeaf) out << "  sw ra, " << stackMgr.stackAddr["_ra"] << "(sp)\n";
	out << '\n';
	// out << "  # body of " << mir->name << '\n';
	for(auto block: mir->block) {
		BlockToASM(out, block);
	}
	out << mir->name << "_epilogue:\n";
	out << "  # epilogue of " << mir->name << '\n';
	if(!isLeaf) out << "  lw ra, " << stackMgr.stackAddr["_ra"] << "(sp)\n";
	if(isImm12(int(stackSize))) {
		out << "  addi sp, sp, " << int(stackSize) << '\n';
	}
	else {
		auto tmp = regMgr.allocate();
		out << "  li " << tmp << ", " << int(stackSize) << '\n';
		out << "  add sp, sp, " << tmp << "\n";
		regMgr.free(tmp);
	}
	out << "  " << "ret\n";
	out << '\n';
}

void ProgramToASM(std::ostream &out, ProgramInfo *mir) {
	for(auto func: mir -> funcs) {
		FuncToASM(out, func);
	}
}