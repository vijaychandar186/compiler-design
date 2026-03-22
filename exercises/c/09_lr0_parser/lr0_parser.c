/*
 * ==============================================================================
 * Exercise 9: LR(0) Parser - Canonical Collection of Item Sets
 * ==============================================================================
 *
 * CONCEPT:
 * An LR(0) item is a production with a dot at some position in the RHS,
 * indicating how much of the production has been recognized. The canonical
 * collection of LR(0) item sets forms the basis for constructing LR parsers
 * (SLR, LALR, canonical LR).
 *
 * Each item set represents a parser state. Transitions between states on
 * grammar symbols form a DFA that guides shift-reduce parsing decisions.
 *
 * ALGORITHM:
 *   1. Augment grammar with S' -> S.
 *   2. CLOSURE: for item A -> alpha . B beta, add B -> . gamma for all
 *      productions of B. Repeat until no new items.
 *   3. GOTO(I, X): advance dot past X for matching items, then take closure.
 *   4. Build collection starting from I0 = CLOSURE({S' -> . S}).
 *   5. For each state and symbol, compute GOTO; add new states until stable.
 *
 * TIME COMPLEXITY:  O(|G|^2 * |V|) worst case.
 * SPACE COMPLEXITY: O(|G|^2) for item sets.
 *
 * EXAMPLE INPUT:
 *   E -> E + T | T
 *   T -> T * F | F
 *   F -> ( E ) | id
 *
 * EXAMPLE OUTPUT:
 *   I0: { E' -> . E, E -> . E + T, E -> . T, T -> . T * F, ... }
 *   I1: { E' -> E ., E -> E . + T }
 *   ...
 * ==============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PRODUCTIONS  20
#define MAX_SYMBOLS      10
#define MAX_SYMBOL_LEN   8
#define MAX_ITEMS        100
#define MAX_ITEM_SETS    50
#define MAX_ALL_SYMBOLS  30
#define MAX_GOTOS        200

/* --- Data structures --- */

typedef struct {
    char lhs[MAX_SYMBOL_LEN];
    char rhs[MAX_SYMBOLS][MAX_SYMBOL_LEN];
    int rhs_count;
} Production;

typedef struct {
    int prod_index;  /* Index into productions[] */
    int dot_pos;     /* Position of the dot (0 = before first symbol) */
} Item;

typedef struct {
    Item items[MAX_ITEMS];
    int item_count;
} ItemSet;

typedef struct {
    int from_state;
    char symbol[MAX_SYMBOL_LEN];
    int to_state;
} GotoEntry;

/* --- Global data --- */

static Production productions[MAX_PRODUCTIONS];
static int production_count = 0;

static char non_terminals[MAX_ALL_SYMBOLS][MAX_SYMBOL_LEN];
static int non_terminal_count = 0;

static char all_symbols[MAX_ALL_SYMBOLS][MAX_SYMBOL_LEN];
static int all_symbol_count = 0;

static ItemSet item_sets[MAX_ITEM_SETS];
static int item_set_count = 0;

static GotoEntry goto_table[MAX_GOTOS];
static int goto_count = 0;

/* --- Helper functions --- */

static int is_non_terminal(const char *sym)
{
    int i;
    for (i = 0; i < non_terminal_count; i++) {
        if (strcmp(non_terminals[i], sym) == 0)
            return 1;
    }
    return 0;
}

static void add_non_terminal(const char *sym)
{
    int i;
    for (i = 0; i < non_terminal_count; i++) {
        if (strcmp(non_terminals[i], sym) == 0)
            return;
    }
    strncpy(non_terminals[non_terminal_count], sym, MAX_SYMBOL_LEN - 1);
    non_terminals[non_terminal_count][MAX_SYMBOL_LEN - 1] = '\0';
    non_terminal_count++;
}

static void add_all_symbol(const char *sym)
{
    int i;
    for (i = 0; i < all_symbol_count; i++) {
        if (strcmp(all_symbols[i], sym) == 0)
            return;
    }
    strncpy(all_symbols[all_symbol_count], sym, MAX_SYMBOL_LEN - 1);
    all_symbols[all_symbol_count][MAX_SYMBOL_LEN - 1] = '\0';
    all_symbol_count++;
}

static int add_production(const char *lhs, const char *rhs[], int rhs_count)
{
    int i;
    Production *p = &productions[production_count];

    strncpy(p->lhs, lhs, MAX_SYMBOL_LEN - 1);
    p->lhs[MAX_SYMBOL_LEN - 1] = '\0';
    p->rhs_count = rhs_count;

    for (i = 0; i < rhs_count; i++) {
        strncpy(p->rhs[i], rhs[i], MAX_SYMBOL_LEN - 1);
        p->rhs[i][MAX_SYMBOL_LEN - 1] = '\0';
    }

    return production_count++;
}

static void collect_symbols(void)
{
    int i, j;
    for (i = 0; i < production_count; i++) {
        add_all_symbol(productions[i].lhs);
        for (j = 0; j < productions[i].rhs_count; j++) {
            add_all_symbol(productions[i].rhs[j]);
        }
    }
}

/* --- Item operations --- */

static int items_equal(const Item *a, const Item *b)
{
    return a->prod_index == b->prod_index && a->dot_pos == b->dot_pos;
}

static int item_set_contains(const ItemSet *set, const Item *item)
{
    int i;
    for (i = 0; i < set->item_count; i++) {
        if (items_equal(&set->items[i], item))
            return 1;
    }
    return 0;
}

static void item_set_add(ItemSet *set, const Item *item)
{
    if (!item_set_contains(set, item)) {
        set->items[set->item_count++] = *item;
    }
}

static const char *symbol_after_dot(const Item *item)
{
    Production *p = &productions[item->prod_index];
    if (item->dot_pos >= p->rhs_count)
        return NULL;
    return p->rhs[item->dot_pos];
}

/* --- Closure and GOTO --- */

static void compute_closure(ItemSet *set)
{
    /*
     * For each item with dot before a non-terminal B, add all
     * productions B -> . gamma. Repeat until no change.
     */
    int changed = 1;
    while (changed) {
        changed = 0;
        int current_count = set->item_count;
        int i, p;
        for (i = 0; i < current_count; i++) {
            const char *next_sym = symbol_after_dot(&set->items[i]);
            if (next_sym == NULL || !is_non_terminal(next_sym))
                continue;

            for (p = 0; p < production_count; p++) {
                if (strcmp(productions[p].lhs, next_sym) == 0) {
                    Item new_item;
                    new_item.prod_index = p;
                    new_item.dot_pos = 0;
                    if (!item_set_contains(set, &new_item)) {
                        item_set_add(set, &new_item);
                        changed = 1;
                    }
                }
            }
        }
    }
}

static int compute_goto(const ItemSet *source, const char *symbol, ItemSet *result)
{
    int i;
    result->item_count = 0;

    /* Move dot past 'symbol' for matching items */
    for (i = 0; i < source->item_count; i++) {
        const char *next_sym = symbol_after_dot(&source->items[i]);
        if (next_sym != NULL && strcmp(next_sym, symbol) == 0) {
            Item advanced;
            advanced.prod_index = source->items[i].prod_index;
            advanced.dot_pos = source->items[i].dot_pos + 1;
            item_set_add(result, &advanced);
        }
    }

    if (result->item_count == 0)
        return 0;

    compute_closure(result);
    return 1;
}

/* --- Check if two item sets are identical --- */

static int item_sets_equal(const ItemSet *a, const ItemSet *b)
{
    int i;
    if (a->item_count != b->item_count)
        return 0;
    for (i = 0; i < a->item_count; i++) {
        if (!item_set_contains(b, &a->items[i]))
            return 0;
    }
    return 1;
}

static int find_item_set(const ItemSet *target)
{
    int i;
    for (i = 0; i < item_set_count; i++) {
        if (item_sets_equal(&item_sets[i], target))
            return i;
    }
    return -1;
}

/* --- Build canonical collection --- */

static void build_canonical_collection(void)
{
    int worklist[MAX_ITEM_SETS];
    int worklist_front = 0, worklist_back = 0;
    int i, s;

    /* I0 = closure({S' -> . S}) */
    item_sets[0].item_count = 0;
    Item initial;
    initial.prod_index = 0; /* First production is the augmented one */
    initial.dot_pos = 0;
    item_set_add(&item_sets[0], &initial);
    compute_closure(&item_sets[0]);
    item_set_count = 1;

    worklist[worklist_back++] = 0;

    while (worklist_front < worklist_back) {
        int state = worklist[worklist_front++];

        for (s = 0; s < all_symbol_count; s++) {
            ItemSet goto_result;
            if (!compute_goto(&item_sets[state], all_symbols[s], &goto_result))
                continue;

            int existing = find_item_set(&goto_result);
            int target;

            if (existing >= 0) {
                target = existing;
            } else {
                target = item_set_count;
                item_sets[item_set_count] = goto_result;
                item_set_count++;
                worklist[worklist_back++] = target;
            }

            /* Record GOTO transition */
            goto_table[goto_count].from_state = state;
            strncpy(goto_table[goto_count].symbol, all_symbols[s],
                    MAX_SYMBOL_LEN - 1);
            goto_table[goto_count].symbol[MAX_SYMBOL_LEN - 1] = '\0';
            goto_table[goto_count].to_state = target;
            goto_count++;
        }
    }
}

/* --- Printing --- */

static void print_item(const Item *item)
{
    Production *p = &productions[item->prod_index];
    int j;

    printf("    %s ->", p->lhs);
    for (j = 0; j < p->rhs_count; j++) {
        if (j == item->dot_pos)
            printf(" .");
        printf(" %s", p->rhs[j]);
    }
    if (item->dot_pos == p->rhs_count)
        printf(" .");
    printf("\n");
}

static void print_augmented_grammar(void)
{
    int i, j;

    printf("============================================================\n");
    printf("  Augmented Grammar\n");
    printf("============================================================\n");

    for (i = 0; i < production_count; i++) {
        printf("  (%d) %s ->", i, productions[i].lhs);
        for (j = 0; j < productions[i].rhs_count; j++) {
            printf(" %s", productions[i].rhs[j]);
        }
        printf("\n");
    }
    printf("============================================================\n");
}

static void print_all_item_sets(void)
{
    int i, j;

    printf("\n============================================================\n");
    printf("  Canonical Collection of LR(0) Item Sets\n");
    printf("  Total states: %d\n", item_set_count);
    printf("============================================================\n");

    for (i = 0; i < item_set_count; i++) {
        printf("\n  I%d:\n", i);
        printf("  ----------------------------------------\n");
        for (j = 0; j < item_sets[i].item_count; j++) {
            print_item(&item_sets[i].items[j]);
        }
    }

    printf("\n============================================================\n");
}

static void print_goto_transitions(void)
{
    int i;

    printf("\n============================================================\n");
    printf("  GOTO Transitions\n");
    printf("============================================================\n");

    for (i = 0; i < goto_count; i++) {
        printf("  GOTO(I%d, %s) = I%d\n",
               goto_table[i].from_state,
               goto_table[i].symbol,
               goto_table[i].to_state);
    }

    printf("============================================================\n");
}

/* --- Main --- */

int main(void)
{
    const char *r0[] = {"E"};
    const char *r1[] = {"E", "+", "T"};
    const char *r2[] = {"T"};
    const char *r3[] = {"T", "*", "F"};
    const char *r4[] = {"F"};
    const char *r5[] = {"(", "E", ")"};
    const char *r6[] = {"id"};

    printf("============================================================\n");
    printf("  LR(0) Parser - Canonical Collection Constructor\n");
    printf("============================================================\n");

    /* Register non-terminals first */
    add_non_terminal("E'");
    add_non_terminal("E");
    add_non_terminal("T");
    add_non_terminal("F");

    /* Augmented grammar: E' -> E */
    add_production("E'", r0, 1);
    /* E -> E + T | T */
    add_production("E", r1, 3);
    add_production("E", r2, 1);
    /* T -> T * F | F */
    add_production("T", r3, 3);
    add_production("T", r4, 1);
    /* F -> ( E ) | id */
    add_production("F", r5, 3);
    add_production("F", r6, 1);

    collect_symbols();
    print_augmented_grammar();

    build_canonical_collection();
    print_all_item_sets();
    print_goto_transitions();

    return 0;
}
