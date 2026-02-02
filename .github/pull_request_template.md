# Pull Request - ThemisDB

## Description

<!-- Provide a clear and concise description of your changes -->

### Type of Change
- [ ] 🐛 Bug fix (non-breaking change that fixes an issue)
- [ ] ✨ New feature (non-breaking change that adds functionality)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to change)
- [ ] 📝 Documentation update
- [ ] ⚡ Performance improvement
- [ ] 🔒 Security fix
- [ ] 🧪 Test improvement

### Related Issues
<!-- Link related issues: Fixes #123, Closes #456, Relates to #789 -->

---

## Changes Made

<!-- List the key changes in this PR -->

- 
- 
- 

---

## Testing Checklist

### Basic Testing
- [ ] Code compiles without errors
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Test coverage maintained or improved (≥80% target)

### Code Quality
- [ ] Code follows project coding standards
- [ ] No compiler warnings introduced
- [ ] Static analysis checks pass (cppcheck, clang-tidy)
- [ ] No merge conflicts with base branch

### Security Checklist
- [ ] No hardcoded secrets or credentials
- [ ] Input validation implemented for user inputs
- [ ] No SQL/AQL injection vulnerabilities
- [ ] Appropriate error handling (no sensitive data in errors)
- [ ] Authentication/authorization checked if applicable
- [ ] Security-sensitive changes reviewed by security team

### Documentation
- [ ] Code comments added for complex logic
- [ ] API documentation updated (if applicable)
- [ ] README.md updated (if applicable)
- [ ] CHANGELOG.md updated with changes
- [ ] Migration guide provided for breaking changes

---

## Audit & Compliance (Auto-checked by CI)

The following will be automatically verified by the `audit-check` workflow:

### Automated Checks (CI)
- 🔍 SAST (Static Analysis) - cppcheck, clang-tidy
- 🔐 Secret Scanning - gitleaks
- 📊 Test Coverage - gcov
- 🐳 Container Security - Trivy (if Docker changes)
- 🔒 Dependency Vulnerabilities

### For Security-Sensitive Changes
If this PR involves authentication, encryption, input validation, or sensitive data:
- [ ] Security design review requested
- [ ] Threat modeling updated (if architectural changes)
- [ ] Security tests added
- [ ] OWASP ASVS requirements checked

---

## Performance Impact

- [ ] No performance degradation (benchmarks run if applicable)
- [ ] Performance improvements documented with metrics
- [ ] N/A - No performance-critical changes

---

## Deployment Considerations

- [ ] No database migrations required
- [ ] Database migrations documented and tested
- [ ] Configuration changes documented
- [ ] Backward compatible with previous version
- [ ] Breaking changes documented in CHANGELOG.md

---

## Additional Notes

<!-- Any additional information reviewers should know -->

---

## Reviewer Checklist

<!-- For reviewers - check these items during review -->

### Code Review
- [ ] Code logic is correct and efficient
- [ ] Error handling is appropriate
- [ ] Resource management (memory, connections) is proper
- [ ] Thread safety considered (if applicable)
- [ ] Security best practices followed

### Testing Review
- [ ] Test cases cover edge cases
- [ ] Tests are maintainable and clear
- [ ] Integration tests included for cross-module changes
- [ ] Performance tests added for critical paths

### Documentation Review
- [ ] Documentation is clear and accurate
- [ ] API changes properly documented
- [ ] Examples provided where helpful

---

## Audit Gate Status

<!-- Filled automatically by CI -->

**Audit Check Status:** 🔄 Running / ✅ Passed / ❌ Failed / ⚠️ Warnings

**Key Findings:**
<!-- CI will comment with any security or quality findings -->

---

## Pre-Merge Checklist

Before merging, ensure:
- [ ] All CI checks pass (audit-check, build-and-test)
- [ ] At least one approving review from maintainer
- [ ] All review comments addressed
- [ ] Branch is up-to-date with base branch
- [ ] No merge conflicts
- [ ] All audit findings resolved or documented

---

## Release Notes

<!-- If this should be included in release notes, provide a user-facing description -->

```markdown
### Added / Changed / Fixed / Security
- 
```

---

**For more information:**
- [Contributing Guide](../CONTRIBUTING.md)
- [Audit Framework](../docs/audit-framework/README.md)
- [Security Policy](../SECURITY.md)
