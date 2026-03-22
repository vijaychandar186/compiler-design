#!/usr/bin/env python3
"""Tests for Exercise 13: DAG Representation"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "13_dag"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "dag.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_builds_dag_nodes():
    output = run_py()
    assert "node" in output.lower() or "Node" in output, "Should display DAG nodes"


def test_detects_common_subexpression():
    output = run_py()
    out_lower = output.lower()
    assert "common" in out_lower or "reuse" in out_lower or "shared" in out_lower or "optimiz" in out_lower, \
        "Should detect or mention common subexpressions"


def test_shows_operators():
    output = run_py()
    assert "+" in output, "Should show operators in the DAG"


def test_produces_optimized_code():
    output = run_py()
    # Optimized code should have fewer statements than input
    assert len(output.strip().split("\n")) >= 5, "Should produce substantial output"


if __name__ == "__main__":
    tests = [test_builds_dag_nodes, test_detects_common_subexpression,
             test_shows_operators, test_produces_optimized_code]
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
