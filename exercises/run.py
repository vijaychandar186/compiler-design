#!/usr/bin/env python3
"""
Compiler Design Exercises - Main Runner
========================================
Run any exercise from the command line:
    python run.py                  # Show menu
    python run.py 1                # Run exercise 1 (Lexical Analyzer)
    python run.py 4a               # Run exercise 4a (Left Factoring)
    python run.py all              # Run all exercises sequentially
"""

import os
import sys
import subprocess
import importlib.util

EXERCISES = {
    "1":   ("python/01_lexical_analyzer",       "lexical_analyzer.py",       "Lexical Analyzer"),
    "2":   ("python/02_re_to_nfa",              "re_to_nfa.py",              "Regular Expression to NFA"),
    "3":   ("python/03_nfa_to_dfa",             "nfa_to_dfa.py",             "NFA to DFA Conversion"),
    "4a":  ("python/04a_left_factoring",        "left_factoring.py",         "Left Factoring"),
    "4b":  ("python/04b_left_recursion",        "left_recursion.py",         "Left Recursion Elimination"),
    "5":   ("python/05_first_follow",           "first_follow.py",           "First and Follow Sets"),
    "6":   ("python/06_predictive_parser",      "predictive_parser.py",      "Predictive Parser (LL(1))"),
    "7":   ("python/07_shift_reduce_parser",    "shift_reduce_parser.py",    "Shift-Reduce Parser"),
    "8":   ("python/08_leading_trailing",       "leading_trailing.py",       "Leading and Trailing Sets"),
    "9":   ("python/09_lr0_parser",             "lr0_parser.py",             "LR(0) Parser"),
    "10":  ("python/10_infix_prefix_postfix",   "infix_prefix_postfix.py",   "Infix/Prefix/Postfix Conversion"),
    "11":  ("python/11_three_address_code",     "three_address_code.py",     "Three Address Code Generation"),
    "12":  ("python/12_code_generator",         "code_generator.py",         "Simple Code Generator"),
    "13":  ("python/13_dag",                    "dag.py",                    "DAG Representation"),
}

BASE_DIR = os.path.dirname(os.path.abspath(__file__))


def print_menu():
    print("=" * 60)
    print("  COMPILER DESIGN EXERCISES")
    print("=" * 60)
    print()
    for key in sorted(EXERCISES.keys(), key=lambda x: (len(x), x)):
        _, _, name = EXERCISES[key]
        print(f"  [{key:>3}]  {name}")
    print()
    print("  [all]  Run all exercises")
    print("  [q]    Quit")
    print()
    print("=" * 60)


def run_exercise(key):
    if key not in EXERCISES:
        print(f"Unknown exercise: {key}")
        return False

    folder, filename, name = EXERCISES[key]
    filepath = os.path.join(BASE_DIR, folder, filename)

    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        return False

    print()
    print("#" * 60)
    print(f"#  Exercise {key}: {name}")
    print("#" * 60)
    print()

    result = subprocess.run(
        [sys.executable, filepath],
        cwd=os.path.join(BASE_DIR, folder),
        timeout=30,
    )
    print()
    return result.returncode == 0


def main():
    if len(sys.argv) > 1:
        arg = sys.argv[1].lower()
        if arg == "all":
            for key in sorted(EXERCISES.keys(), key=lambda x: (len(x), x)):
                run_exercise(key)
        elif arg in EXERCISES:
            run_exercise(arg)
        else:
            print(f"Unknown option: {arg}")
            print_menu()
        return

    while True:
        print_menu()
        choice = input("Select an exercise to run: ").strip().lower()

        if choice == "q":
            print("Goodbye!")
            break
        elif choice == "all":
            for key in sorted(EXERCISES.keys(), key=lambda x: (len(x), x)):
                run_exercise(key)
        elif choice in EXERCISES:
            run_exercise(choice)
        else:
            print(f"Invalid choice: {choice}")

        print()
        input("Press Enter to continue...")


if __name__ == "__main__":
    main()
