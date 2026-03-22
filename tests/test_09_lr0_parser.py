#!/usr/bin/env python3
"""Tests for Exercise 9: LR(0) Parser"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "09_lr0_parser"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "lr0_parser.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_shows_augmented_grammar():
    output = run_py()
    out_lower = output.lower()
    assert "augment" in out_lower or "'" in output, \
        "Should show augmented grammar"


def test_shows_item_sets():
    output = run_py()
    assert "I0" in output, "Should show item set I0"


def test_shows_dot_in_items():
    output = run_py()
    # LR(0) items use a dot (.) to mark position
    assert "." in output or "·" in output, "Should show dot position in items"


def test_multiple_item_sets():
    output = run_py()
    assert "I1" in output, "Should generate multiple item sets"


if __name__ == "__main__":
    tests = [test_shows_augmented_grammar, test_shows_item_sets,
             test_shows_dot_in_items, test_multiple_item_sets]
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
