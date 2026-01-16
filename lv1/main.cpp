#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "ast.hpp"

extern FILE *yyin;
extern int yyparse (ASTree &ast);

int main(int argc, char *argv[]) {
	if( argc != 5 || std::strcmp(argv[3], "-o") ) {
		fprintf(stderr, 
			"SysY Compiler - Compile SysY code to Koopa IR\n"
			"\n"
			"  Usage: %s <Mode> <Source File Path> -o <Output Path>\n",
			argv[0]
		);
		return 1;
	}

	const char *input = argv[2];
	const char *output = argv[4];

	yyin = fopen(input, "r");
	
	if(yyin == NULL) {
		fprintf(stderr, "Could not open source file: %s\n", input);
		return 1;
	}
	
	ASTree ast;
	int ret = yyparse(ast);
	if(ret) {
		fprintf(stderr, "Parse failed. Returned %d.\n", ret);
		return 1;
	}

	std::cerr << *ast << std::endl;

	std::ofstream fout(output, std::ios::out);
	ast -> DumpIR(fout);
	
	return 0;
}