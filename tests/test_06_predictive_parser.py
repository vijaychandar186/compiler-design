#!/usr/bin/env python3
"""Tests for Exercise 6: Predictive Parser (LL(1))"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "06_predictive_parser"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "predictive_parser.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_shows_parsing_table():
    output = run_py()
    out_lower = output.lower()
    assert "table" in out_lower or "pars" in out_lower, "Should display parsing table"


def test_shows_parsing_trace():
    output = run_py()
    out_lower = output.lower()
    assert "stack" in out_lower or "input" in out_lower or "action" in out_lower, \
        "Should show parsing trace with stack/input/action"


def test_accepts_valid_input():
    output = run_py()
    out_lower = output.lower()
    assert "accept" in out_lower or "success" in out_lower, \
        "Should accept valid input string"


if __name__ == "__main__":
    tests = [test_shows_parsing_table, test_shows_parsing_trace, test_accepts_valid_input]
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
