/*
================================================================================
Exercise 7: Shift-Reduce Parser (C Implementation)
================================================================================

CONCEPT:
A shift-reduce parser is a bottom-up parser that builds a parse tree from the
leaves up to the root. It uses a stack and processes the input left to right.
At each step, it either "shifts" the next input symbol onto the stack, or
"reduces" a sequence of symbols on top of the stack that matches the RHS of
a production, replacing them with the LHS non-terminal.

Four types of actions:
  - Shift: Push the next input symbol onto the stack.
  - Reduce: Replace a handle (matching RHS) on top of the stack with LHS.
  - Accept: Input consumed and stack contains only the start symbol.
  - Error: No valid action is possible.

ALGORITHM:
Uses operator-precedence to resolve ambiguity in E -> E+E | E*E | (E) | id.
Operator precedence (* > +) and left-associativity determine whether to shift
or reduce when E op E is on top of the stack and the next input is an operator.

COMPLEXITY:
  Time:  O(n^2) worst case, O(n) steps each potentially O(n) for handle search
  Space: O(n) for the stack

EXAMPLE:
  Input: id+id*id
  Result: id reduces to E, * has higher precedence than + so id*id reduces
          first as E*E -> E, then E+E -> E. Accept.
================================================================================
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_STACK   100
#define MAX_INPUT   100
#define MAX_TOKEN   10

/* ---- Token representation ----
   We use short strings as tokens: "id", "+", "*", "(", ")", "E", "$" */

typedef struct {
    char tokens[MAX_STACK][MAX_TOKEN];
    int top;  /* index of topmost element, -1 if empty */
} Stack;

void stack_init(Stack *s) { s->top = -1; }
int stack_size(Stack *s) { return s->top + 1; }
void stack_push(Stack *s, const char *tok) { strcpy(s->tokens[++(s->top)], tok); }
void stack_pop(Stack *s) { if (s->top >= 0) s->top--; }
const char *stack_at(Stack *s, int idx) { return s->tokens[idx]; }
const char *stack_top(Stack *s) { return (s->top >= 0) ? s->tokens[s->top] : ""; }

/* Format stack contents as a string */
void stack_to_string(Stack *s, char *buf, int bufsize) {
    buf[0] = '\0';
    if (s->top < 0) { strncpy(buf, "(empty)", bufsize); return; }
    for (int i = 0; i <= s->top; i++) {
        if (i > 0) strncat(buf, " ", bufsize - strlen(buf) - 1);
        strncat(buf, s->tokens[i], bufsize - strlen(buf) - 1);
    }
}

/* ---- Tokenizer ---- */

int tokenize(const char *input, char tokens[][MAX_TOKEN]) {
    int count = 0;
    int i = 0;
    int len = (int)strlen(input);
    while (i < len) {
        if (input[i] == ' ' || input[i] == '\t') { i++; continue; }
        if (i + 1 < len && input[i] == 'i' && input[i+1] == 'd') {
            strcpy(tokens[count++], "id");
            i += 2;
        } else {
            tokens[count][0] = input[i];
            tokens[count][1] = '\0';
            count++;
            i++;
        }
    }
    strcpy(tokens[count++], "$");
    return count;
}

/* ---- Precedence ---- */

int precedence(const char *op) {
    if (strcmp(op, "+") == 0) return 1;
    if (strcmp(op, "*") == 0) return 2;
    return 0;
}

int is_operator(const char *tok) {
    return (strcmp(tok, "+") == 0 || strcmp(tok, "*") == 0);
}

/* Find topmost operator on stack, returns its index or -1 */
int find_top_operator(Stack *s) {
    for (int i = s->top; i >= 0; i--) {
        if (is_operator(s->tokens[i])) return i;
    }
    return -1;
}

/* Should we reduce (1) or shift (0)? */
int should_reduce(Stack *s, const char *next_token) {
    int op_idx = find_top_operator(s);
    if (op_idx < 0) return 0;

    const char *stack_op = s->tokens[op_idx];

    if (!is_operator(next_token)) {
        /* Next is ) or $ -> reduce */
        return (strcmp(next_token, ")") == 0 || strcmp(next_token, "$") == 0);
    }

    return (precedence(stack_op) >= precedence(next_token));
}

/* ---- Reduction ----
   Returns 1 if a reduction was performed, 0 otherwise.
   Sets production_name to describe the reduction. */

int try_reduce(Stack *s, char *production_name) {
    int n = stack_size(s);

    /* E -> id */
    if (n >= 1 && strcmp(stack_top(s), "id") == 0) {
        stack_pop(s);
        stack_push(s, "E");
        strcpy(production_name, "E -> id");
        return 1;
    }

    /* E -> ( E ) */
    if (n >= 3 &&
        strcmp(stack_at(s, s->top - 2), "(") == 0 &&
        strcmp(stack_at(s, s->top - 1), "E") == 0 &&
        strcmp(stack_at(s, s->top), ")") == 0) {
        stack_pop(s); stack_pop(s); stack_pop(s);
        stack_push(s, "E");
        strcpy(production_name, "E -> ( E )");
        return 1;
    }

    /* E -> E + E */
    if (n >= 3 &&
        strcmp(stack_at(s, s->top - 2), "E") == 0 &&
        strcmp(stack_at(s, s->top - 1), "+") == 0 &&
        strcmp(stack_at(s, s->top), "E") == 0) {
        stack_pop(s); stack_pop(s); stack_pop(s);
        stack_push(s, "E");
        strcpy(production_name, "E -> E + E");
        return 1;
    }

    /* E -> E * E */
    if (n >= 3 &&
        strcmp(stack_at(s, s->top - 2), "E") == 0 &&
        strcmp(stack_at(s, s->top - 1), "*") == 0 &&
        strcmp(stack_at(s, s->top), "E") == 0) {
        stack_pop(s); stack_pop(s); stack_pop(s);
        stack_push(s, "E");
        strcpy(production_name, "E -> E * E");
        return 1;
    }

    return 0;
}

/* ---- Parser ---- */

int parse(const char *input) {
    char tokens[MAX_INPUT][MAX_TOKEN];
    int num_tokens = tokenize(input, tokens);
    Stack stack;
    stack_init(&stack);
    int pos = 0;
    int step = 0;
    char stack_str[500], remaining_str[500], production[50];

    printf("\n  %-6s %-30s %-25s %-30s\n", "Step", "Stack", "Input", "Action");
    printf("  %-6s %-30s %-25s %-30s\n",
           "------", "------------------------------",
           "-------------------------", "------------------------------");

    while (1) {
        const char *current = tokens[pos];
        step++;

        stack_to_string(&stack, stack_str, sizeof(stack_str));

        /* Build remaining input string */
        remaining_str[0] = '\0';
        for (int i = pos; i < num_tokens; i++) {
            if (i > pos) strcat(remaining_str, " ");
            strcat(remaining_str, tokens[i]);
        }

        /* Accept */
        if (stack_size(&stack) == 1 && strcmp(stack_top(&stack), "E") == 0 &&
            strcmp(current, "$") == 0) {
            printf("  %-6d %-30s %-25s %-30s\n", step, stack_str, remaining_str, "ACCEPT");
            printf("\n  ** Input \"%s\" successfully parsed! **\n", input);
            return 1;
        }

        /* Reduce id immediately */
        if (stack_size(&stack) >= 1 && strcmp(stack_top(&stack), "id") == 0) {
            if (try_reduce(&stack, production)) {
                char action[60];
                snprintf(action, sizeof(action), "Reduce: %s", production);
                printf("  %-6d %-30s %-25s %-30s\n", step, stack_str, remaining_str, action);
                continue;
            }
        }

        /* Reduce ( E ) */
        if (stack_size(&stack) >= 3 &&
            strcmp(stack_at(&stack, stack.top), ")") == 0 &&
            strcmp(stack_at(&stack, stack.top - 1), "E") == 0 &&
            strcmp(stack_at(&stack, stack.top - 2), "(") == 0) {
            if (try_reduce(&stack, production)) {
                char action[60];
                snprintf(action, sizeof(action), "Reduce: %s", production);
                printf("  %-6d %-30s %-25s %-30s\n", step, stack_str, remaining_str, action);
                continue;
            }
        }

        /* E op E on top: decide shift vs reduce */
        if (stack_size(&stack) >= 3 &&
            strcmp(stack_at(&stack, stack.top - 2), "E") == 0 &&
            is_operator(stack_at(&stack, stack.top - 1)) &&
            strcmp(stack_at(&stack, stack.top), "E") == 0) {
            if (should_reduce(&stack, current)) {
                if (try_reduce(&stack, production)) {
                    char action[60];
                    snprintf(action, sizeof(action), "Reduce: %s", production);
                    printf("  %-6d %-30s %-25s %-30s\n",
                           step, stack_str, remaining_str, action);
                    continue;
                }
            }
        }

        /* Shift */
        if (strcmp(current, "$") != 0) {
            char action[50];
            snprintf(action, sizeof(action), "Shift '%s'", current);
            printf("  %-6d %-30s %-25s %-30s\n", step, stack_str, remaining_str, action);
            stack_push(&stack, current);
            pos++;
        } else {
            /* Try final reduce */
            if (try_reduce(&stack, production)) {
                char action[60];
                snprintf(action, sizeof(action), "Reduce: %s", production);
                printf("  %-6d %-30s %-25s %-30s\n", step, stack_str, remaining_str, action);
            } else {
                printf("  %-6d %-30s %-25s %-30s\n",
                       step, stack_str, remaining_str, "ERROR");
                printf("\n  ** Parse error on \"%s\" **\n", input);
                return 0;
            }
        }
    }
}

int main(void) {
    printf("===========================================================================\n");
    printf("  Exercise 7: Shift-Reduce Parser\n");
    printf("===========================================================================\n");

    printf("\n  Grammar:\n");
    printf("    E -> E + E\n");
    printf("    E -> E * E\n");
    printf("    E -> ( E )\n");
    printf("    E -> id\n");
    printf("\n  Operator Precedence: * > +  (left-associative)\n");

    const char *test_inputs[] = {
        "id+id*id",
        "id*id+id",
        "id+id+id",
        "(id+id)*id",
        "id"
    };
    int num_tests = 5;

    for (int t = 0; t < num_tests; t++) {
        printf("\n===========================================================================\n");
        printf("\n  Parsing: \"%s\"\n", test_inputs[t]);
        parse(test_inputs[t]);
    }

    printf("\n===========================================================================\n");
    return 0;
}
