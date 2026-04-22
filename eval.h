#pragma once

#include "ast.h"
#include <vector>
#include <string>
#include <iostream>

struct Fraction {
    long long n, d;

    Fraction(long long n = 0, long long d = 1) : n(n), d(d) { simplify(); }

    void simplify();

    Fraction operator+(const Fraction& o) const {
        return Fraction(n * o.d + o.n * d, d * o.d);
    }
    Fraction operator-(const Fraction& o) const {
        return Fraction(n * o.d - o.n * d, d * o.d);
    }
    Fraction operator*(const Fraction& o) const {
        return Fraction(n * o.n, d * o.d);
    }
    Fraction operator/(const Fraction& o) const {
        return Fraction(n * o.d, d * o.n);
    }
    bool operator==(const Fraction& o) const {
        return n == o.n && d == o.d;
    }
};

struct Arc {
    Fraction start;
    Fraction end;
};

struct Event {
    Arc part;
    Arc active;
    std::string value;
};

std::vector<Event> evaluate(ASTNodePtr root);
std::vector<Event> evaluate(ASTNodePtr root, long long cycle);
std::string parse_string(const std::string& input);
std::string parse_string(const std::string& input, long long cycle);
