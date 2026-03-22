#!/usr/bin/env python3
"""Tests for Exercise 7: Shift-Reduce Parser"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "07_shift_reduce_parser"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "shift_reduce_parser.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_shows_shift_actions():
    output = run_py()
    assert "shift" in output.lower() or "SHIFT" in output, "Should show SHIFT actions"


def test_shows_reduce_actions():
    output = run_py()
    assert "reduce" in output.lower() or "REDUCE" in output, "Should show REDUCE actions"


def test_shows_accept_or_reject():
    output = run_py()
    out_lower = output.lower()
    assert "accept" in out_lower or "reject" in out_lower, \
        "Should show ACCEPT or REJECT result"


def test_shows_parsing_steps():
    output = run_py()
    out_lower = output.lower()
    assert "stack" in out_lower or "input" in out_lower, \
        "Should show stack and input in parsing trace"


if __name__ == "__main__":
    tests = [test_shows_shift_actions, test_shows_reduce_actions,
             test_shows_accept_or_reject, test_shows_parsing_steps]
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
