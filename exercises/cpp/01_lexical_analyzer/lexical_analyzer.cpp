/*
================================================================================
Exercise 1: Lexical Analyzer (Scanner)  --  C++ Implementation
================================================================================

CONCEPT:
    A lexical analyzer (scanner / tokenizer) is the first phase of a compiler.
    It reads the raw source code character by character and produces a stream
    of tokens.  Each token is a pair (token-type, lexeme) that the parser will
    consume in the next phase.

ALGORITHM:
    We maintain a position index into the source string and advance it inside a
    single while-loop.  At each iteration the current character determines
    which branch to take:
      * Whitespace / newlines  -> skip, update line/col counters
      * "//" -> single-line comment, skip to end of line
      * '"' -> read a string literal until the matching quote
      * Letter / underscore -> read identifier, then check keyword table
      * Digit -> read integer; if '.' follows, continue for a float
      * Two-char operator candidates -> lookahead one char
      * Single-char operator or punctuation -> emit directly
    A one-character lookahead is used to distinguish '=' from '==', etc.

COMPLEXITY:
    Time:  O(n) -- each character is visited at most twice.
    Space: O(n) -- for storing the token vector.

EXAMPLE:
    Input:   int x = 42;
    Output:  KEYWORD(int)  IDENTIFIER(x)  OPERATOR(=)  INTEGER(42)  PUNCT(;)
================================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <set>
#include <map>
#include <sstream>

/* ---- token categories --------------------------------------------------- */
enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    INTEGER_CONST,
    FLOAT_CONST,
    STRING_LITERAL,
    OPERATOR,
    PUNCTUATION,
    UNKNOWN
};

static std::string type_name(TokenType t) {
    switch (t) {
        case TokenType::KEYWORD:        return "KEYWORD";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::INTEGER_CONST:  return "INTEGER_CONST";
        case TokenType::FLOAT_CONST:    return "FLOAT_CONST";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::OPERATOR:       return "OPERATOR";
        case TokenType::PUNCTUATION:    return "PUNCTUATION";
        case TokenType::UNKNOWN:        return "UNKNOWN";
    }
    return "UNKNOWN";
}

/* ---- token structure ---------------------------------------------------- */
struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;
    int         col;
};

/* ---- language tables ---------------------------------------------------- */
static const std::set<std::string> keywords = {
    "int", "float", "char", "double", "void",
    "if", "else", "while", "for", "do",
    "return", "break", "continue",
    "struct", "typedef", "enum", "switch", "case", "default",
    "main", "printf", "scanf", "include", "define"
};

static const std::set<std::string> two_char_operators = {
    "==", "!=", "<=", ">=", "++", "--",
    "+=", "-=", "*=", "/=", "%=",
    "&&", "||", "<<", ">>"
};

static bool is_operator_char(char ch) {
    static const std::string ops = "+-*/%=<>!&|^~";
    return ops.find(ch) != std::string::npos;
}

static bool is_punctuation(char ch) {
    static const std::string puncts = ";,(){}[]:#.";
    return puncts.find(ch) != std::string::npos;
}

/* ======================================================================== */
/*  Lexer class                                                             */
/* ======================================================================== */
class Lexer {
public:
    explicit Lexer(const std::string &source) : src_(source), pos_(0),
                                                 line_(1), col_(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        int length = static_cast<int>(src_.size());

        while (pos_ < length) {
            char ch = src_[pos_];

            /* ---- whitespace -------------------------------------------- */
            if (ch == ' ' || ch == '\t' || ch == '\r') {
                pos_++; col_++;
                continue;
            }
            if (ch == '\n') {
                pos_++; line_++; col_ = 1;
                continue;
            }

            /* ---- single-line comment ----------------------------------- */
            if (ch == '/' && peek(1) == '/') {
                while (pos_ < length && src_[pos_] != '\n') pos_++;
                continue;
            }

            /* ---- multi-line comment ------------------------------------ */
            if (ch == '/' && peek(1) == '*') {
                pos_ += 2; col_ += 2;
                while (pos_ + 1 < length) {
                    if (src_[pos_] == '*' && src_[pos_ + 1] == '/') {
                        pos_ += 2; col_ += 2; break;
                    }
                    if (src_[pos_] == '\n') { line_++; col_ = 1; } else { col_++; }
                    pos_++;
                }
                continue;
            }

            /* ---- string literal ---------------------------------------- */
            if (ch == '"' || ch == '\'') {
                tokens.push_back(read_string(ch));
                continue;
            }

            /* ---- identifiers / keywords -------------------------------- */
            if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
                tokens.push_back(read_word());
                continue;
            }

            /* ---- numeric constants ------------------------------------- */
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                tokens.push_back(read_number());
                continue;
            }

            /* ---- two-character operators -------------------------------- */
            if (pos_ + 1 < length) {
                std::string two(1, ch);
                two += src_[pos_ + 1];
                if (two_char_operators.count(two)) {
                    tokens.push_back({TokenType::OPERATOR, two, line_, col_});
                    pos_ += 2; col_ += 2;
                    continue;
                }
            }

            /* ---- single-character operator ----------------------------- */
            if (is_operator_char(ch)) {
                tokens.push_back({TokenType::OPERATOR, std::string(1, ch), line_, col_});
                pos_++; col_++;
                continue;
            }

            /* ---- punctuation ------------------------------------------- */
            if (is_punctuation(ch)) {
                tokens.push_back({TokenType::PUNCTUATION, std::string(1, ch), line_, col_});
                pos_++; col_++;
                continue;
            }

            /* ---- unknown ----------------------------------------------- */
            tokens.push_back({TokenType::UNKNOWN, std::string(1, ch), line_, col_});
            pos_++; col_++;
        }

        return tokens;
    }

private:
    const std::string &src_;
    int pos_, line_, col_;

    char peek(int offset) const {
        int idx = pos_ + offset;
        if (idx >= 0 && idx < static_cast<int>(src_.size())) return src_[idx];
        return '\0';
    }

    Token read_string(char quote) {
        int start_col = col_;
        std::string buf(1, quote);
        pos_++; col_++;
        while (pos_ < static_cast<int>(src_.size()) && src_[pos_] != quote) {
            if (src_[pos_] == '\\') { buf += src_[pos_]; pos_++; col_++; }
            if (pos_ < static_cast<int>(src_.size())) {
                buf += src_[pos_]; pos_++; col_++;
            }
        }
        if (pos_ < static_cast<int>(src_.size())) {
            buf += src_[pos_]; pos_++; col_++;
        }
        return {TokenType::STRING_LITERAL, buf, line_, start_col};
    }

    Token read_word() {
        int start_col = col_;
        std::string buf;
        while (pos_ < static_cast<int>(src_.size()) &&
               (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) {
            buf += src_[pos_]; pos_++; col_++;
        }
        TokenType tt = keywords.count(buf) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
        return {tt, buf, line_, start_col};
    }

    Token read_number() {
        int start_col = col_;
        bool is_float = false;
        std::string buf;
        while (pos_ < static_cast<int>(src_.size()) &&
               std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            buf += src_[pos_]; pos_++; col_++;
        }
        if (pos_ < static_cast<int>(src_.size()) && src_[pos_] == '.') {
            is_float = true;
            buf += src_[pos_]; pos_++; col_++;
            while (pos_ < static_cast<int>(src_.size()) &&
                   std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                buf += src_[pos_]; pos_++; col_++;
            }
        }
        TokenType tt = is_float ? TokenType::FLOAT_CONST : TokenType::INTEGER_CONST;
        return {tt, buf, line_, start_col};
    }
};

/* ---- printing ----------------------------------------------------------- */
static void print_tokens(const std::vector<Token> &tokens) {
    size_t max_lex = 6;
    for (auto &t : tokens)
        max_lex = std::max(max_lex, t.lexeme.size());

    auto sep = [&]() {
        std::cout << "+-----+-------------------+-"
                  << std::string(max_lex, '-') << "-+----------+\n";
    };

    sep();
    std::cout << "|  #  | " << std::left << std::setw(17) << "TOKEN TYPE"
              << " | " << std::setw(static_cast<int>(max_lex)) << "LEXEME"
              << " | LOCATION |\n";
    sep();

    for (size_t i = 0; i < tokens.size(); i++) {
        std::ostringstream loc;
        loc << "L" << tokens[i].line << ":C" << tokens[i].col;
        std::cout << "| " << std::right << std::setw(3) << (i + 1) << " | "
                  << std::left << std::setw(17) << type_name(tokens[i].type)
                  << " | " << std::setw(static_cast<int>(max_lex)) << tokens[i].lexeme
                  << " | " << std::setw(8) << loc.str() << " |\n";
    }
    sep();
}

static void print_summary(const std::vector<Token> &tokens) {
    std::map<TokenType, int> counts;
    for (auto &t : tokens) counts[t.type]++;

    std::cout << "\n+-------------------+-------+\n";
    std::cout << "| " << std::left << std::setw(17) << "TOKEN TYPE"
              << " | " << std::right << std::setw(5) << "COUNT" << " |\n";
    std::cout << "+-------------------+-------+\n";
    for (auto &[tt, cnt] : counts) {
        std::cout << "| " << std::left << std::setw(17) << type_name(tt)
                  << " | " << std::right << std::setw(5) << cnt << " |\n";
    }
    std::cout << "+-------------------+-------+\n";
    std::cout << "| " << std::left << std::setw(17) << "TOTAL"
              << " | " << std::right << std::setw(5) << tokens.size() << " |\n";
    std::cout << "+-------------------+-------+\n";
}

/* ---- default sample ----------------------------------------------------- */
static const std::string SAMPLE_CODE = R"(
// A small C program for lexical analysis demonstration
#include <stdio.h>

int main() {
    int count = 0;
    float ratio = 3.14;
    char name[] = "hello";

    for (int i = 0; i <= 10; i++) {
        if (i != 5 && i >= 2) {
            count += i;
        } else {
            count--;
        }
    }

    while (count > 0) {
        ratio *= 0.5;
        count = count - 1;
    }

    printf("Result: %d\n", count);
    return 0;
}
)";

/* ---- main --------------------------------------------------------------- */
int main() {
    std::cout << "======================================================================\n"
              << "         LEXICAL ANALYZER  --  Compiler Design Exercise 1\n"
              << "======================================================================\n\n";

    std::cout << "Choose input mode:\n"
              << "  1) Use default sample C code\n"
              << "  2) Enter your own code (end with an empty line)\n"
              << "\nEnter choice [1]: ";

    std::string choice;
    std::getline(std::cin, choice);

    std::string source;
    if (choice == "2") {
        std::cout << "\nEnter source code (press Enter on an empty line to finish):\n\n";
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) break;
            source += line + "\n";
        }
    } else {
        source = SAMPLE_CODE;
    }

    /* display source */
    std::cout << "\n--- Source Code ---\n";
    std::istringstream ss(source);
    std::string line;
    int line_no = 1;
    while (std::getline(ss, line)) {
        std::cout << "  " << std::setw(3) << line_no++ << " | " << line << "\n";
    }
    std::cout << "--- End of Source ---\n\n";

    /* tokenize and display */
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::cout << "--- Token Table ---\n";
    print_tokens(tokens);
    print_summary(tokens);

    return 0;
}
