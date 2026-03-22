"""
==============================================================================
Exercise 10: Infix to Prefix and Postfix Conversion
==============================================================================

CONCEPT:
Infix notation places operators between operands (e.g., A + B). This is
natural for humans but ambiguous without precedence rules and parentheses.
Prefix (Polish) notation places the operator before its operands (+ A B),
and postfix (Reverse Polish) places it after (A B +). Both prefix and
postfix notations are unambiguous and do not require parentheses.

ALGORITHM FOR INFIX TO POSTFIX (Shunting-Yard):
  1. Scan the infix expression left to right.
  2. Operands go directly to the output.
  3. '(' is pushed onto the operator stack.
  4. ')' pops operators to output until '(' is found.
  5. An operator pops higher-or-equal precedence operators (respecting
     associativity) from the stack to output, then is pushed.
  6. After scanning, pop remaining operators to output.

ALGORITHM FOR INFIX TO PREFIX (Reverse-Scan Method):
  1. Reverse the infix expression, swapping '(' and ')'.
  2. Apply a modified shunting-yard (for left-associative operators, pop
     only strictly higher precedence, not equal).
  3. Reverse the resulting postfix to get prefix.

TIME COMPLEXITY:  O(n) where n is the length of the expression.
SPACE COMPLEXITY: O(n) for the operator stack and output queue.

EXAMPLE INPUT:  (A+B)*C-D/E
EXAMPLE OUTPUT:
  Postfix: A B + C * D E / -
  Prefix:  - * + A B C / D E
==============================================================================
"""


def precedence(operator):
    """Return the precedence level of an operator."""
    precedence_map = {
        '+': 1, '-': 1,
        '*': 2, '/': 2,
        '^': 3
    }
    return precedence_map.get(operator, 0)


def is_right_associative(operator):
    """Check if an operator is right-associative."""
    return operator == '^'


def is_operator(char):
    """Check if a character is a recognized operator."""
    return char in {'+', '-', '*', '/', '^'}


def is_operand(char):
    """Check if a character is an operand (letter or digit)."""
    return char.isalnum()


def tokenize(expression):
    """
    Tokenize an infix expression into a list of tokens.
    Handles multi-character operands like 'id' or numbers.
    """
    tokens = []
    i = 0
    while i < len(expression):
        char = expression[i]
        if char.isspace():
            i += 1
            continue
        if char in '()' or is_operator(char):
            tokens.append(char)
            i += 1
        elif char.isalnum():
            # Collect multi-character operand
            start = i
            while i < len(expression) and expression[i].isalnum():
                i += 1
            tokens.append(expression[start:i])
        else:
            i += 1  # Skip unknown characters
    return tokens


def infix_to_postfix(tokens):
    """
    Convert a tokenized infix expression to postfix using the
    Shunting-Yard algorithm.
    """
    output = []
    operator_stack = []

    for token in tokens:
        if is_operator(token):
            # Pop operators with higher or equal precedence (left-assoc)
            # or strictly higher precedence (right-assoc)
            while (operator_stack and
                   operator_stack[-1] != '(' and
                   is_operator(operator_stack[-1]) and
                   (precedence(operator_stack[-1]) > precedence(token) or
                    (precedence(operator_stack[-1]) == precedence(token) and
                     not is_right_associative(token)))):
                output.append(operator_stack.pop())
            operator_stack.append(token)

        elif token == '(':
            operator_stack.append(token)

        elif token == ')':
            while operator_stack and operator_stack[-1] != '(':
                output.append(operator_stack.pop())
            if operator_stack and operator_stack[-1] == '(':
                operator_stack.pop()  # Discard the '('

        else:
            # Operand
            output.append(token)

    # Pop all remaining operators
    while operator_stack:
        top = operator_stack.pop()
        if top != '(':
            output.append(top)

    return output


def infix_to_prefix(tokens):
    """
    Convert a tokenized infix expression to prefix using the
    reverse-scan method:
      1. Reverse tokens, swapping ( and ).
      2. Apply modified shunting-yard (invert associativity behavior).
      3. Reverse the result.
    """
    # Step 1: Reverse and swap parentheses
    reversed_tokens = []
    for token in reversed(tokens):
        if token == '(':
            reversed_tokens.append(')')
        elif token == ')':
            reversed_tokens.append('(')
        else:
            reversed_tokens.append(token)

    # Step 2: Modified shunting-yard
    # For the reversed scan, left-associative operators should only pop
    # strictly higher precedence (not equal), because the reversal
    # inverts the associativity behavior.
    output = []
    operator_stack = []

    for token in reversed_tokens:
        if is_operator(token):
            while (operator_stack and
                   operator_stack[-1] != '(' and
                   is_operator(operator_stack[-1]) and
                   (precedence(operator_stack[-1]) > precedence(token) or
                    (precedence(operator_stack[-1]) == precedence(token) and
                     is_right_associative(token)))):
                output.append(operator_stack.pop())
            operator_stack.append(token)

        elif token == '(':
            operator_stack.append(token)

        elif token == ')':
            while operator_stack and operator_stack[-1] != '(':
                output.append(operator_stack.pop())
            if operator_stack and operator_stack[-1] == '(':
                operator_stack.pop()

        else:
            output.append(token)

    while operator_stack:
        top = operator_stack.pop()
        if top != '(':
            output.append(top)

    # Step 3: Reverse to get prefix
    output.reverse()
    return output


def convert_expression(expression):
    """Convert an infix expression and print both prefix and postfix."""
    tokens = tokenize(expression)
    postfix = infix_to_postfix(tokens)
    prefix = infix_to_prefix(tokens)

    print(f"  Infix:   {expression}")
    print(f"  Tokens:  {' '.join(tokens)}")
    print(f"  Postfix: {' '.join(postfix)}")
    print(f"  Prefix:  {' '.join(prefix)}")


def main():
    print("=" * 55)
    print("  Infix to Prefix and Postfix Converter")
    print("=" * 55)

    test_expressions = [
        "(A+B)*C-D/E",
        "A+B*C",
        "A*(B+C)/D",
        "A+B*C-D",
        "A^B^C",
        "((A+B)*C-(D-E))/(F+G)",
        "A*B+C*D",
        "A+B+C+D",
    ]

    for i, expr in enumerate(test_expressions):
        print(f"\n  {'─' * 50}")
        print(f"  Example {i + 1}:")
        print(f"  {'─' * 50}")
        convert_expression(expr)

    print(f"\n{'=' * 55}")

    # Demonstrate right-associativity of ^
    print("\n  Note on right-associativity of ^ (exponentiation):")
    print("  A^B^C is parsed as A^(B^C), NOT (A^B)^C")
    print("  Postfix: A B C ^ ^  (B^C is computed first)")
    print("  Prefix:  ^ A ^ B C")
    print(f"\n{'=' * 55}")


if __name__ == "__main__":
    main()
