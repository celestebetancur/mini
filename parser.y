%{
#include "ast.h"
#include <iostream>
#include <string>
#include <vector>

void yyerror(const char *s);
int yylex(void);

extern ASTNodePtr root;
static int global_seed = 0;
%}

%code requires {
#include "ast.h"
#include <vector>
#include <string>
}

%union {
    double d_val;
    std::string* str_val;
    ASTNode *node_val;
    std::vector<ASTNode*> *node_list;
    std::vector<Operation> *op_list;
    Operation *op;
}

%token <d_val> NUMBER
%token <str_val> STEP
%token SETCPS SETBPM HUSH
%token STRUCT TARGET EUCLID SLOW ROTL ROTR FAST SCALE CAT
%token LBRACE RBRACE LBRACK RBRACK LT GT LPAREN RPAREN
%token DOTDOT PIPE DOT COMMA PERCENT AT UNDERSCORE BANG SLASH STAR QUESTION COLON CARET DOLLAR

%type <node_val> start statement command mini_definition sequ_or_operator_or_comment
%type <node_val> mini_or_operator mini_or_group cat operator mini polymeter_stack stack_or_choose
%type <node_list> dot_tail choose_tail stack_tail
%type <node_val> sequence slice_with_ops slice step sub_cycle polymeter slow_sequence polymeter_steps
%type <op> slice_op op_weight op_bjorklund op_slow op_fast op_replicate op_degrade op_tail op_range
%type <op_list> slice_op_list
%type <node_list> slice_with_ops_list dot_tail_list choose_tail_list stack_tail_list sequence_list mini_or_operator_list

%%

start:
    statement { root = ASTNodePtr($1); }
    ;

statement:
    mini_definition { $$ = $1; }
    | command { $$ = $1; }
    ;

command:
    SETCPS NUMBER { auto c = new CommandStub("setcps"); c->options["value"] = { "", $2, 0, nullptr, 1 }; $$ = c; }
    | SETBPM NUMBER { auto c = new CommandStub("setcps"); c->options["value"] = { "", $2 / 120.0 / 2.0, 0, nullptr, 1 }; $$ = c; }
    | HUSH { $$ = new CommandStub("hush"); }
    ;

mini_definition:
    sequ_or_operator_or_comment { $$ = $1; }
    | /* empty */ { $$ = nullptr; }
    ;

sequ_or_operator_or_comment:
    mini_or_operator { $$ = $1; }
    ;

mini_or_operator:
    mini_or_group { $$ = $1; }
    | operator DOLLAR mini_or_operator { 
        auto op = dynamic_cast<OperatorStub*>($1);
        if(op) op->child = ASTNodePtr($3);
        $$ = op;
    }
    ;

mini_or_group:
    cat { $$ = $1; }
    | mini { $$ = $1; }
    ;

cat:
    CAT LBRACK mini_or_operator mini_or_operator_list RBRACK {
        auto p = new PatternStub("slowcat");
        p->list.push_back(ASTNodePtr($3));
        auto lst = $4;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    ;

mini_or_operator_list:
    /* empty */ { $$ = new std::vector<ASTNode*>(); }
    | mini_or_operator_list COMMA mini_or_operator { $1->push_back($3); $$ = $1; }
    ;

mini:
    '"' stack_or_choose '"' { $$ = $2; }
    | '\'' stack_or_choose '\'' { $$ = $2; }
    ;

operator:
    SCALE '"' STEP '"' {
        auto op = new OperatorStub("scale");
        op->args["scale"] = { *$3, 0.0, 0, nullptr, 0 };
        delete $3;
        $$ = op;
    }
    | SCALE '\'' STEP '\'' {
        auto op = new OperatorStub("scale");
        op->args["scale"] = { *$3, 0.0, 0, nullptr, 0 };
        delete $3;
        $$ = op;
    }
    | SLOW NUMBER {
        auto op = new OperatorStub("stretch");
        op->args["amount"] = { "", $2, 0, nullptr, 1 };
        $$ = op;
    }
    | FAST NUMBER {
        auto op = new OperatorStub("stretch");
        op->args["amount"] = { "", 1.0 / $2, 0, nullptr, 1 };
        $$ = op;
    }
    | TARGET '"' STEP '"' {
        auto op = new OperatorStub("target");
        op->args["name"] = { *$3, 0.0, 0, nullptr, 0 };
        delete $3;
        $$ = op;
    }
    | TARGET '\'' STEP '\'' {
        auto op = new OperatorStub("target");
        op->args["name"] = { *$3, 0.0, 0, nullptr, 0 };
        delete $3;
        $$ = op;
    }
    | EUCLID NUMBER NUMBER {
        auto op = new OperatorStub("bjorklund");
        op->args["pulse"] = { "", $2, 0, nullptr, 1 };
        op->args["step"] = { "", $3, 0, nullptr, 1 };
        $$ = op;
    }
    | STRUCT mini_or_operator {
        auto op = new OperatorStub("struct");
        op->args["mini"] = { "", 0.0, 0, ASTNodePtr($2), 3 };
        $$ = op;
    }
    | ROTR NUMBER {
        auto op = new OperatorStub("shift");
        op->args["amount"] = { "", $2, 0, nullptr, 1 };
        $$ = op;
    }
    | ROTL NUMBER {
        auto op = new OperatorStub("shift");
        op->args["amount"] = { "", -$2, 0, nullptr, 1 };
        $$ = op;
    }
    ;

stack_or_choose:
    sequence stack_tail { 
        auto p = new PatternStub("stack");
        p->list.push_back(ASTNodePtr($1));
        auto lst = $2;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    | sequence choose_tail {
        auto p = new PatternStub("rand", global_seed++);
        p->list.push_back(ASTNodePtr($1));
        auto lst = $2;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    | sequence dot_tail {
        auto p = new PatternStub("feet", global_seed++);
        p->list.push_back(ASTNodePtr($1));
        auto lst = $2;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    | sequence { $$ = $1; }
    ;

stack_tail:
    stack_tail_list { $$ = $1; }
    ;

stack_tail_list:
    COMMA sequence { $$ = new std::vector<ASTNode*>(); $$->push_back($2); }
    | stack_tail_list COMMA sequence { $1->push_back($3); $$ = $1; }
    ;

choose_tail:
    choose_tail_list { $$ = $1; }
    ;

choose_tail_list:
    PIPE sequence { $$ = new std::vector<ASTNode*>(); $$->push_back($2); }
    | choose_tail_list PIPE sequence { $1->push_back($3); $$ = $1; }
    ;

dot_tail:
    dot_tail_list { $$ = $1; }
    ;

dot_tail_list:
    DOT sequence { $$ = new std::vector<ASTNode*>(); $$->push_back($2); }
    | dot_tail_list DOT sequence { $1->push_back($3); $$ = $1; }
    ;

polymeter_stack:
    sequence { $$ = new PatternStub("polymeter"); dynamic_cast<PatternStub*>($$)->list.push_back(ASTNodePtr($1)); }
    | sequence stack_tail {
        auto p = new PatternStub("polymeter");
        p->list.push_back(ASTNodePtr($1));
        auto lst = $2;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    ;

sequence:
    CARET sequence_list {
        auto p = new PatternStub("fastcat", 0, true);
        auto lst = $2;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    | sequence_list {
        auto p = new PatternStub("fastcat");
        auto lst = $1;
        for(size_t i=0; i<lst->size(); ++i) p->list.push_back(ASTNodePtr((*lst)[i]));
        delete lst;
        $$ = p;
    }
    ;

sequence_list:
    slice_with_ops { $$ = new std::vector<ASTNode*>(); $$->push_back($1); }
    | sequence_list slice_with_ops { $1->push_back($2); $$ = $1; }
    ;

slice_with_ops:
    slice slice_op_list {
        auto e = new ElementStub(ASTNodePtr($1));
        auto lst = $2;
        for(size_t i=0; i<lst->size(); ++i) {
            auto& op = (*lst)[i];
            if(op.type_ == "weight") {
                e->weight += op.arguments["amount"].d_val - 1.0;
            } else if(op.type_ == "replicate") {
                e->reps += op.arguments["amount"].d_val - 1.0;
                e->weight = e->reps;
            } else {
                e->ops.push_back(op);
            }
        }
        delete lst;
        $$ = e;
    }
    | slice {
        $$ = new ElementStub(ASTNodePtr($1));
    }
    ;

slice_with_ops_list:
    slice_with_ops { $$ = new std::vector<ASTNode*>(); $$->push_back($1); }
    | slice_with_ops_list slice_with_ops { $1->push_back($2); $$ = $1; }
    ;

slice_op_list:
    slice_op { $$ = new std::vector<Operation>(); $$->push_back(*$1); delete $1; }
    | slice_op_list slice_op { $1->push_back(*$2); delete $2; $$ = $1; }
    ;

slice_op:
    op_weight { $$ = $1; }
    | op_bjorklund { $$ = $1; }
    | op_slow { $$ = $1; }
    | op_fast { $$ = $1; }
    | op_replicate { $$ = $1; }
    | op_degrade { $$ = $1; }
    | op_tail { $$ = $1; }
    | op_range { $$ = $1; }
    ;

op_weight:
    AT NUMBER { $$ = new Operation{"weight", {{"amount", {"", $2, 0, nullptr, 1}}}}; }
    | UNDERSCORE NUMBER { $$ = new Operation{"weight", {{"amount", {"", $2, 0, nullptr, 1}}}}; }
    | AT { $$ = new Operation{"weight", {{"amount", {"", 2.0, 0, nullptr, 1}}}}; }
    | UNDERSCORE { $$ = new Operation{"weight", {{"amount", {"", 2.0, 0, nullptr, 1}}}}; }
    ;

op_replicate:
    BANG NUMBER { $$ = new Operation{"replicate", {{"amount", {"", $2, 0, nullptr, 1}}}}; }
    | BANG { $$ = new Operation{"replicate", {{"amount", {"", 2.0, 0, nullptr, 1}}}}; }
    ;

op_bjorklund:
    LPAREN slice_with_ops COMMA slice_with_ops RPAREN {
        $$ = new Operation{"bjorklund", {
            {"pulse", {"", 0.0, 0, ASTNodePtr($2), 3}},
            {"step", {"", 0.0, 0, ASTNodePtr($4), 3}}
        }};
    }
    | LPAREN slice_with_ops COMMA slice_with_ops COMMA slice_with_ops RPAREN {
        $$ = new Operation{"bjorklund", {
            {"pulse", {"", 0.0, 0, ASTNodePtr($2), 3}},
            {"step", {"", 0.0, 0, ASTNodePtr($4), 3}},
            {"rotation", {"", 0.0, 0, ASTNodePtr($6), 3}}
        }};
    }
    ;

op_slow:
    SLASH slice {
        $$ = new Operation{"stretch", {
            {"amount", {"", 0.0, 0, ASTNodePtr($2), 3}},
            {"type", {"slow", 0.0, 0, nullptr, 0}}
        }};
    }
    ;

op_fast:
    STAR slice {
        $$ = new Operation{"stretch", {
            {"amount", {"", 0.0, 0, ASTNodePtr($2), 3}},
            {"type", {"fast", 0.0, 0, nullptr, 0}}
        }};
    }
    ;

op_degrade:
    QUESTION NUMBER { $$ = new Operation{"degradeBy", {{"amount", {"", $2, 0, nullptr, 1}}, {"seed", {"", 0.0, global_seed++, nullptr, 2}}}}; }
    | QUESTION { $$ = new Operation{"degradeBy", {{"seed", {"", 0.0, global_seed++, nullptr, 2}}}}; }
    ;

op_tail:
    COLON slice { $$ = new Operation{"tail", {{"element", {"", 0.0, 0, ASTNodePtr($2), 3}}}}; }
    ;

op_range:
    DOTDOT slice { $$ = new Operation{"range", {{"element", {"", 0.0, 0, ASTNodePtr($2), 3}}}}; }
    ;

slice:
    step { $$ = $1; }
    | sub_cycle { $$ = $1; }
    | polymeter { $$ = $1; }
    | slow_sequence { $$ = $1; }
    ;

step:
    STEP { $$ = new AtomStub(*$1); delete $1; }
    | NUMBER { $$ = new AtomStub(std::to_string($1)); }
    ;

sub_cycle:
    LBRACK stack_or_choose RBRACK { $$ = $2; }
    ;

polymeter:
    LBRACE polymeter_stack RBRACE polymeter_steps { 
        dynamic_cast<PatternStub*>($2)->has_steps = true;
        $$ = $2; 
    }
    | LBRACE polymeter_stack RBRACE { $$ = $2; }
    ;

polymeter_steps:
    PERCENT slice { $$ = $2; }
    ;

slow_sequence:
    LT polymeter_stack GT { 
        auto p = dynamic_cast<PatternStub*>($2);
        if(p) p->alignment = "polymeter_slowcat";
        $$ = $2; 
    }
    ;

%%

void yyerror(const char *s) {
    std::cerr << "Error: " << s << std::endl;
}
