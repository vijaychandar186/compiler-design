/*
 * ===========================================================================
 * Exercise 12: Simple Code Generator (Target Code from Three-Address Code)
 *              -- C Implementation
 * ===========================================================================
 *
 * CONCEPT:
 *   A code generator translates intermediate representation (three-address
 *   code) into target machine instructions.  This implementation produces
 *   assembly-like instructions (MOV, ADD, SUB, MUL, DIV) using a small pool
 *   of registers (R0-R7).
 *
 * ALGORITHM:
 *   For each TAC statement  result = left op right:
 *     1. If `left` is already in a register, reuse it; otherwise load it
 *        with MOV into a free register.
 *     2. Emit the operation instruction (e.g. ADD right, Ri).
 *     3. Update the register descriptor so Ri holds `result`.
 *     4. If `result` is a user variable (not a temporary), store it back
 *        with MOV Ri, result.
 *   A register descriptor and an address descriptor avoid redundant loads.
 *
 * TIME COMPLEXITY:  O(n) where n = number of TAC instructions.
 * SPACE COMPLEXITY: O(r + v) for registers and variable descriptors.
 *
 * EXAMPLE:
 *   TAC:  t1 = a + b       ->   MOV a, R0  /  ADD b, R0
 *         t2 = t1 * c      ->   MUL c, R0
 *         t3 = d - e       ->   MOV d, R1  /  SUB e, R1
 *         x  = t2 + t3     ->   ADD R1, R0 /  MOV R0, x
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- constants ----------------------------------------------------- */
#define MAX_NAME     64
#define MAX_REGS      8
#define MAX_VARS    128
#define MAX_ASM     256
#define MAX_TAC      64

/* ---------- register & variable descriptors ------------------------------- */

static char reg_contents[MAX_REGS][MAX_NAME];   /* what variable is in Ri   */
static char var_location[MAX_VARS][MAX_NAME];    /* variable name            */
static int  var_reg[MAX_VARS];                   /* register index, -1 = none */
static int  var_count = 0;

/* ---------- assembly output buffer ---------------------------------------- */

static char asm_lines[MAX_ASM][128];
static int  asm_count = 0;

/* ---------- helper: map operator to mnemonic ----------------------------- */

static const char *op_to_mnemonic(char op)
{
    switch (op) {
        case '+': return "ADD";
        case '-': return "SUB";
        case '*': return "MUL";
        case '/': return "DIV";
        default:  return "???";
    }
}

/* ---------- descriptor functions ----------------------------------------- */

static void reset_descriptors(void)
{
    for (int i = 0; i < MAX_REGS; i++)
        reg_contents[i][0] = '\0';
    var_count = 0;
    asm_count = 0;
}

/* Find the variable index, creating a new entry if needed. */
static int find_or_add_var(const char *name)
{
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_location[i], name) == 0)
            return i;
    /* New entry */
    int idx = var_count++;
    strncpy(var_location[idx], name, MAX_NAME - 1);
    var_location[idx][MAX_NAME - 1] = '\0';
    var_reg[idx] = -1;
    return idx;
}

/* Return the register index holding `name`, or -1. */
static int get_var_reg(const char *name)
{
    int vi = find_or_add_var(name);
    return var_reg[vi];
}

/* Assign register `ri` to variable `name`, clearing previous mappings. */
static void assign_reg(int ri, const char *name)
{
    /* Clear old variable in this register */
    if (reg_contents[ri][0] != '\0') {
        int old_vi = find_or_add_var(reg_contents[ri]);
        if (var_reg[old_vi] == ri)
            var_reg[old_vi] = -1;
    }
    /* Clear old register for this variable */
    int vi = find_or_add_var(name);
    if (var_reg[vi] >= 0)
        reg_contents[var_reg[vi]][0] = '\0';

    strncpy(reg_contents[ri], name, MAX_NAME - 1);
    reg_contents[ri][MAX_NAME - 1] = '\0';
    var_reg[vi] = ri;
}

/* Allocate a free register, or evict R0 if none free. */
static int allocate_reg(void)
{
    for (int i = 0; i < MAX_REGS; i++)
        if (reg_contents[i][0] == '\0')
            return i;
    /* Evict R0 */
    if (reg_contents[0][0] != '\0') {
        int vi = find_or_add_var(reg_contents[0]);
        var_reg[vi] = -1;
        reg_contents[0][0] = '\0';
    }
    return 0;
}

/* ---------- emit assembly ------------------------------------------------ */

static void emit_asm(const char *line)
{
    if (asm_count < MAX_ASM) {
        strncpy(asm_lines[asm_count], line, 127);
        asm_lines[asm_count][127] = '\0';
        asm_count++;
    }
}

/* ---------- check if name is a temporary (t followed by digits) ---------- */

static int is_temporary(const char *name)
{
    if (name[0] != 't') return 0;
    for (int i = 1; name[i]; i++)
        if (!isdigit((unsigned char)name[i]))
            return 0;
    return (name[1] != '\0');  /* must have at least one digit */
}

/* ---------- operand name: register name if loaded, else variable name ---- */

static void operand_name(const char *name, char *out, int out_size)
{
    int ri = get_var_reg(name);
    if (ri >= 0)
        snprintf(out, out_size, "R%d", ri);
    else
        snprintf(out, out_size, "%s", name);
}

/* ---------- TAC statement ------------------------------------------------ */

typedef struct {
    char result[MAX_NAME];
    char left[MAX_NAME];
    char op;                 /* '+', '-', '*', '/', or '\0' for copy */
    char right[MAX_NAME];
} TACStmt;

/* Parse a TAC string like "t1 = a + b" or "x = t1". */
static TACStmt parse_tac(const char *line)
{
    TACStmt stmt;
    memset(&stmt, 0, sizeof(stmt));
    char buf[4][MAX_NAME];
    int fields = sscanf(line, "%63s = %63s %c %63s", buf[0], buf[1], &stmt.op, buf[2]);
    strncpy(stmt.result, buf[0], MAX_NAME - 1);
    strncpy(stmt.left, buf[1], MAX_NAME - 1);
    if (fields < 4) {
        stmt.op = '\0';
        stmt.right[0] = '\0';
    } else {
        strncpy(stmt.right, buf[2], MAX_NAME - 1);
    }
    return stmt;
}

/* ---------- code generation for one TAC statement ------------------------ */

static void generate_one(const TACStmt *stmt)
{
    char buf[128];
    char rname[16];

    if (stmt->op == '\0') {
        /* Simple copy: result = source */
        int src_ri = get_var_reg(stmt->left);
        int ri;
        if (src_ri >= 0) {
            ri = src_ri;
        } else {
            ri = allocate_reg();
            snprintf(buf, sizeof(buf), "MOV %s, R%d", stmt->left, ri);
            emit_asm(buf);
        }
        assign_reg(ri, stmt->result);
        if (!is_temporary(stmt->result)) {
            snprintf(buf, sizeof(buf), "MOV R%d, %s", ri, stmt->result);
            emit_asm(buf);
        }
    } else {
        /* Binary: result = left op right */
        int left_ri = get_var_reg(stmt->left);
        int ri;
        if (left_ri >= 0) {
            ri = left_ri;
        } else {
            ri = allocate_reg();
            snprintf(buf, sizeof(buf), "MOV %s, R%d", stmt->left, ri);
            emit_asm(buf);
        }
        operand_name(stmt->right, rname, sizeof(rname));
        snprintf(buf, sizeof(buf), "%s %s, R%d",
                 op_to_mnemonic(stmt->op), rname, ri);
        emit_asm(buf);
        assign_reg(ri, stmt->result);
        if (!is_temporary(stmt->result)) {
            snprintf(buf, sizeof(buf), "MOV R%d, %s", ri, stmt->result);
            emit_asm(buf);
        }
    }
}

/* ---------- driver ------------------------------------------------------- */

static void generate_and_print(const char *description,
                                const char **tac_lines, int tac_n)
{
    reset_descriptors();

    printf("  Input Three-Address Code:\n");
    for (int i = 0; i < tac_n; i++)
        printf("    %s\n", tac_lines[i]);
    printf("\n");

    for (int i = 0; i < tac_n; i++) {
        TACStmt stmt = parse_tac(tac_lines[i]);
        generate_one(&stmt);
    }

    printf("  Generated Assembly:\n");
    for (int i = 0; i < asm_count; i++)
        printf("    %3d:  %s\n", i + 1, asm_lines[i]);
    printf("\n");
}

int main(void)
{
    printf("=================================================================\n");
    printf("   SIMPLE CODE GENERATOR  (Target Code from TAC)  --  C\n");
    printf("=================================================================\n\n");

    /* Test case 1 */
    const char *tac1[] = {
        "t1 = a + b",
        "t2 = t1 * c",
        "t3 = d - e",
        "x = t2 + t3"
    };
    printf("  --- Test Case 1 ---\n");
    generate_and_print("x = a+b*c - d-e", tac1, 4);

    /* Test case 2 */
    const char *tac2[] = {
        "t1 = a + b",
        "t2 = c - d",
        "t3 = t1 * t2",
        "y = t3"
    };
    printf("  --- Test Case 2 ---\n");
    generate_and_print("y = (a+b)*(c-d)", tac2, 4);

    /* Test case 3 */
    const char *tac3[] = {
        "t1 = a * b",
        "t2 = c * d",
        "t3 = t1 + t2",
        "t4 = e * f",
        "z = t3 + t4"
    };
    printf("  --- Test Case 3 ---\n");
    generate_and_print("z = a*b + c*d + e*f", tac3, 5);

    printf("=================================================================\n");
    printf("  All test cases processed successfully.\n");
    printf("=================================================================\n");
    return 0;
}
