%option noyywrap

%{

#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "sysy.tab.hpp"

%}

/*Flex supports comments*/

LineComment "//".*
BlockComment "/*"([^*]|"*"[^/])*"*/"
WhiteSpace [ \t\r\n]

Identifier [A-Za-z_][A-Za-z0-9_]*
Decimal [1-9][0-9]*
Octal 0[0-7]*
Hexadecimal 0[xX][0-9A-Fa-f]+

%%

{WhiteSpace}	{ /* Ignore */ }
{LineComment}	{ /* Ignore */ std::cerr << "LineComment " << yytext <<'\n' ;}
{BlockComment}	{ /* Ignore */ std::cerr << "BlockComment " << yytext <<'\n' ;}
"int"			{  std::cerr << "INT " << yytext <<'\n' ;return INT;}
"return"		{ std::cerr << "RETURN " << yytext <<'\n' ;return RETURN; }
{Identifier}	{ std::cerr << "IDENT" << yytext <<'\n' ;yylval.str_val = new std::string(yytext); return IDENT; }
{Decimal}		{ std::cerr << "DEC" << yytext <<'\n' ; yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST;  }
{Octal}			{ std::cerr << "OCT" << yytext <<'\n' ; yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST;  }
{Hexadecimal}	{ std::cerr << "HEX" << yytext <<'\n' ; yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST; }
.				{ return yytext[0]; }

%%
