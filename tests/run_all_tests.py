#!/usr/bin/env python3
"""
Compiler Design Exercises - Test Runner
========================================
Runs all tests for all exercises across Python, C, and C++ implementations.

Usage:
    python tests/run_all_tests.py              # Run all tests
    python tests/run_all_tests.py --py         # Run only Python tests
    python tests/run_all_tests.py --c          # Run only C tests
    python tests/run_all_tests.py --cpp        # Run only C++ tests
    python tests/run_all_tests.py --ex 1       # Run tests for exercise 1 only
    python tests/run_all_tests.py --ex 4a      # Run tests for exercise 4a only
    python tests/run_all_tests.py --verbose    # Show full output on failures
"""

import os
import sys
import subprocess
import argparse
import time
from pathlib import Path

# Resolve paths
SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
EXERCISES_DIR = PROJECT_ROOT / "exercises"

# ANSI colors
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"

PASS = 0
FAIL = 0
SKIP = 0


def log_pass(msg):
    global PASS
    PASS += 1
    print(f"  {GREEN}PASS{RESET}  {msg}")


def log_fail(msg, detail=""):
    global FAIL
    FAIL += 1
    print(f"  {RED}FAIL{RESET}  {msg}")
    if detail:
        for line in detail.strip().split("\n")[:15]:
            print(f"        {line}")


def log_skip(msg):
    global SKIP
    SKIP += 1
    print(f"  {YELLOW}SKIP{RESET}  {msg}")


def run_cmd(cmd, cwd=None, stdin_data=None, timeout=15):
    """Run a command and return (success, stdout, stderr)."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            input=stdin_data,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "TIMEOUT"
    except FileNotFoundError:
        return False, "", "FILE NOT FOUND"


def compile_c(src, out, cwd):
    """Compile a C file. Returns (success, error_msg)."""
    ok, stdout, stderr = run_cmd(
        ["gcc", "-Wall", "-Wextra", "-std=c11", "-o", str(out), str(src), "-lm"],
        cwd=str(cwd),
        timeout=30,
    )
    return ok, stderr


def compile_cpp(src, out, cwd):
    """Compile a C++ file. Returns (success, error_msg)."""
    ok, stdout, stderr = run_cmd(
        ["g++", "-Wall", "-Wextra", "-std=c++17", "-o", str(out), str(src)],
        cwd=str(cwd),
        timeout=30,
    )
    return ok, stderr


# ============================================================
# Test definitions per exercise
# Each test returns True/False and checks output for correctness
# ============================================================

def check_output_contains(output, required_strings, case_insensitive=False):
    """Check that output contains all required strings."""
    text = output.lower() if case_insensitive else output
    missing = []
    for s in required_strings:
        target = s.lower() if case_insensitive else s
        if target not in text:
            missing.append(s)
    return missing


EXERCISE_TESTS = {}


def register_test(ex_num, folder, base_name, stdin_input, required_in_output,
                  case_insensitive=False, description=""):
    """Register a test case for an exercise."""
    if ex_num not in EXERCISE_TESTS:
        EXERCISE_TESTS[ex_num] = []
    EXERCISE_TESTS[ex_num].append({
        "folder": folder,
        "base_name": base_name,
        "stdin": stdin_input,
        "required": required_in_output,
        "case_insensitive": case_insensitive,
        "description": description,
    })


# --- Exercise 1: Lexical Analyzer ---
register_test("1", "01_lexical_analyzer", "lexical_analyzer",
    stdin_input="1\n",
    required_in_output=["keyword", "identifier", "operator"],
    case_insensitive=True,
    description="Tokenizes sample C code into keywords, identifiers, operators")

# --- Exercise 2: RE to NFA ---
register_test("2", "02_re_to_nfa", "re_to_nfa",
    stdin_input="1\n",
    required_in_output=["state", "transition"],
    case_insensitive=True,
    description="Converts regex (a|b)*abb to NFA using Thompson's construction")

# --- Exercise 3: NFA to DFA ---
register_test("3", "03_nfa_to_dfa", "nfa_to_dfa",
    stdin_input="1\n",
    required_in_output=["DFA", "state"],
    case_insensitive=True,
    description="Converts NFA to DFA using subset construction")

# --- Exercise 4a: Left Factoring ---
register_test("4a", "04a_left_factoring", "left_factoring",
    stdin_input="1\n",
    required_in_output=["->"],
    case_insensitive=False,
    description="Factors common prefixes from grammar productions")

# --- Exercise 4b: Left Recursion ---
register_test("4b", "04b_left_recursion", "left_recursion",
    stdin_input="1\n",
    required_in_output=["->"],
    case_insensitive=False,
    description="Eliminates left recursion from grammar")

# --- Exercise 5: First and Follow ---
register_test("5", "05_first_follow", "first_follow",
    stdin_input="1\n",
    required_in_output=["FIRST", "FOLLOW"],
    case_insensitive=True,
    description="Computes FIRST and FOLLOW sets for grammar")

# --- Exercise 6: Predictive Parser ---
register_test("6", "06_predictive_parser", "predictive_parser",
    stdin_input="1\n",
    required_in_output=["pars"],
    case_insensitive=True,
    description="LL(1) predictive parsing with table construction")

# --- Exercise 7: Shift-Reduce Parser ---
register_test("7", "07_shift_reduce_parser", "shift_reduce_parser",
    stdin_input="1\n",
    required_in_output=["shift", "reduce"],
    case_insensitive=True,
    description="Shift-reduce parsing trace for expression grammar")

# --- Exercise 8: Leading and Trailing ---
register_test("8", "08_leading_trailing", "leading_trailing",
    stdin_input="1\n",
    required_in_output=["LEADING", "TRAILING"],
    case_insensitive=True,
    description="Computes LEADING and TRAILING sets for operator grammar")

# --- Exercise 9: LR(0) Parser ---
register_test("9", "09_lr0_parser", "lr0_parser",
    stdin_input="1\n",
    required_in_output=["I0"],
    case_insensitive=False,
    description="Builds canonical collection of LR(0) item sets")

# --- Exercise 10: Infix/Prefix/Postfix ---
register_test("10", "10_infix_prefix_postfix", "infix_prefix_postfix",
    stdin_input="1\n",
    required_in_output=["postfix", "prefix"],
    case_insensitive=True,
    description="Converts infix to prefix and postfix notation")

# --- Exercise 11: Three Address Code ---
register_test("11", "11_three_address_code", "three_address_code",
    stdin_input="1\n",
    required_in_output=["t1"],
    case_insensitive=False,
    description="Generates three-address code from expression")

# --- Exercise 12: Code Generator ---
register_test("12", "12_code_generator", "code_generator",
    stdin_input="1\n",
    required_in_output=["MOV"],
    case_insensitive=True,
    description="Generates assembly-like target code from TAC")

# --- Exercise 13: DAG ---
register_test("13", "13_dag", "dag",
    stdin_input="1\n",
    required_in_output=["node", "+"],
    case_insensitive=True,
    description="Builds DAG and detects common subexpressions")


def run_python_test(test, verbose=False):
    """Run a Python test case."""
    py_file = EXERCISES_DIR / "python" / test["folder"] / f"{test['base_name']}.py"
    if not py_file.exists():
        log_skip(f"[Python] python/{test['folder']}/{test['base_name']}.py (not found)")
        return

    ok, stdout, stderr = run_cmd(
        [sys.executable, str(py_file)],
        cwd=str(EXERCISES_DIR / "python" / test["folder"]),
        stdin_data=test["stdin"],
    )

    output = stdout + stderr
    if not ok:
        log_fail(f"[Python] {test['base_name']}.py - {test['description']}",
                 stderr if verbose else "")
        return

    missing = check_output_contains(output, test["required"], test["case_insensitive"])
    if missing:
        log_fail(f"[Python] {test['base_name']}.py - missing in output: {missing}",
                 output[:500] if verbose else "")
    else:
        log_pass(f"[Python] {test['base_name']}.py - {test['description']}")


def run_c_test(test, verbose=False):
    """Compile and run a C test case."""
    c_file = EXERCISES_DIR / "c" / test["folder"] / f"{test['base_name']}.c"
    if not c_file.exists():
        log_skip(f"[C]      c/{test['folder']}/{test['base_name']}.c (not found)")
        return

    binary = EXERCISES_DIR / "c" / test["folder"] / f"test_{test['base_name']}_c"
    ok, err = compile_c(c_file, binary, EXERCISES_DIR / "c" / test["folder"])
    if not ok:
        log_fail(f"[C]      {test['base_name']}.c - compilation failed",
                 err if verbose else "")
        return

    ok, stdout, stderr = run_cmd(
        [str(binary)],
        cwd=str(EXERCISES_DIR / "c" / test["folder"]),
        stdin_data=test["stdin"],
    )

    # Clean up binary
    binary.unlink(missing_ok=True)

    output = stdout + stderr
    if not ok:
        log_fail(f"[C]      {test['base_name']}.c - runtime error",
                 stderr if verbose else "")
        return

    missing = check_output_contains(output, test["required"], test["case_insensitive"])
    if missing:
        log_fail(f"[C]      {test['base_name']}.c - missing in output: {missing}",
                 output[:500] if verbose else "")
    else:
        log_pass(f"[C]      {test['base_name']}.c - {test['description']}")


def run_cpp_test(test, verbose=False):
    """Compile and run a C++ test case."""
    cpp_file = EXERCISES_DIR / "cpp" / test["folder"] / f"{test['base_name']}.cpp"
    if not cpp_file.exists():
        log_skip(f"[C++]    cpp/{test['folder']}/{test['base_name']}.cpp (not found)")
        return

    binary = EXERCISES_DIR / "cpp" / test["folder"] / f"test_{test['base_name']}_cpp"
    ok, err = compile_cpp(cpp_file, binary, EXERCISES_DIR / "cpp" / test["folder"])
    if not ok:
        log_fail(f"[C++]    {test['base_name']}.cpp - compilation failed",
                 err if verbose else "")
        return

    ok, stdout, stderr = run_cmd(
        [str(binary)],
        cwd=str(EXERCISES_DIR / "cpp" / test["folder"]),
        stdin_data=test["stdin"],
    )

    # Clean up binary
    binary.unlink(missing_ok=True)

    output = stdout + stderr
    if not ok:
        log_fail(f"[C++]    {test['base_name']}.cpp - runtime error",
                 stderr if verbose else "")
        return

    missing = check_output_contains(output, test["required"], test["case_insensitive"])
    if missing:
        log_fail(f"[C++]    {test['base_name']}.cpp - missing in output: {missing}",
                 output[:500] if verbose else "")
    else:
        log_pass(f"[C++]    {test['base_name']}.cpp - {test['description']}")


def main():
    parser = argparse.ArgumentParser(description="Run all compiler design exercise tests")
    parser.add_argument("--py", action="store_true", help="Run only Python tests")
    parser.add_argument("--c", action="store_true", help="Run only C tests")
    parser.add_argument("--cpp", action="store_true", help="Run only C++ tests")
    parser.add_argument("--ex", type=str, help="Run tests for specific exercise (e.g., 1, 4a)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Show output on failures")
    args = parser.parse_args()

    # If no language filter, run all
    run_py = args.py or (not args.py and not args.c and not args.cpp)
    run_c = args.c or (not args.py and not args.c and not args.cpp)
    run_cpp = args.cpp or (not args.py and not args.c and not args.cpp)

    print()
    print(f"{BOLD}{'=' * 64}{RESET}")
    print(f"{BOLD}  COMPILER DESIGN EXERCISES - TEST SUITE{RESET}")
    print(f"{BOLD}{'=' * 64}{RESET}")
    print()

    start_time = time.time()

    # Determine which exercises to test
    exercise_keys = sorted(EXERCISE_TESTS.keys(), key=lambda x: (len(x), x))
    if args.ex:
        if args.ex in EXERCISE_TESTS:
            exercise_keys = [args.ex]
        else:
            print(f"{RED}Unknown exercise: {args.ex}{RESET}")
            print(f"Available: {', '.join(exercise_keys)}")
            sys.exit(1)

    for ex_key in exercise_keys:
        tests = EXERCISE_TESTS[ex_key]
        for test in tests:
            print(f"\n{CYAN}--- Exercise {ex_key}: {test['description']} ---{RESET}")

            if run_py:
                run_python_test(test, args.verbose)
            if run_c:
                run_c_test(test, args.verbose)
            if run_cpp:
                run_cpp_test(test, args.verbose)

    elapsed = time.time() - start_time

    print()
    print(f"{BOLD}{'=' * 64}{RESET}")
    total = PASS + FAIL + SKIP
    print(f"  {GREEN}{PASS} passed{RESET}  |  {RED}{FAIL} failed{RESET}  |  "
          f"{YELLOW}{SKIP} skipped{RESET}  |  {total} total  |  {elapsed:.1f}s")
    print(f"{BOLD}{'=' * 64}{RESET}")
    print()

    sys.exit(1 if FAIL > 0 else 0)


if __name__ == "__main__":
    main()
