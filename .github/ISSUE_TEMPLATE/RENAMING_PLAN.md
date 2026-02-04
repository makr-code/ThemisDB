# GitHub Issue Template Renaming & Organization Plan

## Problem Statement (German)
Wir haben in den GitHub Issue Templates verschiedene Arten von Vorlagen:
- Leere Vorlagen für Benutzer
- Automatische wiederholbare KI-Aufgaben  
- Fragmente von bereits erledigten oder offenen Aufgaben

Die Templates sollten besser benannt werden und genauer beschrieben werden, um sie unterscheidbar zu machen.

## Current State Analysis

### Categories Identified:
1. **User-Facing Templates (4)** - Bug reports, feature requests, documentation
2. **AI Systematic Review Templates (5)** - Repeatable quarterly component reviews
3. **Implementation Task Templates (23)** - Specific features and enhancements
4. **Security Analysis Templates (6)** - Attack vector and compliance analysis
5. **Research Templates (5)** - Paper investigation and technology research
6. **Documentation Files (9)** - Guides and READMEs (should be moved)

### Naming Issues:
- No consistent prefix convention
- Mix of UPPERCASE and lowercase
- Documentation files mixed with templates
- Hard to distinguish template types at a glance

## Proposed Naming Convention

### Prefix System:
- **No prefix** - User-facing templates (bug, feature, docs)
- **`ai-review-`** - Repeatable AI systematic review templates
- **`task-`** - Specific implementation tasks
- **`security-`** - Security and compliance analysis
- **`research-`** - Research and investigation templates

### Template Renaming Map

#### User-Facing (Keep Simple Names):
```
bug_report.md                           → bug_report.md (no change)
feature_request.md                      → feature_request.md (no change)
documentation_improvement.md            → documentation_improvement.md (no change)
documentation_issue.md                  → documentation_issue.md (no change)
```

#### AI Systematic Review Templates:
```
SYSTEMATIC_COMPONENT_REVIEW_TEMPLATE.md → ai-review-component-template.md
core_database_component_review.md       → ai-review-database-components.md
ai_llm_component_review.md              → ai-review-llm-components.md
distributed_systems_component_review.md → ai-review-distributed-systems.md
network_api_component_review.md         → ai-review-network-api.md
```

#### Security Templates:
```
security_network_attack.md              → security-attack-network.md (no change in prefix)
security_authentication_attack.md       → security-attack-authentication.md
security_injection_attack.md            → security-attack-injection.md
security_crypto_attack.md               → security-attack-cryptography.md
security_distributed_attack.md          → security-attack-distributed.md
security_compliance_systematic_investigation.md → security-compliance-investigation.md
```

#### Implementation Tasks - Sharding:
```
sharding_bug.md                         → task-sharding-bug-report.md
sharding_feature.md                     → task-sharding-feature.md
sharding_performance.md                 → task-sharding-performance.md
consensus_implementation.md             → task-consensus-implementation.md
transaction_implementation.md           → task-transaction-implementation.md
```

#### Implementation Tasks - Video Processing:
```
video_batch_processing.md               → task-video-batch-processing.md
video_hardware_acceleration.md          → task-video-hardware-acceleration.md
video_jpeg_encoding.md                  → task-video-jpeg-encoding.md
video_multiple_thumbnails.md            → task-video-multiple-thumbnails.md
video_scene_detection.md                → task-video-scene-detection.md
video_streaming_support.md              → task-video-streaming-support.md
video_subtitle_extraction.md            → task-video-subtitle-extraction.md
```

#### Implementation Tasks - RoPE:
```
rope_cuda_hip_kernels.md                → task-rope-cuda-hip-kernels.md
rope_learned_parameters.md              → task-rope-learned-parameters.md
rope_lora_integration.md                → task-rope-lora-integration.md
rope_rest_api.md                        → task-rope-rest-api.md
rope_visualization.md                   → task-rope-visualization.md
```

#### Research Templates:
```
research_paper_investigation.md         → research-paper-investigation.md (no change)
gpu_indexing_research.md                → research-gpu-indexing.md
learned_index_research.md               → research-learned-indexes.md
product_quantization_research.md        → research-product-quantization.md
vector_indexing_research.md             → research-vector-indexing.md
```

#### Other Implementation Tasks:
```
faiss_migration.md                      → task-faiss-migration.md
ethics_plugin_cpp_implementation.md     → task-ethics-plugin-implementation.md
```

#### Documentation Files (Move to _guides/ subdirectory):
```
README.md                               → _guides/README.md
TEMPLATES_README.md                     → _guides/templates-overview.md
TEMPLATE_SELECTION.md                   → _guides/template-selection-guide.md
TEMPLATE_AUSWAHL_DE.md                  → _guides/template-auswahl-guide-de.md
SYSTEMATIC_REVIEW_GUIDE.md              → _guides/systematic-review-guide.md
RESEARCH_TEMPLATES_GUIDE.md             → _guides/research-templates-guide.md
SECURITY_TEMPLATES_GUIDE.md             → _guides/security-templates-guide.md
SHARDING_TEMPLATES_README.md            → _guides/sharding-templates-guide.md
IMPLEMENTATION_SUMMARY.md               → _guides/implementation-summary.md
EXAMPLE_REVIEW.md                       → _guides/example-review.md
SYSTEMATIC_INVESTIGATION_EXAMPLE.md     → _guides/systematic-investigation-example.md
```

#### Ethics AI Subdirectory (Keep as is, but rename for clarity):
```
ethics_ai/                              → ethics-ai-tasks/
```

## Missing Repeatable Templates to Add

Based on analysis, we need these new repeatable AI task templates:

1. **`ai-review-performance-optimization.md`**
   - Repeatable performance audit template
   - Benchmarking, profiling, optimization opportunities
   
2. **`ai-review-api-design.md`**
   - API design review template
   - REST, gRPC, GraphQL API consistency
   
3. **`ai-review-testing-quality.md`**
   - Test coverage and quality review
   - Unit, integration, e2e test analysis
   
4. **`ai-review-documentation-audit.md`**
   - Documentation completeness review
   - User docs, API docs, architecture docs
   
5. **`ai-review-migration-planning.md`**
   - Template for planning major migrations
   - Dependency analysis, risk assessment, rollout plan

## Benefits

### Clarity:
- Templates grouped by prefix show purpose at a glance
- Easy to find the right template for the task

### Discoverability:
- Alphabetical sorting groups related templates
- GitHub's template picker shows clear categories

### Maintainability:
- Documentation separated from templates
- Consistent naming makes automation easier

### Usability:
- Users can quickly identify user-facing templates
- Developers find implementation task templates easily
- AI reviewers find systematic review templates clearly

## Implementation Steps

1. Create `_guides/` subdirectory
2. Move documentation files to `_guides/`
3. Rename templates according to map
4. Update front matter (name, about) in each template
5. Update references in moved documentation files
6. Create new missing repeatable templates
7. Update main `.github/ISSUE_TEMPLATE/` README
8. Test template selection in GitHub UI

## Success Criteria

- [ ] All templates have clear, descriptive prefixes
- [ ] Documentation files separated from templates
- [ ] Front matter updated with clear descriptions
- [ ] Missing repeatable templates added
- [ ] All cross-references updated
- [ ] README reflects new structure
- [ ] Templates validated (YAML, formatting)

---

**Created:** 2026-02-02
**Status:** Plan Ready for Implementation
