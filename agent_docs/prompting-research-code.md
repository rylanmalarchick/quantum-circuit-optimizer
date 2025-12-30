# Prompting Research Code: How to Pair with Claude/ChatGPT

**Author:** Rylan Malarchick  
**Version:** 1.0  
**Last Updated:** December 2025

This guide synthesizes:
1. **Research Code Principles** (this repo): Why good code matters
2. **Prompt Engineering Best Practices** (Google, Feb 2025): How to get AI to write better code

Together: A complete system for AI-assisted research software development.

---

## The Problem

When you ask Claude "write a rotation gate," you get code that compiles. But:
- No validation of rotation angle
- No tests against known matrices
- No citation of Nielsen & Chuang
- No edge case handling

**Why?** Because Claude optimizes for "code that runs," not "code that's correct."

---

## Mapping Prompting Techniques to Research Principles

| Principle | Technique | Use Case |
|-----------|-----------|----------|
| **Correctness First** | Few-Shot with examples | Provide correct examples before asking |
| **Specification Before Code** | Chain of Thought (CoT) | "Think step by step: what edge cases exist?" |
| **Fail Fast** | System Prompt + Role | Set role: "Write research code that validates all inputs" |
| **Simplicity** | Few-Shot role examples | Show short functions, ask for same pattern |
| **Infrastructure** | CoT for test-first | "Write tests first, then code" |

---

## Pattern 1: Test-First Code (Chain of Thought)

**Bad Prompt:**
```
Write a function to compute quantum fidelity between two unitaries.
```

**Good Prompt:**
```
You are implementing a quantum fidelity function following research-code-principles.

Think step by step:
1. What is the mathematical definition? (Cite source)
2. What inputs must be validated? (Shapes, unitarity, NaN)
3. What edge cases must tests cover? (Identity, single qubit, max size)
4. What physical constraints must hold? (F in [0, 1])
5. What numerical stability issues exist?

After thinking:
1. Write test cases (at least 6: happy path + 5 edge cases)
2. Then the implementation
3. Then validation code

Cite Nielsen & Chuang for the definition.
```

**Why it works:** CoT forces the model to reason about correctness before writing code. The explicit request for citations and edge cases prevents the "optimistic code" failure mode.

---

## Pattern 2: Correctness Through Examples (Few-Shot)

**Bad Prompt:**
```
Implement a Pauli X gate.
```

**Good Prompt:**
```
Here are examples of correct gate implementations:

Example 1: Pauli Z Gate
```python
def z_gate() -> np.ndarray:
    """Pauli Z gate: |0⟩ → |0⟩, |1⟩ → -|1⟩.
    
    Matrix representation from Nielsen & Chuang, Eq. 4.2:
    Z = [[1, 0], [0, -1]]
    
    Returns:
        2x2 unitary matrix.
    """
    return np.array([[1, 0], [0, -1]], dtype=complex)

# Test: Z² = I
assert np.allclose(z_gate() @ z_gate(), np.eye(2))
```

Example 2: Hadamard Gate
```python
def h_gate() -> np.ndarray:
    """Hadamard gate: creates equal superposition.
    
    Matrix representation from Nielsen & Chuang, Eq. 4.3:
    H = (1/√2) [[1, 1], [1, -1]]
    
    Returns:
        2x2 unitary matrix.
    """
    return np.array([[1, 1], [1, -1]], dtype=complex) / np.sqrt(2)

# Test: H² = I
assert np.allclose(h_gate() @ h_gate(), np.eye(2))
```

Now implement Pauli X gate following the same pattern:
1. Definition comment with source citation
2. Test that X² = I
3. Validate against ground truth matrix
```

**Why it works:** Few-shot provides a concrete template. The model learns the pattern (docstring → citation → matrix → test) and applies it consistently.

---

## Pattern 3: Enforcing Validation (System Prompt)

**Bad Prompt:**
```
Write a circuit optimization function.
```

**Good Prompt:**
```
You are a research software engineer implementing quantum circuit optimization.
Every function must validate all inputs at boundaries and fail fast.

Task: Write an optimization function that accepts:
- Input circuit (must be non-null, valid gates)
- Number of iterations (must be > 0)
- Tolerance (must be in (0, 1))

Requirements:
1. Validate all inputs immediately, throw descriptive exceptions
2. Include at least 3 test cases for invalid inputs
3. Error messages must include: what went wrong, what was expected, where

Example of correct validation:
```python
if tolerance <= 0.0 or tolerance >= 1.0:
    raise ValueError(
        f"Tolerance must be in (0, 1), got {tolerance}"
    )
```

Now implement the function with full validation.
```

**Why it works:** The system prompt establishes the "fail fast" expectation. The concrete example shows the error message format. The explicit requirement for invalid input tests prevents the "optimistic code" failure.

---

## Pattern 4: Simplicity Through Constraints

**Bad Prompt:**
```
Write an optimization loop.
```

**Good Prompt:**
```
Functions must be ≤50 lines (Python), ≤60 lines (C++).

Example of correct breakdown:
```python
def optimize_circuit(circuit, max_iterations):
    """High-level optimization loop."""
    state = _initialize_state(circuit)
    
    for _ in range(max_iterations):
        state = _update_state(state, circuit)
        if _converged(state):
            break
    
    return _finalize_results(state)

def _initialize_state(circuit):
    """Set up initial optimization state."""
    # 10 lines

def _update_state(state, circuit):
    """Single iteration of optimization."""
    # 15 lines
```

Now write an optimization function following this pattern.
Break complex logic into helper functions. Each function does ONE thing.
```

**Why it works:** The line limit constraint is explicit. The example shows the decomposition pattern. The model learns to break down rather than monolith.

---

## Temperature Settings

| Task | Temperature | Rationale |
|------|-------------|-----------|
| **Implementation** | 0.0-0.1 | Deterministic, correct code |
| **Test generation** | 0.1-0.2 | Slight variation for edge cases |
| **Edge case exploration** | 0.3-0.5 | Creative thinking about failures |
| **Never for research code** | > 0.8 | Too random, introduces errors |

---

## Quick Templates

### Template 1: Test-First Implementation

```
You are implementing [function] for research software.

Think step by step:
1. What is the spec? (cite source)
2. What edge cases exist?
3. What physical constraints apply?

Then:
1. Write ≥5 test cases (happy path + edge cases)
2. Implement the function
3. Show validation against ground truth

Temperature: 0.1
```

### Template 2: Validation-Heavy Code

```
Every input must be validated at function boundaries.

Example:
```python
if condition_invalid:
    raise ValueError(
        f"Parameter X must satisfy constraint, got {value}"
    )
```

Now implement [function] with comprehensive input validation.
Fail fast. Include ≥3 test cases for invalid inputs.

Temperature: 0.1
```

### Template 3: Simplicity-First Code

```
Functions must be ≤50 lines (Python), ≤60 lines (C++).
Each function does ONE thing.

If [task] is complex, break into:
1. _helper1: [focused subtask]
2. _helper2: [focused subtask]
3. main_function: orchestrates helpers

Now implement [task] following this pattern.

Temperature: 0.1
```

### Template 4: Physics Validation

```
You are implementing [physical calculation].

Physical constraints that MUST hold:
- [constraint 1, e.g., unitarity: U†U = I]
- [constraint 2, e.g., normalization: sum to 1]
- [constraint 3, e.g., bounds: value in [0, 1]]

Include validation code that checks these constraints.
Include tests that verify constraint violations are caught.

Cite source for the mathematical definition.

Temperature: 0.1
```

---

## Common Issues and Fixes

| Problem | Symptom | Fix |
|---------|---------|-----|
| Agent skips tests | "Here's the implementation..." | Use CoT: "Write tests first. Think step by step." |
| No error handling | Assumes valid input | System prompt: "Every function validates inputs" |
| Magic numbers | `tolerance = 1e-10` | Few-shot with constants defined and commented |
| Missing citations | Algorithm with no source | Few-shot: `# From Nielsen & Chuang, Eq 2.X` |
| Functions too long | 100+ lines | Constraint: "≤50 lines, break into helpers" |
| Numerical issues | Subtracts nearly-equal numbers | CoT: "What numerical stability issues exist?" |
| No edge cases | Only happy path tested | Explicit: "Test empty, single, max, zero, invalid" |

---

## Session Log Template

Track your prompting experiments:

```markdown
## Session: [Date] [Task]

### Attempt 1
- **Temperature:** 0.1
- **Prompt:** [exact prompt]
- **Result:** [what worked/didn't]
- **Issues:** [hallucinations, missing validation, etc.]

### Attempt 2
- **Temperature:** 0.1  
- **Prompt:** [refined]
- **Result:** [better? worse?]

### Takeaway
[What made it better]
```

---

## See Also

- `examples/quantum-gate-example/prompting-log.md` — Real prompting session
- `agent-coding-context.md` — Paste into AI for coding sessions
- `research-code-principles.md` — The philosophy behind these patterns
