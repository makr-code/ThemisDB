# PR Documentation Checklist

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Status:** Official Template

---

## Purpose

This checklist ensures that all code changes are properly documented before merging. Use this template when creating or reviewing pull requests.

## Quick Reference

✅ **Required for all PRs with code changes**  
📝 **Required for feature/API changes**  
🔄 **Required for breaking changes**  
📚 **Optional but recommended**

---

## Documentation Checklist Template

Copy this checklist into your PR description:

```markdown
## 📝 Documentation Checklist

### 1. Documentation Impact Assessment

- [ ] This PR changes **no** documentation requirements (skip to sign-off)
- [ ] This PR affects existing documentation (continue checklist)
- [ ] This PR requires new documentation (continue checklist)

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

### 2. Documentation Updates Made

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
- 

### 3. Accuracy Verification

- [ ] All code examples compile and run
- [ ] Command-line examples are tested
- [ ] Configuration examples are valid
- [ ] API examples use correct syntax
- [ ] Version numbers are accurate
- [ ] Links are not broken

### 4. Completeness Check

- [ ] All new features are documented
- [ ] All API changes are documented
- [ ] Prerequisites are listed
- [ ] Common use cases are covered
- [ ] Error messages/codes are documented
- [ ] Limitations are noted

### 5. Quality Assurance

- [ ] Language is clear and concise
- [ ] Examples are helpful and realistic
- [ ] Technical terms are explained
- [ ] Follows documentation style guide
- [ ] Consistent with existing documentation
- [ ] Translations updated (if applicable)

### 6. Review and Validation

- [ ] Documentation reviewed by at least one person
- [ ] Documentation builds without errors (`mkdocs build --strict`)
- [ ] Links validated (no broken links)
- [ ] Spelling and grammar checked

### 7. Future Documentation Tasks

**Follow-up Issues Created:**
<!-- List any documentation work that needs to be done later -->
- [ ] None - all documentation complete
- [ ] Issue #XXX - Additional examples needed
- [ ] Issue #XXX - Translation required
- [ ] Issue #XXX - Performance benchmarks to add

### 8. Sign-Off

**Documentation Status:**
- [ ] ✅ All required documentation is complete and reviewed
- [ ] 📋 Documentation debt tracked in issues (linked above)
- [ ] 🚫 No documentation required (explain why):
  <!-- Explanation required if checking this box -->

**Reviewer Verification:**
- [ ] @reviewer-name verified documentation accuracy
- [ ] @reviewer-name verified documentation completeness

---

**Notes:**
<!-- Additional notes or context -->

```

---

## Detailed Guidelines

### 1. Documentation Impact Assessment

**How to assess:**
1. Review your code changes
2. Identify user-facing changes
3. Determine what documentation is affected
4. Check if new documentation is needed

**Common scenarios:**

| Change Type | Documentation Required |
|-------------|----------------------|
| New feature | ✅ Full documentation (user guide, API, examples) |
| API change (breaking) | 🔄 Update API docs, migration guide, examples |
| API change (non-breaking) | ✅ Update API docs, add examples |
| Bug fix (behavior change) | ✅ Update docs if documented behavior changes |
| Bug fix (no behavior change) | 📚 Optional - add to CHANGELOG |
| Internal refactoring | 📚 Optional - update architecture docs |
| Performance improvement | ✅ Update performance docs, benchmarks |
| Security fix | ✅ Update security docs, CHANGELOG |
| Configuration change | ✅ Update config docs, examples |
| Deprecation | 🔄 Add deprecation notice, migration guide |

### 2. Documentation Updates Made

**Required files to update:**

#### For User-Facing Features
- User guide in `/docs/`
- Quick start guide (if applicable)
- Examples in `/examples/`
- CHANGELOG.md

#### For API Changes
- API documentation (code comments)
- OpenAPI specification (if REST API)
- Client library documentation
- API migration guide (if breaking)

#### For Configuration Changes
- Configuration reference
- Example configurations
- Deployment guides
- Docker/container documentation

#### For Breaking Changes
- CHANGELOG.md (with ⚠️ BREAKING CHANGE)
- Migration guide from old to new
- Deprecation notices
- Version compatibility matrix

### 3. Accuracy Verification

**Test all examples:**

```bash
# Compile code examples
cd examples/your-feature
mkdir build && cd build
cmake ..
make

# Run examples
./example-binary

# Verify output matches documentation
```

**Validate configuration examples:**
```bash
# Check configuration syntax
./scripts/validate-config.sh config/example.yaml

# Test with actual system
./themisdb --config config/example.yaml --dry-run
```

**Check links:**
```bash
# Validate internal links
./scripts/check-links.sh

# Build documentation
mkdocs build --strict
```

### 4. Completeness Check

**Ask yourself:**
- Can a new user understand this feature?
- Are prerequisites clearly stated?
- Are error scenarios documented?
- Is there at least one working example?
- Are performance implications noted?
- Are security considerations mentioned?

**Common gaps to avoid:**
- Missing prerequisites
- No examples
- Undocumented error codes
- Missing configuration options
- No migration path for breaking changes
- Undocumented limitations

### 5. Quality Assurance

**Style guidelines:**

✅ **Good:**
```markdown
## Configuring Connection Pooling

Set the maximum number of connections in `themis.yaml`:

\```yaml
database:
  max_connections: 100
\```

This limits the connection pool to 100 concurrent connections.
```

❌ **Bad:**
```markdown
## Connection Pooling

You can configure connections.

\```yaml
max_connections: 100
\```
```

**Writing tips:**
- Use active voice
- Be specific with numbers/values
- Provide context for examples
- Explain "why" not just "how"
- Use consistent terminology

### 6. Review and Validation

**Self-review checklist:**
1. Read documentation as if you're new to the project
2. Follow your own examples step by step
3. Verify all commands work
4. Check that links aren't broken
5. Ensure consistency with existing docs

**Ask reviewers to verify:**
- Technical accuracy
- Clarity for target audience
- Completeness of coverage
- Example correctness

### 7. Future Documentation Tasks

**When to create follow-up issues:**
- Large documentation effort beyond PR scope
- Translations needed
- Advanced examples to be added later
- Performance benchmarks to be run
- Video tutorials to be created

**Issue template:**
```markdown
## Documentation Follow-up

**Related PR:** #XXX

**What needs to be documented:**
- [ ] Task 1
- [ ] Task 2

**Target milestone:** vX.X.X
**Priority:** P1/P2/P3

**Context:**
Brief explanation of what documentation is needed and why.
```

### 8. Sign-Off

**When documentation is complete:**
- All required updates made
- Examples tested
- Links validated
- At least one reviewer approved

**When documentation has debt:**
- Critical documentation complete
- Follow-up issues created and linked
- Debt is acceptable (explain why)

**When no documentation required:**
- Internal-only changes
- Documentation would be misleading
- Must provide explanation

---

## Examples

### Example 1: New Feature PR

```markdown
## 📝 Documentation Checklist

### 1. Documentation Impact Assessment
- [x] This PR requires new documentation

**Affected Areas:**
- [x] User-facing features
- [x] API/Interface changes
- [x] Configuration options
- [x] Example code

### 2. Documentation Updates Made

**Updated Files:**
- [x] docs/features/vector-search.md (NEW)
- [x] docs/api/query-api.md (UPDATED)
- [x] examples/vector-search/ (NEW)
- [x] CHANGELOG.md

**Documentation Links:**
- [Vector Search Guide](docs/features/vector-search.md)
- [Query API Updates](docs/api/query-api.md)
- [Examples](examples/vector-search/)

### 3-6. Verification Complete
- [x] All checked and verified

### 7. Future Documentation Tasks
- [ ] Issue #1234 - Add performance benchmarks
- [ ] Issue #1235 - Create video tutorial

### 8. Sign-Off
- [x] ✅ All required documentation is complete and reviewed
- [x] @alice verified documentation accuracy
- [x] @bob verified documentation completeness
```

### Example 2: Bug Fix PR (No Behavior Change)

```markdown
## 📝 Documentation Checklist

### 1. Documentation Impact Assessment
- [x] This PR changes **no** documentation requirements

**Reason:**
Internal bug fix that doesn't change documented behavior. 
Fixed memory leak in internal connection pool implementation.
Users won't notice any functional changes.

### 8. Sign-Off
- [x] 🚫 No documentation required
- [x] CHANGELOG.md updated with bug fix note
```

### Example 3: Breaking API Change

```markdown
## 📝 Documentation Checklist

### 1. Documentation Impact Assessment
- [x] This PR requires new documentation

**Affected Areas:**
- [x] API/Interface changes (BREAKING)
- [x] Migration/upgrade path
- [x] Example code

### 2. Documentation Updates Made

**Updated Files:**
- [x] docs/api/authentication-api.md
- [x] docs/migration/v2-to-v3.md (NEW)
- [x] examples/authentication/
- [x] CHANGELOG.md (⚠️ BREAKING CHANGE noted)

**Documentation Links:**
- [Updated API Docs](docs/api/authentication-api.md)
- [Migration Guide v2→v3](docs/migration/v2-to-v3.md)

### 3-6. Verification Complete
- [x] All examples tested against new API
- [x] Migration guide tested with real v2 applications

### 7. Future Documentation Tasks
- [x] None - all documentation complete

### 8. Sign-Off
- [x] ✅ All required documentation is complete and reviewed
- [x] @charlie verified technical accuracy
- [x] @dana verified migration guide works
```

---

## Integration with PR Template

This checklist should be integrated into the PR template at `.github/pull_request_template.md`. See [PR_TEMPLATE_INTEGRATION.md](PR_TEMPLATE_INTEGRATION.md) for details.

## Related Documentation

- **Review Guidelines**: [DOCUMENTATION_REVIEW_GUIDELINES.md](DOCUMENTATION_REVIEW_GUIDELINES.md)
- **Contributing Guide**: [CONTRIBUTING.md](../CONTRIBUTING.md)
- **Archival Process**: [DOCUMENTATION_ARCHIVAL_PROCESS.md](DOCUMENTATION_ARCHIVAL_PROCESS.md)

---

**Questions?** Open an issue with the `docs` label.
