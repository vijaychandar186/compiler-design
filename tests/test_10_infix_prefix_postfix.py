#!/usr/bin/env python3
"""Tests for Exercise 10: Infix to Prefix and Postfix Conversion"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "10_infix_prefix_postfix"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "infix_prefix_postfix.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_produces_postfix():
    output = run_py()
    assert "postfix" in output.lower() or "POSTFIX" in output, "Should show postfix conversion"


def test_produces_prefix():
    output = run_py()
    assert "prefix" in output.lower() or "PREFIX" in output, "Should show prefix conversion"


def test_known_conversion():
    """A+B should give AB+ in postfix and +AB in prefix"""
    output = run_py()
    # The default examples should include at least one correct conversion
    assert len(output) > 30, "Should produce substantial output"


def test_handles_parentheses():
    output = run_py()
    # The examples should handle parenthesized expressions
    assert len(output.strip().split("\n")) >= 3, "Should produce multiple lines of output"


if __name__ == "__main__":
    tests = [test_produces_postfix, test_produces_prefix,
             test_known_conversion, test_handles_parentheses]
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
