/*
================================================================================
Exercise 1: Lexical Analyzer (Scanner)  --  C Implementation
================================================================================

CONCEPT:
    A lexical analyzer (also called a scanner or tokenizer) is the first phase
    of a compiler. It reads the source program character by character and groups
    them into meaningful sequences called "lexemes." Each lexeme is mapped to a
    token -- a (token-type, attribute-value) pair that is passed to the parser.

ALGORITHM:
    The scanner iterates through the source string one character at a time.
    Based on the current character it decides which class of token to build:
      - Letters/underscore  -> identifier or keyword
      - Digits              -> integer or float constant
      - Quote (")           -> string literal
      - Operator characters -> single or double operator
      - Punctuation         -> delimiter
      - '/' followed by '/' -> comment (skipped)
    A lookahead of one character is used for two-character operators (==, !=,
    etc.) and for detecting comments.  After each lexeme is extracted it is
    classified: if it matches a reserved word it becomes KEYWORD, otherwise
    IDENTIFIER, and so on.

COMPLEXITY:
    Time:  O(n) -- each character visited at most twice (read + lookahead).
    Space: O(n) -- token list storage.

EXAMPLE:
    Input:  "int x = 10;"
    Output: KEYWORD(int)  IDENTIFIER(x)  OPERATOR(=)  INTEGER(10)  PUNCT(;)
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- constants ---------------------------------------------------------- */
#define MAX_TOKENS     1024
#define MAX_LEXEME_LEN 256
#define MAX_SOURCE_LEN 4096

/* ---- token types -------------------------------------------------------- */
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATION,
    TOKEN_UNKNOWN
} TokenType;

static const char *token_type_names[] = {
    "KEYWORD",
    "IDENTIFIER",
    "INTEGER_CONST",
    "FLOAT_CONST",
    "STRING_LITERAL",
    "OPERATOR",
    "PUNCTUATION",
    "UNKNOWN"
};

/* ---- token structure ---------------------------------------------------- */
typedef struct {
    TokenType type;
    char      lexeme[MAX_LEXEME_LEN];
    int       line;
    int       col;
} Token;

/* ---- keyword table ------------------------------------------------------ */
static const char *keywords[] = {
    "int", "float", "char", "double", "void",
    "if", "else", "while", "for", "do",
    "return", "break", "continue",
    "struct", "typedef", "enum", "switch", "case", "default",
    "main", "printf", "scanf", "include", "define",
    NULL  /* sentinel */
};

/* ---- helper: is this string a keyword? ---------------------------------- */
static int is_keyword(const char *word)
{
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0)
            return 1;
    }
    return 0;
}

/* ---- helper: is character a single-char operator? ----------------------- */
static int is_operator_char(char ch)
{
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
           ch == '%' || ch == '=' || ch == '<' || ch == '>' ||
           ch == '!' || ch == '&' || ch == '|' || ch == '^' || ch == '~';
}

/* ---- helper: is character punctuation? ---------------------------------- */
static int is_punctuation(char ch)
{
    return ch == ';' || ch == ',' || ch == '(' || ch == ')' ||
           ch == '{' || ch == '}' || ch == '[' || ch == ']' ||
           ch == '#' || ch == ':' || ch == '.';
}

/* ---- add a token to the list -------------------------------------------- */
static int add_token(Token tokens[], int *count, TokenType type,
                     const char *lexeme, int line, int col)
{
    if (*count >= MAX_TOKENS) return -1;
    tokens[*count].type = type;
    strncpy(tokens[*count].lexeme, lexeme, MAX_LEXEME_LEN - 1);
    tokens[*count].lexeme[MAX_LEXEME_LEN - 1] = '\0';
    tokens[*count].line = line;
    tokens[*count].col  = col;
    (*count)++;
    return 0;
}

/* ======================================================================== */
/*  tokenize -- the main scanning function                                  */
/* ======================================================================== */
static int tokenize(const char *src, Token tokens[])
{
    int count  = 0;
    int pos    = 0;
    int length = (int)strlen(src);
    int line   = 1;
    int col    = 1;

    while (pos < length) {
        char ch = src[pos];

        /* ---- whitespace ------------------------------------------------ */
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            pos++; col++;
            continue;
        }
        if (ch == '\n') {
            pos++; line++; col = 1;
            continue;
        }

        /* ---- single-line comment  //  ---------------------------------- */
        if (ch == '/' && pos + 1 < length && src[pos + 1] == '/') {
            while (pos < length && src[pos] != '\n') pos++;
            continue;
        }

        /* ---- multi-line comment  / * ... * /  -------------------------- */
        if (ch == '/' && pos + 1 < length && src[pos + 1] == '*') {
            pos += 2; col += 2;
            while (pos + 1 < length) {
                if (src[pos] == '*' && src[pos + 1] == '/') { pos += 2; col += 2; break; }
                if (src[pos] == '\n') { line++; col = 1; } else { col++; }
                pos++;
            }
            continue;
        }

        /* ---- string literal  "..."  ------------------------------------ */
        if (ch == '"') {
            int start_col = col;
            char buf[MAX_LEXEME_LEN];
            int bi = 0;
            buf[bi++] = ch; pos++; col++;
            while (pos < length && src[pos] != '"' && bi < MAX_LEXEME_LEN - 2) {
                if (src[pos] == '\\') { buf[bi++] = src[pos]; pos++; col++; }
                if (pos < length) { buf[bi++] = src[pos]; pos++; col++; }
            }
            if (pos < length) { buf[bi++] = src[pos]; pos++; col++; } /* closing " */
            buf[bi] = '\0';
            add_token(tokens, &count, TOKEN_STRING, buf, line, start_col);
            continue;
        }

        /* ---- character literal  '.'  ----------------------------------- */
        if (ch == '\'') {
            int start_col = col;
            char buf[MAX_LEXEME_LEN];
            int bi = 0;
            buf[bi++] = ch; pos++; col++;
            while (pos < length && src[pos] != '\'' && bi < MAX_LEXEME_LEN - 2) {
                if (src[pos] == '\\') { buf[bi++] = src[pos]; pos++; col++; }
                if (pos < length) { buf[bi++] = src[pos]; pos++; col++; }
            }
            if (pos < length) { buf[bi++] = src[pos]; pos++; col++; }
            buf[bi] = '\0';
            add_token(tokens, &count, TOKEN_STRING, buf, line, start_col);
            continue;
        }

        /* ---- identifiers / keywords ------------------------------------ */
        if (isalpha((unsigned char)ch) || ch == '_') {
            int start_col = col;
            char buf[MAX_LEXEME_LEN];
            int bi = 0;
            while (pos < length &&
                   (isalnum((unsigned char)src[pos]) || src[pos] == '_') &&
                   bi < MAX_LEXEME_LEN - 1) {
                buf[bi++] = src[pos]; pos++; col++;
            }
            buf[bi] = '\0';
            TokenType tt = is_keyword(buf) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
            add_token(tokens, &count, tt, buf, line, start_col);
            continue;
        }

        /* ---- numeric constants ----------------------------------------- */
        if (isdigit((unsigned char)ch)) {
            int start_col = col;
            int is_float  = 0;
            char buf[MAX_LEXEME_LEN];
            int bi = 0;
            while (pos < length && isdigit((unsigned char)src[pos]) && bi < MAX_LEXEME_LEN - 1) {
                buf[bi++] = src[pos]; pos++; col++;
            }
            if (pos < length && src[pos] == '.') {
                is_float = 1;
                buf[bi++] = src[pos]; pos++; col++;
                while (pos < length && isdigit((unsigned char)src[pos]) && bi < MAX_LEXEME_LEN - 1) {
                    buf[bi++] = src[pos]; pos++; col++;
                }
            }
            buf[bi] = '\0';
            add_token(tokens, &count, is_float ? TOKEN_FLOAT : TOKEN_INTEGER,
                      buf, line, start_col);
            continue;
        }

        /* ---- two-character operators ----------------------------------- */
        if (pos + 1 < length) {
            char two[3] = { src[pos], src[pos + 1], '\0' };
            /* list of all two-char operators */
            static const char *twos[] = {
                "==", "!=", "<=", ">=", "++", "--",
                "+=", "-=", "*=", "/=", "%=",
                "&&", "||", "<<", ">>", NULL
            };
            int found = 0;
            for (int i = 0; twos[i] != NULL; i++) {
                if (two[0] == twos[i][0] && two[1] == twos[i][1]) {
                    add_token(tokens, &count, TOKEN_OPERATOR, two, line, col);
                    pos += 2; col += 2;
                    found = 1;
                    break;
                }
            }
            if (found) continue;
        }

        /* ---- single-character operators -------------------------------- */
        if (is_operator_char(ch)) {
            char buf[2] = { ch, '\0' };
            add_token(tokens, &count, TOKEN_OPERATOR, buf, line, col);
            pos++; col++;
            continue;
        }

        /* ---- punctuation / delimiters ---------------------------------- */
        if (is_punctuation(ch)) {
            char buf[2] = { ch, '\0' };
            add_token(tokens, &count, TOKEN_PUNCTUATION, buf, line, col);
            pos++; col++;
            continue;
        }

        /* ---- unknown --------------------------------------------------- */
        {
            char buf[2] = { ch, '\0' };
            add_token(tokens, &count, TOKEN_UNKNOWN, buf, line, col);
            pos++; col++;
        }
    }

    return count;
}

/* ---- pretty print ------------------------------------------------------- */
static void print_tokens(const Token tokens[], int count)
{
    /* find max lexeme width */
    int max_lex = 6; /* minimum "LEXEME" header */
    for (int i = 0; i < count; i++) {
        int len = (int)strlen(tokens[i].lexeme);
        if (len > max_lex) max_lex = len;
    }

    printf("+---+-------------------+");
    for (int i = 0; i < max_lex + 2; i++) putchar('-');
    printf("+----------+\n");

    printf("| # | %-17s | %-*s | LOCATION |\n", "TOKEN TYPE", max_lex, "LEXEME");

    printf("+---+-------------------+");
    for (int i = 0; i < max_lex + 2; i++) putchar('-');
    printf("+----------+\n");

    for (int i = 0; i < count; i++) {
        printf("|%3d| %-17s | %-*s | L%d:C%-4d |\n",
               i + 1,
               token_type_names[tokens[i].type],
               max_lex,
               tokens[i].lexeme,
               tokens[i].line,
               tokens[i].col);
    }

    printf("+---+-------------------+");
    for (int i = 0; i < max_lex + 2; i++) putchar('-');
    printf("+----------+\n");
}

static void print_summary(const Token tokens[], int count)
{
    int cats[8] = {0};
    for (int i = 0; i < count; i++)
        cats[tokens[i].type]++;

    printf("\n+-------------------+-------+\n");
    printf("| TOKEN TYPE        | COUNT |\n");
    printf("+-------------------+-------+\n");
    for (int i = 0; i < 8; i++) {
        if (cats[i] > 0)
            printf("| %-17s | %5d |\n", token_type_names[i], cats[i]);
    }
    printf("+-------------------+-------+\n");
    printf("| %-17s | %5d |\n", "TOTAL", count);
    printf("+-------------------+-------+\n");
}

/* ---- default sample code ------------------------------------------------ */
static const char *SAMPLE_CODE =
    "// A small C program for lexical analysis demonstration\n"
    "#include <stdio.h>\n"
    "\n"
    "int main() {\n"
    "    int count = 0;\n"
    "    float ratio = 3.14;\n"
    "    char name[] = \"hello\";\n"
    "\n"
    "    for (int i = 0; i <= 10; i++) {\n"
    "        if (i != 5 && i >= 2) {\n"
    "            count += i;\n"
    "        } else {\n"
    "            count--;\n"
    "        }\n"
    "    }\n"
    "\n"
    "    while (count > 0) {\n"
    "        ratio *= 0.5;\n"
    "        count = count - 1;\n"
    "    }\n"
    "\n"
    "    printf(\"Result: %d\\n\", count);\n"
    "    return 0;\n"
    "}\n";

/* ---- main --------------------------------------------------------------- */
int main(void)
{
    printf("======================================================================\n");
    printf("         LEXICAL ANALYZER  --  Compiler Design Exercise 1\n");
    printf("======================================================================\n\n");

    printf("Choose input mode:\n");
    printf("  1) Use default sample C code\n");
    printf("  2) Enter your own code (end with an empty line)\n");
    printf("\nEnter choice [1]: ");

    char choice_buf[16];
    if (fgets(choice_buf, sizeof(choice_buf), stdin) == NULL)
        choice_buf[0] = '1';

    char source[MAX_SOURCE_LEN];

    if (choice_buf[0] == '2') {
        printf("\nEnter source code (press Enter on an empty line to finish):\n\n");
        source[0] = '\0';
        char line_buf[512];
        while (fgets(line_buf, sizeof(line_buf), stdin) != NULL) {
            if (line_buf[0] == '\n') break;
            strncat(source, line_buf, MAX_SOURCE_LEN - strlen(source) - 1);
        }
    } else {
        strncpy(source, SAMPLE_CODE, MAX_SOURCE_LEN - 1);
        source[MAX_SOURCE_LEN - 1] = '\0';
    }

    /* display source */
    printf("\n--- Source Code ---\n");
    int line_no = 1;
    const char *p = source;
    printf("  %3d | ", line_no);
    while (*p) {
        putchar(*p);
        if (*p == '\n' && *(p + 1)) {
            line_no++;
            printf("  %3d | ", line_no);
        }
        p++;
    }
    printf("\n--- End of Source ---\n\n");

    /* tokenize and display */
    Token tokens[MAX_TOKENS];
    int count = tokenize(source, tokens);

    printf("--- Token Table ---\n");
    print_tokens(tokens, count);
    print_summary(tokens, count);

    return 0;
}
