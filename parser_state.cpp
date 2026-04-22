// Defines the global AST root used by the bison parser and parse_string().
// Included in the chugin build but NOT in the standalone binary
// (main.cpp defines its own root there).
#include "ast.h"

ASTNodePtr root = nullptr;
