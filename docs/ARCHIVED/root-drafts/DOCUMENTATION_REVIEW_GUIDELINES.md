## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# Documentation Review Guidelines

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Status:** Official

---

## Overview

This document establishes guidelines for continuous documentation review and improvement to ensure that ThemisDB documentation remains accurate, up-to-date, and reflects the current state of development.

## Review Principles

### Living Documentation
Documentation is treated as a **living artifact** that evolves with the codebase:
- ✅ Documentation is updated **alongside** code changes
- ✅ Reviews are **mandatory** before merging code changes
- ✅ Documentation debt is **tracked and prioritized**
- ✅ Outdated content is **archived, not deleted**

### Quality Standards
All documentation must meet these standards:
- **Accurate**: Reflects current implementation
- **Complete**: Covers all features and APIs
- **Clear**: Written in understandable language
- **Tested**: Code examples are validated
- **Maintained**: Regularly reviewed and updated

## When to Review Documentation

### Mandatory Review Triggers

Documentation review is **required** when:

1. **Code Changes**
   - New features added
   - API changes (breaking or non-breaking)
   - Configuration changes
   - Security updates
   - Performance improvements
   - Bug fixes that affect documented behavior

2. **Architectural Changes**
   - New modules or components
   - Design pattern changes
   - Technology stack updates
   - Integration changes

3. **Release Events**
   - Before each release (major, minor, patch)
   - After hotfixes
   - During release candidate testing

4. **Scheduled Reviews**
   - Monthly documentation audit
   - Quarterly comprehensive review
   - Annual archive maintenance

### Optional Review Opportunities

Consider reviewing when:
- User feedback indicates confusion
- Support questions reveal documentation gaps
- Community contributions suggest improvements
- New use cases emerge

## Review Scope

### Documentation Directories

All reviews should cover these key areas:

#### Primary Documentation
- `/docs/` - Main documentation
- `/docs/de/` - German documentation
- `/docs/en/` - English documentation
- `/docs/fr/` - French documentation
- `/docs/es/` - Spanish documentation
- `/docs/ja/` - Japanese documentation

#### Specialized Documentation
- `/compendium/` - Compendium and extended guides
- `/examples/` - Code examples
- `README.md` - Project overview
- `CONTRIBUTING.md` - Contribution guidelines
- `CHANGELOG.md` - Change history

#### Code Documentation
- API documentation in headers
- Inline code comments
- OpenAPI specifications (`/openapi/`)

### Review Categories

Each review should assess:

1. **Accuracy**
   - Does documentation match current implementation?
   - Are code examples working?
   - Are command examples correct?
   - Are version numbers current?

2. **Completeness**
   - Are new features documented?
   - Are all APIs covered?
   - Are migration paths provided?
   - Are prerequisites listed?

3. **Clarity**
   - Is language clear and concise?
   - Are examples helpful?
   - Is navigation intuitive?
   - Are diagrams current?

4. **Consistency**
   - Do naming conventions match?
   - Is formatting consistent?
   - Are cross-references working?
   - Are translations aligned?

## Review Process

### 1. Pre-Merge Review (Required)

**For every PR that changes code:**

1. **Identify Documentation Impact**
   - What features/APIs are affected?
   - What documentation needs updates?
   - Are examples impacted?

2. **Update Documentation**
   - Update affected pages
   - Add new documentation if needed
   - Update code examples
   - Verify cross-references

3. **Mandatory Sourcecode Verification (per module)**
   - Verify documented symbols/behavior against real source files
   - Verify feature/build guards against CMake or header definitions
   - Record evidence per module (file paths + symbols/behavior)

4. **Fill Documentation Checklist**
   - Complete the PR documentation checklist (see template)
   - Link to updated documentation
   - Note any follow-up documentation tasks

5. **Review Documentation Changes**
   - Documentation changes reviewed alongside code
   - At least one reviewer verifies documentation
   - Documentation must be approved before merge
   - Reviews fail if per-module sourcecode evidence is missing

### 2. Scheduled Reviews

**Monthly Quick Review** (1st Monday of month):
- Review recent changes (past 30 days)
- Check for quick wins and corrections
- Identify documentation debt
- Schedule follow-up work

**Quarterly Comprehensive Review** (Start of Q1, Q2, Q3, Q4):
- Full documentation audit
- Update all version references
- Test all code examples
- Review and update architecture docs
- Check link integrity
- Update translations

**Annual Archive Review** (January):
- Review archived documentation
- Remove obsolete archives (>3 years)
- Reorganize archive structure
- Update archive indexes

### 3. Release Reviews

**Before Release:**
- Update version numbers
- Review release notes
- Update migration guides
- Test all quickstart guides
- Verify API documentation

**After Release:**
- Archive superseded documentation
- Update "latest" references
- Publish release documentation
- Update external documentation sites

## Review Checklist

Use this checklist for each documentation review:

```markdown
### Documentation Review Checklist

**PR/Issue:** #XXX  
**Reviewer:** @username  
**Date:** YYYY-MM-DD

#### Accuracy
- [ ] Documentation matches current implementation
- [ ] Code examples compile and run
- [ ] Command examples are tested
- [ ] Version numbers are current
- [ ] Links are not broken

#### Completeness
- [ ] New features are documented
- [ ] API changes are documented
- [ ] Migration guides provided (if needed)
- [ ] Examples cover main use cases
- [ ] Prerequisites are listed

#### Clarity
- [ ] Language is clear and concise
- [ ] Examples are helpful
- [ ] Structure is logical
- [ ] Technical terms are explained
- [ ] Diagrams are clear (if present)

#### Consistency
- [ ] Naming matches codebase
- [ ] Formatting is consistent
- [ ] Cross-references work
- [ ] Style guide followed

#### Maintenance
- [ ] Archive outdated content
- [ ] Update indexes
- [ ] Update CHANGELOG
- [ ] Label issues appropriately

#### Notes
<!-- Add any additional notes or concerns -->
```

## Documentation Debt Management

### Tracking Documentation Debt

1. **Label Issues**
   - Use `docs` label for documentation tasks
   - Use `tech-debt/docs` for documentation debt
   - Priority: `P0`, `P1`, `P2`, `P3`

2. **Create Issues for Gaps**
   - One issue per documentation gap
   - Link to related code changes
   - Assign to appropriate person
   - Set milestone

3. **Track in Project Board**
   - Maintain documentation project board
   - Prioritize based on user impact
   - Review in planning meetings

### Reducing Documentation Debt

- **Address debt incrementally** in each sprint
- **Allocate time** for documentation in estimates
- **Pair documentation** with feature work
- **Block merges** if documentation is incomplete

## Review Roles and Responsibilities

### Documentation Owners
- **Maintainers**: Overall documentation quality
- **Module Owners**: Documentation for their modules
- **Contributors**: Documentation for their changes

### Reviewers
- At least **one reviewer** must verify documentation
- Reviewers check both technical accuracy and clarity
- Language-specific reviewers for translations

### Release Manager
- Ensures documentation is ready for release
- Coordinates documentation updates
- Publishes release documentation

## Tools and Automation

### Documentation Validation

Use these tools to validate documentation:

```bash
# Check for broken links
./scripts/check-links.sh

# Validate markdown syntax
./scripts/check-markdown.sh

# Build documentation
mkdocs build --strict

# Generate PDF (if needed)
cd compendium && python step1_generate_svgs.py
```

### Continuous Integration

Documentation checks run automatically on PR:
- Markdown linting
- Link validation
- Build verification
- Spelling check (if configured)

### Pre-Commit Hooks

Optionally enable pre-commit hooks:
```bash
# Install pre-commit hooks
pre-commit install

# Run manually
pre-commit run --all-files
```

## Best Practices

### Writing Guidelines

1. **Use Active Voice**
   - ✅ "Configure the database by editing..."
   - ❌ "The database can be configured by editing..."

2. **Be Specific**
   - ✅ "Set `max_connections` to 1000"
   - ❌ "Set the connection limit to a high value"

3. **Include Examples**
   - Provide working code examples
   - Show expected output
   - Explain what the example demonstrates

4. **Keep Updated**
   - Remove outdated information immediately
   - Update version numbers in examples
   - Archive superseded content properly

### Review Tips

1. **Review Early**
   - Review documentation as soon as possible
   - Don't wait until PR is ready to merge

2. **Test Examples**
   - Run all code examples
   - Verify command-line examples
   - Check that links work

3. **Consider Users**
   - Read from user perspective
   - Identify confusing sections
   - Suggest improvements

4. **Be Constructive**
   - Suggest specific improvements
   - Explain why changes are needed
   - Offer to help with updates

## Feedback Mechanisms

### User Feedback

Collect feedback through:
- **GitHub Issues**: User-reported documentation issues
- **Pull Requests**: Community contributions
- **Discussions**: Q&A and suggestions
- **Support Channels**: Common questions indicate gaps

### Internal Feedback

Gather feedback from:
- **Code Reviews**: Identify documentation needs
- **Planning Meetings**: Prioritize documentation work
- **Retrospectives**: Improve documentation process

## Metrics and Monitoring

Track these metrics to assess documentation quality:

### Quality Metrics
- Number of broken links
- Number of outdated pages
- Time since last update per page
- Documentation coverage (features documented vs total)

### Process Metrics
- PRs with documentation updates (%)
- Average time to update documentation after code change
- Documentation issues resolved per sprint
- User-reported documentation issues

### Engagement Metrics
- Documentation page views
- Time on page
- Search queries
- External contributions

## Related Documentation

- **Archival Process**: [DOCUMENTATION_ARCHIVAL_PROCESS.md](DOCUMENTATION_ARCHIVAL_PROCESS.md)
- **Contributing Guide**: [CONTRIBUTING.md](../CONTRIBUTING.md)
- **PR Checklist**: [.github/pull_request_template.md](../.github/pull_request_template.md)
- **Documentation Index**: [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)

## Questions?

- Open an issue with the `docs` label
- Ask in project discussions
- Contact documentation maintainers

---

**Next Steps:**
1. Review this document with the team
2. Integrate checklist into PR template
3. Set up scheduled review calendar
4. Begin tracking documentation metrics
