/*
================================================================================
Exercise 3: NFA to DFA Conversion (Subset Construction) -- C++ Implementation
================================================================================

CONCEPT:
    The subset construction (powerset construction) converts an NFA into an
    equivalent DFA.  Each DFA state corresponds to a set of NFA states.  The
    algorithm systematically explores all reachable subsets, computing
    epsilon-closures and move sets to build the DFA transition table.

ALGORITHM:
    1. Compute epsilon-closure({start}).  This becomes DFA state D0.
    2. While there are unmarked DFA states:
       a. Pick an unmarked state D.
       b. For each symbol 'a' in the alphabet:
          - Compute move(D, a) and then epsilon-closure of that.
          - If the result is a new set, add it as a DFA state.
          - Record the DFA transition.
    3. A DFA state is accepting if it contains any NFA accept state.

COMPLEXITY:
    Time:  O(2^n * m) worst case  (n = NFA states, m = alphabet size).
    Space: O(2^n) for DFA states.

EXAMPLE:
    NFA for (a|b)*abb  ->  DFA with 5 states.
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <sstream>

/* ---- constants ---------------------------------------------------------- */
static const std::string EPS = "eps";

/* ---- NFA ---------------------------------------------------------------- */
struct NFATransition {
    int         from;
    std::string symbol;
    int         to;
};

class NFA {
public:
    int                         num_states;
    int                         start;
    std::set<int>               accept_states;
    std::set<std::string>       alphabet;   /* excludes epsilon */
    /* transitions indexed by (state, symbol) -> set of target states */
    std::map<std::pair<int,std::string>, std::set<int>> trans;

    NFA() : num_states(0), start(0) {}

    void add_transition(int from, const std::string &symbol, int to) {
        trans[{from, symbol}].insert(to);
        if (symbol != EPS) alphabet.insert(symbol);
        int mx = std::max(from, to) + 1;
        if (mx > num_states) num_states = mx;
    }

    std::set<int> epsilon_closure(const std::set<int> &states) const {
        std::set<int> closure = states;
        std::queue<int> worklist;
        for (int s : states) worklist.push(s);
        while (!worklist.empty()) {
            int curr = worklist.front(); worklist.pop();
            auto it = trans.find({curr, EPS});
            if (it == trans.end()) continue;
            for (int t : it->second) {
                if (closure.insert(t).second)
                    worklist.push(t);
            }
        }
        return closure;
    }

    std::set<int> move(const std::set<int> &states, const std::string &symbol) const {
        std::set<int> result;
        for (int s : states) {
            auto it = trans.find({s, symbol});
            if (it != trans.end())
                result.insert(it->second.begin(), it->second.end());
        }
        return result;
    }
};

/* ---- DFA ---------------------------------------------------------------- */
struct DFAState {
    int                 id;
    std::set<int>       nfa_states;
    bool                is_accept;
    std::map<std::string, int> transitions;
};

class DFA {
public:
    std::vector<DFAState>      states;
    int                        start;
    std::set<std::string>      alphabet;
};

/* ---- subset construction ------------------------------------------------ */
DFA subset_construction(const NFA &nfa) {
    DFA dfa;
    dfa.alphabet = nfa.alphabet;

    std::map<std::set<int>, int> state_map;
    int id_counter = 0;

    /* start state */
    std::set<int> start_set = nfa.epsilon_closure({nfa.start});
    state_map[start_set] = id_counter;
    {
        DFAState ds;
        ds.id = id_counter++;
        ds.nfa_states = start_set;
        ds.is_accept = false;
        for (int s : start_set)
            if (nfa.accept_states.count(s)) { ds.is_accept = true; break; }
        dfa.states.push_back(ds);
    }
    dfa.start = 0;

    std::queue<std::set<int>> worklist;
    worklist.push(start_set);

    while (!worklist.empty()) {
        std::set<int> current = worklist.front(); worklist.pop();
        int curr_id = state_map[current];

        for (auto &sym : dfa.alphabet) {
            std::set<int> moved = nfa.move(current, sym);
            if (moved.empty()) continue;
            std::set<int> closure = nfa.epsilon_closure(moved);
            if (closure.empty()) continue;

            if (state_map.find(closure) == state_map.end()) {
                state_map[closure] = id_counter;
                DFAState ds;
                ds.id = id_counter++;
                ds.nfa_states = closure;
                ds.is_accept = false;
                for (int s : closure)
                    if (nfa.accept_states.count(s)) { ds.is_accept = true; break; }
                dfa.states.push_back(ds);
                worklist.push(closure);
            }
            dfa.states[curr_id].transitions[sym] = state_map[closure];
        }
    }

    return dfa;
}

/* ---- printing ----------------------------------------------------------- */
static std::string set_to_string(const std::set<int> &s) {
    std::string result = "{";
    bool first = true;
    for (int v : s) {
        if (!first) result += ", ";
        result += "q" + std::to_string(v);
        first = false;
    }
    result += "}";
    return result;
}

static void print_nfa(const NFA &nfa) {
    std::vector<std::string> symbols = {EPS};
    for (auto &s : nfa.alphabet) symbols.push_back(s);

    int col_w = 18;
    std::cout << std::left << std::setw(10) << "STATE";
    for (auto &sym : symbols)
        std::cout << "| " << std::setw(col_w) << sym;
    std::cout << "\n";

    int total_w = 10 + static_cast<int>(symbols.size()) * (col_w + 2);
    std::cout << std::string(total_w, '-') << "\n";

    for (int st = 0; st < nfa.num_states; st++) {
        std::string prefix;
        if (st == nfa.start) prefix += "->";
        if (nfa.accept_states.count(st)) prefix += "*";
        std::string label = prefix + "q" + std::to_string(st);
        std::cout << std::left << std::setw(10) << label;

        for (auto &sym : symbols) {
            auto it = nfa.trans.find({st, sym});
            std::string cell = "-";
            if (it != nfa.trans.end() && !it->second.empty()) {
                cell = "";
                bool first = true;
                for (int t : it->second) {
                    if (!first) cell += ", ";
                    cell += "q" + std::to_string(t);
                    first = false;
                }
            }
            std::cout << "| " << std::setw(col_w) << cell;
        }
        std::cout << "\n";
    }
    std::cout << std::string(total_w, '-') << "\n";
}

static void print_dfa(const DFA &dfa) {
    std::vector<std::string> symbols(dfa.alphabet.begin(), dfa.alphabet.end());

    int col_w = 10;
    int set_w = 26;
    std::cout << std::left << std::setw(10) << "STATE"
              << std::setw(set_w) << "NFA STATES";
    for (auto &sym : symbols)
        std::cout << "| " << std::setw(col_w) << sym;
    std::cout << "\n";

    int total_w = 10 + set_w + static_cast<int>(symbols.size()) * (col_w + 2);
    std::cout << std::string(total_w, '-') << "\n";

    for (auto &ds : dfa.states) {
        std::string prefix;
        if (ds.id == dfa.start) prefix += "->";
        if (ds.is_accept) prefix += "*";
        std::string label = prefix + "D" + std::to_string(ds.id);
        std::cout << std::left << std::setw(10) << label
                  << std::setw(set_w) << set_to_string(ds.nfa_states);

        for (auto &sym : symbols) {
            auto it = ds.transitions.find(sym);
            std::string cell = "-";
            if (it != ds.transitions.end())
                cell = "D" + std::to_string(it->second);
            std::cout << "| " << std::setw(col_w) << cell;
        }
        std::cout << "\n";
    }
    std::cout << std::string(total_w, '-') << "\n";
}

/* ---- build example NFA for (a|b)*abb ------------------------------------ */
static NFA build_example_nfa() {
    NFA nfa;
    nfa.start = 0;
    nfa.accept_states = {10};
    nfa.num_states = 11;

    struct { int from; std::string sym; int to; } transitions[] = {
        {0, EPS, 1}, {0, EPS, 7},
        {1, EPS, 2}, {1, EPS, 4},
        {2, "a", 3},
        {3, EPS, 6},
        {4, "b", 5},
        {5, EPS, 6},
        {6, EPS, 1}, {6, EPS, 7},
        {7, "a", 8},
        {8, "b", 9},
        {9, "b", 10},
    };

    for (auto &t : transitions)
        nfa.add_transition(t.from, t.sym, t.to);

    return nfa;
}

/* ---- main --------------------------------------------------------------- */
int main() {
    std::cout << "======================================================================\n"
              << "  NFA -> DFA (Subset Construction)  --  Exercise 3\n"
              << "======================================================================\n\n";

    std::cout << "Choose input mode:\n"
              << "  1) Use default NFA for (a|b)*abb\n"
              << "  2) Enter your own NFA\n"
              << "\nEnter choice [1]: ";

    std::string choice;
    std::getline(std::cin, choice);

    NFA nfa;
    if (choice == "2") {
        std::cout << "\nEnter number of NFA states: ";
        int n; std::cin >> n;
        nfa.num_states = n;

        std::cout << "Enter start state: ";
        std::cin >> nfa.start;

        std::cout << "Enter number of accept states: ";
        int na; std::cin >> na;
        std::cout << "Enter accept state IDs: ";
        for (int i = 0; i < na; i++) {
            int acc; std::cin >> acc;
            nfa.accept_states.insert(acc);
        }

        std::cout << "Enter number of transitions: ";
        int nt; std::cin >> nt;
        std::cout << "Enter transitions (from symbol to), use 'eps' for epsilon:\n";
        for (int i = 0; i < nt; i++) {
            int from, to;
            std::string sym;
            std::cin >> from >> sym >> to;
            nfa.add_transition(from, sym, to);
        }
    } else {
        nfa = build_example_nfa();
    }

    std::cout << "\n--- NFA Transition Table ---\n\n";
    print_nfa(nfa);

    DFA dfa = subset_construction(nfa);

    std::cout << "\n--- DFA Transition Table (Subset Construction) ---\n\n";
    print_dfa(dfa);

    /* summary */
    std::cout << "\nSummary:\n";
    std::cout << "  NFA states: " << nfa.num_states << "\n";
    std::cout << "  DFA states: " << dfa.states.size() << "\n";
    std::cout << "  DFA start : D" << dfa.start << "\n";
    std::cout << "  DFA accept: ";
    bool first = true;
    for (auto &ds : dfa.states) {
        if (ds.is_accept) {
            if (!first) std::cout << ", ";
            std::cout << "D" << ds.id;
            first = false;
        }
    }
    std::cout << "\n";

    return 0;
}
