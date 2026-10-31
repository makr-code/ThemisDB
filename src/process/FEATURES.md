---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: include/process/process_model_manager.h; include/process/process_graph_rag.h; include/process/process_light_retriever.h; include/process/process_orchestration_registry.h; include/process/process_diagnostics.h; src/process/*.cpp
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Features

**AI provenance:** Exact model backend not verifiable from final document text alone.

This index lists source-visible capabilities, not maturity, performance, or release status. Consult public headers and implementations for exact contracts.

## Module role and scenarios

The process module provides process-model management and related retrieval, linking, import/export, and analytics surfaces to ThemisDB. Typical integration scenarios include importing a process definition, assembling context for a stored process instance, and associating process instances with external objects or documents.

## Capabilities

- Process model import/export, persistence, lifecycle, search, optional embedding/index integrations, and deployment to a supplied `ProcessGraphManager`.
- Process graph retrieval, node-focused context, similar-case lookup, optional PPR scoring, and LOCAL/GLOBAL/AUTO light retrieval.
- Process-object and process-process links, required-document checks, and related analytics/helpers.
- Format-specific serializers/importers, DMN evaluation, OCEL export, and object-centric tracing.
- Agentic retrieval and model descriptor generation; embedding behavior and generated content depend on supplied integrations/input.
- External orchestration profiles with staged/active/archived state, optional catalog synchronization, and prediction output.
- Diagnostics, audit, federation, conflict-resolution, and telemetry interfaces/implementation files.

## Operational value and limitations

These components support process-data integration and retrieval, but do not by themselves provide a general workflow execution service or production deployment controls. The light-retriever resource budgets are currently unenforced; profile activation/rollback is not transactional; audit persistence/delta handling and telemetry export are simplified; federation maturity and guarantees are not independently established.

## Developer references

See the [API guide](API.md), [implementation notes](IMPLEMENTATION.md), [architecture](ARCHITECTURE.md), and [production requirements](PRODUCTION_REQUIREMENTS.md).
