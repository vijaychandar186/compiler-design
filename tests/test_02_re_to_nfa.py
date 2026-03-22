#!/usr/bin/env python3
"""Tests for Exercise 2: Regular Expression to NFA"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "02_re_to_nfa"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "re_to_nfa.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_produces_states():
    output = run_py()
    out_lower = output.lower()
    assert "state" in out_lower or "q" in output, "Should show NFA states"


def test_produces_transitions():
    output = run_py()
    out_lower = output.lower()
    assert "transition" in out_lower or "->" in output or "=>" in output, \
        "Should show transition table/function"


def test_handles_example_regex():
    output = run_py()
    # Should process without crashing and produce meaningful output
    assert len(output) > 50, "Should produce substantial output for example regex"


if __name__ == "__main__":
    tests = [test_produces_states, test_produces_transitions, test_handles_example_regex]
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
