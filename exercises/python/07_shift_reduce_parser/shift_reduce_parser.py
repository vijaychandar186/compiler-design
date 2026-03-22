"""
================================================================================
Exercise 7: Shift-Reduce Parser
================================================================================

CONCEPT:
A shift-reduce parser is a bottom-up parser that builds a parse tree from the
leaves up to the root. It uses a stack and processes the input left to right.
At each step, it either "shifts" the next input symbol onto the stack, or
"reduces" a sequence of symbols on top of the stack that matches the right-hand
side of a production, replacing them with the left-hand side non-terminal.

The parser performs four types of actions:
  - Shift: Push the next input symbol onto the stack.
  - Reduce: Replace a handle (matching RHS) on top of the stack with LHS.
  - Accept: Input is fully consumed and stack contains only the start symbol.
  - Error: No valid action is possible.

ALGORITHM:
This implementation uses an operator-precedence approach to handle the ambiguous
grammar E -> E+E | E*E | (E) | id. Operator precedence (* binds tighter than +)
and left-associativity are used to decide between shift and reduce when the
stack top contains a reducible expression and the next input is an operator:
  - If the operator on the stack has higher or equal precedence to the input
    operator, reduce. Otherwise, shift.

COMPLEXITY:
  Time:  O(n^2) worst case where n = input length (each shift/reduce is O(n)
         for handle finding, and there are O(n) steps)
  Space: O(n) for the stack

EXAMPLE:
  Grammar: E -> E+E | E*E | (E) | id
  Input: id+id*id
  Expected: id is shifted and reduced to E, then + and id are shifted,
  * binds tighter so id*id reduces first, then E+E reduces.
================================================================================
"""

# Operator precedence: higher number = higher precedence
PRECEDENCE = {
    "+": 1,
    "*": 2,
}


def tokenize(input_string):
    """Tokenize input string into a list of tokens."""
    tokens = []
    i = 0
    while i < len(input_string):
        ch = input_string[i]
        if ch.isspace():
            i += 1
            continue
        if input_string[i:i+2] == "id":
            tokens.append("id")
            i += 2
        else:
            tokens.append(ch)
            i += 1
    tokens.append("$")
    return tokens


def find_top_operator(stack):
    """Find the topmost operator (+, *) in the stack for precedence comparison."""
    for i in range(len(stack) - 1, -1, -1):
        if stack[i] in PRECEDENCE:
            return stack[i], i
    return None, -1


def should_reduce_over_shift(stack, next_token):
    """
    Decide whether to reduce (True) or shift (False) based on operator
    precedence when both actions are possible.

    We reduce when:
      - The stack's top operator has higher precedence than the input operator.
      - The stack's top operator has equal precedence (left-associativity).
    We shift when:
      - The input operator has higher precedence.
      - The next token is not an operator (like '(').
    """
    stack_op, _ = find_top_operator(stack)
    if stack_op is None:
        return False  # No operator on stack, must shift

    if next_token not in PRECEDENCE:
        # Next token is ) or $ -> reduce
        if next_token in (")", "$"):
            return True
        return False

    # Compare precedence
    if PRECEDENCE[stack_op] >= PRECEDENCE[next_token]:
        return True  # Reduce (higher or equal precedence, left-assoc)
    return False  # Shift (input has higher precedence)


def try_reduce(stack):
    """
    Try to find a handle on top of the stack and reduce it.
    Returns (new_stack, production_used) or (None, None) if no reduction possible.

    Reductions (checked top-down on the stack):
      E -> id        (top is "id")
      E -> E + E     (top 3 are E + E)
      E -> E * E     (top 3 are E * E)
      E -> ( E )     (top 3 are ( E ))
    """
    n = len(stack)

    # E -> id
    if n >= 1 and stack[-1] == "id":
        return stack[:-1] + ["E"], "E -> id"

    # E -> ( E )
    if n >= 3 and stack[-3] == "(" and stack[-2] == "E" and stack[-1] == ")":
        return stack[:-3] + ["E"], "E -> ( E )"

    # E -> E + E
    if n >= 3 and stack[-3] == "E" and stack[-2] == "+" and stack[-1] == "E":
        return stack[:-3] + ["E"], "E -> E + E"

    # E -> E * E
    if n >= 3 and stack[-3] == "E" and stack[-2] == "*" and stack[-1] == "E":
        return stack[:-3] + ["E"], "E -> E * E"

    return None, None


def parse(input_string):
    """
    Perform shift-reduce parsing on the input string.
    Prints a step-by-step trace.
    """
    tokens = tokenize(input_string)
    stack = []
    pos = 0
    step = 0

    print(f"\n  {'Step':<6} {'Stack':<30} {'Input':<25} {'Action':<30}")
    print(f"  {'-'*6} {'-'*30} {'-'*25} {'-'*30}")

    while True:
        current = tokens[pos] if pos < len(tokens) else "$"
        remaining = " ".join(tokens[pos:])
        stack_str = " ".join(stack) if stack else "(empty)"
        step += 1

        # Check for accept: stack is ["E"] and input is exhausted
        if stack == ["E"] and current == "$":
            print(f"  {step:<6} {stack_str:<30} {remaining:<25} {'ACCEPT':<30}")
            print(f"\n  ** Input \"{input_string}\" successfully parsed! **")
            return True

        # Try to reduce "id" immediately (always reduce id to E)
        if stack and stack[-1] == "id":
            new_stack, production = try_reduce(stack)
            if new_stack is not None:
                print(f"  {step:<6} {stack_str:<30} {remaining:<25} "
                      f"{'Reduce: ' + production:<30}")
                stack = new_stack
                continue

        # After closing paren, always try to reduce ( E )
        if len(stack) >= 3 and stack[-1] == ")" and stack[-2] == "E" and stack[-3] == "(":
            new_stack, production = try_reduce(stack)
            if new_stack is not None:
                print(f"  {step:<6} {stack_str:<30} {remaining:<25} "
                      f"{'Reduce: ' + production:<30}")
                stack = new_stack
                continue

        # If the stack has E op E on top, decide shift vs reduce
        if (len(stack) >= 3 and stack[-3] == "E" and
                stack[-2] in PRECEDENCE and stack[-1] == "E"):
            if should_reduce_over_shift(stack, current):
                new_stack, production = try_reduce(stack)
                if new_stack is not None:
                    print(f"  {step:<6} {stack_str:<30} {remaining:<25} "
                          f"{'Reduce: ' + production:<30}")
                    stack = new_stack
                    continue

        # Shift
        if current != "$":
            action = f"Shift '{current}'"
            print(f"  {step:<6} {stack_str:<30} {remaining:<25} {action:<30}")
            stack.append(current)
            pos += 1
        else:
            # No shift possible (input exhausted), try to reduce
            new_stack, production = try_reduce(stack)
            if new_stack is not None:
                print(f"  {step:<6} {stack_str:<30} {remaining:<25} "
                      f"{'Reduce: ' + production:<30}")
                stack = new_stack
            else:
                print(f"  {step:<6} {stack_str:<30} {remaining:<25} "
                      f"{'ERROR: cannot parse':<30}")
                print(f"\n  ** Parse error on \"{input_string}\" **")
                return False


def main():
    print("=" * 95)
    print("  Exercise 7: Shift-Reduce Parser")
    print("=" * 95)

    print("\n  Grammar:")
    print("    E -> E + E")
    print("    E -> E * E")
    print("    E -> ( E )")
    print("    E -> id")
    print("\n  Operator Precedence: * > +  (left-associative)")

    test_inputs = [
        "id+id*id",
        "id*id+id",
        "id+id+id",
        "(id+id)*id",
        "id",
    ]

    for test_input in test_inputs:
        print(f"\n{'='*95}")
        print(f"\n  Parsing: \"{test_input}\"")
        parse(test_input)

    print(f"\n{'='*95}")


if __name__ == "__main__":
    main()
