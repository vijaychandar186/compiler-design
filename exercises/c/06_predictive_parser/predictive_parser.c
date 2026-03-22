/*
================================================================================
Exercise 6: LL(1) Predictive Parser (C Implementation)
================================================================================

CONCEPT:
An LL(1) predictive parser is a top-down parser that uses a parsing table to
decide which production to apply. "LL(1)" means: scan input Left-to-right,
produce a Leftmost derivation, using 1 symbol of lookahead.

ALGORITHM:
1. Compute FIRST and FOLLOW sets.
2. Build parsing table M[A, a] = production to use when non-terminal A is on
   top of the stack and terminal a is the current input symbol.
3. Parse using a stack: start with [$, S], match terminals, expand non-terminals
   using the table, accept when both stack and input are $.

COMPLEXITY:
  Table construction: O(n * p * t)
  Parsing: O(input_length)
  Space: O(n * t) for the table

EXAMPLE:
  Grammar: E -> TR, R -> +TR | #, T -> FY, Y -> *FY | #, F -> (E) | i
  (Using single chars: i=id, #=epsilon)
  Input: i+i*i
  Output: Step-by-step parsing trace showing stack, input, and action.
================================================================================
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NONTERMS   10
#define MAX_PRODS      10
#define MAX_PROD_LEN   20
#define MAX_SET_SIZE   20
#define MAX_STACK      100
#define MAX_INPUT      100
#define MAX_TERMINALS  20

#define EPSILON '#'
#define END_MARKER '$'

/* ---- Grammar ---- */
typedef struct {
    char lhs;
    char rhs[MAX_PRODS][MAX_PROD_LEN];
    int num_prods;
} Rule;

char nonterminals[MAX_NONTERMS];
int num_nonterminals = 0;
char terminals[MAX_TERMINALS];
int num_terminals = 0;
Rule rules[MAX_NONTERMS];
int num_rules = 0;

/* FIRST and FOLLOW sets (indexed by position in nonterminals[]) */
char first_sets[MAX_NONTERMS][MAX_SET_SIZE];
int first_sizes[MAX_NONTERMS];
char follow_sets[MAX_NONTERMS][MAX_SET_SIZE];
int follow_sizes[MAX_NONTERMS];

/* Parsing table: table[nt_index][term_index] = production string, "" if empty */
/* Terminal index: index in all_terminals[] which includes terminals + $ */
char all_terminals[MAX_TERMINALS + 1];
int num_all_terminals = 0;
char parse_table[MAX_NONTERMS][MAX_TERMINALS + 1][MAX_PROD_LEN];

/* ---- Helpers ---- */

int is_nonterminal(char ch) {
    for (int i = 0; i < num_nonterminals; i++)
        if (nonterminals[i] == ch) return 1;
    return 0;
}

int nt_index(char ch) {
    for (int i = 0; i < num_nonterminals; i++)
        if (nonterminals[i] == ch) return i;
    return -1;
}

int term_index(char ch) {
    for (int i = 0; i < num_all_terminals; i++)
        if (all_terminals[i] == ch) return i;
    return -1;
}

int set_contains(char *set, int size, char ch) {
    for (int i = 0; i < size; i++)
        if (set[i] == ch) return 1;
    return 0;
}

int set_add(char *set, int *size, char ch) {
    if (set_contains(set, *size, ch)) return 0;
    set[(*size)++] = ch;
    return 1;
}

int set_add_all(char *dst, int *dst_size, char *src, int src_size, int excl_eps) {
    int changed = 0;
    for (int i = 0; i < src_size; i++) {
        if (excl_eps && src[i] == EPSILON) continue;
        if (set_add(dst, dst_size, src[i])) changed = 1;
    }
    return changed;
}

/* ---- Grammar setup ---- */

void add_nt(char ch) {
    if (!is_nonterminal(ch))
        nonterminals[num_nonterminals++] = ch;
}

void add_rule(char lhs, const char *prods[], int count) {
    rules[num_rules].lhs = lhs;
    rules[num_rules].num_prods = count;
    for (int i = 0; i < count; i++)
        strcpy(rules[num_rules].rhs[i], prods[i]);
    num_rules++;
    add_nt(lhs);
}

void collect_terminals(void) {
    num_terminals = 0;
    for (int r = 0; r < num_rules; r++) {
        for (int p = 0; p < rules[r].num_prods; p++) {
            for (int s = 0; rules[r].rhs[p][s]; s++) {
                char ch = rules[r].rhs[p][s];
                if (ch != EPSILON && !is_nonterminal(ch)) {
                    int found = 0;
                    for (int t = 0; t < num_terminals; t++)
                        if (terminals[t] == ch) { found = 1; break; }
                    if (!found) terminals[num_terminals++] = ch;
                }
            }
        }
    }
    /* Build all_terminals = terminals + $ */
    num_all_terminals = 0;
    for (int i = 0; i < num_terminals; i++)
        all_terminals[num_all_terminals++] = terminals[i];
    all_terminals[num_all_terminals++] = END_MARKER;
}

void print_grammar(void) {
    for (int r = 0; r < num_rules; r++) {
        printf("  %c -> ", rules[r].lhs);
        for (int p = 0; p < rules[r].num_prods; p++) {
            if (p > 0) printf(" | ");
            printf("%s", rules[r].rhs[p]);
        }
        printf("\n");
    }
}

/* ---- FIRST computation ---- */

void compute_first(void) {
    for (int i = 0; i < num_nonterminals; i++)
        first_sizes[i] = 0;

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int r = 0; r < num_rules; r++) {
            int ni = nt_index(rules[r].lhs);
            for (int p = 0; p < rules[r].num_prods; p++) {
                const char *prod = rules[r].rhs[p];
                if (prod[0] == EPSILON) {
                    if (set_add(first_sets[ni], &first_sizes[ni], EPSILON))
                        changed = 1;
                    continue;
                }
                int all_eps = 1;
                for (int s = 0; prod[s]; s++) {
                    char sym = prod[s];
                    if (!is_nonterminal(sym)) {
                        if (set_add(first_sets[ni], &first_sizes[ni], sym))
                            changed = 1;
                        all_eps = 0;
                        break;
                    }
                    int si = nt_index(sym);
                    if (set_add_all(first_sets[ni], &first_sizes[ni],
                                    first_sets[si], first_sizes[si], 1))
                        changed = 1;
                    if (!set_contains(first_sets[si], first_sizes[si], EPSILON)) {
                        all_eps = 0;
                        break;
                    }
                }
                if (all_eps) {
                    if (set_add(first_sets[ni], &first_sizes[ni], EPSILON))
                        changed = 1;
                }
            }
        }
    }
}

/* FIRST of a string (substring starting at position start) */
void first_of_string(const char *str, int start, char *result, int *result_size) {
    *result_size = 0;
    int all_eps = 1;
    for (int s = start; str[s]; s++) {
        char sym = str[s];
        if (!is_nonterminal(sym)) {
            set_add(result, result_size, sym);
            all_eps = 0;
            break;
        }
        int si = nt_index(sym);
        set_add_all(result, result_size, first_sets[si], first_sizes[si], 1);
        if (!set_contains(first_sets[si], first_sizes[si], EPSILON)) {
            all_eps = 0;
            break;
        }
    }
    if (all_eps) set_add(result, result_size, EPSILON);
}

/* ---- FOLLOW computation ---- */

void compute_follow(void) {
    for (int i = 0; i < num_nonterminals; i++)
        follow_sizes[i] = 0;

    set_add(follow_sets[0], &follow_sizes[0], END_MARKER);

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int r = 0; r < num_rules; r++) {
            int lhs_i = nt_index(rules[r].lhs);
            for (int p = 0; p < rules[r].num_prods; p++) {
                const char *prod = rules[r].rhs[p];
                int len = (int)strlen(prod);
                for (int s = 0; s < len; s++) {
                    if (!is_nonterminal(prod[s])) continue;
                    int si = nt_index(prod[s]);
                    if (s + 1 < len) {
                        char fb[MAX_SET_SIZE];
                        int fb_size;
                        first_of_string(prod, s + 1, fb, &fb_size);
                        if (set_add_all(follow_sets[si], &follow_sizes[si],
                                        fb, fb_size, 1))
                            changed = 1;
                        if (set_contains(fb, fb_size, EPSILON)) {
                            if (set_add_all(follow_sets[si], &follow_sizes[si],
                                            follow_sets[lhs_i], follow_sizes[lhs_i], 0))
                                changed = 1;
                        }
                    } else {
                        if (set_add_all(follow_sets[si], &follow_sizes[si],
                                        follow_sets[lhs_i], follow_sizes[lhs_i], 0))
                            changed = 1;
                    }
                }
            }
        }
    }
}

/* ---- Build parsing table ---- */

void build_table(void) {
    /* Initialize table entries to empty */
    for (int i = 0; i < num_nonterminals; i++)
        for (int j = 0; j < num_all_terminals; j++)
            parse_table[i][j][0] = '\0';

    for (int r = 0; r < num_rules; r++) {
        int ni = nt_index(rules[r].lhs);
        for (int p = 0; p < rules[r].num_prods; p++) {
            const char *prod = rules[r].rhs[p];

            /* Compute FIRST of this production */
            char fa[MAX_SET_SIZE];
            int fa_size;
            if (prod[0] == EPSILON) {
                fa_size = 1;
                fa[0] = EPSILON;
            } else {
                first_of_string(prod, 0, fa, &fa_size);
            }

            /* For each terminal in FIRST(alpha), set table entry */
            for (int k = 0; k < fa_size; k++) {
                if (fa[k] == EPSILON) continue;
                int ti = term_index(fa[k]);
                if (ti >= 0) strcpy(parse_table[ni][ti], prod);
            }

            /* If epsilon in FIRST(alpha), use FOLLOW */
            if (set_contains(fa, fa_size, EPSILON)) {
                for (int k = 0; k < follow_sizes[ni]; k++) {
                    int ti = term_index(follow_sets[ni][k]);
                    if (ti >= 0) strcpy(parse_table[ni][ti], prod);
                }
            }
        }
    }
}

void print_table(void) {
    int col_w = 12;
    printf("\n  %5s |", "");
    for (int j = 0; j < num_all_terminals; j++)
        printf(" %*c |", col_w, all_terminals[j]);
    printf("\n  ------+");
    for (int j = 0; j < num_all_terminals; j++) {
        for (int k = 0; k < col_w + 2; k++) printf("-");
        printf("+");
    }
    printf("\n");

    for (int i = 0; i < num_nonterminals; i++) {
        printf("  %5c |", nonterminals[i]);
        for (int j = 0; j < num_all_terminals; j++) {
            if (parse_table[i][j][0]) {
                char entry[30];
                snprintf(entry, sizeof(entry), "%c->%s", nonterminals[i], parse_table[i][j]);
                printf(" %*s |", col_w, entry);
            } else {
                printf(" %*s |", col_w, "");
            }
        }
        printf("\n");
    }
}

/* ---- Parsing ---- */

void parse_string(const char *input) {
    char stack[MAX_STACK];
    int stack_top = 0;

    /* Push $ and start symbol */
    stack[stack_top++] = END_MARKER;
    stack[stack_top++] = nonterminals[0];

    int pos = 0;
    int input_len = (int)strlen(input);
    int step = 0;

    printf("\n  %-6s %-25s %-20s %-30s\n", "Step", "Stack", "Input", "Action");
    printf("  %-6s %-25s %-20s %-30s\n", "------", "-------------------------",
           "--------------------", "------------------------------");

    while (1) {
        char top = stack[stack_top - 1];
        char current = (pos < input_len) ? input[pos] : END_MARKER;
        step++;

        /* Format stack string */
        char stack_str[MAX_STACK + 1];
        for (int i = 0; i < stack_top; i++)
            stack_str[i] = stack[i];
        stack_str[stack_top] = '\0';

        /* Format remaining input */
        char input_str[MAX_INPUT + 2];
        if (pos < input_len) {
            strcpy(input_str, input + pos);
            strcat(input_str, "$");
        } else {
            strcpy(input_str, "$");
        }

        if (top == END_MARKER && current == END_MARKER) {
            printf("  %-6d %-25s %-20s %-30s\n", step, stack_str, input_str, "ACCEPT");
            printf("\n  ** Input successfully parsed! **\n");
            return;
        }

        if (top == current) {
            char action[50];
            snprintf(action, sizeof(action), "Match '%c'", top);
            printf("  %-6d %-25s %-20s %-30s\n", step, stack_str, input_str, action);
            stack_top--;
            pos++;
        } else if (is_nonterminal(top)) {
            int ni = nt_index(top);
            int ti = term_index(current);
            if (ni >= 0 && ti >= 0 && parse_table[ni][ti][0]) {
                char action[50];
                snprintf(action, sizeof(action), "%c -> %s", top, parse_table[ni][ti]);
                printf("  %-6d %-25s %-20s %-30s\n", step, stack_str, input_str, action);

                stack_top--;  /* Pop non-terminal */
                const char *prod = parse_table[ni][ti];
                if (prod[0] != EPSILON) {
                    int plen = (int)strlen(prod);
                    for (int k = plen - 1; k >= 0; k--)
                        stack[stack_top++] = prod[k];
                }
            } else {
                printf("  %-6d %-25s %-20s ERROR: no table entry\n",
                       step, stack_str, input_str);
                printf("\n  ** Parse error at '%c' **\n", current);
                return;
            }
        } else {
            printf("  %-6d %-25s %-20s ERROR: mismatch\n",
                   step, stack_str, input_str);
            printf("\n  ** Parse error **\n");
            return;
        }
    }
}

int main(void) {
    printf("======================================================================\n");
    printf("  Exercise 6: LL(1) Predictive Parser\n");
    printf("======================================================================\n");
    printf("\n  (Using: i = id, # = epsilon, $ = end marker)\n\n");

    /* Build grammar: E -> TR, R -> +TR | #, T -> FY, Y -> *FY | #, F -> (E) | i */
    {
        const char *e_prods[] = {"TR"};
        add_rule('E', e_prods, 1);
        const char *r_prods[] = {"+TR", "#"};
        add_rule('R', r_prods, 2);
        const char *t_prods[] = {"FY"};
        add_rule('T', t_prods, 1);
        const char *y_prods[] = {"*FY", "#"};
        add_rule('Y', y_prods, 2);
        const char *f_prods[] = {"(E)", "i"};
        add_rule('F', f_prods, 2);
    }
    collect_terminals();

    printf("Grammar:\n");
    print_grammar();

    compute_first();
    compute_follow();

    printf("\nFIRST Sets:\n");
    for (int i = 0; i < num_nonterminals; i++) {
        printf("  FIRST(%c) = { ", nonterminals[i]);
        for (int j = 0; j < first_sizes[i]; j++) {
            if (j > 0) printf(", ");
            printf("%c", first_sets[i][j]);
        }
        printf(" }\n");
    }

    printf("\nFOLLOW Sets:\n");
    for (int i = 0; i < num_nonterminals; i++) {
        printf("  FOLLOW(%c) = { ", nonterminals[i]);
        for (int j = 0; j < follow_sizes[i]; j++) {
            if (j > 0) printf(", ");
            printf("%c", follow_sets[i][j]);
        }
        printf(" }\n");
    }

    build_table();
    printf("\nLL(1) Parsing Table:");
    print_table();

    /* Parse example inputs */
    const char *test_inputs[] = {"i+i*i", "i*(i+i)", "i+i"};
    int num_tests = 3;

    for (int t = 0; t < num_tests; t++) {
        printf("\n======================================================================\n");
        printf("\n  Parsing: \"%s\"\n", test_inputs[t]);
        parse_string(test_inputs[t]);
    }

    printf("\n======================================================================\n");
    return 0;
}
