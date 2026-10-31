# Embedding Boundary in Metadata

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** module-behavior
**Canonical source references:** [schema type declarations](../../include/metadata/schema_manager.h), [property discovery](./schema_manager.cpp), [schema discovery tests](../../tests/metadata/test_schema_manager.cpp)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Scope

`src/metadata` describes schema and statistics. It is not an embedding model
runner, text-to-vector pipeline, model registry, or vector index implementation.
No embedding-generation API is declared by the metadata interfaces listed in
[`include/metadata`](../../include/metadata/).

## What this module does with vector metadata

- `SchemaManager::PropertyInfo` permits a `"vector"` property type.
- `SchemaManager::TableSchema` documents `"vector"` as one possible table kind.
- Property discovery maps a stored `std::vector<float>` field to the type label
  `"vector"`; it records schema metadata rather than generating the values.
- Validation accepts `"vector"` among recognized property types.
- The schema version migration API maps this type label to SQL `VECTOR` when
  generating a migration script. That mapping is only script generation; it is
  not proof that a database backend accepts or executes the SQL type.

See [`schema_manager.h`](../../include/metadata/schema_manager.h),
[`schema_manager.cpp`](./schema_manager.cpp), and
[`schema_version_manager.h`](../../include/metadata/schema_version_manager.h).

## Developer guidance

1. For a change to schema inference, add or update tests in
   [`test_schema_manager.cpp`](../../tests/metadata/test_schema_manager.cpp)
   and confirm the vector-valued entity fixture exercises the intended
   `std::vector<float>` path.
2. For a change to schema validation or migration SQL mapping, use
   [`test_schema_migration_script.cpp`](../../tests/metadata/test_schema_migration_script.cpp)
   and [`test_schema_migration_regression.cpp`](../../tests/metadata/test_schema_migration_regression.cpp).
3. Keep embedding model selection, normalization, dimensionality constraints,
   and index/search behavior documented by the code that owns those APIs. Do not
   add those claims here unless metadata source contracts explicitly change.

## Non-goals

- Selecting or downloading an embedding model.
- Encoding text, images, or other content into vectors.
- Defining vector dimensions, distance functions, or retrieval quality.
- Persisting embedding provenance or guaranteeing that vectors match a model.

For module orientation, see [MODULE_GUIDE.md](./MODULE_GUIDE.md); for the
metadata's type model, see [MODELING.md](./MODELING.md).
