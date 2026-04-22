#include "ast.h"
#include "eval.h"
#include <iostream>

extern int yyparse();
extern FILE *yyin;

ASTNodePtr root;

int main(int argc, char **argv) {
  if (argc > 1) {
    yyin = fopen(argv[1], "r");
    if (!yyin) {
      std::cerr << "Could not open " << argv[1] << std::endl;
      return 1;
    }
  }

  if (yyparse() == 0) {
    if (root) {
      std::cout << "Parse successful!\n";
      root->print();
      auto events = evaluate(root);
      for (const auto &ev : events) {
        std::cout << "(" << ev.part.start.n << "/" << ev.part.start.d << " -> "
                  << ev.part.end.n << "/" << ev.part.end.d << ", "
                  << ev.active.start.n << "/" << ev.active.start.d << " -> "
                  << ev.active.end.n << "/" << ev.active.end.d << ", "
                  << ev.value << ")\n";
      }
    }
  } else {
    std::cerr << "Parse failed.\n";
  }
  return 0;
}
