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
%type <ast_val> FuncDef FuncType Block Stmt 
%type <ast_val> Exp PrimaryExp UnaryExp MulExp AddExp EqExp LAndExp LOrExp
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
		tmp -> exp = $2;
		$$ = std::move(tmp);
	}

Exp
	: LOrExp { $$ = std::move($1); }

PrimaryExp
	: "(" Exp ")" {}
	| Number {}

UnaryExp
	: PrimaryExp {
		
	}
	| "+" UnaryExp {}
	| "-" UnaryExp {}
	| "!" UnaryExp {}

MulExp
	: UnaryExp {}
	| MulExp "*" UnaryExp {}
	| MulExp "/" UnaryExp {}
	| MulExp "%" UnaryExp {}

AddExp
	: MulExp {}
	| AddExp "+" MulExp {}
	| AddExp "-" MulExp {}

RelExp
	: AddExp {}
	| RelExp "<" AddExp {}
	| RelExp ">" AddExp {}
	| RelExp "<=" AddExp {}
	| RelExp ">=" AddExp {}

EqExp
	: RelExp {}
	| EqExp "==" RelExp {}
	| EqExp "!=" RelExp {}

LAndExp
	: EqExp {}
	| LAndExp "&&" EqExp {}

LOrExp
	: LAndExp {}
	| LOrExp "||" LAndExp {}

Number
	: INT_CONST {
		$$ = $1;
	}

%%

void yyerror(ASTree &ast, const char *s) {
	(void) ast ;
	std::cerr << "error: " << s << std::endl;
}