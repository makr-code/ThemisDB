---
name: Documentation Archival
about: Template for archiving outdated or superseded documentation
title: 'docs: Archive [Document/Section Name]'
labels: 'type:documentation, priority:P3, effort:small'
assignees: ''
---

## 📋 Overview

**Document/Section:** [Name of document or section to archive]  
**Type:** Documentation Archival  
**Priority:** Low-Medium  
**Estimated Effort:** Small

## 🎯 Objective

Archive outdated, superseded, or no-longer-relevant documentation while preserving historical context and maintaining git history.

## 🔍 Reason for Archival

**Select Reason(s):**
- [ ] Content is outdated and superseded by newer documentation
- [ ] Feature/component no longer exists or has been replaced
- [ ] Information has been consolidated into another document
- [ ] Content is no longer accurate or relevant
- [ ] Duplicate or redundant information
- [ ] Planned feature was not implemented
- [ ] Other: [Specify]

**Context:**
[Explain why this documentation should be archived. Reference verification findings if applicable.]

**Verification Finding:**
- Verification Report: `docs/DOCUMENTATION_UPDATE_FINAL_REPORT.md`
- Related Component: [If applicable]
- Current Status: [e.g., Implemented differently, Not implemented, Superseded]

## 📝 Archival Tasks

### 1. Document Assessment
- [ ] Verify content is truly outdated/irrelevant
- [ ] Check if any content should be preserved elsewhere
- [ ] Identify dependent documentation (references to this doc)
- [ ] Review git history for context
- [ ] Get team confirmation if needed

### 2. Content Preservation
- [ ] Extract valuable information to preserve
- [ ] Move to appropriate location if applicable
- [ ] Create archive note with context
- [ ] Document why archived and when
- [ ] Add reference to replacement (if any)

### 3. Archival Process
- [ ] Create archive directory structure if needed
- [ ] Move document to archive location
- [ ] Add ARCHIVED.md note to original location (if applicable)
- [ ] Update documentation index/README
- [ ] Remove from navigation/TOC

### 4. Update References
- [ ] Find all references to archived document
- [ ] Update or remove references
- [ ] Add redirects/notes where appropriate
- [ ] Update cross-references
- [ ] Verify no broken links

### 5. Communication
- [ ] Document archival in CHANGELOG
- [ ] Notify stakeholders if needed
- [ ] Update issue tracker
- [ ] Add migration guide if needed

## 📂 Archival Structure

**Proposed Archive Location:**
```
docs/archive/[YEAR]/[CATEGORY]/[document-name].md
```

**Example:**
```
docs/archive/2026/security/OLD_ENCRYPTION_STRATEGY.md
```

**Archive Note Template:**
```markdown
# ARCHIVED: [Document Name]

**Archived Date:** YYYY-MM-DD  
**Reason:** [Brief reason]  
**Replaced By:** [Link to replacement, or "N/A"]  
**Last Valid Version:** [Commit SHA or date]

## Context
[Explanation of why archived]

## Historical Information
[Any important context to preserve]

## See Also
- [Link to current documentation]
- [Link to related resources]
```

## 🔗 Replacement/Alternative

**Replacement Documentation:** [Link to new/current documentation, or "N/A"]  
**Migration Path:** [How users should transition, or "N/A"]  
**Alternative Resources:** [Other relevant documentation]

## 📊 Impact Assessment

**Documents Affected:** [Number]  
**Internal References:** [Number of internal links]  
**External References:** [Known external links]  
**User Impact:** [Low/Medium/High]

**Mitigation:**
- [ ] Add redirect notes
- [ ] Update all internal references
- [ ] Notify external link owners if applicable
- [ ] Provide clear migration path

## ⚠️ Preservation Requirements

**Information to Preserve Elsewhere:**
- [ ] [Specific content/sections to extract]
- [ ] [Historical context to document]
- [ ] [Examples or use cases still relevant]

**Destination for Preserved Content:**
- [List where preserved content will be integrated]

## ✅ Checklist

### Pre-Archival
- [ ] Team approval obtained (if required)
- [ ] Valuable content extracted and preserved
- [ ] Replacement documentation identified
- [ ] Impact assessment completed

### Archival
- [ ] Document moved to archive location
- [ ] Archive note created
- [ ] Git history preserved
- [ ] Archive directory structure maintained

### Post-Archival
- [ ] All references updated
- [ ] Navigation/index updated
- [ ] CHANGELOG updated
- [ ] Stakeholders notified
- [ ] No broken links

### Verification
- [ ] Archive location accessible
- [ ] Archive note complete
- [ ] References correct
- [ ] Links working
- [ ] Changes reviewed

## 📚 Related Documentation

- Archive Policy: `docs/CONTRIBUTING.md#documentation-archival`
- Documentation Standards: [Link]
- Verification Report: `docs/DOCUMENTATION_UPDATE_FINAL_REPORT.md`

## 💡 Examples from Verification

**Archival Candidates Identified:**
- [ ] todo.md items marked as "not planned" (21 items marked "BEREITS IMPLEMENTIERT" but TODOs remain)
- [ ] DOCUMENTATION_RENEWAL_TODO.md completed items (security policies now created)
- [ ] Outdated architecture documents superseded by current implementation
- [ ] Planning documents for features implemented differently

## ✅ Definition of Done

- [ ] Document archived to appropriate location
- [ ] Archive note created with full context
- [ ] All references updated or removed
- [ ] Navigation/index updated
- [ ] No broken links
- [ ] Valuable content preserved
- [ ] Changes reviewed and approved
- [ ] PR merged to `develop`

---

**Template Version:** 1.0  
**Created:** 2026-01-12  
**Based on:** Documentation Update Verification Findings
