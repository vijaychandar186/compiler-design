/*
 * ===========================================================================
 * Exercise 12: Simple Code Generator (Target Code from Three-Address Code)
 *              -- C++ Implementation
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
 *   Register and address descriptors eliminate redundant MOV instructions.
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

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <cctype>

static const int NUM_REGISTERS = 8;

/* ---------- helper: operator -> mnemonic --------------------------------- */

static std::string op_to_mnemonic(const std::string &op) {
    if (op == "+") return "ADD";
    if (op == "-") return "SUB";
    if (op == "*") return "MUL";
    if (op == "/") return "DIV";
    return "???";
}

/* ---------- check if a name is a compiler temporary ---------------------- */

static bool is_temporary(const std::string &name) {
    if (name.size() < 2 || name[0] != 't') return false;
    for (size_t i = 1; i < name.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return false;
    return true;
}

/* ---------- TAC statement ------------------------------------------------ */

struct TACStmt {
    std::string result;
    std::string left;
    std::string op;     /* "+", "-", "*", "/", or "" for simple copy */
    std::string right;
};

static TACStmt parse_tac(const std::string &line) {
    TACStmt stmt;
    std::istringstream iss(line);
    std::string eq;
    iss >> stmt.result >> eq >> stmt.left;
    if (iss >> stmt.op >> stmt.right) {
        /* binary operation */
    } else {
        stmt.op.clear();
        stmt.right.clear();
    }
    return stmt;
}

/* ---------- Code Generator ----------------------------------------------- */

class CodeGenerator {
public:
    CodeGenerator() : reg_contents_(NUM_REGISTERS) {}

    std::vector<std::string> generate(const std::vector<std::string> &tac_lines) {
        reset();
        for (const auto &line : tac_lines) {
            TACStmt stmt = parse_tac(line);
            generate_one(stmt);
        }
        return assembly_;
    }

private:
    /* Register descriptor: reg_contents_[i] = variable name or "" */
    std::vector<std::string> reg_contents_;
    /* Address descriptor: variable -> register index (-1 = not loaded) */
    std::unordered_map<std::string, int> var_reg_;
    /* Output buffer */
    std::vector<std::string> assembly_;

    void reset() {
        for (auto &rc : reg_contents_) rc.clear();
        var_reg_.clear();
        assembly_.clear();
    }

    void emit(const std::string &instr) {
        assembly_.push_back(instr);
    }

    int find_var_reg(const std::string &name) const {
        auto it = var_reg_.find(name);
        return (it != var_reg_.end()) ? it->second : -1;
    }

    std::string reg_name(int idx) const {
        return "R" + std::to_string(idx);
    }

    /* Get the best operand name: register if loaded, else the variable name */
    std::string operand(const std::string &name) const {
        int ri = find_var_reg(name);
        return (ri >= 0) ? reg_name(ri) : name;
    }

    int allocate() {
        for (int i = 0; i < NUM_REGISTERS; ++i)
            if (reg_contents_[i].empty()) return i;
        /* Evict R0 */
        evict(0);
        return 0;
    }

    void evict(int ri) {
        if (!reg_contents_[ri].empty()) {
            auto it = var_reg_.find(reg_contents_[ri]);
            if (it != var_reg_.end() && it->second == ri)
                var_reg_.erase(it);
            reg_contents_[ri].clear();
        }
    }

    void assign(int ri, const std::string &name) {
        /* Clear old register for this variable */
        int old_ri = find_var_reg(name);
        if (old_ri >= 0 && old_ri != ri)
            reg_contents_[old_ri].clear();
        /* Clear old variable for this register */
        evict(ri);
        reg_contents_[ri] = name;
        var_reg_[name] = ri;
    }

    void generate_one(const TACStmt &stmt) {
        if (stmt.op.empty()) {
            /* Simple copy: result = source */
            int src_ri = find_var_reg(stmt.left);
            int ri;
            if (src_ri >= 0) {
                ri = src_ri;
            } else {
                ri = allocate();
                emit("MOV " + stmt.left + ", " + reg_name(ri));
            }
            assign(ri, stmt.result);
            if (!is_temporary(stmt.result))
                emit("MOV " + reg_name(ri) + ", " + stmt.result);
        } else {
            /* Binary: result = left op right */
            int left_ri = find_var_reg(stmt.left);
            int ri;
            if (left_ri >= 0) {
                ri = left_ri;
            } else {
                ri = allocate();
                emit("MOV " + stmt.left + ", " + reg_name(ri));
            }
            emit(op_to_mnemonic(stmt.op) + " " + operand(stmt.right) +
                 ", " + reg_name(ri));
            assign(ri, stmt.result);
            if (!is_temporary(stmt.result))
                emit("MOV " + reg_name(ri) + ", " + stmt.result);
        }
    }
};

/* ---------- print helper ------------------------------------------------- */

static void print_assembly(const std::vector<std::string> &asm_lines) {
    for (size_t i = 0; i < asm_lines.size(); ++i)
        std::cout << "    " << (i + 1) << ":  " << asm_lines[i] << "\n";
}

/* ---------- main --------------------------------------------------------- */

int main() {
    std::cout << "=================================================================\n";
    std::cout << "   SIMPLE CODE GENERATOR  (Target Code from TAC)  --  C++\n";
    std::cout << "=================================================================\n\n";

    struct TestCase {
        std::string              description;
        std::vector<std::string> tac;
    };

    std::vector<TestCase> tests = {
        {"x = a + b * c - d / e",
         {"t1 = a + b", "t2 = t1 * c", "t3 = d - e", "x = t2 + t3"}},
        {"y = (a + b) * (c - d)",
         {"t1 = a + b", "t2 = c - d", "t3 = t1 * t2", "y = t3"}},
        {"z = a*b + c*d + e*f",
         {"t1 = a * b", "t2 = c * d", "t3 = t1 + t2",
          "t4 = e * f", "z = t3 + t4"}},
    };

    CodeGenerator generator;

    for (size_t i = 0; i < tests.size(); ++i) {
        std::cout << "  --- Test Case " << (i + 1) << ": "
                  << tests[i].description << " ---\n";
        std::cout << "  Input Three-Address Code:\n";
        for (const auto &line : tests[i].tac)
            std::cout << "    " << line << "\n";
        std::cout << "\n";

        auto asm_out = generator.generate(tests[i].tac);
        std::cout << "  Generated Assembly:\n";
        print_assembly(asm_out);
        std::cout << "\n";
    }

    std::cout << "=================================================================\n";
    std::cout << "  All test cases processed successfully.\n";
    std::cout << "=================================================================\n";
    return 0;
}
