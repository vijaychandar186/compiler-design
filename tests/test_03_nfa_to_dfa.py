#!/usr/bin/env python3
"""Tests for Exercise 3: NFA to DFA Conversion"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "03_nfa_to_dfa"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "nfa_to_dfa.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_shows_nfa():
    output = run_py()
    assert "NFA" in output.upper(), "Should display NFA information"


def test_shows_dfa():
    output = run_py()
    assert "DFA" in output.upper(), "Should display DFA information"


def test_computes_states():
    output = run_py()
    out_lower = output.lower()
    assert "state" in out_lower or "{" in output, "Should show computed DFA states"


def test_identifies_final_states():
    output = run_py()
    out_lower = output.lower()
    assert "final" in out_lower or "accept" in out_lower or "*" in output, \
        "Should identify final/accepting states"


if __name__ == "__main__":
    tests = [test_shows_nfa, test_shows_dfa, test_computes_states, test_identifies_final_states]
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
