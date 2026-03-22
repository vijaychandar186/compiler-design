"""
================================================================================
Exercise 11: Three-Address Code (TAC) Generation
================================================================================

CONCEPT:
    Three-Address Code is an intermediate representation used in compilers where
    each instruction has at most three operands (two sources and one destination).
    Complex expressions are broken down into a sequence of simple instructions,
    each involving at most one operator. Temporary variables (t1, t2, ...) hold
    intermediate results.

ALGORITHM:
    This implementation uses a recursive descent parser that respects operator
    precedence. The grammar is:
        assignment  -> IDENTIFIER '=' expression
        expression  -> term (('+' | '-') term)*
        term        -> factor (('*' | '/') factor)*
        factor      -> IDENTIFIER | NUMBER | '(' expression ')'

    As the parser recognizes each binary operation, it emits a three-address
    code instruction using a fresh temporary variable. The parser returns the
    name of the temporary (or the leaf operand) so that higher-level rules can
    reference the result. This naturally produces code in correct evaluation
    order with proper precedence.

TIME COMPLEXITY:  O(n) where n is the length of the expression (single pass).
SPACE COMPLEXITY: O(n) for the generated TAC instructions and temporaries.

EXAMPLE:
    Input:  x = a + b * c - d / e
    Output:
        t1 = b * c
        t2 = a + t1
        t3 = d / e
        t4 = t2 - t3
        x = t4
================================================================================
"""


class Lexer:
    """Tokenizes an expression string into a stream of tokens."""

    def __init__(self, text):
        self.text = text
        self.pos = 0
        self.tokens = []
        self._tokenize()
        self.current = 0

    def _tokenize(self):
        """Scan the input and produce a list of (type, value) tokens."""
        i = 0
        while i < len(self.text):
            ch = self.text[i]
            if ch.isspace():
                i += 1
            elif ch in "+-*/=()":
                self.tokens.append(("OP", ch))
                i += 1
            elif ch.isalpha() or ch == "_":
                start = i
                while i < len(self.text) and (self.text[i].isalnum() or self.text[i] == "_"):
                    i += 1
                self.tokens.append(("ID", self.text[start:i]))
            elif ch.isdigit():
                start = i
                while i < len(self.text) and self.text[i].isdigit():
                    i += 1
                self.tokens.append(("NUM", self.text[start:i]))
            else:
                raise SyntaxError(f"Unexpected character: '{ch}' at position {i}")
        self.tokens.append(("EOF", None))

    def peek(self):
        """Return the current token without consuming it."""
        return self.tokens[self.current]

    def advance(self):
        """Consume and return the current token."""
        token = self.tokens[self.current]
        self.current += 1
        return token

    def expect(self, token_type, value=None):
        """Consume the current token, raising an error if it does not match."""
        token = self.advance()
        if token[0] != token_type or (value is not None and token[1] != value):
            raise SyntaxError(
                f"Expected ({token_type}, {value}), got ({token[0]}, {token[1]})"
            )
        return token


class TACGenerator:
    """
    Recursive descent parser that emits three-address code while parsing.
    Operator precedence is encoded in the grammar:
        expression handles + and -
        term       handles * and /
        factor     handles identifiers, numbers, and parenthesised sub-expressions
    """

    def __init__(self):
        self.temp_counter = 0
        self.instructions = []

    def new_temp(self):
        """Allocate a fresh temporary variable name."""
        self.temp_counter += 1
        return f"t{self.temp_counter}"

    def emit(self, result, left, operator, right):
        """Append a three-address code instruction."""
        self.instructions.append((result, left, operator, right))

    # -------------------------------------------------------------------------
    # Parsing rules
    # -------------------------------------------------------------------------

    def parse_assignment(self, lexer):
        """assignment -> ID '=' expression"""
        name_token = lexer.expect("ID")
        target = name_token[1]
        lexer.expect("OP", "=")
        expr_result = self.parse_expression(lexer)

        # Emit the final assignment
        self.instructions.append((target, expr_result, None, None))
        return target

    def parse_expression(self, lexer):
        """expression -> term (('+' | '-') term)*"""
        left = self.parse_term(lexer)

        while lexer.peek()[0] == "OP" and lexer.peek()[1] in ("+", "-"):
            operator = lexer.advance()[1]
            right = self.parse_term(lexer)
            temp = self.new_temp()
            self.emit(temp, left, operator, right)
            left = temp

        return left

    def parse_term(self, lexer):
        """term -> factor (('*' | '/') factor)*"""
        left = self.parse_factor(lexer)

        while lexer.peek()[0] == "OP" and lexer.peek()[1] in ("*", "/"):
            operator = lexer.advance()[1]
            right = self.parse_factor(lexer)
            temp = self.new_temp()
            self.emit(temp, left, operator, right)
            left = temp

        return left

    def parse_factor(self, lexer):
        """factor -> ID | NUM | '(' expression ')'"""
        token = lexer.peek()

        if token[0] == "ID":
            lexer.advance()
            return token[1]
        elif token[0] == "NUM":
            lexer.advance()
            return token[1]
        elif token[0] == "OP" and token[1] == "(":
            lexer.advance()  # consume '('
            result = self.parse_expression(lexer)
            lexer.expect("OP", ")")
            return result
        else:
            raise SyntaxError(f"Unexpected token: {token}")

    # -------------------------------------------------------------------------
    # Public interface
    # -------------------------------------------------------------------------

    def generate(self, expression):
        """Parse an expression string and return three-address code."""
        self.instructions = []
        self.temp_counter = 0
        lexer = Lexer(expression)
        self.parse_assignment(lexer)
        return self.instructions

    def format_instructions(self):
        """Return a human-readable string of all TAC instructions."""
        lines = []
        for instr in self.instructions:
            result, left, operator, right = instr
            if operator is None:
                # Simple copy / assignment
                lines.append(f"    {result} = {left}")
            else:
                lines.append(f"    {result} = {left} {operator} {right}")
        return "\n".join(lines)


def process_expression(generator, expression):
    """Generate and display TAC for a single expression."""
    print(f"  Expression : {expression}")
    print(f"  Three-Address Code:")
    generator.generate(expression)
    print(generator.format_instructions())
    print()


def main():
    print("=" * 65)
    print("   THREE-ADDRESS CODE GENERATION")
    print("=" * 65)
    print()

    generator = TACGenerator()

    test_expressions = [
        "x = a + b * c - d / e",
        "y = (a + b) * (c - d)",
        "z = a * b + c * d + e * f",
        "w = a + b",
        "r = (a + b) * c - d / (e + f)",
    ]

    for i, expr in enumerate(test_expressions, start=1):
        print(f"  --- Test Case {i} ---")
        process_expression(generator, expr)

    print("=" * 65)
    print("  All expressions processed successfully.")
    print("=" * 65)


if __name__ == "__main__":
    main()
