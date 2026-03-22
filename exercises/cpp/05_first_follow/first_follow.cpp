/*
================================================================================
Exercise 5: FIRST and FOLLOW Sets Computation (C++ Implementation)
================================================================================

CONCEPT:
FIRST and FOLLOW sets are fundamental to constructing predictive parsers (LL(1)).

FIRST(X) is the set of terminals that can appear as the first symbol of any
string derived from X. If X can derive epsilon, then epsilon is also in FIRST(X).

FOLLOW(A) is the set of terminals that can appear immediately after A in some
sentential form. For the start symbol, $ (end of input) is always in FOLLOW.

ALGORITHM (iterative fixed-point):
FIRST:
  1. If X is a terminal, FIRST(X) = {X}.
  2. If X -> epsilon, add epsilon to FIRST(X).
  3. If X -> Y1 Y2 ... Yk, add FIRST(Y1)-{eps} to FIRST(X), and if Y1 =>* eps,
     add FIRST(Y2)-{eps}, etc. If all derive eps, add eps.
  Repeat until no changes.

FOLLOW:
  1. Add $ to FOLLOW(S).
  2. For A -> alpha B beta: add FIRST(beta)-{eps} to FOLLOW(B).
     If eps in FIRST(beta) or beta is empty, add FOLLOW(A) to FOLLOW(B).
  Repeat until no changes.

COMPLEXITY:
  Time:  O(n * p * k) per iteration, O(n) iterations worst case
  Space: O(n * t) where n = non-terminals, t = terminals

EXAMPLE:
  Grammar: E -> T R, R -> + T R | epsilon, T -> F Y, Y -> * F Y | epsilon,
           F -> ( E ) | id
  FIRST(E) = { (, id }     FOLLOW(E) = { $, ) }
  FIRST(R) = { +, eps }    FOLLOW(R) = { $, ) }
  FIRST(T) = { (, id }     FOLLOW(T) = { +, $, ) }
  FIRST(Y) = { *, eps }    FOLLOW(Y) = { +, $, ) }
  FIRST(F) = { (, id }     FOLLOW(F) = { *, +, $, ) }
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <iomanip>
#include <algorithm>

using std::string;
using std::vector;
using std::set;
using std::map;
using std::cout;
using std::endl;

const string EPSILON = "epsilon";
const string END_MARKER = "$";

using Production = vector<string>;

struct Grammar {
    vector<string> nonterminal_order;   /* preserves insertion order */
    set<string> nonterminals;
    set<string> terminals;
    map<string, vector<Production>> rules;
};

/* ---- Parsing ---- */

Grammar parse_grammar(const string &text) {
    Grammar grammar;
    std::istringstream iss(text);
    string line;

    while (std::getline(iss, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;

        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;

        string lhs = line.substr(0, arrow);
        lhs.erase(lhs.find_last_not_of(" \t") + 1);
        lhs.erase(0, lhs.find_first_not_of(" \t"));

        string rhs = line.substr(arrow + 2);

        if (grammar.nonterminals.find(lhs) == grammar.nonterminals.end()) {
            grammar.nonterminals.insert(lhs);
            grammar.nonterminal_order.push_back(lhs);
        }

        /* Split by | */
        size_t pos = 0;
        while (pos < rhs.size()) {
            size_t pipe = rhs.find('|', pos);
            if (pipe == string::npos) pipe = rhs.size();
            string alt = rhs.substr(pos, pipe - pos);
            Production prod;
            std::istringstream alt_stream(alt);
            string sym;
            while (alt_stream >> sym) prod.push_back(sym);
            if (!prod.empty()) grammar.rules[lhs].push_back(prod);
            pos = pipe + 1;
        }
    }

    /* Collect terminals */
    for (const auto &pair : grammar.rules) {
        for (const auto &prod : pair.second) {
            for (const auto &sym : prod) {
                if (sym != EPSILON && grammar.nonterminals.find(sym) == grammar.nonterminals.end()) {
                    grammar.terminals.insert(sym);
                }
            }
        }
    }

    return grammar;
}

void print_grammar(const Grammar &grammar) {
    for (const auto &nt : grammar.nonterminal_order) {
        cout << "  " << nt << " -> ";
        const auto &prods = grammar.rules.at(nt);
        for (size_t i = 0; i < prods.size(); i++) {
            if (i > 0) cout << " | ";
            for (size_t j = 0; j < prods[i].size(); j++) {
                if (j > 0) cout << " ";
                cout << prods[i][j];
            }
        }
        cout << endl;
    }
}

/* ---- FIRST computation ---- */

map<string, set<string>> compute_first(const Grammar &grammar) {
    map<string, set<string>> first;
    for (const auto &nt : grammar.nonterminals) {
        first[nt] = set<string>();
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &nt : grammar.nonterminal_order) {
            for (const auto &prod : grammar.rules.at(nt)) {
                /* Epsilon production */
                if (prod.size() == 1 && prod[0] == EPSILON) {
                    if (first[nt].insert(EPSILON).second) changed = true;
                    continue;
                }

                bool all_have_epsilon = true;
                for (const auto &sym : prod) {
                    if (grammar.nonterminals.find(sym) == grammar.nonterminals.end()) {
                        /* Terminal */
                        if (first[nt].insert(sym).second) changed = true;
                        all_have_epsilon = false;
                        break;
                    } else {
                        /* Non-terminal: add FIRST(sym) - {epsilon} */
                        for (const auto &f : first[sym]) {
                            if (f != EPSILON) {
                                if (first[nt].insert(f).second) changed = true;
                            }
                        }
                        if (first[sym].find(EPSILON) == first[sym].end()) {
                            all_have_epsilon = false;
                            break;
                        }
                    }
                }
                if (all_have_epsilon) {
                    if (first[nt].insert(EPSILON).second) changed = true;
                }
            }
        }
    }
    return first;
}

/* Compute FIRST of a string of symbols */
set<string> first_of_string(const vector<string> &symbols, int start,
                            const map<string, set<string>> &first,
                            const Grammar &grammar) {
    set<string> result;
    bool all_have_epsilon = true;

    for (int i = start; i < (int)symbols.size(); i++) {
        const string &sym = symbols[i];
        if (grammar.nonterminals.find(sym) == grammar.nonterminals.end()) {
            result.insert(sym);
            all_have_epsilon = false;
            break;
        } else {
            for (const auto &f : first.at(sym)) {
                if (f != EPSILON) result.insert(f);
            }
            if (first.at(sym).find(EPSILON) == first.at(sym).end()) {
                all_have_epsilon = false;
                break;
            }
        }
    }
    if (all_have_epsilon) result.insert(EPSILON);
    return result;
}

/* ---- FOLLOW computation ---- */

map<string, set<string>> compute_follow(const Grammar &grammar,
                                         const map<string, set<string>> &first) {
    map<string, set<string>> follow;
    for (const auto &nt : grammar.nonterminals) {
        follow[nt] = set<string>();
    }

    /* Add $ to FOLLOW of start symbol */
    follow[grammar.nonterminal_order[0]].insert(END_MARKER);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &lhs : grammar.nonterminal_order) {
            for (const auto &prod : grammar.rules.at(lhs)) {
                if (prod.size() == 1 && prod[0] == EPSILON) continue;

                for (int i = 0; i < (int)prod.size(); i++) {
                    const string &sym = prod[i];
                    if (grammar.nonterminals.find(sym) == grammar.nonterminals.end())
                        continue;

                    if (i + 1 < (int)prod.size()) {
                        set<string> first_beta = first_of_string(prod, i + 1, first, grammar);
                        for (const auto &f : first_beta) {
                            if (f != EPSILON) {
                                if (follow[sym].insert(f).second) changed = true;
                            }
                        }
                        if (first_beta.find(EPSILON) != first_beta.end()) {
                            for (const auto &f : follow[lhs]) {
                                if (follow[sym].insert(f).second) changed = true;
                            }
                        }
                    } else {
                        for (const auto &f : follow[lhs]) {
                            if (follow[sym].insert(f).second) changed = true;
                        }
                    }
                }
            }
        }
    }
    return follow;
}

/* ---- Printing ---- */

string set_to_string(const set<string> &s) {
    string result = "{ ";
    bool first_elem = true;
    for (const auto &elem : s) {
        if (!first_elem) result += ", ";
        result += elem;
        first_elem = false;
    }
    result += " }";
    return result;
}

void print_sets(const Grammar &grammar,
                const map<string, set<string>> &first,
                const map<string, set<string>> &follow) {
    cout << "\n  " << std::left << std::setw(14) << "Non-Terminal"
         << " |  " << std::setw(28) << "FIRST"
         << " |  " << "FOLLOW" << endl;
    cout << "  " << string(14, '-') << "-+--" << string(28, '-') << "-+--"
         << string(28, '-') << endl;

    for (const auto &nt : grammar.nonterminal_order) {
        cout << "  " << std::left << std::setw(14) << nt
             << " |  " << std::setw(28) << set_to_string(first.at(nt))
             << " |  " << set_to_string(follow.at(nt)) << endl;
    }
}

int main() {
    cout << "======================================================================" << endl;
    cout << "  Exercise 5: FIRST and FOLLOW Sets" << endl;
    cout << "======================================================================" << endl;

    /* --- Example 1: Expression grammar --- */
    cout << "\n--- Example 1: Expression Grammar ---\n" << endl;
    Grammar grammar1 = parse_grammar(
        "E -> T R\n"
        "R -> + T R | epsilon\n"
        "T -> F Y\n"
        "Y -> * F Y | epsilon\n"
        "F -> ( E ) | id\n"
    );
    cout << "Grammar:" << endl;
    print_grammar(grammar1);

    auto first1 = compute_first(grammar1);
    auto follow1 = compute_follow(grammar1, first1);
    print_sets(grammar1, first1, follow1);

    /* --- Example 2: Grammar with multiple epsilon productions --- */
    cout << "\n----------------------------------------------------------------------" << endl;
    cout << "\n--- Example 2: Multiple Epsilon Productions ---\n" << endl;
    Grammar grammar2 = parse_grammar(
        "S -> a B D h\n"
        "B -> c C\n"
        "C -> b C | epsilon\n"
        "D -> E F\n"
        "E -> g | epsilon\n"
        "F -> f | epsilon\n"
    );
    cout << "Grammar:" << endl;
    print_grammar(grammar2);

    auto first2 = compute_first(grammar2);
    auto follow2 = compute_follow(grammar2, first2);
    print_sets(grammar2, first2, follow2);

    /* --- Example 3: Two nullable non-terminals --- */
    cout << "\n----------------------------------------------------------------------" << endl;
    cout << "\n--- Example 3: Two Nullable Non-Terminals ---\n" << endl;
    Grammar grammar3 = parse_grammar(
        "S -> A B\n"
        "A -> a | epsilon\n"
        "B -> b | epsilon\n"
    );
    cout << "Grammar:" << endl;
    print_grammar(grammar3);

    auto first3 = compute_first(grammar3);
    auto follow3 = compute_follow(grammar3, first3);
    print_sets(grammar3, first3, follow3);

    cout << "\n======================================================================" << endl;
    return 0;
}
