#!/usr/bin/env python3
"""Tests for Exercise 12: Simple Code Generator"""

import sys, subprocess
from pathlib import Path

EXERCISE_DIR = Path(__file__).parent.parent / "exercises" / "python" / "12_code_generator"


def run_py(stdin="1\n"):
    result = subprocess.run(
        [sys.executable, str(EXERCISE_DIR / "code_generator.py")],
        input=stdin, capture_output=True, text=True, timeout=10, cwd=str(EXERCISE_DIR)
    )
    return result.stdout + result.stderr


def test_generates_mov_instructions():
    output = run_py()
    assert "MOV" in output.upper(), "Should generate MOV instructions"


def test_generates_arithmetic_instructions():
    output = run_py()
    out_upper = output.upper()
    assert any(op in out_upper for op in ["ADD", "SUB", "MUL", "DIV"]), \
        "Should generate arithmetic instructions (ADD/SUB/MUL/DIV)"


def test_uses_registers():
    output = run_py()
    assert "R0" in output or "R1" in output or "r0" in output.lower(), \
        "Should use registers (R0, R1, etc.)"


def test_multiple_instructions():
    output = run_py()
    lines = [l.strip() for l in output.split("\n") if l.strip()]
    # Should have at least 3 assembly instructions
    asm_lines = [l for l in lines if any(kw in l.upper() for kw in ["MOV", "ADD", "SUB", "MUL", "DIV"])]
    assert len(asm_lines) >= 3, "Should generate multiple assembly instructions"


if __name__ == "__main__":
    tests = [test_generates_mov_instructions, test_generates_arithmetic_instructions,
             test_uses_registers, test_multiple_instructions]
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
