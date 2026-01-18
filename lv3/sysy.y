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
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符类型定义
%type <ast_val> FuncDef FuncType Block Stmt Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
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
	: '{' Stmt '}' {
		auto tmp = new Block;
		tmp -> stmt = PtrAST($2);
		$$ = std::move(tmp);
	}
	;

Stmt
	: RETURN Exp ';' {
		auto tmp = new Stmt;
		tmp -> exp = PtrAST($2);
		$$ = std::move(tmp);
	}
	;

Exp
	: LOrExp { $$ = std::move($1); }
	;

PrimaryExp
	: '(' Exp ')' { $$ = std::move($2); }
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
	| EqExp '=' '=' RelExp { $$ = new Expr(OP_EQ, std::move($1), std::move($4)); }
	| EqExp '!' '=' RelExp { $$ = new Expr(OP_NEQ, std::move($1), std::move($4)); }
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