"""
================================================================================
Exercise 12: Simple Code Generator (Target Code from Three-Address Code)
================================================================================

CONCEPT:
    A code generator is the final phase of a compiler that translates
    intermediate representation (three-address code) into target machine
    instructions.  This implementation produces assembly-like instructions
    (MOV, ADD, SUB, MUL, DIV) using a small set of registers (R0-R7).

ALGORITHM:
    For each three-address code statement of the form  result = left op right:
      1. Load `left` into a register (MOV left, Ri).  If `left` is already in
         a register, skip the MOV (register tracking optimisation).
      2. Apply the operation with `right` (OP right, Ri).  If `right` is in a
         register, use that register name instead.
      3. Record that Ri now holds `result`.  If `result` is not a temporary
         (i.e. it is a user variable), also emit MOV Ri, result to store it.

    A register descriptor tracks which variable each register currently holds.
    An address descriptor tracks which register (if any) holds a given variable.
    This allows the generator to avoid redundant loads when an operand is
    already available in a register.

TIME COMPLEXITY:  O(n) where n is the number of TAC instructions.
SPACE COMPLEXITY: O(r + v) where r = registers, v = live variables.

EXAMPLE:
    TAC Input:         Generated Assembly:
    t1 = a + b         MOV a, R0
                        ADD b, R0
    t2 = t1 * c        MUL c, R0
    t3 = d - e         MOV d, R1
                        SUB e, R1
    x  = t2 + t3       ADD R1, R0
                        MOV R0, x
================================================================================
"""

# Map TAC operators to assembly mnemonics
OP_MAP = {
    "+": "ADD",
    "-": "SUB",
    "*": "MUL",
    "/": "DIV",
}

NUM_REGISTERS = 8  # R0 through R7


class RegisterAllocator:
    """
    Manages a pool of registers and tracks which variable each register holds.
    Uses a simple next-available strategy with spill when all registers are in use.
    """

    def __init__(self, num_registers=NUM_REGISTERS):
        self.num_registers = num_registers
        # register_contents[i] = name of variable currently in Ri, or None
        self.register_contents = [None] * num_registers
        # variable_location[name] = register index, or None
        self.variable_location = {}

    def get_register_name(self, index):
        """Return the printable name for register index."""
        return f"R{index}"

    def find_variable(self, name):
        """Return the register index holding `name`, or -1 if not loaded."""
        return self.variable_location.get(name, -1)

    def allocate(self):
        """Return the index of a free register, or evict the first one."""
        # First, look for an empty register
        for i in range(self.num_registers):
            if self.register_contents[i] is None:
                return i
        # All occupied -- evict R0 (simple strategy)
        self._evict(0)
        return 0

    def _evict(self, index):
        """Remove the variable from a register (no spill code emitted here)."""
        old_var = self.register_contents[index]
        if old_var and old_var in self.variable_location:
            del self.variable_location[old_var]
        self.register_contents[index] = None

    def assign(self, index, variable_name):
        """Record that register `index` now holds `variable_name`."""
        # Remove old mapping for this variable if it exists elsewhere
        old_idx = self.find_variable(variable_name)
        if old_idx >= 0:
            self.register_contents[old_idx] = None

        # Remove old variable from this register
        old_var = self.register_contents[index]
        if old_var and old_var in self.variable_location:
            del self.variable_location[old_var]

        self.register_contents[index] = variable_name
        self.variable_location[variable_name] = index

    def reset(self):
        """Clear all register state."""
        self.register_contents = [None] * self.num_registers
        self.variable_location = {}


class CodeGenerator:
    """
    Translates three-address code into register-based assembly instructions.
    """

    def __init__(self):
        self.allocator = RegisterAllocator()
        self.assembly = []

    def emit(self, instruction):
        """Append an assembly instruction string."""
        self.assembly.append(instruction)

    def operand_name(self, name):
        """
        If `name` is currently in a register, return the register name;
        otherwise return the variable/literal name.
        """
        idx = self.allocator.find_variable(name)
        if idx >= 0:
            return self.allocator.get_register_name(idx)
        return name

    def is_temporary(self, name):
        """Temporaries start with 't' followed by digits."""
        return len(name) >= 2 and name[0] == "t" and name[1:].isdigit()

    def generate(self, tac_statements):
        """
        Generate assembly from a list of TAC strings.
        Each string is of the form:  result = left op right
        or a simple copy:            result = source
        """
        self.assembly = []
        self.allocator.reset()

        for statement in tac_statements:
            parts = statement.split()
            result = parts[0]
            # parts[1] is '='
            left = parts[2]

            if len(parts) == 3:
                # Simple copy: result = source
                src_reg = self.allocator.find_variable(left)
                if src_reg >= 0:
                    # Source already in a register
                    reg_idx = src_reg
                else:
                    reg_idx = self.allocator.allocate()
                    self.emit(f"MOV {left}, {self.allocator.get_register_name(reg_idx)}")
                self.allocator.assign(reg_idx, result)

                if not self.is_temporary(result):
                    self.emit(f"MOV {self.allocator.get_register_name(reg_idx)}, {result}")

            elif len(parts) == 5:
                # Binary operation: result = left op right
                operator = parts[3]
                right = parts[4]
                mnemonic = OP_MAP.get(operator, "???")

                # Load left operand
                left_reg = self.allocator.find_variable(left)
                if left_reg >= 0:
                    reg_idx = left_reg
                else:
                    reg_idx = self.allocator.allocate()
                    self.emit(f"MOV {left}, {self.allocator.get_register_name(reg_idx)}")

                # Apply operation with right operand
                right_name = self.operand_name(right)
                self.emit(f"{mnemonic} {right_name}, {self.allocator.get_register_name(reg_idx)}")

                # Mark register as holding the result
                self.allocator.assign(reg_idx, result)

                # If result is a user variable, store it to memory
                if not self.is_temporary(result):
                    self.emit(f"MOV {self.allocator.get_register_name(reg_idx)}, {result}")

        return self.assembly


def print_assembly(asm_lines):
    """Print assembly instructions with line numbers."""
    for i, line in enumerate(asm_lines, start=1):
        print(f"    {i:3d}:  {line}")


def main():
    print("=" * 65)
    print("   SIMPLE CODE GENERATOR  (Target Code from TAC)")
    print("=" * 65)
    print()

    # ---- Test Case 1 ----
    tac_input_1 = [
        "t1 = a + b",
        "t2 = t1 * c",
        "t3 = d - e",
        "x = t2 + t3",
    ]

    # ---- Test Case 2 ----
    tac_input_2 = [
        "t1 = a + b",
        "t2 = c - d",
        "t3 = t1 * t2",
        "y = t3",
    ]

    # ---- Test Case 3 ----
    tac_input_3 = [
        "t1 = a * b",
        "t2 = c * d",
        "t3 = t1 + t2",
        "t4 = e * f",
        "z = t3 + t4",
    ]

    test_cases = [
        ("x = a + b * c - d / e  (simplified TAC)", tac_input_1),
        ("y = (a + b) * (c - d)", tac_input_2),
        ("z = a*b + c*d + e*f", tac_input_3),
    ]

    generator = CodeGenerator()

    for idx, (description, tac_input) in enumerate(test_cases, start=1):
        print(f"  --- Test Case {idx}: {description} ---")
        print("  Input Three-Address Code:")
        for stmt in tac_input:
            print(f"    {stmt}")
        print()

        asm = generator.generate(tac_input)

        print("  Generated Assembly:")
        print_assembly(asm)
        print()

    print("=" * 65)
    print("  All test cases processed successfully.")
    print("=" * 65)


if __name__ == "__main__":
    main()
