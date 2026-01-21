%debug

%code requires {
	#include <memory>
	#include <string>
	#include "ast.hpp"
}
// This will be written to the header

%{

#include <iostream>
#include <memory>
#include <string>

#include "ast.hpp"

int yylex();
void yyerror(ASTree &ast, const char *s);

%}
// This will be written to the source

%parse-param { ASTree &ast }

%union {
	std::string *str_val;
	int int_val;
	BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT CONST RETURN SYM_EQ SYM_NEQ
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符类型定义
%type <ast_val> FuncDef FuncType Block Stmt Stmts
%type <ast_val> ConstExp Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> BType InitVal VarDef VarDefs
%type <ast_val> ConstInitVal ConstDef ConstDefs
%type <ast_val> LVal 
%type <int_val> Number

%%

CompUnit
	: FuncDef {
		auto tmp = std::make_unique<CompUnit>();
		tmp -> func_def = PtrAST($1);
		ast = std::move(tmp);
	}
	;

FuncDef
	: FuncType IDENT '(' ')' Block {
		auto tmp = new FuncDef;
		tmp -> func_type = PtrAST($1);
		tmp -> ident = *$2;
		tmp -> block = PtrAST($5);
		delete $2;
		$$ = std::move(tmp);
	}
	;

FuncType
	: INT {
		auto tmp = new FuncType;
		tmp -> type = "int";
		$$ = std::move(tmp);
	}
	;

Block
	: '{' Stmts '}' {
		auto tmp = new Block;
		tmp->stmt = PtrAST($2);
		$$ = std::move(tmp);
	}
	;

Stmts
	: { $$ = nullptr; }
	| Stmt Stmts {
		auto tmp = dynamic_cast<Stmt*>($1);
		tmp->next = PtrAST($2);
		$$ = std::move(tmp);
	}
	;

Stmt
	: CONST BType ConstDefs ';' {
		auto shared = SharedAST($2);
		for(auto it = dynamic_cast<StmtConstDef*>($3); it != nullptr; it = dynamic_cast<StmtConstDef*>(it->next.get()) ) {
			it->type = shared;
		}
		auto tmp = new Stmt;
		tmp->tag = AST_ST_CONSTDEF;
		tmp->detail = PtrAST($3);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| BType VarDefs ';' {
		auto shared = SharedAST($1);
		for(auto it = dynamic_cast<StmtVarDef*>($2); it != nullptr; it = dynamic_cast<StmtVarDef*>(it->next.get()) ) {
			it->type = shared;
		}
		auto tmp = new Stmt;
		tmp->tag = AST_ST_VARDEF;
		tmp->detail = PtrAST($2);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| Exp ';' {
		auto tmp = new Stmt;
		tmp->tag = AST_ST_EXPR;
		tmp->detail = PtrAST($1);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| ';' { 
		auto tmp = new Stmt;
		tmp->tag = AST_ST_EXPR;
		tmp->detail = nullptr;
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| LVal '=' Exp ';' {
		auto det = new StmtAssign;
		det->lval = PtrAST($1);
		det->expr = PtrAST($3);
		auto tmp = new Stmt;
		tmp->tag = AST_ST_ASSIGN;
		tmp->detail = PtrAST(det);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| Block {
		auto tmp = new Stmt;
		tmp->tag = AST_ST_BLOCK;
		tmp->detail = PtrAST($1);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| RETURN ';' {
		auto det = new StmtReturn;
		det->expr = nullptr;
		auto tmp = new Stmt;
		tmp->tag = AST_ST_RETURN;
		tmp->detail = PtrAST(det);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	| RETURN Exp ';' {
		auto det = new StmtReturn;
		det->expr = PtrAST($2);
		auto tmp = new Stmt;
		tmp->tag = AST_ST_RETURN;
		tmp->detail = PtrAST(det);
		tmp->next = nullptr;
		$$ = std::move(tmp);
	}
	;

BType
	: INT { 
		auto tmp = new BType;
		tmp -> type = "int";
		$$ = std::move(tmp);
	}
	;

LVal
	: IDENT { 
		auto tmp = new LVal;
		tmp -> ident = *($1);
		delete $1;
		$$ = std::move(tmp);
	}
	;

ConstDefs
	: ConstDef {
		$$ = std::move($1);
	}
	| ConstDef ',' ConstDefs {
		auto tmp = dynamic_cast<StmtConstDef*>($1);
		tmp -> next = PtrAST($3);
		$$ = std::move(tmp);
	}
	;

ConstDef
	: IDENT '=' ConstInitVal {
		auto tmp = new StmtConstDef;
		tmp->name = *$1;
		tmp->type = nullptr;
		tmp->expr = PtrAST($3);
		tmp->next = nullptr;
		delete $1;
		$$ = std::move(tmp);
	}
	;

ConstInitVal
	: ConstExp { $$ = std::move($1); }
	;

ConstExp
	: Exp { $$ = std::move($1); }
	;

VarDefs
	: VarDef {
		$$ = std::move($1);
	}
	| VarDef ',' VarDefs {
		auto tmp = dynamic_cast<StmtVarDef*>($1);
		tmp -> next = PtrAST($3);
		$$ = std::move(tmp);
	}
	;

VarDef
	: IDENT {
		auto tmp = new StmtVarDef;
		tmp->name = *$1;
		tmp->type = nullptr;
		tmp->expr = nullptr;
		tmp->next = nullptr;
		delete $1;
		$$ = std::move(tmp);
	}
	| IDENT '=' InitVal {
		auto tmp = new StmtVarDef;
		tmp->name = *$1;
		tmp->type = nullptr;
		tmp->expr = PtrAST($3);
		tmp->next = nullptr;
		delete $1;
		$$ = std::move(tmp);
	}
	;

InitVal
	: Exp { $$ = std::move($1); }
	;

Exp
	: LOrExp { $$ = std::move($1); }
	;

PrimaryExp
	: '(' Exp ')' { $$ = std::move($2); }
	| LVal {
		$$ = $1;
	}
	| Number {
		auto tmp = new Number;
		tmp->val = $1;
		$$ = std::move(tmp);
	}
	;

UnaryExp
	: PrimaryExp { $$ = std::move($1); }
	| '+' UnaryExp { $$ = new Expr(OP_POS, std::move($2), nullptr); }
	| '-' UnaryExp { $$ = new Expr(OP_NEG, std::move($2), nullptr); }
	| '!' UnaryExp { $$ = new Expr(OP_LNOT, std::move($2), nullptr); }
	;

MulExp
	: UnaryExp { $$ = std::move($1); }
	| MulExp '*' UnaryExp { $$ = new Expr(OP_MUL, std::move($1), std::move($3)); }
	| MulExp '/' UnaryExp { $$ = new Expr(OP_DIV, std::move($1), std::move($3)); }
	| MulExp '%' UnaryExp { $$ = new Expr(OP_MOD, std::move($1), std::move($3)); }
	;

AddExp
	: MulExp { $$ = std::move($1); }
	| AddExp '+' MulExp { $$ = new Expr(OP_ADD, std::move($1), std::move($3)); }
	| AddExp '-' MulExp { $$ = new Expr(OP_SUB, std::move($1), std::move($3)); }
	;

RelExp
	: AddExp { $$ = std::move($1); }
	| RelExp '<' AddExp { $$ = new Expr(OP_LT, std::move($1), std::move($3)); }
	| RelExp '>' AddExp { $$ = new Expr(OP_GT, std::move($1), std::move($3)); }
	| RelExp '<' '=' AddExp { $$ = new Expr(OP_LE, std::move($1), std::move($4)); }
	| RelExp '>' '=' AddExp { $$ = new Expr(OP_GE, std::move($1), std::move($4)); }
	;

EqExp
	: RelExp { $$ = std::move($1); }
	| EqExp SYM_EQ RelExp  { $$ = new Expr(OP_EQ, std::move($1), std::move($3)); }
	| EqExp SYM_NEQ RelExp { $$ = new Expr(OP_NEQ, std::move($1), std::move($3)); }
	;

LAndExp
	: EqExp { $$ = std::move($1); }
	| LAndExp '&' '&' EqExp { $$ = new Expr(OP_LAND, std::move($1), std::move($4)); }
	;

LOrExp
	: LAndExp { $$ = std::move($1); }
	| LOrExp '|' '|' LAndExp { $$ = new Expr(OP_LOR, std::move($1), std::move($4)); }
	;

Number
	: INT_CONST {
		$$ = $1;
	}
	;

%%

void yyerror(ASTree &ast, const char *s) {
	(void) ast ;
	std::cerr << "error: " << s << std::endl;
}