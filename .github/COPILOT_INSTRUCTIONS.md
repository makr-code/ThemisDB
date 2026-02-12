# ThemisDB - GitHub Copilot Instructions

## 🎯 Project Overview

**Name:** ThemisDB  
**Language:** C++17/20  
**Type:** High-Performance Hybrid Database (Graph + Vector + Relational)  
**Build System:** CMake 3.20+ with vcpkg  
**Architecture:** Multi-platform (Windows, Linux, ARM64, Docker)

### Purpose

ThemisDB is a high-performance C++ database system featuring:
- Vector database with RocksDB integration
- Advanced Query Language (AQL)
- Multi-Version Concurrency Control (MVCC)
- LLM integration via llama.cpp
- Cross-platform support (x86_64, ARM64)

## 📖 Detailed Module Documentation

For specific development areas, consult these focused guides:

### Core Development

- **[Git Branching & PRs](copilot/BRANCHING_GUIDE.md)** - Git Flow strategy, merge policies, PR guidelines
- **[Code Standards](copilot/CODE_STANDARDS.md)** - C++ style guide, naming conventions, code quality tools
- **[Testing](copilot/TESTING_GUIDE.md)** - Test requirements, coverage targets, test patterns

### Build & Deployment

- **[Build System](copilot/BUILD_GUIDE.md)** - CMake presets, platform builds, vcpkg integration
- **[Cross-Compilation](copilot/CROSS_COMPILATION_CONTEXT.md)** - Platform-specific rules, ARM64, Docker multi-arch
- **[VSCode Setup](copilot/VSCODE_CONTEXT.md)** - Remote development, debugging, IntelliSense

## ⚡ Quick Reference

### Code Style Examples

```cpp
// ✅ Modern C++17/20
auto result = database->query("SELECT * FROM users");

// ✅ RAII patterns
std::unique_ptr<Connection> conn = pool->acquire();

// ✅ Naming conventions
class VectorIndex {};              // PascalCase for classes
void processQuery() {};            // camelCase for functions
int max_retries_{3};               // snake_case_ for members
const int MAX_CONNECTIONS = 100;   // UPPER_CASE for constants
```

### Branching Workflow

```bash
# Feature development
git checkout develop
git pull origin develop
git checkout -b feature/xyz

# Create PR to develop (NOT main!)
```

### PR Title Format

```
<type>(<scope>): <description>

Examples:
feat(storage): Add vector search optimization
fix(aql): Resolve memory leak in query parser
docs(api): Update REST API documentation
```

### Build Commands

**VSCode (Recommended):**
```
Use CMake Tools extension for integrated build management:
- Command Palette → "CMake: Select Configure Preset"
- Press F7 or Command Palette → "CMake: Build"
- Automatically uses presets from cmake/CMakePresets.json
```

**Command Line:**
```bash
# Windows (MSVC)
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Linux (GCC)
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel $(nproc)

# Docker
docker build -f docker/Dockerfile.themis-server -t themis-server .
```

### Testing

```bash
# Run all tests
cmake --build build --target test

# Run specific test suite
./build/tests/storage_tests --gtest_filter="RocksDB*"

# Coverage
cmake -B build -DTHEMIS_ENABLE_COVERAGE=ON
cmake --build build --target coverage
```

## 🔧 Development Guidelines

### Development Toolchain

**Primary IDE Setup:**
- **VSCode** with CMake Tools extension (ms-vscode.cmake-tools)
- CMake presets automatically detected from `cmake/CMakePresets.json`
- Build/test/debug integrated via CMake Tools UI
- IntelliSense configured via compile_commands.json

**Build Management:**
- Use CMake Tools extension for all build operations
- Presets handle platform-specific configuration
- Parallel builds automatically configured
- See [VSCode Context](copilot/VSCODE_CONTEXT.md) for setup

### What Copilot Should Help With

1. **Generate idiomatic C++ code** for:
   - Core algorithms and data structures
   - Concurrency control and thread safety
   - Memory-safe patterns with RAII
   - Query processing and optimization

2. **Suggest tests** for:
   - Correctness verification
   - Performance benchmarks
   - Edge cases and error handling
   - Regression prevention

3. **Avoid unsafe patterns:**
   - Manual memory management (new/delete)
   - Raw pointers for ownership
   - Race conditions
   - Undefined behavior

### Code Quality Requirements

- **Style:** Follow project `.clang-format` and `.clang-tidy` configs
- **Modern C++:** Prefer C++17/20 features over legacy patterns
- **Thread Safety:** Document locking strategies in code/docs
- **Testing:** Unit tests for new features, regression tests for bug fixes
- **Coverage:** Aim for 80%+ on core modules

### Documentation Expectations

- **Architecture:** Document major design decisions in `docs/architecture.md`
- **API Design:** Describe AQL semantics, indexing strategy in `docs/design.md`
- **Build Changes:** Update relevant build guide when modifying CMake
- **Breaking Changes:** Clearly document in PR description

## 📋 GitHub Labels

**Required Labels for Issues/PRs:**
- **Priority:** `priority:P0` (Critical) to `priority:P3` (Low)
- **Type:** `type:bug`, `type:feature`, `type:enhancement`, `type:documentation`

**Optional Labels:**
- **Area:** `area:storage`, `area:aql`, `area:llm`, `area:build`, etc.
- **Effort:** `effort:small`, `effort:medium`, `effort:large`

**Label Validation:**
- Always use labels from `.github/labels.yml`
- See [LABELS_GUIDE.md](LABELS_GUIDE.md) for details

## 🏗️ Project Structure

```
themis/
├── .github/
│   ├── copilot/              # Copilot instruction modules
│   ├── workflows/            # CI/CD pipelines
│   └── ISSUE_TEMPLATE/       # Issue templates
├── cmake/                    # Build system (CMakeLists.txt, presets)
├── docker/                   # Dockerfiles and compose files
├── docs/                     # Documentation
├── src/                      # Source code
├── include/                  # Public headers
├── tests/                    # Test suites
├── benchmarks/               # Performance benchmarks
└── vcpkg/                    # Package management
```

## 🚀 Getting Started

### For New Contributors

1. **Read Core Docs:**
   - [../CONTRIBUTING.md](../CONTRIBUTING.md)
   - [../ARCHITECTURE.md](../ARCHITECTURE.md)
   - [Branching Guide](copilot/BRANCHING_GUIDE.md)

2. **Setup Development Environment:**
   - Follow [VSCode Setup](copilot/VSCODE_CONTEXT.md)
   - Install pre-commit hooks (see below)
   - Install **CMake Tools extension** (ms-vscode.cmake-tools)
   - Configure CMake presets (auto-detected by CMake Tools)

3. **First Build:**
   - **VSCode:** Press F7 or use CMake Tools extension
   - **CLI:** See [Build Guide](copilot/BUILD_GUIDE.md)
   - Start with minimal build (no LLM/GPU)
   - Verify tests pass

### Common Tasks

| Task | Guide |
|------|-------|
| Add new feature | [Code Standards](copilot/CODE_STANDARDS.md) + [Testing](copilot/TESTING_GUIDE.md) |
| Fix bug | [Branching Guide](copilot/BRANCHING_GUIDE.md) → Create bugfix/* branch |
| Build for ARM | [Cross-Compilation](copilot/CROSS_COMPILATION_CONTEXT.md) |
| Setup Docker | [Build Guide](copilot/BUILD_GUIDE.md) → Docker section |
| Debug issue | [VSCode Context](copilot/VSCODE_CONTEXT.md) → Debugging |

## 🛡️ Quality Assurance

### Pre-Commit Validation

Before committing:
```bash
# Format code
clang-format -i src/**/*.cpp

# Run linter
clang-tidy src/**/*.cpp

# Run tests
cmake --build build --target test
```

### CI Pipeline

All PRs must pass:
- ✅ Build on Windows (MSVC) + Linux (GCC)
- ✅ All tests passing
- ✅ Code coverage ≥ 70%
- ✅ Static analysis (clang-tidy, cppcheck)
- ✅ Security scan (CodeQL)

## 📚 Additional Resources

### Core Documentation
- [../README.md](../README.md) - Project overview
- [../ARCHITECTURE.md](../ARCHITECTURE.md) - System design
- [../CHANGELOG.md](../CHANGELOG.md) - Version history
- [../SECURITY.md](../SECURITY.md) - Security policies

### Build Guides
- [../docs/build-guide/BUILD_WINDOWS.md](../docs/build-guide/BUILD_WINDOWS.md)
- [../docs/build-guide/BUILD_LINUX.md](../docs/build-guide/BUILD_LINUX.md)
- [../docs/build-guide/BUILD_DOCKER.md](../docs/build-guide/BUILD_DOCKER.md)

### Development
- [../CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
- [../CODE_OF_CONDUCT.md](../CODE_OF_CONDUCT.md) - Community standards

## 💡 Copilot Best Practices

### DO:
- ✅ Reference module docs for detailed context
- ✅ Follow existing code patterns in the repository
- ✅ Write tests for all new functionality
- ✅ Document complex algorithms and thread safety
- ✅ Use modern C++ idioms (RAII, smart pointers, ranges)

### DON'T:
- ❌ Mix platform-specific code without guards (`#ifdef`)
- ❌ Introduce dependencies without checking vcpkg availability
- ❌ Commit directly to `main` or `develop`
- ❌ Skip tests for "simple" changes
- ❌ Use unsafe patterns (raw pointers, manual memory management)

---

**Last Updated:** 2026-02-12  
**Copilot Version:** Modular Architecture v1.0
