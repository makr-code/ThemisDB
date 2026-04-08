---
category: "🛠️ Developer/Technical"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🛠️ Code Quality Pipeline

Comprehensive code quality and testing infrastructure.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Quick Start](#-quick-start)
- [📖 Quality Checks](#-quality-checks)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB uses a comprehensive code quality pipeline to ensure high standards across the codebase.

**Stand:** 6. April 2026  
**Version:** 1.3.0  
**Kategorie:** 🛠️ Developer/Technical

---

## ✨ Features

- 🔍 **Static Analysis** - clang-tidy for C++17 best practices
- 🧹 **Linting** - cppcheck for additional validation
- 📊 **Coverage** - gcov/lcov code coverage measurement
- 🔐 **Secret Scanning** - Gitleaks prevents credential leaks

---

## 🚀 Quick Start

### Local Testing

**Linux/macOS:**
```bash
# Run all checks
./scripts/check-quality.sh

# Skip specific checks
./scripts/check-quality.sh --skip-tidy --skip-tests

# Auto-fix issues
./scripts/check-quality.sh --fix
```

**Windows:**
```powershell
# Run all checks
.\scripts\check-quality.ps1

# Skip specific checks
.\scripts\check-quality.ps1 -SkipTidy -SkipTests

# Auto-fix issues
.\scripts\check-quality.ps1 -Fix
```

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install -y \
  cmake \
  ninja-build \
  clang-tidy \
  clang-tools \
  cppcheck \
  lcov \
  gcovr

# Install gitleaks
wget https://github.com/gitleaks/gitleaks/releases/download/v8.18.4/gitleaks_8.18.4_linux_x64.tar.gz
tar -xzf gitleaks_8.18.4_linux_x64.tar.gz
sudo mv gitleaks /usr/local/bin/
```

**macOS:**
```bash
brew install cmake ninja llvm cppcheck lcov gitleaks
```

**Windows:**
```powershell
# Using Chocolatey
choco install cmake llvm cppcheck gitleaks

# Using Scoop
scoop install cmake llvm cppcheck gitleaks
```

## GitHub Actions Workflows

### Code Quality Workflow

**File:** `.github/workflows/code-quality.yml`

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`

**Jobs:**

1. **clang-tidy**: Static analysis with modern C++ checks
   - Runs on Ubuntu latest
   - Uses `.clang-tidy` configuration
  - Generates `logs/clang-tidy-report.txt`
   - Uploads artifact for review

2. **cppcheck**: C++ linting
   - Runs on Ubuntu latest
   - Uses `.cppcheck-suppressions` for known false positives
   - Generates XML and text reports
   - Uploads artifacts

3. **coverage**: Code coverage analysis
   - Builds with `--coverage` flags
   - Runs full test suite
   - Generates lcov reports and HTML output
   - Comments on PRs with coverage summary
   - Uploads coverage artifacts

4. **gitleaks**: Secret scanning
   - Scans full repository history
   - Uses `.gitleaks.toml` configuration
   - **Fails build** if secrets detected
   - Uploads report for review

5. **quality-summary**: Aggregates all job results
   - Fails if gitleaks finds secrets
   - Reports overall status

### Coverage Badge Workflow

**File:** `.github/workflows/coverage-badge.yml`

**Triggers:**
- Push to `main` branch
- Manual workflow dispatch

**Purpose:**
- Generates coverage percentage badge
- Deploys HTML coverage report to GitHub Pages
- Updates coverage badge in README

**Setup Required:**
1. Create a GitHub Gist for badge storage
2. Add `GIST_SECRET` to repository secrets (Personal Access Token with `gist` scope)
3. Update `gistID` in workflow file
4. Enable GitHub Pages in repository settings

## Configuration Files

### `.clang-tidy`

Configures clang-tidy checks for modern C++17:

```yaml
Checks: >
  bugprone-*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  concurrency-*
```

**Disabled checks:**
- `modernize-use-trailing-return-type`: Unnecessary for our style
- `readability-magic-numbers`: Too noisy, use sparingly
- `cppcoreguidelines-pro-bounds-pointer-arithmetic`: RocksDB integration requires this

**Naming conventions:**
- Namespaces: `lower_case`
- Classes/Structs: `CamelCase`
- Functions: `camelCase`
- Variables: `lower_case`
- Private members: `lower_case_`
- Constants: `UPPER_CASE`

### `.cppcheck-suppressions`

Suppresses known false positives:

```
# System headers
missingIncludeSystem
unmatchedSuppression

# Third-party code
*:vcpkg_installed/*
*:*/crow/*
*:*/rocksdb/*

# Test files (more flexible)
unusedFunction:tests/*
```

### `.gitleaks.toml`

Configures secret detection rules:

**Custom rules:**
- ThemisDB API keys
- Database connection strings with credentials
- JWT secrets
- Encryption keys
- AWS credentials
- GitHub tokens
- Slack tokens

**Allowlists:**
- Test files: `tests/**`
- Documentation: `docs/**`
- Example configs: `*.example.*`, `*.template.*`

## CI Integration

### Pull Request Checks

When you open a PR, the code-quality workflow runs automatically:

1. **clang-tidy** analyzes code for bugs and style issues
2. **cppcheck** performs additional linting
3. **coverage** measures test coverage and comments on PR
4. **gitleaks** scans for secrets (blocks merge if found)

### Required Status Checks

Configure branch protection rules to require:
- `clang-tidy` (recommended)
- `cppcheck` (recommended)
- `coverage` (optional, for metrics)
- **`gitleaks`** (mandatory, blocks secrets)

### Artifacts

Each workflow uploads artifacts:
- **clang-tidy-report** (30 days retention)
- **cppcheck-xml-report** (30 days)
- **cppcheck-text-report** (30 days)
- **coverage-reports** (30 days, includes HTML)
- **gitleaks-report** (30 days, JSON + summary)

Download from GitHub Actions UI: Actions → Workflow Run → Artifacts

## Coverage Reporting

### Viewing Coverage

**Local:**
```bash
# Generate coverage locally
./scripts/check-quality.sh

# Generate HTML report
mkdir -p coverage
lcov --capture --directory build --output-file coverage/coverage.info --rc lcov_branch_coverage=1
lcov --remove coverage/coverage.info '/usr/*' '*/vcpkg_installed/*' '*/tests/*' \
  --output-file coverage/coverage-filtered.info --rc lcov_branch_coverage=1
genhtml coverage/coverage-filtered.info --output-directory coverage/html

# Open in browser
xdg-open coverage/html/index.html  # Linux
open coverage/html/index.html      # macOS
start coverage/html/index.html     # Windows
```

**GitHub Actions:**
- Coverage report comments on PRs
- HTML report deployed to GitHub Pages: `https://<org>.github.io/<repo>/coverage/`
- Badge in README (after setup)

### Coverage Goals

- **Overall:** Target 80%+ line coverage
- **Critical paths:** 90%+ coverage
  - Storage engine
  - Transaction logic
  - Query engine
- **Nice-to-have:** 70%+ coverage
  - HTTP handlers
  - Utility functions

## Best Practices

### Before Committing

1. **Run local checks:**
   ```bash
   ./scripts/check-quality.sh
   ```

2. **Fix clang-tidy warnings:**
   ```bash
   ./scripts/check-quality.sh --fix
   ```

3. **Review cppcheck output:**
   - Suppress known false positives in `.cppcheck-suppressions`
   - Use `// cppcheck-suppress <error_id>` inline for one-off cases

4. **Check coverage:**
   - Add tests for new code
   - Aim for >80% coverage on modified files

5. **Scan for secrets:**
   - Review gitleaks output carefully
   - **Never commit** real API keys or passwords
   - Use `.env.example` for templates

### Handling False Positives

**clang-tidy:**
```cpp
// Disable specific check for one line
// NOLINTNEXTLINE(check-name)
auto ptr = reinterpret_cast<void*>(addr);

// Disable for block
// NOLINTBEGIN(check-name)
// ... code ...
// NOLINTEND(check-name)
```

**cppcheck:**
```cpp
// Inline suppression
// cppcheck-suppress unusedFunction
void helperFunction() { }
```

**gitleaks:**
Add to `.gitleaks.toml` allowlist:
```toml
[rules.allowlist]
paths = [
  '''tests/fixtures/test_keys.json'''
]
regexes = [
  '''(?i)test[-_]?api[-_]?key'''
]
```

## Troubleshooting

### Issue: clang-tidy too slow

**Solution:**
```bash
# Run on changed files only
git diff --name-only main | grep -E '\.(cpp|h)$' | \
  xargs clang-tidy -p build
```

### Issue: cppcheck false positives

**Solution:**
Add to `.cppcheck-suppressions`:
```
specificError:path/to/file.cpp:123
```

### Issue: Gitleaks flagging test data

**Solution:**
Update `.gitleaks.toml`:
```toml
[allowlist]
paths = [
  '''tests/test_data/.*'''
]
```

### Issue: Coverage report incomplete

**Solution:**
Ensure all tests run before generating report:
```bash
cd build
ctest --output-on-failure
cd ..
lcov --capture --directory build ...
```

## Resources

- [Clang-Tidy Checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [Cppcheck Manual](http://cppcheck.sourceforge.net/manual.pdf)
- [LCOV Documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- [Gitleaks Configuration](https://github.com/gitleaks/gitleaks#configuration)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)

## Metrics Dashboard (Future)

Planned integrations:
- **SonarQube**: Comprehensive code quality dashboard
- **Codecov**: Advanced coverage tracking
- **CodeClimate**: Maintainability scoring
- **Snyk**: Dependency vulnerability scanning

## 🔧 Unused Variable Management

### Overview

ThemisDB enforces strict unused variable checks to maintain code quality and prevent potential bugs. We use C++17's `[[maybe_unused]]` attribute instead of pragma directives or (void) casts.

### Compiler Warnings

**MSVC:**
- `C4100`: Unreferenced formal parameter
- `C4101`: Unreferenced local variable  
- `C4189`: Local variable initialized but not referenced
- `C4505`: Unreferenced local function

**GCC/Clang:**
- `-Wunused-variable`: Unused local variables
- `-Wunused-parameter`: Unused function parameters
- `-Wunused-but-set-variable`: Variables set but never read

### Best Practices

#### 1. Remove Unused Variables

**Before:**
```cpp
void process() {
    int unused_counter = 0;  // ⚠️ C4101 warning
    std::string unused_name = "test";
    // ... code without using these variables
}
```

**After:**
```cpp
void process() {
    // Variables removed
}
```

#### 2. Mark Conditionally Used Parameters

Use `[[maybe_unused]]` for parameters used only in certain builds:

**Before:**
```cpp
void log_debug(const char* message) {  // ⚠️ C4100 in Release builds
#ifdef DEBUG
    fprintf(stderr, "%s\n", message);
#endif
}
```

**After:**
```cpp
void log_debug([[maybe_unused]] const char* message) {
#ifdef DEBUG
    fprintf(stderr, "%s\n", message);
#endif
}
```

#### 3. API Compatibility Parameters

Use `[[maybe_unused]]` for parameters required by interface but not yet used:

**Before:**
```cpp
void callback(int event_type, void* user_data) {  // ⚠️ C4100
    // user_data not used yet
    process_event(event_type);
}
```

**After:**
```cpp
void callback(int event_type, [[maybe_unused]] void* user_data) {
    process_event(event_type);
}
```

#### 4. Lambda Capture Parameters

**Before:**
```cpp
db->scan([&](std::string_view key, std::string_view value) {
    (void)key;  // Old style
    return process(value);
});
```

**After:**
```cpp
db->scan([&]([[maybe_unused]] std::string_view key, std::string_view value) {
    return process(value);
});
```

### What NOT to Use

❌ **Pragma directives** (non-portable):
```cpp
#ifdef _MSC_VER
#pragma warning(disable: 4100)  // Don't do this
#endif
```

❌ **(void) casts** (verbose, unclear intent):
```cpp
void func(int param) {
    (void)param;  // Don't do this
}
```

✅ **Use [[maybe_unused]]** instead (C++17 standard):
```cpp
void func([[maybe_unused]] int param) {
    // Clear intent: parameter may be used conditionally
}
```

### Checklist for Changes

Before marking a variable as `[[maybe_unused]]`, verify:

- [ ] Variable is not used in any code path
- [ ] No side effects from constructor/destructor are lost
- [ ] Parameter not used in derived class implementations
- [ ] Tests still pass after changes
- [ ] No functional changes introduced

### CI Integration

Enable strict unused variable checking in builds:

**CMake:**
```cmake
if(MSVC)
  add_compile_options(/W4 /WX)  # Treat warnings as errors
else()
  add_compile_options(-Wall -Wextra -Werror=unused)
endif()
```

**GitHub Actions:**
```yaml
- name: Check for unused variables
  run: |
    cmake -B build -DCMAKE_CXX_FLAGS="-Werror=unused-variable -Werror=unused-parameter"
    cmake --build build
```

### Common Patterns

#### Function Context Parameters

Many function implementations receive a context parameter that isn't always used:

```cpp
class IFunction {
    virtual nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        [[maybe_unused]] const FunctionContext& ctx
    ) const override {
        // ctx used only by some implementations
        return compute(args[0]);
    }
};
```

#### Future Implementation Placeholders

Parameters for future features:

```cpp
bool processData(
    const Data& data,
    [[maybe_unused]] const Options& options  // Will be used in v2.0
) {
    return validate(data);
}
```

### Migration from Pragma Directives

When removing pragma directives:

1. **Identify** all uses of the suppressed warning
2. **Analyze** if variable is truly unused or conditionally used
3. **Remove** pragma directives
4. **Mark** parameters with `[[maybe_unused]]` if appropriate
5. **Delete** truly unused variables/functions
6. **Test** to ensure no functionality is lost

### Example: Before and After

**Before (with pragma):**
```cpp
#ifdef _MSC_VER
#pragma warning(disable: 4100)
#endif

void handler(Request& req, Response& res) {
    (void)res;  // unused
    return process(req);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
```

**After (with [[maybe_unused]]):**
```cpp
void handler(Request& req, [[maybe_unused]] Response& res) {
    return process(req);
}
```

Benefits:
- ✅ More readable
- ✅ Standard C++17
- ✅ Works across all compilers
- ✅ Clear intent
- ✅ No preprocessor noise

---

**Last Updated:** April 2026  
**PR:** [#XXX](link-to-pr) - Systematic unused variable removal
