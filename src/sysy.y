%code requires {
	#include <memory>
	#include <string>
}
// This will be written to the header

%{

#include <iostream>
#include <memory>
#include <string>

int yylex();
void yyerror(std::unique_ptr<std::string> &ast, const char *s);

%}
// This will be written to the source

%parse-param { std::unique_ptr<std::string> &ast }

%union {
	std::string *str_val;
	int int_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符类型定义
%type <str_val> FuncDef FuncType Block Stmt Number

%%

CompUnit
  : FuncDef {
	ast = std::unique_ptr<std::string>($1);
  }
  ;

FuncDef
	: FuncType IDENT '(' ')' Block {
		auto type = std::unique_ptr<std::string>($1);
		auto ident = std::unique_ptr<std::string>($2);
		auto block = std::unique_ptr<std::string>($5);
		$$ = new std::string(*type + " " +  *ident + "() " + *block); 
	}
	;

FuncType
	: INT {
		$$ = new std::string("int");
	}
	;

Block
	: '{' Stmt '}' {
		auto stmt = std::unique_ptr<std::string>($2);
		$$ = new std::string("{ " + *stmt + " }");
	}
	;

Stmt
	: RETURN Number ';' {
		auto val = std::unique_ptr<std::string>($2);
		$$ = new std::string("return " + *val + ";");
	}

Number
	: INT_CONST {
		$$ = new std::string(std::to_string($1));
	}

%%

void yyerror(std::unique_ptr<std::string> &ast, const char *s) {
	std::cerr << "error: " << s << std::endl;
}