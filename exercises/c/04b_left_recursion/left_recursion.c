/*
================================================================================
Exercise 4b: Left Recursion Elimination (C Implementation)
================================================================================

CONCEPT:
Left recursion in a context-free grammar occurs when a non-terminal A can
derive a sentential form starting with itself: A =>+ A alpha. This is
problematic for top-down (recursive descent / LL) parsers because they would
enter infinite loops trying to expand A.

ALGORITHM:
For direct left recursion:
  Original:    A -> A alpha1 | A alpha2 | ... | beta1 | beta2 | ...
  Transformed: A  -> beta1 A' | beta2 A' | ...
               A' -> alpha1 A' | alpha2 A' | ... | epsilon

For indirect left recursion, we use the ordering algorithm:
  1. Order non-terminals A1, A2, ..., An.
  2. For i = 1 to n:
       For j = 1 to i-1:
         Substitute Aj in Ai productions.
       Eliminate direct left recursion from Ai.

COMPLEXITY:
  Time:  O(n^2 * p) where n = number of non-terminals, p = max productions
  Space: O(n * p) for storing the grammar

EXAMPLE:
  Input:   E -> E+T | T,  T -> T*F | F,  F -> (E) | id
  Output:  E  -> TE'
           E' -> +TE' | epsilon
           T  -> FT'
           T' -> *FT' | epsilon
           F  -> (E) | id
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NONTERMS    20
#define MAX_PRODS       20
#define MAX_PROD_LEN    100
#define MAX_NAME_LEN    10

typedef struct {
    char name[MAX_NAME_LEN];                         /* non-terminal name */
    char productions[MAX_PRODS][MAX_PROD_LEN];       /* each production as string */
    int num_productions;
} NonTerminal;

typedef struct {
    NonTerminal nonterminals[MAX_NONTERMS];
    int num_nonterminals;
} Grammar;

/* ---- Utility functions ---- */

void init_grammar(Grammar *grammar) {
    grammar->num_nonterminals = 0;
}

int find_nonterminal(Grammar *grammar, const char *name) {
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        if (strcmp(grammar->nonterminals[i].name, name) == 0)
            return i;
    }
    return -1;
}

int add_nonterminal(Grammar *grammar, const char *name) {
    int idx = find_nonterminal(grammar, name);
    if (idx >= 0) return idx;
    idx = grammar->num_nonterminals;
    strcpy(grammar->nonterminals[idx].name, name);
    grammar->nonterminals[idx].num_productions = 0;
    grammar->num_nonterminals++;
    return idx;
}

/* Insert a non-terminal at a specific position, shifting others down */
void insert_nonterminal_at(Grammar *grammar, int pos, const char *name) {
    for (int i = grammar->num_nonterminals; i > pos; i--) {
        grammar->nonterminals[i] = grammar->nonterminals[i - 1];
    }
    strcpy(grammar->nonterminals[pos].name, name);
    grammar->nonterminals[pos].num_productions = 0;
    grammar->num_nonterminals++;
}

void add_production(Grammar *grammar, int nt_idx, const char *prod) {
    int p = grammar->nonterminals[nt_idx].num_productions;
    strcpy(grammar->nonterminals[nt_idx].productions[p], prod);
    grammar->nonterminals[nt_idx].num_productions++;
}

void print_grammar(Grammar *grammar) {
    for (int i = 0; i < grammar->num_nonterminals; i++) {
        NonTerminal *nt = &grammar->nonterminals[i];
        printf("  %s -> ", nt->name);
        for (int j = 0; j < nt->num_productions; j++) {
            if (j > 0) printf(" | ");
            printf("%s", nt->productions[j]);
        }
        printf("\n");
    }
}

/* Check if string starts with a given prefix */
int starts_with(const char *str, const char *prefix) {
    size_t plen = strlen(prefix);
    if (strncmp(str, prefix, plen) != 0) return 0;
    /* Make sure the prefix is a whole token: next char should be space, null, or
       a non-alphanumeric character that isn't part of the name (like prime). */
    char next = str[plen];
    if (next == '\0' || next == ' ') return 1;
    return 0;
}

/*
 * Eliminate direct left recursion for non-terminal at index nt_idx.
 * A -> A alpha1 | A alpha2 | ... | beta1 | beta2 | ...
 * becomes:
 *   A  -> beta1 A' | beta2 A' | ...
 *   A' -> alpha1 A' | alpha2 A' | ... | epsilon
 */
void eliminate_direct(Grammar *grammar, int nt_idx) {
    NonTerminal *nt = &grammar->nonterminals[nt_idx];
    char recursive[MAX_PRODS][MAX_PROD_LEN];
    char non_recursive[MAX_PRODS][MAX_PROD_LEN];
    int num_rec = 0, num_nonrec = 0;

    int name_len = (int)strlen(nt->name);

    /* Separate recursive and non-recursive productions */
    for (int i = 0; i < nt->num_productions; i++) {
        if (starts_with(nt->productions[i], nt->name)) {
            /* Skip the non-terminal name and the space after it */
            const char *alpha = nt->productions[i] + name_len;
            if (*alpha == ' ') alpha++;
            strcpy(recursive[num_rec], alpha);
            num_rec++;
        } else {
            strcpy(non_recursive[num_nonrec], nt->productions[i]);
            num_nonrec++;
        }
    }

    if (num_rec == 0) return;  /* No direct left recursion */

    /* Create new non-terminal name: A' */
    char new_name[MAX_NAME_LEN];
    snprintf(new_name, MAX_NAME_LEN, "%s'", nt->name);

    /* Replace A's productions: A -> beta1 A' | beta2 A' | ... */
    nt->num_productions = 0;
    for (int i = 0; i < num_nonrec; i++) {
        char new_prod[MAX_PROD_LEN];
        if (strcmp(non_recursive[i], "epsilon") == 0) {
            snprintf(new_prod, MAX_PROD_LEN, "%s", new_name);
        } else {
            snprintf(new_prod, MAX_PROD_LEN, "%s %s", non_recursive[i], new_name);
        }
        strcpy(nt->productions[nt->num_productions], new_prod);
        nt->num_productions++;
    }

    /* Insert A' right after A in the grammar */
    insert_nonterminal_at(grammar, nt_idx + 1, new_name);
    int prime_idx = nt_idx + 1;

    /* A' -> alpha1 A' | alpha2 A' | ... | epsilon */
    for (int i = 0; i < num_rec; i++) {
        char new_prod[MAX_PROD_LEN];
        if (strlen(recursive[i]) > 0) {
            snprintf(new_prod, MAX_PROD_LEN, "%s %s", recursive[i], new_name);
        } else {
            snprintf(new_prod, MAX_PROD_LEN, "%s", new_name);
        }
        add_production(grammar, prime_idx, new_prod);
    }
    add_production(grammar, prime_idx, "epsilon");
}

/*
 * Substitute: for every production of Ai that starts with Aj,
 * replace Aj with all of Aj's alternatives.
 */
void substitute(Grammar *grammar, int idx_i, int idx_j) {
    NonTerminal *nt_i = &grammar->nonterminals[idx_i];
    NonTerminal *nt_j = &grammar->nonterminals[idx_j];
    char new_prods[MAX_PRODS * MAX_PRODS][MAX_PROD_LEN];
    int num_new = 0;

    for (int p = 0; p < nt_i->num_productions; p++) {
        if (starts_with(nt_i->productions[p], nt_j->name)) {
            /* Get the gamma part (after Aj) */
            const char *gamma = nt_i->productions[p] + strlen(nt_j->name);
            if (*gamma == ' ') gamma++;

            /* Substitute each production of Aj */
            for (int q = 0; q < nt_j->num_productions; q++) {
                if (strcmp(nt_j->productions[q], "epsilon") == 0) {
                    if (strlen(gamma) > 0)
                        strcpy(new_prods[num_new], gamma);
                    else
                        strcpy(new_prods[num_new], "epsilon");
                } else {
                    if (strlen(gamma) > 0)
                        snprintf(new_prods[num_new], MAX_PROD_LEN, "%s %s",
                                 nt_j->productions[q], gamma);
                    else
                        strcpy(new_prods[num_new], nt_j->productions[q]);
                }
                num_new++;
            }
        } else {
            strcpy(new_prods[num_new], nt_i->productions[p]);
            num_new++;
        }
    }

    /* Replace productions of Ai */
    nt_i->num_productions = 0;
    for (int k = 0; k < num_new; k++) {
        strcpy(nt_i->productions[nt_i->num_productions], new_prods[k]);
        nt_i->num_productions++;
    }
}

/*
 * Build and run a grammar example.
 */
void build_expression_grammar(Grammar *grammar) {
    init_grammar(grammar);
    int e_idx = add_nonterminal(grammar, "E");
    int t_idx = add_nonterminal(grammar, "T");
    int f_idx = add_nonterminal(grammar, "F");

    add_production(grammar, e_idx, "E + T");
    add_production(grammar, e_idx, "T");

    add_production(grammar, t_idx, "T * F");
    add_production(grammar, t_idx, "F");

    add_production(grammar, f_idx, "( E )");
    add_production(grammar, f_idx, "id");
}

void build_indirect_grammar(Grammar *grammar) {
    init_grammar(grammar);
    int s_idx = add_nonterminal(grammar, "S");
    int a_idx = add_nonterminal(grammar, "A");

    add_production(grammar, s_idx, "A a");
    add_production(grammar, s_idx, "b");

    add_production(grammar, a_idx, "S c");
    add_production(grammar, a_idx, "d");
}

/*
 * Eliminate all left recursion (direct and indirect) using the standard
 * ordering algorithm. Tracks original names to handle index shifts from
 * inserting new non-terminals (A').
 */
void eliminate_all_left_recursion(Grammar *grammar) {
    /* Save original non-terminal names in order */
    char orig_names[MAX_NONTERMS][MAX_NAME_LEN];
    int orig_count = grammar->num_nonterminals;
    for (int i = 0; i < orig_count; i++) {
        strcpy(orig_names[i], grammar->nonterminals[i].name);
    }

    for (int i = 0; i < orig_count; i++) {
        for (int j = 0; j < i; j++) {
            int cur_i = find_nonterminal(grammar, orig_names[i]);
            int cur_j = find_nonterminal(grammar, orig_names[j]);
            if (cur_i >= 0 && cur_j >= 0) {
                substitute(grammar, cur_i, cur_j);
            }
        }
        int cur_i = find_nonterminal(grammar, orig_names[i]);
        if (cur_i >= 0) {
            eliminate_direct(grammar, cur_i);
        }
    }
}

int main(void) {
    Grammar grammar;

    printf("======================================================================\n");
    printf("  Exercise 4b: Left Recursion Elimination\n");
    printf("======================================================================\n");

    /* --- Example 1: Expression grammar with direct left recursion --- */
    printf("\n--- Example 1: Direct Left Recursion ---\n\n");
    build_expression_grammar(&grammar);
    printf("Original Grammar:\n");
    print_grammar(&grammar);

    eliminate_all_left_recursion(&grammar);
    printf("\nAfter Left Recursion Elimination:\n");
    print_grammar(&grammar);

    /* --- Example 2: Indirect left recursion --- */
    printf("\n----------------------------------------------------------------------\n");
    printf("\n--- Example 2: Indirect Left Recursion ---\n\n");
    build_indirect_grammar(&grammar);
    printf("Original Grammar:\n");
    print_grammar(&grammar);

    eliminate_all_left_recursion(&grammar);
    printf("\nAfter Left Recursion Elimination:\n");
    print_grammar(&grammar);

    /* --- Example 3: Single non-terminal with multiple recursive alts --- */
    printf("\n----------------------------------------------------------------------\n");
    printf("\n--- Example 3: Multiple Recursive Alternatives ---\n\n");
    init_grammar(&grammar);
    int a_idx = add_nonterminal(&grammar, "A");
    add_production(&grammar, a_idx, "A b c");
    add_production(&grammar, a_idx, "A d");
    add_production(&grammar, a_idx, "e");
    add_production(&grammar, a_idx, "f g");

    printf("Original Grammar:\n");
    print_grammar(&grammar);

    eliminate_all_left_recursion(&grammar);
    printf("\nAfter Left Recursion Elimination:\n");
    print_grammar(&grammar);

    printf("\n======================================================================\n");
    return 0;
}
