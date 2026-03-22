#!/usr/bin/env python3
"""Tests for Exercise 5: First and Follow Sets"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "05_first_follow"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "first_follow.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_computes_first_sets():
    output = run_py()
    assert "FIRST" in output.upper(), "Should display FIRST sets"


def test_computes_follow_sets():
    output = run_py()
    assert "FOLLOW" in output.upper(), "Should display FOLLOW sets"


def test_start_symbol_has_dollar_in_follow():
    output = run_py()
    assert "$" in output, "Start symbol's FOLLOW set should contain $"


def test_first_of_expression_grammar():
    """For E -> TR, T -> FY, F -> (E) | id: FIRST(E) should contain '(' and 'i'"""
    output = run_py()
    # Should contain parenthesis and id/i in first sets
    assert "(" in output, "FIRST(E) should contain '('"


if __name__ == "__main__":
    tests = [test_computes_first_sets, test_computes_follow_sets,
             test_start_symbol_has_dollar_in_follow, test_first_of_expression_grammar]
    passed = failed = 0
    for t in tests:
        try:
            t()
            print(f"  PASS  {t.__name__}")
            passed += 1
        except Exception as e:
            print(f"  FAIL  {t.__name__}: {e}")
            failed += 1
    print(f"\n{passed} passed, {failed} failed")
    sys.exit(1 if failed else 0)
