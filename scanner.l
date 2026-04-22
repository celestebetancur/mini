%{
#include "ast.h"
#include "parser.tab.h"
#include <cstdlib>
#include <string>

// To keep track of location if needed
%}

%option noyywrap
%option yylineno

/* Unicode letter approximations and symbols for steps */
STEP_CHAR     ([A-Za-z0-9~\-#\.\^_])

%%

[ \t\n\r\xA0]+  { /* Ignore whitespace */ }

"//"([^\n]*)    { /* Ignore comments */ }

"setcps"        { return SETCPS; }
"setbpm"        { return SETBPM; }
"hush"          { return HUSH;   }

"struct"        { return STRUCT; }
"target"        { return TARGET; }
"euclid"        { return EUCLID; }
"slow"          { return SLOW;   }
"rotL"          { return ROTL;   }
"rotR"          { return ROTR;   }
"fast"          { return FAST;   }
"scale"         { return SCALE;  }
"cat"           { return CAT;    }

"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACK; }
"]"             { return RBRACK; }
"<"             { return LT;     }
">"             { return GT;     }
"("             { return LPAREN; }
")"             { return RPAREN; }

".."            { return DOTDOT; }
"|"             { return PIPE;   }
"."             { return DOT;    }
","             { return COMMA;  }
"%"             { return PERCENT;}
"@"             { return AT;     }
"_"             { return UNDERSCORE; }
"!"             { return BANG;   }
"/"             { return SLASH;  }
"*"             { return STAR;   }
"?"             { return QUESTION;}
":"             { return COLON;  }
"^"             { return CARET;  }
"$"             { return DOLLAR; }

\"              { return '"'; }
\'              { return '\''; }

\-?[0-9]+(\.[0-9]+)?([eE][\+\-]?[0-9]+)? {
                    yylval.d_val = std::atof(yytext);
                    return NUMBER;
                }

{STEP_CHAR}+    {
                    yylval.str_val = new std::string(yytext);
                    return STEP;
                }

.               { /* Catch all for other single characters if any */ return yytext[0]; }

%%
