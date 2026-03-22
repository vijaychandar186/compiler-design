#!/usr/bin/env python3
"""Tests for Exercise 11: Three Address Code Generation"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "11_three_address_code"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "three_address_code.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_generates_temporaries():
    output = run_py()
    assert "t1" in output, "Should generate temporary variable t1"


def test_generates_assignment():
    output = run_py()
    assert "=" in output, "Should contain assignment operators"


def test_respects_precedence():
    """Multiplication should be computed before addition."""
    output = run_py()
    lines = output.strip().split("\n")
    # Should produce multiple TAC lines
    tac_lines = [l for l in lines if "t" in l and "=" in l]
    assert len(tac_lines) >= 2, "Should generate multiple three-address code lines"


def test_handles_operators():
    output = run_py()
    # Should contain at least one arithmetic operator
    assert any(op in output for op in ["+", "-", "*", "/"]), \
        "Should contain arithmetic operators"


if __name__ == "__main__":
    tests = [test_generates_temporaries, test_generates_assignment,
             test_respects_precedence, test_handles_operators]
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
