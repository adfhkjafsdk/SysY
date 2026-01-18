%option noyywrap

%{

#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "sysy.tab.hpp"
#include "debug.hpp"

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
{LineComment}	{ /* Ignore */ derr << "LineComment " << yytext <<'\n' ;}
{BlockComment}	{ /* Ignore */ derr << "BlockComment " << yytext <<'\n' ;}
"int"			{ derr << "INT " << yytext <<'\n' ;return INT;}
"return"		{ derr << "RETURN " << yytext <<'\n' ; return RETURN; }
{Identifier}	{ derr << "IDENT " << yytext <<'\n' ; yylval.str_val = new std::string(yytext); return IDENT; }
{Decimal}		{ derr << "DEC " << yytext <<'\n' ; yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST;  }
{Octal}			{ derr << "OCT " << yytext <<'\n' ; yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST;  }
{Hexadecimal}	{ derr << "HEX " << yytext <<'\n' ; yylval.int_val = std::strtol(yytext, nullptr, 0); return INT_CONST; }
.				{ derr << "SYMBOL " << yytext[0] << '\n'; return yytext[0]; }

%%
