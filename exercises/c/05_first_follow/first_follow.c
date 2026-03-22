/*
================================================================================
Exercise 5: FIRST and FOLLOW Sets Computation (C Implementation)
================================================================================

CONCEPT:
FIRST and FOLLOW sets are fundamental to constructing predictive parsers (LL(1)).

FIRST(X) is the set of terminals that can appear as the first symbol of any
string derived from X. If X can derive epsilon (the empty string), then epsilon
is also in FIRST(X).

FOLLOW(A) is the set of terminals that can appear immediately after A in some
sentential form. For the start symbol, $ (end of input) is always in FOLLOW.

ALGORITHM (iterative fixed-point):
FIRST:
  1. If X is a terminal, FIRST(X) = {X}.
  2. If X -> epsilon, add epsilon to FIRST(X).
  3. If X -> Y1 Y2 ... Yk, add FIRST(Y1) - {epsilon} to FIRST(X).
     If epsilon in FIRST(Y1), also add FIRST(Y2) - {epsilon}, etc.
  Repeat until no changes.

FOLLOW:
  1. Add $ to FOLLOW(S).
  2. For A -> alpha B beta: add FIRST(beta) - {epsilon} to FOLLOW(B).
     If beta =>* epsilon or beta is empty, add FOLLOW(A) to FOLLOW(B).
  Repeat until no changes.

COMPLEXITY:
  Time:  O(n * p * k) per iteration, O(n) iterations worst case
  Space: O(n * t) where n = non-terminals, t = terminals

EXAMPLE:
  Grammar: E -> T R, R -> + T R | #, T -> F Y, Y -> * F Y | #, F -> ( E ) | id
  (Using '#' for epsilon, '$' for end marker)
  FIRST(E) = { (, i }     FOLLOW(E) = { $, ) }
  FIRST(R) = { +, # }     FOLLOW(R) = { $, ) }
  FIRST(T) = { (, i }     FOLLOW(T) = { +, $, ) }
  FIRST(Y) = { *, # }     FOLLOW(Y) = { +, $, ) }
  FIRST(F) = { (, i }     FOLLOW(F) = { *, +, $, ) }
================================================================================
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NONTERMS   10
#define MAX_PRODS      10
#define MAX_PROD_LEN   10
#define MAX_SET_SIZE   20

/* '#' represents epsilon, '$' represents end-of-input */
#define EPSILON '#'
#define END_MARKER '$'

/* ---- Grammar representation ---- */
/* Non-terminals are uppercase letters; terminals are everything else. */
/* Each production is a string of characters (each char is one symbol). */
/* Special: "i" represents the terminal "id" (single char shorthand). */

typedef struct {
    char lhs;                                        /* non-terminal */
    char rhs[MAX_PRODS][MAX_PROD_LEN];               /* productions */
    int num_prods;
} Rule;

typedef struct {
    Rule rules[MAX_NONTERMS];
    int num_rules;
    char nonterminals[MAX_NONTERMS];
    int num_nonterminals;
    char terminals[MAX_SET_SIZE];
    int num_terminals;
} Grammar;

typedef struct {
    char elements[MAX_SET_SIZE];
    int size;
} CharSet;

/* ---- Set operations ---- */

void set_init(CharSet *set) {
    set->size = 0;
}

int set_contains(CharSet *set, char ch) {
    for (int i = 0; i < set->size; i++) {
        if (set->elements[i] == ch) return 1;
    }
    return 0;
}

/* Add element to set. Returns 1 if element was added (not already present). */
int set_add(CharSet *set, char ch) {
    if (set_contains(set, ch)) return 0;
    set->elements[set->size++] = ch;
    return 1;
}

/* Add all elements from src to dst (except optionally excluding epsilon).
   Returns 1 if any new element was added. */
int set_add_all(CharSet *dst, CharSet *src, int exclude_epsilon) {
    int changed = 0;
    for (int i = 0; i < src->size; i++) {
        if (exclude_epsilon && src->elements[i] == EPSILON) continue;
        if (set_add(dst, src->elements[i])) changed = 1;
    }
    return changed;
}

void set_print(CharSet *set) {
    printf("{ ");
    for (int i = 0; i < set->size; i++) {
        if (i > 0) printf(", ");
        if (set->elements[i] == EPSILON)
            printf("#");
        else if (set->elements[i] == END_MARKER)
            printf("$");
        else
            printf("%c", set->elements[i]);
    }
    printf(" }");
}

/* ---- Grammar operations ---- */

int is_nonterminal(Grammar *grammar, char ch) {
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        if (grammar->nonterminals[i] == ch) return 1;
    }
    return 0;
}

int find_rule(Grammar *grammar, char nonterminal) {
    for (int i = 0; i < grammar->num_rules; i++) {
        if (grammar->rules[i].lhs == nonterminal) return i;
    }
    return -1;
}

void init_grammar(Grammar *grammar) {
    grammar->num_rules = 0;
    grammar->num_nonterminals = 0;
    grammar->num_terminals = 0;
}

void add_rule(Grammar *grammar, char lhs, const char *prods[], int num_prods) {
    Rule *rule = &grammar->rules[grammar->num_rules];
    rule->lhs = lhs;
    rule->num_prods = num_prods;
    for (int i = 0; i < num_prods; i++) {
        strcpy(rule->rhs[i], prods[i]);
    }
    grammar->num_rules++;

    /* Track non-terminal */
    int found = 0;
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        if (grammar->nonterminals[i] == lhs) { found = 1; break; }
    }
    if (!found) grammar->nonterminals[grammar->num_nonterminals++] = lhs;
}

void collect_terminals(Grammar *grammar) {
    grammar->num_terminals = 0;
    for (int r = 0; r < grammar->num_rules; r++) {
        for (int p = 0; p < grammar->rules[r].num_prods; p++) {
            const char *prod = grammar->rules[r].rhs[p];
            for (int s = 0; (size_t)s < strlen(prod); s++) {
                char ch = prod[s];
                if (ch == EPSILON) continue;
                if (!is_nonterminal(grammar, ch)) {
                    int found = 0;
                    for (int t = 0; t < grammar->num_terminals; t++) {
                        if (grammar->terminals[t] == ch) { found = 1; break; }
                    }
                    if (!found) grammar->terminals[grammar->num_terminals++] = ch;
                }
            }
        }
    }
}

void print_grammar(Grammar *grammar) {
    for (int r = 0; r < grammar->num_rules; r++) {
        Rule *rule = &grammar->rules[r];
        printf("  %c -> ", rule->lhs);
        for (int p = 0; p < rule->num_prods; p++) {
            if (p > 0) printf(" | ");
            printf("%s", rule->rhs[p]);
        }
        printf("\n");
    }
}

/* ---- FIRST computation ---- */

void compute_first(Grammar *grammar, CharSet first[]) {
    /* Initialize all FIRST sets */
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        set_init(&first[i]);
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int r = 0; r < grammar->num_rules; r++) {
            Rule *rule = &grammar->rules[r];
            int nt_idx = -1;
            for (int i = 0; i < grammar->num_nonterminals; i++) {
                if (grammar->nonterminals[i] == rule->lhs) { nt_idx = i; break; }
            }
            if (nt_idx < 0) continue;

            for (int p = 0; p < rule->num_prods; p++) {
                const char *prod = rule->rhs[p];

                /* Epsilon production */
                if (prod[0] == EPSILON) {
                    if (set_add(&first[nt_idx], EPSILON)) changed = 1;
                    continue;
                }

                /* Process symbols left to right */
                int all_have_epsilon = 1;
                for (int s = 0; prod[s] != '\0'; s++) {
                    char sym = prod[s];
                    if (!is_nonterminal(grammar, sym)) {
                        /* Terminal */
                        if (set_add(&first[nt_idx], sym)) changed = 1;
                        all_have_epsilon = 0;
                        break;
                    } else {
                        /* Non-terminal: find its index and add FIRST - {epsilon} */
                        int sym_idx = -1;
                        for (int i = 0; i < grammar->num_nonterminals; i++) {
                            if (grammar->nonterminals[i] == sym) { sym_idx = i; break; }
                        }
                        if (sym_idx >= 0) {
                            if (set_add_all(&first[nt_idx], &first[sym_idx], 1))
                                changed = 1;
                            if (!set_contains(&first[sym_idx], EPSILON)) {
                                all_have_epsilon = 0;
                                break;
                            }
                        }
                    }
                }
                if (all_have_epsilon) {
                    if (set_add(&first[nt_idx], EPSILON)) changed = 1;
                }
            }
        }
    }
}

/* Compute FIRST of a string of symbols (for FOLLOW computation) */
void first_of_string(Grammar *grammar, CharSet first[], const char *str,
                     int start, CharSet *result) {
    set_init(result);
    int all_have_epsilon = 1;

    for (int s = start; str[s] != '\0'; s++) {
        char sym = str[s];
        if (!is_nonterminal(grammar, sym)) {
            set_add(result, sym);
            all_have_epsilon = 0;
            break;
        } else {
            int sym_idx = -1;
            for (int i = 0; i < grammar->num_nonterminals; i++) {
                if (grammar->nonterminals[i] == sym) { sym_idx = i; break; }
            }
            if (sym_idx >= 0) {
                set_add_all(result, &first[sym_idx], 1);
                if (!set_contains(&first[sym_idx], EPSILON)) {
                    all_have_epsilon = 0;
                    break;
                }
            }
        }
    }
    if (all_have_epsilon) {
        set_add(result, EPSILON);
    }
}

/* ---- FOLLOW computation ---- */

void compute_follow(Grammar *grammar, CharSet first[], CharSet follow[]) {
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        set_init(&follow[i]);
    }

    /* Add $ to FOLLOW of start symbol */
    set_add(&follow[0], END_MARKER);

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int r = 0; r < grammar->num_rules; r++) {
            Rule *rule = &grammar->rules[r];
            int lhs_idx = -1;
            for (int i = 0; i < grammar->num_nonterminals; i++) {
                if (grammar->nonterminals[i] == rule->lhs) { lhs_idx = i; break; }
            }

            for (int p = 0; p < rule->num_prods; p++) {
                const char *prod = rule->rhs[p];
                int len = (int)strlen(prod);

                for (int s = 0; s < len; s++) {
                    char sym = prod[s];
                    if (!is_nonterminal(grammar, sym)) continue;

                    int sym_idx = -1;
                    for (int i = 0; i < grammar->num_nonterminals; i++) {
                        if (grammar->nonterminals[i] == sym) { sym_idx = i; break; }
                    }
                    if (sym_idx < 0) continue;

                    if (s + 1 < len) {
                        /* There are symbols after this non-terminal */
                        CharSet first_beta;
                        first_of_string(grammar, first, prod, s + 1, &first_beta);

                        if (set_add_all(&follow[sym_idx], &first_beta, 1))
                            changed = 1;

                        if (set_contains(&first_beta, EPSILON)) {
                            if (set_add_all(&follow[sym_idx], &follow[lhs_idx], 0))
                                changed = 1;
                        }
                    } else {
                        /* Nothing after this non-terminal */
                        if (set_add_all(&follow[sym_idx], &follow[lhs_idx], 0))
                            changed = 1;
                    }
                }
            }
        }
    }
}

/* ---- Main ---- */

void print_sets(Grammar *grammar, CharSet first[], CharSet follow[]) {
    printf("\n  %-12s |  %-25s |  %-25s\n", "Non-Terminal", "FIRST", "FOLLOW");
    printf("  ------------ | ------------------------- | -------------------------\n");
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        printf("  %-12c |  ", grammar->nonterminals[i]);
        set_print(&first[i]);
        /* Pad to column width */
        int first_width = 2 + first[i].size * 3;
        for (int j = first_width; j < 25; j++) printf(" ");
        printf(" |  ");
        set_print(&follow[i]);
        printf("\n");
    }
}

int main(void) {
    Grammar grammar;
    CharSet first[MAX_NONTERMS];
    CharSet follow[MAX_NONTERMS];

    printf("======================================================================\n");
    printf("  Exercise 5: FIRST and FOLLOW Sets\n");
    printf("======================================================================\n");

    /* --- Example 1: Expression grammar ---
       E -> TR, R -> +TR | #, T -> FY, Y -> *FY | #, F -> (E) | i
       Using single chars: 'i' for 'id', '#' for epsilon */
    printf("\n--- Example 1: Expression Grammar ---\n\n");
    printf("  (Using: i = id, # = epsilon, $ = end marker)\n\n");

    init_grammar(&grammar);
    {
        const char *e_prods[] = {"TR"};
        add_rule(&grammar, 'E', e_prods, 1);
        const char *r_prods[] = {"+TR", "#"};
        add_rule(&grammar, 'R', r_prods, 2);
        const char *t_prods[] = {"FY"};
        add_rule(&grammar, 'T', t_prods, 1);
        const char *y_prods[] = {"*FY", "#"};
        add_rule(&grammar, 'Y', y_prods, 2);
        const char *f_prods[] = {"(E)", "i"};
        add_rule(&grammar, 'F', f_prods, 2);
    }
    collect_terminals(&grammar);

    printf("Grammar:\n");
    print_grammar(&grammar);

    compute_first(&grammar, first);
    compute_follow(&grammar, first, follow);
    print_sets(&grammar, first, follow);

    /* --- Example 2: Grammar with multiple epsilon productions --- */
    printf("\n----------------------------------------------------------------------\n");
    printf("\n--- Example 2: Multiple Epsilon Productions ---\n\n");

    init_grammar(&grammar);
    {
        /* S -> aBDh, B -> cC, C -> bC | #, D -> EF, E -> g | #, F -> f | # */
        const char *s_prods[] = {"aBDh"};
        add_rule(&grammar, 'S', s_prods, 1);
        const char *b_prods[] = {"cC"};
        add_rule(&grammar, 'B', b_prods, 1);
        const char *c_prods[] = {"bC", "#"};
        add_rule(&grammar, 'C', c_prods, 2);
        const char *d_prods[] = {"EF"};
        add_rule(&grammar, 'D', d_prods, 1);
        const char *e_prods[] = {"g", "#"};
        add_rule(&grammar, 'E', e_prods, 2);
        const char *f_prods[] = {"f", "#"};
        add_rule(&grammar, 'F', f_prods, 2);
    }
    collect_terminals(&grammar);

    printf("Grammar:\n");
    print_grammar(&grammar);

    compute_first(&grammar, first);
    compute_follow(&grammar, first, follow);
    print_sets(&grammar, first, follow);

    printf("\n======================================================================\n");
    return 0;
}
