# Contributing to ThemisDB

Thank you for your interest in contributing to ThemisDB! This document provides guidelines and instructions for contributing.

## Table of Contents

<div align="center">

# 🤝 Contributing to ThemisDB

**Thank you for your interest in making ThemisDB better!**

[![Contributors](https://img.shields.io/github/contributors/makr-code/ThemisDB)](https://github.com/makr-code/ThemisDB/graphs/contributors)
[![Pull Requests](https://img.shields.io/github/issues-pr/makr-code/ThemisDB)](https://github.com/makr-code/ThemisDB/pulls)
[![Good First Issues](https://img.shields.io/github/issues/makr-code/ThemisDB/good%20first%20issue)](https://github.com/makr-code/ThemisDB/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)

</div>

---

## 📋 Table of Contents

| Section | Description |
|---------|-------------|
| [🤝 Code of Conduct](#-code-of-conduct) | Community guidelines |
| [🧭 Governance Roles & Escalation](#governance-roles--escalation) | Role model, contribution path, and escalation channels |
| [🚀 Getting Started](#-getting-started) | Set up development environment |
| [💻 Development Workflow](#-development-workflow) | Branching and committing |
| [✅ Code Quality Standards](#-code-quality-standards) | Enforced quality checks |
| [📋 License Compliance](#-license-compliance) | License policy and compliance |
| [🔄 Pull Request Process](#-pull-request-process) | Submitting changes |
| [🏷️ Issue Labels](#️-issue-labels) | GitHub label system |
| [🐛 Reporting Bugs](#-reporting-bugs) | Bug report guidelines |
| [💡 Feature Requests](#-feature-requests) | Suggesting enhancements |
| [📝 Documentation](#documentation) | Living documentation and continuous review process |
| [📦 Package Maintenance](#-package-maintenance) | Platform packaging |

---

## 🤝 Code of Conduct

> [!IMPORTANT]
> This project adheres to the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to:
> - **Be respectful** and constructive in all interactions
> - **Welcome newcomers** and help them get started
> - **Assume good intentions** in discussions
> - **Focus on what is best** for the community and project
>
> Please read the full [Code of Conduct](CODE_OF_CONDUCT.md) for details.

---

## 🧭 Governance Roles & Escalation

Use this unified path for both external and internal contributors:

1. Follow this document for setup, workflow, quality checks, and PR process.
2. Use [GOVERNANCE.md](GOVERNANCE.md) for role boundaries and final decision authority.
3. Use [MAINTAINERS.md](MAINTAINERS.md) to identify module ownership and review routing.
4. Use [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for behavior and moderation rules.
5. Use [SECURITY.md](SECURITY.md) and [SOP.md](SOP.md) for private vulnerability and incident response processes.

Canonical channels:

- **General questions / governance discussion:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Bug reports / feature requests / contributor blockers:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Security vulnerabilities (private):** [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new)

---

## Getting Started

### Prerequisites


### Development Environment Setup

**1. Clone the repository:**

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

**2. Install dependencies:**

**Linux/macOS:**
```bash
./setup.sh
```

**Windows:**
```powershell
.\setup.ps1
```

**3. Build the project:**

**Linux/macOS:**
```bash
./build.sh
```

**Windows:**
```powershell
.\build.ps1
```

**4. Run tests:**

```bash
cd build
ctest --output-on-failure
```

**5. Optional Features (v1.3.0+):**

ThemisDB has optional features that require specific build flags:

```bash
# Build with LLM support (requires llama.cpp)
cmake -B build -DTHEMIS_ENABLE_LLM=ON
git clone https://github.com/ggerganov/llama.cpp.git

# Build with HTTP/2 support
cmake -B build -DTHEMIS_ENABLE_HTTP2=ON

# Build with WebSocket support
cmake -B build -DTHEMIS_ENABLE_WEBSOCKET=ON

# Build with MQTT support
cmake -B build -DTHEMIS_ENABLE_MQTT=ON

# Build with PostgreSQL Wire Protocol
cmake -B build -DTHEMIS_ENABLE_POSTGRES_WIRE=ON

# Build with MCP Server support
cmake -B build -DTHEMIS_ENABLE_MCP=ON

# Build with Image Analysis plugins
cmake -B build -DTHEMIS_ENABLE_IMAGE_ANALYSIS=ON

# Build with all optional features
cmake -B build -DTHEMIS_ENABLE_ALL_PROTOCOLS=ON
```
---

## 🚀 Getting Started

### Prerequisites

| Tool | Minimum Version | Purpose |
|------|:---------------:|---------|
| **C++ Compiler** | GCC 10+ / Clang 12+ / MSVC 2019+ | Core compilation |
| **CMake** | 3.20+ | Build system |
| **vcpkg** | Latest | Dependency management |
| **Git** | 2.x | Version control |

### Development Environment Setup

<details open>
<summary><b>1️⃣ Clone the Repository</b></summary>

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

</details>

<details>
<summary><b>2️⃣ Install Dependencies</b></summary>

**Linux/macOS:**
```bash
./setup.sh
```

**Windows:**
```powershell
.\setup.ps1
```

> [!TIP]
> The setup script automatically installs vcpkg and all required dependencies.

</details>

<details>
<summary><b>3️⃣ Build the Project</b></summary>

**Linux/macOS:**
```bash
./build.sh
```

**Windows:**
```powershell
.\build.ps1
```

> [!NOTE]
> First build may take 10-15 minutes as vcpkg compiles dependencies from source.

</details>

<details>
<summary><b>4️⃣ Run Tests</b></summary>

```bash
cd build
ctest --output-on-failure
```

**Expected result:** All tests should pass ✅

</details>

<details>
<summary><b>5️⃣ Optional Features (v1.3.0+)</b></summary>

ThemisDB supports optional protocol and AI integrations:

| Feature | CMake Flag | Dependencies |
|---------|------------|--------------|
| 🤖 **LLM Support** | `-DTHEMIS_ENABLE_LLM=ON` | llama.cpp |
| 🌐 **HTTP/2** | `-DTHEMIS_ENABLE_HTTP2=ON` | nghttp2 |
| 📡 **WebSocket** | `-DTHEMIS_ENABLE_WEBSOCKET=ON` | uWebSockets |
| 📬 **MQTT** | `-DTHEMIS_ENABLE_MQTT=ON` | mosquitto |
| 🐘 **PostgreSQL Wire** | `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON` | libpq |
| 🔌 **MCP Server** | `-DTHEMIS_ENABLE_MCP=ON` | - |
| 🖼️ **Image Analysis** | `-DTHEMIS_ENABLE_IMAGE_ANALYSIS=ON` | OpenCV |

**Build with LLM support:**
```bash
cmake -B build -DTHEMIS_ENABLE_LLM=ON
git clone https://github.com/ggerganov/llama.cpp.git
cmake --build build
```

**Build with all protocols:**
```bash
cmake -B build -DTHEMIS_ENABLE_ALL_PROTOCOLS=ON
cmake --build build
```

</details>

See [docs/de/legal/ATTRIBUTIONS.md](docs/de/legal/ATTRIBUTIONS.md) for third-party dependency information.

### Cross-Compile Toolchain/Triplet Validation

When configuring cross builds, ThemisDB now validates the full tuple at CMake
configure time:

- `CMAKE_CROSSCOMPILING=TRUE` requires `CMAKE_TOOLCHAIN_FILE`.
- `VCPKG_TARGET_TRIPLET` must be consistent with the target
  `CMAKE_SYSTEM_PROCESSOR` and `CMAKE_SYSTEM_NAME`.
- For Linux ARM cross targets (`arm64`, `armv7`), `CMAKE_SYSROOT` is required
  and must point to an existing path.

Supported toolchain examples:

- `cmake/platforms/Toolchains/amd64-linux-gnu.cmake` → `x64-linux`
- `cmake/platforms/Toolchains/arm64-linux-gnu.cmake` → `arm64-linux`
- `cmake/platforms/Toolchains/armv7-linux-gnueabihf.cmake` → `arm-linux`
- `cmake/platforms/Toolchains/windows-msvc.cmake` → `x64-windows`
- `cmake/platforms/Toolchains/x86_64-w64-mingw32.cmake` → `x64-mingw-static`

Example (ARM64 Linux cross compile):

```bash
cmake -S . -B build-arm64 \
  -DCMAKE_CROSSCOMPILING=TRUE \
  -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/arm64-linux-gnu.cmake \
  -DVCPKG_TARGET_TRIPLET=arm64-linux \
  -DCMAKE_SYSROOT=/usr/aarch64-linux-gnu
```

**Running tests under Windows / WSL (developer tips)**

- If you build under WSL the default build output used by repository helper scripts is `build-wsl/` (e.g. `build-wsl/themis_tests` and `build-wsl/themis_server`). Helper scripts (such as `.tools/vault_dev_run.ps1`) rely on this layout.
- To run the GoogleTest binary directly in WSL and export JUnit XML to the Windows host:

```powershell
# from PowerShell (host):
wsl bash -lc "cd /mnt/c/VCC/themis; ./build-wsl/themis_tests --gtest_output=xml:/mnt/c/Temp/themis_tests.xml"
```

- Vault integration tests expect a reachable Vault at `VAULT_ADDR` and a valid token in `VAULT_TOKEN`. The repository includes a small helper `.tools/vault_dev_run.ps1` which:
   - starts a local Vault dev container, enables KV v2 at mount `themis/`, writes a 32‑byte base64 key to `themis/keys/test_key`, and runs the Vault tests from WSL (writing XML output to `C:\Temp`).
   - Preconditions: Docker available locally and a WSL installation with repo mounted under `/mnt/c/VCC/themis`.


### Install Code Quality Tools

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install -y clang-tidy cppcheck lcov gcovr

# Install gitleaks
wget https://github.com/gitleaks/gitleaks/releases/download/v8.18.4/gitleaks_8.18.4_linux_x64.tar.gz
tar -xzf gitleaks_8.18.4_linux_x64.tar.gz
sudo mv gitleaks /usr/local/bin/
```

**macOS:**
```bash
brew install llvm cppcheck lcov gitleaks
```

**Windows:**
```powershell
choco install llvm cppcheck gitleaks
```

## Development Workflow

> [!IMPORTANT]
> **ThemisDB uses a Git Flow branching strategy:**
> - `develop` = Active development branch (integration)
> - All features branch from `develop` and merge back to `develop`
> - Release lanes are edition-specific: `minimal`, `community`, `enterprise`, `hyperscaler`, `military`
> - See [docs/ci-cd/branching-release-history/BRANCHING_STRATEGY.md](docs/ci-cd/branching-release-history/BRANCHING_STRATEGY.md) for complete details

### 1. Create a Feature Branch

```bash
# IMPORTANT: Always branch from develop
git checkout develop
git pull origin develop
git checkout -b feature/your-feature-name

# OR for bug fixes
git checkout -b bugfix/bug-description
```

**Branch naming conventions:**

### 2. Make Your Changes


### 3. Run Code Quality Checks

**Before committing, run local quality checks:**

```bash
# Linux/macOS
./scripts/check-quality.sh

# Windows
.\scripts\check-quality.ps1
```

**Auto-fix issues where possible:**

```bash
# Linux/macOS
./scripts/check-quality.sh --fix

# Windows
.\scripts\check-quality.ps1 -Fix
```

### 4. Commit Your Changes

**Write clear, descriptive commit messages:**

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Examples:**

```
feat(storage): Add checkpoint-based incremental backups


Closes #123
```

```
fix(query): Correct off-by-one error in pagination

The cursor offset calculation was incorrect for empty result sets,
causing the next page to skip the first result.

Fixes #456
```

**Commit types:**

### 5. Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

**Create Pull Request:**
- **Base Branch**: `develop`
- **Compare Branch**: `feature/your-feature-name`
- Add clear description of changes
- Link related issues

> [!NOTE]
> Pull requests should target the `develop` branch unless you are working on a hotfix for production.
---

## 💻 Development Workflow

> [!IMPORTANT]
> **ThemisDB uses a Git Flow branching strategy:**
> - 🚧 `develop` = Active development branch (integration)  
> - 🌿 All features branch from `develop` and merge back to `develop`
> - 🚢 Edition release lanes: `minimal`, `community`, `enterprise`, `hyperscaler`, `military`
> - 📖 See [docs/ci-cd/branching-release-history/BRANCHING_STRATEGY.md](docs/ci-cd/branching-release-history/BRANCHING_STRATEGY.md) for complete details

### 1️⃣ Create a Feature Branch

```bash
# IMPORTANT: Always branch from develop
git checkout develop
git pull origin develop
git checkout -b feature/your-feature-name

# OR for bug fixes
git checkout -b bugfix/bug-description
```

**Branch Naming Conventions:**

| Prefix | Purpose | Example |
|--------|---------|---------|
| `feature/` | 🎉 New features | `feature/vector-search-optimization` |
| `fix/` | 🐛 Bug fixes | `fix/connection-leak-in-pool` |
| `docs/` | 📖 Documentation | `docs/improve-api-examples` |
| `refactor/` | ♻️ Code refactoring | `refactor/extract-storage-interface` |
| `test/` | 🧪 Test improvements | `test/add-transaction-edge-cases` |
| `perf/` | ⚡ Performance | `perf/optimize-index-lookup` |

### 2️⃣ Make Your Changes

> [!IMPORTANT]
> **Best Practices:**
> - ✅ Write clean, maintainable code following **C++17** standards
> - ✅ Add **unit tests** for new functionality
> - ✅ Update **documentation** as needed
> - ✅ Keep commits **focused and atomic**

### 3️⃣ Run Code Quality Checks

**Before committing, run local quality checks:**

<details>
<summary><b>Linux/macOS</b></summary>

```bash
# Run all checks
./scripts/check-quality.sh

# Auto-fix issues
./scripts/check-quality.sh --fix
```

</details>

<details>
<summary><b>Windows</b></summary>

```powershell
# Run all checks
.\scripts\check-quality.ps1

# Auto-fix issues
.\scripts\check-quality.ps1 -Fix
```

</details>

> [!TIP]
> Use `--fix` to automatically resolve formatting and minor issues.

### 4️⃣ Commit Your Changes

**Commit Message Format:**

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Example Commits:**

<details>
<summary><b>Feature Commit</b></summary>

```
feat(storage): Add checkpoint-based incremental backups

- Implement RocksDB checkpoint API integration
- Add WAL archiving with retention policies
- Create cross-platform backup scripts (PowerShell + Bash)
- Add systemd and Kubernetes automation examples

Closes #123
```

</details>

<details>
<summary><b>Bug Fix Commit</b></summary>

```
fix(query): Correct off-by-one error in pagination

The cursor offset calculation was incorrect for empty result sets,
causing the next page to skip the first result.

Fixes #456
```

</details>

**Commit Types:**

| Type | Emoji | Description |
|------|:-----:|-------------|
| `feat` | ✨ | New feature |
| `fix` | 🐛 | Bug fix |
| `docs` | 📝 | Documentation changes |
| `style` | 💄 | Code style (no logic change) |
| `refactor` | ♻️ | Code refactoring |
| `perf` | ⚡ | Performance improvements |
| `test` | ✅ | Test additions/improvements |
| `chore` | 🔧 | Build/tooling changes |

### 5️⃣ Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

> [!NOTE]
> **Target Branch**: Pull requests should target `develop` unless you are working on an edition-specific release/hotfix.

**When creating your PR:**
1. Set **Base** to `develop`
2. Set **Compare** to your feature branch
3. Add clear description of changes
4. Link related issues with `Closes #123`
5. Add appropriate labels

## Code Quality Standards

ThemisDB enforces strict code quality standards through automated CI checks:

### Static Analysis (clang-tidy)


**Common issues to avoid:**

### Linting (cppcheck)


### Code Coverage


**Generate local coverage report:**

```bash
# After running tests with coverage
mkdir -p coverage
lcov --capture --directory build --output-file coverage/coverage.info
lcov --remove coverage/coverage.info '/usr/*' '*/vcpkg_installed/*' '*/tests/*' \
  --output-file coverage/coverage-filtered.info
genhtml coverage/coverage-filtered.info --output-directory coverage/html

# Open report
xdg-open coverage/html/index.html  # Linux
open coverage/html/index.html      # macOS
start coverage/html/index.html     # Windows
```

### Secret Scanning (Gitleaks)


**Avoid committing:**

**Use instead:**

### Code Style

**Naming conventions:**

**File structure:**

**Comments:**
---

## ✅ Code Quality Standards

> [!IMPORTANT]
> ThemisDB enforces **strict code quality standards** through automated CI checks.  
> All PRs must pass these checks before merging.

### 🔍 Static Analysis (clang-tidy)

<details>
<summary><b>Configuration Details</b></summary>

**Enabled Checks:**
- `bugprone-*` - Detect potential bugs
- `clang-analyzer-*` - Deep code analysis
- `cppcoreguidelines-*` - C++ Core Guidelines compliance
- `modernize-*` - Modern C++ features
- `performance-*` - Performance optimizations
- `readability-*` - Code readability

**Configuration File:** `.clang-tidy`

**Auto-fix:**
```bash
./scripts/check-quality.sh --fix  # Linux/macOS
.\scripts\check-quality.ps1 -Fix  # Windows
```

</details>

> [!WARNING]
> **Common Issues to Avoid:**
> - ❌ Magic numbers → Use named constants
> - ❌ Unnecessary copies → Use `const&` or `std::move`
> - ❌ Missing `override` keyword
> - ❌ C-style casts → Use `static_cast`/`dynamic_cast`

### 🧪 Linting (cppcheck)

- **Enabled Checks:** All (with suppressions for known false positives)
- **Configuration:** `.cppcheck-suppressions`
- **Purpose:** Catch memory leaks, null pointer dereferences, undefined behavior

### 📊 Code Coverage

| Component | Target | Critical Path |
|-----------|:------:|:-------------:|
| Overall | **80%+** | - |
| Storage Engine | - | **90%+** |
| Transaction Manager | - | **90%+** |
| Query Engine | - | **90%+** |

<details>
<summary><b>Generate Local Coverage Report</b></summary>

```bash
# After running tests with coverage
mkdir -p coverage
lcov --capture --directory build --output-file coverage/coverage.info
lcov --remove coverage/coverage.info '/usr/*' '*/vcpkg_installed/*' '*/tests/*' \
   --output-file coverage/coverage-filtered.info
genhtml coverage/coverage-filtered.info --output-directory coverage/html

# Open report
xdg-open coverage/html/index.html  # Linux
open coverage/html/index.html      # macOS
start coverage/html/index.html     # Windows
```

</details>

### 🔐 Secret Scanning (Gitleaks)

> [!CAUTION]
> **CI fails immediately if secrets are detected!**

- **Tool:** Gitleaks
- **Configuration:** `.gitleaks.toml`

**❌ Never Commit:**
- API keys
- Passwords
- Private keys
- Database credentials

**✅ Use Instead:**
- Environment variables
- `.env.example` templates (never `.env`)
- Configuration placeholders (`YOUR_API_KEY_HERE`)

### 📋 License Compliance

> [!IMPORTANT]
> **All dependencies must comply with ThemisDB's license policy!**  
> The automated license compliance workflow blocks PRs with incompatible licenses.

**License Policy:**
- **Allowed:** MIT, Apache-2.0, BSD-2-Clause, BSD-3-Clause, ISC, Unlicense, CC0-1.0, MPL-2.0
- **Warning (Weak Copyleft):** LGPL-2.1, LGPL-3.0, EPL-1.0, EPL-2.0 (acceptable with dynamic linking)
- **Blocked (Strong Copyleft):** GPL-2.0, GPL-3.0, AGPL-3.0
- **Blocked:** Proprietary, Commercial, UNLICENSED

**Before adding dependencies:**

1. **Check the license** - Verify it's in the allowed list
2. **Review `.license-policy.json`** - Consult the policy file
3. **Document in PR** - Mention the license in your pull request

**If your PR is blocked:**
- **Option A:** Replace the dependency with a compatible alternative (preferred)
- **Option B:** Request an exception by creating an issue with label `license-exception`

**Automated Checks:**
- ✅ Runs on every PR that changes dependencies
- 📊 Monthly audit on the first of each month
- 🔍 Scans direct and transitive `vcpkg.json` dependencies against `.license-policy.json`
- 📦 Publishes `license-summary.md` and `vcpkg-license-sbom.json` as workflow artifacts
- 🚫 Must stay green before a PR can be approved and merged

**Local verification:**

```bash
cmake --build build --target license-compliance
```

**Documentation:**
- [Full License Compliance Process](docs/de/compliance/license-compliance.md) (German)
- [License Compliance Process](docs/en/compliance/license-compliance.md) (English)
- [License Policy File](.license-policy.json)

### 🎨 Code Style

<details>
<summary><b>Naming Conventions</b></summary>

| Element | Convention | Example |
|---------|-----------|---------|
| **Namespaces** | `lower_case` | `vccdb`, `themis` |
| **Classes/Structs** | `CamelCase` | `StorageEngine`, `HttpServer` |
| **Functions** | `camelCase` | `getValue`, `processQuery` |
| **Variables** | `lower_case` | `table_name`, `max_size` |
| **Private Members** | `lower_case_` | `db_path_`, `cache_size_` |
| **Constants** | `UPPER_CASE` | `MAX_CONNECTIONS`, `DEFAULT_PORT` |

</details>

<details>
<summary><b>File Structure</b></summary>

```
include/<module>/<name>.h     # Header files
src/<module>/<name>.cpp       # Implementation files
tests/test_<module>_<feature>.cpp  # Test files
```

</details>

<details>
<summary><b>Comment Guidelines</b></summary>

- Use `//` for single-line comments
- Use `/** ... */` for documentation (Doxygen style)
- Explain **why**, not **what** (code should be self-documenting)

**Example:**
```cpp
// Good: Explains WHY
// Use exponential backoff to prevent thundering herd
std::this_thread::sleep_for(std::chrono::milliseconds(delay));

// Bad: Explains WHAT (obvious from code)
// Sleep for delay milliseconds
std::this_thread::sleep_for(std::chrono::milliseconds(delay));
```

</details>

## Pull Request Process

### Before Submitting

1. **Run all checks locally:**
   ```bash
   ./scripts/check-quality.sh
   ```

2. **Ensure all tests pass:**
   ```bash
   cd build
   ctest --output-on-failure
   ```

3. **Update documentation:**
   - Add/update relevant `.md` files in `docs/`
   - Update `README.md` if needed
   - Add docstrings for new public APIs

4. **Add tests:**
   - Unit tests for new functions/classes
   - Integration tests for new features
   - Update existing tests if behavior changes

### PR Description Template

```markdown
## Description
Brief description of changes

## Type of Change
---

## 🔄 Pull Request Process

### Before Submitting

> [!IMPORTANT]
> **Pre-submission Checklist:**
> - ✅ All quality checks pass locally
> - ✅ All tests pass
> - ✅ Documentation updated
> - ✅ Tests added for new functionality

<details open>
<summary><b>1️⃣ Run All Checks Locally</b></summary>

```bash
./scripts/check-quality.sh  # Linux/macOS
.\scripts\check-quality.ps1  # Windows
```

**This runs:**
- clang-tidy (static analysis)
- cppcheck (linting)
- Gitleaks (secret scanning)

</details>

<details>
<summary><b>2️⃣ Ensure All Tests Pass</b></summary>

```bash
cd build
ctest --output-on-failure
```

**Expected:** All tests should pass ✅

</details>

<details>
<summary><b>3️⃣ Update Documentation</b></summary>

- 📝 Add/update relevant `.md` files in `docs/`
- 📖 Update `README.md` if needed
- 💬 Add docstrings for new public APIs (Doxygen format)

</details>

<details>
<summary><b>4️⃣ Add Tests</b></summary>

| Test Type | Purpose | Example |
|-----------|---------|---------|
| **Unit Tests** | Test individual functions/classes | `test_vector_search.cpp` |
| **Integration Tests** | Test feature workflows | `test_backup_restore.cpp` |
| **Regression Tests** | Ensure bugs stay fixed | `test_issue_456_pagination.cpp` |

</details>

### Tier-Based Security & Hardening Checklist (Required)

For all changes that touch runtime behavior, contributors must include tier information and security evidence.

1. Identify impacted tier(s) using the model in [ARCHITECTURE.md](ARCHITECTURE.md#security--hardening-tiering-model-core-module---plugin).
2. Document trust-boundary crossings (for example T3 -> T2, T5 -> T4 broker calls).
3. Confirm boundary controls on changed ingress/extension paths:
   - authentication and authorization
   - input validation and parser limits
   - rate limiting and quota behavior
   - audit event emission
4. Add or update tests for changed boundaries, especially for T3/T4/T5 code paths.
5. If a change effectively increases trust or privilege, include explicit architecture/security maintainer approval in the PR.

### PR Description Template

```markdown
## 📋 Description
Brief description of changes

## 🔖 Type of Change
- [ ] 🐛 Bug fix (non-breaking change which fixes an issue)
- [ ] ✨ New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## How Has This Been Tested?
Describe the tests you ran to verify your changes.

## Checklist
- [ ] My code follows the code style of this project
- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing unit tests pass locally with my changes
- [ ] Any dependent changes have been merged and published
- [ ] I have run `./scripts/check-quality.sh` and fixed all issues
- [ ] I did not introduce Simulation/Stub/Mockup or legacy compatibility paths without explicit human approval
- [ ] Any approved non-production path is explicitly human-marked (Reason, Activation, Production Delta, Approved By, Removal Target)
- [ ] I classified impacted Security Tier(s) and documented trust-boundary crossings
- [ ] I verified boundary controls (AuthN/AuthZ, validation, rate limits, audit) for affected T3/T4/T5 paths
- [ ] I added boundary-focused tests for tier-crossing behavior or documented why not applicable
```

### Reviewer Gate: Non-Production Paths

Reviewers must treat this as a hard blocker policy:

- No Simulation/Stub/Mockup or legacy compatibility path without explicit human approval.
- No unmarked non-production path.
- If approved, require explicit human marker fields in code comments:
   - Reason
   - Activation
   - Production Delta
   - Approved By
   - Removal Target

### Review Process

1. **Automated checks** run on all PRs:
   - Build (Linux + Windows)
   - Tests
   - Clang-tidy static analysis
   - Cppcheck linting
   - Code coverage
   - Gitleaks secret scanning

2. **Maintainer review**:
   - Code quality and style
   - Test coverage
   - Documentation
   - Architecture fit

3. **Address feedback**:
   - Make requested changes
   - Push updates to the same branch
   - Request re-review

4. **Merge**:
   - Once approved and all checks pass
   - Maintainer will **squash and merge** (preferred for feature/bugfix PRs to keep history clean)
   - Merge commits are only used for release and hotfix branches

### Merge Strategy Guidelines

> [!IMPORTANT]
> **ThemisDB uses different merge strategies depending on the branch type:**

| Branch Type | Merge Method | Reason |
|------------|--------------|---------|
| **feature/** → develop | **Squash and merge** ✅ | Keeps develop history clean, one commit per feature |
| **bugfix/** → develop | **Squash and merge** ✅ | Keeps develop history clean, one commit per fix |
| **release/** → edition lane | **Merge commit** | Preserves full release history and commit metadata |
| **hotfix/** → edition lane | **Merge commit** | Preserves full hotfix history for audit purposes |

**Why squash merge for features/bugfixes?**
- ✅ Cleaner, more readable git history
- ✅ One logical commit per feature/fix
- ✅ Easier to revert if needed
- ✅ Better changelog generation
- ❌ Development commits (WIP, fix typo, etc.) stay in feature branch

**Configuring GitHub Repository Settings:**

Maintainers should configure the repository settings on GitHub to enforce this:
1. Go to Settings → General → Pull Requests
2. Enable "Allow squash merging" ✅
3. Enable "Allow merge commits" ✅ (needed for releases)
4. Disable "Allow rebase merging" ❌ (optional)
5. Set "Squash merging" as the default for the repository

## 🏷️ Issue Labels

ThemisDB uses a comprehensive labeling system to categorize and organize issues and pull requests.

**Quick Reference:**

| Label Category | Purpose | Examples |
|----------------|---------|----------|
| **Priority** | Urgency level | `priority:P0` (critical), `priority:P1` (high) |
| **Type** | Issue nature | `type:bug`, `type:feature`, `type:enhancement` |
| **Area** | Component affected | `area:llm`, `area:storage`, `area:api` |
| **Status** | Current state | `status:ready`, `status:in-progress` |
| **Effort** | Work required | `effort:small` (<1 day), `effort:large` (1-2 weeks) |
| **Experience** | Contributor level | `good first issue`, `help wanted` |

**For issue reporters:** Don't worry about adding labels - maintainers will add appropriate labels during triage.

**For contributors:** Look for `good first issue` labels if you're new to the project.

**Complete documentation:**
- **Full Guide:** [.github/LABELS_GUIDE.md](.github/LABELS_GUIDE.md) (English & German)
- **Quick Reference:** [.github/LABELS_QUICK_REF.md](.github/LABELS_QUICK_REF.md)
- **Label Definitions:** [.github/labels.yml](.github/labels.yml)

---

## Reporting Bugs

**Before submitting a bug report:**

1. **Check existing issues**: Search for similar reports
2. **Verify it's a bug**: Ensure it's not expected behavior
3. **Test on latest version**: Bug may already be fixed

**Bug report template:**

```markdown
## Description
Clear description of the bug

## Steps to Reproduce
1. Step one
2. Step two
3. ...

## Expected Behavior
What you expected to happen

## Actual Behavior
What actually happened

## Environment
- OS: [e.g., Ubuntu 22.04, Windows 11]
- Compiler: [e.g., GCC 11.2, MSVC 2022]
- ThemisDB version/commit: [e.g., v1.0.0 or commit hash]

## Additional Context
Logs, screenshots, or other relevant information
```

## Feature Requests

**Feature request template:**

```markdown
## Feature Description
Clear description of the proposed feature

## Motivation
Why is this feature needed? What problem does it solve?

## Proposed Solution
How should this feature work?

## Alternatives Considered
Other approaches you've thought about

## Additional Context
Any other relevant information
```

## Documentation

ThemisDB follows a **Living Documentation** approach where documentation evolves with the codebase through continuous review and improvement.

### Documentation Structure

- **Architecture docs**: `docs/architecture/`
- **API docs**: `docs/api/`
- **User guides**: `docs/`
- **Examples**: `examples/`
- **Compendium**: `compendium/`
- **Translations**: `docs/de/`, `docs/fr/`, `docs/es/`, `docs/ja/`
- **Archived docs**: `docs/ARCHIVED/` - Historical development documents (GAP analyses, old roadmaps, completed implementations)

> **Note:** Historical development documents (GAP analyses, roadmaps, TODO lists, implementation summaries) have been archived to `docs/ARCHIVED/`. See [docs/ARCHIVED/README.md](docs/ARCHIVED/README.md) for the complete archive index.

### Documentation Requirements

**All PRs with code changes must include documentation updates:**
- [ ] Complete the [PR Documentation Checklist](docs/PR_DOCUMENTATION_CHECKLIST.md)
- [ ] Update affected documentation files
- [ ] Add/update code examples if applicable
- [ ] Update CHANGELOG.md
- [ ] Ensure documentation builds successfully (`mkdocs build --strict`)

**Documentation must be:**
- **Accurate** - Reflects current implementation
- **Complete** - Covers all features and use cases
- **Clear** - Easy to understand for target audience
- **Tested** - Code examples compile and run
- **Maintained** - Regularly reviewed and updated

### Continuous Review Process

Documentation is reviewed at multiple levels:

1. **PR Review** (Every PR with code changes)
   - Documentation changes reviewed alongside code
   - At least one reviewer verifies accuracy
   - Must pass before merge

2. **Monthly Reviews** (First Monday of each month)
   - Quick check of recent changes
   - Identify and fix quick wins
   - Track documentation debt

3. **Quarterly Reviews** (Start of each quarter)
   - Comprehensive documentation audit
   - Test all examples
   - Update translations
   - Archive outdated content

4. **Release Reviews** (Before each release)
   - Verify release documentation
   - Update version references
   - Validate migration guides

**Review schedule and process:** [docs/DOCUMENTATION_REVIEW_SCHEDULE.md](docs/DOCUMENTATION_REVIEW_SCHEDULE.md)

### Documentation Guidelines

**Writing style:**
- Use active voice ("Configure the database..." not "The database can be configured...")
- Be specific with values and examples
- Provide working code examples
- Explain prerequisites clearly
- Keep language clear and concise

**Quality standards:**
```bash
# Before submitting PR:
mkdocs build --strict              # Verify build
./scripts/check-links.sh           # Validate links
./scripts/test-examples.sh         # Test examples (if available)
```

**Complete documentation guidelines:** [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](docs/DOCUMENTATION_REVIEW_GUIDELINES.md)

### Documentation Archival

ThemisDB preserves outdated documentation in archives to maintain historical context while keeping active documentation current.

**Archive documentation when:**
- Content is outdated and superseded by newer documentation
- Feature/component no longer exists or has been replaced
- Information has been consolidated into another document
- Content is no longer accurate or relevant to current versions
- Implementation summaries from completed work (keep for reference)

**Archive structure:**
- Language-agnostic: `docs/archive/`
- Language-specific: `docs/{LANG}/archive/` (e.g., `docs/de/archive/`)

**Archival process:**

1. **Review** - Verify document should be archived
2. **Preserve** - Extract valuable content to current docs
3. **Archive** - Move with `git mv` (preserves history)
4. **Annotate** - Add archive note using template
5. **Update** - Fix all references and update indexes
6. **Document** - Update CHANGELOG and archive README

**Resources:**
- **Process Guide:** [docs/DOCUMENTATION_ARCHIVAL_PROCESS.md](docs/DOCUMENTATION_ARCHIVAL_PROCESS.md)
- **Archive Template:** [docs/archive/ARCHIVE_NOTE_TEMPLATE.md](docs/archive/ARCHIVE_NOTE_TEMPLATE.md)
- **Archive Indexes:** [docs/archive/](docs/archive/), [docs/de/archive/](docs/de/archive/)

**Key principles:**
- ✅ Always use `git mv` to preserve history
- ✅ Add archive note explaining why and when
- ✅ Update all references to prevent broken links
- ✅ Document archival in CHANGELOG
- ✅ Keep archive READMEs organized and current

## Package Maintenance

If you're interested in maintaining ThemisDB packages for a specific distribution:

### Becoming a Package Maintainer

We welcome package maintainers for all platforms! To become a maintainer:

1. **Review the packaging documentation:**
   - Read `docs/packaging.md` for detailed instructions
   - Check `docs/PACKAGING-QUICKREF.md` for quick reference

2. **Test the package build:**
   - Build the package for your target platform
   - Install and test in a clean environment
   - Verify all functionality works as expected

3. **Submit to distribution repositories:**
   - Follow platform-specific guidelines (see `docs/packaging.md`)
   - Submit package to appropriate repository (PPA, AUR, Copr, etc.)
   - Notify us via GitHub issue when package is published

4. **Keep packages updated:**
   - Monitor releases and security updates
   - Update packages within 1-2 weeks of new releases
   - Coordinate with core team for pre-release testing

### Supported Platforms

We currently support packaging for:

### Automation Tools

Use the provided scripts to prepare new releases:

```bash
# Linux/macOS
./scripts/prepare-release.sh 1.0.1

# Windows
.\scripts\prepare-release.ps1 -Version 1.0.1
```

These scripts automatically update version numbers across all packaging files.

### Package Maintainer Benefits


## Questions?


## License

By contributing to ThemisDB, you agree that your contributions will be licensed under the MIT License.


Thank you for contributing to ThemisDB! 🚀
---

## 🚀 Release Process

ThemisDB uses a **tag-based release strategy** with semantic versioning. Releases are automated through GitHub Actions when version tags are pushed.

### Branching Strategy Overview

> [!IMPORTANT]
> **Canonical branch model:**
> - `develop` = default integration branch for feature and bugfix work
> - `minimal`, `community`, `enterprise`, `hyperscaler`, `military` = protected edition release lanes
> - Legacy names `main` and `millitary` are historical only and must not be used for new PR targets
> - See [BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md) and [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) for normative release governance

### Release Flow

```
feature/bugfix branches -> develop -> release/<edition>/vX.Y.Z -> <edition lane> -> tag -> GitHub Release
                              ^                                              |
                              +-------------- back-merge/cherry-pick --------+
```

### Creating an Edition Release

<details>
<summary><b>1️⃣ Prepare Release Branch</b></summary>

```bash
# Branch from develop for release preparation
git checkout develop
git pull --ff-only origin develop
git checkout -b release/community/v1.4.0

# Update version and release notes
# VERSION -> 1.4.0
# CHANGELOG.md -> add notes under [1.4.0] - YYYY-MM-DD

git add VERSION CHANGELOG.md
git commit -m "chore(release): prepare community v1.4.0"
git push origin release/community/v1.4.0
```

</details>

<details>
<summary><b>2️⃣ Create PR to Edition Lane</b></summary>

1. Create a Pull Request from `release/community/v1.4.0` to `community`
2. Ensure required checks pass
3. Get maintainer approval
4. Merge using a merge commit for release provenance

</details>

<details>
<summary><b>3️⃣ Tag from Edition Lane</b></summary>

```bash
git checkout community
git pull --ff-only origin community

# Community tag example
git tag -a v1.4.0 -m "ThemisDB Community v1.4.0"
git push origin v1.4.0
```

Edition tag examples:
- `minimal-v1.4.0`
- `enterprise-v1.4.0`
- `hyperscaler-v1.4.0`
- `military-v1.4.0`

</details>

<details>
<summary><b>4️⃣ Back-Merge to Develop</b></summary>

```bash
git checkout develop
git pull --ff-only origin develop
git merge community -m "chore: merge community v1.4.0 back to develop"
git push origin develop
```

</details>

### Version Numbering

ThemisDB follows **Semantic Versioning 2.0.0** (semver.org):

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]

Examples:
- 1.4.0         (stable release)
- 1.4.0-alpha   (alpha pre-release)
- 1.4.0-beta.1  (beta pre-release)
- 1.4.0-rc.1    (release candidate)
- 1.4.0+build.1 (build metadata)
```

**Version Increment Rules:**

| Version | When to Increment | Example |
|---------|------------------|---------|
| **MAJOR** | Breaking changes to API/behavior | 1.4.0 → 2.0.0 |
| **MINOR** | New features (backward compatible) | 1.4.0 → 1.5.0 |
| **PATCH** | Bug fixes (backward compatible) | 1.4.0 → 1.4.1 |

### Hotfix Process

For critical production issues that need immediate release:

<details>
<summary><b>Hotfix Workflow</b></summary>

```bash
# 1. Create hotfix branch from affected edition lane (example: community)
git checkout community
git pull origin community
git checkout -b hotfix/v1.4.1

# 2. Fix the issue
# ... make changes ...

# 3. Update VERSION and CHANGELOG
echo "1.4.1" > VERSION
# Update CHANGELOG.md with hotfix notes

# 4. Commit and push
git add .
git commit -m "fix: Critical security issue in authentication"
git push origin hotfix/v1.4.1

# 5. Create PR to affected edition lane (fast-track approval)
# Merge after required checks pass

# 6. Tag the hotfix release
git checkout community
git pull origin community
git tag -a v1.4.1 -m "Hotfix v1.4.1: Security patch"
git push origin v1.4.1

# 7. Merge back to develop
git checkout develop
git pull origin develop
git merge community -m "chore: Merge hotfix v1.4.1 to develop"
git push origin develop
```

</details>

### Pre-Release Process

For alpha, beta, or release candidate versions:

```bash
# Example: Create beta release
echo "1.5.0-beta.1" > VERSION

# Tag with pre-release label
git tag -a v1.5.0-beta.1 -m "Beta release v1.5.0-beta.1"
git push origin v1.5.0-beta.1
```

> [!NOTE]
> Pre-release tags (containing `-` like v1.5.0-beta.1) are automatically marked as **pre-release** in GitHub Releases.

### Release Checklist

Use this checklist when preparing a release:

- [ ] All features for this version are merged to `develop`
- [ ] All tests pass on `develop` branch
- [ ] CHANGELOG.md is updated with all changes
- [ ] VERSION file is updated to new version
- [ ] Documentation is up to date
- [ ] Migration guide prepared (if breaking changes)
- [ ] Security scan passed
- [ ] Release notes drafted
- [ ] Release branch created from develop
- [ ] PR to target edition lane created and approved
- [ ] Tag created and pushed
- [ ] GitHub Release published (automatic)
- [ ] Changes merged back to develop
- [ ] Package maintainers notified

### Status Check Requirements

| Branch | Required Checks | Optional Checks |
|--------|----------------|-----------------|
| **develop** | Ubuntu build & test | Windows, macOS |
| **edition lanes** (`minimal`/`community`/`enterprise`/`hyperscaler`/`military`) | All platforms (Ubuntu, Windows, macOS) | Security scan |

### Automated Release Workflow

When you push a version tag (e.g., `v1.4.0`), the release workflow automatically:

1. ✅ Validates version tag matches VERSION file
2. 🔨 Builds release binaries for:
   - Ubuntu (`.tar.gz`, `.deb`)
   - Windows (`.zip`)
   - macOS (`.tar.gz`)
3. 📦 Packages all binaries with CPack
4. 📝 Generates release notes from CHANGELOG.md and commits
5. 🚀 Creates GitHub Release with all artifacts
6. 📢 Publishes release (or marks as pre-release)

### Monitoring Releases

- **GitHub Actions**: [Actions Tab](https://github.com/makr-code/ThemisDB/actions)
- **Releases**: [Releases Page](https://github.com/makr-code/ThemisDB/releases)
- **Tags**: [Tags List](https://github.com/makr-code/ThemisDB/tags)

---

## 📦 Package Maintenance

> **Interested in maintaining ThemisDB packages for your platform?**  
> We welcome package maintainers for all distributions!

### Becoming a Package Maintainer

<details>
<summary><b>1️⃣ Review Packaging Documentation</b></summary>

- 📖 Read [docs/packaging.md](docs/packaging.md) for detailed instructions
- 📄 Check [docs/PACKAGING-QUICKREF.md](docs/PACKAGING-QUICKREF.md) for quick reference

</details>

<details>
<summary><b>2️⃣ Test Package Build</b></summary>

- 🔨 Build the package for your target platform
- 🧪 Install and test in a **clean environment**
- ✅ Verify all functionality works as expected

</details>

<details>
<summary><b>3️⃣ Submit to Distribution Repositories</b></summary>

- 📝 Follow platform-specific guidelines
- 📤 Submit package to appropriate repository (PPA, AUR, Copr, etc.)
- 📣 Notify us via GitHub issue when package is published

</details>

<details>
<summary><b>4️⃣ Keep Packages Updated</b></summary>

- 👀 Monitor releases and security updates
- ⚡ Update packages within **1-2 weeks** of new releases
- 🤝 Coordinate with core team for pre-release testing

</details>

### Supported Platforms

| Platform | Package Format | Repository |
|----------|---------------|------------|
| 🐧 **Linux** | `.deb`, `.rpm`, `PKGBUILD` | Debian/Ubuntu, Fedora/RHEL, Arch |
| 🪟 **Windows** | WinGet (`ThemisDB.ThemisDB`), Chocolatey | [winget-pkgs](https://github.com/microsoft/winget-pkgs), [chocolatey.org](https://chocolatey.org/) |

#### WinGet Package Maintainer Workflow

Manifests live in `packaging/winget/manifests/`. After each stable release:

1. Generate manifests from the published GitHub Release asset:
   ```powershell
   pwsh scripts/release/new-winget-manifest.ps1 \
       -Version <X.Y.Z> \
       -InstallerType zip \
       -InstallerUrl <URL_FROM_GITHUB_RELEASE> \
       -InstallerSha256 <HASH_FROM_GITHUB_RELEASE> \
       -IncludeGermanLocale
   ```
2. Validate locally: `winget validate --manifest packaging/winget/manifests/t/ThemisDB/ThemisDB/<X.Y.Z>`
3. Submit PR: `pwsh scripts/release/submit-winget-pkgs.ps1 -Version <X.Y.Z> -ForkOwner <github-username>`
4. Remove Draft status on the PR to trigger Microsoft's automated pipeline.

Pre-release versions (`-rc*`, `-alpha`, `-beta`) are submitted only after the stable version PR is merged. See `RELEASE_STRATEGY.md` § 9.1 for the full policy.
| 🍎 **macOS** | Homebrew Formula | [brew.sh](https://brew.sh/) |

### Automation Tools

Use provided scripts to prepare new releases:

```bash
# Linux/macOS
./scripts/prepare-release.sh 1.0.1

# Windows
.\scripts\prepare-release.ps1 -Version 1.0.1
```

> [!NOTE]
> These scripts automatically update version numbers across **all** packaging files.

### Package Maintainer Benefits

- 🏆 Listed as package maintainer in README
- 🚀 Early access to release candidates for testing
- 💬 Direct communication channel with core team
- 🎉 Recognition in release notes

---

## ❓ Questions?

| Resource | Purpose | Link |
|----------|---------|------|
| 💬 **GitHub Discussions** | General questions | [Discussions](https://github.com/makr-code/ThemisDB/discussions) |
| 🐛 **GitHub Issues** | Bug reports & features | [Issues](https://github.com/makr-code/ThemisDB/issues) |
| 🔒 **GitHub Security Advisories** | Private vulnerability reports | [Report](https://github.com/makr-code/ThemisDB/security/advisories/new) |
| 📚 **Documentation** | Detailed guides | [docs/](docs/) |

---

## � Contributing to GS3 (Gap Scanner V3)

ThemisDB includes **Gap Scanner V3**, a comprehensive gap detection system. Contributors are welcome to add new scanners!

### Overview

- **46 scanners** organized into 4 phases
- Each scanner inherits from `BaseGapScanner`
- Automatic impact classification (Severity × Impact)
- Auto-discovery via filesystem pattern matching

### Adding a New Scanner

#### 1. Identify Scanner Category

Choose the appropriate phase and category:

| Phase | Category | Use For |
|-------|----------|---------|
| **Phase 1** | `ai` | AI-Vibe specific patterns |
| **Phase 1** | `core` | C++ baseline issues |
| **Phase 1** | `check` | Syntactic validation |
| **Phase 2** | `safety` | Exception/input safety |
| **Phase 3** | `security` | Cryptography, hardening |
| **Phase 4** | `design` | Architecture rules |
| **Phase 4** | `quality` | Documentation standards |

#### 2. Create Scanner File

Create file in `tools/scanners/` following the naming convention:

```
gs3_step<N>_<category>_<name>.py
```

Example: `gs3_step01_core_memory_bounds.py`

#### 3. Implement Scanner Class

```python
"""
Memory bounds checking scanner.

Detects: Buffer overflows, out-of-bounds access, pointer arithmetic errors
"""

from tools.gs3_base_scanner import BaseGapScanner, GapSeverity


class MemoryBoundsScanner(BaseGapScanner):
    """Detects memory bounds violations."""
    
    def __init__(self):
        super().__init__(
            name="MemoryBounds",
            description="Detects buffer overflows and out-of-bounds access",
            phase=1,  # Phase number
        )
    
    def scan(self, codebase_dir):
        """Scan codebase for memory bounds issues."""
        
        # Your scanning logic here
        for file in self.collect_files(codebase_dir, pattern='*.cpp'):
            gaps = self.detect_memory_issues(file)
            
            for gap in gaps:
                # Add gap with automatic impact classification
                self._add_gap(
                    file=gap['file'],
                    line=gap['line'],
                    category=gap['category'],  # e.g., 'out_of_bounds'
                    message=gap['message'],
                    severity=gap['severity'],  # GapSeverity.HIGH
                    tool_tip=gap.get('details', '')
                )
        
        return self.gaps
    
    def detect_memory_issues(self, filepath):
        """Implementation of your scanning logic."""
        # TODO: Your detection logic
        return []
```

#### 4. Update Orchestrator (if needed)

If not using auto-discovery, update `tools/scanners/gs3_step00_uniform_full.py`:

```python
from scanners.gs3_step01_core_memory_bounds import MemoryBoundsScanner

# In UniformFullScanner.scan():
scanner = MemoryBoundsScanner()
self.gaps.extend(scanner.scan(codebase_dir))
```

*Note: Modern system uses auto-discovery, so manual import usually not needed.*

#### 5. Test Your Scanner

```bash
# Run just your scanner
python -c "
from tools.scanners.gs3_step01_core_memory_bounds import MemoryBoundsScanner
scanner = MemoryBoundsScanner()
gaps = scanner.scan('src')
print(f'Found {len(gaps)} gaps')
for gap in gaps[:3]:
    print(f'  {gap.file}:{gap.line} - {gap.message}')
"

# Run full GS3 test suite
python tools/test_gs3_integration.py

# List your scanner in GS3 CLI
python tools/gs3.py list-scanners --step 1
```

#### 6. Document Your Scanner

Add a docstring with:

```python
"""
Brief description of what this scanner detects.

Detects:
- Specific pattern 1
- Specific pattern 2
- Specific pattern 3

Supported:
- C++ detection patterns
- Regex patterns
- AST analysis

Not supported:
- Dynamic analysis
- Runtime detection

False positive rate: ~5-10% (estimated)
Performance: ~0.5s per 1MB of code
"""
```

#### 7. Add Tests

Create `tools/tests/test_gs3_step01_core_memory_bounds.py`:

```python
import unittest
from pathlib import Path
from tools.scanners.gs3_step01_core_memory_bounds import MemoryBoundsScanner


class TestMemoryBoundsScanner(unittest.TestCase):
    
    def setUp(self):
        self.scanner = MemoryBoundsScanner()
    
    def test_detects_buffer_overflow(self):
        # Test case for buffer overflow detection
        test_file = "test_data/buffer_overflow.cpp"
        gaps = self.scanner.scan(".")
        
        # Assert detection
        self.assertTrue(len(gaps) > 0)
    
    def test_no_false_positives(self):
        # Test safe code doesn't trigger
        test_file = "test_data/safe_code.cpp"
        gaps = self.scanner.scan(".")
        
        # Assert no false positives
        self.assertEqual(len(gaps), 0)


if __name__ == '__main__':
    unittest.main()
```

#### 8. Submit PR

1. Create feature branch: `feature/scanner-memory-bounds`
2. Implement and test scanner
3. Update `CHANGELOG.md` with scanner addition
4. Submit PR with:
   - Clear description of what the scanner detects
   - Test results (pass/fail counts)
   - Performance metrics (time per 1MB code)
   - Example gaps from real codebase

### Scanner Development Checklist

- ✅ File named `gs3_step<N>_<category>_<name>.py`
- ✅ Class inherits from `BaseGapScanner`
- ✅ All `_add_gap()` calls use proper severity/impact
- ✅ Comprehensive docstring with examples
- ✅ Unit tests with real code samples
- ✅ No external dependencies (use only std lib + ThemisDB imports)
- ✅ Performance acceptable (< 1s per 1000 files)
- ✅ Registered in orchestrator (if auto-discovery doesn't work)
- ✅ Listed in `tools/GS3_CLI_GUIDE.md`
- ✅ PR includes test results and metrics

### Key BaseGapScanner Methods

```python
# Available in your scanner class:

self._add_gap(
    file: str,           # File path
    line: int,           # Line number
    category: str,       # Gap category
    message: str,        # Human-readable message
    severity: GapSeverity,  # CRITICAL/HIGH/MEDIUM/LOW
    tool_tip: str = None # Additional details
)

self.collect_files(
    directory: str,      # Root directory
    pattern: str = "*.cpp"  # File pattern
) -> List[str]

self.read_file(filepath: str) -> str
```

### Example Scanners to Reference

- [gs3_step01_core_memory.py](tools/scanners/gs3_step01_core_memory.py)
- [gs3_step02_safety_exception.py](tools/scanners/gs3_step02_safety_exception.py)
- [gs3_step03_security_encryption_leak.py](tools/scanners/gs3_step03_security_encryption_leak.py)
- [gs3_step04_design_architecture.py](tools/scanners/gs3_step04_design_architecture.py)

### Resources

- [BaseGapScanner API](tools/gs3_base_scanner.py)
- [Impact Classifier](tools/scanners/gs3_impact_classifier.py)
- [GS3 CLI Guide](tools/GS3_CLI_GUIDE.md)
- [GS3 Integration Test Suite](tools/test_gs3_integration.py)

---

## �📄 License

> By contributing to ThemisDB, you agree that your contributions will be licensed under the **MIT License**.

---

<div align="center">

**🙏 Thank you for contributing to ThemisDB!**

[⭐ Star us on GitHub](https://github.com/makr-code/ThemisDB) · [📖 Read the Docs](https://makr-code.github.io/ThemisDB/) · [💬 Join Discussions](https://github.com/makr-code/ThemisDB/discussions)

</div>

---
Zuletzt geprueft (Root-Sync): 2026-06-21
