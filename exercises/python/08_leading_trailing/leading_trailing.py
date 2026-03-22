"""
==============================================================================
Exercise 8: LEADING and TRAILING Sets for Operator Grammars
==============================================================================

CONCEPT:
In operator precedence parsing, LEADING and TRAILING sets are used to
establish precedence relations between terminal symbols. An operator grammar
is a context-free grammar where no production right-hand side contains two
adjacent non-terminals.

LEADING(A) is the set of terminals 'a' such that there exists a derivation
A =>* Ba... (i.e., 'a' can appear as the first terminal in some string
derived from non-terminal A, possibly preceded by non-terminals).

TRAILING(A) is the set of terminals 'a' such that there exists a derivation
A =>* ...aB (i.e., 'a' can appear as the last terminal in some string
derived from non-terminal A, possibly followed by non-terminals).

ALGORITHM:
We use an iterative fixed-point computation:
  1. Initialize LEADING(A) with terminals that appear as the first terminal
     in any production of A (scanning from the left).
  2. If a production A -> B... starts with a non-terminal B, add
     LEADING(B) to LEADING(A).
  3. Repeat step 2 until no changes occur (fixed-point).
  Similarly for TRAILING but scanning from the right.

TIME COMPLEXITY:  O(|P| * |V|) where |P| is total production length and
                  |V| is the number of grammar symbols.
SPACE COMPLEXITY: O(|N| * |T|) for storing the sets, where |N| is the
                  number of non-terminals and |T| the number of terminals.

EXAMPLE INPUT:
  E -> E + T | T
  T -> T * F | F
  F -> ( E ) | id

EXAMPLE OUTPUT:
  LEADING(E) = { +, *, (, id }
  LEADING(T) = { *, (, id }
  LEADING(F) = { (, id }
  TRAILING(E) = { +, *, ), id }
  TRAILING(T) = { *, ), id }
  TRAILING(F) = { ), id }
==============================================================================
"""


def parse_grammar(grammar_text):
    """
    Parse a grammar specification into a dictionary.
    Each key is a non-terminal, and the value is a list of productions,
    where each production is a list of symbols.
    """
    grammar = {}
    non_terminals = set()
    terminals = set()

    for line in grammar_text.strip().split("\n"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        lhs, rhs = line.split("->")
        lhs = lhs.strip()
        non_terminals.add(lhs)

        alternatives = rhs.split("|")
        productions = []
        for alt in alternatives:
            symbols = alt.strip().split()
            productions.append(symbols)

        if lhs in grammar:
            grammar[lhs].extend(productions)
        else:
            grammar[lhs] = productions

    # Identify terminals: any symbol that is not a non-terminal
    for lhs, productions in grammar.items():
        for production in productions:
            for symbol in production:
                if symbol not in non_terminals:
                    terminals.add(symbol)

    return grammar, non_terminals, terminals


def compute_leading(grammar, non_terminals, terminals):
    """
    Compute LEADING sets using an iterative fixed-point algorithm.

    Rule 1: If A -> a... or A -> Ba..., then a is in LEADING(A).
    Rule 2: If A -> B..., then LEADING(B) is a subset of LEADING(A).
    Repeat until stable.
    """
    leading = {nt: set() for nt in non_terminals}

    # Initial pass: direct terminals from productions
    for non_terminal, productions in grammar.items():
        for production in productions:
            for symbol in production:
                if symbol in terminals:
                    leading[non_terminal].add(symbol)
                    break  # Only the first terminal matters
                elif symbol in non_terminals:
                    # Non-terminal at the start; we will propagate later.
                    # But keep scanning in case a terminal follows immediately.
                    continue
                break  # Should not reach here with well-formed grammar

    # Iterative fixed-point: propagate through non-terminal prefixes
    changed = True
    while changed:
        changed = False
        for non_terminal, productions in grammar.items():
            for production in productions:
                for symbol in production:
                    if symbol in non_terminals:
                        # Add LEADING of this non-terminal
                        before_size = len(leading[non_terminal])
                        leading[non_terminal] |= leading[symbol]
                        if len(leading[non_terminal]) > before_size:
                            changed = True
                        # Continue scanning; the non-terminal might be
                        # followed by a terminal that is also a leading
                        # terminal (but actually, the first terminal we
                        # encounter from the left is the only one to add
                        # directly, and the non-terminal prefix means
                        # we propagate). We break after processing the
                        # prefix of non-terminals and the first terminal.
                        continue
                    elif symbol in terminals:
                        # Already added in initial pass
                        break
                    break

    return leading


def compute_trailing(grammar, non_terminals, terminals):
    """
    Compute TRAILING sets using an iterative fixed-point algorithm.

    Rule 1: If A -> ...a or A -> ...aB, then a is in TRAILING(A).
    Rule 2: If A -> ...B, then TRAILING(B) is a subset of TRAILING(A).
    Repeat until stable.
    """
    trailing = {nt: set() for nt in non_terminals}

    # Initial pass: scan from the right of each production
    for non_terminal, productions in grammar.items():
        for production in productions:
            for symbol in reversed(production):
                if symbol in terminals:
                    trailing[non_terminal].add(symbol)
                    break
                elif symbol in non_terminals:
                    continue
                break

    # Iterative fixed-point: propagate through non-terminal suffixes
    changed = True
    while changed:
        changed = False
        for non_terminal, productions in grammar.items():
            for production in productions:
                for symbol in reversed(production):
                    if symbol in non_terminals:
                        before_size = len(trailing[non_terminal])
                        trailing[non_terminal] |= trailing[symbol]
                        if len(trailing[non_terminal]) > before_size:
                            changed = True
                        continue
                    elif symbol in terminals:
                        break
                    break

    return trailing


def print_sets(title, sets, non_terminal_order):
    """Print LEADING or TRAILING sets in a formatted table."""
    print(f"\n{'=' * 50}")
    print(f"  {title}")
    print(f"{'=' * 50}")
    for nt in non_terminal_order:
        if nt in sets:
            elements = ", ".join(sorted(sets[nt]))
            print(f"  {title.split()[0]}({nt}) = {{ {elements} }}")
    print(f"{'=' * 50}")


def print_grammar(grammar, non_terminal_order):
    """Print the grammar in a readable format."""
    print(f"\n{'=' * 50}")
    print("  Grammar Productions")
    print(f"{'=' * 50}")
    for nt in non_terminal_order:
        if nt in grammar:
            alternatives = [" ".join(prod) for prod in grammar[nt]]
            print(f"  {nt} -> {' | '.join(alternatives)}")
    print(f"{'=' * 50}")


def main():
    # Example grammar: E -> E+T | T, T -> T*F | F, F -> (E) | id
    grammar_text = """
        E -> E + T | T
        T -> T * F | F
        F -> ( E ) | id
    """

    print("=" * 50)
    print("  LEADING and TRAILING Sets Calculator")
    print("  (Operator Precedence Parsing)")
    print("=" * 50)

    grammar, non_terminals, terminals = parse_grammar(grammar_text)

    # Maintain a consistent ordering for output
    non_terminal_order = ["E", "T", "F"]
    # Fall back to sorted order if grammar has different non-terminals
    non_terminal_order = [nt for nt in non_terminal_order if nt in non_terminals]
    for nt in sorted(non_terminals):
        if nt not in non_terminal_order:
            non_terminal_order.append(nt)

    print_grammar(grammar, non_terminal_order)

    print(f"\n  Non-terminals: {{ {', '.join(sorted(non_terminals))} }}")
    print(f"  Terminals:     {{ {', '.join(sorted(terminals))} }}")

    leading = compute_leading(grammar, non_terminals, terminals)
    trailing = compute_trailing(grammar, non_terminals, terminals)

    print_sets("LEADING Sets", leading, non_terminal_order)
    print_sets("TRAILING Sets", trailing, non_terminal_order)

    # Demonstrate with a second grammar
    print("\n\n" + "#" * 50)
    print("  Second Example Grammar")
    print("#" * 50)

    grammar_text_2 = """
        S -> S + A | A
        A -> A * B | B
        B -> ( S ) | a
    """

    grammar2, non_terminals2, terminals2 = parse_grammar(grammar_text_2)
    nt_order2 = ["S", "A", "B"]
    nt_order2 = [nt for nt in nt_order2 if nt in non_terminals2]

    print_grammar(grammar2, nt_order2)

    leading2 = compute_leading(grammar2, non_terminals2, terminals2)
    trailing2 = compute_trailing(grammar2, non_terminals2, terminals2)

    print_sets("LEADING Sets", leading2, nt_order2)
    print_sets("TRAILING Sets", trailing2, nt_order2)


if __name__ == "__main__":
    main()
