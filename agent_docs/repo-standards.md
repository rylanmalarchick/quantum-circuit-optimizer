# Repository Standards

**Author:** Rylan Malarchick  
**Version:** 1.0  
**Last Updated:** December 2025  
**Scope:** Git workflow, commit conventions, and repository hygiene

---

## Related Documents

| Document | Purpose |
|----------|---------|
| [agent-coding-context.md](agent-coding-context.md) | Quick reference for AI agents |
| [research-code-principles.md](research-code-principles.md) | Philosophy and principles |
| [style-guide-reference.md](style-guide-reference.md) | Detailed style conventions |
| [repo-standards.md](repo-standards.md) | Git workflow and repository hygiene |

---

## Overview

This document defines repository standards for research projects. For coding principles and philosophy, see [research-code-principles.md](research-code-principles.md). For detailed style conventions, see [style-guide-reference.md](style-guide-reference.md).

---

## Git Workflow

### Branch Naming

| Branch Type | Pattern | Example |
|-------------|---------|---------|
| Main | `main` | `main` |
| Feature | `feat/<short-description>` | `feat/rotation-merge-pass` |
| Bug fix | `fix/<short-description>` | `fix/dag-cycle-detection` |
| Refactor | `refactor/<short-description>` | `refactor/raii-memory` |
| Documentation | `docs/<short-description>` | `docs/api-reference` |
| Experiment | `exp/<short-description>` | `exp/sabre-routing` |

### Commit Message Format (Conventional Commits)

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

**Types:**

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `style` | Formatting, no code change |
| `refactor` | Code change that neither fixes bug nor adds feature |
| `perf` | Performance improvement |
| `test` | Adding or correcting tests |
| `build` | Build system or dependencies |
| `ci` | CI/CD configuration |
| `chore` | Other changes (e.g., .gitignore) |

**Scope:** Component affected (e.g., `ir`, `passes`, `routing`, `grape`, `io`)

### Commit Examples

```
feat(ir): add Gate factory methods for common gates

Add static factory methods (x, h, cnot, rz) to Gate class.
This provides a cleaner API than direct construction.

Closes #42
```

```
fix(passes): handle zero-angle rotations in RotationMerge

Previously, merging Rz(θ) with Rz(-θ) could produce Rz(0),
which should be eliminated. Now correctly removes identity gates.
```

```
refactor(simulator): replace raw pointers with CudaMemory<T>

Convert NoisySimulator and BatchedSimulator to use RAII
memory management. Eliminates manual cudaFree calls.
```

```
docs: add SPDX license headers to all source files
```

```
test(dag): add tests for topological sort with cycles
```

---

## .gitignore Template

```gitignore
# Build artifacts
build/
cmake-build-*/
*.o
*.a
*.so
*.dylib

# IDE/Editor
.vscode/
.idea/
*.swp
*.swo
*~
.cache/
compile_commands.json

# Python
__pycache__/
*.py[cod]
*$py.class
*.egg-info/
dist/
.eggs/
*.egg
.mypy_cache/
.pytest_cache/
.ruff_cache/
.coverage
htmlcov/
venv/
.venv/

# Jupyter
.ipynb_checkpoints/
*.ipynb_checkpoints

# Data (keep small test fixtures, ignore large data)
*.hdf5
*.h5
!tests/fixtures/*.hdf5
*.npz
!tests/fixtures/*.npz

# Secrets (NEVER commit these)
.env
.env.*
*.pem
*.key
credentials.json
secrets.yaml
*_secret*
*.token
id_rsa*

# OS
.DS_Store
Thumbs.db

# Logs
*.log
logs/

# Temporary
tmp/
temp/
*.tmp
```

---

## README Structure

Every project should have a README with these sections:

```markdown
# Project Name

One-line description of what the project does.

## Overview

2-3 paragraph description including:
- What problem this solves
- Key features
- Target audience

## Installation

### Prerequisites
- Required software/libraries
- Version requirements

### Quick Start
```bash
# Clone, build, run example
```

## Usage

### Basic Example
```python
# Minimal working example
```

### API Reference
Link to detailed docs or inline documentation.

## Project Structure

```
project/
├── src/          # Source code
├── tests/        # Test suite
└── docs/         # Documentation
```

## Development

### Building
```bash
# Build commands
```

### Testing
```bash
# Test commands
```

### Code Style
Link to coding standards.

## Citation

If you use this work, please cite:
```bibtex
@software{...}
```

## License

MIT License - see LICENSE file.

## Contact

- Author: Name <email>
- Issues: GitHub Issues link
```

---

## LICENSE File

Use MIT License for all projects:

```
MIT License

Copyright (c) 2025 Rylan Malarchick

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Project Skeleton

New projects should start with this structure:

```
project-name/
├── .github/
│   └── workflows/
│       └── ci.yml
├── .gitignore
├── LICENSE
├── README.md
├── CMakeLists.txt / pyproject.toml
├── src/ or include/
│   └── (source files)
├── tests/
│   └── (test files)
├── docs/
│   └── (documentation)
└── examples/
    └── (usage examples)
```

### Bootstrap Checklist

- [ ] Create repository with LICENSE, README, .gitignore
- [ ] Set up build system (CMakeLists.txt or pyproject.toml)
- [ ] Configure CI/CD (.github/workflows/ci.yml)
- [ ] Set up testing framework (GoogleTest or pytest)
- [ ] Create test directory with example test
- [ ] Configure linting (clang-format, ruff)
- [ ] Configure type checking (mypy)
- [ ] Enable compiler warnings (`-Wall -Wextra -Werror`)
- [ ] Pin dependencies (requirements.txt or lock file)

---

## Secrets Management

### Never Commit Secrets

The following should **never** be committed to version control:

| File Pattern | Description |
|--------------|-------------|
| `.env` | Environment variables |
| `.env.*` | Environment variants (.env.local, .env.prod) |
| `*.pem`, `*.key` | Private keys |
| `credentials.json` | API credentials |
| `secrets.yaml` | Secret configuration |
| `*_secret*` | Any file with "secret" in name |
| `*.token` | API tokens |
| `id_rsa*` | SSH keys |

### Pre-commit Hook

Add to `.pre-commit-config.yaml`:

```yaml
repos:
  - repo: https://github.com/Yelp/detect-secrets
    rev: v1.4.0
    hooks:
      - id: detect-secrets
        args: ['--baseline', '.secrets.baseline']
```

---

**License:** MIT

**Author:** Rylan Malarchick (rylan1012@gmail.com)
