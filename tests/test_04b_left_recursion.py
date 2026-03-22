#!/usr/bin/env python3
"""Tests for Exercise 4b: Left Recursion Elimination"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "04b_left_recursion"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "left_recursion.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_shows_original_grammar():
    output = run_py()
    assert "->" in output, "Should display productions"


def test_eliminates_recursion():
    output = run_py()
    assert "'" in output, "Should introduce primed non-terminals (E', T', etc.)"


def test_includes_epsilon():
    output = run_py()
    out_lower = output.lower()
    assert "epsilon" in out_lower or "ε" in output or "#" in output or "eps" in out_lower, \
        "Primed non-terminals should have epsilon production"


if __name__ == "__main__":
    tests = [test_shows_original_grammar, test_eliminates_recursion, test_includes_epsilon]
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
