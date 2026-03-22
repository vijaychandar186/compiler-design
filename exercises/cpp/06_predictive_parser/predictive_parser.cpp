/*
================================================================================
Exercise 6: LL(1) Predictive Parser (C++ Implementation)
================================================================================

CONCEPT:
An LL(1) predictive parser is a top-down parser that uses a parsing table to
decide which production to apply. "LL(1)" means: scan input Left-to-right,
produce a Leftmost derivation, using 1 symbol of lookahead.

ALGORITHM:
1. Compute FIRST and FOLLOW sets for the grammar.
2. Build the LL(1) parsing table M[A, a]:
   For each production A -> alpha:
     - For each terminal a in FIRST(alpha), set M[A,a] = A -> alpha.
     - If epsilon in FIRST(alpha), for each b in FOLLOW(A), set M[A,b] = A -> alpha.
3. Parse with stack: push $S, then repeatedly match terminals or expand
   non-terminals using the table until accept or error.

COMPLEXITY:
  Table construction: O(n * p * t)
  Parsing: O(input_length)
  Space: O(n * t) for the table

EXAMPLE:
  Grammar: E -> TR, R -> +TR | eps, T -> FY, Y -> *FY | eps, F -> (E) | id
  Input: id+id*id
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <sstream>
#include <iomanip>
#include <algorithm>

using std::string;
using std::vector;
using std::map;
using std::set;
using std::cout;
using std::endl;

const string EPSILON = "epsilon";
const string END_MARKER = "$";

using Production = vector<string>;

struct Grammar {
    vector<string> nonterminal_order;
    set<string> nonterminals;
    set<string> terminals;
    map<string, vector<Production>> rules;
};

/* ---- Parsing grammar definition ---- */

Grammar parse_grammar(const string &text) {
    Grammar grammar;
    std::istringstream iss(text);
    string line;
    while (std::getline(iss, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        line = line.substr(start);
        if (line.empty()) continue;
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
        size_t pos = 0;
        while (pos < rhs.size()) {
            size_t pipe = rhs.find('|', pos);
            if (pipe == string::npos) pipe = rhs.size();
            string alt = rhs.substr(pos, pipe - pos);
            Production prod;
            std::istringstream alt_s(alt);
            string sym;
            while (alt_s >> sym) prod.push_back(sym);
            if (!prod.empty()) grammar.rules[lhs].push_back(prod);
            pos = pipe + 1;
        }
    }
    for (const auto &pair : grammar.rules)
        for (const auto &prod : pair.second)
            for (const auto &sym : prod)
                if (sym != EPSILON && grammar.nonterminals.find(sym) == grammar.nonterminals.end())
                    grammar.terminals.insert(sym);
    return grammar;
}

/* ---- FIRST sets ---- */

map<string, set<string>> compute_first(const Grammar &g) {
    map<string, set<string>> first;
    for (const auto &nt : g.nonterminals) first[nt] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &nt : g.nonterminal_order) {
            for (const auto &prod : g.rules.at(nt)) {
                if (prod.size() == 1 && prod[0] == EPSILON) {
                    if (first[nt].insert(EPSILON).second) changed = true;
                    continue;
                }
                bool all_eps = true;
                for (const auto &sym : prod) {
                    if (g.nonterminals.find(sym) == g.nonterminals.end()) {
                        if (first[nt].insert(sym).second) changed = true;
                        all_eps = false;
                        break;
                    }
                    for (const auto &f : first[sym])
                        if (f != EPSILON && first[nt].insert(f).second) changed = true;
                    if (first[sym].find(EPSILON) == first[sym].end()) {
                        all_eps = false;
                        break;
                    }
                }
                if (all_eps && first[nt].insert(EPSILON).second) changed = true;
            }
        }
    }
    return first;
}

set<string> first_of_string(const Production &syms, int start,
                            const map<string, set<string>> &first,
                            const Grammar &g) {
    set<string> result;
    bool all_eps = true;
    for (int i = start; i < (int)syms.size(); i++) {
        const string &sym = syms[i];
        if (g.nonterminals.find(sym) == g.nonterminals.end()) {
            result.insert(sym);
            all_eps = false;
            break;
        }
        for (const auto &f : first.at(sym))
            if (f != EPSILON) result.insert(f);
        if (first.at(sym).find(EPSILON) == first.at(sym).end()) {
            all_eps = false;
            break;
        }
    }
    if (all_eps) result.insert(EPSILON);
    return result;
}

/* ---- FOLLOW sets ---- */

map<string, set<string>> compute_follow(const Grammar &g,
                                         const map<string, set<string>> &first) {
    map<string, set<string>> follow;
    for (const auto &nt : g.nonterminals) follow[nt] = {};
    follow[g.nonterminal_order[0]].insert(END_MARKER);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &lhs : g.nonterminal_order) {
            for (const auto &prod : g.rules.at(lhs)) {
                if (prod.size() == 1 && prod[0] == EPSILON) continue;
                for (int i = 0; i < (int)prod.size(); i++) {
                    const string &sym = prod[i];
                    if (g.nonterminals.find(sym) == g.nonterminals.end()) continue;
                    if (i + 1 < (int)prod.size()) {
                        auto fb = first_of_string(prod, i + 1, first, g);
                        for (const auto &f : fb)
                            if (f != EPSILON && follow[sym].insert(f).second) changed = true;
                        if (fb.find(EPSILON) != fb.end())
                            for (const auto &f : follow[lhs])
                                if (follow[sym].insert(f).second) changed = true;
                    } else {
                        for (const auto &f : follow[lhs])
                            if (follow[sym].insert(f).second) changed = true;
                    }
                }
            }
        }
    }
    return follow;
}

/* ---- Parsing table ---- */

using TableKey = std::pair<string, string>;
map<TableKey, Production> build_table(const Grammar &g,
                                       const map<string, set<string>> &first,
                                       const map<string, set<string>> &follow) {
    map<TableKey, Production> table;
    for (const auto &nt : g.nonterminal_order) {
        for (const auto &prod : g.rules.at(nt)) {
            set<string> fa;
            if (prod.size() == 1 && prod[0] == EPSILON)
                fa.insert(EPSILON);
            else
                fa = first_of_string(prod, 0, first, g);

            for (const auto &a : fa)
                if (a != EPSILON)
                    table[{nt, a}] = prod;

            if (fa.find(EPSILON) != fa.end())
                for (const auto &b : follow.at(nt))
                    table[{nt, b}] = prod;
        }
    }
    return table;
}

void print_table(const map<TableKey, Production> &table,
                 const Grammar &g) {
    vector<string> cols(g.terminals.begin(), g.terminals.end());
    std::sort(cols.begin(), cols.end());
    cols.push_back(END_MARKER);
    int cw = 16;

    cout << "\n  " << std::setw(8) << "" << " |";
    for (const auto &t : cols)
        cout << " " << std::setw(cw) << t << " |";
    cout << endl;
    cout << "  " << string(8, '-') << "-+";
    for (size_t i = 0; i < cols.size(); i++)
        cout << "-" << string(cw, '-') << "-+";
    cout << endl;

    for (const auto &nt : g.nonterminal_order) {
        cout << "  " << std::setw(8) << nt << " |";
        for (const auto &t : cols) {
            auto it = table.find({nt, t});
            if (it != table.end()) {
                string entry = nt + " -> ";
                for (size_t k = 0; k < it->second.size(); k++) {
                    if (k > 0) entry += " ";
                    entry += it->second[k];
                }
                cout << " " << std::setw(cw) << entry << " |";
            } else {
                cout << " " << std::setw(cw) << "" << " |";
            }
        }
        cout << endl;
    }
}

/* ---- Tokenizer ---- */

vector<string> tokenize(const string &input) {
    vector<string> tokens;
    size_t i = 0;
    while (i < input.size()) {
        if (isspace(input[i])) { i++; continue; }
        if (input.substr(i, 2) == "id") { tokens.push_back("id"); i += 2; }
        else { tokens.push_back(string(1, input[i])); i++; }
    }
    tokens.push_back(END_MARKER);
    return tokens;
}

/* ---- Parser ---- */

bool parse_input(const vector<string> &tokens,
                 const map<TableKey, Production> &table,
                 const Grammar &g) {
    vector<string> parse_stack;
    parse_stack.push_back(END_MARKER);
    parse_stack.push_back(g.nonterminal_order[0]);

    int pos = 0;
    int step = 0;

    cout << "\n  " << std::left << std::setw(6) << "Step"
         << std::setw(30) << "Stack"
         << std::setw(25) << "Input"
         << "Action" << endl;
    cout << "  " << string(6, '-') << " " << string(29, '-') << " "
         << string(24, '-') << " " << string(30, '-') << endl;

    while (true) {
        string top = parse_stack.back();
        const string &current = tokens[pos];
        step++;

        /* Format stack */
        string stack_str;
        for (const auto &s : parse_stack) stack_str += s + " ";

        /* Format remaining input */
        string input_str;
        for (int i = pos; i < (int)tokens.size(); i++) {
            if (i > pos) input_str += " ";
            input_str += tokens[i];
        }

        if (top == END_MARKER && current == END_MARKER) {
            cout << "  " << std::left << std::setw(6) << step
                 << std::setw(30) << stack_str
                 << std::setw(25) << input_str
                 << "ACCEPT" << endl;
            cout << "\n  ** Input successfully parsed! **" << endl;
            return true;
        }

        if (top == current) {
            string action = "Match '" + top + "'";
            cout << "  " << std::left << std::setw(6) << step
                 << std::setw(30) << stack_str
                 << std::setw(25) << input_str
                 << action << endl;
            parse_stack.pop_back();
            pos++;
        } else if (g.nonterminals.find(top) != g.nonterminals.end()) {
            auto it = table.find({top, current});
            if (it != table.end()) {
                string action = top + " -> ";
                for (size_t k = 0; k < it->second.size(); k++) {
                    if (k > 0) action += " ";
                    action += it->second[k];
                }
                cout << "  " << std::left << std::setw(6) << step
                     << std::setw(30) << stack_str
                     << std::setw(25) << input_str
                     << action << endl;
                parse_stack.pop_back();
                if (!(it->second.size() == 1 && it->second[0] == EPSILON)) {
                    for (int k = (int)it->second.size() - 1; k >= 0; k--)
                        parse_stack.push_back(it->second[k]);
                }
            } else {
                cout << "  " << std::left << std::setw(6) << step
                     << std::setw(30) << stack_str
                     << std::setw(25) << input_str
                     << "ERROR: no table entry for [" << top << ", " << current << "]" << endl;
                cout << "\n  ** Parse error **" << endl;
                return false;
            }
        } else {
            cout << "  " << std::left << std::setw(6) << step
                 << std::setw(30) << stack_str
                 << std::setw(25) << input_str
                 << "ERROR: mismatch" << endl;
            cout << "\n  ** Parse error **" << endl;
            return false;
        }
    }
}

int main() {
    cout << "==========================================================================" << endl;
    cout << "  Exercise 6: LL(1) Predictive Parser" << endl;
    cout << "==========================================================================" << endl;

    Grammar grammar = parse_grammar(
        "E -> T R\n"
        "R -> + T R | epsilon\n"
        "T -> F Y\n"
        "Y -> * F Y | epsilon\n"
        "F -> ( E ) | id\n"
    );

    cout << "\nGrammar:" << endl;
    for (const auto &nt : grammar.nonterminal_order) {
        cout << "  " << nt << " -> ";
        const auto &prods = grammar.rules[nt];
        for (size_t i = 0; i < prods.size(); i++) {
            if (i > 0) cout << " | ";
            for (size_t j = 0; j < prods[i].size(); j++) {
                if (j > 0) cout << " ";
                cout << prods[i][j];
            }
        }
        cout << endl;
    }

    auto first_sets = compute_first(grammar);
    auto follow_sets = compute_follow(grammar, first_sets);

    cout << "\nFIRST Sets:" << endl;
    for (const auto &nt : grammar.nonterminal_order) {
        cout << "  FIRST(" << nt << ") = { ";
        bool f = true;
        for (const auto &s : first_sets[nt]) { if (!f) cout << ", "; cout << s; f = false; }
        cout << " }" << endl;
    }

    cout << "\nFOLLOW Sets:" << endl;
    for (const auto &nt : grammar.nonterminal_order) {
        cout << "  FOLLOW(" << nt << ") = { ";
        bool f = true;
        for (const auto &s : follow_sets[nt]) { if (!f) cout << ", "; cout << s; f = false; }
        cout << " }" << endl;
    }

    auto table = build_table(grammar, first_sets, follow_sets);
    cout << "\nLL(1) Parsing Table:";
    print_table(table, grammar);

    /* Parse test inputs */
    vector<string> test_inputs = {"id+id*id", "id*(id+id)", "id+id"};
    for (const auto &input : test_inputs) {
        cout << "\n==========================================================================" << endl;
        cout << "\n  Parsing: \"" << input << "\"" << endl;
        auto tokens = tokenize(input);
        parse_input(tokens, table, grammar);
    }

    cout << "\n==========================================================================" << endl;
    return 0;
}
