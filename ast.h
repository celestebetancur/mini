#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include <variant>

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

struct AtomStub : public ASTNode {
    std::string source;
    AtomStub(std::string s) : source(std::move(s)) {}
    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "Atom(" << source << ")\n";
    }
};

struct ArgValue {
    std::string s_val;
    double d_val;
    int i_val;
    ASTNodePtr node_val;
    int type; // 0=string, 1=double, 2=int, 3=node
};

struct Operation {
    std::string type_;
    std::map<std::string, ArgValue> arguments;
};

struct ElementStub : public ASTNode {
    ASTNodePtr source;
    std::vector<Operation> ops;
    double weight = 1.0;
    int reps = 1;
    
    ElementStub(ASTNodePtr s) : source(s) {}
    
    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ElementStub(weight=" << weight << ", reps=" << reps << ")\n";
        if (source) source->print(indent + 2);
        for(const auto& o : ops) {
            std::cout << std::string(indent + 2, ' ') << "Op: " << o.type_ << "\n";
        }
    }
};

struct PatternStub : public ASTNode {
    std::string alignment;
    int seed;
    bool has_steps;
    std::vector<ASTNodePtr> list;

    PatternStub(std::string align, int sd = 0, bool steps = false) 
        : alignment(std::move(align)), seed(sd), has_steps(steps) {}
    
    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "Pattern(align=" << alignment << ", seed=" << seed << ", steps=" << has_steps << ")\n";
        for (const auto& n : list) {
            if (n) n->print(indent + 2);
        }
    }
};

struct OperatorStub : public ASTNode {
    std::string name;
    std::map<std::string, ArgValue> args;
    ASTNodePtr child;

    OperatorStub(std::string n, ASTNodePtr c = nullptr) : name(std::move(n)), child(c) {}
    
    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "Operator(" << name << ")\n";
        if (child) child->print(indent + 2);
    }
};

struct CommandStub : public ASTNode {
    std::string name;
    std::map<std::string, ArgValue> options;

    CommandStub(std::string n) : name(std::move(n)) {}
    
    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "Command(" << name << ")\n";
    }
};
