/*
 * ===========================================================================
 * Exercise 11: Three-Address Code (TAC) Generation  --  C++ Implementation
 * ===========================================================================
 *
 * CONCEPT:
 *   Three-Address Code is an intermediate representation used in compilers
 *   where each instruction has at most three operands (two sources and one
 *   destination).  Complex expressions are decomposed into a sequence of
 *   simple instructions, each involving at most one arithmetic operator.
 *   Temporary variables (t1, t2, ...) hold intermediate results.
 *
 * ALGORITHM:
 *   A recursive descent parser walks the token stream.  Operator precedence
 *   is encoded directly in the grammar:
 *       assignment  -> ID '=' expression
 *       expression  -> term (('+' | '-') term)*
 *       term        -> factor (('*' | '/') factor)*
 *       factor      -> ID | NUMBER | '(' expression ')'
 *   Each binary operation emits a TAC instruction with a fresh temporary
 *   and returns the temporary name so the caller can reference it.
 *
 * TIME COMPLEXITY:  O(n), single pass over the expression.
 * SPACE COMPLEXITY: O(n) for tokens and generated instructions.
 *
 * EXAMPLE:
 *   Input:  x = a + b * c - d / e
 *   Output:
 *       t1 = b * c
 *       t2 = a + t1
 *       t3 = d / e
 *       t4 = t2 - t3
 *       x  = t4
 * ===========================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <stdexcept>

/* ---- Token -------------------------------------------------------------- */

enum class TokenType { ID, NUM, OP, END };

struct Token {
    TokenType   type;
    std::string value;
};

/* ---- Lexer -------------------------------------------------------------- */

class Lexer {
public:
    explicit Lexer(const std::string &text) {
        tokenize(text);
        pos_ = 0;
    }

    const Token &peek() const          { return tokens_[pos_]; }
    Token        advance()             { return tokens_[pos_++]; }
    Token        expect(TokenType t, const std::string &val = "") {
        Token tok = advance();
        if (tok.type != t || (!val.empty() && tok.value != val))
            throw std::runtime_error("Unexpected token: " + tok.value);
        return tok;
    }

private:
    void tokenize(const std::string &text) {
        size_t i = 0, len = text.size();
        while (i < len) {
            if (std::isspace(static_cast<unsigned char>(text[i]))) {
                ++i;
            } else if (std::string("+-*/=()").find(text[i]) != std::string::npos) {
                tokens_.push_back({TokenType::OP, std::string(1, text[i])});
                ++i;
            } else if (std::isalpha(static_cast<unsigned char>(text[i])) || text[i] == '_') {
                size_t start = i;
                while (i < len && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_'))
                    ++i;
                tokens_.push_back({TokenType::ID, text.substr(start, i - start)});
            } else if (std::isdigit(static_cast<unsigned char>(text[i]))) {
                size_t start = i;
                while (i < len && std::isdigit(static_cast<unsigned char>(text[i])))
                    ++i;
                tokens_.push_back({TokenType::NUM, text.substr(start, i - start)});
            } else {
                throw std::runtime_error(std::string("Unexpected character: ") + text[i]);
            }
        }
        tokens_.push_back({TokenType::END, ""});
    }

    std::vector<Token> tokens_;
    size_t             pos_;
};

/* ---- TAC Instruction ---------------------------------------------------- */

struct TACInstr {
    std::string result;
    std::string left;
    char        op;          /* '\0' for simple copy / assignment */
    std::string right;
};

/* ---- TAC Generator (recursive descent parser) --------------------------- */

class TACGenerator {
public:
    /* Parse an assignment expression and return the generated TAC list. */
    std::vector<TACInstr> generate(const std::string &expression) {
        instructions_.clear();
        temp_counter_ = 0;

        Lexer lexer(expression);
        std::string target = lexer.expect(TokenType::ID).value;
        lexer.expect(TokenType::OP, "=");
        std::string expr_result = parse_expression(lexer);
        emit(target, expr_result, '\0', "");

        return instructions_;
    }

private:
    /* Allocate a fresh temporary. */
    std::string new_temp() {
        return "t" + std::to_string(++temp_counter_);
    }

    void emit(const std::string &result, const std::string &left,
              char op, const std::string &right) {
        instructions_.push_back({result, left, op, right});
    }

    /* expression -> term (('+' | '-') term)* */
    std::string parse_expression(Lexer &lexer) {
        std::string left = parse_term(lexer);
        while (lexer.peek().type == TokenType::OP &&
               (lexer.peek().value == "+" || lexer.peek().value == "-")) {
            char op = lexer.advance().value[0];
            std::string right = parse_term(lexer);
            std::string temp = new_temp();
            emit(temp, left, op, right);
            left = temp;
        }
        return left;
    }

    /* term -> factor (('*' | '/') factor)* */
    std::string parse_term(Lexer &lexer) {
        std::string left = parse_factor(lexer);
        while (lexer.peek().type == TokenType::OP &&
               (lexer.peek().value == "*" || lexer.peek().value == "/")) {
            char op = lexer.advance().value[0];
            std::string right = parse_factor(lexer);
            std::string temp = new_temp();
            emit(temp, left, op, right);
            left = temp;
        }
        return left;
    }

    /* factor -> ID | NUM | '(' expression ')' */
    std::string parse_factor(Lexer &lexer) {
        const Token &tok = lexer.peek();
        if (tok.type == TokenType::ID || tok.type == TokenType::NUM) {
            return lexer.advance().value;
        } else if (tok.type == TokenType::OP && tok.value == "(") {
            lexer.advance();
            std::string result = parse_expression(lexer);
            lexer.expect(TokenType::OP, ")");
            return result;
        }
        throw std::runtime_error("Unexpected token: " + tok.value);
    }

    int                      temp_counter_ = 0;
    std::vector<TACInstr>    instructions_;
};

/* ---- Pretty printer ----------------------------------------------------- */

static void print_tac(const std::vector<TACInstr> &instrs) {
    for (const auto &instr : instrs) {
        if (instr.op != '\0')
            std::cout << "    " << instr.result << " = "
                      << instr.left << " " << instr.op << " " << instr.right << "\n";
        else
            std::cout << "    " << instr.result << " = " << instr.left << "\n";
    }
}

/* ---- main --------------------------------------------------------------- */

int main() {
    std::vector<std::string> test_expressions = {
        "x = a + b * c - d / e",
        "y = (a + b) * (c - d)",
        "z = a * b + c * d + e * f",
        "w = a + b",
        "r = (a + b) * c - d / (e + f)"
    };

    std::cout << "=================================================================\n";
    std::cout << "   THREE-ADDRESS CODE GENERATION  (C++)\n";
    std::cout << "=================================================================\n\n";

    TACGenerator generator;

    for (size_t i = 0; i < test_expressions.size(); ++i) {
        std::cout << "  --- Test Case " << (i + 1) << " ---\n";
        std::cout << "  Expression : " << test_expressions[i] << "\n";
        std::cout << "  Three-Address Code:\n";
        auto instrs = generator.generate(test_expressions[i]);
        print_tac(instrs);
        std::cout << "\n";
    }

    std::cout << "=================================================================\n";
    std::cout << "  All expressions processed successfully.\n";
    std::cout << "=================================================================\n";
    return 0;
}
