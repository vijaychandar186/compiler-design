#!/usr/bin/env python3
"""Tests for Exercise 8: Leading and Trailing Sets"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "08_leading_trailing"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "leading_trailing.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_computes_leading():
    output = run_py()
    assert "LEADING" in output.upper(), "Should display LEADING sets"


def test_computes_trailing():
    output = run_py()
    assert "TRAILING" in output.upper(), "Should display TRAILING sets"


def test_leading_contains_terminals():
    output = run_py()
    # For E -> E+T | T, LEADING(E) should contain + and whatever T leads with
    assert any(t in output for t in ["+", "*", "(", "i", "id"]), \
        "LEADING sets should contain terminals"


def test_trailing_contains_terminals():
    output = run_py()
    assert any(t in output for t in ["+", "*", ")", "i", "id"]), \
        "TRAILING sets should contain terminals"


if __name__ == "__main__":
    tests = [test_computes_leading, test_computes_trailing,
             test_leading_contains_terminals, test_trailing_contains_terminals]
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
