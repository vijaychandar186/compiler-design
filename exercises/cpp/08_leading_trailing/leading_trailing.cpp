/*
 * ==============================================================================
 * Exercise 8: LEADING and TRAILING Sets for Operator Grammars
 * ==============================================================================
 *
 * CONCEPT:
 * In operator precedence parsing, LEADING and TRAILING sets determine the
 * precedence relations between terminals. An operator grammar has no two
 * adjacent non-terminals in any production right-hand side.
 *
 * LEADING(A) is the set of terminals that can appear as the first terminal
 * in a string derived from non-terminal A (possibly preceded by non-terminals).
 * TRAILING(A) is the set of terminals that can appear as the last terminal
 * in a string derived from A (possibly followed by non-terminals).
 *
 * ALGORITHM:
 * Iterative fixed-point computation:
 *   1. For each production A -> X1 X2 ..., scan from the left. Add the first
 *      terminal found to LEADING(A). If a non-terminal B appears before that
 *      terminal, mark that LEADING(B) should propagate to LEADING(A).
 *   2. Repeat propagation until no set changes (fixed-point).
 *   3. Analogous process scanning from the right for TRAILING.
 *
 * TIME COMPLEXITY:  O(|P| * |V|) per iteration; converges in O(|N|) iterations.
 * SPACE COMPLEXITY: O(|N| * |T|) for the sets.
 *
 * EXAMPLE INPUT:
 *   E -> E + T | T
 *   T -> T * F | F
 *   F -> ( E ) | id
 *
 * EXAMPLE OUTPUT:
 *   LEADING(E) = { (, *, +, id }
 *   TRAILING(E) = { ), *, +, id }
 * ==============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <algorithm>

/* A production: lhs -> rhs_symbols */
struct Production {
    std::string lhs;
    std::vector<std::string> rhs;
};

class LeadingTrailingCalculator {
private:
    std::vector<Production> productions_;
    std::set<std::string> non_terminals_;
    std::set<std::string> terminals_;
    std::vector<std::string> nt_order_; /* Insertion-ordered non-terminals */

    std::map<std::string, std::set<std::string>> leading_;
    std::map<std::string, std::set<std::string>> trailing_;

    bool is_non_terminal(const std::string &sym) const {
        return non_terminals_.count(sym) > 0;
    }

public:
    void add_non_terminal(const std::string &nt) {
        if (non_terminals_.insert(nt).second) {
            nt_order_.push_back(nt);
        }
    }

    void add_production(const std::string &lhs,
                        const std::vector<std::string> &rhs) {
        add_non_terminal(lhs);
        productions_.push_back({lhs, rhs});
    }

    void identify_terminals() {
        for (const auto &prod : productions_) {
            for (const auto &sym : prod.rhs) {
                if (!is_non_terminal(sym)) {
                    terminals_.insert(sym);
                }
            }
        }
    }

    void compute_leading() {
        /* Initialize empty sets */
        for (const auto &nt : non_terminals_) {
            leading_[nt] = {};
        }

        /* Initial pass: direct terminals from left scan */
        for (const auto &prod : productions_) {
            for (const auto &sym : prod.rhs) {
                if (terminals_.count(sym)) {
                    leading_[prod.lhs].insert(sym);
                    break;
                } else if (is_non_terminal(sym)) {
                    continue; /* Skip leading non-terminals */
                } else {
                    break;
                }
            }
        }

        /* Fixed-point propagation */
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto &prod : productions_) {
                for (const auto &sym : prod.rhs) {
                    if (is_non_terminal(sym)) {
                        size_t before = leading_[prod.lhs].size();
                        leading_[prod.lhs].insert(
                            leading_[sym].begin(), leading_[sym].end());
                        if (leading_[prod.lhs].size() > before) {
                            changed = true;
                        }
                    } else {
                        break; /* Terminal reached */
                    }
                }
            }
        }
    }

    void compute_trailing() {
        for (const auto &nt : non_terminals_) {
            trailing_[nt] = {};
        }

        /* Initial pass: direct terminals from right scan */
        for (const auto &prod : productions_) {
            for (int i = static_cast<int>(prod.rhs.size()) - 1; i >= 0; i--) {
                const std::string &sym = prod.rhs[i];
                if (terminals_.count(sym)) {
                    trailing_[prod.lhs].insert(sym);
                    break;
                } else if (is_non_terminal(sym)) {
                    continue;
                } else {
                    break;
                }
            }
        }

        /* Fixed-point propagation */
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto &prod : productions_) {
                for (int i = static_cast<int>(prod.rhs.size()) - 1; i >= 0; i--) {
                    const std::string &sym = prod.rhs[i];
                    if (is_non_terminal(sym)) {
                        size_t before = trailing_[prod.lhs].size();
                        trailing_[prod.lhs].insert(
                            trailing_[sym].begin(), trailing_[sym].end());
                        if (trailing_[prod.lhs].size() > before) {
                            changed = true;
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    }

    void print_grammar() const {
        std::cout << "\n==================================================\n";
        std::cout << "  Grammar Productions\n";
        std::cout << "==================================================\n";

        /* Group productions by LHS for pretty printing */
        for (const auto &nt : nt_order_) {
            bool first_alt = true;
            std::cout << "  " << nt << " -> ";
            for (const auto &prod : productions_) {
                if (prod.lhs != nt) continue;
                if (!first_alt) std::cout << " | ";
                first_alt = false;
                for (size_t i = 0; i < prod.rhs.size(); i++) {
                    if (i > 0) std::cout << " ";
                    std::cout << prod.rhs[i];
                }
            }
            std::cout << "\n";
        }
        std::cout << "==================================================\n";
    }

    void print_symbols() const {
        std::cout << "\n  Non-terminals: { ";
        bool first = true;
        for (const auto &nt : nt_order_) {
            if (!first) std::cout << ", ";
            std::cout << nt;
            first = false;
        }
        std::cout << " }\n";

        std::cout << "  Terminals:     { ";
        first = true;
        for (const auto &t : terminals_) {
            if (!first) std::cout << ", ";
            std::cout << t;
            first = false;
        }
        std::cout << " }\n";
    }

    void print_sets(const std::string &name,
                    const std::map<std::string, std::set<std::string>> &sets) const {
        std::cout << "\n==================================================\n";
        std::cout << "  " << name << " Sets\n";
        std::cout << "==================================================\n";

        for (const auto &nt : nt_order_) {
            auto it = sets.find(nt);
            if (it == sets.end()) continue;

            std::cout << "  " << name << "(" << nt << ") = { ";
            bool first = true;
            for (const auto &t : it->second) {
                if (!first) std::cout << ", ";
                std::cout << t;
                first = false;
            }
            std::cout << " }\n";
        }
        std::cout << "==================================================\n";
    }

    void print_results() const {
        print_sets("LEADING", leading_);
        print_sets("TRAILING", trailing_);
    }
};

int main() {
    std::cout << "==================================================\n";
    std::cout << "  LEADING and TRAILING Sets Calculator\n";
    std::cout << "  (Operator Precedence Parsing)\n";
    std::cout << "==================================================\n";

    LeadingTrailingCalculator calc;

    /* Register non-terminals first to control ordering */
    calc.add_non_terminal("E");
    calc.add_non_terminal("T");
    calc.add_non_terminal("F");

    /* Grammar: E -> E+T | T, T -> T*F | F, F -> (E) | id */
    calc.add_production("E", {"E", "+", "T"});
    calc.add_production("E", {"T"});
    calc.add_production("T", {"T", "*", "F"});
    calc.add_production("T", {"F"});
    calc.add_production("F", {"(", "E", ")"});
    calc.add_production("F", {"id"});

    calc.identify_terminals();
    calc.print_grammar();
    calc.print_symbols();

    calc.compute_leading();
    calc.compute_trailing();
    calc.print_results();

    /* Second example */
    std::cout << "\n\n##################################################\n";
    std::cout << "  Second Example Grammar\n";
    std::cout << "##################################################\n";

    LeadingTrailingCalculator calc2;
    calc2.add_non_terminal("S");
    calc2.add_non_terminal("A");
    calc2.add_non_terminal("B");

    calc2.add_production("S", {"S", "+", "A"});
    calc2.add_production("S", {"A"});
    calc2.add_production("A", {"A", "*", "B"});
    calc2.add_production("A", {"B"});
    calc2.add_production("B", {"(", "S", ")"});
    calc2.add_production("B", {"a"});

    calc2.identify_terminals();
    calc2.print_grammar();
    calc2.print_symbols();

    calc2.compute_leading();
    calc2.compute_trailing();
    calc2.print_results();

    return 0;
}
