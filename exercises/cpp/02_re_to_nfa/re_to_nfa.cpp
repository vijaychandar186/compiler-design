/*
================================================================================
Exercise 2: Regular Expression to NFA (Thompson's Construction) -- C++
================================================================================

CONCEPT:
    Thompson's construction transforms a regular expression into an equivalent
    NFA.  Each NFA fragment has one start state and one accept state.  Small
    fragments for individual symbols are composed via concatenation, union (|),
    and Kleene star (*) operations.

ALGORITHM:
    1. Insert explicit concatenation operators ('.') where implied.
    2. Convert infix -> postfix using the shunting-yard algorithm.
       Precedence: * (3) > . (2) > | (1).
    3. Evaluate the postfix expression with a stack of NFA fragments:
       - Literal 'c' : two-state fragment  start --c--> accept.
       - '.'         : pop two, eps-link first.accept -> second.start.
       - '|'         : pop two, new start eps-branches to both, both
                       eps-converge to new accept.
       - '*'         : pop one, eps loop + bypass.
    4. The remaining fragment is the full NFA.

COMPLEXITY:
    Time:  O(n)  -- linear in the regex length.
    Space: O(n)  -- at most 2n states created.

EXAMPLE:
    Input:  (a|b)*abb
    Output: NFA transition table with ~11 states.
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <iomanip>

/* ---- constants ---------------------------------------------------------- */
static const std::string EPSILON_LABEL = "eps";

/* ---- state -------------------------------------------------------------- */
struct State {
    int id;
    std::map<std::string, std::vector<int>> transitions;  /* symbol -> target ids */
};

/* ---- NFA ---------------------------------------------------------------- */
class NFA {
public:
    std::vector<State> states;
    int start_id;
    int accept_id;

    int new_state() {
        int id = static_cast<int>(states.size());
        states.push_back({id, {}});
        return id;
    }

    void add_transition(int from, const std::string &symbol, int to) {
        states[from].transitions[symbol].push_back(to);
    }
};

/* ---- fragment (indices into the NFA's state vector) --------------------- */
struct Fragment {
    int start;
    int accept;
};

/* ---- Thompson primitives ------------------------------------------------ */
static Fragment make_char(NFA &nfa, char ch) {
    int s = nfa.new_state();
    int a = nfa.new_state();
    nfa.add_transition(s, std::string(1, ch), a);
    return {s, a};
}

static Fragment make_concat(NFA &nfa, Fragment first, Fragment second) {
    nfa.add_transition(first.accept, EPSILON_LABEL, second.start);
    return {first.start, second.accept};
}

static Fragment make_union(NFA &nfa, Fragment upper, Fragment lower) {
    int s = nfa.new_state();
    int a = nfa.new_state();
    nfa.add_transition(s, EPSILON_LABEL, upper.start);
    nfa.add_transition(s, EPSILON_LABEL, lower.start);
    nfa.add_transition(upper.accept, EPSILON_LABEL, a);
    nfa.add_transition(lower.accept, EPSILON_LABEL, a);
    return {s, a};
}

static Fragment make_star(NFA &nfa, Fragment inner) {
    int s = nfa.new_state();
    int a = nfa.new_state();
    nfa.add_transition(s, EPSILON_LABEL, inner.start);
    nfa.add_transition(s, EPSILON_LABEL, a);
    nfa.add_transition(inner.accept, EPSILON_LABEL, inner.start);
    nfa.add_transition(inner.accept, EPSILON_LABEL, a);
    return {s, a};
}

/* ---- insert explicit concatenation -------------------------------------- */
static std::string insert_concat(const std::string &regex) {
    std::string result;
    for (size_t i = 0; i < regex.size(); i++) {
        result += regex[i];
        if (i + 1 < regex.size()) {
            char curr = regex[i];
            char next = regex[i + 1];
            if (curr != '(' && curr != '|' && curr != '.' &&
                next != ')' && next != '|' && next != '*' && next != '.') {
                result += '.';
            }
        }
    }
    return result;
}

/* ---- shunting-yard: infix -> postfix ------------------------------------ */
static int precedence(char op) {
    if (op == '*') return 3;
    if (op == '.') return 2;
    if (op == '|') return 1;
    return 0;
}

static std::string to_postfix(const std::string &infix) {
    std::string output;
    std::stack<char> ops;
    for (char ch : infix) {
        if (ch == '(') {
            ops.push(ch);
        } else if (ch == ')') {
            while (!ops.empty() && ops.top() != '(') {
                output += ops.top(); ops.pop();
            }
            if (!ops.empty()) ops.pop();
        } else if (ch == '|' || ch == '.' || ch == '*') {
            while (!ops.empty() && ops.top() != '(' &&
                   precedence(ops.top()) >= precedence(ch)) {
                output += ops.top(); ops.pop();
            }
            ops.push(ch);
        } else {
            output += ch;
        }
    }
    while (!ops.empty()) { output += ops.top(); ops.pop(); }
    return output;
}

/* ---- build NFA from postfix --------------------------------------------- */
static NFA build_nfa(const std::string &postfix) {
    NFA nfa;
    std::stack<Fragment> frag_stack;

    for (char ch : postfix) {
        if (ch == '.') {
            Fragment second = frag_stack.top(); frag_stack.pop();
            Fragment first  = frag_stack.top(); frag_stack.pop();
            frag_stack.push(make_concat(nfa, first, second));
        } else if (ch == '|') {
            Fragment lower = frag_stack.top(); frag_stack.pop();
            Fragment upper = frag_stack.top(); frag_stack.pop();
            frag_stack.push(make_union(nfa, upper, lower));
        } else if (ch == '*') {
            Fragment inner = frag_stack.top(); frag_stack.pop();
            frag_stack.push(make_star(nfa, inner));
        } else {
            frag_stack.push(make_char(nfa, ch));
        }
    }

    Fragment result = frag_stack.top();
    nfa.start_id  = result.start;
    nfa.accept_id = result.accept;
    return nfa;
}

/* ---- print the NFA ------------------------------------------------------ */
static void print_nfa(const NFA &nfa) {
    /* gather alphabet */
    std::set<std::string> alpha_set;
    for (auto &state : nfa.states) {
        for (auto &[sym, _] : state.transitions) {
            alpha_set.insert(sym);
        }
    }
    /* order: epsilon first, then sorted */
    std::vector<std::string> symbols;
    if (alpha_set.count(EPSILON_LABEL)) symbols.push_back(EPSILON_LABEL);
    for (auto &s : alpha_set)
        if (s != EPSILON_LABEL) symbols.push_back(s);

    int col_w = 16;
    /* header */
    std::cout << std::left << std::setw(10) << "STATE";
    for (auto &sym : symbols)
        std::cout << "| " << std::setw(col_w - 2) << sym;
    std::cout << "\n";

    int total_w = 10 + static_cast<int>(symbols.size()) * col_w;
    std::cout << std::string(total_w, '-') << "\n";

    for (auto &state : nfa.states) {
        std::string prefix;
        if (state.id == nfa.start_id)  prefix += "->";
        if (state.id == nfa.accept_id) prefix += "*";
        std::string label = prefix + "q" + std::to_string(state.id);
        std::cout << std::left << std::setw(10) << label;

        for (auto &sym : symbols) {
            auto it = state.transitions.find(sym);
            std::string cell = "-";
            if (it != state.transitions.end() && !it->second.empty()) {
                cell = "";
                for (size_t i = 0; i < it->second.size(); i++) {
                    if (i > 0) cell += ", ";
                    cell += "q" + std::to_string(it->second[i]);
                }
            }
            std::cout << "| " << std::setw(col_w - 2) << cell;
        }
        std::cout << "\n";
    }

    std::cout << std::string(total_w, '-') << "\n";
    std::cout << "\nStart state : q" << nfa.start_id << "\n";
    std::cout << "Accept state: q" << nfa.accept_id << "\n";
    std::cout << "Total states: " << nfa.states.size() << "\n";
}

/* ---- main --------------------------------------------------------------- */
int main() {
    std::cout << "======================================================================\n"
              << "  RE -> NFA (Thompson's Construction)  --  Exercise 2\n"
              << "======================================================================\n\n";

    std::string default_regex = "(a|b)*abb";
    std::cout << "Default regex: " << default_regex << "\n";
    std::cout << "Enter a regex (or press Enter for default): ";

    std::string user_input;
    std::getline(std::cin, user_input);
    std::string regex = user_input.empty() ? default_regex : user_input;

    std::string with_concat = insert_concat(regex);
    std::string postfix     = to_postfix(with_concat);

    std::cout << "\n1. Original regex      : " << regex << "\n";
    std::cout << "2. With explicit concat: " << with_concat << "\n";
    std::cout << "3. Postfix form        : " << postfix << "\n";

    NFA nfa = build_nfa(postfix);

    std::cout << "\n--- NFA Transition Table ---\n\n";
    print_nfa(nfa);

    return 0;
}
