#include "eval.h"
#include <cmath>
#include <sstream>

// root is defined in the TU that owns the parser entry point
extern ASTNodePtr root;


long long gcd(long long a, long long b) {
    while (b) {
        a %= b;
        std::swap(a, b);
    }
    return a;
}

void Fraction::simplify() {
    if (d < 0) {
        n = -n;
        d = -d;
    }
    long long g = gcd(std::abs(n), d);
    if (g > 1) {
        n /= g;
        d /= g;
    }
}

double get_rand(double seed, Fraction time) {
    long long n = time.n;
    long long d = time.d;
    long long s = (long long)(seed * 1000000.0);
    long long hash = (s * 31337 + n * 1337 + d * 9999 + 1234567) % 2147483647;
    if (hash < 0) hash += 2147483647;
    return (double)hash / 2147483647.0;
}

std::vector<bool> compute_bjorklund(int pulses, int steps) {
    if (steps == 0) return {};
    if (pulses == 0) return std::vector<bool>(steps, false);
    
    std::vector<std::vector<bool>> buckets(steps);
    for (int i = 0; i < steps; ++i) {
        buckets[i] = { i < pulses };
    }
    
    // a simplified euclid generation distributing pulses smoothly
    // we can just use Bresenham-like DDA or the standard offset
    std::vector<bool> result(steps, false);
    for (int i = 0; i < steps; ++i) {
        // basic euclidean rhythmic distribution formula
        result[i] = ((i * pulses) % steps) < pulses;
    }
    return result;
}

double get_amt(const ArgValue& arg) {
    if (arg.type == 1) return arg.d_val;
    if (arg.type == 2) return arg.i_val;
    if (arg.type == 3 && arg.node_val) {
        if (auto atm = std::dynamic_pointer_cast<AtomStub>(arg.node_val)) {
            try { return std::stod(atm->source); } catch(...) {}
        } else if (auto elem = std::dynamic_pointer_cast<ElementStub>(arg.node_val)) {
            if (auto atm = std::dynamic_pointer_cast<AtomStub>(elem->source)) {
                try { return std::stod(atm->source); } catch(...) {}
            }
        }
    }
    return 1.0;
}

std::vector<Event> eval_node(ASTNodePtr node, Arc arc);

std::vector<Event> eval_atom(std::shared_ptr<AtomStub> atom, Arc arc) {
    return { { arc, arc, atom->source } };
}

std::vector<Event> eval_ops(const std::vector<Operation>& ops, size_t index, ASTNodePtr source, Arc arc) {
    if (index >= ops.size()) return eval_node(source, arc);
    
    const auto& op = ops[index];
    if (op.type_ == "stretch") {
        std::string type = op.arguments.at("type").s_val;
        double amount = get_amt(op.arguments.at("amount"));
        
        if (type == "fast") {
            std::vector<Event> res;
            Fraction step = (arc.end - arc.start) / Fraction((long long)std::round(amount), 1);
            for (int i = 0; i < (int)std::round(amount); ++i) {
                Arc sub_arc;
                sub_arc.start = arc.start + step * Fraction(i, 1);
                sub_arc.end = sub_arc.start + step;
                auto evs = eval_ops(ops, index + 1, source, sub_arc);
                res.insert(res.end(), evs.begin(), evs.end());
            }
            return res;
        } else if (type == "slow") {
            // For 'slow', it expands the arc space it evaluates. 
            // In a simple fixed-arc evaluate, 'slow x' on 0->1 actually requests 0 -> 1/x from the child.
            // Let's implement slow by squeezing the child arc into an expanded space.
            Arc expanded_arc;
            expanded_arc.start = arc.start / Fraction((long long)std::round(amount), 1);
            expanded_arc.end = arc.end / Fraction((long long)std::round(amount), 1);
            
            // To be accurate we evaluate the expanded and then reshape the returned events back
            auto evs = eval_ops(ops, index + 1, source, expanded_arc);
            for (auto& ev : evs) {
                ev.active.start = ev.active.start * Fraction((long long)std::round(amount), 1);
                ev.active.end = ev.active.end * Fraction((long long)std::round(amount), 1);
                ev.part.start = ev.part.start * Fraction((long long)std::round(amount), 1);
                ev.part.end = ev.part.end * Fraction((long long)std::round(amount), 1);
            }
            return evs;
        }
    } else if (op.type_ == "replicate") {
        double amount = get_amt(op.arguments.at("amount"));
        std::vector<Event> res;
        int rep = (int)std::round(amount);
        if (rep <= 0) rep = 1;
        Fraction step = (arc.end - arc.start) / Fraction(rep, 1);
        for (int i = 0; i < rep; ++i) {
            Arc sub_arc;
            sub_arc.start = arc.start + step * Fraction(i, 1);
            sub_arc.end = sub_arc.start + step;
            auto evs = eval_ops(ops, index + 1, source, sub_arc);
            res.insert(res.end(), evs.begin(), evs.end());
        }
        return res;
    } else if (op.type_ == "bjorklund") {
        int pulses = (int)std::round(get_amt(op.arguments.at("pulse")));
        int steps = (int)std::round(get_amt(op.arguments.at("step")));
        int rotation = 0;
        if (op.arguments.count("rotation")) {
            rotation = (int)std::round(get_amt(op.arguments.at("rotation")));
        }
        
        std::vector<bool> b_pattern = compute_bjorklund(pulses, steps);
        std::vector<Event> res;
        Fraction step_size = (arc.end - arc.start) / Fraction(steps, 1);
        
        for (int i = 0; i < steps; ++i) {
            int shifted_i = (i + steps - (rotation % steps)) % steps;
            if (b_pattern[shifted_i]) {
                Arc sub_arc;
                sub_arc.start = arc.start + step_size * Fraction(i, 1);
                sub_arc.end = sub_arc.start + step_size;
                auto evs = eval_ops(ops, index + 1, source, sub_arc);
                res.insert(res.end(), evs.begin(), evs.end());
            }
        }
        return res;
    } else if (op.type_ == "degradeBy") {
        double amount = 0.5;
        if (op.arguments.count("amount")) {
            amount = op.arguments.at("amount").d_val;
        }
        double seed = op.arguments.at("seed").i_val; // seed is stored in i_val
        
        auto evs = eval_ops(ops, index + 1, source, arc);
        std::vector<Event> res;
        for (auto& ev : evs) {
            if (get_rand(seed, ev.part.start) >= amount) {
                res.push_back(ev);
            }
        }
        return res;
    } else if (op.type_ == "tail") {
        auto friend_node = op.arguments.at("element").node_val;
        auto evs = eval_ops(ops, index + 1, source, arc);
        auto friend_evs = eval_node(friend_node, arc);
        
        std::string tag = ", ";
        if (!friend_evs.empty()) {
            tag += friend_evs[0].value;
        }
        
        for (auto& ev : evs) {
            ev.value += tag;
        }
        return evs;
    } else if (op.type_ == "range") {
        // range `..` returns list interpolation. e.g. 1..4 = [1,2,3,4]
        // Actually, friend_node contains the upper bound
        double start_val = 0.0, end_val = 0.0;
        if (auto atm = std::dynamic_pointer_cast<AtomStub>(source)) {
            try { start_val = std::stod(atm->source); } catch(...) {}
        }
        auto friend_node = op.arguments.at("element").node_val;
        if (auto atm = std::dynamic_pointer_cast<AtomStub>(friend_node)) {
            try { end_val = std::stod(atm->source); } catch(...) {}
        }
        
        int steps = std::max(1, (int)std::abs(end_val - start_val) + 1);
        std::vector<Event> res;
        Fraction step_size = (arc.end - arc.start) / Fraction(steps, 1);
        
        for (int i = 0; i < steps; ++i) {
            Arc sub_arc;
            sub_arc.start = arc.start + step_size * Fraction(i, 1);
            sub_arc.end = sub_arc.start + step_size;
            
            double current_val = start_val < end_val ? start_val + i : start_val - i;
            
            Event ev;
            ev.part = sub_arc;
            ev.active = sub_arc;
            ev.value = std::to_string((int)current_val); // emit int strings for simplicity
            res.push_back(ev);
        }
        return res;
    }
    
    return eval_ops(ops, index + 1, source, arc);
}

std::vector<Event> eval_element(std::shared_ptr<ElementStub> elem, Arc arc) {
    if (elem->reps == 1) {
        return eval_ops(elem->ops, 0, elem->source, arc);
    } else {
        std::vector<Event> res;
        Fraction step = (arc.end - arc.start) / Fraction((long long)elem->reps, 1);
        for (int i = 0; i < elem->reps; ++i) {
            Arc sub_arc;
            sub_arc.start = arc.start + step * Fraction(i, 1);
            sub_arc.end = sub_arc.start + step;
            auto evs = eval_ops(elem->ops, 0, elem->source, sub_arc);
            res.insert(res.end(), evs.begin(), evs.end());
        }
        return res;
    }
}

std::vector<Event> eval_pattern(std::shared_ptr<PatternStub> pat, Arc arc) {
    std::vector<Event> res;
    
    if (pat->alignment == "fastcat" || pat->alignment == "feet") {
        double total_weight = 0;
        for (auto n : pat->list) {
            auto elem = std::dynamic_pointer_cast<ElementStub>(n);
            if (elem) {
                total_weight += elem->weight;
            } else {
                total_weight += 1.0;
            }
        }
        
        Fraction current = arc.start;
        Fraction len = arc.end - arc.start;
        
        for (auto n : pat->list) {
            double w = 1.0;
            auto elem = std::dynamic_pointer_cast<ElementStub>(n);
            if (elem) w = elem->weight;
            
            Fraction start = current;
            Fraction step = len * Fraction((long long)w, (long long)total_weight);
            Fraction end = current + step;
            
            Arc sub_arc { start, end };
            auto evs = eval_node(n, sub_arc);
            res.insert(res.end(), evs.begin(), evs.end());
            
            current = end;
        }
    } else if (pat->alignment == "polymeter_slowcat") {
        Fraction current = arc.start;
        Fraction len = arc.end - arc.start;
        
        for (auto n : pat->list) {
            double w = 1.0;
            auto elem = std::dynamic_pointer_cast<ElementStub>(n);
            if (elem) w = elem->weight;
            
            Fraction start = current;
            Fraction step = len * Fraction((long long)w, 1);
            Fraction end = current + step;
            
            Arc sub_arc { start, end };
            auto evs = eval_node(n, sub_arc);
            res.insert(res.end(), evs.begin(), evs.end());
            
            current = end;
        }
    } else if (pat->alignment == "stack") {
        for (auto n : pat->list) {
            auto evs = eval_node(n, arc);
            res.insert(res.end(), evs.begin(), evs.end());
        }
    } else if (pat->alignment == "rand") {
        if (!pat->list.empty()) {
            long long cycle = arc.start.n / arc.start.d;
            double r = get_rand(pat->seed, Fraction(cycle, 1));
            int chosen_idx = (int)(r * pat->list.size());
            if (chosen_idx >= (int)pat->list.size()) chosen_idx = pat->list.size() - 1;
            auto evs = eval_node(pat->list[chosen_idx], arc);
            res.insert(res.end(), evs.begin(), evs.end());
        }
    } else if (pat->alignment == "polymeter") {
        auto get_ast_weight = [](auto& self, ASTNodePtr node) -> double {
            if (auto e = std::dynamic_pointer_cast<ElementStub>(node)) return e->weight;
            if (auto p = std::dynamic_pointer_cast<PatternStub>(node)) {
                if (p->alignment == "fastcat" || p->alignment == "feet" || p->alignment == "polymeter_slowcat") {
                    double w = 0;
                    for (auto c : p->list) w += self(self, c);
                    return w;
                }
            }
            return 1.0;
        };
        
        double spc = 1.0;
        if (!pat->list.empty()) {
            spc = get_ast_weight(get_ast_weight, pat->list[0]);
        }
        
        for (auto n : pat->list) {
            double w = get_ast_weight(get_ast_weight, n);
            double speed = spc / w;
            Fraction f_speed((long long)std::round(speed * 1000), 1000);
            
            Arc expanded_arc;
            expanded_arc.start = arc.start * f_speed;
            expanded_arc.end = arc.end * f_speed;
            
            auto evs = eval_node(n, expanded_arc);
            for (auto& ev : evs) {
                ev.active.start = ev.active.start / f_speed;
                ev.active.end = ev.active.end / f_speed;
                ev.part.start = ev.part.start / f_speed;
                ev.part.end = ev.part.end / f_speed;
            }
            res.insert(res.end(), evs.begin(), evs.end());
        }
    }
    
    return res;
}

std::vector<Event> eval_node(ASTNodePtr node, Arc arc) {
    if (auto atom = std::dynamic_pointer_cast<AtomStub>(node)) {
        return eval_atom(atom, arc);
    } else if (auto elem = std::dynamic_pointer_cast<ElementStub>(node)) {
        return eval_element(elem, arc);
    } else if (auto pat = std::dynamic_pointer_cast<PatternStub>(node)) {
        return eval_pattern(pat, arc);
    }
    return {};
}

std::vector<Event> evaluate(ASTNodePtr root) {
    Arc initial_arc { {0, 1}, {1, 1} };
    return eval_node(root, initial_arc);
}

std::vector<Event> evaluate(ASTNodePtr root, long long cycle) {
    Arc initial_arc { {cycle, 1}, {cycle + 1, 1} };
    auto events = eval_node(root, initial_arc);
    // Normalize events back to 0->1 relative positions
    for (auto& ev : events) {
        ev.part.start  = ev.part.start  - Fraction(cycle, 1);
        ev.part.end    = ev.part.end    - Fraction(cycle, 1);
        ev.active.start = ev.active.start - Fraction(cycle, 1);
        ev.active.end   = ev.active.end   - Fraction(cycle, 1);
    }
    return events;
}

// Forward declarations from generated flex/bison code
extern int yyparse();
struct yy_buffer_state;
extern yy_buffer_state* yy_scan_string(const char* str);
extern void yy_delete_buffer(yy_buffer_state* buf);

static std::string format_events(const std::vector<Event>& events) {
    std::ostringstream oss;
    for (size_t i = 0; i < events.size(); ++i) {
        const auto& ev = events[i];
        if (i > 0) oss << "|";
        // Format: value:start_n/start_d->end_n/end_d
        oss << ev.value
            << ":" << ev.part.start.n << "/" << ev.part.start.d
            << "->" << ev.part.end.n << "/" << ev.part.end.d;
    }
    return oss.str();
}

static ASTNodePtr run_parser(const std::string& input) {
    root = nullptr;
    // The grammar's `mini` rule expects: '"' stack_or_choose '"'
    // so we wrap the caller's string in literal quote characters.
    std::string wrapped = "\"" + input + "\"";
    auto buf = yy_scan_string(wrapped.c_str());
    int result = yyparse();
    yy_delete_buffer(buf);
    if (result != 0 || !root) return nullptr;
    return root;
}

std::string parse_string(const std::string& input) {
    auto ast = run_parser(input);
    if (!ast) return "";
    auto events = evaluate(ast);
    root = nullptr;
    return format_events(events);
}

std::string parse_string(const std::string& input, long long cycle) {
    auto ast = run_parser(input);
    if (!ast) return "";
    auto events = evaluate(ast, cycle);
    root = nullptr;
    return format_events(events);
}

