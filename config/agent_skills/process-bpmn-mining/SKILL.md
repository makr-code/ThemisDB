---
name: process-bpmn-mining
description: Guide separately verified process-model YAML, BPMN conversion, and event-log mining without assuming automatic orchestration or permissions.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> AI provenance: Exact model backend not verifiable from final document text
> alone.

# Guide BPMN and process mining

Use this skill to plan or perform only the explicitly requested stages of a
process-analysis task. Treat process-model YAML, BPMN conversion, and event-log
mining as separate operations. A host orchestrator must explicitly invoke and
verify each stage; skills do not chain themselves or grant tool permissions.
Do not claim that a model was changed or deployed.

1. Establish the process-model format, intended outcome, ThemisDB version and
   platform, the user-provided model or event-log source, and which outputs
   are wanted. Verify that any named file or collection is actually available.
   Use only separately authorized tools and data; otherwise provide a plan or
   clearly-labeled draft.
2. **Process-model YAML:** Check the current process-module importer,
   implementation, focused tests, and an applicable real sample before
   drafting or reviewing YAML. `docs/de/process/PROCESS_GRAPH_DIMENSIONS.md`
   is marked `draft`; do not treat its illustrative schema or examples as a
   canonical runtime schema without confirming them against the importer and
   tests. The server configuration YAML schema is not a schema for
   process-model YAML. `VccVpbImporter::importYamlList()` supports a
   configurable top-level list key (default `administrative_models`), but
   this does not establish that an administrative YAML file exists. Do not
   invent its path or claim that an absent file was imported.
3. **BPMN conversion:** Verify the current `BpmnSerializer` API and its tests
   for the requested direction (YAML/process graph to BPMN XML or BPMN XML to
   process graph). Report unsupported or lossy details; for example, the
   serializer documentation says BPMNDI layout hints are imported but not
   emitted. Do not claim that YAML-to-BPMN conversion exists unless a
   concrete, tested adapter is available. Do not assume that
   `ProcessModelRecord`, serializer graph data, and
   `ProcessMining::DiscoveredProcess` are interchangeable. Before using a
   model for conformance checking, verify that a tested adapter exists for
   the exact input representation.
4. **Event-log mining:** Verify the chosen collection and field mappings for
   case ID, activity, and timestamp using an authorized read path. Check for
   missing/empty identifiers or activities, invalid timestamps, timezone
   assumptions, and ambiguous equal-time events. Report exclusions and sort
   assumptions; do not silently repair source records. Confirm each mining
   operation and result status against the current implementation and focused
   tests. The Process-Mining AQL documents conflict about implementation
   status, so examples or design summaries alone do not prove that an AQL
   operation is registered and executable; verify its parser, dispatch path,
   and tests for the target version.
5. Before claiming mining is available, check the actual build platform and
   configuration. In particular, when both `_WIN32` and
   `THEMIS_PROCESS_MINING_WINDOWS_STUB` are defined,
   `src/analytics/process_mining.cpp` provides a stub that returns an error
   for ProcessMining operations. State this limitation and stop or use a
   verified supported deployment; do not report a successful analysis.
6. Present outputs as evidence-backed results: identify the input/model
   version, selected fields and algorithm, event/case counts, validation
   failures, deviations and limitations. Separate observed values from
   inferred explanations. A discovered process is evidence about the selected
   log, not automatically the normative or compliant process. Keep proposed
   model edits separate from the conformance report, and do not apply them
   automatically.

If a stage depends on an unavailable file, unverified AQL integration,
unsupported platform path, missing adapter, or unauthorized tool, state the
blocker and stop that stage. A successful YAML parse, BPMN serialization, or
mining call does not itself authorize the next operation. Do not label an
LLM-generated conformance or compliance assessment as formal certification.
