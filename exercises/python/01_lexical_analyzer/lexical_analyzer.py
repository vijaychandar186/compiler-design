"""
================================================================================
Exercise 1: Lexical Analyzer (Scanner)
================================================================================

CONCEPT:
    A lexical analyzer (also called a scanner or tokenizer) is the first phase
    of a compiler. It reads the source program character by character and groups
    them into meaningful sequences called "lexemes." Each lexeme is mapped to a
    token, which is a <token-type, attribute-value> pair that is passed to the
    subsequent phase (the parser/syntax analyzer).

ALGORITHM:
    The scanner uses a character-by-character approach with a state-driven loop.
    At each step, the current character determines which kind of token is being
    formed. For example, if the character is a letter, the scanner continues
    reading letters and digits to form an identifier or keyword. If it is a
    digit, it reads a numeric constant (integer or float). Operators that can
    be one or two characters (like '=' vs '==') require a one-character lookahead
    to decide the correct token. Comments starting with '//' are detected and
    skipped. String literals are read from one quote to the matching quote.

    After a lexeme is fully read, it is classified:
      - If it matches a reserved keyword, the token type is KEYWORD.
      - Otherwise an alphabetic lexeme is an IDENTIFIER.
      - Numeric lexemes are INTEGER_CONST or FLOAT_CONST.
      - Operator and punctuation lexemes have their own categories.

COMPLEXITY:
    Time:  O(n) where n is the length of the source code -- each character is
           visited at most twice (once for reading, once for lookahead).
    Space: O(n) for storing the resulting list of tokens.

EXAMPLE INPUT:
    int main() {
        int x = 10;
        if (x >= 5) {
            x = x + 1;
        }
        return 0;
    }

EXAMPLE OUTPUT (partial):
    TOKEN TYPE        | LEXEME
    ------------------+---------
    KEYWORD           | int
    KEYWORD           | main
    PUNCTUATION       | (
    PUNCTUATION       | )
    PUNCTUATION       | {
    KEYWORD           | int
    IDENTIFIER        | x
    OPERATOR          | =
    INTEGER_CONST     | 10
    ...
================================================================================
"""

# ---------------------------------------------------------------------------
# Token categories
# ---------------------------------------------------------------------------
TOKEN_KEYWORD       = "KEYWORD"
TOKEN_IDENTIFIER    = "IDENTIFIER"
TOKEN_INTEGER       = "INTEGER_CONST"
TOKEN_FLOAT         = "FLOAT_CONST"
TOKEN_STRING        = "STRING_LITERAL"
TOKEN_OPERATOR      = "OPERATOR"
TOKEN_PUNCTUATION   = "PUNCTUATION"
TOKEN_UNKNOWN       = "UNKNOWN"

# ---------------------------------------------------------------------------
# Language tables
# ---------------------------------------------------------------------------
KEYWORDS = {
    "int", "float", "char", "double", "void",
    "if", "else", "while", "for", "do",
    "return", "break", "continue",
    "struct", "typedef", "enum", "switch", "case", "default",
    "main", "printf", "scanf", "include", "define",
}

# Two-character operators must be checked before single-character ones.
TWO_CHAR_OPERATORS = {
    "==", "!=", "<=", ">=", "++", "--",
    "+=", "-=", "*=", "/=", "%=",
    "&&", "||", "<<", ">>",
}

SINGLE_CHAR_OPERATORS = set("+-*/%=<>!&|^~")

PUNCTUATION = set(";,(){}[]:#.")


# ---------------------------------------------------------------------------
# Lexical Analyzer
# ---------------------------------------------------------------------------
class Token:
    """Represents a single token produced by the lexer."""
    def __init__(self, token_type: str, lexeme: str, line: int, col: int):
        self.token_type = token_type
        self.lexeme = lexeme
        self.line = line
        self.col = col

    def __repr__(self):
        return f"Token({self.token_type}, {self.lexeme!r}, line={self.line})"


def tokenize(source_code: str) -> list:
    """
    Scan *source_code* character by character and return a list of Token objects.
    """
    tokens = []
    pos = 0                # current index in source_code
    length = len(source_code)
    line = 1               # current source line  (for error messages)
    col = 1                # current column

    while pos < length:
        ch = source_code[pos]

        # ---- whitespace ------------------------------------------------
        if ch in " \t\r":
            pos += 1
            col += 1
            continue

        if ch == "\n":
            pos += 1
            line += 1
            col = 1
            continue

        # ---- single-line comment  //  ----------------------------------
        if ch == "/" and pos + 1 < length and source_code[pos + 1] == "/":
            # skip until end of line
            while pos < length and source_code[pos] != "\n":
                pos += 1
            continue  # the newline itself will be handled on next iteration

        # ---- multi-line comment  /* ... */  ----------------------------
        if ch == "/" and pos + 1 < length and source_code[pos + 1] == "*":
            pos += 2
            col += 2
            while pos + 1 < length:
                if source_code[pos] == "*" and source_code[pos + 1] == "/":
                    pos += 2
                    col += 2
                    break
                if source_code[pos] == "\n":
                    line += 1
                    col = 1
                else:
                    col += 1
                pos += 1
            continue

        # ---- string literal  "..."  ------------------------------------
        if ch == '"':
            start_col = col
            start_pos = pos
            pos += 1
            col += 1
            while pos < length and source_code[pos] != '"':
                if source_code[pos] == "\\":  # handle escape sequences
                    pos += 1
                    col += 1
                if source_code[pos] == "\n":
                    line += 1
                    col = 1
                else:
                    col += 1
                pos += 1
            if pos < length:  # skip closing quote
                pos += 1
                col += 1
            lexeme = source_code[start_pos:pos]
            tokens.append(Token(TOKEN_STRING, lexeme, line, start_col))
            continue

        # ---- character literal  '.'  -----------------------------------
        if ch == "'":
            start_col = col
            start_pos = pos
            pos += 1
            col += 1
            while pos < length and source_code[pos] != "'":
                if source_code[pos] == "\\":
                    pos += 1
                    col += 1
                col += 1
                pos += 1
            if pos < length:
                pos += 1
                col += 1
            lexeme = source_code[start_pos:pos]
            tokens.append(Token(TOKEN_STRING, lexeme, line, start_col))
            continue

        # ---- identifiers / keywords ------------------------------------
        if ch.isalpha() or ch == "_":
            start_col = col
            start_pos = pos
            while pos < length and (source_code[pos].isalnum() or source_code[pos] == "_"):
                pos += 1
                col += 1
            lexeme = source_code[start_pos:pos]
            tok_type = TOKEN_KEYWORD if lexeme in KEYWORDS else TOKEN_IDENTIFIER
            tokens.append(Token(tok_type, lexeme, line, start_col))
            continue

        # ---- numeric constants (integer and float) ---------------------
        if ch.isdigit():
            start_col = col
            start_pos = pos
            is_float = False
            while pos < length and source_code[pos].isdigit():
                pos += 1
                col += 1
            # check for decimal point
            if pos < length and source_code[pos] == ".":
                is_float = True
                pos += 1
                col += 1
                while pos < length and source_code[pos].isdigit():
                    pos += 1
                    col += 1
            lexeme = source_code[start_pos:pos]
            tok_type = TOKEN_FLOAT if is_float else TOKEN_INTEGER
            tokens.append(Token(tok_type, lexeme, line, start_col))
            continue

        # ---- preprocessor directive  # --------------------------------
        if ch == "#":
            start_col = col
            tokens.append(Token(TOKEN_PUNCTUATION, "#", line, start_col))
            pos += 1
            col += 1
            continue

        # ---- two-character operators -----------------------------------
        if pos + 1 < length:
            two = source_code[pos:pos + 2]
            if two in TWO_CHAR_OPERATORS:
                tokens.append(Token(TOKEN_OPERATOR, two, line, col))
                pos += 2
                col += 2
                continue

        # ---- single-character operators --------------------------------
        if ch in SINGLE_CHAR_OPERATORS:
            tokens.append(Token(TOKEN_OPERATOR, ch, line, col))
            pos += 1
            col += 1
            continue

        # ---- punctuation / delimiters ----------------------------------
        if ch in PUNCTUATION:
            tokens.append(Token(TOKEN_PUNCTUATION, ch, line, col))
            pos += 1
            col += 1
            continue

        # ---- unknown character -----------------------------------------
        tokens.append(Token(TOKEN_UNKNOWN, ch, line, col))
        pos += 1
        col += 1

    return tokens


# ---------------------------------------------------------------------------
# Pretty-printing
# ---------------------------------------------------------------------------
def print_tokens(tokens: list) -> None:
    """Print tokens in a formatted table."""
    header_type = "TOKEN TYPE"
    header_lex  = "LEXEME"
    header_loc  = "LOCATION"

    # determine column widths
    type_width = max(len(header_type), max((len(t.token_type) for t in tokens), default=0))
    lex_width  = max(len(header_lex),  max((len(t.lexeme) for t in tokens), default=0))
    loc_width  = max(len(header_loc), 12)

    sep = "+" + "-" * (type_width + 2) + "+" + "-" * (lex_width + 2) + "+" + "-" * (loc_width + 2) + "+"

    print(sep)
    print(f"| {header_type:<{type_width}} | {header_lex:<{lex_width}} | {header_loc:<{loc_width}} |")
    print(sep)
    for tok in tokens:
        loc = f"L{tok.line}:C{tok.col}"
        print(f"| {tok.token_type:<{type_width}} | {tok.lexeme:<{lex_width}} | {loc:<{loc_width}} |")
    print(sep)


def print_summary(tokens: list) -> None:
    """Print a summary count of each token category."""
    counts: dict[str, int] = {}
    for tok in tokens:
        counts[tok.token_type] = counts.get(tok.token_type, 0) + 1

    print("\n+---------------------+-------+")
    print(f"| {'TOKEN TYPE':<19} | {'COUNT':>5} |")
    print("+---------------------+-------+")
    for cat in sorted(counts):
        print(f"| {cat:<19} | {counts[cat]:>5} |")
    print("+---------------------+-------+")
    print(f"| {'TOTAL':<19} | {len(tokens):>5} |")
    print("+---------------------+-------+")


# ---------------------------------------------------------------------------
# Default sample input
# ---------------------------------------------------------------------------
SAMPLE_CODE = r"""
// A small C program for lexical analysis demonstration
#include <stdio.h>

int main() {
    int count = 0;
    float ratio = 3.14;
    char name[] = "hello";

    // Loop with multiple operators
    for (int i = 0; i <= 10; i++) {
        if (i != 5 && i >= 2) {
            count += i;
        } else {
            count--;
        }
    }

    while (count > 0) {
        ratio *= 0.5;
        count = count - 1;
    }

    printf("Result: %d\n", count);
    return 0;
}
"""


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    print("=" * 70)
    print("         LEXICAL ANALYZER  --  Compiler Design Exercise 1")
    print("=" * 70)

    print("\nChoose input mode:")
    print("  1) Use default sample C code")
    print("  2) Enter your own code (end with a blank line)")

    choice = input("\nEnter choice [1]: ").strip()

    if choice == "2":
        print("\nEnter source code (press Enter twice to finish):\n")
        lines = []
        while True:
            line = input()
            if line == "":
                break
            lines.append(line)
        source = "\n".join(lines)
    else:
        source = SAMPLE_CODE

    print("\n--- Source Code ---")
    for num, line in enumerate(source.split("\n"), start=1):
        print(f"  {num:3d} | {line}")
    print("--- End of Source ---\n")

    tokens = tokenize(source)

    print("--- Token Table ---")
    print_tokens(tokens)
    print_summary(tokens)


if __name__ == "__main__":
    main()
