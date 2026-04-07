# Repository Structure Documentation

This document describes the organizational structure of the ThemisDB repository and explains where different types of files should be placed.

## Root Directory

The root directory contains only essential files that are relevant for GitHub and the project's core functionality:

### GitHub Standard Files (Markdown)
- **README.md** - Project overview, quick start, and main documentation entry point
- **LICENSE** - MIT License
- **CHANGELOG.md** - Version history and release notes
- **CONTRIBUTING.md** - Contribution guidelines and development workflow
- **SECURITY.md** - Security policy and vulnerability reporting
- **CODE_OF_CONDUCT.md** - Community guidelines and code of conduct
- **SUPPORT.md** - How to get help and support

### Build System Files
- **CMakeLists.txt** - Main CMake build configuration
- **CMakePresets.json** - CMake build presets
- **vcpkg.json**, **vcpkg-configuration.json**, **vcpkg.docker.json** - Dependency management

### CI/CD Files
- **VERSION** - Current version number
- **RELEASE_TYPE** - Release type indicator (alpha, beta, stable)

### Documentation Build
- **mkdocs.yml**, **mkdocs-nopdf.yml** - MkDocs configuration for documentation site
- **requirements-docs.txt** - Python dependencies for documentation build

### Code Quality
- **sonar-project.properties** - SonarQube configuration

### IDE
- **themis.code-workspace** - VS Code workspace configuration

### Configuration Files (Hidden)
- **.gitignore** - Git ignore patterns
- **.gitattributes** - Git attributes
- **.gitmodules** - Git submodules
- **.dockerignore** - Docker build context exclusions
- **.editorconfig** - Editor configuration
- **.clang-format**, **.clang-tidy** - C++ code formatting
- **.cppcheck**, **.cppcheck-suppressions** - Static analysis
- **.gitleaks.toml** - Secret scanning configuration

## Directory Structure

### `/docs/` - Documentation
All project documentation is organized under this directory:
- **`/docs/api/`** - API reference documentation
- **`/docs/architecture/`** - Architecture and design documents
- **`/docs/build-guide/`** - Build system documentation
- **`/docs/reports/`** - Analysis and investigation reports
- **`/docs/de/`, `/docs/en/`, `/docs/fr/`** - Internationalized documentation
- **`/docs/features/`** - Feature documentation
- **`/docs/security/`** - Security documentation
- **`/docs/tools/`** - Tools documentation
- **Other documentation files** - CODING_STANDARDS.md, etc.

### `/scripts/` - Scripts and Tools
All utility scripts for development, building, and maintenance:
- Build scripts (`.sh`, `.ps1`)
- CI/CD helper scripts
- Maintenance tools (`.py`)
- Issue management scripts

### `/tests/` - Test Files
All test code and test utilities:
- Unit tests
- Integration tests
- Test utilities
- Standalone test files

### `/src/` - Source Code
Main application source code

### `/include/` - Header Files
Public header files

### `/examples/` - Example Code
Example applications and use cases

### `/clients/` - Client SDKs
Client libraries for different languages

### `/benchmarks/` - Performance Benchmarks
Benchmark code and results

### `/cmake/` - CMake Modules
CMake helper modules and scripts

### `/docker/` - Docker Files
Dockerfiles and container configuration

### `/deploy/` - Deployment Files
Deployment configurations and scripts

### `/helm/` - Kubernetes Helm Charts
Helm charts for Kubernetes deployment

### `/.github/` - GitHub Configuration
GitHub-specific configuration:
- **`.github/workflows/`** - GitHub Actions workflows
- **`.github/ISSUE_TEMPLATE/`** - Issue templates
- **`.github/CODEOWNERS`** - Code ownership
- **`.github/pull_request_template.md`** - PR template

## File Organization Guidelines

### When adding new files:

1. **Documentation**: Place in `/docs/` with appropriate subdirectory
2. **Scripts**: Place in `/scripts/` 
3. **Tests**: Place in `/tests/`
4. **Source code**: Place in `/src/` or `/include/`
5. **Examples**: Place in `/examples/`
6. **Configuration**: Place in `/config/` or root if it's a standard config file

### Root directory should ONLY contain:
- GitHub standard markdown files (README, CONTRIBUTING, etc.)
- Essential build system files (CMakeLists.txt, vcpkg.json)
- CI/CD metadata files (VERSION, RELEASE_TYPE)
- Documentation build configuration (mkdocs.yml)
- Standard configuration files (.gitignore, .editorconfig, etc.)

### What should NOT be in root:
- ❌ Build output files (*.log, *.txt output files)
- ❌ Generated files (_codeql_detected_source_root)
- ❌ Test files (test_*.cpp)
- ❌ Utility scripts (*.sh, *.ps1, *.py)
- ❌ Internal documentation (analysis reports, architecture docs)
- ❌ Temporary files

## Benefits of This Structure

1. **Clean root directory** - Easy to find essential files
2. **Better organization** - Files grouped by purpose
3. **Easier navigation** - Clear hierarchy
4. **GitHub best practices** - Follows community standards
5. **Scalability** - Structure supports growth
6. **Maintainability** - Easier to manage and update

## Migration Notes

Files were reorganized on 2026-01-14 to implement this structure:
- Moved 13 documentation markdown files from root to `/docs/`
- Moved 9 scripts from root to `/scripts/`
- Moved 2 test files from root to `/tests/`
- Removed build output files from version control
- Added CODE_OF_CONDUCT.md and SUPPORT.md to root
- Updated references in documentation files

## Related Documentation

- [CONTRIBUTING.md](../CONTRIBUTING.md) - How to contribute
- [docs/DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md) - Complete documentation index
- [.gitignore](../.gitignore) - Files excluded from version control

---

**Last Updated:** 2026-04-06
