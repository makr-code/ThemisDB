# Documentation Organization Plan

## Current State
- 24 markdown files in docs root (too many)
- 52 subdirectories (good organization exists for technical docs)

## Proposed Organization

### Keep in Root
- README.md (main docs readme)
- index.md (documentation index)
- home.md (documentation home)
- glossary.md (terminology)
- DOCUMENTATION_INDEX.md (master index)

### Move to build/
- BUILD-SYSTEM.md
- BUILDGUIDE.md
- BUILD_ORGANIZATION.md
- QUICK_BUILD_GUIDE.md
- BUILD_STRATEGY_DOTNET_FRONTEND.md

### Move to development/
- IMPLEMENTATION-SUMMARY.md
- IMPLEMENTATION_COMPLETE.md
- IMPLEMENTATION_PLAN.md
- SOURCE_CODE_CHANGES_v1.0.md
- CODE_REVIEW_v1.1.0_v1.2.0.md
- DOCUMENTATION_NAMING_CONVENTION.md
- DOCUMENTATION_RENEWAL_TODO.md

### Move to guides/
- RAILWAY_COMPLETE_GUIDE.md
- DOCS_QUICKREF.md

### Move to architecture/
- ARCHITECTURE_OVERVIEW.md
- TRUETIME_INTEGRATION.md
- wire_protocol_v1.md

### Move to stakeholder/
- ~~STAKEHOLDER_VALUE_PROPOSITION.md~~ - 🔒 Confidential (removed from public repository)

### Archive (lowercase, to be removed/renamed)
- changelog.md (duplicate of ../CHANGELOG.md)
