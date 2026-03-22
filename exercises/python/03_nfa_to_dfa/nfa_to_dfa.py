"""
================================================================================
Exercise 3: NFA to DFA Conversion (Subset Construction / Powerset Construction)
================================================================================

CONCEPT:
    A Non-deterministic Finite Automaton (NFA) can have multiple transitions
    from a single state on the same input symbol, as well as epsilon (empty)
    transitions.  A Deterministic Finite Automaton (DFA) has exactly one
    transition per state per symbol and no epsilon transitions.  Every NFA
    can be converted to an equivalent DFA using the subset construction
    algorithm.

ALGORITHM (Subset Construction):
    1. Compute epsilon-closure(start_state) -- the set of NFA states
       reachable from the start state via epsilon transitions alone.
       This set becomes the DFA start state.
    2. For each unmarked DFA state D (which is a set of NFA states):
       a. Mark D.
       b. For each input symbol 'a':
          - Compute move(D, a) = union of NFA transitions on 'a' from
            every state in D.
          - Compute epsilon-closure(move(D, a)).  This is a new DFA state.
          - If this DFA state is not already in the table, add it.
          - Record the transition D --a--> new_state.
    3. A DFA state is an accepting state if it contains any NFA accepting
       state.

    The epsilon-closure itself is computed by a BFS/DFS from a given set of
    states, following only epsilon edges.

COMPLEXITY:
    Time:  O(2^n * m) in the worst case, where n = number of NFA states and
           m = number of input symbols.  In practice, most DFA states are
           unreachable so the actual number is far smaller.
    Space: O(2^n) for the DFA state table (worst case).

EXAMPLE:
    NFA for (a|b)*abb  ->  DFA with 5 states.
================================================================================
"""

from collections import deque

EPSILON = "eps"

# ---------------------------------------------------------------------------
# NFA representation
# ---------------------------------------------------------------------------
class NFAState:
    """Represents a single NFA state with its transition map."""
    def __init__(self, state_id):
        self.id = state_id
        self.transitions = {}   # symbol -> set of state ids

    def add_transition(self, symbol, target_id):
        self.transitions.setdefault(symbol, set()).add(target_id)


class NFAutomaton:
    """Complete NFA with states, alphabet, start, and accept states."""
    def __init__(self):
        self.states = {}        # id -> NFAState
        self.alphabet = set()   # input symbols (no epsilon)
        self.start = None       # start state id
        self.accept_states = set()

    def add_state(self, state_id):
        if state_id not in self.states:
            self.states[state_id] = NFAState(state_id)

    def add_transition(self, from_id, symbol, to_id):
        self.add_state(from_id)
        self.add_state(to_id)
        self.states[from_id].add_transition(symbol, to_id)
        if symbol != EPSILON:
            self.alphabet.add(symbol)

    def epsilon_closure(self, state_set):
        """Compute the epsilon-closure of a set of NFA state ids."""
        closure = set(state_set)
        worklist = deque(state_set)
        while worklist:
            state_id = worklist.popleft()
            eps_targets = self.states[state_id].transitions.get(EPSILON, set())
            for target in eps_targets:
                if target not in closure:
                    closure.add(target)
                    worklist.append(target)
        return frozenset(closure)

    def move(self, state_set, symbol):
        """Compute move(state_set, symbol): union of transitions on symbol."""
        result = set()
        for state_id in state_set:
            targets = self.states[state_id].transitions.get(symbol, set())
            result |= targets
        return result


# ---------------------------------------------------------------------------
# DFA representation
# ---------------------------------------------------------------------------
class DFAState:
    """A DFA state, which corresponds to a set of NFA states."""
    def __init__(self, dfa_id, nfa_state_set):
        self.id = dfa_id
        self.nfa_states = nfa_state_set   # frozenset of NFA ids
        self.transitions = {}             # symbol -> dfa_id
        self.is_accept = False


class DFAutomaton:
    def __init__(self):
        self.states = {}        # id -> DFAState
        self.start = None
        self.alphabet = set()


# ---------------------------------------------------------------------------
# Subset construction
# ---------------------------------------------------------------------------
def subset_construction(nfa):
    """Convert an NFA to a DFA using the subset construction algorithm."""
    dfa = DFAutomaton()
    dfa.alphabet = set(nfa.alphabet)

    # Map from frozenset of NFA states -> DFA state id
    state_map = {}
    dfa_id_counter = 0

    # 1. Start state = epsilon-closure({nfa.start})
    start_closure = nfa.epsilon_closure({nfa.start})
    state_map[start_closure] = dfa_id_counter
    start_dfa = DFAState(dfa_id_counter, start_closure)
    start_dfa.is_accept = bool(start_closure & nfa.accept_states)
    dfa.states[dfa_id_counter] = start_dfa
    dfa.start = dfa_id_counter
    dfa_id_counter += 1

    # 2. Worklist algorithm
    worklist = deque([start_closure])

    while worklist:
        current_nfa_set = worklist.popleft()
        current_dfa_id = state_map[current_nfa_set]

        for symbol in sorted(dfa.alphabet):
            # move then epsilon-closure
            moved = nfa.move(current_nfa_set, symbol)
            if not moved:
                continue
            new_closure = nfa.epsilon_closure(moved)

            if new_closure not in state_map:
                state_map[new_closure] = dfa_id_counter
                new_dfa = DFAState(dfa_id_counter, new_closure)
                new_dfa.is_accept = bool(new_closure & nfa.accept_states)
                dfa.states[dfa_id_counter] = new_dfa
                dfa_id_counter += 1
                worklist.append(new_closure)

            dfa.states[current_dfa_id].transitions[symbol] = state_map[new_closure]

    return dfa


# ---------------------------------------------------------------------------
# Printing
# ---------------------------------------------------------------------------
def print_nfa(nfa):
    """Print NFA transition table."""
    symbols = [EPSILON] + sorted(nfa.alphabet)
    state_ids = sorted(nfa.states.keys())

    col_w = 18
    state_w = 10
    print(f"{'STATE':<{state_w}}", end="")
    for sym in symbols:
        print(f"| {sym:^{col_w}}", end="")
    print()
    print("-" * (state_w + len(symbols) * (col_w + 2)))

    for sid in state_ids:
        state = nfa.states[sid]
        prefix = ""
        if sid == nfa.start:
            prefix += "->"
        if sid in nfa.accept_states:
            prefix += "*"
        label = f"{prefix}q{sid}"
        print(f"{label:<{state_w}}", end="")
        for sym in symbols:
            targets = state.transitions.get(sym, set())
            cell = ", ".join(f"q{t}" for t in sorted(targets)) if targets else "-"
            print(f"| {cell:^{col_w}}", end="")
        print()

    print("-" * (state_w + len(symbols) * (col_w + 2)))


def print_dfa(dfa):
    """Print DFA transition table with corresponding NFA state sets."""
    symbols = sorted(dfa.alphabet)
    state_ids = sorted(dfa.states.keys())

    col_w = 10
    state_w = 10
    set_w = 24

    print(f"{'STATE':<{state_w}}{'NFA STATES':<{set_w}}", end="")
    for sym in symbols:
        print(f"| {sym:^{col_w}}", end="")
    print()
    print("-" * (state_w + set_w + len(symbols) * (col_w + 2)))

    for sid in state_ids:
        ds = dfa.states[sid]
        prefix = ""
        if sid == dfa.start:
            prefix += "->"
        if ds.is_accept:
            prefix += "*"
        label = f"{prefix}D{sid}"

        nfa_set_str = "{" + ", ".join(f"q{s}" for s in sorted(ds.nfa_states)) + "}"

        print(f"{label:<{state_w}}{nfa_set_str:<{set_w}}", end="")
        for sym in symbols:
            target = ds.transitions.get(sym, None)
            cell = f"D{target}" if target is not None else "-"
            print(f"| {cell:^{col_w}}", end="")
        print()

    print("-" * (state_w + set_w + len(symbols) * (col_w + 2)))


# ---------------------------------------------------------------------------
# Build the example NFA for (a|b)*abb
# ---------------------------------------------------------------------------
def build_example_nfa():
    """
    Build the classic NFA for (a|b)*abb with the following structure:

    State 0 (start) -- eps --> 1, 7
    State 1         -- eps --> 2, 4
    State 2         -- a   --> 3
    State 3         -- eps --> 6
    State 4         -- b   --> 5
    State 5         -- eps --> 6
    State 6         -- eps --> 1, 7
    State 7         -- a   --> 8
    State 8         -- b   --> 9
    State 9         -- b   --> 10 (accept)
    """
    nfa = NFAutomaton()
    nfa.start = 0
    nfa.accept_states = {10}

    transitions = [
        (0, EPSILON, 1), (0, EPSILON, 7),
        (1, EPSILON, 2), (1, EPSILON, 4),
        (2, "a", 3),
        (3, EPSILON, 6),
        (4, "b", 5),
        (5, EPSILON, 6),
        (6, EPSILON, 1), (6, EPSILON, 7),
        (7, "a", 8),
        (8, "b", 9),
        (9, "b", 10),
    ]
    for (src, sym, dst) in transitions:
        nfa.add_transition(src, sym, dst)

    return nfa


def build_nfa_from_user():
    """Interactive NFA input from the user."""
    print("\nEnter NFA details:")
    num_states = int(input("  Number of states: "))
    for i in range(num_states):
        pass  # states 0..n-1

    nfa = NFAutomaton()
    for i in range(num_states):
        nfa.add_state(i)

    nfa.start = int(input("  Start state: "))
    accept_input = input("  Accept states (comma-separated): ")
    nfa.accept_states = set(int(x.strip()) for x in accept_input.split(","))

    num_trans = int(input("  Number of transitions: "))
    print("  Enter each transition as: from symbol to")
    print("  (use 'eps' for epsilon transitions)")
    for _ in range(num_trans):
        parts = input("    > ").split()
        from_id = int(parts[0])
        symbol = parts[1]
        to_id = int(parts[2])
        nfa.add_transition(from_id, symbol, to_id)

    return nfa


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    print("=" * 70)
    print("  NFA -> DFA (Subset Construction)  --  Exercise 3")
    print("=" * 70)

    print("\nChoose input mode:")
    print("  1) Use default NFA for (a|b)*abb")
    print("  2) Enter your own NFA")
    choice = input("\nEnter choice [1]: ").strip()

    if choice == "2":
        nfa = build_nfa_from_user()
    else:
        nfa = build_example_nfa()

    print("\n--- NFA Transition Table ---\n")
    print_nfa(nfa)

    # Convert
    dfa = subset_construction(nfa)

    print("\n--- DFA Transition Table (via Subset Construction) ---\n")
    print_dfa(dfa)

    # Summary
    print(f"\nSummary:")
    print(f"  NFA states: {len(nfa.states)}")
    print(f"  DFA states: {len(dfa.states)}")
    print(f"  Alphabet  : {sorted(dfa.alphabet)}")

    accept_dfa_ids = [sid for sid, ds in dfa.states.items() if ds.is_accept]
    print(f"  DFA start : D{dfa.start}")
    print(f"  DFA accept: {', '.join(f'D{a}' for a in sorted(accept_dfa_ids))}")


if __name__ == "__main__":
    main()
