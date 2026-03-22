/*
================================================================================
Exercise 4b: Left Recursion Elimination (C++ Implementation)
================================================================================

CONCEPT:
Left recursion in a context-free grammar occurs when a non-terminal A can
derive a sentential form starting with itself: A =>+ A alpha. This is
problematic for top-down (recursive descent / LL) parsers because they would
enter infinite loops trying to expand A. Left recursion comes in two forms:

  - Direct left recursion:   A -> A alpha | beta
  - Indirect left recursion: A -> B alpha, B -> A beta

ALGORITHM:
For direct left recursion:
  Original:    A -> A alpha1 | A alpha2 | ... | beta1 | beta2 | ...
  Transformed: A  -> beta1 A' | beta2 A' | ...
               A' -> alpha1 A' | alpha2 A' | ... | epsilon

For indirect left recursion, we use the ordering algorithm:
  1. Order non-terminals A1, A2, ..., An.
  2. For i = 1 to n:
       For j = 1 to i-1:
         Substitute Aj in Ai productions.
       Eliminate direct left recursion from Ai.

COMPLEXITY:
  Time:  O(n^2 * p) where n = number of non-terminals, p = max productions
  Space: O(n * p) for storing the grammar

EXAMPLE:
  Input:   E -> E+T | T,  T -> T*F | F,  F -> (E) | id
  Output:  E  -> TE'
           E' -> +TE' | epsilon
           T  -> FT'
           T' -> *FT' | epsilon
           F  -> (E) | id
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using std::string;
using std::vector;
using std::cout;
using std::endl;

/* A production is a list of symbols (strings) */
using Production = vector<string>;

struct NonTerminal {
    string name;
    vector<Production> productions;
};

/* Grammar is an ordered list of non-terminals */
using Grammar = vector<NonTerminal>;

/* ---- Parsing and printing utilities ---- */

Production parse_production(const string &text) {
    Production prod;
    std::istringstream iss(text);
    string symbol;
    while (iss >> symbol) {
        prod.push_back(symbol);
    }
    return prod;
}

string production_to_string(const Production &prod) {
    string result;
    for (size_t i = 0; i < prod.size(); i++) {
        if (i > 0) result += " ";
        result += prod[i];
    }
    return result;
}

int find_nonterminal(const Grammar &grammar, const string &name) {
    for (size_t i = 0; i < grammar.size(); i++) {
        if (grammar[i].name == name) return (int)i;
    }
    return -1;
}

Grammar parse_grammar(const string &text) {
    Grammar grammar;
    std::istringstream iss(text);
    string line;

    while (std::getline(iss, line)) {
        /* Trim whitespace */
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;

        size_t arrow_pos = line.find("->");
        if (arrow_pos == string::npos) continue;

        string lhs = line.substr(0, arrow_pos);
        /* Trim lhs */
        lhs.erase(lhs.find_last_not_of(" \t") + 1);
        lhs.erase(0, lhs.find_first_not_of(" \t"));

        string rhs = line.substr(arrow_pos + 2);

        /* Split by | */
        vector<Production> productions;
        std::istringstream rhs_stream(rhs);
        string segment;
        /* Manual split by | since getline with '|' works */
        size_t pos = 0;
        while (pos < rhs.size()) {
            size_t pipe = rhs.find('|', pos);
            if (pipe == string::npos) pipe = rhs.size();
            string alt = rhs.substr(pos, pipe - pos);
            Production prod = parse_production(alt);
            if (!prod.empty()) {
                productions.push_back(prod);
            }
            pos = pipe + 1;
        }

        int idx = find_nonterminal(grammar, lhs);
        if (idx >= 0) {
            for (auto &p : productions)
                grammar[idx].productions.push_back(p);
        } else {
            NonTerminal nt;
            nt.name = lhs;
            nt.productions = productions;
            grammar.push_back(nt);
        }
    }
    return grammar;
}

void print_grammar(const Grammar &grammar) {
    for (const auto &nt : grammar) {
        cout << "  " << nt.name << " -> ";
        for (size_t j = 0; j < nt.productions.size(); j++) {
            if (j > 0) cout << " | ";
            cout << production_to_string(nt.productions[j]);
        }
        cout << endl;
    }
}

/* ---- Left recursion elimination ---- */

/*
 * Eliminate direct left recursion for grammar[nt_idx].
 * Returns the new non-terminal (A') to insert, or empty if no recursion found.
 */
NonTerminal eliminate_direct_left_recursion(Grammar &grammar, int nt_idx) {
    NonTerminal &nt = grammar[nt_idx];
    vector<Production> recursive_parts;     /* alpha parts */
    vector<Production> non_recursive_parts; /* beta parts */

    for (const auto &prod : nt.productions) {
        if (!prod.empty() && prod[0] == nt.name) {
            /* Left-recursive: extract alpha (everything after A) */
            Production alpha(prod.begin() + 1, prod.end());
            recursive_parts.push_back(alpha);
        } else {
            non_recursive_parts.push_back(prod);
        }
    }

    NonTerminal new_nt;
    if (recursive_parts.empty()) {
        return new_nt;  /* No left recursion; new_nt.name is empty */
    }

    string new_name = nt.name + "'";
    new_nt.name = new_name;

    /* A -> beta1 A' | beta2 A' | ... */
    nt.productions.clear();
    for (auto &beta : non_recursive_parts) {
        Production new_prod;
        if (beta.size() == 1 && beta[0] == "epsilon") {
            new_prod.push_back(new_name);
        } else {
            new_prod = beta;
            new_prod.push_back(new_name);
        }
        nt.productions.push_back(new_prod);
    }

    /* A' -> alpha1 A' | alpha2 A' | ... | epsilon */
    for (auto &alpha : recursive_parts) {
        Production new_prod = alpha;
        new_prod.push_back(new_name);
        new_nt.productions.push_back(new_prod);
    }
    new_nt.productions.push_back({"epsilon"});

    return new_nt;
}

/*
 * Substitute: replace all occurrences of grammar[idx_j].name at the start
 * of productions of grammar[idx_i] with the actual productions of idx_j.
 */
void substitute_productions(Grammar &grammar, int idx_i, int idx_j) {
    const string &name_j = grammar[idx_j].name;
    vector<Production> new_prods;

    for (const auto &prod : grammar[idx_i].productions) {
        if (!prod.empty() && prod[0] == name_j) {
            Production gamma(prod.begin() + 1, prod.end());
            for (const auto &delta : grammar[idx_j].productions) {
                Production new_prod;
                if (delta.size() == 1 && delta[0] == "epsilon") {
                    if (!gamma.empty()) {
                        new_prod = gamma;
                    } else {
                        new_prod.push_back("epsilon");
                    }
                } else {
                    new_prod = delta;
                    new_prod.insert(new_prod.end(), gamma.begin(), gamma.end());
                }
                new_prods.push_back(new_prod);
            }
        } else {
            new_prods.push_back(prod);
        }
    }

    grammar[idx_i].productions = new_prods;
}

Grammar eliminate_left_recursion(Grammar grammar) {
    /* Save original non-terminal names */
    vector<string> orig_names;
    for (const auto &nt : grammar) {
        orig_names.push_back(nt.name);
    }
    int orig_count = (int)orig_names.size();

    for (int i = 0; i < orig_count; i++) {
        for (int j = 0; j < i; j++) {
            int cur_i = find_nonterminal(grammar, orig_names[i]);
            int cur_j = find_nonterminal(grammar, orig_names[j]);
            if (cur_i >= 0 && cur_j >= 0) {
                substitute_productions(grammar, cur_i, cur_j);
            }
        }

        int cur_i = find_nonterminal(grammar, orig_names[i]);
        if (cur_i >= 0) {
            NonTerminal new_nt = eliminate_direct_left_recursion(grammar, cur_i);
            if (!new_nt.name.empty()) {
                /* Insert A' right after A */
                grammar.insert(grammar.begin() + cur_i + 1, new_nt);
            }
        }
    }

    return grammar;
}

int main() {
    cout << "======================================================================" << endl;
    cout << "  Exercise 4b: Left Recursion Elimination" << endl;
    cout << "======================================================================" << endl;

    /* --- Example 1: Expression grammar --- */
    cout << "\n--- Example 1: Direct Left Recursion ---\n" << endl;
    Grammar grammar1 = parse_grammar(
        "E -> E + T | T\n"
        "T -> T * F | F\n"
        "F -> ( E ) | id\n"
    );
    cout << "Original Grammar:" << endl;
    print_grammar(grammar1);

    Grammar result1 = eliminate_left_recursion(grammar1);
    cout << "\nAfter Left Recursion Elimination:" << endl;
    print_grammar(result1);

    /* --- Example 2: Indirect left recursion --- */
    cout << "\n----------------------------------------------------------------------" << endl;
    cout << "\n--- Example 2: Indirect Left Recursion ---\n" << endl;
    Grammar grammar2 = parse_grammar(
        "S -> A a | b\n"
        "A -> S c | d\n"
    );
    cout << "Original Grammar:" << endl;
    print_grammar(grammar2);

    Grammar result2 = eliminate_left_recursion(grammar2);
    cout << "\nAfter Left Recursion Elimination:" << endl;
    print_grammar(result2);

    /* --- Example 3: Multiple recursive alternatives --- */
    cout << "\n----------------------------------------------------------------------" << endl;
    cout << "\n--- Example 3: Multiple Recursive Alternatives ---\n" << endl;
    Grammar grammar3 = parse_grammar(
        "A -> A b c | A d | e | f g\n"
    );
    cout << "Original Grammar:" << endl;
    print_grammar(grammar3);

    Grammar result3 = eliminate_left_recursion(grammar3);
    cout << "\nAfter Left Recursion Elimination:" << endl;
    print_grammar(result3);

    cout << "\n======================================================================" << endl;
    return 0;
}
