"""
================================================================================
Exercise 6: LL(1) Predictive Parser
================================================================================

CONCEPT:
An LL(1) predictive parser is a top-down parser that uses a parsing table to
decide which production to apply. "LL(1)" means: scan input Left-to-right,
produce a Leftmost derivation, using 1 symbol of lookahead. The parser uses an
explicit stack (rather than recursive calls) and a parsing table indexed by
[non-terminal, terminal] pairs.

ALGORITHM:
1. Compute FIRST and FOLLOW sets for the grammar.
2. Build the LL(1) parsing table:
   For each production A -> alpha:
     - For each terminal a in FIRST(alpha), add A -> alpha to M[A, a].
     - If epsilon in FIRST(alpha), for each terminal b in FOLLOW(A),
       add A -> alpha to M[A, b].
     - If epsilon in FIRST(alpha) and $ in FOLLOW(A),
       add A -> alpha to M[A, $].
3. Parse: Initialize stack with [$, S]. Read input left to right.
   - If top == input symbol, pop and advance (match).
   - If top is non-terminal, look up M[top, input]. If entry exists,
     pop top and push production RHS in reverse. If no entry, error.
   - If top == $ and input == $, accept.

COMPLEXITY:
  Table construction: O(n * p * t) where n=non-terminals, p=productions, t=terminals
  Parsing: O(n) where n = input length (each symbol matched exactly once)
  Space: O(n * t) for the table, O(n) for the stack

EXAMPLE:
  Grammar: E -> TR, R -> +TR | epsilon, T -> FY, Y -> *FY | epsilon, F -> (E) | id
  Input: id+id*id
  The parser produces a leftmost derivation via table-driven stack operations.
================================================================================
"""

from collections import OrderedDict

EPSILON = "epsilon"
END_MARKER = "$"


def parse_grammar(grammar_text):
    """Parse grammar text into ordered dict of non-terminal -> list of productions."""
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


def get_nonterminals_and_terminals(grammar):
    """Extract non-terminals (from keys) and terminals (everything else)."""
    nonterminals = set(grammar.keys())
    terminals = set()
    for productions in grammar.values():
        for prod in productions:
            for sym in prod:
                if sym not in nonterminals and sym != EPSILON:
                    terminals.add(sym)
    return nonterminals, terminals


def compute_first(grammar, nonterminals):
    """Compute FIRST sets using iterative fixed-point."""
    first = {nt: set() for nt in nonterminals}

    changed = True
    while changed:
        changed = False
        for nt, productions in grammar.items():
            for prod in productions:
                if prod == [EPSILON]:
                    if EPSILON not in first[nt]:
                        first[nt].add(EPSILON)
                        changed = True
                    continue

                all_eps = True
                for sym in prod:
                    if sym not in nonterminals:
                        if sym not in first[nt]:
                            first[nt].add(sym)
                            changed = True
                        all_eps = False
                        break
                    else:
                        before = len(first[nt])
                        first[nt] |= first[sym] - {EPSILON}
                        if len(first[nt]) > before:
                            changed = True
                        if EPSILON not in first[sym]:
                            all_eps = False
                            break

                if all_eps:
                    if EPSILON not in first[nt]:
                        first[nt].add(EPSILON)
                        changed = True
    return first


def first_of_string(symbols, first_sets, nonterminals):
    """Compute FIRST of a sequence of symbols."""
    result = set()
    all_eps = True
    for sym in symbols:
        if sym not in nonterminals:
            result.add(sym)
            all_eps = False
            break
        else:
            result |= first_sets[sym] - {EPSILON}
            if EPSILON not in first_sets[sym]:
                all_eps = False
                break
    if all_eps:
        result.add(EPSILON)
    return result


def compute_follow(grammar, first_sets, nonterminals):
    """Compute FOLLOW sets using iterative fixed-point."""
    follow = {nt: set() for nt in nonterminals}
    start = list(grammar.keys())[0]
    follow[start].add(END_MARKER)

    changed = True
    while changed:
        changed = False
        for lhs, productions in grammar.items():
            for prod in productions:
                if prod == [EPSILON]:
                    continue
                for i, sym in enumerate(prod):
                    if sym not in nonterminals:
                        continue
                    beta = prod[i + 1:]
                    if beta:
                        fb = first_of_string(beta, first_sets, nonterminals)
                        before = len(follow[sym])
                        follow[sym] |= fb - {EPSILON}
                        if EPSILON in fb:
                            follow[sym] |= follow[lhs]
                        if len(follow[sym]) > before:
                            changed = True
                    else:
                        before = len(follow[sym])
                        follow[sym] |= follow[lhs]
                        if len(follow[sym]) > before:
                            changed = True
    return follow


def build_parsing_table(grammar, first_sets, follow_sets, nonterminals, terminals):
    """
    Build the LL(1) parsing table.
    Returns a dict: table[(non-terminal, terminal)] = production (list of symbols).
    Also returns any conflicts detected.
    """
    table = {}
    conflicts = []
    all_terminals = terminals | {END_MARKER}

    for nt, productions in grammar.items():
        for prod in productions:
            # Compute FIRST of this production
            if prod == [EPSILON]:
                first_alpha = {EPSILON}
            else:
                first_alpha = first_of_string(prod, first_sets, nonterminals)

            # For each terminal in FIRST(alpha), add to table
            for terminal in first_alpha:
                if terminal == EPSILON:
                    continue
                key = (nt, terminal)
                if key in table:
                    conflicts.append(f"Conflict at [{nt}, {terminal}]: "
                                     f"{' '.join(table[key])} vs {' '.join(prod)}")
                table[key] = prod

            # If epsilon in FIRST(alpha), add for each terminal in FOLLOW(A)
            if EPSILON in first_alpha:
                for terminal in follow_sets[nt]:
                    key = (nt, terminal)
                    if key in table:
                        conflicts.append(f"Conflict at [{nt}, {terminal}]: "
                                         f"{' '.join(table[key])} vs {' '.join(prod)}")
                    table[key] = prod

    return table, conflicts


def print_parsing_table(table, grammar, terminals):
    """Print the parsing table in a formatted grid."""
    nonterminals = list(grammar.keys())
    all_terminals = sorted(terminals) + [END_MARKER]

    # Determine column widths
    col_width = 16
    nt_width = max(len(nt) for nt in nonterminals) + 2

    # Header
    header = f"  {'':>{nt_width}} |"
    for t in all_terminals:
        header += f" {t:^{col_width}} |"
    print(header)
    separator = f"  {'':>{nt_width}}-+"
    for _ in all_terminals:
        separator += f"-{'-' * col_width}-+"
    print(separator)

    # Rows
    for nt in nonterminals:
        row = f"  {nt:>{nt_width}} |"
        for t in all_terminals:
            key = (nt, t)
            if key in table:
                entry = f"{nt}->{' '.join(table[key])}"
            else:
                entry = ""
            row += f" {entry:^{col_width}} |"
        print(row)
    print(separator)


def tokenize(input_string):
    """
    Simple tokenizer that splits input into tokens.
    Recognizes: id, +, *, (, )
    """
    tokens = []
    i = 0
    while i < len(input_string):
        ch = input_string[i]
        if ch.isspace():
            i += 1
            continue
        if ch in "+-*/()":
            tokens.append(ch)
            i += 1
        elif input_string[i:i+2] == "id":
            tokens.append("id")
            i += 2
        else:
            tokens.append(ch)
            i += 1
    tokens.append(END_MARKER)
    return tokens


def parse_input(tokens, table, grammar):
    """
    Parse tokens using the LL(1) table-driven algorithm.
    Prints a step-by-step trace.
    """
    start_symbol = list(grammar.keys())[0]
    nonterminals = set(grammar.keys())
    stack = [END_MARKER, start_symbol]

    token_idx = 0
    step = 0

    # Print header
    print(f"\n  {'Step':<6} {'Stack':<30} {'Input':<25} {'Action':<30}")
    print(f"  {'-'*6} {'-'*30} {'-'*25} {'-'*30}")

    while True:
        top = stack[-1]
        current_token = tokens[token_idx]
        remaining = " ".join(tokens[token_idx:])
        stack_str = " ".join(stack)
        step += 1

        if top == END_MARKER and current_token == END_MARKER:
            print(f"  {step:<6} {stack_str:<30} {remaining:<25} {'ACCEPT':<30}")
            print("\n  ** Input successfully parsed! **")
            return True

        if top == current_token:
            # Match
            action = f"Match '{top}'"
            print(f"  {step:<6} {stack_str:<30} {remaining:<25} {action:<30}")
            stack.pop()
            token_idx += 1

        elif top in nonterminals:
            key = (top, current_token)
            if key in table:
                prod = table[key]
                prod_str = " ".join(prod)
                action = f"{top} -> {prod_str}"
                print(f"  {step:<6} {stack_str:<30} {remaining:<25} {action:<30}")
                stack.pop()
                # Push production symbols in reverse (so leftmost is on top)
                if prod != [EPSILON]:
                    for sym in reversed(prod):
                        stack.append(sym)
            else:
                action = f"ERROR: no entry for [{top}, {current_token}]"
                print(f"  {step:<6} {stack_str:<30} {remaining:<25} {action:<30}")
                print(f"\n  ** Parse error at token '{current_token}' **")
                return False
        else:
            action = f"ERROR: mismatch (expected '{top}', got '{current_token}')"
            print(f"  {step:<6} {stack_str:<30} {remaining:<25} {action:<30}")
            print(f"\n  ** Parse error **")
            return False


def main():
    print("=" * 90)
    print("  Exercise 6: LL(1) Predictive Parser")
    print("=" * 90)

    grammar_text = """
        E -> T R
        R -> + T R | epsilon
        T -> F Y
        Y -> * F Y | epsilon
        F -> ( E ) | id
    """

    grammar = parse_grammar(grammar_text)
    nonterminals, terminals = get_nonterminals_and_terminals(grammar)

    print("\nGrammar:")
    for nt, prods in grammar.items():
        prod_strs = [" ".join(p) for p in prods]
        print(f"  {nt} -> {' | '.join(prod_strs)}")

    # Compute FIRST and FOLLOW sets
    first_sets = compute_first(grammar, nonterminals)
    follow_sets = compute_follow(grammar, first_sets, nonterminals)

    print("\nFIRST Sets:")
    for nt in grammar:
        print(f"  FIRST({nt}) = {{ {', '.join(sorted(first_sets[nt]))} }}")

    print("\nFOLLOW Sets:")
    for nt in grammar:
        print(f"  FOLLOW({nt}) = {{ {', '.join(sorted(follow_sets[nt]))} }}")

    # Build parsing table
    table, conflicts = build_parsing_table(
        grammar, first_sets, follow_sets, nonterminals, terminals
    )

    print("\nLL(1) Parsing Table:")
    print_parsing_table(table, grammar, terminals)

    if conflicts:
        print("\n  WARNING: Grammar is not LL(1). Conflicts detected:")
        for c in conflicts:
            print(f"    {c}")
    else:
        print("\n  Grammar is LL(1) (no conflicts).")

    # --- Parse example inputs ---
    test_inputs = ["id+id*id", "id*(id+id)", "id+id"]

    for test_input in test_inputs:
        print(f"\n{'='*90}")
        print(f"\n  Parsing: \"{test_input}\"")
        tokens = tokenize(test_input)
        parse_input(tokens, table, grammar)

    print(f"\n{'='*90}")


if __name__ == "__main__":
    main()
