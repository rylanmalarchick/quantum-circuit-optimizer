# Building from Source {#building}

This guide explains how to build the Quantum Circuit Optimizer from source.

## Prerequisites

- **C++17 compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: Version 3.18 or later
- **Git**: For cloning the repository

GoogleTest is automatically fetched during the build process.

## Quick Build

```bash
# Clone the repository
git clone https://github.com/username/quantum-circuit-optimizer.git
cd quantum-circuit-optimizer

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
ctest --test-dir build
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type: Debug, Release, RelWithDebInfo |
| `BUILD_TESTING` | ON | Build the test suite |
| `BUILD_DOCS` | OFF | Build Doxygen documentation |
| `BUILD_BENCHMARKS` | OFF | Build benchmark suite |
| `BUILD_EXAMPLES` | ON | Build example programs |

### Debug Build

For development with debug symbols and assertions:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Building Documentation

Requires Doxygen and optionally Graphviz for diagrams:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install doxygen graphviz

# Build with documentation
cmake -B build -DBUILD_DOCS=ON
cmake --build build --target docs
```

Documentation will be generated in `docs/api/html/`.

### Building Benchmarks

```bash
cmake -B build -DBUILD_BENCHMARKS=ON
cmake --build build
./build/benchmarks/benchmark_optimization
```

## Compiler Flags

The project is compiled with strict warning flags:

- `-Wall -Wextra -Wpedantic -Werror` (GCC/Clang)
- `/W4 /WX` (MSVC)

This ensures high code quality and catches potential issues early.

## Running Tests

```bash
# Run all tests
ctest --test-dir build

# Run with verbose output
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R "GateTest"

# Run tests in parallel
ctest --test-dir build -j$(nproc)
```

### Test Categories

| Test Target | Description | Count |
|-------------|-------------|-------|
| test_gate | Gate creation and operations | 26 |
| test_circuit | Circuit manipulation | 24 |
| test_dag | DAG IR operations | 54 |
| test_lexer | Tokenization | 58 |
| test_parser | Parsing | 51 |
| test_passes | Optimization passes | 64 |
| test_routing | Topology and routing | 63 |
| **Total** | | **340** |

## Installation

```bash
# Install to system (requires sudo)
cmake --install build

# Install to custom prefix
cmake --install build --prefix /path/to/install
```

This installs:
- Headers to `<prefix>/include/`
- Library to `<prefix>/lib/`
- CMake config to `<prefix>/lib/cmake/quantum-circuit-optimizer/`

## Using in Your Project

After installation, you can use the library in your CMake project:

```cmake
find_package(quantum-circuit-optimizer REQUIRED)
target_link_libraries(your_target PRIVATE quantum-circuit-optimizer::qopt)
```

Or use FetchContent to include directly:

```cmake
include(FetchContent)
FetchContent_Declare(
    quantum-circuit-optimizer
    GIT_REPOSITORY https://github.com/username/quantum-circuit-optimizer.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(quantum-circuit-optimizer)
target_link_libraries(your_target PRIVATE quantum-circuit-optimizer::qopt)
```

## Troubleshooting

### CMake version too old

```
CMake Error at CMakeLists.txt:1 (cmake_minimum_required):
  CMake 3.18 or higher is required.
```

**Solution**: Update CMake or use `cmake3` if available.

### Compiler doesn't support C++17

```
error: 'string_view' is not a member of 'std'
```

**Solution**: Use a newer compiler (GCC 9+, Clang 10+, MSVC 2019+).

### Tests fail to link

```
undefined reference to `testing::internal::...'
```

**Solution**: Ensure GoogleTest was properly fetched. Try deleting `build/` and rebuilding.

## Platform Support

| Platform | Compiler | Status |
|----------|----------|--------|
| Linux | GCC 9+ | Fully supported |
| Linux | Clang 10+ | Fully supported |
| macOS | Apple Clang 12+ | Fully supported |
| Windows | MSVC 2019+ | Fully supported |
| Windows | MinGW-w64 | Experimental |
