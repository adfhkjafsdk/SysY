%option noyywrap

%{

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "sysy.tab.hpp"

%}

/*Flex supports comments*/

WhiteSpace [ \t\r\n]
LineComment "//".*

Identifier [A-Za-z_][A-Za-z0-9_]*
Decimal [1-9][0-9]*
Octal 0[0-7]*
Hexadecimal 0[xX][0-9A-Fa-f]+

%%

{WhiteSpace}	{ /* Ignore */ }
{LineComment}	{ /* Ignore */ }
"int"			{ return INT; }
"return"		{ return RETURN; }
{Identifier}	{ yylval.str_val = new std::string(yytext); return IDENT; }
{Decimal}		{ yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}			{ yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}	{ yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST; }
.				{ return yytext[0]; }

%%
