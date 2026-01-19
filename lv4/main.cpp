#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cassert>

#include "ast.hpp"

extern FILE *yyin;
extern int yyparse (ASTree &ast);
extern void ProgramToIR(std::ostream &out, ProgramInfo *mir);
extern void ProgramToASM(std::ostream &out, ProgramInfo *mir);

extern int yydebug;

int main(int argc, char *argv[]) {
	if( argc != 5 || std::strcmp(argv[3], "-o") ) {
		fprintf(stderr, 
			"  SysY Compiler - Compile SysY code to Koopa IR\n"
			"\n"
			"  Usage: %s <Mode> <Source File Path> -o <Output Path>\n",
			argv[0]
		);
		return 1;
	}

	const char *mode   = argv[1];
	const char *input  = argv[2];
	const char *output = argv[4];

	yyin = fopen(input, "r");
	
	if(yyin == NULL) {
		fprintf(stderr, "Could not open source file: %s\n", input);
		return 1;
	}
	
	ASTree ast;

	#ifndef NDEBUG
	yydebug = 1;
	#endif

	int ret = yyparse(ast);
	if(ret) {
		fprintf(stderr, "Parse failed. Returned %d.\n", ret);
		return 1;
	}

	std::cerr << *ast << std::endl;

	return 0;

	std::ofstream fout(output, std::ios::out);
	
	if(strcmp(mode, "-koopa") && strcmp(mode, "-riscv")) {
		fprintf(stderr, "Unknown mode \"%s\"\n", mode);
		return 1;
	}
	
	// return 0;
	auto prog = dynamic_cast<ProgramInfo*> (ast -> DumpMIR(nullptr).mir);
	// return 0;
	
	if(!strcmp(mode, "-koopa")) {
		ProgramToIR(fout, prog);
	}
	else if(!strcmp(mode, "-riscv")) {
		ProgramToASM(fout, prog);
	}
	prog -> clean();
	// clean ast
	return 0;
}