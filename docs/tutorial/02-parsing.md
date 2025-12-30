# Tutorial: Parsing OpenQASM {#tutorial-parsing}

This tutorial covers parsing OpenQASM 3.0 files into circuits.

## Basic Parsing

The simplest way to parse OpenQASM:

```cpp
#include "parser/Parser.hpp"

using namespace qopt;

// Parse from a string
Circuit circuit = parseQASM(R"(
    OPENQASM 3.0;
    qubit[2] q;
    h q[0];
    cx q[0], q[1];
)");

std::cout << "Parsed " << circuit.size() << " gates\n";  // 2 gates
```

## Parsing from Files

```cpp
#include "parser/Parser.hpp"
#include <fstream>

using namespace qopt;

// Read file contents
std::ifstream file("circuit.qasm");
std::stringstream buffer;
buffer << file.rdbuf();
std::string source = buffer.str();

// Parse
Circuit circuit = parseQASM(source);
```

## Using the Parser Class

For more control and error handling:

```cpp
#include "parser/Parser.hpp"

using namespace qopt;

std::string source = R"(
    OPENQASM 3.0;
    qubit[3] q;
    h q[0];
    cx q[0], q[1];
    cx q[1], q[2];
)";

Parser parser(source);
Circuit circuit = parser.parse();

// Check for errors
if (parser.hasErrors()) {
    for (const auto& error : parser.errors()) {
        std::cerr << "Line " << error.line << ", Col " << error.column
                  << ": " << error.message << "\n";
    }
}
```

## Supported OpenQASM 3.0 Constructs

### Version Declaration

```qasm
OPENQASM 3.0;
```

The version declaration is required and must be the first statement.

### Qubit Declarations

```qasm
qubit[4] q;        // Declare 4 qubits named 'q'
qubit[2] ancilla;  // Declare 2 ancilla qubits
```

### Classical Bit Declarations

```qasm
bit[4] c;          // Declare 4 classical bits
```

### Standard Gates

```qasm
// Single-qubit gates
h q[0];            // Hadamard
x q[1];            // Pauli-X (NOT)
y q[2];            // Pauli-Y
z q[0];            // Pauli-Z
s q[0];            // S gate (√Z)
sdg q[0];          // S† gate
t q[0];            // T gate
tdg q[0];          // T† gate

// Rotation gates
rx(pi/2) q[0];     // X-axis rotation
ry(pi/4) q[1];     // Y-axis rotation
rz(pi/8) q[2];     // Z-axis rotation

// Two-qubit gates
cx q[0], q[1];     // CNOT (controlled-X)
cz q[0], q[1];     // Controlled-Z
swap q[0], q[1];   // SWAP gate
```

### Angle Expressions

```qasm
rz(pi) q[0];           // π
rz(pi/2) q[0];         // π/2
rz(pi/4) q[0];         // π/4
rz(2*pi) q[0];         // 2π
rz(-pi/8) q[0];        // -π/8
rz(0.5) q[0];          // Numeric literal
rz(1.5707963) q[0];    // Numeric literal
```

### Measurements

```qasm
bit[2] c;
qubit[2] q;
c[0] = measure q[0];   // Measure q[0] into c[0]
c[1] = measure q[1];   // Measure q[1] into c[1]
```

### Comments

```qasm
// This is a single-line comment
h q[0];  // Comment after statement

/* This is a
   multi-line comment */
```

## Complete Example

```qasm
// Quantum Teleportation Circuit
OPENQASM 3.0;

qubit[3] q;
bit[2] c;

// Prepare Bell pair between q[1] and q[2]
h q[1];
cx q[1], q[2];

// Alice's operations on q[0] and q[1]
cx q[0], q[1];
h q[0];

// Measure Alice's qubits
c[0] = measure q[0];
c[1] = measure q[1];

// Bob's corrections would be classical-controlled
// (not yet supported in this parser version)
```

## Error Handling

The parser provides detailed error messages:

```cpp
std::string bad_source = R"(
    OPENQASM 3.0;
    qubit[2] q;
    h q[5];        // Error: qubit index out of range
    unknowngate;   // Error: unknown gate
)";

Parser parser(bad_source);
Circuit circuit = parser.parse();

if (parser.hasErrors()) {
    for (const auto& error : parser.errors()) {
        std::cerr << "Error at line " << error.line 
                  << ", column " << error.column << ": "
                  << error.message << "\n";
    }
}
```

## Working with the Lexer

For advanced use cases, you can use the Lexer directly:

```cpp
#include "parser/Lexer.hpp"

using namespace qopt;

std::string source = "h q[0]; cx q[0], q[1];";
Lexer lexer(source);

std::vector<Token> tokens = lexer.tokenize();

for (const Token& token : tokens) {
    std::cout << "Type: " << static_cast<int>(token.type)
              << " Value: '" << token.value << "'"
              << " Line: " << token.line
              << " Column: " << token.column << "\n";
}
```

### Token Types

| Token Type | Examples |
|------------|----------|
| OPENQASM | `OPENQASM` |
| QUBIT | `qubit` |
| BIT | `bit` |
| MEASURE | `measure` |
| IDENTIFIER | `q`, `myqubit` |
| INTEGER | `0`, `42` |
| FLOAT | `3.14`, `1e-3` |
| PI | `pi` |
| GATE_H, GATE_X, etc. | `h`, `x`, `cx` |
| LBRACKET, RBRACKET | `[`, `]` |
| LPAREN, RPAREN | `(`, `)` |
| SEMICOLON | `;` |
| COMMA | `,` |
| EQUALS | `=` |
| PLUS, MINUS, STAR, SLASH | `+`, `-`, `*`, `/` |

## Parsing to DAG

You can parse directly to a DAG for optimization:

```cpp
#include "parser/Parser.hpp"
#include "ir/DAG.hpp"

using namespace qopt;

std::string source = R"(
    OPENQASM 3.0;
    qubit[2] q;
    h q[0];
    cx q[0], q[1];
)";

Parser parser(source);
Circuit circuit = parser.parse();

// Convert to DAG
DAG dag = DAG::fromCircuit(circuit);

std::cout << "DAG has " << dag.numNodes() << " nodes\n";
```

## Limitations

The current parser supports a subset of OpenQASM 3.0:

**Supported:**
- Version declaration
- Qubit and bit register declarations
- Standard gates (h, x, y, z, s, sdg, t, tdg, rx, ry, rz, cx, cz, swap)
- Measurement operations
- Basic angle expressions with pi

**Not Yet Supported:**
- Gate definitions (`gate mygate(a, b) q { ... }`)
- Classical control (`if (c == 1) x q[0];`)
- Loops (`for`, `while`)
- Subroutines
- Include statements
- Arrays and complex expressions

## Next Steps

- @ref tutorial-circuits "Tutorial: Working with Circuits" - Programmatic circuit creation
- @ref tutorial-optimization "Tutorial: Optimization Passes" - Optimize parsed circuits
- @ref tutorial-routing "Tutorial: Qubit Routing" - Route to hardware topology
