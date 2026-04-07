# Documentation Continuous Improvement - Quick Reference

**Version:** 1.0  
**Last Updated:** 2026-04-06

---

## 🎯 Overview

This quick reference guide provides a summary of the continuous documentation improvement and review process for ThemisDB.

## 📚 Core Documents

| Document | Purpose | When to Use |
|----------|---------|-------------|
| [DOCUMENTATION_REVIEW_GUIDELINES.md](DOCUMENTATION_REVIEW_GUIDELINES.md) | Complete review process guidelines | Learn review process, standards, and best practices |
| [PR_DOCUMENTATION_CHECKLIST.md](PR_DOCUMENTATION_CHECKLIST.md) | PR documentation checklist template | Every PR with code changes |
| [DOCUMENTATION_MERGE_PROTOCOL.md](DOCUMENTATION_MERGE_PROTOCOL.md) | Merge decision documentation | Before merging documentation PRs |
| [DOCUMENTATION_REVIEW_SCHEDULE.md](DOCUMENTATION_REVIEW_SCHEDULE.md) | Review calendar and templates | Plan and execute scheduled reviews |
| [DOCUMENTATION_ARCHIVAL_PROCESS.md](DOCUMENTATION_ARCHIVAL_PROCESS.md) | Archive outdated documentation | When documentation becomes outdated |

## ⚡ Quick Actions

### For Contributors

**Before submitting a PR with code changes:**
1. ✅ Check if documentation needs updating
2. ✅ Update affected documentation files
3. ✅ Add/update examples if needed
4. ✅ Complete PR documentation checklist
5. ✅ Ensure CHANGELOG.md is updated
6. ✅ Run `mkdocs build --strict` to verify

**Checklist location:** [PR_DOCUMENTATION_CHECKLIST.md](PR_DOCUMENTATION_CHECKLIST.md)

### For Reviewers

**When reviewing a PR:**
1. ✅ Verify documentation checklist is complete
2. ✅ Check documentation accuracy
3. ✅ Test code examples
4. ✅ Validate links
5. ✅ Ensure consistency with existing docs
6. ✅ Approve or request changes

### For Maintainers

**Before merging documentation PRs:**
1. ✅ Verify all reviews approved
2. ✅ Check CI/CD is green
3. ✅ Complete merge protocol
4. ✅ Verify follow-up issues created
5. ✅ Merge and verify deployment

**Protocol location:** [DOCUMENTATION_MERGE_PROTOCOL.md](DOCUMENTATION_MERGE_PROTOCOL.md)

## 📅 Review Schedule

| Review Type | Frequency | Duration | Next Review |
|-------------|-----------|----------|-------------|
| **Monthly Quick Review** | First Monday of month | 2-4 hours | See [schedule](DOCUMENTATION_REVIEW_SCHEDULE.md#monthly-quick-reviews) |
| **Quarterly Comprehensive** | Mid-month, start of quarter | 1-2 days | See [schedule](DOCUMENTATION_REVIEW_SCHEDULE.md#quarterly-comprehensive-reviews) |
| **Release Review** | 3-5 days before release | 4-8 hours | See [schedule](DOCUMENTATION_REVIEW_SCHEDULE.md#release-documentation-reviews) |
| **Ad-Hoc Review** | After major changes | As needed | On-demand |

**Full schedule:** [DOCUMENTATION_REVIEW_SCHEDULE.md](DOCUMENTATION_REVIEW_SCHEDULE.md)

## 🎯 Review Scopes

### Monthly Review (2-4 hours)
- Recent PRs (past 30 days)
- Quick wins (typos, broken links)
- User-reported issues
- Documentation debt tracking

### Quarterly Review (1-2 days)
- Complete documentation audit
- Test all examples
- Validate all links
- Update translations
- Archive outdated content
- Update architecture docs

### Release Review (4-8 hours)
- Version number updates
- Release notes
- Migration guides
- Feature documentation
- API documentation
- Installation guides

## ✅ Minimum Requirements

### For PRs to Merge

**Quality:**
- [ ] Documentation is accurate
- [ ] Examples work
- [ ] Links validated
- [ ] Spelling correct
- [ ] Style guide followed

**Completeness:**
- [ ] Changes documented
- [ ] Prerequisites listed
- [ ] Use cases covered
- [ ] CHANGELOG updated

**Review:**
- [ ] ≥1 approval
- [ ] Technical accuracy verified
- [ ] Comments resolved

**CI/CD:**
- [ ] All checks passing
- [ ] Documentation builds
- [ ] No broken links

## 🔧 Common Tasks

### Update Documentation for New Feature

```bash
# 1. Create/update documentation
vim docs/features/my-feature.md

# 2. Add examples
mkdir examples/my-feature
vim examples/my-feature/example.cpp

# 3. Update API docs (if applicable)
vim docs/api/my-api.md

# 4. Update CHANGELOG
vim CHANGELOG.md

# 5. Verify build
mkdocs build --strict

# 6. Test examples
cd examples/my-feature && make && ./example

# 7. Create PR with checklist
gh pr create --template
```

### Archive Outdated Documentation

```bash
# 1. Move to archive (preserves git history)
git mv docs/old-doc.md docs/archive/old-doc.md

# 2. Add archive note to document
vim docs/archive/old-doc.md
# Add note at top explaining why archived

# 3. Update archive README
vim docs/archive/README.md

# 4. Update references
grep -r "old-doc" docs/ # Find references
# Update links to point to new docs or archive

# 5. Update indexes
vim docs/DOCUMENTATION_INDEX.md

# 6. Update CHANGELOG
vim CHANGELOG.md

# 7. Commit
git add docs/ CHANGELOG.md
git commit -m "docs: Archive old-doc (superseded by new-doc)"

# 8. Create PR
gh pr create
```

### Conduct Monthly Review

```bash
# 1. Get review template
cp docs/DOCUMENTATION_REVIEW_SCHEDULE.md /tmp/review-YYYY-MM.md

# 2. List recent PRs
gh pr list --state merged --search "merged:>=$(date -d '30 days ago' +%Y-%m-%d)"

# 3. Check for broken links
./scripts/check-links.sh

# 4. Find stale pages
find docs/ -name "*.md" -mtime +180

# 5. Build documentation
mkdocs build --strict

# 6. Document findings in template

# 7. Create issues for problems found
gh issue create --label docs

# 8. Save review log
mkdir -p docs/reviews/monthly
mv /tmp/review-YYYY-MM.md docs/reviews/monthly/
```

## 🚨 Documentation Debt

### When to Create Follow-up Issues

Create issues for documentation work that:
- ❌ Can't be done in current PR
- ❌ Requires significant effort
- ❌ Needs domain expertise
- ❌ Depends on future changes
- ❌ Translation work
- ❌ Benchmarking/measurement

### How to Track

```bash
# Create issue
gh issue create \
  --label "docs,tech-debt/docs" \
  --title "docs: Add performance benchmarks for feature X" \
  --body "Follow-up from PR #XXX..."

# Link to PR
# Add to milestone
# Assign priority (P0-P3)
```

## 📊 Key Metrics

Track these in quarterly reviews:

| Metric | Target | How to Check |
|--------|--------|--------------|
| Feature Coverage | >95% | Compare features vs docs |
| API Coverage | 100% | Compare endpoints vs docs |
| Broken Links | 0 | `./scripts/check-links.sh` |
| Stale Pages | <10 | `find docs/ -mtime +180` |
| User Issues | <5/month | GitHub issue count |

## 🛠️ Tools

### Documentation Build
```bash
mkdocs build --strict      # Build with strict error checking
mkdocs serve              # Serve locally on http://127.0.0.1:8000
```

### Link Validation
```bash
./scripts/check-links.sh            # Check all links
./scripts/check-links.sh docs/api/  # Check specific directory
```

### Example Testing
```bash
./scripts/test-examples.sh          # Test all examples
cd examples/feature && ./test.sh    # Test specific example
```

### Metrics
```bash
find docs/ -name "*.md" | wc -l           # Count pages
find docs/ -name "*.md" -mtime +180       # Find stale pages
./scripts/doc-coverage.sh                  # Coverage report
```

## 📝 Templates

### Issue Template
```markdown
## Documentation Issue

**Related PR/Feature:** #XXX

**What needs to be documented:**
- [ ] Task 1
- [ ] Task 2

**Target:** vX.X.X
**Priority:** P1/P2/P3

**Context:**
Brief explanation...
```

### Archive Note Template
```markdown
# ARCHIVED: Document Title

**Archived Date:** YYYY-MM-DD
**Reason:** [Superseded/Removed/Consolidated/Outdated]
**Replaced By:** [Link to new documentation]
**Last Valid Version:** [commit SHA]

## Context
Explanation of why this was archived...

---

[Original content follows...]
```

## 🔗 Quick Links

### Process Documentation
- [Review Guidelines](DOCUMENTATION_REVIEW_GUIDELINES.md)
- [PR Checklist](PR_DOCUMENTATION_CHECKLIST.md)
- [Merge Protocol](DOCUMENTATION_MERGE_PROTOCOL.md)
- [Review Schedule](DOCUMENTATION_REVIEW_SCHEDULE.md)
- [Archival Process](DOCUMENTATION_ARCHIVAL_PROCESS.md)

### Contributing
- [Contributing Guide](../CONTRIBUTING.md)
- [Code of Conduct](../CODE_OF_CONDUCT.md)
- [PR Template](../.github/pull_request_template.md)

### Documentation Structure
- [Documentation Index](DOCUMENTATION_INDEX.md)
- [Architecture Docs](architecture/)
- [API Docs](api/)
- [Examples](../examples/)
- [Compendium](../compendium/)

## ❓ FAQ

**Q: Do I need to update documentation for a small bug fix?**  
A: If the bug fix changes documented behavior, yes. If it's purely internal, just update CHANGELOG.md.

**Q: How do I know if my changes need documentation?**  
A: Use the PR checklist. If your changes affect users, APIs, configuration, or behavior, update docs.

**Q: What if I don't have time to complete documentation in my PR?**  
A: Document the core changes and create follow-up issues for additional documentation work.

**Q: Who approves documentation changes?**  
A: At least one reviewer with knowledge of the feature/area must approve documentation accuracy.

**Q: How often should I review documentation?**  
A: Contributors: with every code change. Reviewers: with every review. Maintainers: follow the schedule.

**Q: What should I do with old documentation?**  
A: Don't delete! Archive it using the archival process to preserve history.

**Q: How do I report a documentation issue?**  
A: Open a GitHub issue with the `docs` label.

**Q: Where do I find translation status?**  
A: Check the quarterly review logs or run `./scripts/translation-status.sh` (if available).

## 🎓 Best Practices

### ✅ Do

- ✅ Update docs with code changes
- ✅ Test all examples
- ✅ Use clear, simple language
- ✅ Archive outdated content (don't delete)
- ✅ Track documentation debt
- ✅ Review regularly

### ❌ Don't

- ❌ Merge code without documentation
- ❌ Delete old documentation
- ❌ Use placeholder text ("TODO", "TBD")
- ❌ Break existing links
- ❌ Skip the checklist
- ❌ Assume documentation is optional

## 📞 Getting Help

**Documentation Issues:**
- Label: `docs`
- Ask in: Project discussions
- Contact: Documentation owner

**Process Questions:**
- Review this guide
- Check full guidelines
- Ask maintainers

**Technical Questions:**
- Ask module owner
- Check existing docs
- Open discussion

---

**Last Updated:** 2026-04-06  
**Next Review:** 2026-05-02 (Quarterly)

**Questions?** Open an issue with the `docs` label.
