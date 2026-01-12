---
name: Documentation - Source Directory Structure Guide
about: Create comprehensive documentation for src/ directory structure
title: '[DOCS] Create Comprehensive src/ Directory Structure Documentation'
labels: 'type:documentation, priority:P1, area:architecture, effort:medium'
assignees: ''
---

## Problem Statement

The `src/` directory contains **35 subdirectories**, but only **8 (23%)** are documented in the architecture documentation. This makes it difficult for new contributors to navigate the codebase and understand where to find or add functionality.

## Current Documentation Coverage

### Documented Directories (8/35 = 23%):

| Directory | Documented As | Location |
|-----------|---------------|----------|
| src/acceleration/ | GPU backend abstraction | Architecture docs |
| src/index/ | Vector, graph, spatial indexes | Architecture docs |
| src/llm/ | LLM plugin | Architecture docs |
| src/query/ | AQL parser, query engine | Architecture docs |
| src/security/ | Encryption, HSM, PKI | Architecture docs |
| src/server/ | HTTP server, API handlers | Architecture docs |
| src/sharding/ | Distributed sharding, replication | Architecture docs |
| src/storage/ | RocksDB wrapper, key-value storage | Architecture docs |

### Undocumented Directories (27/35 = 77%):

- analytics/
- api/
- aql/
- auth/
- base/
- cache/
- cdc/
- content/
- exporters/
- geo/
- governance/
- gpu/
- importers/
- network/
- observability/
- performance/
- plugins/
- replication/
- scheduler/
- search/
- temporal/
- timeseries/
- transaction/
- updates/
- utils/
- voice/
- + 1 more

## Impact

**Severity:** HIGH

**Affected Users:**
- New contributors: Cannot find where to add code
- Code reviewers: Cannot assess if code is in the right place
- Maintainers: Difficult to enforce architecture boundaries
- Documentation readers: Incomplete picture of system architecture

**Consequences:**
- Increased onboarding time for new developers
- Code placed in wrong directories
- Difficulty locating existing functionality
- Harder to maintain consistent architecture

## Proposed Solution

Create a comprehensive source directory guide document: `docs/architecture/SOURCE_DIRECTORY_GUIDE.md`

### Document Structure

```markdown
# ThemisDB Source Code Directory Guide

## Overview

Brief description of src/ organization philosophy

## Directory Structure

### Core Infrastructure
- base/ - Base classes, utilities, common types
- utils/ - Utility functions and helpers
- network/ - Network communication layer

### Storage Layer
- storage/ - RocksDB wrapper, key-value storage
- cache/ - Caching implementations
- transaction/ - Transaction management
- replication/ - Replication logic

[... continue for all 35 directories ...]

## Adding New Code

Guidelines for where to place new code based on functionality

## Cross-Cutting Concerns

How to handle code that spans multiple directories

## Deprecated Directories

List any directories scheduled for removal or refactoring
```

### Information to Include for Each Directory

1. **Purpose** - What this directory contains
2. **Key Files** - Notable files and their purposes
3. **Dependencies** - What other directories it depends on
4. **Feature Flags** - Any CMake flags that control this code
5. **Documentation Links** - Related documentation
6. **Example Usage** - Code examples or reference implementations

## Implementation Plan

### Phase 1: Discovery (2-4 hours)
- [ ] List all 35 directories in src/
- [ ] Identify key files in each directory
- [ ] Determine purpose of each directory from code inspection
- [ ] Check for existing README files in subdirectories

### Phase 2: Documentation (8-12 hours)
- [ ] Create docs/architecture/SOURCE_DIRECTORY_GUIDE.md
- [ ] Document each directory with purpose and key files
- [ ] Add cross-references between related directories
- [ ] Include code examples where helpful
- [ ] Add visual diagram showing relationships

### Phase 3: Integration (2-3 hours)
- [ ] Link from main documentation index
- [ ] Update DOCUMENTATION_INDEX.md
- [ ] Cross-reference in CMAKE_MODULAR_ARCHITECTURE.md
- [ ] Add to contributor onboarding guides

### Phase 4: Validation (1-2 hours)
- [ ] Review by maintainer
- [ ] Test all links
- [ ] Verify accuracy with code inspection
- [ ] Get feedback from new contributor

## Acceptance Criteria

### Completeness
- [ ] All 35 src/ directories documented
- [ ] Each directory has purpose description
- [ ] Key files identified for each directory
- [ ] Dependencies mapped between directories

### Quality
- [ ] Information accurate (verified against code)
- [ ] Examples provided where helpful
- [ ] Clear writing, consistent formatting
- [ ] Diagram showing directory relationships

### Usability
- [ ] Easy to navigate and search
- [ ] Linked from main documentation
- [ ] Referenced in contributing guides
- [ ] Tested by new contributor for clarity

### Maintenance
- [ ] Document how to keep guide updated
- [ ] Add guide location to PR template
- [ ] Include in documentation review checklist

## Additional Context

**Source:** Documentation-Source Code Gap Analysis  
**Gap Analysis Document:** docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md

**Directory Counts:**
- Total: 35 directories
- Documented: 8 directories
- Undocumented: 27 directories
- Coverage: 23%

**Related Issues:**
- Architecture documentation overhaul
- CMake modular architecture

## Example Directory Documentation

### Template for Each Directory

```markdown
### src/analytics/

**Purpose:** Real-time analytics and metrics aggregation

**Key Files:**
- `analytics_engine.cpp` - Main analytics processing engine
- `metrics_collector.cpp` - Collects metrics from various subsystems
- `query_statistics.cpp` - Query performance statistics

**Dependencies:**
- src/storage/ - For data access
- src/observability/ - For metrics export

**Feature Flags:**
- `THEMIS_ENABLE_ANALYTICS` - Enable analytics subsystem

**Documentation:**
- [Analytics Guide](../features/analytics.md)
- [Metrics API](../apis/metrics_api.md)

**Example Usage:**
```cpp
#include "analytics/analytics_engine.h"

AnalyticsEngine engine;
engine.recordQuery(query, execution_time);
auto stats = engine.getQueryStatistics();
```
```

## Resources Needed

- [ ] Access to full source code
- [ ] CMakeLists.txt for feature flag mapping
- [ ] Input from core maintainers on directory purposes
- [ ] Diagramming tool for relationship visualization

## Timeline

**Estimated Effort:** 2-3 days (16-24 hours)

**Breakdown:**
- Day 1: Discovery and categorization
- Day 2: Documentation writing
- Day 3: Integration, review, and polish

**Priority:** P1 (High) - Important for developer productivity and code quality
