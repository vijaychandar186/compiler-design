# Compiler Design Exercises

13 compiler design exercises implemented in **Python**, **C**, and **C++** (42 programs total). Each exercise includes detailed algorithm explanations.

## Exercises

| #   | Exercise                          | Concepts                                     |
|-----|-----------------------------------|----------------------------------------------|
| 1   | Lexical Analyzer                  | Tokenization, pattern matching, DFA scanning |
| 2   | Regular Expression to NFA         | Thompson's construction, epsilon transitions |
| 3   | NFA to DFA Conversion             | Subset construction, epsilon-closure         |
| 4a  | Left Factoring                    | Grammar transformation, common prefixes      |
| 4b  | Left Recursion Elimination        | Direct/indirect recursion removal            |
| 5   | First and Follow Sets             | LL parsing prerequisites, fixed-point        |
| 6   | Predictive Parser (LL(1))         | Table-driven parsing, parse table            |
| 7   | Shift-Reduce Parser               | Bottom-up parsing, stack-based               |
| 8   | Leading and Trailing Sets         | Operator precedence parsing                  |
| 9   | LR(0) Parser                      | Canonical item sets, closure, goto           |
| 10  | Infix/Prefix/Postfix Conversion   | Shunting-yard algorithm, expression trees    |
| 11  | Three Address Code Generation     | Intermediate representation, temporaries     |
| 12  | Simple Code Generator             | Target code, register allocation             |
| 13  | DAG Representation                | Common subexpression elimination             |

## Project Structure

```
compiler-design/
├── exercises/
│   ├── python/                    # Python implementations
│   │   ├── .gitignore
│   │   ├── 01_lexical_analyzer/
│   │   │   └── lexical_analyzer.py
│   │   ├── 02_re_to_nfa/
│   │   │   └── re_to_nfa.py
│   │   └── ...
│   ├── c/                         # C implementations
│   │   ├── .gitignore
│   │   ├── 01_lexical_analyzer/
│   │   │   └── lexical_analyzer.c
│   │   └── ...
│   ├── cpp/                       # C++ implementations
│   │   ├── .gitignore
│   │   ├── 01_lexical_analyzer/
│   │   │   └── lexical_analyzer.cpp
│   │   └── ...
│   ├── run.py                     # Interactive exercise runner
│   └── Makefile                   # Compile C/C++ exercises
├── tests/
│   ├── run_all_tests.py           # Main test runner (42 tests)
│   ├── test_01_lexical_analyzer.py
│   ├── test_02_re_to_nfa.py
│   └── ...                        # Per-exercise test files
└── README.md
```

## Quick Start

### Run a Python exercise

```bash
# Interactive menu
python exercises/run.py

# Run a specific exercise
python exercises/run.py 1          # Lexical Analyzer
python exercises/run.py 5          # First and Follow
python exercises/run.py 4a         # Left Factoring

# Run all exercises
python exercises/run.py all
```

### Run a specific file directly

```bash
# Python
python exercises/python/01_lexical_analyzer/lexical_analyzer.py

# C
gcc -std=c11 -o lexer exercises/c/01_lexical_analyzer/lexical_analyzer.c && ./lexer

# C++
g++ -std=c++17 -o lexer exercises/cpp/01_lexical_analyzer/lexical_analyzer.cpp && ./lexer
```

### Compile all C/C++

```bash
cd exercises
make all       # Compile everything
make c         # C only
make cpp       # C++ only
make clean     # Remove binaries
```

## Testing

### Run all tests (Python + C + C++)

```bash
python tests/run_all_tests.py
```

### Filter by language

```bash
python tests/run_all_tests.py --py      # Python only
python tests/run_all_tests.py --c       # C only
python tests/run_all_tests.py --cpp     # C++ only
```

### Filter by exercise

```bash
python tests/run_all_tests.py --ex 1    # Exercise 1 only
python tests/run_all_tests.py --ex 4a   # Exercise 4a only
```

### Run per-exercise detailed tests

```bash
python tests/test_01_lexical_analyzer.py
python tests/test_05_first_follow.py
python tests/test_13_dag.py
```

### Verbose mode (show output on failures)

```bash
python tests/run_all_tests.py --verbose
```

## Requirements

- **Python 3.6+**
- **GCC** (for C exercises) - `gcc` with C11 support
- **G++** (for C++ exercises) - `g++` with C++17 support
