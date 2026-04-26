# Documentation Merge Protocol

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Status:** Official Template

---

## Overview

This document defines the merge protocol for pull requests that include documentation changes. Following this protocol ensures documentation quality and traceability.

## Merge Protocol Template

Use this template to document the merge decision for documentation-related PRs:

```markdown
# Documentation Merge Protocol

**PR Number:** #XXX  
**PR Title:** [Title]  
**Author:** @username  
**Reviewers:** @reviewer1, @reviewer2  
**Merge Date:** YYYY-MM-DD  
**Merged By:** @merger-username

---

## 1. Documentation Review Summary

### Documentation Changes
**Files Modified:**
- `docs/feature-guide.md` - Added new feature documentation
- `examples/feature-example/` - Added working example
- `CHANGELOG.md` - Added changelog entry

**Change Type:**
- [ ] New documentation
- [x] Updated existing documentation
- [ ] Archived outdated documentation
- [ ] Fixed documentation errors
- [ ] Improved clarity/formatting

### Review Status
- [x] Technical accuracy verified by @reviewer1
- [x] Code examples tested by @reviewer2
- [x] Links validated
- [x] Spelling/grammar checked
- [x] Consistent with style guide
- [x] Build successful (`mkdocs build --strict`)

---

## 2. Quality Checks

### Accuracy
- [x] Documentation matches implementation
- [x] Version numbers are correct
- [x] Code examples compile and run
- [x] Command examples are tested
- [x] API signatures are accurate

### Completeness
- [x] All changes are documented
- [x] Prerequisites are listed
- [x] Examples cover main use cases
- [x] Error scenarios documented
- [x] Migration path provided (if breaking change)

### Clarity
- [x] Language is clear and concise
- [x] Structure is logical
- [x] Examples are helpful
- [x] Diagrams are clear (if present)
- [x] Technical terms explained

### Consistency
- [x] Naming matches codebase
- [x] Formatting is consistent
- [x] Cross-references work
- [x] Style guide followed
- [x] Translations aligned (if applicable)

---

## 3. CI/CD Results

**Continuous Integration:**
- [x] Build: ✅ Passed
- [x] Tests: ✅ Passed (if applicable)
- [x] Linting: ✅ Passed
- [x] Link Check: ✅ Passed
- [x] Documentation Build: ✅ Passed

**Build Logs:** [Link to CI run]

---

## 4. Review Comments Resolution

**Total Comments:** X  
**Resolved:** X  
**Outstanding:** 0

| Comment | Resolution | Resolved By |
|---------|-----------|-------------|
| Example 1 | Fixed in commit abc123 | @author |
| Example 2 | Accepted, no change needed | @reviewer1 |

---

## 5. Documentation Debt

**New Documentation Debt Created:**
- [ ] None
- [x] Follow-up tasks tracked in issues

**Follow-up Issues:**
- #YYY - Add performance benchmarks (P2, due v1.5.0)
- #ZZZ - Translate to German (P3, due v1.5.0)

**Debt Justification:**
Core documentation complete. Enhancements tracked for next release.

---

## 6. Impact Assessment

### User Impact
- **Audience:** Developers using feature X
- **Impact Level:** 🟢 Low / 🟡 Medium / 🔴 High
- **User Benefit:** Clear documentation for new feature X

### Documentation Scope
- **Pages Updated:** 3
- **New Pages:** 1
- **Examples Added:** 2
- **Deprecated Content:** 1 (archived)

### Release Impact
- **Blocks Release:** No
- **Release Notes:** Yes (included in CHANGELOG.md)
- **Migration Guide:** N/A
- **Breaking Changes:** None

---

## 7. Merge Decision

**Decision:** ✅ APPROVED FOR MERGE

**Rationale:**
- All documentation requirements met
- Quality checks passed
- Reviews approved
- CI/CD green
- Documentation debt acceptable and tracked

**Special Considerations:**
None

**Merge Strategy:**
- [x] Squash and merge (default)
- [ ] Merge commit (preserve history)
- [ ] Rebase and merge (linear history)

---

## 8. Post-Merge Actions

**Completed:**
- [x] Merged to `develop` branch
- [x] Updated documentation version
- [x] Notified stakeholders (if applicable)
- [x] Updated project board
- [x] Closed related issues: #AAA

**Pending:**
- [ ] Deploy to documentation site (automatic)
- [ ] Verify live documentation
- [ ] Archive old version (if major update)

---

## 9. Feedback and Lessons Learned

**What Went Well:**
- Examples were comprehensive
- Review process was smooth
- Good collaboration with reviewers

**What Could Improve:**
- Could have added diagrams earlier
- Initial examples had minor errors

**Process Improvements:**
- Consider adding diagram templates
- Set up example auto-testing

---

## 10. Sign-Off

**Documentation Owner:** @doc-owner ✅  
**Technical Reviewer:** @tech-reviewer ✅  
**Language Reviewer:** @lang-reviewer ✅ (if applicable)  
**Release Manager:** @release-manager ✅ (for releases)

**Final Approval:** @merger-username ✅

---

**Protocol Complete:** ✅  
**Merge Timestamp:** YYYY-MM-DD HH:MM:SS UTC  
**Commit SHA:** abc123def456...
```

---

## Usage Instructions

### When to Use This Protocol

Use this merge protocol for:

✅ **Required:**
- PRs with significant documentation changes
- New feature documentation
- API documentation updates
- Breaking changes requiring migration guides
- Release documentation updates

📝 **Recommended:**
- Documentation improvements
- Example additions
- Architecture documentation updates
- Translation updates

📚 **Optional:**
- Minor typo fixes
- Formatting improvements
- Small clarifications
- Link fixes

### How to Use

#### For PR Authors

1. **Create PR**
   - Include documentation checklist (see [PR_DOCUMENTATION_CHECKLIST.md](PR_DOCUMENTATION_CHECKLIST.md))
   - Link to affected documentation
   - Note any follow-up work

2. **During Review**
   - Respond to reviewer comments
   - Update documentation as needed
   - Ensure all checks pass

3. **Before Merge**
   - Verify all checklist items complete
   - Ensure CI/CD is green
   - Get required approvals

#### For Reviewers

1. **Initial Review**
   - Check documentation accuracy
   - Test examples
   - Verify links
   - Review clarity and completeness

2. **Provide Feedback**
   - Be specific and constructive
   - Suggest improvements
   - Approve or request changes

3. **Final Approval**
   - Verify all comments resolved
   - Check updated documentation
   - Approve for merge

#### For Maintainers/Mergers

1. **Pre-Merge Check**
   - All reviews approved
   - CI/CD green
   - Documentation checklist complete
   - No blocking issues

2. **Complete Protocol**
   - Fill out merge protocol template
   - Document decisions and rationale
   - Note any follow-up work

3. **Merge**
   - Use appropriate merge strategy
   - Add protocol to PR or documentation log
   - Complete post-merge actions

4. **Post-Merge**
   - Verify deployment
   - Update tracking systems
   - Close related issues

---

## Merge Criteria

### Minimum Requirements

Documentation PR must meet these criteria to merge:

✅ **Quality**
- [ ] Documentation is technically accurate
- [ ] Examples are tested and working
- [ ] Links are validated
- [ ] Spelling/grammar is correct
- [ ] Follows style guide

✅ **Completeness**
- [ ] All changes are documented
- [ ] Prerequisites are listed
- [ ] Main use cases covered
- [ ] CHANGELOG updated

✅ **Review**
- [ ] At least one approval
- [ ] Technical accuracy verified
- [ ] All comments resolved

✅ **CI/CD**
- [ ] All checks passing
- [ ] Documentation builds successfully
- [ ] No broken links

✅ **Process**
- [ ] Documentation checklist complete
- [ ] Follow-up issues created (if needed)
- [ ] Related issues linked

### Blocking Issues

Do **NOT** merge if:

❌ **Critical Issues**
- Documentation contains incorrect information
- Examples don't work
- Breaking changes not documented
- CI/CD failing
- Outstanding blocking review comments

❌ **Process Issues**
- Required reviewers haven't approved
- Documentation checklist incomplete
- Merge protocol not filled out
- Conflicts not resolved

### Special Cases

**Emergency Hotfix Documentation:**
- Expedited review process
- Minimum one approval required
- Can merge with minor issues if documented
- Must create follow-up issues

**Experimental Feature Documentation:**
- Mark as "Experimental" or "Beta"
- Can have incomplete coverage
- Must note limitations clearly
- Follow-up issues required

**Translation PRs:**
- Native speaker review required
- Technical accuracy already verified (if translated from English)
- Can merge with minor language improvements tracked

---

## Documentation Merge Log

For major documentation changes, maintain a merge log:

### Log Format

```markdown
## Documentation Merge Log

### YYYY-MM-DD: PR #XXX - [Title]
**Author:** @username  
**Category:** New Feature Documentation  
**Impact:** High  
**Status:** ✅ Merged

**Summary:** Added comprehensive documentation for feature X including user guide, API reference, and examples.

**Files:**
- docs/features/feature-x.md (NEW)
- docs/api/feature-x-api.md (NEW)
- examples/feature-x/ (NEW)

**Follow-up:**
- Issue #YYY - Performance benchmarks
- Issue #ZZZ - Translation

**Reviewers:** @reviewer1, @reviewer2
```

### Log Location

Maintain merge logs at:
- `docs/MERGE_LOG.md` - Current year
- `docs/archive/MERGE_LOG_YYYY.md` - Previous years

---

## Review Calendar and Schedule

Track scheduled reviews:

### Monthly Reviews
- **First Monday of month**: Quick review of recent changes
- **Log format**: `docs/reviews/monthly/YYYY-MM.md`

### Quarterly Reviews
- **Start of Q1, Q2, Q3, Q4**: Comprehensive documentation audit
- **Log format**: `docs/reviews/quarterly/YYYY-QX.md`

### Release Reviews
- **Before each release**: Complete documentation verification
- **Log format**: `docs/reviews/releases/vX.Y.Z.md`

### Review Template

```markdown
# Documentation Review - [Period]

**Date:** YYYY-MM-DD  
**Reviewers:** @reviewer1, @reviewer2  
**Type:** Monthly / Quarterly / Release

## Scope
- [ ] Recent changes (past month)
- [ ] All documentation
- [ ] Specific areas: [list]

## Findings
### Issues Found
1. Issue description - Priority PX
2. Issue description - Priority PX

### Improvements Made
1. Improvement description
2. Improvement description

## Action Items
- [ ] Issue #XXX - Fix broken links (P1)
- [ ] Issue #YYY - Update examples (P2)
- [ ] Issue #ZZZ - Add diagrams (P3)

## Next Review
**Scheduled:** YYYY-MM-DD  
**Type:** Monthly / Quarterly / Release
```

---

## Metrics Tracking

Track these metrics for documentation merges:

### Per PR
- Review time (hours)
- Number of review cycles
- Number of comments
- Time to merge (days)

### Aggregate
- Documentation PRs per month
- Average review time
- Documentation coverage (% of features documented)
- User-reported documentation issues

### Quarterly Report Format

```markdown
# Documentation Metrics - QX YYYY

## Summary
- **PRs Merged:** XX
- **Average Review Time:** XX hours
- **Documentation Coverage:** XX%
- **User Issues:** XX

## Quality Indicators
- Broken links: XX (↓ from last quarter)
- Outdated pages: XX
- Missing examples: XX

## Improvements
- Action taken 1
- Action taken 2

## Goals for Next Quarter
- Goal 1
- Goal 2
```

---

## Related Documentation

- **Review Guidelines**: [DOCUMENTATION_REVIEW_GUIDELINES.md](DOCUMENTATION_REVIEW_GUIDELINES.md)
- **PR Checklist**: [PR_DOCUMENTATION_CHECKLIST.md](PR_DOCUMENTATION_CHECKLIST.md)
- **Contributing Guide**: [CONTRIBUTING.md](../CONTRIBUTING.md)
- **Archival Process**: [DOCUMENTATION_ARCHIVAL_PROCESS.md](DOCUMENTATION_ARCHIVAL_PROCESS.md)

---

**Questions?** Open an issue with the `docs` label or ask in project discussions.
