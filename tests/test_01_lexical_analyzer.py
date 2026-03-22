#!/usr/bin/env python3
"""Tests for Exercise 1: Lexical Analyzer"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'exercises', '01_lexical_analyzer'))

import subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "01_lexical_analyzer"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "lexical_analyzer.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_identifies_keywords():
    output = run_py()
    out_lower = output.lower()
    assert "keyword" in out_lower, "Should identify keywords"
    # Check for common C keywords in output
    assert any(kw in output for kw in ["int", "return", "void", "if", "while"]), \
        "Should find at least one C keyword"


def test_identifies_identifiers():
    output = run_py()
    assert "identifier" in output.lower(), "Should identify identifiers"


def test_identifies_operators():
    output = run_py()
    assert "operator" in output.lower(), "Should identify operators"


def test_identifies_numbers():
    output = run_py()
    out_lower = output.lower()
    assert "number" in out_lower or "constant" in out_lower or "integer" in out_lower or "literal" in out_lower, \
        "Should identify numeric constants"


def test_identifies_punctuation():
    output = run_py()
    out_lower = output.lower()
    assert "punct" in out_lower or "delimit" in out_lower or "separator" in out_lower or "symbol" in out_lower, \
        "Should identify punctuation/delimiters"


if __name__ == "__main__":
    tests = [test_identifies_keywords, test_identifies_identifiers,
             test_identifies_operators, test_identifies_numbers, test_identifies_punctuation]
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
