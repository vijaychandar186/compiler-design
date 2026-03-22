/*
 * ==============================================================================
 * Exercise 8: LEADING and TRAILING Sets for Operator Grammars
 * ==============================================================================
 *
 * CONCEPT:
 * In operator precedence parsing, LEADING and TRAILING sets are used to
 * establish precedence relations between terminal symbols. An operator grammar
 * is a context-free grammar where no production right-hand side contains two
 * adjacent non-terminals.
 *
 * LEADING(A) is the set of terminals 'a' such that there exists a derivation
 * A =>* Ba... (the first terminal in some sentential form derived from A).
 * TRAILING(A) is the set of terminals 'a' such that there exists a derivation
 * A =>* ...aB (the last terminal in some sentential form derived from A).
 *
 * ALGORITHM:
 * We use an iterative fixed-point computation:
 *   1. Scan each production left-to-right for LEADING (right-to-left for
 *      TRAILING) and add the first (last) terminal encountered.
 *   2. Propagate: if A -> B..., then LEADING(B) is a subset of LEADING(A).
 *      Similarly for TRAILING with trailing non-terminals.
 *   3. Repeat until no changes occur.
 *
 * TIME COMPLEXITY:  O(|P| * |V|) per iteration, with at most |N|*|T| iterations.
 * SPACE COMPLEXITY: O(|N| * |T|) for storing the sets.
 *
 * EXAMPLE INPUT:
 *   E -> E + T | T
 *   T -> T * F | F
 *   F -> ( E ) | id
 *
 * EXAMPLE OUTPUT:
 *   LEADING(E) = { +, *, (, id }
 *   TRAILING(E) = { +, *, ), id }
 * ==============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NON_TERMINALS   10
#define MAX_TERMINALS       20
#define MAX_PRODUCTIONS     20
#define MAX_SYMBOLS         10
#define MAX_SYMBOL_LEN      8

/* A production: lhs -> symbols[0] symbols[1] ... symbols[symbol_count-1] */
typedef struct {
    int lhs_index;                              /* Index into non_terminals[] */
    char symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN];  /* RHS symbols */
    int symbol_count;
} Production;

/* Grammar data */
static char non_terminals[MAX_NON_TERMINALS][MAX_SYMBOL_LEN];
static int non_terminal_count = 0;

static char terminals[MAX_TERMINALS][MAX_SYMBOL_LEN];
static int terminal_count = 0;

static Production productions[MAX_PRODUCTIONS];
static int production_count = 0;

/* LEADING and TRAILING are boolean matrices: [non_terminal][terminal] */
static int leading[MAX_NON_TERMINALS][MAX_TERMINALS];
static int trailing[MAX_NON_TERMINALS][MAX_TERMINALS];

/* ---------- Helper functions ---------- */

static int find_or_add_non_terminal(const char *name)
{
    int i;
    for (i = 0; i < non_terminal_count; i++) {
        if (strcmp(non_terminals[i], name) == 0)
            return i;
    }
    strncpy(non_terminals[non_terminal_count], name, MAX_SYMBOL_LEN - 1);
    non_terminals[non_terminal_count][MAX_SYMBOL_LEN - 1] = '\0';
    return non_terminal_count++;
}

static int find_or_add_terminal(const char *name)
{
    int i;
    for (i = 0; i < terminal_count; i++) {
        if (strcmp(terminals[i], name) == 0)
            return i;
    }
    strncpy(terminals[terminal_count], name, MAX_SYMBOL_LEN - 1);
    terminals[terminal_count][MAX_SYMBOL_LEN - 1] = '\0';
    return terminal_count++;
}

static int is_non_terminal(const char *name)
{
    int i;
    for (i = 0; i < non_terminal_count; i++) {
        if (strcmp(non_terminals[i], name) == 0)
            return 1;
    }
    return 0;
}

static int get_non_terminal_index(const char *name)
{
    int i;
    for (i = 0; i < non_terminal_count; i++) {
        if (strcmp(non_terminals[i], name) == 0)
            return i;
    }
    return -1;
}

static int get_terminal_index(const char *name)
{
    int i;
    for (i = 0; i < terminal_count; i++) {
        if (strcmp(terminals[i], name) == 0)
            return i;
    }
    return -1;
}

/* ---------- Grammar setup ---------- */

static void add_production(const char *lhs, const char *rhs_symbols[], int rhs_count)
{
    int i;
    Production *prod = &productions[production_count];

    prod->lhs_index = find_or_add_non_terminal(lhs);
    prod->symbol_count = rhs_count;
    for (i = 0; i < rhs_count; i++) {
        strncpy(prod->symbols[i], rhs_symbols[i], MAX_SYMBOL_LEN - 1);
        prod->symbols[i][MAX_SYMBOL_LEN - 1] = '\0';
    }
    production_count++;
}

static void register_terminals(void)
{
    /*
     * After all productions are added, scan for terminals.
     * Any symbol that is not a non-terminal is a terminal.
     */
    int i, j;
    for (i = 0; i < production_count; i++) {
        for (j = 0; j < productions[i].symbol_count; j++) {
            if (!is_non_terminal(productions[i].symbols[j])) {
                find_or_add_terminal(productions[i].symbols[j]);
            }
        }
    }
}

/* ---------- LEADING and TRAILING computation ---------- */

static void compute_leading(void)
{
    int changed, i, j, nt_idx, t_idx, src;

    memset(leading, 0, sizeof(leading));

    /* Initial pass: for each production A -> X1 X2 ... scan from left */
    for (i = 0; i < production_count; i++) {
        nt_idx = productions[i].lhs_index;
        for (j = 0; j < productions[i].symbol_count; j++) {
            const char *sym = productions[i].symbols[j];
            if (!is_non_terminal(sym)) {
                t_idx = get_terminal_index(sym);
                if (t_idx >= 0)
                    leading[nt_idx][t_idx] = 1;
                break; /* Stop at first terminal */
            }
            /* Non-terminal: continue scanning (will propagate later) */
        }
    }

    /* Fixed-point iteration: propagate through leading non-terminals */
    changed = 1;
    while (changed) {
        changed = 0;
        for (i = 0; i < production_count; i++) {
            nt_idx = productions[i].lhs_index;
            for (j = 0; j < productions[i].symbol_count; j++) {
                const char *sym = productions[i].symbols[j];
                if (is_non_terminal(sym)) {
                    src = get_non_terminal_index(sym);
                    for (t_idx = 0; t_idx < terminal_count; t_idx++) {
                        if (leading[src][t_idx] && !leading[nt_idx][t_idx]) {
                            leading[nt_idx][t_idx] = 1;
                            changed = 1;
                        }
                    }
                    /* Continue: there might be more non-terminals before
                       the first terminal */
                } else {
                    break; /* Terminal reached; stop */
                }
            }
        }
    }
}

static void compute_trailing(void)
{
    int changed, i, j, nt_idx, t_idx, src;

    memset(trailing, 0, sizeof(trailing));

    /* Initial pass: scan from right */
    for (i = 0; i < production_count; i++) {
        nt_idx = productions[i].lhs_index;
        for (j = productions[i].symbol_count - 1; j >= 0; j--) {
            const char *sym = productions[i].symbols[j];
            if (!is_non_terminal(sym)) {
                t_idx = get_terminal_index(sym);
                if (t_idx >= 0)
                    trailing[nt_idx][t_idx] = 1;
                break;
            }
        }
    }

    /* Fixed-point iteration: propagate through trailing non-terminals */
    changed = 1;
    while (changed) {
        changed = 0;
        for (i = 0; i < production_count; i++) {
            nt_idx = productions[i].lhs_index;
            for (j = productions[i].symbol_count - 1; j >= 0; j--) {
                const char *sym = productions[i].symbols[j];
                if (is_non_terminal(sym)) {
                    src = get_non_terminal_index(sym);
                    for (t_idx = 0; t_idx < terminal_count; t_idx++) {
                        if (trailing[src][t_idx] && !trailing[nt_idx][t_idx]) {
                            trailing[nt_idx][t_idx] = 1;
                            changed = 1;
                        }
                    }
                } else {
                    break;
                }
            }
        }
    }
}

/* ---------- Printing ---------- */

static void print_grammar(void)
{
    int i;

    printf("\n==================================================\n");
    printf("  Grammar Productions\n");
    printf("==================================================\n");

    for (i = 0; i < production_count; i++) {
        int j;
        printf("  %s ->", non_terminals[productions[i].lhs_index]);
        for (j = 0; j < productions[i].symbol_count; j++) {
            printf(" %s", productions[i].symbols[j]);
        }
        printf("\n");
    }
    printf("==================================================\n");
}

static void print_set(const char *set_name, int set_matrix[][MAX_TERMINALS])
{
    int i, j, first;

    printf("\n==================================================\n");
    printf("  %s Sets\n", set_name);
    printf("==================================================\n");

    for (i = 0; i < non_terminal_count; i++) {
        printf("  %s(%s) = { ", set_name, non_terminals[i]);
        first = 1;
        for (j = 0; j < terminal_count; j++) {
            if (set_matrix[i][j]) {
                if (!first)
                    printf(", ");
                printf("%s", terminals[j]);
                first = 0;
            }
        }
        printf(" }\n");
    }
    printf("==================================================\n");
}

static void print_symbols(void)
{
    int i;

    printf("\n  Non-terminals: { ");
    for (i = 0; i < non_terminal_count; i++) {
        if (i > 0) printf(", ");
        printf("%s", non_terminals[i]);
    }
    printf(" }\n");

    printf("  Terminals:     { ");
    for (i = 0; i < terminal_count; i++) {
        if (i > 0) printf(", ");
        printf("%s", terminals[i]);
    }
    printf(" }\n");
}

/* ---------- Main ---------- */

int main(void)
{
    const char *rhs1[] = {"E", "+", "T"};
    const char *rhs2[] = {"T"};
    const char *rhs3[] = {"T", "*", "F"};
    const char *rhs4[] = {"F"};
    const char *rhs5[] = {"(", "E", ")"};
    const char *rhs6[] = {"id"};

    printf("==================================================\n");
    printf("  LEADING and TRAILING Sets Calculator\n");
    printf("  (Operator Precedence Parsing)\n");
    printf("==================================================\n");

    /* Build grammar: E -> E+T | T, T -> T*F | F, F -> (E) | id */
    /* First register all non-terminals so they are known before scanning */
    find_or_add_non_terminal("E");
    find_or_add_non_terminal("T");
    find_or_add_non_terminal("F");

    add_production("E", rhs1, 3);  /* E -> E + T */
    add_production("E", rhs2, 1);  /* E -> T     */
    add_production("T", rhs3, 3);  /* T -> T * F */
    add_production("T", rhs4, 1);  /* T -> F     */
    add_production("F", rhs5, 3);  /* F -> ( E ) */
    add_production("F", rhs6, 1);  /* F -> id    */

    register_terminals();
    print_grammar();
    print_symbols();

    compute_leading();
    compute_trailing();

    print_set("LEADING", leading);
    print_set("TRAILING", trailing);

    return 0;
}
