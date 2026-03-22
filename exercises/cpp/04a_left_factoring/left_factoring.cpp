/*
================================================================================
Exercise 4a: Left Factoring of Context-Free Grammars -- C++ Implementation
================================================================================

CONCEPT:
    Left factoring is a grammar transformation that eliminates common prefixes
    among the right-hand sides of productions for the same non-terminal.  This
    makes the grammar suitable for predictive (top-down) parsing by ensuring
    that the choice of production can be determined by looking ahead at only
    one input symbol.

    Given:   A -> alpha beta1 | alpha beta2
    Result:  A  -> alpha A'
             A' -> beta1 | beta2

ALGORITHM:
    For each non-terminal A:
      1. Find the longest common prefix among its productions.
      2. If a prefix of length >= 1 is shared by two or more productions:
         a. Create a new non-terminal A'.
         b. Replace the matching productions with:  A -> prefix A'.
         c. Add: A' -> suffix1 | suffix2 | ...  (eps for empty suffix).
      3. Repeat until no more factoring is possible.

COMPLEXITY:
    Time:  O(P * L) per non-terminal per round (P = productions, L = max length).
    Space: O(G) for the grammar.

EXAMPLE:
    Input:   S -> a A B | a A c | b B c    A -> d A | e
    Output:  S -> a A S' | b B c           S' -> B | c      A -> d A | e
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

/* ---- types -------------------------------------------------------------- */
using Production = std::vector<std::string>;      /* list of symbols */
using ProductionList = std::vector<Production>;

/* ordered grammar: vector of (name, productions) to preserve insertion order */
struct GrammarEntry {
    std::string     name;
    ProductionList  prods;
};
using Grammar = std::vector<GrammarEntry>;

/* ---- find a non-terminal in the grammar --------------------------------- */
static int find_nt(const Grammar &g, const std::string &name) {
    for (size_t i = 0; i < g.size(); i++)
        if (g[i].name == name) return static_cast<int>(i);
    return -1;
}

/* ---- generate a new non-terminal name ----------------------------------- */
static std::string new_nt_name(const Grammar &g, const std::string &base) {
    std::string candidate = base + "'";
    while (find_nt(g, candidate) >= 0)
        candidate += "'";
    return candidate;
}

/* ---- parse a grammar from text ------------------------------------------ */
static Grammar parse_grammar(const std::string &text) {
    Grammar grammar;
    std::istringstream ss(text);
    std::string line;

    while (std::getline(ss, line)) {
        /* trim */
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        /* find -> */
        auto arrow = line.find("->");
        if (arrow == std::string::npos) continue;

        std::string lhs = line.substr(0, arrow);
        std::string rhs = line.substr(arrow + 2);

        /* trim lhs */
        lhs.erase(lhs.find_last_not_of(" \t") + 1);
        lhs.erase(0, lhs.find_first_not_of(" \t"));

        int idx = find_nt(grammar, lhs);
        if (idx < 0) {
            grammar.push_back({lhs, {}});
            idx = static_cast<int>(grammar.size()) - 1;
        }

        /* split rhs by '|' */
        std::istringstream rhs_ss(rhs);
        std::string segment;
        /* manual split */
        size_t pos = 0;
        while (pos < rhs.size()) {
            size_t bar = rhs.find('|', pos);
            if (bar == std::string::npos) bar = rhs.size();
            std::string alt = rhs.substr(pos, bar - pos);
            pos = bar + 1;

            /* tokenize alt */
            Production prod;
            std::istringstream alt_ss(alt);
            std::string sym;
            while (alt_ss >> sym)
                prod.push_back(sym);
            if (!prod.empty())
                grammar[idx].prods.push_back(prod);
        }
    }
    return grammar;
}

/* ---- print grammar ------------------------------------------------------ */
static void print_grammar(const Grammar &g) {
    for (auto &entry : g) {
        std::cout << "  " << entry.name << " ->";
        for (size_t i = 0; i < entry.prods.size(); i++) {
            if (i > 0) std::cout << " |";
            for (auto &sym : entry.prods[i])
                std::cout << " " << sym;
        }
        std::cout << "\n";
    }
}

/* ---- find longest common prefix ----------------------------------------- */
static Production longest_common_prefix(const ProductionList &prods) {
    if (prods.size() < 2) return {};

    Production best;
    size_t max_len = 0;
    for (auto &p : prods)
        max_len = std::max(max_len, p.size());

    for (size_t length = 1; length <= max_len; length++) {
        bool found = false;
        for (size_t i = 0; i < prods.size() && !found; i++) {
            if (prods[i].size() < length) continue;
            int count = 1;
            for (size_t j = i + 1; j < prods.size(); j++) {
                if (prods[j].size() < length) continue;
                bool match = true;
                for (size_t k = 0; k < length; k++) {
                    if (prods[i][k] != prods[j][k]) { match = false; break; }
                }
                if (match) count++;
            }
            if (count >= 2) {
                best = Production(prods[i].begin(), prods[i].begin() + length);
                found = true;
            }
        }
        if (!found) break;
    }
    return best;
}

/* ---- one round of left factoring ---------------------------------------- */
static bool left_factor_once(Grammar &g) {
    for (size_t ni = 0; ni < g.size(); ni++) {
        auto &entry = g[ni];
        Production prefix = longest_common_prefix(entry.prods);
        if (prefix.empty()) continue;

        size_t plen = prefix.size();

        /* partition into matching and non-matching */
        ProductionList matching, non_matching;
        for (auto &prod : entry.prods) {
            bool match = (prod.size() >= plen);
            if (match) {
                for (size_t k = 0; k < plen; k++) {
                    if (prod[k] != prefix[k]) { match = false; break; }
                }
            }
            if (match) matching.push_back(prod);
            else        non_matching.push_back(prod);
        }

        /* create new non-terminal */
        std::string new_name = new_nt_name(g, entry.name);

        /* factored production: prefix + new_nt */
        Production factored = prefix;
        factored.push_back(new_name);

        /* rebuild original */
        ProductionList new_prods;
        new_prods.push_back(factored);
        for (auto &p : non_matching) new_prods.push_back(p);
        entry.prods = new_prods;

        /* suffix productions */
        ProductionList suffix_prods;
        for (auto &prod : matching) {
            Production suffix(prod.begin() + plen, prod.end());
            if (suffix.empty()) suffix.push_back("eps");
            suffix_prods.push_back(suffix);
        }

        /* insert new non-terminal right after current one */
        g.insert(g.begin() + ni + 1, {new_name, suffix_prods});

        return true;
    }
    return false;
}

/* ---- full left factoring ------------------------------------------------ */
static void left_factor(Grammar &g) {
    int iteration = 0;
    while (left_factor_once(g)) {
        iteration++;
        if (iteration > 50) {
            std::cout << "  [Warning: exceeded 50 iterations]\n";
            break;
        }
    }
}

/* ---- main --------------------------------------------------------------- */
int main() {
    std::cout << "======================================================================\n"
              << "  Left Factoring of Context-Free Grammars  --  Exercise 4a\n"
              << "======================================================================\n\n";

    std::cout << "Choose input mode:\n"
              << "  1) Example: S -> a A B | a A c | b B c,  A -> d A | e\n"
              << "  2) Example: E -> T + E | T,  T -> int | int * T | ( E )\n"
              << "  3) Enter your own grammar\n"
              << "\nEnter choice [1]: ";

    std::string choice;
    std::getline(std::cin, choice);

    Grammar grammar;

    if (choice == "2") {
        grammar = parse_grammar(
            "E -> T + E | T\n"
            "T -> int | int * T | ( E )\n"
        );
    } else if (choice == "3") {
        std::cout << "\nEnter grammar (one rule per line, end with empty line):\n";
        std::string all_text;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) break;
            all_text += line + "\n";
        }
        grammar = parse_grammar(all_text);
    } else {
        grammar = parse_grammar(
            "S -> a A B | a A c | b B c\n"
            "A -> d A | e\n"
        );
    }

    std::cout << "\n--- Original Grammar ---\n";

    /* save a copy for display */
    Grammar original = grammar;
    print_grammar(original);

    left_factor(grammar);

    std::cout << "\n--- Left-Factored Grammar ---\n";
    print_grammar(grammar);

    /* show new non-terminals */
    std::vector<std::string> new_nts;
    for (auto &entry : grammar) {
        if (find_nt(original, entry.name) < 0)
            new_nts.push_back(entry.name);
    }
    if (!new_nts.empty()) {
        std::cout << "\n  New non-terminals introduced:";
        for (auto &n : new_nts) std::cout << " " << n;
        std::cout << "\n";
    } else {
        std::cout << "\n  No factoring was necessary.\n";
    }

    return 0;
}
