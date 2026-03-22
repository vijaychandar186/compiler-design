/*
================================================================================
Exercise 3: NFA to DFA Conversion (Subset Construction) -- C Implementation
================================================================================

CONCEPT:
    The subset construction (powerset construction) converts an NFA into an
    equivalent DFA.  Each DFA state corresponds to a *set* of NFA states.
    The algorithm systematically explores all reachable subsets, computing
    epsilon-closures and move sets to build the DFA transition table.

ALGORITHM:
    1. Compute epsilon-closure({start}).  This becomes DFA state D0.
    2. While there are unmarked DFA states:
       a. Pick an unmarked DFA state D (a set of NFA states).
       b. For each input symbol 'a':
          - Compute move(D, a) = union of NFA targets on 'a'.
          - Compute epsilon-closure(move(D, a)) = new set T.
          - If T is not already a DFA state, create it.
          - Set DFA transition D --a--> T.
    3. A DFA state is accepting if it contains any NFA accepting state.

COMPLEXITY:
    Time:  O(2^n * m) worst case (n = NFA states, m = alphabet size).
    Space: O(2^n) for DFA states.

EXAMPLE:
    NFA for (a|b)*abb  ->  DFA with 5 states (D0 .. D4).
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- constants ---------------------------------------------------------- */
#define MAX_NFA_STATES  32
#define MAX_DFA_STATES  64
#define MAX_SYMBOLS     10
#define MAX_TRANSITIONS 256

/* ---- NFA representation using bitsets ----------------------------------- */
/* A set of NFA states is represented as a 32-bit bitmask (supports up to 32 states). */
typedef unsigned int StateSet;

#define SET_EMPTY       0u
#define SET_ADD(s, id)  ((s) | (1u << (id)))
#define SET_HAS(s, id)  (((s) >> (id)) & 1u)

/* NFA transition table: nfa_trans[state][sym_index] = StateSet of targets */
static StateSet nfa_trans[MAX_NFA_STATES][MAX_SYMBOLS];
static int      nfa_state_count = 0;
static int      nfa_start = 0;
static StateSet nfa_accept = SET_EMPTY;

/* symbol table: index 0 = epsilon */
static char     symbols[MAX_SYMBOLS];
static int      symbol_count = 0;   /* includes epsilon at index 0 */
#define EPS_INDEX 0

static int get_symbol_index(char ch) {
    for (int i = 0; i < symbol_count; i++)
        if (symbols[i] == ch) return i;
    symbols[symbol_count] = ch;
    return symbol_count++;
}

static void nfa_add_transition(int from, char symbol, int to) {
    int si = get_symbol_index(symbol);
    nfa_trans[from][si] = nfa_trans[from][si] | (1u << to);
}

/* ---- epsilon-closure ---------------------------------------------------- */
static StateSet epsilon_closure(StateSet input_set) {
    StateSet closure = input_set;
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int s = 0; s < nfa_state_count; s++) {
            if (!SET_HAS(closure, s)) continue;
            StateSet eps_targets = nfa_trans[s][EPS_INDEX];
            StateSet new_states = eps_targets & ~closure;
            if (new_states) {
                closure |= new_states;
                changed = 1;
            }
        }
    }
    return closure;
}

/* ---- move --------------------------------------------------------------- */
static StateSet move_set(StateSet input_set, int sym_index) {
    StateSet result = SET_EMPTY;
    for (int s = 0; s < nfa_state_count; s++) {
        if (SET_HAS(input_set, s))
            result |= nfa_trans[s][sym_index];
    }
    return result;
}

/* ---- DFA state table ---------------------------------------------------- */
typedef struct {
    StateSet nfa_set;
    int      transitions[MAX_SYMBOLS];  /* -1 = no transition */
    int      is_accept;
    int      marked;
} DFAStateEntry;

static DFAStateEntry dfa_states[MAX_DFA_STATES];
static int           dfa_count = 0;

static int find_or_add_dfa_state(StateSet nfa_set) {
    if (nfa_set == SET_EMPTY) return -1;
    for (int i = 0; i < dfa_count; i++)
        if (dfa_states[i].nfa_set == nfa_set) return i;
    /* new state */
    int id = dfa_count++;
    dfa_states[id].nfa_set = nfa_set;
    dfa_states[id].marked = 0;
    dfa_states[id].is_accept = (nfa_set & nfa_accept) ? 1 : 0;
    for (int i = 0; i < MAX_SYMBOLS; i++)
        dfa_states[id].transitions[i] = -1;
    return id;
}

/* ---- subset construction ------------------------------------------------ */
static void subset_construction(void) {
    StateSet start_set = epsilon_closure(SET_ADD(SET_EMPTY, nfa_start));
    find_or_add_dfa_state(start_set);

    int has_unmarked = 1;
    while (has_unmarked) {
        has_unmarked = 0;
        for (int d = 0; d < dfa_count; d++) {
            if (dfa_states[d].marked) continue;
            dfa_states[d].marked = 1;
            has_unmarked = 1;

            /* for each input symbol (skip epsilon at index 0) */
            for (int si = 1; si < symbol_count; si++) {
                StateSet moved = move_set(dfa_states[d].nfa_set, si);
                if (moved == SET_EMPTY) continue;
                StateSet closure = epsilon_closure(moved);
                int target = find_or_add_dfa_state(closure);
                dfa_states[d].transitions[si] = target;
            }
            break;  /* restart loop to find next unmarked */
        }
    }
}

/* ---- printing helpers --------------------------------------------------- */
static void print_stateset(StateSet s) {
    printf("{");
    int first = 1;
    for (int i = 0; i < nfa_state_count; i++) {
        if (SET_HAS(s, i)) {
            if (!first) printf(", ");
            printf("q%d", i);
            first = 0;
        }
    }
    printf("}");
}

static void print_nfa_table(void) {
    printf("%-10s", "STATE");
    for (int si = 0; si < symbol_count; si++) {
        char label[8];
        if (si == EPS_INDEX) snprintf(label, sizeof(label), "eps");
        else                 snprintf(label, sizeof(label), "%c", symbols[si]);
        printf("| %-16s", label);
    }
    printf("\n");

    int total_w = 10 + symbol_count * 18;
    for (int i = 0; i < total_w; i++) putchar('-');
    printf("\n");

    for (int st = 0; st < nfa_state_count; st++) {
        char prefix[8] = "";
        if (st == nfa_start) strcat(prefix, "->");
        if (SET_HAS(nfa_accept, st)) strcat(prefix, "*");
        printf("%sq%-*d", prefix, (int)(8 - strlen(prefix)), st);

        for (int si = 0; si < symbol_count; si++) {
            StateSet targets = nfa_trans[st][si];
            if (targets == SET_EMPTY) {
                printf("| %-16s", "-");
            } else {
                char buf[64] = "";
                int first = 1;
                for (int t = 0; t < nfa_state_count; t++) {
                    if (SET_HAS(targets, t)) {
                        char tmp[16];
                        snprintf(tmp, sizeof(tmp), "%sq%d", first ? "" : ", ", t);
                        strcat(buf, tmp);
                        first = 0;
                    }
                }
                printf("| %-16s", buf);
            }
        }
        printf("\n");
    }

    for (int i = 0; i < total_w; i++) putchar('-');
    printf("\n");
}

static void print_dfa_table(void) {
    printf("%-8s %-26s", "STATE", "NFA STATES");
    for (int si = 1; si < symbol_count; si++)
        printf("| %-10c", symbols[si]);
    printf("\n");

    int total_w = 8 + 26 + (symbol_count - 1) * 12;
    for (int i = 0; i < total_w; i++) putchar('-');
    printf("\n");

    for (int d = 0; d < dfa_count; d++) {
        char prefix[8] = "";
        if (d == 0) strcat(prefix, "->");
        if (dfa_states[d].is_accept) strcat(prefix, "*");

        char label[16];
        snprintf(label, sizeof(label), "%sD%d", prefix, d);
        printf("%-8s", label);

        /* NFA state set */
        char set_buf[128] = "{";
        int first = 1;
        for (int i = 0; i < nfa_state_count; i++) {
            if (SET_HAS(dfa_states[d].nfa_set, i)) {
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "%sq%d", first ? "" : ", ", i);
                strcat(set_buf, tmp);
                first = 0;
            }
        }
        strcat(set_buf, "}");
        printf("%-26s", set_buf);

        for (int si = 1; si < symbol_count; si++) {
            int target = dfa_states[d].transitions[si];
            if (target >= 0) {
                char tbuf[8];
                snprintf(tbuf, sizeof(tbuf), "D%d", target);
                printf("| %-10s", tbuf);
            } else {
                printf("| %-10s", "-");
            }
        }
        printf("\n");
    }

    for (int i = 0; i < total_w; i++) putchar('-');
    printf("\n");
}

/* ---- build example NFA for (a|b)*abb ------------------------------------ */
static void build_example_nfa(void) {
    nfa_state_count = 11;
    nfa_start = 0;
    nfa_accept = SET_ADD(SET_EMPTY, 10);

    /* initialize transition table */
    memset(nfa_trans, 0, sizeof(nfa_trans));

    /* register epsilon as symbol 0 */
    get_symbol_index('\0');  /* eps stored as '\0' internally */

    /* register input symbols */
    get_symbol_index('a');
    get_symbol_index('b');

    /* epsilon = '\0' internally */
    nfa_add_transition(0, '\0', 1);
    nfa_add_transition(0, '\0', 7);
    nfa_add_transition(1, '\0', 2);
    nfa_add_transition(1, '\0', 4);
    nfa_add_transition(2, 'a', 3);
    nfa_add_transition(3, '\0', 6);
    nfa_add_transition(4, 'b', 5);
    nfa_add_transition(5, '\0', 6);
    nfa_add_transition(6, '\0', 1);
    nfa_add_transition(6, '\0', 7);
    nfa_add_transition(7, 'a', 8);
    nfa_add_transition(8, 'b', 9);
    nfa_add_transition(9, 'b', 10);
}

/* ---- main --------------------------------------------------------------- */
int main(void)
{
    printf("======================================================================\n");
    printf("  NFA -> DFA (Subset Construction)  --  Exercise 3\n");
    printf("======================================================================\n\n");

    printf("Choose input mode:\n");
    printf("  1) Use default NFA for (a|b)*abb\n");
    printf("  2) Enter your own NFA\n");
    printf("\nEnter choice [1]: ");

    char choice_buf[16];
    if (fgets(choice_buf, sizeof(choice_buf), stdin) == NULL)
        choice_buf[0] = '1';

    if (choice_buf[0] == '2') {
        /* user input */
        printf("\nEnter number of NFA states: ");
        scanf("%d", &nfa_state_count);
        printf("Enter start state: ");
        scanf("%d", &nfa_start);

        int num_accept;
        printf("Enter number of accept states: ");
        scanf("%d", &num_accept);
        printf("Enter accept state IDs: ");
        nfa_accept = SET_EMPTY;
        for (int i = 0; i < num_accept; i++) {
            int acc;
            scanf("%d", &acc);
            nfa_accept = SET_ADD(nfa_accept, acc);
        }

        memset(nfa_trans, 0, sizeof(nfa_trans));
        get_symbol_index('\0');  /* epsilon */

        int num_sym;
        printf("Enter number of input symbols: ");
        scanf("%d", &num_sym);
        printf("Enter symbols (one per line):\n");
        for (int i = 0; i < num_sym; i++) {
            char sym;
            scanf(" %c", &sym);
            get_symbol_index(sym);
        }

        int num_trans;
        printf("Enter number of transitions: ");
        scanf("%d", &num_trans);
        printf("Enter transitions (from symbol to), use 'e' for epsilon:\n");
        for (int i = 0; i < num_trans; i++) {
            int from, to;
            char sym;
            scanf("%d %c %d", &from, &sym, &to);
            if (sym == 'e')
                nfa_add_transition(from, '\0', to);
            else
                nfa_add_transition(from, sym, to);
        }
    } else {
        build_example_nfa();
    }

    printf("\n--- NFA Transition Table ---\n\n");
    print_nfa_table();

    /* perform subset construction */
    subset_construction();

    printf("\n--- DFA Transition Table (Subset Construction) ---\n\n");
    print_dfa_table();

    /* summary */
    printf("\nSummary:\n");
    printf("  NFA states: %d\n", nfa_state_count);
    printf("  DFA states: %d\n", dfa_count);
    printf("  DFA start : D0\n");
    printf("  DFA accept: ");
    int first = 1;
    for (int d = 0; d < dfa_count; d++) {
        if (dfa_states[d].is_accept) {
            if (!first) printf(", ");
            printf("D%d", d);
            first = 0;
        }
    }
    printf("\n");

    return 0;
}
