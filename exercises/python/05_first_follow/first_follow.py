"""
================================================================================
Exercise 5: FIRST and FOLLOW Sets Computation
================================================================================

CONCEPT:
FIRST and FOLLOW sets are fundamental to constructing predictive parsers (LL(1)).

FIRST(X) is the set of terminals that can appear as the first symbol of any
string derived from X. If X can derive epsilon (the empty string), then epsilon
is also in FIRST(X). For a string of symbols X1 X2 ... Xn, FIRST includes
FIRST(X1), and if X1 can derive epsilon, also FIRST(X2), and so on.

FOLLOW(A) is the set of terminals that can appear immediately after A in some
sentential form. For the start symbol, $ (end of input) is always in FOLLOW.
FOLLOW is computed by examining where each non-terminal appears in the right-
hand sides of productions.

ALGORITHM (iterative fixed-point):
FIRST:
  1. If X is a terminal, FIRST(X) = {X}.
  2. If X -> epsilon, add epsilon to FIRST(X).
  3. If X -> Y1 Y2 ... Yk, add FIRST(Y1) - {epsilon} to FIRST(X).
     If epsilon in FIRST(Y1), also add FIRST(Y2) - {epsilon}, and so on.
     If all Y1..Yk derive epsilon, add epsilon to FIRST(X).
  Repeat until no changes.

FOLLOW:
  1. Add $ to FOLLOW(S) where S is the start symbol.
  2. For each production A -> alpha B beta:
     Add FIRST(beta) - {epsilon} to FOLLOW(B).
     If beta derives epsilon (or beta is empty), add FOLLOW(A) to FOLLOW(B).
  Repeat until no changes.

COMPLEXITY:
  Time:  O(n * p * k) per iteration, O(n) iterations worst case
         where n = non-terminals, p = productions, k = max production length
  Space: O(n * t) where t = number of terminals

EXAMPLE:
  Grammar: E -> T R, R -> + T R | epsilon, T -> F Y, Y -> * F Y | epsilon,
           F -> ( E ) | id
  FIRST(E)  = { (, id }          FOLLOW(E) = { $, ) }
  FIRST(R)  = { +, epsilon }     FOLLOW(R) = { $, ) }
  FIRST(T)  = { (, id }          FOLLOW(T) = { +, $, ) }
  FIRST(Y)  = { *, epsilon }     FOLLOW(Y) = { +, $, ) }
  FIRST(F)  = { (, id }          FOLLOW(F) = { *, +, $, ) }
================================================================================
"""

from collections import OrderedDict

EPSILON = "epsilon"


def parse_grammar(grammar_text):
    """Parse grammar text into an ordered dict of non-terminal -> list of productions."""
    grammar = OrderedDict()
    for line in grammar_text.strip().split("\n"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        lhs, rhs = line.split("->")
        lhs = lhs.strip()
        alternatives = rhs.split("|")
        productions = []
        for alt in alternatives:
            symbols = alt.strip().split()
            productions.append(symbols)
        grammar[lhs] = productions
    return grammar


def get_terminals(grammar):
    """Extract all terminal symbols from the grammar."""
    nonterminals = set(grammar.keys())
    terminals = set()
    for productions in grammar.values():
        for prod in productions:
            for symbol in prod:
                if symbol not in nonterminals and symbol != EPSILON:
                    terminals.add(symbol)
    return terminals


def compute_first(grammar):
    """
    Compute FIRST sets for all non-terminals using iterative fixed-point.
    Returns a dict mapping each non-terminal to its FIRST set.
    """
    nonterminals = set(grammar.keys())
    first = {nt: set() for nt in nonterminals}

    changed = True
    while changed:
        changed = False
        for nonterminal, productions in grammar.items():
            for prod in productions:
                # Handle epsilon production
                if prod == [EPSILON]:
                    if EPSILON not in first[nonterminal]:
                        first[nonterminal].add(EPSILON)
                        changed = True
                    continue

                # Process symbols left to right
                all_have_epsilon = True
                for symbol in prod:
                    if symbol not in nonterminals:
                        # Terminal symbol
                        if symbol not in first[nonterminal]:
                            first[nonterminal].add(symbol)
                            changed = True
                        all_have_epsilon = False
                        break
                    else:
                        # Non-terminal: add FIRST(symbol) - {epsilon}
                        additions = first[symbol] - {EPSILON}
                        if not additions.issubset(first[nonterminal]):
                            first[nonterminal] |= additions
                            changed = True
                        # If this non-terminal cannot derive epsilon, stop
                        if EPSILON not in first[symbol]:
                            all_have_epsilon = False
                            break

                # If all symbols in the production can derive epsilon
                if all_have_epsilon:
                    if EPSILON not in first[nonterminal]:
                        first[nonterminal].add(EPSILON)
                        changed = True

    return first


def first_of_string(symbols, first_sets, nonterminals):
    """
    Compute FIRST of a string of symbols (used for FOLLOW computation).
    """
    result = set()
    all_have_epsilon = True

    for symbol in symbols:
        if symbol not in nonterminals:
            # Terminal
            result.add(symbol)
            all_have_epsilon = False
            break
        else:
            result |= first_sets[symbol] - {EPSILON}
            if EPSILON not in first_sets[symbol]:
                all_have_epsilon = False
                break

    if all_have_epsilon:
        result.add(EPSILON)

    return result


def compute_follow(grammar, first_sets):
    """
    Compute FOLLOW sets for all non-terminals using iterative fixed-point.
    Returns a dict mapping each non-terminal to its FOLLOW set.
    """
    nonterminals = set(grammar.keys())
    follow = {nt: set() for nt in nonterminals}

    # Rule 1: Add $ to FOLLOW of start symbol
    start_symbol = list(grammar.keys())[0]
    follow[start_symbol].add("$")

    changed = True
    while changed:
        changed = False
        for lhs, productions in grammar.items():
            for prod in productions:
                if prod == [EPSILON]:
                    continue

                for i, symbol in enumerate(prod):
                    if symbol not in nonterminals:
                        continue

                    # symbol is a non-terminal at position i
                    beta = prod[i + 1:]  # everything after this symbol

                    if beta:
                        # Rule 2: Add FIRST(beta) - {epsilon} to FOLLOW(symbol)
                        first_beta = first_of_string(beta, first_sets, nonterminals)
                        additions = first_beta - {EPSILON}
                        if not additions.issubset(follow[symbol]):
                            follow[symbol] |= additions
                            changed = True

                        # If beta can derive epsilon, add FOLLOW(lhs) to FOLLOW(symbol)
                        if EPSILON in first_beta:
                            if not follow[lhs].issubset(follow[symbol]):
                                follow[symbol] |= follow[lhs]
                                changed = True
                    else:
                        # Rule 3: beta is empty, add FOLLOW(lhs) to FOLLOW(symbol)
                        if not follow[lhs].issubset(follow[symbol]):
                            follow[symbol] |= follow[lhs]
                            changed = True

    return follow


def print_sets(first_sets, follow_sets, grammar):
    """Print FIRST and FOLLOW sets in a formatted table."""
    nonterminals = list(grammar.keys())

    # Determine column widths
    max_name = max(len(nt) for nt in nonterminals)
    max_name = max(max_name, len("Non-Terminal"))

    print(f"\n  {'Non-Terminal':<{max_name}}  |  {'FIRST':<30}  |  {'FOLLOW':<30}")
    print(f"  {'-' * max_name}--+--{'-' * 30}--+--{'-' * 30}")

    for nt in nonterminals:
        first_str = "{ " + ", ".join(sorted(first_sets[nt])) + " }"
        follow_str = "{ " + ", ".join(sorted(follow_sets[nt])) + " }"
        print(f"  {nt:<{max_name}}  |  {first_str:<30}  |  {follow_str:<30}")


def main():
    print("=" * 72)
    print("  Exercise 5: FIRST and FOLLOW Sets Computation")
    print("=" * 72)

    # --- Example 1: Standard expression grammar (after left-recursion removal) ---
    grammar_text_1 = """
        E -> T R
        R -> + T R | epsilon
        T -> F Y
        Y -> * F Y | epsilon
        F -> ( E ) | id
    """
    print("\n--- Example 1: Expression Grammar ---\n")
    grammar_1 = parse_grammar(grammar_text_1)
    print("Grammar:")
    for nt, prods in grammar_1.items():
        prod_strs = [" ".join(p) for p in prods]
        print(f"  {nt} -> {' | '.join(prod_strs)}")

    first_1 = compute_first(grammar_1)
    follow_1 = compute_follow(grammar_1, first_1)
    print_sets(first_1, follow_1, grammar_1)

    # --- Example 2: Another grammar ---
    grammar_text_2 = """
        S -> a B D h
        B -> c C
        C -> b C | epsilon
        D -> E F
        E -> g | epsilon
        F -> f | epsilon
    """
    print("\n" + "-" * 72)
    print("\n--- Example 2: Grammar with Multiple Epsilon Productions ---\n")
    grammar_2 = parse_grammar(grammar_text_2)
    print("Grammar:")
    for nt, prods in grammar_2.items():
        prod_strs = [" ".join(p) for p in prods]
        print(f"  {nt} -> {' | '.join(prod_strs)}")

    first_2 = compute_first(grammar_2)
    follow_2 = compute_follow(grammar_2, first_2)
    print_sets(first_2, follow_2, grammar_2)

    # --- Example 3: Simple grammar ---
    grammar_text_3 = """
        S -> A B
        A -> a | epsilon
        B -> b | epsilon
    """
    print("\n" + "-" * 72)
    print("\n--- Example 3: Two Nullable Non-Terminals ---\n")
    grammar_3 = parse_grammar(grammar_text_3)
    print("Grammar:")
    for nt, prods in grammar_3.items():
        prod_strs = [" ".join(p) for p in prods]
        print(f"  {nt} -> {' | '.join(prod_strs)}")

    first_3 = compute_first(grammar_3)
    follow_3 = compute_follow(grammar_3, first_3)
    print_sets(first_3, follow_3, grammar_3)

    print("\n" + "=" * 72)


if __name__ == "__main__":
    main()
