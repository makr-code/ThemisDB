## Description

<!-- Provide a clear and concise description of your changes -->

## Type of Change

<!-- Mark the relevant option with an [x] -->

- [ ] 🐛 Bug fix (non-breaking change which fixes an issue)
- [ ] ✨ New feature (non-breaking change which adds functionality)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] 📝 Documentation update
- [ ] ♻️ Code refactoring (no functional changes)
- [ ] ⚡ Performance improvement
- [ ] ✅ Test addition or update
- [ ] 🔧 Configuration change
- [ ] 🎨 UI/UX change

## Related Issues

<!-- Link to related issues using #issue_number -->

Closes #
Relates to #

## Changes Made

<!-- List the key changes made in this PR -->

- 
- 
- 

## Testing

<!-- Describe the tests you ran and how to reproduce them -->

### Test Environment
- **OS**: <!-- e.g., Ubuntu 22.04, Windows 11, macOS 13 -->
- **Compiler**: <!-- e.g., GCC 11, MSVC 2019, Clang 14 -->
- **Build Type**: <!-- e.g., Release, Debug -->

### Test Results
- [ ] All existing tests pass
- [ ] New tests added for changes
- [ ] Manual testing performed

### Test Commands
```bash
# Commands used to test the changes
cmake -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Checklist

<!-- Verify all items before submitting -->

- [ ] My code follows the [coding standards](../CODING_STANDARDS.md)
- [ ] I have performed a self-review of my code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have updated the documentation accordingly
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing unit tests pass locally with my changes
- [ ] Any dependent changes have been merged and published

## Code Quality

- [ ] Code builds without errors
- [ ] Code builds without warnings
- [ ] Static analysis (cppcheck) passes
- [ ] No memory leaks detected
- [ ] Code follows C++17 standards

## Documentation

- [ ] README.md updated (if applicable)
- [ ] CHANGELOG.md updated
- [ ] API documentation updated (if applicable)
- [ ] Code comments added/updated

### 📝 Documentation Review Checklist

<!-- Complete this checklist for all PRs with code changes -->
<!-- See docs/PR_DOCUMENTATION_CHECKLIST.md for detailed guidelines -->

#### Documentation Impact Assessment

- [ ] This PR changes **no** documentation requirements (skip to sign-off below)
- [ ] This PR affects existing documentation (complete checklist below)
- [ ] This PR requires new documentation (complete checklist below)

**Affected Areas** (check all that apply):
- [ ] User-facing features
- [ ] API/Interface changes
- [ ] Configuration options
- [ ] Command-line tools
- [ ] Installation/deployment
- [ ] Architecture/design
- [ ] Performance characteristics
- [ ] Security considerations
- [ ] Migration/upgrade path

#### Documentation Updates Made

**Updated Files:**
- [ ] Main README.md (if applicable)
- [ ] User documentation in `/docs/`
- [ ] API documentation (code comments/OpenAPI)
- [ ] Example code in `/examples/`
- [ ] Configuration examples in `/config/`
- [ ] CHANGELOG.md entry
- [ ] Migration guides (if breaking change)

**Documentation Links:**
<!-- Provide links to updated documentation files -->
- 
- 

#### Accuracy & Completeness

- [ ] All code examples compile and run
- [ ] Command-line examples are tested
- [ ] Configuration examples are valid
- [ ] All new features/API changes are documented
- [ ] Prerequisites are listed
- [ ] Common use cases are covered

#### Quality & Review

- [ ] Language is clear and concise
- [ ] Examples are helpful and realistic
- [ ] Follows documentation style guide
- [ ] Documentation reviewed by at least one person
- [ ] Documentation builds without errors (`mkdocs build --strict`)
- [ ] Links validated (no broken links)

#### Documentation Sign-Off

- [ ] ✅ All required documentation is complete and reviewed
- [ ] 📋 Documentation debt tracked in issues (list issue numbers below)
- [ ] 🚫 No documentation required (explain why below)

**Explanation/Notes:**
<!-- If no documentation required or documentation debt exists, explain here -->

**Full Checklist:** See [docs/PR_DOCUMENTATION_CHECKLIST.md](../docs/PR_DOCUMENTATION_CHECKLIST.md) for detailed guidelines

## Branch Strategy Compliance

<!-- Ensure your PR follows the Git Flow branching strategy -->

- [ ] PR targets the correct branch (`develop` for features, `main` for releases/hotfixes)
- [ ] Branch naming follows convention (e.g., `feature/`, `bugfix/`, `hotfix/`, `release/`)
- [ ] No direct commits to `main` or `develop`

## Performance Impact

<!-- If applicable, describe any performance implications -->

- [ ] No significant performance impact
- [ ] Performance improvement (describe below)
- [ ] Performance regression (justify below)

**Performance Notes:**
<!-- Add performance benchmarks or profiling results if applicable -->

## Breaking Changes

<!-- If this PR introduces breaking changes, describe them here -->

**Breaking Change Details:**
<!-- Explain what breaks and how users should migrate -->

## Security Considerations

- [ ] No security implications
- [ ] Security review required
- [ ] Dependencies updated to secure versions

## Additional Notes

<!-- Any additional information that reviewers should know -->

## Screenshots/Logs

<!-- If applicable, add screenshots or logs to help explain your changes -->

---

**For Maintainers:**

### Review Checklist
- [ ] Code quality acceptable
- [ ] Tests adequate
- [ ] Documentation complete
- [ ] No security concerns
- [ ] Ready to merge

### Merge Strategy
- [ ] **Squash and merge** (✅ Recommended for feature/bugfix PRs - cleaner history)
- [ ] Merge commit (Only for release/hotfix branches)
- [ ] Rebase and merge
