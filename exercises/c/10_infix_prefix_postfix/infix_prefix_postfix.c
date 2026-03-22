/*
 * ==============================================================================
 * Exercise 10: Infix to Prefix and Postfix Conversion
 * ==============================================================================
 *
 * CONCEPT:
 * Infix notation (A + B) is human-readable but ambiguous without precedence
 * rules. Prefix notation (+ A B) and postfix notation (A B +) are unambiguous
 * and do not need parentheses, making them ideal for compiler evaluation.
 *
 * ALGORITHM FOR INFIX TO POSTFIX (Shunting-Yard):
 *   Scan left to right. Operands go to output. Operators are pushed onto a
 *   stack after popping operators of higher-or-equal precedence (considering
 *   associativity). Parentheses control grouping by acting as barriers.
 *
 * ALGORITHM FOR INFIX TO PREFIX (Reverse-Scan Method):
 *   1. Reverse the expression, swapping '(' and ')'.
 *   2. Apply a modified shunting-yard where same-precedence left-associative
 *      operators do NOT pop each other (reversed associativity).
 *   3. Reverse the output.
 *
 * TIME COMPLEXITY:  O(n) where n is expression length.
 * SPACE COMPLEXITY: O(n) for the stack and output buffer.
 *
 * EXAMPLE INPUT:  (A+B)*C-D/E
 * EXAMPLE OUTPUT:
 *   Postfix: AB+C*DE/-
 *   Prefix:  -*+ABCDE/
 * ==============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_EXPR_LEN 256

/* --- Stack implementation --- */

typedef struct {
    char data[MAX_EXPR_LEN];
    int top;
} CharStack;

static void stack_init(CharStack *s) { s->top = -1; }
static int  stack_empty(const CharStack *s) { return s->top == -1; }
static void stack_push(CharStack *s, char c) { s->data[++(s->top)] = c; }
static char stack_pop(CharStack *s) { return s->data[(s->top)--]; }
static char stack_peek(const CharStack *s) { return s->data[s->top]; }

/* --- Operator utilities --- */

static int is_operator(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

static int precedence(char op)
{
    switch (op) {
    case '+': case '-': return 1;
    case '*': case '/': return 2;
    case '^':           return 3;
    default:            return 0;
    }
}

static int is_right_associative(char op)
{
    return op == '^';
}

/* --- Infix to Postfix (Shunting-Yard) --- */

static void infix_to_postfix(const char *infix, char *postfix)
{
    CharStack ops;
    int j = 0;
    int i;

    stack_init(&ops);

    for (i = 0; infix[i] != '\0'; i++) {
        char c = infix[i];

        if (c == ' ')
            continue;

        if (isalnum(c)) {
            /* Operand: send to output */
            postfix[j++] = c;
        } else if (c == '(') {
            stack_push(&ops, c);
        } else if (c == ')') {
            while (!stack_empty(&ops) && stack_peek(&ops) != '(') {
                postfix[j++] = stack_pop(&ops);
            }
            if (!stack_empty(&ops))
                stack_pop(&ops); /* Remove '(' */
        } else if (is_operator(c)) {
            while (!stack_empty(&ops) &&
                   stack_peek(&ops) != '(' &&
                   is_operator(stack_peek(&ops)) &&
                   (precedence(stack_peek(&ops)) > precedence(c) ||
                    (precedence(stack_peek(&ops)) == precedence(c) &&
                     !is_right_associative(c)))) {
                postfix[j++] = stack_pop(&ops);
            }
            stack_push(&ops, c);
        }
    }

    while (!stack_empty(&ops)) {
        char top = stack_pop(&ops);
        if (top != '(')
            postfix[j++] = top;
    }

    postfix[j] = '\0';
}

/* --- Infix to Prefix (Reverse-Scan Method) --- */

static void reverse_string(char *str)
{
    int len = (int)strlen(str);
    int i;
    for (i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

static void infix_to_prefix(const char *infix, char *prefix)
{
    CharStack ops;
    char reversed[MAX_EXPR_LEN];
    int len = (int)strlen(infix);
    int i, j = 0;

    stack_init(&ops);

    /* Step 1: Reverse the infix expression and swap parentheses */
    for (i = 0; i < len; i++) {
        char c = infix[len - 1 - i];
        if (c == '(')
            reversed[i] = ')';
        else if (c == ')')
            reversed[i] = '(';
        else
            reversed[i] = c;
    }
    reversed[len] = '\0';

    /* Step 2: Modified shunting-yard on the reversed expression.
     * For left-associative operators with same precedence, do NOT pop
     * (because the reversal inverts associativity). */
    for (i = 0; reversed[i] != '\0'; i++) {
        char c = reversed[i];

        if (c == ' ')
            continue;

        if (isalnum(c)) {
            prefix[j++] = c;
        } else if (c == '(') {
            stack_push(&ops, c);
        } else if (c == ')') {
            while (!stack_empty(&ops) && stack_peek(&ops) != '(') {
                prefix[j++] = stack_pop(&ops);
            }
            if (!stack_empty(&ops))
                stack_pop(&ops);
        } else if (is_operator(c)) {
            while (!stack_empty(&ops) &&
                   stack_peek(&ops) != '(' &&
                   is_operator(stack_peek(&ops)) &&
                   (precedence(stack_peek(&ops)) > precedence(c) ||
                    (precedence(stack_peek(&ops)) == precedence(c) &&
                     is_right_associative(c)))) {
                prefix[j++] = stack_pop(&ops);
            }
            stack_push(&ops, c);
        }
    }

    while (!stack_empty(&ops)) {
        char top = stack_pop(&ops);
        if (top != '(')
            prefix[j++] = top;
    }
    prefix[j] = '\0';

    /* Step 3: Reverse to get the final prefix expression */
    reverse_string(prefix);
}

/* --- Formatted output helper --- */

static void print_spaced(const char *str)
{
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if (i > 0)
            printf(" ");
        printf("%c", str[i]);
    }
}

static void convert_and_print(const char *infix, int example_num)
{
    char postfix[MAX_EXPR_LEN];
    char prefix[MAX_EXPR_LEN];

    infix_to_postfix(infix, postfix);
    infix_to_prefix(infix, prefix);

    printf("\n  --------------------------------------------------\n");
    printf("  Example %d:\n", example_num);
    printf("  --------------------------------------------------\n");
    printf("  Infix:   %s\n", infix);
    printf("  Postfix: ");
    print_spaced(postfix);
    printf("\n");
    printf("  Prefix:  ");
    print_spaced(prefix);
    printf("\n");
}

/* --- Main --- */

int main(void)
{
    const char *expressions[] = {
        "(A+B)*C-D/E",
        "A+B*C",
        "A*(B+C)/D",
        "A+B*C-D",
        "A^B^C",
        "((A+B)*C-(D-E))/(F+G)",
        "A*B+C*D",
        "A+B+C+D"
    };
    int count = sizeof(expressions) / sizeof(expressions[0]);
    int i;

    printf("=======================================================\n");
    printf("  Infix to Prefix and Postfix Converter\n");
    printf("=======================================================\n");

    for (i = 0; i < count; i++) {
        convert_and_print(expressions[i], i + 1);
    }

    printf("\n=======================================================\n");
    printf("\n  Note on right-associativity of ^ (exponentiation):\n");
    printf("  A^B^C is parsed as A^(B^C), NOT (A^B)^C\n");
    printf("  Postfix: A B C ^ ^  (B^C is computed first)\n");
    printf("  Prefix:  ^ A ^ B C\n");
    printf("\n=======================================================\n");

    return 0;
}
