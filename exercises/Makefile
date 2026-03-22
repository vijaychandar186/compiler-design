# Compiler Design Exercises - Makefile
# =====================================
# Usage:
#   make all        - Compile all C and C++ exercises
#   make c          - Compile all C exercises
#   make cpp        - Compile all C++ exercises
#   make clean      - Remove all compiled binaries
#   make run-py     - Run all Python exercises
#   make test       - Run all tests

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c11
CXXFLAGS = -Wall -Wextra -std=c++17

EXERCISES = 01_lexical_analyzer 02_re_to_nfa 03_nfa_to_dfa 04a_left_factoring \
            04b_left_recursion 05_first_follow 06_predictive_parser \
            07_shift_reduce_parser 08_leading_trailing 09_lr0_parser \
            10_infix_prefix_postfix 11_three_address_code 12_code_generator 13_dag

# Source file base names (matching directory names without prefix numbers)
SRC_01 = lexical_analyzer
SRC_02 = re_to_nfa
SRC_03 = nfa_to_dfa
SRC_04A = left_factoring
SRC_04B = left_recursion
SRC_05 = first_follow
SRC_06 = predictive_parser
SRC_07 = shift_reduce_parser
SRC_08 = leading_trailing
SRC_09 = lr0_parser
SRC_10 = infix_prefix_postfix
SRC_11 = three_address_code
SRC_12 = code_generator
SRC_13 = dag

.PHONY: all c cpp clean run-py test

all: c cpp

c:
	@for ex in $(EXERCISES); do \
		src=$$(echo $$ex | sed 's/^[0-9]*[ab]*_//'); \
		echo "Compiling c/$$ex/$$src.c"; \
		$(CC) $(CFLAGS) -o c/$$ex/$${src}_c c/$$ex/$$src.c -lm 2>/dev/null || true; \
	done

cpp:
	@for ex in $(EXERCISES); do \
		src=$$(echo $$ex | sed 's/^[0-9]*[ab]*_//'); \
		echo "Compiling cpp/$$ex/$$src.cpp"; \
		$(CXX) $(CXXFLAGS) -o cpp/$$ex/$${src}_cpp cpp/$$ex/$$src.cpp 2>/dev/null || true; \
	done

run-py:
	python3 run.py all

test:
	python3 ../tests/run_all_tests.py

clean:
	@find c/ -type f ! -name "*.c" ! -name ".gitignore" -delete 2>/dev/null || true
	@find cpp/ -type f ! -name "*.cpp" ! -name ".gitignore" -delete 2>/dev/null || true
	@echo "Cleaned all binaries."
