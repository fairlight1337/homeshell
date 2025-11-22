# Homeshell Build Targets

This document describes the various build targets available in Homeshell.

## Standard Build Targets

### Build Everything
```bash
cd build
make -j$(nproc)
```

### Build and Run Tests
```bash
cd build
make -j$(nproc)
make test
# Or with verbose output:
ctest --output-on-failure
```

### Clean Build
```bash
cd build
make clean
# Or for a complete rebuild:
rm -rf *
cmake ..
make -j$(nproc)
```

## Documentation Targets

### Generate HTML Documentation (make docs)

Generates comprehensive HTML documentation using Doxygen.

**Prerequisites:**
- Doxygen must be installed: `sudo apt-get install doxygen`

**Usage:**
```bash
cd build
make docs
```

**Output:**
- HTML documentation: `build/docs/html/index.html`

**View Documentation:**
```bash
firefox build/docs/html/index.html
# or
xdg-open build/docs/html/index.html
```

**Features:**
- Full API documentation from Doxygen comments
- Class hierarchy diagrams
- Call graphs (if Graphviz dot is installed)
- Source code browsing
- Search functionality
- README.md as main page

---

### Check Documentation Coverage (make docs-coverage)

Identifies classes, functions, and parameters that lack Doxygen documentation.

**Prerequisites:**
- Doxygen must be installed

**Usage:**
```bash
cd build
make docs-coverage
```

**Output:**
- Console output with warnings about undocumented items
- Text report: `build/docs-coverage/undocumented.txt`

**Example Output:**
```
=== Undocumented Items Report ===
/path/to/file.hpp:42: warning: Compound MyClass is not documented.
/path/to/file.hpp:50: warning: Member myFunction() is not documented.
/path/to/file.hpp:55: warning: parameter 'arg1' not documented

Report saved to: build/docs-coverage/undocumented.txt
```

**Use Cases:**
- Track documentation progress
- Identify missing documentation before releases
- Ensure consistent documentation coverage
- Find undocumented public APIs

---

## Code Coverage Targets

### Generate Code Coverage Report (make coverage)

Generates detailed code coverage reports showing which lines of code are executed by the test suite.

**Prerequisites:**
- lcov must be installed: `sudo apt-get install lcov`
- Project must be built with coverage enabled

**Setup:**
```bash
# 1. Configure with coverage enabled
cd build
cmake -DENABLE_COVERAGE=ON ..

# 2. Build with coverage instrumentation
make -j$(nproc)

# 3. Generate coverage report
make coverage
```

**Output:**
- Text summary: `build/coverage/coverage_summary.txt`
- HTML report: `build/coverage/html/index.html`
- Raw coverage data: `build/coverage/coverage.info`
- Filtered coverage data: `build/coverage/coverage_filtered.info`

**View Coverage Report:**
```bash
firefox build/coverage/html/index.html
# or
xdg-open build/coverage/html/index.html
```

**Coverage Report Features:**
- Line-by-line coverage visualization
- Function coverage statistics
- Branch coverage (if available)
- File-level coverage percentages
- Directory-level summaries
- Sortable tables
- Color-coded coverage (green = covered, red = not covered)

**Coverage Exclusions:**
The coverage report automatically excludes:
- System headers (`/usr/*`)
- External libraries (`*/external/*`)
- Test files (`*/tests/*`)
- Build artifacts (`*/build/*`)

**Interpreting Results:**
- **Line Coverage**: Percentage of executable lines that were run
- **Function Coverage**: Percentage of functions that were called
- **Branch Coverage**: Percentage of conditional branches taken

**Example Text Output:**
```
=== Coverage Summary ===
Overall coverage rate:
  lines......: 85.2% (1234 of 1448 lines)
  functions..: 92.3% (245 of 265 functions)
```

**Workflow Tips:**
1. Write tests for uncovered code
2. Run `make coverage` to verify improvements
3. Aim for >80% line coverage for critical code
4. Focus on testing error paths and edge cases

---

## Code Quality Targets

### Format Code (make format)

Automatically formats all source files according to the project's style guide.

**Prerequisites:**
- clang-format must be installed

**Usage:**
```bash
cd build
make format
```

**What it does:**
- Formats all `.cpp`, `.hpp`, `.c`, `.h` files
- Uses the configuration in `config/.clang-format`
- Applies formatting in-place

---

## CI/CD Integration

### GitHub Actions / GitLab CI

Example workflow for documentation and coverage:

```yaml
name: CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
      with:
        submodules: recursive
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          doxygen \
          lcov \
          libsqlcipher-dev \
          libncurses-dev \
          libtinfo-dev
    
    - name: Build with coverage
      run: |
        mkdir build && cd build
        cmake -DENABLE_COVERAGE=ON ..
        make -j$(nproc)
    
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
    
    - name: Generate coverage
      run: |
        cd build
        make coverage
    
    - name: Generate documentation
      run: |
        cd build
        make docs
    
    - name: Check documentation coverage
      run: |
        cd build
        make docs-coverage
    
    - name: Upload coverage to Codecov
      uses: codecov/codecov-action@v2
      with:
        files: build/coverage/coverage_filtered.info
```

---

## Troubleshooting

### Doxygen not found
```bash
sudo apt-get install doxygen graphviz
```

### lcov/genhtml not found
```bash
sudo apt-get install lcov
```

### Coverage shows 0%
Make sure you:
1. Configured with `-DENABLE_COVERAGE=ON`
2. Rebuilt after configuring
3. Tests are actually running (check test output)

### Documentation warnings are excessive
This is normal during development. Use `make docs-coverage` to track progress systematically.

### HTML reports not opening
Try different browsers or check file permissions:
```bash
chmod -R +r build/docs/html
chmod -R +r build/coverage/html
```

---

## Performance Tips

### Parallel Builds
Always use `-j` flag with make:
```bash
make -j$(nproc)  # Uses all CPU cores
```

### Incremental Documentation
Doxygen supports incremental builds. Only changed files are re-documented.

### Coverage Build Performance
Coverage builds are slower due to instrumentation. Use regular builds for development:
```bash
# Development (fast)
cmake ..
make -j$(nproc)

# Coverage (slower, for testing)
cmake -DENABLE_COVERAGE=ON ..
make -j$(nproc)
make coverage
```

---

## Summary of All Targets

| Target | Purpose | Prerequisites | Output Location |
|--------|---------|---------------|-----------------|
| `make` | Build all binaries | - | `build/` |
| `make test` | Run unit tests | - | Console |
| `make docs` | Generate documentation | doxygen | `build/docs/html/` |
| `make docs-coverage` | Check doc coverage | doxygen | `build/docs-coverage/` |
| `make coverage` | Generate coverage report | lcov, `-DENABLE_COVERAGE=ON` | `build/coverage/` |
| `make format` | Format source code | clang-format | In-place |
| `make clean` | Clean build artifacts | - | - |

---

## Best Practices

1. **Before committing:**
   ```bash
   make format
   make test
   ```

2. **Before releasing:**
   ```bash
   make docs
   make docs-coverage  # Ensure <10 warnings
   make coverage       # Ensure >80% coverage
   ```

3. **During development:**
   - Run tests frequently
   - Check documentation as you write code
   - Use coverage reports to guide test writing

4. **For contributors:**
   - New features must include tests
   - Public APIs must be documented
   - Maintain or improve coverage percentage

