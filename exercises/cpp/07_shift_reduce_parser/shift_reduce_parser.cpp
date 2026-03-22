/*
================================================================================
Exercise 7: Shift-Reduce Parser (C++ Implementation)
================================================================================

CONCEPT:
A shift-reduce parser is a bottom-up parser that builds a parse tree from the
leaves up to the root. It uses a stack and processes the input left to right.
At each step, it either "shifts" the next input symbol onto the stack, or
"reduces" a sequence of symbols on top of the stack that matches the RHS of
a production, replacing them with the LHS non-terminal.

Four types of actions:
  - Shift: Push the next input symbol onto the stack.
  - Reduce: Replace a handle on top of the stack with LHS.
  - Accept: Input consumed and stack contains only the start symbol.
  - Error: No valid action possible.

ALGORITHM:
Uses operator-precedence to resolve ambiguity in E -> E+E | E*E | (E) | id.
When E op E is on top of the stack and the next input is an operator:
  - If stack operator has higher or equal precedence, reduce (left-associative).
  - If input operator has higher precedence, shift.

COMPLEXITY:
  Time:  O(n^2) worst case
  Space: O(n) for the stack

EXAMPLE:
  Input: id+id*id
  * binds tighter than +, so id*id reduces to E first, then E+E reduces to E.
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::map;

/* ---- Precedence ---- */

map<string, int> PRECEDENCE = {{"+", 1}, {"*", 2}};

bool is_operator(const string &tok) {
    return PRECEDENCE.find(tok) != PRECEDENCE.end();
}

int precedence(const string &op) {
    auto it = PRECEDENCE.find(op);
    return (it != PRECEDENCE.end()) ? it->second : 0;
}

/* ---- Tokenizer ---- */

vector<string> tokenize(const string &input) {
    vector<string> tokens;
    size_t i = 0;
    while (i < input.size()) {
        if (isspace(input[i])) { i++; continue; }
        if (i + 1 < input.size() && input.substr(i, 2) == "id") {
            tokens.push_back("id");
            i += 2;
        } else {
            tokens.push_back(string(1, input[i]));
            i++;
        }
    }
    tokens.push_back("$");
    return tokens;
}

/* ---- Stack helpers ---- */

string stack_to_string(const vector<string> &stack) {
    if (stack.empty()) return "(empty)";
    string result;
    for (size_t i = 0; i < stack.size(); i++) {
        if (i > 0) result += " ";
        result += stack[i];
    }
    return result;
}

string remaining_to_string(const vector<string> &tokens, int pos) {
    string result;
    for (int i = pos; i < (int)tokens.size(); i++) {
        if (i > pos) result += " ";
        result += tokens[i];
    }
    return result;
}

/* Find topmost operator index on stack */
int find_top_operator(const vector<string> &stack) {
    for (int i = (int)stack.size() - 1; i >= 0; i--) {
        if (is_operator(stack[i])) return i;
    }
    return -1;
}

bool should_reduce_over_shift(const vector<string> &stack, const string &next_token) {
    int op_idx = find_top_operator(stack);
    if (op_idx < 0) return false;

    const string &stack_op = stack[op_idx];

    if (!is_operator(next_token)) {
        return (next_token == ")" || next_token == "$");
    }

    return (precedence(stack_op) >= precedence(next_token));
}

/* ---- Reduction ---- */

struct ReductionResult {
    bool success;
    string production;
};

ReductionResult try_reduce(vector<string> &stack) {
    int n = (int)stack.size();

    /* E -> id */
    if (n >= 1 && stack.back() == "id") {
        stack.pop_back();
        stack.push_back("E");
        return {true, "E -> id"};
    }

    /* E -> ( E ) */
    if (n >= 3 && stack[n-3] == "(" && stack[n-2] == "E" && stack[n-1] == ")") {
        stack.erase(stack.end() - 3, stack.end());
        stack.push_back("E");
        return {true, "E -> ( E )"};
    }

    /* E -> E + E */
    if (n >= 3 && stack[n-3] == "E" && stack[n-2] == "+" && stack[n-1] == "E") {
        stack.erase(stack.end() - 3, stack.end());
        stack.push_back("E");
        return {true, "E -> E + E"};
    }

    /* E -> E * E */
    if (n >= 3 && stack[n-3] == "E" && stack[n-2] == "*" && stack[n-1] == "E") {
        stack.erase(stack.end() - 3, stack.end());
        stack.push_back("E");
        return {true, "E -> E * E"};
    }

    return {false, ""};
}

/* ---- Parser ---- */

bool parse(const string &input) {
    vector<string> tokens = tokenize(input);
    vector<string> stack;
    int pos = 0;
    int step = 0;

    cout << "\n  " << std::left << std::setw(6) << "Step"
         << std::setw(30) << "Stack"
         << std::setw(25) << "Input"
         << "Action" << endl;
    cout << "  " << string(6, '-') << " " << string(29, '-') << " "
         << string(24, '-') << " " << string(30, '-') << endl;

    while (true) {
        const string &current = tokens[pos];
        step++;
        string stack_str = stack_to_string(stack);
        string remain_str = remaining_to_string(tokens, pos);

        /* Accept */
        if (stack.size() == 1 && stack[0] == "E" && current == "$") {
            cout << "  " << std::left << std::setw(6) << step
                 << std::setw(30) << stack_str
                 << std::setw(25) << remain_str
                 << "ACCEPT" << endl;
            cout << "\n  ** Input \"" << input << "\" successfully parsed! **" << endl;
            return true;
        }

        /* Reduce id immediately */
        if (!stack.empty() && stack.back() == "id") {
            auto result = try_reduce(stack);
            if (result.success) {
                cout << "  " << std::left << std::setw(6) << step
                     << std::setw(30) << stack_str
                     << std::setw(25) << remain_str
                     << "Reduce: " << result.production << endl;
                continue;
            }
        }

        /* Reduce ( E ) */
        int n = (int)stack.size();
        if (n >= 3 && stack[n-1] == ")" && stack[n-2] == "E" && stack[n-3] == "(") {
            auto result = try_reduce(stack);
            if (result.success) {
                cout << "  " << std::left << std::setw(6) << step
                     << std::setw(30) << stack_str
                     << std::setw(25) << remain_str
                     << "Reduce: " << result.production << endl;
                continue;
            }
        }

        /* E op E on top: decide shift vs reduce */
        n = (int)stack.size();
        if (n >= 3 && stack[n-3] == "E" && is_operator(stack[n-2]) && stack[n-1] == "E") {
            if (should_reduce_over_shift(stack, current)) {
                auto result = try_reduce(stack);
                if (result.success) {
                    cout << "  " << std::left << std::setw(6) << step
                         << std::setw(30) << stack_str
                         << std::setw(25) << remain_str
                         << "Reduce: " << result.production << endl;
                    continue;
                }
            }
        }

        /* Shift */
        if (current != "$") {
            cout << "  " << std::left << std::setw(6) << step
                 << std::setw(30) << stack_str
                 << std::setw(25) << remain_str
                 << "Shift '" << current << "'" << endl;
            stack.push_back(current);
            pos++;
        } else {
            /* Try final reduce */
            auto result = try_reduce(stack);
            if (result.success) {
                cout << "  " << std::left << std::setw(6) << step
                     << std::setw(30) << stack_str
                     << std::setw(25) << remain_str
                     << "Reduce: " << result.production << endl;
            } else {
                cout << "  " << std::left << std::setw(6) << step
                     << std::setw(30) << stack_str
                     << std::setw(25) << remain_str
                     << "ERROR" << endl;
                cout << "\n  ** Parse error on \"" << input << "\" **" << endl;
                return false;
            }
        }
    }
}

int main() {
    cout << "===========================================================================" << endl;
    cout << "  Exercise 7: Shift-Reduce Parser" << endl;
    cout << "===========================================================================" << endl;

    cout << "\n  Grammar:" << endl;
    cout << "    E -> E + E" << endl;
    cout << "    E -> E * E" << endl;
    cout << "    E -> ( E )" << endl;
    cout << "    E -> id" << endl;
    cout << "\n  Operator Precedence: * > +  (left-associative)" << endl;

    vector<string> test_inputs = {
        "id+id*id",
        "id*id+id",
        "id+id+id",
        "(id+id)*id",
        "id"
    };

    for (const auto &input : test_inputs) {
        cout << "\n===========================================================================" << endl;
        cout << "\n  Parsing: \"" << input << "\"" << endl;
        parse(input);
    }

    cout << "\n===========================================================================" << endl;
    return 0;
}
