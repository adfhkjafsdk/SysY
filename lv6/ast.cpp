#include "ast.hpp"

void CompUnit::Dump(std::ostream &out) const {
	out << "CompUnit { " << *func_def << " }";
}
MIRRet CompUnit::DumpMIR(std::vector<MIRInfo*>*) const {
	auto tmp = new ProgramInfo;
	tmp -> vars .init(0);
	tmp -> funcs .init(1);
	tmp -> funcs[0] = dynamic_cast<FuncInfo*>(func_def -> DumpMIR(nullptr).mir);
	return tmp;
}

void FuncDef::Dump(std::ostream &out) const {
	out << "FuncDef { " << *func_type << ", " << ident << ", " << *block << " }";
}
MIRRet FuncDef::DumpMIR(std::vector<MIRInfo*>*) const {
	auto blkEntry = new BlockInfo;
	blkEntry -> name = NewBlock();
	blkEntry -> stmt = std::vector<StmtInfo*>();
	
	std::vector<MIRInfo*> buf(1, blkEntry);
	block -> DumpMIR(&buf);

	auto tmp = new FuncInfo;
	tmp -> ret = dynamic_cast<TypeInfo*>(func_type -> DumpMIR(nullptr).mir);
	tmp -> name = ident;
	tmp -> params = std::vector<VarInfo*>();
	tmp -> block = std::vector<BlockInfo*>(buf.size(), nullptr);
	std::cerr << "buf.size() = " << buf.size() << '\n';
	for(std::size_t i = 0; i < buf.size(); ++ i)
		tmp -> block[i] = dynamic_cast<BlockInfo*>(buf[i]);
	return tmp;
}

void FuncType::Dump(std::ostream &out) const {
	out << "FuncType { " << type << " }";
}
MIRRet FuncType::DumpMIR(std::vector<MIRInfo*>*) const {
	auto tmp = new TypeInfo;
	if(type == "int") {
		tmp -> tag = TT_INT32;
	}
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
		if(dynamic_cast<BlockInfo*>(buf->back()) -> closed()) break;
	}
	domainMgr.pop();
	return MIRRet(nullptr, ret);
}

void Expr::Dump(std::ostream &out) const {
	if(IsUnaryOperator(op)) out << OperatorStr(op) << " { " << *left << " }";
	else out << OperatorStr(op) << " { " << *left << ", " << *right << " }";
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
}
MIRRet Expr::DumpMIR(std::vector<MIRInfo*> *buf) const {		// Dump Expr to vector<StmtInfo*>
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

void LVal::Dump(std::ostream &out) const {
	out << "LVal { " << ident << " }";
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
		buf -> emplace_back(tmp);
		return MIRRet(nullptr, *tmp->symdef.name);
	}
}

void Stmt::Dump(std::ostream &out) const {
	out << "Stmt";
	if(tag == AST_ST_ELSE) out << " (ELSE)";
	if(detail != nullptr)
		out << " { " << *detail << " }";
	else
		out << " {}";
}
MIRRet Stmt::DumpMIR(std::vector<MIRInfo*> *buf) const {		// Dump Stmt to vector<BlockInfo*>
	auto crtBlock = dynamic_cast<BlockInfo*>(buf->back());
	std::vector<MIRInfo*> stmtTmp;
	if(detail != nullptr) {
		switch(tag) {
			case AST_ST_ASSIGN:
			case AST_ST_CONSTDEF:
			case AST_ST_VARDEF:
			case AST_ST_EXPR:
			case AST_ST_RETURN: {
				detail->DumpMIR(&stmtTmp);
				crtBlock->stmt.reserve(crtBlock->stmt.size() + stmtTmp.size());
				for(auto s: stmtTmp)
					crtBlock->stmt.emplace_back(dynamic_cast<StmtInfo*>(s));
				break;
			} 
			case AST_ST_BLOCK:
				detail->DumpMIR(buf);
				break;
			case AST_ST_IF: {
				StmtIf *realDetail = dynamic_cast<StmtIf*>(detail.get());
				auto cond = genValue(realDetail -> expr -> DumpMIR(&stmtTmp));
				crtBlock->stmt.reserve(crtBlock->stmt.size() + stmtTmp.size());
				for(auto s: stmtTmp)
					crtBlock->stmt.emplace_back(dynamic_cast<StmtInfo*>(s));

				auto stmtBr = new StmtInfo;
				stmtBr -> tag = ST_BR;
				stmtBr -> jump.cond = cond;

				auto blkNext = new BlockInfo;
				blkNext->name = NewBlock();
				blkNext->stmt = std::vector<StmtInfo*>();
				
				auto genJump = [&]() {
					auto stmt = new StmtInfo;
					stmt->tag = ST_JUMP;
					stmt->jump.blkThen = new std::string(blkNext->name);
					return stmt;
				};

				BlockInfo *blkThen = new BlockInfo, *blkElse = nullptr;
				blkThen->name = NewBlock();
				blkThen->stmt = std::vector<StmtInfo*>();
				buf->emplace_back(blkThen);

				domainMgr.push();
				realDetail -> stmt -> DumpMIR(buf);
				domainMgr.pop();
				if(! dynamic_cast<BlockInfo*>(buf->back())->closed())
					dynamic_cast<BlockInfo*>(buf->back()) -> stmt.emplace_back(genJump());
				stmtBr -> jump.blkThen = new std::string(blkThen->name);
				stmtBr -> jump.blkElse = nullptr;

				if(realDetail->match != nullptr) {
					blkElse = new BlockInfo;
					blkElse->name = NewBlock();
					blkElse->stmt = std::vector<StmtInfo*>();
					buf->emplace_back(blkElse);
					
					domainMgr.push();
					dynamic_cast<Stmt*>(realDetail->match.get()) -> DumpMIR(buf);
					domainMgr.pop();
					if(! dynamic_cast<BlockInfo*>(buf->back())->closed())
						dynamic_cast<BlockInfo*>(buf->back()) -> stmt.emplace_back(genJump());
					stmtBr -> jump.blkElse = new std::string(blkElse->name);
				}
				if(blkElse == nullptr) stmtBr -> jump.blkElse = new std::string(blkNext->name);
				crtBlock -> stmt.emplace_back(stmtBr);
				buf -> emplace_back(blkNext);
				break;
			}
			case AST_ST_ELSE: {
				detail->DumpMIR(buf);
				break;
			}
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
	buf -> emplace_back(tmp);
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
	buf -> emplace_back(tmp);
	return MIRRet();
}

void Number::Dump(std::ostream &out) const {
	out << val ;
}
int Number::Calc() const { return val; }
MIRRet Number::DumpMIR(std::vector<MIRInfo*>*) const {
	return MIRRet(nullptr, val);
}