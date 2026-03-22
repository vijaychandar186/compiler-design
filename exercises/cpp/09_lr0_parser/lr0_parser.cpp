/*
 * ==============================================================================
 * Exercise 9: LR(0) Parser - Canonical Collection of Item Sets
 * ==============================================================================
 *
 * CONCEPT:
 * An LR(0) item is a production with a "dot" indicating the current parse
 * position. The canonical collection of LR(0) item sets forms the state
 * machine that drives bottom-up (shift-reduce) parsing. Each state encodes
 * what productions could be in progress given the symbols seen so far.
 *
 * The construction proceeds by repeatedly applying two operations:
 * - CLOSURE: expands an item set by adding items for non-terminals after dots.
 * - GOTO: advances the dot past a specific symbol, producing a new state.
 *
 * ALGORITHM:
 *   1. Augment grammar: S' -> S.
 *   2. I0 = CLOSURE({S' -> . S}).
 *   3. For each state I and each symbol X, compute GOTO(I, X).
 *      If the result is non-empty and new, add it to the collection.
 *   4. Repeat until no new states are generated.
 *
 * TIME COMPLEXITY:  O(|G|^2 * |V|) worst case.
 * SPACE COMPLEXITY: O(|G|^2) for storing all item sets.
 *
 * EXAMPLE INPUT:
 *   E -> E + T | T
 *   T -> T * F | F
 *   F -> ( E ) | id
 *
 * EXAMPLE OUTPUT:
 *   12 item sets (I0 through I11) with GOTO transitions.
 * ==============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <sstream>

/* --- Data structures --- */

struct Production {
    std::string lhs;
    std::vector<std::string> rhs;
};

struct LR0Item {
    int prod_index;  /* Index into the global productions vector */
    int dot_pos;     /* Position of the dot */

    bool operator<(const LR0Item &other) const {
        if (prod_index != other.prod_index)
            return prod_index < other.prod_index;
        return dot_pos < other.dot_pos;
    }

    bool operator==(const LR0Item &other) const {
        return prod_index == other.prod_index && dot_pos == other.dot_pos;
    }
};

/* An item set is a sorted set of items (using std::set for uniqueness) */
typedef std::set<LR0Item> ItemSet;

/* --- LR(0) Automaton Builder --- */

class LR0Automaton {
private:
    std::vector<Production> productions_;
    std::set<std::string> non_terminals_;
    std::vector<std::string> all_symbols_;
    std::string augmented_start_;

    std::vector<ItemSet> states_;
    std::map<ItemSet, int> state_index_;
    std::vector<std::tuple<int, std::string, int>> transitions_;

    bool is_non_terminal(const std::string &sym) const {
        return non_terminals_.count(sym) > 0;
    }

    const std::string *symbol_after_dot(const LR0Item &item) const {
        const Production &prod = productions_[item.prod_index];
        if (item.dot_pos >= static_cast<int>(prod.rhs.size()))
            return nullptr;
        return &prod.rhs[item.dot_pos];
    }

    ItemSet closure(const ItemSet &items) const {
        ItemSet result = items;
        bool changed = true;

        while (changed) {
            changed = false;
            ItemSet to_add;

            for (const auto &item : result) {
                const std::string *next_sym = symbol_after_dot(item);
                if (!next_sym || !is_non_terminal(*next_sym))
                    continue;

                for (int p = 0; p < static_cast<int>(productions_.size()); p++) {
                    if (productions_[p].lhs == *next_sym) {
                        LR0Item new_item = {p, 0};
                        if (result.find(new_item) == result.end()) {
                            to_add.insert(new_item);
                        }
                    }
                }
            }

            for (const auto &item : to_add) {
                if (result.insert(item).second)
                    changed = true;
            }
        }

        return result;
    }

    ItemSet goto_func(const ItemSet &items, const std::string &symbol) const {
        ItemSet kernel;

        for (const auto &item : items) {
            const std::string *next_sym = symbol_after_dot(item);
            if (next_sym && *next_sym == symbol) {
                LR0Item advanced = {item.prod_index, item.dot_pos + 1};
                kernel.insert(advanced);
            }
        }

        if (kernel.empty())
            return kernel;

        return closure(kernel);
    }

    void collect_symbols() {
        std::set<std::string> sym_set;
        for (const auto &prod : productions_) {
            sym_set.insert(prod.lhs);
            for (const auto &s : prod.rhs)
                sym_set.insert(s);
        }
        all_symbols_.assign(sym_set.begin(), sym_set.end());
    }

public:
    void add_non_terminal(const std::string &nt) {
        non_terminals_.insert(nt);
    }

    int add_production(const std::string &lhs,
                       const std::vector<std::string> &rhs) {
        non_terminals_.insert(lhs);
        productions_.push_back({lhs, rhs});
        return static_cast<int>(productions_.size()) - 1;
    }

    void build(const std::string &original_start) {
        augmented_start_ = original_start + "'";
        /* The augmented production must be at index 0 */
        /* It was already added by the caller */

        collect_symbols();

        /* I0 = closure({S' -> . S}) */
        ItemSet i0;
        i0.insert({0, 0});
        i0 = closure(i0);

        states_.push_back(i0);
        state_index_[i0] = 0;

        std::vector<int> worklist = {0};

        while (!worklist.empty()) {
            int state = worklist.front();
            worklist.erase(worklist.begin());

            for (const auto &sym : all_symbols_) {
                ItemSet goto_set = goto_func(states_[state], sym);
                if (goto_set.empty())
                    continue;

                int target;
                auto it = state_index_.find(goto_set);
                if (it != state_index_.end()) {
                    target = it->second;
                } else {
                    target = static_cast<int>(states_.size());
                    states_.push_back(goto_set);
                    state_index_[goto_set] = target;
                    worklist.push_back(target);
                }

                transitions_.push_back({state, sym, target});
            }
        }
    }

    void print_augmented_grammar() const {
        std::cout << "============================================================\n";
        std::cout << "  Augmented Grammar\n";
        std::cout << "============================================================\n";

        for (int i = 0; i < static_cast<int>(productions_.size()); i++) {
            std::cout << "  (" << i << ") " << productions_[i].lhs << " ->";
            for (const auto &s : productions_[i].rhs)
                std::cout << " " << s;
            std::cout << "\n";
        }
        std::cout << "============================================================\n";
    }

    void print_item(const LR0Item &item) const {
        const Production &prod = productions_[item.prod_index];
        std::cout << "    " << prod.lhs << " ->";

        for (int j = 0; j < static_cast<int>(prod.rhs.size()); j++) {
            if (j == item.dot_pos)
                std::cout << " .";
            std::cout << " " << prod.rhs[j];
        }
        if (item.dot_pos == static_cast<int>(prod.rhs.size()))
            std::cout << " .";
        std::cout << "\n";
    }

    void print_item_sets() const {
        std::cout << "\n============================================================\n";
        std::cout << "  Canonical Collection of LR(0) Item Sets\n";
        std::cout << "  Total states: " << states_.size() << "\n";
        std::cout << "============================================================\n";

        for (int i = 0; i < static_cast<int>(states_.size()); i++) {
            std::cout << "\n  I" << i << ":\n";
            std::cout << "  ----------------------------------------\n";
            for (const auto &item : states_[i]) {
                print_item(item);
            }
        }
        std::cout << "\n============================================================\n";
    }

    void print_transitions() const {
        std::cout << "\n============================================================\n";
        std::cout << "  GOTO Transitions\n";
        std::cout << "============================================================\n";

        for (const auto &t : transitions_) {
            std::cout << "  GOTO(I" << std::get<0>(t) << ", "
                      << std::get<1>(t) << ") = I" << std::get<2>(t) << "\n";
        }
        std::cout << "============================================================\n";
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "  LR(0) Parser - Canonical Collection Constructor\n";
    std::cout << "============================================================\n";

    LR0Automaton automaton;

    /* Register non-terminals */
    automaton.add_non_terminal("E'");
    automaton.add_non_terminal("E");
    automaton.add_non_terminal("T");
    automaton.add_non_terminal("F");

    /* Augmented grammar */
    automaton.add_production("E'", {"E"});         /* 0: E' -> E     */
    automaton.add_production("E",  {"E", "+", "T"}); /* 1: E -> E + T */
    automaton.add_production("E",  {"T"});           /* 2: E -> T     */
    automaton.add_production("T",  {"T", "*", "F"}); /* 3: T -> T * F */
    automaton.add_production("T",  {"F"});           /* 4: T -> F     */
    automaton.add_production("F",  {"(", "E", ")"}); /* 5: F -> ( E ) */
    automaton.add_production("F",  {"id"});          /* 6: F -> id    */

    automaton.print_augmented_grammar();
    automaton.build("E");
    automaton.print_item_sets();
    automaton.print_transitions();

    /* Second example */
    std::cout << "\n\n############################################################\n";
    std::cout << "  Second Example: S -> A A, A -> a A | b\n";
    std::cout << "############################################################\n";

    LR0Automaton automaton2;
    automaton2.add_non_terminal("S'");
    automaton2.add_non_terminal("S");
    automaton2.add_non_terminal("A");

    automaton2.add_production("S'", {"S"});
    automaton2.add_production("S",  {"A", "A"});
    automaton2.add_production("A",  {"a", "A"});
    automaton2.add_production("A",  {"b"});

    automaton2.print_augmented_grammar();
    automaton2.build("S");
    automaton2.print_item_sets();
    automaton2.print_transitions();

    return 0;
}
