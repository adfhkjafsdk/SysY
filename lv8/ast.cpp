#include "ast.hpp"


struct DomainManager {
	struct LayerInfo {
		std::map<std::string, std::string> recVar;
		std::map<std::string, int > recConst;
		std::string blkBreak, blkContinue;
	};
	std::vector<LayerInfo> rec;
	std::map<std::string, std::size_t> cnt;
	std::set<std::string> used;
	DomainManager() {
		rec.emplace_back();
	}
	std::string newVar(const std::string &name) {
		std::cerr << "newVar " << name << '\n'; 
		std::string res = "";
		if(rec.size() == 1u) {
			res = "@" + name;
		}
		else {
			while(true) {
				++ cnt[name];
				res = "@" + name + "_" + std::to_string(cnt[name]);
				if(used.find(res) == used.end()) break;
			}
		}
		used.emplace(res);
		rec.back().recVar.emplace(name, res);
		return res;
	}
	void newConst(const std::string &name, int imm) {
		rec.back().recConst.emplace(name, imm);
	}
	MIRRet find(const std::string &name) {
		std::cerr << "find " << name << '\n';
		for(int i = (int)rec.size() - 1; i >= 0; -- i) {
			auto it = rec[i].recVar.find(name);
			if(it != rec[i].recVar.end()) 
				return MIRRet(nullptr, it->second);
			auto it2 = rec[i].recConst.find(name);
			if(it2 != rec[i].recConst.end())
				return MIRRet(nullptr, it2->second);
		}
		assert(0);
		return MIRRet();
		// return newName(name);
	}
	std::string GetBreak() {
		std::cerr << "GetBreak()  " << rec.back().blkBreak << '\n';
		assert(! rec.back().blkBreak.empty());
		return rec.back().blkBreak;
	}
	std::string GetContinue() {
		std::cerr << "GetContinue()  " << rec.back().blkContinue << '\n';
		assert(! rec.back().blkContinue.empty());
		return rec.back().blkContinue;
	}
	void push() {
		assert(!rec.empty());
		rec.emplace_back();
		rec.back().blkBreak = rec[rec.size()-2].blkBreak;
		rec.back().blkContinue = rec[rec.size()-2].blkContinue;
	}
	void pushWhile(const std::string &blkBreak, const std::string &blkContinue) {
		rec.emplace_back();
		rec.back().blkBreak = blkBreak;
		rec.back().blkContinue = blkContinue;
	}
	void pop() {
		rec.pop_back();
	}
};

static DomainManager domainMgr;

static std::map<std::string, FuncDef*> funcMgr;

std::vector<StmtVarDef*> globVarsToInit;

static BlockInfo *NewBlockInfo() {
	auto block = new BlockInfo;
	block -> name = NewBlock();
	block -> stmt = std::vector<StmtInfo*>();
	return block;
}
static BlockInfo *GetLastBlock(std::vector<MIRInfo*> *buf) {
	assert(! buf->empty());
	return dynamic_cast<BlockInfo*>(buf->back());
}
static StmtInfo *GenJump(const std::string &name){
	auto stmtJump = new StmtInfo;
	stmtJump->tag = ST_JUMP;
	stmtJump->jump.blkThen = new std::string(name);
	return stmtJump;
}

template <typename T>
static std::size_t LinkedSize(const T *obj) {
	std::size_t ret = 0;
	while(obj != nullptr) {
		++ ret;
		obj = dynamic_cast<T*>(obj->next.get());
	}
	return ret;
}

void CompUnit::Dump(std::ostream &out) const {
	out << "CompUnit { ";
	for(std::size_t i = 0; i < glob_def.size(); ++ i) {
		if(i > 0) out << ", ";
		out << *glob_def[i].second;
	}
	out << " }";
}
MIRRet CompUnit::DumpMIR(std::vector<MIRInfo*>*) const {
	std::vector<FuncDef*> func_lib = {
		new FuncDef(new BType("int"), "getint", {}),
		new FuncDef(new BType("int"), "getch", {}),
		// new FuncDef(new BType("int"), "getarray", {}),
		new FuncDef(new BType("void"), "putint", {new BType{"int"}}),
		new FuncDef(new BType("void"), "putch", {new BType{"int"}}),
		// new FuncDef(new BType("void"), "putarray", {new BType{"int"}, }),
		new FuncDef(new BType("void"), "starttime", {}),
		new FuncDef(new BType("void"), "stoptime", {})
	};

	std::size_t countFunc = 0, countVar = 0;
	for(auto &item: glob_def) {
		auto tag = item.first;
		auto &detail = item.second;
		if(tag == AST_GT_FUNC) ++ countFunc;
		else if(tag == AST_GT_VAR)
			countVar += LinkedSize( dynamic_cast<StmtVarDef*>(
				dynamic_cast<Stmt*>(detail.get())->detail.get()) );
	}

	std::cerr << "countVar = " << countVar << '\n';

	auto tmp = new ProgramInfo;
	tmp -> vars .init(countVar);
	tmp -> funcs .init(countFunc);

	countFunc = 0;
	countVar = 0;

	for(std::size_t i = 0; i < func_lib.size(); ++ i) {
		auto func = func_lib[i];
		funcMgr[func -> ident] = func;
	}

	for(auto &item: glob_def) {
		auto tag = item.first;
		auto &detail = item.second;
		switch(tag) {
			case AST_GT_FUNC: {
				auto func = dynamic_cast<FuncDef*>(detail.get());
				funcMgr[func->ident] = func;
				tmp -> funcs[countFunc] = dynamic_cast<FuncInfo*>(detail -> DumpMIR(nullptr).mir);
				++ countFunc;
				break;
			}
			case AST_GT_CONST:
				detail->DumpMIR(nullptr);
				break;
			case AST_GT_VAR: {
				std::vector<StmtVarDef*> stmts;
				for(auto stmt = dynamic_cast<Stmt*>(detail.get())->detail.get(); stmt != nullptr;) {
					auto ptr = dynamic_cast<StmtVarDef*>(stmt);
					stmts.emplace_back(ptr);
					if(ptr->next == nullptr) break;
					stmt = ptr->next.get();
				}
				for(auto stmt: stmts) {
					auto var = new VarInfo;
					var->name = domainMgr.newVar(stmt->name);
					var->type = dynamic_cast<TypeInfo*>(stmt->type->DumpMIR(nullptr).mir);
					var->init = new InitializerInfo;
					if(stmt->expr == nullptr) {
						var->init->tag = IT_ZERO;
					}
					else if(stmt->expr->isConst()) {
						int result = stmt->expr->Calc();
						if(result == 0) var->init->tag = IT_ZERO;
						else {
							var->init->tag = IT_NUM;
							var->init->num = result;
						}
					}
					else {
						var->init->tag = IT_ZERO;
						globVarsToInit.emplace_back(stmt);
					}
					tmp->vars[countVar] = var;
					++ countVar;
				}
				break;
			}
		}
	}

	for(auto func: func_lib) delete func;
	return tmp;
}

void FuncParam::Dump(std::ostream &out) const {
	out << "FuncParam { " << *type << ", " << name << " }";
}
MIRRet FuncParam::DumpMIR(std::vector<MIRInfo*>*) const {
	return MIRRet();
}

void FuncDef::Dump(std::ostream &out) const {
	out << "FuncDef { " << *func_type << ", [ ";
	for(std::size_t i = 0; i < params.size(); ++ i) {
		if(i > 0) out << ", ";
		out << *params[i];
	}
	out << " ], " << ident << ", " << *block << " }";
}
MIRRet FuncDef::DumpMIR(std::vector<MIRInfo*>*) const {
	auto blkEntry = NewBlockInfo();

	std::vector<MIRInfo*> buf(1, blkEntry);
	std::vector<std::string> param_init = std::vector<std::string>(params.size(), "");

	domainMgr.push();
	for(std::size_t i = 0; i < params.size(); ++ i)
		param_init[i] = domainMgr.newVar("_para" + std::to_string(i));

	if(ident == "main") {
		// TODO: globVarsToInit;
		for(auto stmt: globVarsToInit) {
			auto stmtInit = new StmtInfo;
			stmtInit->tag = ST_STORE;
			stmtInit->store.addr = new std::string(domainMgr.find(stmt->name).res);
			stmtInit->store.isValue = true;
			stmtInit->store.val = genValue(stmt->expr->DumpMIR(&buf));
			GetLastBlock(&buf) -> stmt.emplace_back(stmtInit);
		}
	}

	for(std::size_t i = 0; i < params.size(); ++ i) {
		auto para = dynamic_cast<FuncParam*>(params[i].get());
		domainMgr.newVar(para->name);

		auto stmtDef = new StmtInfo;
		stmtDef->tag = ST_SYMDEF;
		stmtDef->symdef.tag = SDT_ALLOC;
		stmtDef->symdef.name = new std::string(domainMgr.find(para->name).res);
		stmtDef->symdef.alloc = dynamic_cast<TypeInfo*>(para->type->DumpMIR(nullptr).mir);
		GetLastBlock(&buf) -> stmt.emplace_back(stmtDef);

		auto stmtInit = new StmtInfo;
		stmtInit->tag = ST_STORE;
		stmtInit->store.isValue = true;
		stmtInit->store.addr = new std::string(domainMgr.find(para->name).res);
		stmtInit->store.val = new ValueInfo(param_init[i]);
		GetLastBlock(&buf) -> stmt.emplace_back(stmtInit);
	}

	block -> DumpMIR(&buf);
	
	if(! GetLastBlock(&buf)->closed()) {
		auto stmtRet = new StmtInfo;
		stmtRet->tag = ST_RETURN;
		stmtRet->ret.val = nullptr;
		if(	dynamic_cast<BType*>(func_type.get()) -> type != "void") {
			std::cerr << "[Warning] Maybe control reaches end of non-void function.\n";
			stmtRet->ret.val = new ValueInfo(0);
		}
		GetLastBlock(&buf) -> stmt.emplace_back(stmtRet);
	}

	auto tmp = new FuncInfo;
	tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR(nullptr).mir);
	tmp -> name = ident;
	tmp -> params = std::vector<VarInfo*>(params.size(), nullptr);
	tmp -> block = std::vector<BlockInfo*>(buf.size(), nullptr);

	for(std::size_t i = 0; i < buf.size(); ++ i)
		tmp -> block[i] = dynamic_cast<BlockInfo*>(buf[i]);
	for(std::size_t i = 0; i < params.size(); ++ i) {
		auto var = new VarInfo;
		auto p =  dynamic_cast<FuncParam*>(params[i].get());
		var -> name = param_init[i];
		var -> type = dynamic_cast<TypeInfo*>(p -> type -> DumpMIR(nullptr).mir);
		var -> init = nullptr;
		tmp -> params[i] = var;
	}
	domainMgr.pop();

	return tmp;
}

void BType::Dump(std::ostream &out) const {
	out << "BType { " << type << " }";
}
MIRRet BType::DumpMIR(std::vector<MIRInfo*>*) const {
	auto tmp = new TypeInfo;
	if(type == "int") {
		tmp -> tag = TT_INT32;
	}
	else if(type == "void") {
		tmp -> tag = TT_UNIT;
	}
	return tmp; 
}

void Block::Dump(std::ostream &out) const {
	out << "Block { " ;
	for(std::size_t i = 0; i < stmt.size(); ++ i) {
		if(i > 0) out << ",";
		out << *stmt[i];
	}
	out << " }";
}
MIRRet Block::DumpMIR(std::vector<MIRInfo*> *buf) const {		// Dump Block to vector<BlockInfo*>
	// This requires a BlockInfo created before calling.

	assert(buf != nullptr);
	assert(!buf -> empty());
	std::string ret = "";

	domainMgr.push();
	for(std::size_t i = 0; i < stmt.size(); ++ i) {
		stmt[i] -> DumpMIR(buf);
		if(GetLastBlock(buf) -> closed()) break;
	}
	domainMgr.pop();
	return MIRRet(nullptr, ret);
}

void FunCall::Dump(std::ostream &out) const {
	out << "FunCall { " << func;
	for(const auto &p: params) out << ", " << *p ;
	out << " }";
}
bool FunCall::isConst() const {
	return false;	// TOFIX
}
int FunCall::Calc() const {
	std::cerr << "[Error] Const functions aren't supported yet.\n";
	abort();
	return 0;
}
MIRRet FunCall::DumpMIR(std::vector<MIRInfo*> *buf) const {
	auto stmt = new StmtInfo;
	stmt->tag = ST_SYMDEF;
	stmt->symdef.tag = SDT_FUNCALL;
	if( dynamic_cast<BType*>(funcMgr[func]->func_type.get()) -> type != "void")
		stmt->symdef.name = new std::string(GetTmp());
	else
		stmt->symdef.name = new std::string("");
	stmt->symdef.func.fun = new std::string("@" + func);
	stmt->symdef.func.para = new std::vector<ValueInfo*>();
	for(const auto &p: params) {
		stmt->symdef.func.para -> emplace_back(genValue(p -> DumpMIR(buf)));
	}
	GetLastBlock(buf) -> stmt.emplace_back(stmt);
	return MIRRet(nullptr, *stmt->symdef.name);
}

void Expr::Dump(std::ostream &out) const {
	if(IsUnaryOperator(op)) out << OperatorStr(op) << " { " << *left << " }";
	else out << OperatorStr(op) << " { " << *left << ", " << *right << " }";
}
bool Expr::isConst() const {
	if(IsUnaryOperator(op)) return left->isConst();
	else return left->isConst() && right->isConst();
}
int Expr::Calc() const {
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
	__builtin_unreachable();
}
MIRRet Expr::DumpMIR(std::vector<MIRInfo*> *buf) const {		// Dump Expr to vector<BlockInfo*>
	if(op == OP_POS) return left->DumpMIR(buf);
	std::string crt;
	
	if (op == OP_LAND || op == OP_LOR) {
		auto blkShort = NewBlockInfo();
		auto blkRight = NewBlockInfo();
		auto blkEnd = NewBlockInfo();
		
		auto mirLeft = left->DumpMIR(buf);
		
		auto stmtAlloc = new StmtInfo;
		stmtAlloc->tag = ST_SYMDEF;
		stmtAlloc->symdef.tag = SDT_ALLOC;
		stmtAlloc->symdef.name = new std::string(GetTmp());
		stmtAlloc->symdef.alloc = new TypeInfo;
		stmtAlloc->symdef.alloc->tag = TT_INT32;
		GetLastBlock(buf)->stmt.emplace_back(stmtAlloc);

		auto stmtShort = new StmtInfo;
		stmtShort->tag = ST_STORE;
		stmtShort->store.isValue = true;
		stmtShort->store.addr = new std::string(*stmtAlloc->symdef.name);
		stmtShort->store.val = new ValueInfo(op == OP_LOR);
		blkShort -> stmt.emplace_back(stmtShort);

		auto stmtBr = new StmtInfo;
		stmtBr->tag = ST_BR;
		stmtBr->jump.cond = genValue(mirLeft);
		stmtBr->jump.blkThen = new std::string(blkRight->name);
		stmtBr->jump.blkElse = new std::string(blkShort->name);
		if(op == OP_LOR) swap(stmtBr->jump.blkThen, stmtBr->jump.blkElse);
		GetLastBlock(buf)->stmt.emplace_back(stmtBr);

		buf -> emplace_back(blkShort);
		buf -> emplace_back(blkRight);
		auto mirRight = right->DumpMIR(buf);
		auto blkRightBack = GetLastBlock(buf);

		buf -> emplace_back(blkEnd);

		auto toBool = new StmtInfo;
		toBool->tag = ST_SYMDEF;
		toBool->symdef.tag = SDT_EXPR;
		toBool->symdef.name = new std::string(GetTmp());
		toBool->symdef.expr = new ExprInfo(OP_NEQ, genValue(mirRight), new ValueInfo(0));
		blkRightBack -> stmt.emplace_back(toBool);

		auto stmtStore = new StmtInfo;
		stmtStore->tag = ST_STORE;
		stmtStore->store.isValue = true;
		stmtStore->store.addr = new std::string(*stmtAlloc->symdef.name);
		stmtStore->store.val = new ValueInfo(*toBool->symdef.name);
		blkRightBack -> stmt.emplace_back(stmtStore);

		blkShort -> stmt.emplace_back(GenJump(blkEnd->name));
		blkRightBack -> stmt.emplace_back(GenJump(blkEnd->name));

		auto stmtLoad = new StmtInfo;
		stmtLoad->tag = ST_SYMDEF;
		stmtLoad->symdef.tag = SDT_LOAD;
		stmtLoad->symdef.name = new std::string(GetTmp());
		stmtLoad->symdef.load = new std::string(*stmtAlloc->symdef.name);
		blkEnd -> stmt.emplace_back(stmtLoad);

		crt = *stmtLoad->symdef.name;
	}
	else {
		auto tmp = new ExprInfo;
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
			auto mirLeft = left->DumpMIR(buf), mirRight = right -> DumpMIR(buf);
			tmp->op = op;
			tmp->left = genValue(mirLeft);
			tmp->right = genValue(mirRight);
		}
		auto symd = new StmtInfo;
		symd -> tag = ST_SYMDEF;
		symd -> symdef.tag = SDT_EXPR;
		symd -> symdef.name = new std::string(GetTmp());
		symd -> symdef.expr = tmp;
		GetLastBlock(buf) -> stmt.emplace_back(symd);
		crt = *symd->symdef.name;
	}
	return MIRRet(nullptr, crt);
}

void LVal::Dump(std::ostream &out) const {
	out << "LVal { " << ident << " }";
}
bool LVal::isConst() const {
	return false;
}
int LVal::Calc() const {
	auto found = domainMgr.find(ident);
	assert(found.isImm);
	return found.imm;
}
MIRRet LVal::DumpMIR(std::vector<MIRInfo*> *buf) const {		// Dump LVal to vector<StmtInfo*>
	auto found = domainMgr.find(ident);
	if(found.isImm) return found;
	else {
		auto tmp = new StmtInfo;
		tmp->tag = ST_SYMDEF;
		tmp->symdef.tag = SDT_LOAD;
		tmp->symdef.name = new std::string(GetTmp());
		tmp->symdef.load = new std::string(found.res);
		GetLastBlock(buf) -> stmt.emplace_back(tmp);
		return MIRRet(nullptr, *tmp->symdef.name);
	}
}

void Stmt::Dump(std::ostream &out) const {
	out << "Stmt";
	if(tag == AST_ST_ELSE) out << " (ELSE)";
	else if(tag == AST_ST_BREAK) out << " (BREAK)";
	else if(tag == AST_ST_CONTINUE) out << " (CONTINUE)";
	if(detail != nullptr)
		out << " { " << *detail << " }";
	else
		out << " {}";
}
MIRRet Stmt::DumpMIR(std::vector<MIRInfo*> *buf) const {		// Dump Stmt to vector<BlockInfo*>
	// std::cerr << "DumpMIR: " << *this << '\n';
	switch(tag) {
		case AST_ST_BREAK:
		case AST_ST_CONTINUE: {
			auto stmt = new StmtInfo;
			stmt->tag = ST_JUMP;
			if(tag == AST_ST_BREAK) stmt->jump.blkThen = new std::string(domainMgr.GetBreak());
			else stmt->jump.blkThen = new std::string(domainMgr.GetContinue());
			GetLastBlock(buf) -> stmt.emplace_back(stmt);
			break;
		}
		case AST_ST_ASSIGN:
		case AST_ST_CONSTDEF:
		case AST_ST_VARDEF:
		case AST_ST_EXPR:
		case AST_ST_RETURN:
		case AST_ST_BLOCK:
		case AST_ST_ELSE:
			if(detail != nullptr) detail->DumpMIR(buf);
			break;
		case AST_ST_IF: {
			assert(detail != nullptr);
			StmtIf *realDetail = dynamic_cast<StmtIf*>(detail.get());
			auto cond = genValue(realDetail -> expr -> DumpMIR(buf));
			auto crtBlock = GetLastBlock(buf);

			auto stmtBr = new StmtInfo;
			stmtBr -> tag = ST_BR;
			stmtBr -> jump.cond = cond;

			auto blkNext = NewBlockInfo();
			
			BlockInfo *blkThen = NewBlockInfo(), *blkElse = nullptr;
			buf->emplace_back(blkThen);

			domainMgr.push();
			realDetail -> stmt -> DumpMIR(buf);
			domainMgr.pop();
			if(! GetLastBlock(buf)->closed())
				GetLastBlock(buf) -> stmt.emplace_back(GenJump(blkNext->name));
			stmtBr -> jump.blkThen = new std::string(blkThen->name);
			stmtBr -> jump.blkElse = nullptr;

			if(realDetail->match != nullptr) {
				blkElse = NewBlockInfo();
				buf->emplace_back(blkElse);
				
				domainMgr.push();
				dynamic_cast<Stmt*>(realDetail->match.get()) -> DumpMIR(buf);
				domainMgr.pop();
				if(! GetLastBlock(buf)->closed())
					GetLastBlock(buf) -> stmt.emplace_back(GenJump(blkNext->name));
				stmtBr -> jump.blkElse = new std::string(blkElse->name);
			}
			if(blkElse == nullptr) stmtBr -> jump.blkElse = new std::string(blkNext->name);
			crtBlock -> stmt.emplace_back(stmtBr);
			buf -> emplace_back(blkNext);
			break;
		}
		case AST_ST_WHILE: {
			assert(detail != nullptr);
			auto blkCheck = NewBlockInfo();
			auto blkRun = NewBlockInfo();
			auto blkEnd = NewBlockInfo();
			
			StmtIf *realDetail = dynamic_cast<StmtIf*>(detail.get());
			auto stmtEntry = new StmtInfo;
			stmtEntry->tag = ST_JUMP;
			stmtEntry->jump.blkThen = new std::string(blkCheck->name);
			GetLastBlock(buf) -> stmt.emplace_back(stmtEntry);
			
			buf->emplace_back(blkCheck);
			auto stmtBr = new StmtInfo;
			stmtBr->tag = ST_BR;
			stmtBr->jump.cond = genValue(realDetail -> expr -> DumpMIR(buf));
			stmtBr->jump.blkThen = new std::string(blkRun->name);
			stmtBr->jump.blkElse = new std::string(blkEnd->name);
			GetLastBlock(buf) -> stmt.emplace_back(stmtBr);

			buf->emplace_back(blkRun);
			domainMgr.pushWhile(blkEnd->name, blkCheck->name);
			realDetail->stmt->DumpMIR(buf);
			domainMgr.pop();
			if(! GetLastBlock(buf)->closed()) {
				auto stmtJump = new StmtInfo;
				stmtJump->tag = ST_JUMP;
				stmtJump->jump.blkThen = new std::string(blkCheck->name);
				GetLastBlock(buf) -> stmt.emplace_back(stmtJump);
			}

			buf->emplace_back(blkEnd);
			break;
		}
	}
	return MIRRet();	// maybe return the last block generated
}

void StmtIf::Dump(std::ostream &out) const {
	out << "If { " << *expr << ", " << *stmt;
	if(match != nullptr) out << ", " << *match;
	out << " }";
}
MIRRet StmtIf::DumpMIR(std::vector<MIRInfo*> *) const {
	assert(0);
	return MIRRet();
}
bool StmtIf::tryMatch(Stmt *stmtElse) {
	auto realStmt = dynamic_cast<Stmt*>(stmt.get());
	bool matched = false;
	if(realStmt->tag == AST_ST_IF) {
		auto realStmtIf = dynamic_cast<StmtIf*>(realStmt->detail.get());
		if(realStmtIf->tryMatch(stmtElse))
			matched = true;
	}
	if(!matched) {
		if(match == nullptr) {
			std::cerr << "Try 1\n";
			match = PtrAST(stmtElse);
			matched = true;
		}
		else {
			std::cerr << "Try 2\n";
			auto realElseStmt = dynamic_cast<Stmt*>(dynamic_cast<Stmt*>(match.get())->detail.get());
			if(realElseStmt -> tag == AST_ST_IF && 
			  dynamic_cast<StmtIf*>(realElseStmt->detail.get()) -> tryMatch(stmtElse) )
				matched =true; 
		}
	}
	return matched;
}

void StmtVarDef::Dump(std::ostream &out) const {
	if(expr == nullptr) out << "StmtVarDef { " << *type << ", " << name << ", " << "[NOTHING]" << " }";
	else out << "StmtVarDef { " << *type << ", " << name << ", " << *expr << " }";
	if(next != nullptr) out << ", " << *next;
}
MIRRet StmtVarDef::DumpMIR(std::vector<MIRInfo*> *buf) const {
	// TODO: Check if the type are matched. If not, report the error.
	auto varName = domainMgr.newVar(name);
	auto tmp = new StmtInfo;
	tmp->tag = ST_SYMDEF;
	tmp->symdef.tag = SDT_ALLOC;
	tmp->symdef.name = new std::string(varName);
	tmp->symdef.alloc = dynamic_cast<TypeInfo*>(type -> DumpMIR(nullptr).mir);
	dynamic_cast<BlockInfo*>(buf->back()) -> stmt.emplace_back(tmp);
	
	if(expr != nullptr){
		auto res = expr->DumpMIR(buf);
		tmp = new StmtInfo;
		tmp->tag = ST_STORE;
		tmp->store.isValue = true;
		tmp->store.val = genValue(res);
		tmp->store.addr = new std::string(varName);
		dynamic_cast<BlockInfo*>(buf->back()) -> stmt.emplace_back(tmp);
	}

	if(next) next->DumpMIR(buf);
	return MIRRet();
}

void StmtConstDef::Dump(std::ostream &out) const {
	out << "StmtConstDef { " << *type << ", " << name << ", " << *expr << " }";
	if(next != nullptr) out << ", " << *next;
}
MIRRet StmtConstDef::DumpMIR(std::vector<MIRInfo*> *) const {
	domainMgr.newConst(name, expr -> Calc());
	// TODO: check if type is matched with the result of expr
	if(next != nullptr) next->DumpMIR(nullptr);
	return MIRRet();
}

void StmtAssign::Dump(std::ostream &out) const {
	out << "StmtAssign { " << *lval << ", " << *expr << " }";
}
MIRRet StmtAssign::DumpMIR(std::vector<MIRInfo*> *buf) const {
	auto res = expr->DumpMIR(buf);
	auto tmp = new StmtInfo;
	assert( ! domainMgr.find(dynamic_cast<LVal*>(lval.get()) -> ident).isImm );
	tmp -> tag = ST_STORE;
	tmp -> store.isValue = true;
	tmp -> store.val = genValue(res);
	tmp -> store.addr = new std::string(domainMgr.find(dynamic_cast<LVal*>(lval.get()) -> ident).res);
	dynamic_cast<BlockInfo*>(buf->back()) -> stmt.emplace_back(tmp);
	return MIRRet();
}

void StmtReturn::Dump(std::ostream &out) const {
	if(expr != nullptr) out << "StmtReturn { " << *expr << " }";
	else out << "StmtReturn {}";
}
MIRRet StmtReturn::DumpMIR(std::vector<MIRInfo*> *buf) const {
	auto tmp = new StmtInfo;
	tmp -> tag = ST_RETURN;
	tmp -> ret.val = genValue(expr->DumpMIR(buf));
	dynamic_cast<BlockInfo*>(buf->back()) -> stmt.emplace_back(tmp);
	return MIRRet();
}

void Number::Dump(std::ostream &out) const {
	out << val ;
}
int Number::Calc() const { return val; }
MIRRet Number::DumpMIR(std::vector<MIRInfo*>*) const {
	return MIRRet(nullptr, val);
}