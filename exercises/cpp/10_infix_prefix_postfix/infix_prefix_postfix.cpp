/*
 * ==============================================================================
 * Exercise 10: Infix to Prefix and Postfix Conversion
 * ==============================================================================
 *
 * CONCEPT:
 * Infix notation places operators between operands and relies on precedence
 * and parentheses for disambiguation. Prefix (Polish) and postfix (Reverse
 * Polish) notations eliminate this ambiguity entirely, which is why compilers
 * convert expressions to these forms for evaluation.
 *
 * ALGORITHM FOR INFIX TO POSTFIX (Shunting-Yard by Dijkstra):
 *   Uses an operator stack. Operands pass through to output. Operators
 *   trigger popping of higher-or-equal precedence operators from the stack
 *   (accounting for right-associativity). Parentheses act as scope barriers.
 *
 * ALGORITHM FOR INFIX TO PREFIX (Reverse-Scan Method):
 *   Reverse the input (swapping parentheses), apply a modified shunting-yard
 *   where same-precedence left-associative operators do not pop each other,
 *   then reverse the output.
 *
 * TIME COMPLEXITY:  O(n) for each conversion.
 * SPACE COMPLEXITY: O(n) for stack and output storage.
 *
 * EXAMPLE INPUT:  (A+B)*C-D/E
 * EXAMPLE OUTPUT:
 *   Postfix: A B + C * D E / -
 *   Prefix:  - * + A B C / D E
 * ==============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <algorithm>
#include <cctype>

/* --- Operator utilities --- */

static int precedence(char op)
{
    switch (op) {
    case '+': case '-': return 1;
    case '*': case '/': return 2;
    case '^':           return 3;
    default:            return 0;
    }
}

static bool is_operator(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

static bool is_right_associative(char c)
{
    return c == '^';
}

/* --- Tokenizer --- */

static std::vector<std::string> tokenize(const std::string &expr)
{
    std::vector<std::string> tokens;
    size_t i = 0;

    while (i < expr.size()) {
        char c = expr[i];
        if (std::isspace(c)) {
            i++;
            continue;
        }
        if (c == '(' || c == ')' || is_operator(c)) {
            tokens.push_back(std::string(1, c));
            i++;
        } else if (std::isalnum(c)) {
            /* Collect multi-character operand */
            size_t start = i;
            while (i < expr.size() && std::isalnum(expr[i]))
                i++;
            tokens.push_back(expr.substr(start, i - start));
        } else {
            i++; /* Skip unknown */
        }
    }
    return tokens;
}

/* --- Infix to Postfix (Shunting-Yard) --- */

static std::vector<std::string> infix_to_postfix(const std::vector<std::string> &tokens)
{
    std::vector<std::string> output;
    std::stack<std::string> ops;

    for (const auto &token : tokens) {
        if (token.size() == 1 && is_operator(token[0])) {
            char op = token[0];
            while (!ops.empty() &&
                   ops.top() != "(" &&
                   ops.top().size() == 1 &&
                   is_operator(ops.top()[0]) &&
                   (precedence(ops.top()[0]) > precedence(op) ||
                    (precedence(ops.top()[0]) == precedence(op) &&
                     !is_right_associative(op)))) {
                output.push_back(ops.top());
                ops.pop();
            }
            ops.push(token);
        } else if (token == "(") {
            ops.push(token);
        } else if (token == ")") {
            while (!ops.empty() && ops.top() != "(") {
                output.push_back(ops.top());
                ops.pop();
            }
            if (!ops.empty())
                ops.pop(); /* Remove '(' */
        } else {
            /* Operand */
            output.push_back(token);
        }
    }

    while (!ops.empty()) {
        if (ops.top() != "(")
            output.push_back(ops.top());
        ops.pop();
    }

    return output;
}

/* --- Infix to Prefix (Reverse-Scan Method) --- */

static std::vector<std::string> infix_to_prefix(const std::vector<std::string> &tokens)
{
    /* Step 1: Reverse tokens and swap parentheses */
    std::vector<std::string> reversed_tokens;
    for (auto it = tokens.rbegin(); it != tokens.rend(); ++it) {
        if (*it == "(")
            reversed_tokens.push_back(")");
        else if (*it == ")")
            reversed_tokens.push_back("(");
        else
            reversed_tokens.push_back(*it);
    }

    /* Step 2: Modified shunting-yard.
     * For left-associative operators at same precedence, do NOT pop
     * (reversed associativity due to the reversal). */
    std::vector<std::string> output;
    std::stack<std::string> ops;

    for (const auto &token : reversed_tokens) {
        if (token.size() == 1 && is_operator(token[0])) {
            char op = token[0];
            while (!ops.empty() &&
                   ops.top() != "(" &&
                   ops.top().size() == 1 &&
                   is_operator(ops.top()[0]) &&
                   (precedence(ops.top()[0]) > precedence(op) ||
                    (precedence(ops.top()[0]) == precedence(op) &&
                     is_right_associative(op)))) {
                output.push_back(ops.top());
                ops.pop();
            }
            ops.push(token);
        } else if (token == "(") {
            ops.push(token);
        } else if (token == ")") {
            while (!ops.empty() && ops.top() != "(") {
                output.push_back(ops.top());
                ops.pop();
            }
            if (!ops.empty())
                ops.pop();
        } else {
            output.push_back(token);
        }
    }

    while (!ops.empty()) {
        if (ops.top() != "(")
            output.push_back(ops.top());
        ops.pop();
    }

    /* Step 3: Reverse to get prefix */
    std::reverse(output.begin(), output.end());
    return output;
}

/* --- Printing --- */

static std::string join(const std::vector<std::string> &vec,
                        const std::string &sep)
{
    std::string result;
    for (size_t i = 0; i < vec.size(); i++) {
        if (i > 0)
            result += sep;
        result += vec[i];
    }
    return result;
}

static void convert_and_print(const std::string &infix, int example_num)
{
    auto tokens  = tokenize(infix);
    auto postfix = infix_to_postfix(tokens);
    auto prefix  = infix_to_prefix(tokens);

    std::cout << "\n  --------------------------------------------------\n";
    std::cout << "  Example " << example_num << ":\n";
    std::cout << "  --------------------------------------------------\n";
    std::cout << "  Infix:   " << infix << "\n";
    std::cout << "  Tokens:  " << join(tokens, " ") << "\n";
    std::cout << "  Postfix: " << join(postfix, " ") << "\n";
    std::cout << "  Prefix:  " << join(prefix, " ") << "\n";
}

/* --- Main --- */

int main()
{
    std::cout << "=======================================================\n";
    std::cout << "  Infix to Prefix and Postfix Converter\n";
    std::cout << "=======================================================\n";

    std::vector<std::string> expressions = {
        "(A+B)*C-D/E",
        "A+B*C",
        "A*(B+C)/D",
        "A+B*C-D",
        "A^B^C",
        "((A+B)*C-(D-E))/(F+G)",
        "A*B+C*D",
        "A+B+C+D"
    };

    for (int i = 0; i < static_cast<int>(expressions.size()); i++) {
        convert_and_print(expressions[i], i + 1);
    }

    std::cout << "\n=======================================================\n";
    std::cout << "\n  Note on right-associativity of ^ (exponentiation):\n";
    std::cout << "  A^B^C is parsed as A^(B^C), NOT (A^B)^C\n";
    std::cout << "  Postfix: A B C ^ ^  (B^C is computed first)\n";
    std::cout << "  Prefix:  ^ A ^ B C\n";
    std::cout << "\n=======================================================\n";

    return 0;
}
