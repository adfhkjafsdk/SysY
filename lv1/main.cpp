#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "ast.hpp"

extern FILE *yyin;
extern int yyparse (ASTree &ast);

int main(int argc, char *argv[]) {
	if(argc != 5) {
		fprintf(stderr, 
			"Usage: %s <Mode> <Source File Path> -o <Output Path>\n",
			argv[0]
		);
		return 1;
	}

	const char *input = argv[2];

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

	std::cout << *ast << std::endl;
	return 0;
}