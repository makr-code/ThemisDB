---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/schema_inference.cpp; src/importers/schema_validator.cpp; src/importers/huggingface_ingest_plugin.cpp"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers Data and Schema Modeling

This guide is for developers mapping source records into ThemisDB entities.
The normative cross-connector invariants are in
[`importers_api_contract.h`](../../include/importers/importers_api_contract.h);
actual mappings and supported source types are connector-specific.

## Data path

An importer reads source records, extracts or accepts a schema, applies
configured table/column/type mappings and validation, then writes entities
through its target integration. Optional conflict handling and MDM processing
may follow. `ImportStats` keeps separate totals for imported, failed, skipped,
and quarantined records and includes structured errors; these counters should
not be collapsed into one success count.

The shared [`ImportOptions`](../../include/importers/importer_interface.h)
expresses table and schema filters, mappings, `default_namespace`, ID
preservation, duplicate handling, row-size bounds, schema validation, and
conflict/relationship behavior. These are configuration surfaces; a connector
may support only a subset, so confirm that connector's implementation and
tests before depending on an option.

## Schema inference and evolution

[`schema_inference.cpp`](./schema_inference.cpp) and
[`schema_validator.cpp`](./schema_validator.cpp) implement the module's
inference and validation helpers. `ImportOptions::validate_schema` is disabled
by default and `schema_sample_rows` defaults to 100. A sample can miss later
values or type drift; set sampling and validation deliberately for the source
and risk profile. The shared capability model describes sampling with an
`ALL_TEXT` fallback when schema inference is unavailable.

The contract classifies changes as `NoChange`, `Additive`, or `Breaking`.
Adding nullable columns is additive; removed, renamed, or type-changed
existing columns require an explicit migration. A breaking change without
migration is reported as `IMPORT_SCHEMA_MISMATCH`. Do not treat automatic
inference as a migration plan.

## Identity and conflicts

Source IDs are not necessarily target IDs. `preserve_ids`, the default
namespace, table/column mappings, and connector-specific key conversion
determine identity at the destination. The shared conflict strategies are
`OVERWRITE`, `SKIP`, `MERGE`, and `ERROR`; merge behavior also depends on key
columns, protected fields, and merge depth. A change to key selection or
normalization can therefore change which records collide even when source
rows are unchanged.

The MDM configuration in `ImportOptions` can enable entity linking and
resolution behavior. It is an optional post-processing path; do not assume
every connector invokes it or that a semantic match is a stable primary key.

## Wikipedia and HuggingFace projections

The Wikipedia pipeline stores a canonical page/revision/link/category/
redirect snapshot and derives graph, vector, process, and time-series
projections. Incremental refresh tracks dirty pages and rebuilds projections
for those pages. This projection model is distinct from general connector
schema inference.

The current HuggingFace legal ingest class models documents, annotations,
training examples, and compliance audit records in in-process containers.
Its row source is caller-provided seed or changed rows; see
[`huggingface_ingest_plugin.cpp`](./huggingface_ingest_plugin.cpp). Do not
model it as a live remote dataset connector or durable table writer without a
separately implemented adapter and persistence layer.

## Validation checklist

- Test representative nulls, mixed types, missing keys, and extra fields.
- Exercise additive and breaking changes separately.
- Verify transformed IDs and conflict counters against expected destination
  identity.
- Include records beyond the inference sample and maximum configured row size.
- Verify filtering and relationships against the connector-specific contract.

Related references: [API](./API.md), [implementation](./IMPLEMENTATION.md),
[research and evaluation](./RESEARCH.md).
