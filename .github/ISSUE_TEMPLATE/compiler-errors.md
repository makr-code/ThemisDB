---
name: Compiler Errors
about: Report massive compiler errors in Themis build system
title: '[COMPILER] '
labels: ['bug', 'build-system', 'high-priority', 'compiler-error']
assignees: ''
---

## Compiler Error Report

### Build Environment
- **OS:** [e.g., Windows 11, Ubuntu 22.04, macOS]
- **Compiler:** [e.g., MSVC 2022, GCC 11, Clang 15]
- **CMake Version:** [e.g., 3.28.0]
- **Build Configuration:** [e.g., Debug, Release]
- **vcpkg Version:** [if applicable]

### Build Command
```bash
# Command used to trigger the build
```

### Error Summary
<!-- Provide a brief summary of the compiler errors -->

### Full Error Output
<details>
<summary>Click to expand error log</summary>

```
Paste the full compiler error output here
```

</details>

### Affected Components
<!-- Check all that apply -->
- [ ] Core Library
- [ ] HTTP Server
- [ ] Tests
- [ ] Benchmarks
- [ ] Plugins
- [ ] Dependencies
- [ ] Other: ___________

### Error Categories
<!-- Check all that apply -->
- [ ] Syntax errors
- [ ] Linker errors
- [ ] Missing dependencies
- [ ] Header file issues
- [ ] Template instantiation errors
- [ ] Platform-specific errors
- [ ] vcpkg dependency issues
- [ ] CMake configuration errors

### Reproducibility
- [ ] Error occurs consistently
- [ ] Error occurs intermittently
- [ ] Error only on specific configuration

### Steps to Reproduce
1. 
2. 
3. 

### Expected Behavior
<!-- What should happen when building successfully -->

### Additional Context
<!-- Add any other context about the problem here -->
- Recent changes before errors appeared:
- Build system modifications:
- Dependency updates:

### Potential Root Cause
<!-- If you have insights into what might be causing the errors -->

### Workarounds Attempted
<!-- List any workarounds or fixes you've tried -->

### Related Issues
<!-- Link to related issues if any -->
Fixes #
Related to #

### Priority Assessment
<!-- Why should this be prioritized? -->
- [ ] Blocks development
- [ ] Blocks CI/CD pipeline
- [ ] Affects multiple developers
- [ ] Breaking change
- [ ] Security implications

---

**Note:** For massive compiler errors affecting multiple files or the entire build system, please attach the full build log file.
