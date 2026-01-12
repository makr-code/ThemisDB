---
name: Documentation - Examples Index and Quickstart Guide
about: Create comprehensive index for 22+ numbered examples and quickstart guides
title: '[DOCS] Create Examples Index and Quickstart Guides'
labels: 'type:documentation, priority:P2, area:examples, effort:medium, good first issue'
assignees: ''
---

## Problem Statement

ThemisDB has **22 numbered example directories** (examples/01_hello_world/ through examples/22_aql_diagram_tool/) plus **15+ standalone example files**, but they are not indexed in the main documentation. Each numbered example has its own README, but there's no overview or quickstart guide.

## Current Situation

### What Exists
- ✅ 22 numbered example directories (01-22)
- ✅ Each directory has its own README.md
- ✅ 15+ standalone .cpp and .py example files
- ✅ examples/README.md (minimal, doesn't cover numbered examples)

### What's Missing
- ❌ No comprehensive index of all examples
- ❌ No quickstart guide for beginners
- ❌ No categorization by difficulty or topic
- ❌ Numbered examples not linked from main docs
- ❌ No "recommended learning path"
- ❌ No relationship between examples explained

## Impact

**Severity:** MEDIUM

**Affected Users:**
- New users: Can't find examples to get started
- Learners: Don't know which example to start with
- Developers: Can't find examples for specific features
- Documentation readers: Missing practical tutorials

**Benefits of Fixing:**
- Improved onboarding experience
- Faster learning curve for new users
- Better discoverability of features
- More complete documentation

## Discovered Examples

### Numbered Examples (01-22)
1. examples/01_hello_world/
2. examples/02_todo_app/
3. examples/03_contact_manager/
4. examples/04_inventory_system/
5. examples/05_time_series_monitor/
6. examples/06_graph_social_network/
7. examples/07_vector_search_documents/
8. examples/08_dms_erp_system/
9. examples/09_iot_sensor_network/
10. examples/10_drone_image_analysis/
11. examples/11_blog_wiki/
12. examples/12_expense_tracker/
13. examples/13_recipe_manager/
14. examples/14_ecommerce_catalog/
15. examples/15_event_management/
16. examples/16_kanban_board/
17. examples/17_crm/
18. examples/18_realtime_chat/
19. examples/19_recommendation_engine/
20. examples/20_smart_home/
21. examples/21_coding_platform/
22. examples/22_aql_diagram_tool/

### Standalone C++ Examples
- adaptive_retention_example.cpp
- chat_formatting_example.cpp
- data_retention_downsampling_example.cpp
- embedded_llm_examples.cpp
- example_llm_metrics.cpp
- example_multi_ssd_configuration.cpp
- example_vector_encryption.cpp
- hot_reload_example.cpp
- hot_spare_example.cpp
- hybrid_retention_usage_example.cpp
- sharding_demo.cpp
- task_scheduler_integration_example.cpp
- test_optimization_standalone.cpp
- themis_help_lora_example.cpp

### Standalone Python Examples
- archive_pipeline.py
- voice_assistant_example.py

### Specialized Example Directories
- examples/feedback_plugins/
- examples/geo/
- examples/image_analysis/
- examples/nlp/
- examples/railway/

## Proposed Solution

### Create Examples Documentation Hub

Create `docs/EXAMPLES_INDEX.md` and `docs/EXAMPLES_QUICKSTART.md`

### Structure for EXAMPLES_INDEX.md

```markdown
# ThemisDB Examples Index

## Quick Start

→ [Examples Quickstart Guide](EXAMPLES_QUICKSTART.md) - Start here if you're new!

## Numbered Examples (01-22)

### Beginner Level
- [01. Hello World](../examples/01_hello_world/) - Your first ThemisDB application
- [02. Todo App](../examples/02_todo_app/) - Simple CRUD operations
- [03. Contact Manager](../examples/03_contact_manager/) - Basic data management

### Intermediate Level
- [04. Inventory System](../examples/04_inventory_system/) - Business logic
- [05. Time Series Monitor](../examples/05_time_series_monitor/) - Time series data
- ...

### Advanced Level
- [19. Recommendation Engine](../examples/19_recommendation_engine/) - ML integration
- [21. Coding Platform](../examples/21_coding_platform/) - Complex system
- [22. AQL Diagram Tool](../examples/22_aql_diagram_tool/) - Advanced queries

## Feature-Specific Examples

### LLM Integration
- [Embedded LLM Examples](../examples/embedded_llm_examples.cpp)
- [Chat Formatting](../examples/chat_formatting_example.cpp)
- [Themis Help LoRA](../examples/themis_help_lora_example.cpp)
- [LLM Metrics](../examples/example_llm_metrics.cpp)

### Data Retention & Archival
- [Adaptive Retention](../examples/adaptive_retention_example.cpp)
- [Data Retention Downsampling](../examples/data_retention_downsampling_example.cpp)
- [Hybrid Retention](../examples/hybrid_retention_usage_example.cpp)
- [Archive Pipeline](../examples/archive_pipeline.py)

[... continue for all categories ...]
```

### Structure for EXAMPLES_QUICKSTART.md

```markdown
# ThemisDB Examples Quickstart Guide

## Your First 10 Minutes

### Step 1: Hello World (5 minutes)
[Link to 01_hello_world with quick walkthrough]

### Step 2: Run a Query (5 minutes)
[Simple query example]

## Your First Hour

### Learn CRUD Operations
[Todo App tutorial]

### Explore Vector Search
[Vector search example]

## Your First Day

[More comprehensive examples]

## Learning Paths

### Path 1: Web Developer
Recommended sequence: 01 → 02 → 11 → 17 → 18

### Path 2: Data Engineer
Recommended sequence: 01 → 05 → 07 → 19

### Path 3: ML Engineer
Recommended sequence: 01 → 07 → 10 → 19 + LLM examples

[... etc ...]
```

## Implementation Plan

### Phase 1: Discovery & Categorization (4-6 hours)
- [ ] Review all 22 numbered examples
- [ ] Read each README to understand purpose and difficulty
- [ ] Categorize by:
  - Difficulty (Beginner/Intermediate/Advanced)
  - Topic (Web, Data, ML, IoT, etc.)
  - Features used (LLM, Vector, Graph, etc.)
- [ ] Document dependencies between examples
- [ ] Note any broken or outdated examples

### Phase 2: Index Creation (4-6 hours)
- [ ] Create docs/EXAMPLES_INDEX.md
- [ ] List all examples with descriptions
- [ ] Organize by category and difficulty
- [ ] Add links to source directories
- [ ] Include feature matrix (which examples use which features)
- [ ] Add tags for easy searching

### Phase 3: Quickstart Guide (6-8 hours)
- [ ] Create docs/EXAMPLES_QUICKSTART.md
- [ ] Write "First 10 Minutes" tutorial
- [ ] Write "First Hour" tutorial
- [ ] Write "First Day" tutorial
- [ ] Create learning paths for different roles
- [ ] Add troubleshooting section
- [ ] Test guides with fresh clone

### Phase 4: Integration (2-3 hours)
- [ ] Update examples/README.md to link to new docs
- [ ] Add examples section to main documentation index
- [ ] Link from CONTRIBUTING.md
- [ ] Add to onboarding guides
- [ ] Update website/documentation site (if exists)

### Phase 5: Validation (2-3 hours)
- [ ] Test all links
- [ ] Verify example descriptions are accurate
- [ ] Get feedback from new user
- [ ] Fix any broken examples discovered
- [ ] Ensure builds work for examples

## Acceptance Criteria

### Completeness
- [ ] All 22 numbered examples indexed
- [ ] All standalone examples indexed
- [ ] All specialized example directories indexed
- [ ] Examples categorized by difficulty and topic
- [ ] Dependencies between examples documented

### Quality
- [ ] Accurate descriptions for each example
- [ ] Clear difficulty ratings
- [ ] Working links to all examples
- [ ] Consistent formatting throughout
- [ ] Tested by new user for clarity

### Usability
- [ ] Quickstart guide takes <30 minutes
- [ ] Clear learning paths for different roles
- [ ] Easy to find specific examples
- [ ] Searchable tags or categories
- [ ] Linked from main documentation

### Maintenance
- [ ] Template for adding new examples
- [ ] Instructions for maintaining index
- [ ] Checklist for example submission

## Example Documentation Template

For each example in the index, include:

```markdown
### [Number]. Example Name

**Path:** `examples/XX_example_name/`  
**Difficulty:** Beginner/Intermediate/Advanced  
**Time:** ~XX minutes  
**Features:** Vector Search, Graph Queries, LLM  
**Prerequisites:** Examples 01, 03  

**Description:** Brief description of what this example demonstrates.

**You'll Learn:**
- Specific skill 1
- Specific skill 2
- Specific skill 3

**Quick Start:**
```bash
cd examples/XX_example_name
[build/run commands]
```

**Full Guide:** [README](../examples/XX_example_name/README.md)
```

## Additional Context

**Source:** Documentation-Source Code Gap Analysis  
**Gap Analysis Document:** docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md

**Current Coverage:** ~27% (8 documented out of 30+ total)

**Related Files:**
- examples/README.md (needs expansion)
- examples/*/README.md (individual example docs)
- docs/DOCUMENTATION_INDEX.md (needs examples section)

## Timeline

**Estimated Effort:** 3-4 days (24-32 hours)

**Breakdown:**
- Day 1: Discovery and categorization
- Day 2: Index creation and organization
- Day 3: Quickstart guide writing
- Day 4: Integration, testing, and polish

**Priority:** P2 (Medium) - Important for user onboarding but not critical

**Label:** `good first issue` - Good opportunity for new contributors who want to learn the codebase
