/*
================================================================================
Exercise 4a: Left Factoring of Context-Free Grammars -- C Implementation
================================================================================

CONCEPT:
    Left factoring is a grammar transformation that eliminates common prefixes
    among the right-hand sides of productions for the same non-terminal.  This
    makes the grammar suitable for predictive (top-down) parsing.

    Given:   A -> alpha beta1 | alpha beta2
    Result:  A  -> alpha A'
             A' -> beta1 | beta2

ALGORITHM:
    For each non-terminal:
      1. Find the longest common prefix shared by at least two productions.
      2. If found, create A' with the different suffixes; replace the originals
         with "prefix A'".
      3. Repeat until no common prefixes remain.

COMPLEXITY:
    Time:  O(P * L) per non-terminal per round, where P = number of productions
           and L = max production length.
    Space: O(G) for the grammar.

EXAMPLE:
    Input:   S -> a A B | a A c | b B c
             A -> d A | e
    Output:  S  -> a A S' | b B c
             S' -> B | c
             A  -> d A | e
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- constants ---------------------------------------------------------- */
#define MAX_NONTERMS   26
#define MAX_PRODS      16     /* max productions per non-terminal */
#define MAX_SYMBOLS    16     /* max symbols per production */
#define MAX_SYM_LEN   16     /* max length of a single symbol string */

/* ---- grammar structures ------------------------------------------------- */
typedef struct {
    char symbols[MAX_SYMBOLS][MAX_SYM_LEN];
    int  length;            /* number of symbols in this production */
} Production;

typedef struct {
    char        name[MAX_SYM_LEN];
    Production  prods[MAX_PRODS];
    int         prod_count;
} NonTerminal;

typedef struct {
    NonTerminal nts[MAX_NONTERMS];
    int         nt_count;
} Grammar;

/* ---- find non-terminal by name ------------------------------------------ */
static int find_nt(const Grammar *g, const char *name) {
    for (int i = 0; i < g->nt_count; i++)
        if (strcmp(g->nts[i].name, name) == 0) return i;
    return -1;
}

/* ---- add a new non-terminal --------------------------------------------- */
static int add_nt(Grammar *g, const char *name) {
    int idx = g->nt_count++;
    strncpy(g->nts[idx].name, name, MAX_SYM_LEN - 1);
    g->nts[idx].prod_count = 0;
    return idx;
}

/* ---- add a production to a non-terminal --------------------------------- */
static void add_prod(NonTerminal *nt, const Production *p) {
    if (nt->prod_count < MAX_PRODS) {
        nt->prods[nt->prod_count] = *p;
        nt->prod_count++;
    }
}

/* ---- generate new non-terminal name (A -> A' -> A'' etc.) --------------- */
static void new_nt_name(const Grammar *g, const char *base, char *out) {
    strcpy(out, base);
    strcat(out, "'");
    while (find_nt(g, out) >= 0)
        strcat(out, "'");
}

/* ---- find longest common prefix among productions ----------------------- */
static int longest_common_prefix(const NonTerminal *nt, int *group_indices, int *group_size) {
    /* Try increasing prefix lengths, find the longest shared by >= 2 prods */
    int best_len = 0;

    for (int length = 1; length <= MAX_SYMBOLS; length++) {
        int found = 0;
        for (int i = 0; i < nt->prod_count && !found; i++) {
            if (nt->prods[i].length < length) continue;
            int count = 0;
            int indices[MAX_PRODS];
            indices[count++] = i;

            for (int j = i + 1; j < nt->prod_count; j++) {
                if (nt->prods[j].length < length) continue;
                int match = 1;
                for (int k = 0; k < length; k++) {
                    if (strcmp(nt->prods[i].symbols[k], nt->prods[j].symbols[k]) != 0) {
                        match = 0; break;
                    }
                }
                if (match) indices[count++] = j;
            }

            if (count >= 2) {
                best_len = length;
                *group_size = count;
                for (int m = 0; m < count; m++)
                    group_indices[m] = indices[m];
                found = 1;
            }
        }
        if (!found) break;
    }

    return best_len;
}

/* ---- one round of left factoring ---------------------------------------- */
static int left_factor_once(Grammar *g) {
    int changed = 0;

    for (int ni = 0; ni < g->nt_count; ni++) {
        NonTerminal *nt = &g->nts[ni];
        int group_indices[MAX_PRODS];
        int group_size = 0;

        int prefix_len = longest_common_prefix(nt, group_indices, &group_size);
        if (prefix_len == 0) continue;

        changed = 1;

        /* create new non-terminal */
        char new_name[MAX_SYM_LEN];
        new_nt_name(g, nt->name, new_name);
        int new_ni = add_nt(g, new_name);
        NonTerminal *new_nt = &g->nts[new_ni];

        /* build the factored production: prefix + new_nt */
        Production factored;
        factored.length = prefix_len + 1;
        for (int k = 0; k < prefix_len; k++)
            strcpy(factored.symbols[k], nt->prods[group_indices[0]].symbols[k]);
        strcpy(factored.symbols[prefix_len], new_name);

        /* build suffix productions for the new non-terminal */
        for (int m = 0; m < group_size; m++) {
            int pi = group_indices[m];
            Production suffix;
            int slen = nt->prods[pi].length - prefix_len;
            if (slen <= 0) {
                suffix.length = 1;
                strcpy(suffix.symbols[0], "eps");
            } else {
                suffix.length = slen;
                for (int k = 0; k < slen; k++)
                    strcpy(suffix.symbols[k], nt->prods[pi].symbols[prefix_len + k]);
            }
            add_prod(new_nt, &suffix);
        }

        /* rebuild the original non-terminal's productions:
           keep non-matching ones + add the factored one */
        Production kept[MAX_PRODS];
        int kept_count = 0;

        /* mark which indices were in the group */
        int in_group[MAX_PRODS] = {0};
        for (int m = 0; m < group_size; m++)
            in_group[group_indices[m]] = 1;

        /* add the factored production first */
        kept[kept_count++] = factored;

        /* then the non-matching ones */
        for (int pi = 0; pi < nt->prod_count; pi++) {
            if (!in_group[pi])
                kept[kept_count++] = nt->prods[pi];
        }

        /* replace */
        nt->prod_count = kept_count;
        for (int i = 0; i < kept_count; i++)
            nt->prods[i] = kept[i];

        break;  /* restart outer loop after one change */
    }

    return changed;
}

/* ---- left factor the whole grammar -------------------------------------- */
static void left_factor(Grammar *g) {
    int iteration = 0;
    while (left_factor_once(g)) {
        iteration++;
        if (iteration > 50) {
            printf("  [Warning: exceeded 50 iterations]\n");
            break;
        }
    }
}

/* ---- print grammar ------------------------------------------------------ */
static void print_grammar(const Grammar *g) {
    for (int i = 0; i < g->nt_count; i++) {
        const NonTerminal *nt = &g->nts[i];
        printf("  %s ->", nt->name);
        for (int p = 0; p < nt->prod_count; p++) {
            if (p > 0) printf(" |");
            for (int s = 0; s < nt->prods[p].length; s++)
                printf(" %s", nt->prods[p].symbols[s]);
        }
        printf("\n");
    }
}

/* ---- parse grammar from string ------------------------------------------ */
static void parse_line(Grammar *g, const char *line) {
    char buf[256];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* split on -> */
    char *arrow = strstr(buf, "->");
    if (!arrow) return;
    *arrow = '\0';
    char *lhs = buf;
    char *rhs = arrow + 2;

    /* trim lhs */
    while (*lhs == ' ') lhs++;
    char nt_name[MAX_SYM_LEN];
    sscanf(lhs, "%s", nt_name);

    int ni = find_nt(g, nt_name);
    if (ni < 0) ni = add_nt(g, nt_name);

    /* split rhs on | */
    char *alt = strtok(rhs, "|");
    while (alt) {
        Production prod;
        prod.length = 0;
        char *tok = strtok(NULL, ""); /* save rest */
        char *rest = tok;

        /* parse symbols from alt */
        char alt_copy[256];
        strncpy(alt_copy, alt, sizeof(alt_copy) - 1);
        alt_copy[sizeof(alt_copy) - 1] = '\0';
        char *sym = strtok(alt_copy, " \t");
        while (sym) {
            strncpy(prod.symbols[prod.length], sym, MAX_SYM_LEN - 1);
            prod.length++;
            sym = strtok(NULL, " \t");
        }
        if (prod.length > 0)
            add_prod(&g->nts[ni], &prod);

        /* continue with rest of rhs */
        if (rest) {
            alt = strtok(rest, "|");
        } else {
            alt = NULL;
        }
    }
}

/* a simpler parser that handles the grammar correctly */
static void parse_grammar_simple(Grammar *g, const char *text) {
    g->nt_count = 0;
    const char *p = text;
    char line[256];

    while (*p) {
        /* read one line */
        int li = 0;
        while (*p && *p != '\n' && li < 255) {
            line[li++] = *p++;
        }
        line[li] = '\0';
        if (*p == '\n') p++;

        /* skip empty lines */
        int empty = 1;
        for (int i = 0; i < li; i++)
            if (line[i] != ' ' && line[i] != '\t') { empty = 0; break; }
        if (empty) continue;

        /* find -> */
        char *arrow = strstr(line, "->");
        if (!arrow) continue;

        /* extract LHS */
        char lhs[MAX_SYM_LEN];
        *arrow = '\0';
        sscanf(line, "%s", lhs);
        char *rhs = arrow + 2;

        int ni = find_nt(g, lhs);
        if (ni < 0) ni = add_nt(g, lhs);

        /* split RHS by | and parse each alternative */
        char rhs_copy[256];
        strncpy(rhs_copy, rhs, sizeof(rhs_copy) - 1);
        rhs_copy[sizeof(rhs_copy) - 1] = '\0';

        /* manual split by '|' */
        char *start = rhs_copy;
        while (1) {
            char *bar = strchr(start, '|');
            if (bar) *bar = '\0';

            /* parse symbols from this alternative */
            Production prod;
            prod.length = 0;
            char alt_buf[256];
            strncpy(alt_buf, start, sizeof(alt_buf) - 1);
            alt_buf[sizeof(alt_buf) - 1] = '\0';

            char *tok = strtok(alt_buf, " \t");
            while (tok) {
                strncpy(prod.symbols[prod.length], tok, MAX_SYM_LEN - 1);
                prod.symbols[prod.length][MAX_SYM_LEN - 1] = '\0';
                prod.length++;
                tok = strtok(NULL, " \t");
            }
            if (prod.length > 0)
                add_prod(&g->nts[ni], &prod);

            if (bar) start = bar + 1;
            else break;
        }
    }
}

/* ---- main --------------------------------------------------------------- */
int main(void)
{
    printf("======================================================================\n");
    printf("  Left Factoring of Context-Free Grammars  --  Exercise 4a\n");
    printf("======================================================================\n\n");

    printf("Choose input mode:\n");
    printf("  1) Example: S -> a A B | a A c | b B c,  A -> d A | e\n");
    printf("  2) Example: E -> T + E | T,  T -> int | int * T | ( E )\n");
    printf("  3) Enter your own grammar\n");
    printf("\nEnter choice [1]: ");

    char choice_buf[16];
    if (fgets(choice_buf, sizeof(choice_buf), stdin) == NULL)
        choice_buf[0] = '1';

    Grammar grammar;
    grammar.nt_count = 0;

    if (choice_buf[0] == '2') {
        const char *text =
            "E -> T + E | T\n"
            "T -> int | int * T | ( E )\n";
        parse_grammar_simple(&grammar, text);
    } else if (choice_buf[0] == '3') {
        printf("\nEnter grammar (one rule per line, end with empty line):\n");
        char all_text[2048] = "";
        char line_buf[256];
        while (fgets(line_buf, sizeof(line_buf), stdin)) {
            if (line_buf[0] == '\n') break;
            strncat(all_text, line_buf, sizeof(all_text) - strlen(all_text) - 1);
        }
        parse_grammar_simple(&grammar, all_text);
    } else {
        const char *text =
            "S -> a A B | a A c | b B c\n"
            "A -> d A | e\n";
        parse_grammar_simple(&grammar, text);
    }

    printf("\n--- Original Grammar ---\n");
    print_grammar(&grammar);

    left_factor(&grammar);

    printf("\n--- Left-Factored Grammar ---\n");
    print_grammar(&grammar);

    return 0;
}
