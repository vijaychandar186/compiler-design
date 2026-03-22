/*
 * ===========================================================================
 * Exercise 11: Three-Address Code (TAC) Generation  --  C Implementation
 * ===========================================================================
 *
 * CONCEPT:
 *   Three-Address Code is an intermediate representation used in compilers
 *   where each instruction has at most three operands (two sources and one
 *   destination).  Complex expressions are decomposed into a sequence of
 *   simple instructions, each involving at most one arithmetic operator.
 *   Temporary variables (t1, t2, ...) hold intermediate results.
 *
 * ALGORITHM:
 *   A recursive descent parser walks the token stream.  Operator precedence
 *   is encoded directly in the grammar:
 *       assignment  -> ID '=' expression
 *       expression  -> term (('+' | '-') term)*
 *       term        -> factor (('*' | '/') factor)*
 *       factor      -> ID | NUMBER | '(' expression ')'
 *   Each binary operation emits a TAC instruction with a fresh temporary
 *   and returns the temporary name so the caller can reference it.
 *
 * TIME COMPLEXITY:  O(n), single pass over the expression.
 * SPACE COMPLEXITY: O(n) for tokens and generated instructions.
 *
 * EXAMPLE:
 *   Input:  x = a + b * c - d / e
 *   Output:
 *       t1 = b * c
 *       t2 = a + t1
 *       t3 = d / e
 *       t4 = t2 - t3
 *       x  = t4
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- constants ----------------------------------------------------- */
#define MAX_TOKEN_LEN  64
#define MAX_TOKENS     256
#define MAX_TAC        256

/* ---------- token types --------------------------------------------------- */
typedef enum { TOK_ID, TOK_NUM, TOK_OP, TOK_EOF } TokenType;

typedef struct {
    TokenType type;
    char      value[MAX_TOKEN_LEN];
} Token;

/* ---------- TAC instruction ----------------------------------------------- */
typedef struct {
    char result[MAX_TOKEN_LEN];
    char left[MAX_TOKEN_LEN];
    char op;           /* '+', '-', '*', '/', or '\0' for simple copy */
    char right[MAX_TOKEN_LEN];
} TACInstr;

/* ---------- global state -------------------------------------------------- */
static Token    tokens[MAX_TOKENS];
static int      token_count = 0;
static int      token_pos   = 0;

static TACInstr tac[MAX_TAC];
static int      tac_count   = 0;
static int      temp_counter = 0;

/* ---------- utility functions --------------------------------------------- */

/* Create a new temporary name and write it into `buf`. */
static void new_temp(char *buf, int buf_size)
{
    temp_counter++;
    snprintf(buf, buf_size, "t%d", temp_counter);
}

/* Emit a TAC instruction. `op` may be '\0' for a simple copy. */
static void emit(const char *result, const char *left, char op, const char *right)
{
    if (tac_count >= MAX_TAC) {
        fprintf(stderr, "Error: TAC instruction limit exceeded.\n");
        exit(1);
    }
    TACInstr *instr = &tac[tac_count++];
    strncpy(instr->result, result, MAX_TOKEN_LEN - 1);
    strncpy(instr->left,   left,   MAX_TOKEN_LEN - 1);
    instr->op = op;
    if (right)
        strncpy(instr->right, right, MAX_TOKEN_LEN - 1);
    else
        instr->right[0] = '\0';
}

/* ---------- lexer --------------------------------------------------------- */

static void tokenize(const char *text)
{
    token_count = 0;
    int i = 0, len = (int)strlen(text);

    while (i < len) {
        if (isspace((unsigned char)text[i])) {
            i++;
        } else if (strchr("+-*/=()", text[i])) {
            tokens[token_count].type = TOK_OP;
            tokens[token_count].value[0] = text[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
        } else if (isalpha((unsigned char)text[i]) || text[i] == '_') {
            int start = i;
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '_'))
                i++;
            int span = i - start;
            if (span >= MAX_TOKEN_LEN) span = MAX_TOKEN_LEN - 1;
            strncpy(tokens[token_count].value, text + start, span);
            tokens[token_count].value[span] = '\0';
            tokens[token_count].type = TOK_ID;
            token_count++;
        } else if (isdigit((unsigned char)text[i])) {
            int start = i;
            while (i < len && isdigit((unsigned char)text[i]))
                i++;
            int span = i - start;
            if (span >= MAX_TOKEN_LEN) span = MAX_TOKEN_LEN - 1;
            strncpy(tokens[token_count].value, text + start, span);
            tokens[token_count].value[span] = '\0';
            tokens[token_count].type = TOK_NUM;
            token_count++;
        } else {
            fprintf(stderr, "Error: unexpected character '%c'\n", text[i]);
            exit(1);
        }
    }
    tokens[token_count].type = TOK_EOF;
    tokens[token_count].value[0] = '\0';
    token_count++;
}

static Token *peek(void)
{
    return &tokens[token_pos];
}

static Token *advance(void)
{
    return &tokens[token_pos++];
}

static Token *expect(TokenType type, const char *value)
{
    Token *tok = advance();
    if (tok->type != type || (value && strcmp(tok->value, value) != 0)) {
        fprintf(stderr, "Error: expected (%d, %s), got (%d, %s)\n",
                type, value ? value : "?", tok->type, tok->value);
        exit(1);
    }
    return tok;
}

/* ---------- recursive descent parser -------------------------------------- */

/* Forward declarations. */
static void parse_expression(char *result_name);
static void parse_term(char *result_name);
static void parse_factor(char *result_name);

/*
 * factor -> ID | NUM | '(' expression ')'
 * Writes the operand name (variable, number, or temp) into result_name.
 */
static void parse_factor(char *result_name)
{
    Token *tok = peek();

    if (tok->type == TOK_ID) {
        advance();
        strcpy(result_name, tok->value);
    } else if (tok->type == TOK_NUM) {
        advance();
        strcpy(result_name, tok->value);
    } else if (tok->type == TOK_OP && tok->value[0] == '(') {
        advance(); /* consume '(' */
        parse_expression(result_name);
        expect(TOK_OP, ")");
    } else {
        fprintf(stderr, "Error: unexpected token '%s'\n", tok->value);
        exit(1);
    }
}

/*
 * term -> factor (('*' | '/') factor)*
 */
static void parse_term(char *result_name)
{
    char left[MAX_TOKEN_LEN];
    parse_factor(left);

    while (peek()->type == TOK_OP &&
           (peek()->value[0] == '*' || peek()->value[0] == '/')) {
        char op = advance()->value[0];
        char right[MAX_TOKEN_LEN];
        parse_factor(right);

        char temp[MAX_TOKEN_LEN];
        new_temp(temp, MAX_TOKEN_LEN);
        emit(temp, left, op, right);
        strcpy(left, temp);
    }

    strcpy(result_name, left);
}

/*
 * expression -> term (('+' | '-') term)*
 */
static void parse_expression(char *result_name)
{
    char left[MAX_TOKEN_LEN];
    parse_term(left);

    while (peek()->type == TOK_OP &&
           (peek()->value[0] == '+' || peek()->value[0] == '-')) {
        char op = advance()->value[0];
        char right[MAX_TOKEN_LEN];
        parse_term(right);

        char temp[MAX_TOKEN_LEN];
        new_temp(temp, MAX_TOKEN_LEN);
        emit(temp, left, op, right);
        strcpy(left, temp);
    }

    strcpy(result_name, left);
}

/*
 * assignment -> ID '=' expression
 */
static void parse_assignment(void)
{
    Token *name_tok = expect(TOK_ID, NULL);
    expect(TOK_OP, "=");

    char expr_result[MAX_TOKEN_LEN];
    parse_expression(expr_result);

    /* Final assignment */
    emit(name_tok->value, expr_result, '\0', NULL);
}

/* ---------- driver -------------------------------------------------------- */

static void generate_tac(const char *expression)
{
    tac_count    = 0;
    temp_counter = 0;
    token_pos    = 0;

    tokenize(expression);
    parse_assignment();
}

static void print_tac(void)
{
    for (int i = 0; i < tac_count; i++) {
        TACInstr *instr = &tac[i];
        if (instr->op != '\0')
            printf("    %s = %s %c %s\n",
                   instr->result, instr->left, instr->op, instr->right);
        else
            printf("    %s = %s\n", instr->result, instr->left);
    }
}

int main(void)
{
    const char *test_expressions[] = {
        "x = a + b * c - d / e",
        "y = (a + b) * (c - d)",
        "z = a * b + c * d + e * f",
        "w = a + b",
        "r = (a + b) * c - d / (e + f)"
    };
    int num_tests = sizeof(test_expressions) / sizeof(test_expressions[0]);

    printf("=================================================================\n");
    printf("   THREE-ADDRESS CODE GENERATION  (C)\n");
    printf("=================================================================\n\n");

    for (int i = 0; i < num_tests; i++) {
        printf("  --- Test Case %d ---\n", i + 1);
        printf("  Expression : %s\n", test_expressions[i]);
        printf("  Three-Address Code:\n");
        generate_tac(test_expressions[i]);
        print_tac();
        printf("\n");
    }

    printf("=================================================================\n");
    printf("  All expressions processed successfully.\n");
    printf("=================================================================\n");
    return 0;
}
