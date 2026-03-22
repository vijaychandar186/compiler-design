/*
================================================================================
Exercise 2: Regular Expression to NFA (Thompson's Construction) -- C
================================================================================

CONCEPT:
    Thompson's construction converts a regular expression into an equivalent
    Non-deterministic Finite Automaton (NFA).  Each NFA fragment has exactly
    one start state and one accept state.  The construction is recursive:
    small NFAs for individual characters are composed using concatenation,
    union, and Kleene-star operations.

ALGORITHM:
    1. Insert explicit concatenation operators ('.') where implied.
    2. Convert the augmented infix regex to postfix using the shunting-yard
       algorithm with precedence:  * (3)  >  . (2)  >  | (1).
    3. Evaluate the postfix expression with a stack of NFA fragments:
       - Literal 'c'  : push a two-state fragment  start --c--> accept.
       - '.'          : pop two, link first.accept --eps--> second.start.
       - '|'          : pop two, new start with eps to both starts,
                        both accepts eps to new accept.
       - '*'          : pop one, add eps loop and bypass.
    4. The remaining fragment on the stack is the complete NFA.

COMPLEXITY:
    Time:  O(n) -- linear in the length of the regex.
    Space: O(n) -- at most 2*n states.

EXAMPLE:
    Regex: (a|b)*abb   ->  NFA with ~11 states.
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- constants ---------------------------------------------------------- */
#define MAX_STATES      128
#define MAX_TRANSITIONS 4     /* per state per symbol */
#define MAX_SYMBOLS     32
#define MAX_REGEX_LEN   256
#define EPSILON         '\0'  /* we represent epsilon internally as '\0' */

/* ---- state -------------------------------------------------------------- */
typedef struct {
    int   targets[MAX_SYMBOLS][MAX_TRANSITIONS];  /* targets[sym_index][i] */
    int   target_count[MAX_SYMBOLS];
} StateRow;

/* ---- global NFA storage ------------------------------------------------- */
static StateRow nfa_table[MAX_STATES];
static int      state_count = 0;

/* symbol map: index 0 = epsilon, 1.. = input characters */
static char     sym_chars[MAX_SYMBOLS];
static int      sym_count = 0;

static int get_sym_index(char ch) {
    for (int i = 0; i < sym_count; i++)
        if (sym_chars[i] == ch) return i;
    sym_chars[sym_count] = ch;
    return sym_count++;
}

static int new_state(void) {
    int id = state_count++;
    for (int s = 0; s < MAX_SYMBOLS; s++)
        nfa_table[id].target_count[s] = 0;
    return id;
}

static void add_transition(int from, char symbol, int to) {
    int si = get_sym_index(symbol);
    int *cnt = &nfa_table[from].target_count[si];
    if (*cnt < MAX_TRANSITIONS)
        nfa_table[from].targets[si][(*cnt)++] = to;
}

/* ---- NFA fragment ------------------------------------------------------- */
typedef struct {
    int start;
    int accept;
} Fragment;

/* ---- fragment stack ----------------------------------------------------- */
static Fragment frag_stack[MAX_STATES];
static int      frag_top = -1;

static void frag_push(Fragment f)  { frag_stack[++frag_top] = f; }
static Fragment frag_pop(void)     { return frag_stack[frag_top--]; }

/* ---- Thompson primitives ------------------------------------------------ */
static Fragment nfa_for_char(char ch) {
    int s = new_state();
    int a = new_state();
    add_transition(s, ch, a);
    return (Fragment){s, a};
}

static Fragment nfa_concat(Fragment first, Fragment second) {
    add_transition(first.accept, EPSILON, second.start);
    return (Fragment){first.start, second.accept};
}

static Fragment nfa_union(Fragment upper, Fragment lower) {
    int s = new_state();
    int a = new_state();
    add_transition(s, EPSILON, upper.start);
    add_transition(s, EPSILON, lower.start);
    add_transition(upper.accept, EPSILON, a);
    add_transition(lower.accept, EPSILON, a);
    return (Fragment){s, a};
}

static Fragment nfa_star(Fragment inner) {
    int s = new_state();
    int a = new_state();
    add_transition(s, EPSILON, inner.start);
    add_transition(s, EPSILON, a);
    add_transition(inner.accept, EPSILON, inner.start);
    add_transition(inner.accept, EPSILON, a);
    return (Fragment){s, a};
}

/* ---- insert explicit concatenation '.' ---------------------------------- */
static void insert_concat(const char *regex, char *out) {
    int len = (int)strlen(regex);
    int oi = 0;
    for (int i = 0; i < len; i++) {
        out[oi++] = regex[i];
        if (i + 1 < len) {
            char curr = regex[i];
            char next = regex[i + 1];
            if (curr != '(' && curr != '|' && curr != '.' &&
                next != ')' && next != '|' && next != '*' && next != '.') {
                out[oi++] = '.';
            }
        }
    }
    out[oi] = '\0';
}

/* ---- shunting-yard: infix -> postfix ------------------------------------ */
static int precedence(char op) {
    if (op == '*') return 3;
    if (op == '.') return 2;
    if (op == '|') return 1;
    return 0;
}

static void to_postfix(const char *infix, char *postfix) {
    char stack[MAX_REGEX_LEN];
    int  sp = -1;
    int  oi = 0;
    int  len = (int)strlen(infix);

    for (int i = 0; i < len; i++) {
        char ch = infix[i];
        if (ch == '(') {
            stack[++sp] = ch;
        } else if (ch == ')') {
            while (sp >= 0 && stack[sp] != '(')
                postfix[oi++] = stack[sp--];
            if (sp >= 0) sp--;  /* pop '(' */
        } else if (ch == '|' || ch == '.' || ch == '*') {
            while (sp >= 0 && stack[sp] != '(' &&
                   precedence(stack[sp]) >= precedence(ch))
                postfix[oi++] = stack[sp--];
            stack[++sp] = ch;
        } else {
            postfix[oi++] = ch;  /* literal */
        }
    }
    while (sp >= 0) postfix[oi++] = stack[sp--];
    postfix[oi] = '\0';
}

/* ---- build NFA from postfix --------------------------------------------- */
static Fragment build_nfa(const char *postfix) {
    frag_top = -1;
    int len = (int)strlen(postfix);
    for (int i = 0; i < len; i++) {
        char ch = postfix[i];
        if (ch == '.') {
            Fragment second = frag_pop();
            Fragment first  = frag_pop();
            frag_push(nfa_concat(first, second));
        } else if (ch == '|') {
            Fragment lower = frag_pop();
            Fragment upper = frag_pop();
            frag_push(nfa_union(upper, lower));
        } else if (ch == '*') {
            Fragment inner = frag_pop();
            frag_push(nfa_star(inner));
        } else {
            frag_push(nfa_for_char(ch));
        }
    }
    return frag_pop();
}

/* ---- print NFA ---------------------------------------------------------- */
static void print_nfa(Fragment nfa_frag) {
    /* build ordered symbol list: epsilon first, then alphabetical */
    int eps_idx = -1;
    for (int i = 0; i < sym_count; i++)
        if (sym_chars[i] == EPSILON) { eps_idx = i; break; }

    int ordered[MAX_SYMBOLS];
    int oc = 0;
    if (eps_idx >= 0) ordered[oc++] = eps_idx;
    for (int i = 0; i < sym_count; i++)
        if (i != eps_idx) ordered[oc++] = i;

    /* header */
    printf("%-10s", "STATE");
    for (int i = 0; i < oc; i++) {
        char label[8];
        if (sym_chars[ordered[i]] == EPSILON)
            snprintf(label, sizeof(label), "eps");
        else
            snprintf(label, sizeof(label), "%c", sym_chars[ordered[i]]);
        printf("| %-14s", label);
    }
    printf("\n");

    /* separator */
    for (int i = 0; i < 10 + oc * 16; i++) putchar('-');
    printf("\n");

    /* rows */
    for (int st = 0; st < state_count; st++) {
        char prefix[16] = "";
        if (st == nfa_frag.start)  strcat(prefix, "->");
        if (st == nfa_frag.accept) strcat(prefix, "*");
        char label[24];
        snprintf(label, sizeof(label), "%sq%d", prefix, st);
        printf("%-10s", label);

        for (int i = 0; i < oc; i++) {
            int si = ordered[i];
            int cnt = nfa_table[st].target_count[si];
            if (cnt == 0) {
                printf("| %-14s", "-");
            } else {
                char buf[64] = "";
                for (int k = 0; k < cnt; k++) {
                    char tmp[16];
                    snprintf(tmp, sizeof(tmp), "%sq%d", k > 0 ? ", " : "",
                             nfa_table[st].targets[si][k]);
                    strcat(buf, tmp);
                }
                printf("| %-14s", buf);
            }
        }
        printf("\n");
    }

    for (int i = 0; i < 10 + oc * 16; i++) putchar('-');
    printf("\n");

    printf("\nStart state : q%d\n", nfa_frag.start);
    printf("Accept state: q%d\n", nfa_frag.accept);
    printf("Total states: %d\n", state_count);
}

/* ---- main --------------------------------------------------------------- */
int main(void)
{
    printf("======================================================================\n");
    printf("  RE -> NFA (Thompson's Construction)  --  Exercise 2\n");
    printf("======================================================================\n\n");

    const char *default_regex = "(a|b)*abb";

    printf("Default regex: %s\n", default_regex);
    printf("Enter a regex (or press Enter for default): ");

    char input_buf[MAX_REGEX_LEN];
    if (fgets(input_buf, sizeof(input_buf), stdin) == NULL)
        input_buf[0] = '\n';
    /* strip newline */
    input_buf[strcspn(input_buf, "\n")] = '\0';

    const char *regex = (strlen(input_buf) > 0) ? input_buf : default_regex;

    /* initialize epsilon symbol */
    get_sym_index(EPSILON);

    char with_concat[MAX_REGEX_LEN];
    insert_concat(regex, with_concat);

    char postfix[MAX_REGEX_LEN];
    to_postfix(with_concat, postfix);

    printf("\n1. Original regex      : %s\n", regex);
    printf("2. With explicit concat: %s\n", with_concat);
    printf("3. Postfix form        : %s\n", postfix);

    Fragment result = build_nfa(postfix);

    printf("\n--- NFA Transition Table ---\n\n");
    print_nfa(result);

    return 0;
}
