"""
==============================================================================
Exercise 9: LR(0) Parser - Canonical Collection of Item Sets
==============================================================================

CONCEPT:
An LR(0) item is a production with a dot (.) at some position in the
right-hand side, indicating how much of the production has been seen so far.
For example, from E -> E + T we get items:
  E -> . E + T    (nothing seen yet)
  E -> E . + T    (E has been seen)
  E -> E + . T    (E + has been seen)
  E -> E + T .    (complete item; ready to reduce)

The canonical collection of LR(0) item sets is the foundation for building
SLR(1), LALR(1), and canonical LR(1) parsers. Each item set represents a
parser state in the deterministic finite automaton used for shift-reduce
parsing.

ALGORITHM:
  1. Augment the grammar: add S' -> S (new start symbol).
  2. CLOSURE(I): For each item A -> alpha . B beta in I, add all items
     B -> . gamma for every production B -> gamma.
  3. GOTO(I, X): For each item A -> alpha . X beta in I, move the dot
     past X to get A -> alpha X . beta, then take the closure.
  4. Start with I0 = CLOSURE({S' -> . S}).
  5. For each item set and each grammar symbol, compute GOTO. If the
     result is a new set, add it to the collection. Repeat until no new
     sets are generated.

TIME COMPLEXITY:  O(|G|^2 * |V|) in the worst case, where |G| is grammar
                  size and |V| is the number of symbols.
SPACE COMPLEXITY: O(|G|^2) for storing all item sets.

EXAMPLE INPUT:
  E -> E + T | T
  T -> T * F | F
  F -> ( E ) | id

EXAMPLE OUTPUT:
  I0: { E' -> . E, E -> . E + T, E -> . T, T -> . T * F, ... }
  I1: { E' -> E ., E -> E . + T }
  ... (12 item sets total for this grammar)
==============================================================================
"""


class LR0Item:
    """
    Represents an LR(0) item: a production with a dot position.
    For production A -> X Y Z, dot_pos=1 means A -> X . Y Z.
    """

    def __init__(self, lhs, rhs, dot_pos=0):
        self.lhs = lhs
        self.rhs = tuple(rhs)
        self.dot_pos = dot_pos

    def is_complete(self):
        """The dot is at the end; this item is ready for reduction."""
        return self.dot_pos >= len(self.rhs)

    def symbol_after_dot(self):
        """Return the symbol immediately after the dot, or None if complete."""
        if self.is_complete():
            return None
        return self.rhs[self.dot_pos]

    def advance(self):
        """Return a new item with the dot moved one position to the right."""
        return LR0Item(self.lhs, self.rhs, self.dot_pos + 1)

    def __eq__(self, other):
        return (self.lhs == other.lhs and
                self.rhs == other.rhs and
                self.dot_pos == other.dot_pos)

    def __hash__(self):
        return hash((self.lhs, self.rhs, self.dot_pos))

    def __repr__(self):
        rhs_list = list(self.rhs)
        rhs_list.insert(self.dot_pos, ".")
        return f"{self.lhs} -> {' '.join(rhs_list)}"


class LR0Parser:
    """
    Builds the canonical collection of LR(0) item sets for a given grammar.
    """

    def __init__(self, grammar, start_symbol):
        """
        grammar: dict mapping non-terminal -> list of productions,
                 where each production is a list of symbols.
        start_symbol: the original start symbol (will be augmented).
        """
        self.original_start = start_symbol
        self.augmented_start = start_symbol + "'"

        # Augment the grammar
        self.grammar = {self.augmented_start: [[start_symbol]]}
        for lhs, prods in grammar.items():
            self.grammar[lhs] = [list(p) for p in prods]

        self.non_terminals = set(self.grammar.keys())
        self.terminals = set()
        for prods in self.grammar.values():
            for prod in prods:
                for sym in prod:
                    if sym not in self.non_terminals:
                        self.terminals.add(sym)

        self.all_symbols = self.non_terminals | self.terminals
        self.item_sets = []    # List of frozensets of LR0Items
        self.goto_table = {}   # (state_index, symbol) -> state_index

    def closure(self, items):
        """
        Compute the closure of a set of LR(0) items.
        For every item A -> alpha . B beta where B is a non-terminal,
        add B -> . gamma for all productions of B.
        """
        closure_set = set(items)
        worklist = list(items)

        while worklist:
            item = worklist.pop()
            next_sym = item.symbol_after_dot()

            if next_sym is not None and next_sym in self.non_terminals:
                for production in self.grammar[next_sym]:
                    new_item = LR0Item(next_sym, production, 0)
                    if new_item not in closure_set:
                        closure_set.add(new_item)
                        worklist.append(new_item)

        return frozenset(closure_set)

    def goto(self, item_set, symbol):
        """
        Compute GOTO(item_set, symbol).
        Move the dot past 'symbol' for all items where the dot precedes it,
        then take the closure.
        """
        moved_items = set()
        for item in item_set:
            if item.symbol_after_dot() == symbol:
                moved_items.add(item.advance())

        if not moved_items:
            return frozenset()

        return self.closure(moved_items)

    def build_canonical_collection(self):
        """
        Build the canonical collection of LR(0) item sets.
        Starts with I0 = closure({S' -> . S}) and iteratively computes
        GOTO for all symbols until no new item sets are found.
        """
        initial_item = LR0Item(self.augmented_start,
                               self.grammar[self.augmented_start][0], 0)
        start_set = self.closure({initial_item})
        self.item_sets = [start_set]

        # Map from frozenset -> index for quick lookup
        set_to_index = {start_set: 0}

        worklist = [0]
        while worklist:
            state_index = worklist.pop(0)
            current_set = self.item_sets[state_index]

            # Find all symbols that appear after a dot in this set
            symbols_after_dot = set()
            for item in current_set:
                sym = item.symbol_after_dot()
                if sym is not None:
                    symbols_after_dot.add(sym)

            for symbol in sorted(symbols_after_dot):
                goto_set = self.goto(current_set, symbol)
                if not goto_set:
                    continue

                if goto_set not in set_to_index:
                    new_index = len(self.item_sets)
                    self.item_sets.append(goto_set)
                    set_to_index[goto_set] = new_index
                    worklist.append(new_index)

                self.goto_table[(state_index, symbol)] = set_to_index[goto_set]

    def print_augmented_grammar(self):
        """Print the augmented grammar."""
        print("=" * 60)
        print("  Augmented Grammar")
        print("=" * 60)

        # Print augmented production first
        prod_number = 0
        for lhs in [self.augmented_start] + sorted(
                k for k in self.grammar if k != self.augmented_start):
            for rhs in self.grammar[lhs]:
                print(f"  ({prod_number}) {lhs} -> {' '.join(rhs)}")
                prod_number += 1
        print("=" * 60)

    def print_item_sets(self):
        """Print all item sets in the canonical collection."""
        print(f"\n{'=' * 60}")
        print(f"  Canonical Collection of LR(0) Item Sets")
        print(f"  Total states: {len(self.item_sets)}")
        print(f"{'=' * 60}")

        for idx, item_set in enumerate(self.item_sets):
            print(f"\n  I{idx}:")
            print(f"  {'-' * 40}")

            # Sort items for consistent output: augmented start first,
            # then by LHS, then by production, then by dot position
            sorted_items = sorted(
                item_set,
                key=lambda it: (
                    0 if it.lhs == self.augmented_start else 1,
                    it.lhs, it.rhs, it.dot_pos
                )
            )
            for item in sorted_items:
                print(f"    {item}")

        print(f"\n{'=' * 60}")

    def print_goto_table(self):
        """Print the GOTO transitions."""
        print(f"\n{'=' * 60}")
        print(f"  GOTO Transitions")
        print(f"{'=' * 60}")

        for (state, symbol), target in sorted(self.goto_table.items()):
            print(f"  GOTO(I{state}, {symbol}) = I{target}")

        print(f"{'=' * 60}")


def parse_grammar_text(grammar_text):
    """Parse a text-based grammar specification into a dictionary."""
    grammar = {}
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


def main():
    print("=" * 60)
    print("  LR(0) Parser - Canonical Collection Constructor")
    print("=" * 60)

    grammar_text = """
        E -> E + T | T
        T -> T * F | F
        F -> ( E ) | id
    """

    grammar = parse_grammar_text(grammar_text)
    start_symbol = "E"

    parser = LR0Parser(grammar, start_symbol)
    parser.print_augmented_grammar()
    parser.build_canonical_collection()
    parser.print_item_sets()
    parser.print_goto_table()

    # Second smaller example
    print("\n\n" + "#" * 60)
    print("  Second Example: S -> A A, A -> a A | b")
    print("#" * 60)

    grammar_text_2 = """
        S -> A A
        A -> a A | b
    """

    grammar2 = parse_grammar_text(grammar_text_2)
    parser2 = LR0Parser(grammar2, "S")
    parser2.print_augmented_grammar()
    parser2.build_canonical_collection()
    parser2.print_item_sets()
    parser2.print_goto_table()


if __name__ == "__main__":
    main()
