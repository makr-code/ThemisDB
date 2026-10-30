---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding Data in Export Pipelines

Exact model backend not verifiable from final document text alone.

## Scope boundary

The `exporters` module does not expose an embedding-generation model or
embedding API. It exports entity fields provided by the caller. A vector-valued
field can therefore be part of an entity's payload only to the extent that the
selected format's serializer supports its value type; this module does not
compute, normalize, validate semantic dimensions, or assign meaning to that
vector.

Do not treat JSONL LLM formatting, Hugging Face packaging, or metadata fields
as evidence that embeddings are generated or that a model has been evaluated.
Embedding generation and vector-index semantics belong to their owning
modules. This page documents only export-side preservation and validation
responsibilities.

## Exporting records that contain vectors

Before export, establish and record the owning vector contract: field name,
numeric representation, dimension, normalization convention, model/version,
and whether null or missing vectors are allowed. The exporter receives those
values as record data; its `include_fields`, `exclude_fields`, and configured
format/schema determine whether they reach the artifact.

Use explicit schema declarations when available and verify a round trip with
the actual consumer. In particular:

- Confirm the selected Parquet/Arrow path maps the vector's nested or repeated
  representation as intended. A column hint for scalar `INT64`, `DOUBLE`,
  `STRING`, or `BOOLEAN` is not a declaration of vector dimensionality.
- For JSONL, check that the serialized field remains numeric and that any
  template mapping does not omit it. JSONL LLM templates are task-record
  renderers and need not preserve arbitrary entity fields.
- For Hugging Face datasets, configure features when inferred features from
  the first batch would not express the required nested representation.
- Confirm filters and field projections retain the intended vector field.
  Inspect artifact contents; a successful `ExportStats` result alone is not
  schema validation.

## Validation and failure handling

Test empty and null vectors, wrong dimensions, non-finite numeric values,
heterogeneous values, and large vectors in the exporter that owns the output
format. The exporter does not promise to detect semantic dimension mismatch;
validate that invariant upstream or in the consumer-specific integration.
Treat schema mismatch or serialization failure as a failed artifact and do not
publish it.

Keep sensitive vector payloads inside the same authorization, filtering,
encryption, and retention controls as other exported data. See
[TRUST.md](./TRUST.md) for the egress boundary and
[MODELING.md](./MODELING.md) for format/schema choices.

## Source evidence

- Generic record and field projection contract:
  `include/exporters/exporter_interface.h`.
- Columnar format configuration:
  `include/exporters/parquet_exporter.h` and
  `include/exporters/arrow_ipc_exporter.h`.
- JSONL mapping and template configuration:
  `include/exporters/jsonl_llm_exporter.h` and
  `include/exporters/format_template.h`.
- Hugging Face feature inference/configuration:
  `include/exporters/huggingface_exporter.h`.
