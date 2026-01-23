#include "ast.hpp"

void Stmt::Dump(std::ostream &out) const {
	// out << "Stmt { return " << *exp << " }";
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

				if(realDetail->match != nullptr) {
					blkElse = new BlockInfo;
					blkElse->name = NewBlock();
					blkElse->stmt = std::vector<StmtInfo*>();
					buf->emplace_back(blkElse);
					
					domainMgr.push();
					dynamic_cast<Stmt*>(realDetail->match.get()) -> DumpMIR(buf);
					domainMgr.pop();
					stmtBr -> jump.blkElse = new std::string(blkElse->name);
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
	return MIRRet();		// This should return the entry Block name of these statements.	// No, there may be not an entire block
}