"""
================================================================================
Exercise 4a: Left Factoring of Context-Free Grammars
================================================================================

CONCEPT:
    Left factoring is a grammar transformation technique used to make a
    context-free grammar suitable for top-down (predictive) parsing.  When
    two or more productions for the same non-terminal begin with the same
    prefix, a top-down parser cannot decide which production to use by
    looking at just the next input symbol.  Left factoring extracts the
    common prefix into a single production and defers the decision to a
    new non-terminal.

    For example, given:
        A -> alpha beta1 | alpha beta2
    Left factoring produces:
        A  -> alpha A'
        A' -> beta1 | beta2

ALGORITHM:
    For each non-terminal A in the grammar:
      1. Find the longest common prefix among all productions for A.
      2. If a common prefix of length >= 1 exists:
         a. Create a new non-terminal A' (or A'', A''' if needed).
         b. Replace matching productions with:  A -> prefix A'
         c. Add:  A' -> suffix1 | suffix2 | ... (using 'eps' for empty).
      3. Repeat until no more common prefixes exist (a non-terminal may
         need multiple rounds of factoring).

COMPLEXITY:
    Time:  O(n * m) per non-terminal, where n = number of productions and
           m = maximum production length.  Total is proportional to grammar
           size times the number of factoring rounds needed.
    Space: O(G) where G is the size of the grammar.

EXAMPLE:
    Input:
        S -> aAB | aAc | bBc
        A -> dA | e
    Output (after left factoring):
        S  -> aAS' | bBc
        S' -> B | c
        A  -> dA | e
================================================================================
"""

import copy
from collections import OrderedDict

# ---------------------------------------------------------------------------
# Grammar representation
# ---------------------------------------------------------------------------
# A grammar is an OrderedDict:  non-terminal -> list of productions
# Each production is a list of symbols (strings).
# "eps" represents the empty string (epsilon).

def parse_grammar(text):
    """Parse a grammar from multi-line text.  Format:  A -> alpha | beta"""
    grammar = OrderedDict()
    for line in text.strip().split("\n"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        lhs, rhs = line.split("->", 1)
        lhs = lhs.strip()
        alternatives = rhs.strip().split("|")
        prods = []
        for alt in alternatives:
            symbols = alt.strip().split()
            if symbols:
                prods.append(symbols)
        grammar[lhs] = prods
    return grammar


def grammar_to_string(grammar):
    """Pretty-format a grammar back to readable text."""
    lines = []
    for nt, prods in grammar.items():
        rhs_parts = []
        for prod in prods:
            rhs_parts.append(" ".join(prod))
        lines.append(f"  {nt} -> {' | '.join(rhs_parts)}")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Left factoring
# ---------------------------------------------------------------------------
def longest_common_prefix(productions):
    """
    Find the longest common prefix shared by two or more productions.
    Returns the prefix (a list of symbols), or an empty list if no common
    prefix of length >= 1 exists among any pair of productions.
    """
    if len(productions) < 2:
        return []

    # Try decreasing prefix lengths.  For each candidate length, check if
    # at least two productions share that prefix.
    max_len = max(len(p) for p in productions)

    best_prefix = []
    for length in range(1, max_len + 1):
        # group productions by their prefix of this length
        groups = {}
        for prod in productions:
            if len(prod) >= length:
                key = tuple(prod[:length])
                groups.setdefault(key, []).append(prod)

        # find the longest prefix shared by >= 2 productions
        found = False
        for prefix_tuple, members in groups.items():
            if len(members) >= 2:
                best_prefix = list(prefix_tuple)
                found = True
                break  # take the first group found at this length
        if not found:
            break  # no group of size >= 2 at this length, stop

    return best_prefix


def new_nonterminal(base, grammar):
    """Generate a new non-terminal name like A', A'', etc."""
    candidate = base + "'"
    while candidate in grammar:
        candidate += "'"
    return candidate


def left_factor_once(grammar):
    """
    Perform one round of left factoring.
    Returns (new_grammar, changed) where changed is True if any factoring was done.
    """
    new_grammar = OrderedDict()
    changed = False

    for nt, prods in grammar.items():
        prefix = longest_common_prefix(prods)

        if not prefix:
            # no common prefix -- keep productions as-is
            new_grammar[nt] = prods
            continue

        changed = True
        prefix_len = len(prefix)

        # Partition productions into those that match the prefix and those that don't
        matching = []
        non_matching = []
        for prod in prods:
            if prod[:prefix_len] == prefix:
                matching.append(prod)
            else:
                non_matching.append(prod)

        # Create new non-terminal for the suffixes
        new_nt = new_nonterminal(nt, grammar)

        # The original non-terminal gets:  prefix new_nt  +  non_matching prods
        factored_prod = prefix + [new_nt]
        new_prods = [factored_prod] + non_matching
        new_grammar[nt] = new_prods

        # New non-terminal gets the suffixes
        suffix_prods = []
        for prod in matching:
            suffix = prod[prefix_len:]
            if not suffix:
                suffix = ["eps"]  # empty production
            suffix_prods.append(suffix)
        new_grammar[new_nt] = suffix_prods

    return new_grammar, changed


def left_factor(grammar):
    """Repeatedly apply left factoring until no more changes occur."""
    current = copy.deepcopy(grammar)
    iteration = 0
    while True:
        iteration += 1
        new_grammar, changed = left_factor_once(current)
        if not changed:
            break
        current = new_grammar
        # Safety limit
        if iteration > 50:
            print("  [Warning: exceeded 50 iterations, stopping.]")
            break
    return current


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
EXAMPLE_GRAMMAR = """
S -> a A B | a A c | b B c
A -> d A | e
"""

EXAMPLE_GRAMMAR_2 = """
E -> T + E | T
T -> int | int * T | ( E )
"""

def main():
    print("=" * 70)
    print("  Left Factoring of Context-Free Grammars  --  Exercise 4a")
    print("=" * 70)

    print("\nChoose input mode:")
    print("  1) Example 1:  S -> aAB | aAc | bBc,  A -> dA | e")
    print("  2) Example 2:  E -> T+E | T,  T -> int | int*T | (E)")
    print("  3) Enter your own grammar")
    choice = input("\nEnter choice [1]: ").strip()

    if choice == "2":
        grammar = parse_grammar(EXAMPLE_GRAMMAR_2)
    elif choice == "3":
        print("\nEnter grammar (one rule per line, format: A -> alpha | beta)")
        print("End with an empty line:\n")
        lines = []
        while True:
            line = input()
            if not line.strip():
                break
            lines.append(line)
        grammar = parse_grammar("\n".join(lines))
    else:
        grammar = parse_grammar(EXAMPLE_GRAMMAR)

    print("\n--- Original Grammar ---")
    print(grammar_to_string(grammar))

    factored = left_factor(grammar)

    print("\n--- Left-Factored Grammar ---")
    print(grammar_to_string(factored))

    # Show what changed
    original_nts = set(grammar.keys())
    factored_nts = set(factored.keys())
    new_nts = factored_nts - original_nts
    if new_nts:
        print(f"\n  New non-terminals introduced: {', '.join(sorted(new_nts))}")
    else:
        print("\n  No factoring was necessary (grammar already factored).")


if __name__ == "__main__":
    main()
