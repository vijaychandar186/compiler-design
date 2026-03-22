"""
================================================================================
Exercise 2: Regular Expression to NFA (Thompson's Construction)
================================================================================

CONCEPT:
    Thompson's construction is a systematic method for converting any regular
    expression into an equivalent Non-deterministic Finite Automaton (NFA).
    The resulting NFA has exactly one start state and one accept state, uses
    epsilon-transitions liberally, and each state has at most two outgoing
    transitions.  These properties make the NFA easy to compose recursively.

ALGORITHM:
    1. First, insert explicit concatenation operators ('.') between adjacent
       symbols where concatenation is implied (e.g., "ab" becomes "a.b").
    2. Convert the infix expression to postfix using the shunting-yard
       algorithm, respecting operator precedence: '*' > '.' > '|'.
    3. Walk the postfix expression left to right.  For each symbol:
       - Literal character 'c': push a two-state NFA  start --c--> accept.
       - Concatenation '.':  pop two NFAs, connect first's accept to
         second's start via epsilon.
       - Union '|':  pop two NFAs, create new start/accept with epsilon
         branches to both.
       - Kleene star '*':  pop one NFA, create new start/accept with
         epsilon loops.
    4. The single NFA remaining on the stack is the result.

COMPLEXITY:
    Time:  O(n) where n is the length of the regular expression -- each
           character is processed a constant number of times.
    Space: O(n) states are created (at most 2 per character in the regex).

EXAMPLE:
    Input regex:  (a|b)*abb
    Output: An NFA with epsilon transitions printed as a transition table.
================================================================================
"""

# ---------------------------------------------------------------------------
# NFA building blocks
# ---------------------------------------------------------------------------

EPSILON = "eps"  # label for epsilon transitions

class State:
    """A single NFA state."""
    _counter = 0

    def __init__(self):
        State._counter += 1
        self.id = State._counter
        self.transitions = {}   # char -> list of State

    def add_transition(self, symbol, target):
        self.transitions.setdefault(symbol, []).append(target)

    def __repr__(self):
        return f"q{self.id}"


class NFA:
    """An NFA fragment with a single start state and a single accept state."""
    def __init__(self, start, accept):
        self.start = start
        self.accept = accept


def reset_state_counter():
    State._counter = 0


# ---------------------------------------------------------------------------
# Thompson's construction primitives
# ---------------------------------------------------------------------------
def nfa_for_char(ch):
    """Create a basic NFA for a single character: start --ch--> accept."""
    start = State()
    accept = State()
    start.add_transition(ch, accept)
    return NFA(start, accept)


def nfa_concat(first, second):
    """Concatenation: connect first.accept to second.start via epsilon."""
    first.accept.add_transition(EPSILON, second.start)
    return NFA(first.start, second.accept)


def nfa_union(upper, lower):
    """Union (|): new start branches to both; both accepts merge to new accept."""
    start = State()
    accept = State()
    start.add_transition(EPSILON, upper.start)
    start.add_transition(EPSILON, lower.start)
    upper.accept.add_transition(EPSILON, accept)
    lower.accept.add_transition(EPSILON, accept)
    return NFA(start, accept)


def nfa_star(inner):
    """Kleene star (*): zero or more repetitions."""
    start = State()
    accept = State()
    start.add_transition(EPSILON, inner.start)
    start.add_transition(EPSILON, accept)         # zero occurrences
    inner.accept.add_transition(EPSILON, inner.start)  # repeat
    inner.accept.add_transition(EPSILON, accept)       # exit loop
    return NFA(start, accept)


# ---------------------------------------------------------------------------
# Regex preprocessing: insert explicit concatenation operator '.'
# ---------------------------------------------------------------------------
def insert_concat_operator(regex):
    """
    Insert '.' where concatenation is implied.
    Concatenation is implied between:
      - two literals:          a b   -> a . b
      - literal and '(':      a (   -> a . (
      - ')' and literal:      ) a   -> ) . a
      - ')' and '(':          ) (   -> ) . (
      - '*' and literal:      * a   -> * . a
      - '*' and '(':          * (   -> * . (
    """
    result = []
    for i, ch in enumerate(regex):
        result.append(ch)
        if i + 1 < len(regex):
            next_ch = regex[i + 1]
            if (ch not in ('(', '|', '.') and
                next_ch not in (')', '|', '*', '.')):
                result.append('.')
    return ''.join(result)


# ---------------------------------------------------------------------------
# Shunting-yard: infix to postfix
# ---------------------------------------------------------------------------
PRECEDENCE = {'|': 1, '.': 2, '*': 3}

def infix_to_postfix(infix):
    """Convert infix regex (with explicit '.') to postfix."""
    output = []
    stack = []
    for ch in infix:
        if ch == '(':
            stack.append(ch)
        elif ch == ')':
            while stack and stack[-1] != '(':
                output.append(stack.pop())
            if stack:
                stack.pop()  # remove '('
        elif ch in PRECEDENCE:
            while (stack and stack[-1] != '(' and
                   stack[-1] in PRECEDENCE and
                   PRECEDENCE[stack[-1]] >= PRECEDENCE[ch]):
                output.append(stack.pop())
            stack.append(ch)
        else:
            output.append(ch)  # literal
    while stack:
        output.append(stack.pop())
    return ''.join(output)


# ---------------------------------------------------------------------------
# Build NFA from postfix regex
# ---------------------------------------------------------------------------
def build_nfa(postfix):
    """Evaluate postfix regex using a stack of NFA fragments."""
    nfa_stack = []
    for ch in postfix:
        if ch == '.':
            second = nfa_stack.pop()
            first = nfa_stack.pop()
            nfa_stack.append(nfa_concat(first, second))
        elif ch == '|':
            lower = nfa_stack.pop()
            upper = nfa_stack.pop()
            nfa_stack.append(nfa_union(upper, lower))
        elif ch == '*':
            inner = nfa_stack.pop()
            nfa_stack.append(nfa_star(inner))
        else:
            nfa_stack.append(nfa_for_char(ch))
    return nfa_stack.pop()


# ---------------------------------------------------------------------------
# Collect all states and print the NFA
# ---------------------------------------------------------------------------
def collect_states(nfa):
    """BFS/DFS to collect all reachable states from nfa.start."""
    visited = set()
    queue = [nfa.start]
    states = []
    while queue:
        state = queue.pop(0)
        if state.id in visited:
            continue
        visited.add(state.id)
        states.append(state)
        for targets in state.transitions.values():
            for target in targets:
                if target.id not in visited:
                    queue.append(target)
    states.sort(key=lambda s: s.id)
    return states


def get_alphabet(states):
    """Return the set of input symbols (excluding epsilon)."""
    alphabet = set()
    for state in states:
        for sym in state.transitions:
            if sym != EPSILON:
                alphabet.add(sym)
    return sorted(alphabet)


def print_nfa(nfa):
    """Print the NFA transition table."""
    states = collect_states(nfa)
    alphabet = get_alphabet(states)
    all_symbols = [EPSILON] + alphabet

    # column widths
    state_width = max(6, max(len(str(s)) for s in states) + 4)
    col_width = 14

    # header
    header = f"{'STATE':<{state_width}}"
    for sym in all_symbols:
        header += f" | {sym:^{col_width}}"
    sep = "-" * len(header)

    print(sep)
    print(header)
    print(sep)

    for state in states:
        marker = ""
        if state.id == nfa.start.id:
            marker = "->"
        if state.id == nfa.accept.id:
            marker += "*"
        label = f"{marker}{state}"

        row = f"{label:<{state_width}}"
        for sym in all_symbols:
            targets = state.transitions.get(sym, [])
            cell = ", ".join(str(t) for t in sorted(targets, key=lambda s: s.id))
            if not cell:
                cell = "-"
            row += f" | {cell:^{col_width}}"
        print(row)

    print(sep)
    print(f"\nStart state : {nfa.start}")
    print(f"Accept state: {nfa.accept}")
    print(f"Total states: {len(states)}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    print("=" * 70)
    print("  RE -> NFA (Thompson's Construction)  --  Exercise 2")
    print("=" * 70)

    default_regex = "(a|b)*abb"

    print(f"\nDefault regex: {default_regex}")
    user = input("Enter a regex (or press Enter for default): ").strip()
    regex = user if user else default_regex

    reset_state_counter()

    print(f"\n1. Original regex      : {regex}")

    with_concat = insert_concat_operator(regex)
    print(f"2. With explicit concat: {with_concat}")

    postfix = infix_to_postfix(with_concat)
    print(f"3. Postfix form        : {postfix}")

    nfa = build_nfa(postfix)

    print(f"\n--- NFA Transition Table ---\n")
    print_nfa(nfa)


if __name__ == "__main__":
    main()
