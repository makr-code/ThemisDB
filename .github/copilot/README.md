# ThemisDB AI-Guardrails & Development Setup

This directory contains modular Copilot instructions and development tooling for ThemisDB.

## 📂 Directory Structure

```
.github/
├── COPILOT_INSTRUCTIONS.md         # Main entry point (264 lines)
├── copilot/                         # Modular instruction files
│   ├── BRANCHING_GUIDE.md           # Git Flow & PR Guidelines
│   ├── BUILD_GUIDE.md               # CMake Presets & Build Commands
│   ├── CODE_STANDARDS.md            # C++ Style & Quality Tools
│   ├── CUDA_OPTIMIZATION.md         # GPU Kernel Design & Memory Optimization
│   ├── MVCC_CONCURRENCY.md          # MVCC, Transactions & Thread Safety
│   ├── PERFORMANCE_PROFILING.md     # GPU/CPU Profiling & Benchmarking
│   ├── TESTING_GUIDE.md             # Test Requirements & Coverage
│   ├── CROSS_COMPILATION_CONTEXT.md # Platform-specific Rules
│   └── VSCODE_CONTEXT.md            # VSCode Remote Development
├── scripts/
│   └── validate_copilot_refs.py     # Validate file references
└── workflows/
    └── validate-ai-guardrails.yml   # CI validation workflow
```

## 🎯 Purpose

The modular AI-Guardrails architecture provides:

1. **Predictable Copilot Behavior** - Clear, focused instructions per domain
2. **Maintainability** - Easy to update individual modules without conflicts
3. **Discoverability** - Logical organization by topic
4. **Validation** - Automated checks for broken references

## 🚀 Quick Start

### For Contributors

1. **Read Main Instructions**: Start with [../COPILOT_INSTRUCTIONS.md](../COPILOT_INSTRUCTIONS.md)
2. **Dive into Modules**: Follow links to relevant guides
3. **Setup Dev Environment**: Use [VSCODE_CONTEXT.md](VSCODE_CONTEXT.md)

### For Maintainers

1. **Validate Changes**:
   ```bash
   python .github/scripts/validate_copilot_refs.py
   ```

2. **Update Module**: Edit specific module file (e.g., `copilot/BUILD_GUIDE.md`)

3. **Keep Main File Small**: Main file should stay under 300 lines

## 📖 Module Descriptions

### BRANCHING_GUIDE.md
- Git Flow branching strategy
- PR creation workflow
- Merge strategies (squash vs merge commit)
- Branch naming conventions

### BUILD_GUIDE.md
- CMake presets for all platforms
- vcpkg offline-first architecture
- Quick start commands (Windows, Linux, Docker, ARM)
- Edition-specific builds (Community, Enterprise, Hyperscaler)
- Troubleshooting common issues

### CODE_STANDARDS.md
- C++ coding style (C++17/20)
- Naming conventions (PascalCase, camelCase, snake_case)
- Code quality tools (clang-format, clang-tidy, cppcheck)
- Thread safety patterns
- Error handling guidelines

### TESTING_GUIDE.md
- Test framework (Google Test)
- Running tests (all, specific, critical)
- Test categories (unit, integration, benchmark)
- Code coverage targets
- Mock/fixture patterns

### CROSS_COMPILATION_CONTEXT.md
- Platform support matrix
- Platform-specific compiler flags
- SIMD optimizations (x86 SSE/AVX, ARM NEON)
- vcpkg triplet configuration
- Docker multi-architecture builds

### CUDA_OPTIMIZATION.md
- CUDA kernel design best practices (block/grid sizing, warp-aware programming)
- Memory optimization (coalesced access, shared memory, pinned memory)
- SIMD and vector operations (x86 AVX2, ARM NEON)
- ThemisDB-specific GPU patterns (vector search, geospatial, graph traversal)

### MVCC_CONCURRENCY.md
- MVCC fundamentals (version numbering, snapshot vs. serializable isolation)
- Lock strategies (optimistic/pessimistic, lock modes, deadlock detection)
- Transaction lifecycle (begin → read/write → commit/rollback)
- Thread safety patterns (RWLocks, atomics, memory ordering)
- Version garbage collection and common pitfalls

### PERFORMANCE_PROFILING.md
- GPU profiling with Nsight Compute and Nsight Systems
- CPU profiling with perf, VTune, and Callgrind
- Memory access pattern and cache hit-rate analysis
- Google Benchmark setup and regression detection
- CI integration for automated benchmark runs

### VSCODE_CONTEXT.md
- VSCode setup and extensions
- Remote development (WSL, SSH, Dev Containers)
- CMake integration
- IntelliSense configuration
- Debugging workflows
- Code formatting

## 🔍 Validation

### Automated Checks

The CI pipeline validates:
- ✅ All file references exist
- ✅ Main file stays under 300 lines
- ✅ Required modules present
- ✅ Markdown linting

### Manual Validation

```bash
# Validate file references
python .github/scripts/validate_copilot_refs.py

# Check main file size
wc -l .github/COPILOT_INSTRUCTIONS.md
```

## ✏️ Editing Guidelines

### When to Create a New Module

Create a new module when:
- Topic is self-contained (>200 lines of content)
- Multiple sections relate to the same domain
- Content will be referenced frequently
- Reduces complexity in main file

### When to Update Existing Module

Update a module when:
- Instructions change for that domain
- New tools/processes are introduced
- Troubleshooting steps are discovered
- Examples need clarification

### Keeping Main File Clean

The main `COPILOT_INSTRUCTIONS.md` should:
- Provide high-level overview
- Link to detailed modules
- Show quick reference examples
- Stay under 300 lines (target: 200)

### Style Guidelines

1. **Use Headers** - Clear section structure
2. **Code Examples** - Show good vs bad patterns
3. **Tables** - For comparison/reference data
4. **Links** - Reference related docs
5. **Emojis** - Visual cues (optional, use sparingly)

## 🔄 Update Process

1. **Make Changes** - Edit relevant module(s)
2. **Validate** - Run validation script
3. **Test Links** - Click through references
4. **Commit** - Use conventional commits
5. **CI Checks** - Ensure all checks pass

## 🛠️ Tooling

### Validation Script

```python
# .github/scripts/validate_copilot_refs.py
- Checks all markdown links in Copilot instructions
- Validates file existence
- Reports broken references
```

### CI Workflow

```yaml
# .github/workflows/validate-ai-guardrails.yml
- Runs on push/PR to relevant files
- Validates references
- Checks file size
- Lints markdown
- Verifies module structure
```

### Pre-commit Hooks

```yaml
# .pre-commit-config.yaml
- Markdown linting
- YAML validation
- Secret detection
- Custom validation hook
```

## 📊 Metrics

### Before Refactoring
- Single file: 501 lines
- Complex, hard to navigate
- No validation
- Conflicting instructions

### After Refactoring
- Main file: 264 lines (-47%)
- 9 focused modules
- Automated validation
- Clear organization

## 🤝 Contributing

When contributing to AI-Guardrails:

1. **Follow Structure** - Keep modules focused
2. **Validate Changes** - Run validation script
3. **Update Links** - Keep cross-references current
4. **Document Changes** - Update this README if needed

## 📚 Related Documentation

- [CONTRIBUTING.md](../../CONTRIBUTING.md) - Contribution guidelines
- [ARCHITECTURE.md](../../ARCHITECTURE.md) - System architecture
- [.devcontainer/](../../.devcontainer/devcontainer.json) - Dev Container setup
- [.pre-commit-config.yaml](../../.pre-commit-config.yaml) - Pre-commit hooks

## ❓ FAQ

### Q: Why split into modules?

A: The monolithic 501-line file was:
- Hard to maintain
- Contained contradictory instructions
- Difficult to navigate
- No way to validate references

Modular structure provides:
- Clear separation of concerns
- Easier maintenance
- Better organization
- Automated validation

### Q: How often should modules be updated?

A: Update modules when:
- Build process changes
- Coding standards evolve
- New tools are adopted
- Best practices are discovered

### Q: Can I add new modules?

A: Yes! Follow these steps:
1. Create module in `.github/copilot/`
2. Add reference in main file
3. Update validation if needed
4. Document in this README

### Q: What if validation fails?

A: Common issues:
- **Broken link**: Update path or create missing file
- **File too large**: Move content to module
- **Missing module**: Create required module file

## 📞 Support

For questions or issues:
- Open an issue with label `area:documentation`
- Consult [CONTRIBUTING.md](../../CONTRIBUTING.md)
- Ask in team chat/discussions

---

**Version:** 1.0  
**Last Updated:** 2026-02-12  
**Maintainer:** ThemisDB Team
