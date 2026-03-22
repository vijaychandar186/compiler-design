"""
================================================================================
Exercise 4b: Left Recursion Elimination
================================================================================

CONCEPT:
Left recursion in a context-free grammar occurs when a non-terminal A can
derive a sentential form starting with itself: A =>+ A alpha. This is
problematic for top-down (recursive descent / LL) parsers because they would
enter infinite loops trying to expand A. Left recursion comes in two forms:

  - Direct left recursion:   A -> A alpha | beta
  - Indirect left recursion: A -> B alpha, B -> A beta (A derives A through B)

ALGORITHM:
For direct left recursion, the transformation is straightforward:
  Original:    A -> A alpha1 | A alpha2 | ... | beta1 | beta2 | ...
  Transformed: A  -> beta1 A' | beta2 A' | ...
               A' -> alpha1 A' | alpha2 A' | ... | epsilon

For indirect left recursion, we use the standard ordering algorithm:
  1. Order all non-terminals as A1, A2, ..., An.
  2. For i = 1 to n:
       For j = 1 to i-1:
         Replace each production Ai -> Aj gamma with
         Ai -> delta1 gamma | delta2 gamma | ... where Aj -> delta1 | delta2 | ...
       Eliminate direct left recursion from Ai productions.

COMPLEXITY:
  Time:  O(n^2 * p) where n = number of non-terminals, p = max productions
  Space: O(n * p) for storing the grammar

EXAMPLE:
  Input:   E -> E+T | T,  T -> T*F | F,  F -> (E) | id
  Output:  E  -> T E'
           E' -> +T E' | epsilon
           T  -> F T'
           T' -> *F T' | epsilon
           F  -> (E) | id
================================================================================
"""

from collections import OrderedDict


def parse_grammar(grammar_text):
    """
    Parse a grammar string into an ordered dictionary.
    Each key is a non-terminal, each value is a list of productions (each production
    is a list of symbols).
    """
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
        if lhs in grammar:
            grammar[lhs].extend(productions)
        else:
            grammar[lhs] = productions
    return grammar


def grammar_to_string(grammar):
    """Format a grammar dictionary as a readable string."""
    lines = []
    for nonterminal, productions in grammar.items():
        prod_strings = [" ".join(prod) for prod in productions]
        lines.append(f"  {nonterminal} -> {' | '.join(prod_strings)}")
    return "\n".join(lines)


def eliminate_direct_left_recursion(nonterminal, productions):
    """
    Eliminate direct left recursion for a single non-terminal.
    Given A -> A alpha1 | A alpha2 | ... | beta1 | beta2 | ...
    Produces:
      A  -> beta1 A' | beta2 A' | ...
      A' -> alpha1 A' | alpha2 A' | ... | epsilon
    Returns (new_productions_for_A, new_nonterminal_name, new_nonterminal_productions)
    or (original_productions, None, None) if no left recursion found.
    """
    recursive_prods = []     # productions starting with the nonterminal itself
    non_recursive_prods = [] # all other productions

    for prod in productions:
        if prod[0] == nonterminal:
            recursive_prods.append(prod[1:])  # alpha part (without leading A)
        else:
            non_recursive_prods.append(prod)

    if not recursive_prods:
        return productions, None, None

    # Create the new non-terminal name (A')
    new_nonterminal = nonterminal + "'"
    # Avoid collisions with existing names by adding more primes
    # (handled externally if needed)

    # A -> beta1 A' | beta2 A' | ...
    new_a_productions = []
    for prod in non_recursive_prods:
        if prod == ["epsilon"]:
            new_a_productions.append([new_nonterminal])
        else:
            new_a_productions.append(prod + [new_nonterminal])

    # A' -> alpha1 A' | alpha2 A' | ... | epsilon
    new_prime_productions = []
    for alpha in recursive_prods:
        if alpha:  # alpha is non-empty
            new_prime_productions.append(alpha + [new_nonterminal])
        else:
            new_prime_productions.append([new_nonterminal])
    new_prime_productions.append(["epsilon"])

    return new_a_productions, new_nonterminal, new_prime_productions


def substitute_productions(grammar, nonterminal_i, nonterminal_j):
    """
    For every production of Ai that starts with Aj, replace it by substituting
    all productions of Aj in place of Aj.
    Ai -> Aj gamma becomes Ai -> delta1 gamma | delta2 gamma | ...
    where Aj -> delta1 | delta2 | ...
    """
    new_productions = []
    for prod in grammar[nonterminal_i]:
        if prod[0] == nonterminal_j:
            # Substitute: replace Aj with each of its alternatives
            gamma = prod[1:]  # the part after Aj
            for delta in grammar[nonterminal_j]:
                if delta == ["epsilon"]:
                    if gamma:
                        new_productions.append(gamma)
                    else:
                        new_productions.append(["epsilon"])
                else:
                    new_productions.append(delta + gamma)
        else:
            new_productions.append(prod)
    grammar[nonterminal_i] = new_productions


def eliminate_left_recursion(grammar):
    """
    Eliminate all left recursion (both direct and indirect) using the
    standard ordering algorithm.
    """
    nonterminals = list(grammar.keys())
    new_grammar = OrderedDict(grammar)

    for i, nt_i in enumerate(nonterminals):
        # For each earlier non-terminal, substitute its productions
        for j in range(i):
            nt_j = nonterminals[j]
            substitute_productions(new_grammar, nt_i, nt_j)

        # Now eliminate any direct left recursion for nt_i
        new_prods, new_nt, new_nt_prods = eliminate_direct_left_recursion(
            nt_i, new_grammar[nt_i]
        )
        new_grammar[nt_i] = new_prods
        if new_nt is not None:
            # Insert the new non-terminal right after nt_i in the ordered dict
            items = list(new_grammar.items())
            idx = list(new_grammar.keys()).index(nt_i) + 1
            items.insert(idx, (new_nt, new_nt_prods))
            new_grammar = OrderedDict(items)

    return new_grammar


def main():
    print("=" * 70)
    print("  Exercise 4b: Left Recursion Elimination")
    print("=" * 70)

    # --- Example 1: Classic expression grammar with direct left recursion ---
    grammar_text_1 = """
        E -> E + T | T
        T -> T * F | F
        F -> ( E ) | id
    """
    print("\n--- Example 1: Direct Left Recursion ---")
    grammar_1 = parse_grammar(grammar_text_1)
    print("\nOriginal Grammar:")
    print(grammar_to_string(grammar_1))

    result_1 = eliminate_left_recursion(grammar_1)
    print("\nAfter Left Recursion Elimination:")
    print(grammar_to_string(result_1))

    # --- Example 2: Indirect left recursion ---
    grammar_text_2 = """
        S -> A a | b
        A -> S c | d
    """
    print("\n" + "-" * 70)
    print("\n--- Example 2: Indirect Left Recursion ---")
    grammar_2 = parse_grammar(grammar_text_2)
    print("\nOriginal Grammar:")
    print(grammar_to_string(grammar_2))

    result_2 = eliminate_left_recursion(grammar_2)
    print("\nAfter Left Recursion Elimination:")
    print(grammar_to_string(result_2))

    # --- Example 3: No left recursion (should remain unchanged) ---
    grammar_text_3 = """
        S -> a B | c
        B -> b S | d
    """
    print("\n" + "-" * 70)
    print("\n--- Example 3: No Left Recursion (unchanged) ---")
    grammar_3 = parse_grammar(grammar_text_3)
    print("\nOriginal Grammar:")
    print(grammar_to_string(grammar_3))

    result_3 = eliminate_left_recursion(grammar_3)
    print("\nAfter Left Recursion Elimination:")
    print(grammar_to_string(result_3))

    # --- Example 4: Multiple recursive alternatives ---
    grammar_text_4 = """
        A -> A b c | A d | e | f g
    """
    print("\n" + "-" * 70)
    print("\n--- Example 4: Multiple Recursive Alternatives ---")
    grammar_4 = parse_grammar(grammar_text_4)
    print("\nOriginal Grammar:")
    print(grammar_to_string(grammar_4))

    result_4 = eliminate_left_recursion(grammar_4)
    print("\nAfter Left Recursion Elimination:")
    print(grammar_to_string(result_4))

    print("\n" + "=" * 70)


if __name__ == "__main__":
    main()
