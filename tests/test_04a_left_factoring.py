#!/usr/bin/env python3
"""Tests for Exercise 4a: Left Factoring"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "04a_left_factoring"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "left_factoring.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_shows_original_grammar():
    output = run_py()
    assert "->" in output, "Should show productions with -> notation"


def test_shows_factored_grammar():
    output = run_py()
    out_lower = output.lower()
    assert "factor" in out_lower or "new" in out_lower or "result" in out_lower or "'" in output, \
        "Should show factored grammar (with primed non-terminals)"


def test_introduces_new_nonterminal():
    output = run_py()
    assert "'" in output or "_" in output, "Should introduce new non-terminals (S', A', etc.)"


if __name__ == "__main__":
    tests = [test_shows_original_grammar, test_shows_factored_grammar,
             test_introduces_new_nonterminal]
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
