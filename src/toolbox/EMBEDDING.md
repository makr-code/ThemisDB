---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox vector and embedding handoff

**Purpose and audience:** Defines what `src/toolbox` does and does not guarantee when ingestion workflows produce vector records. Intended for RAG, ingestion, and toolbox integrators.

## Ownership boundary

The toolbox module does not define or implement an embedding model. Its extraction API returns an `ingestion::BaseEntitySet`, whose `chunks` are exposed as vector records. The configured ingestion workflow and its steps determine how those records are produced. The content subsystem separately performs its own content-ingest processing; model selection, vector dimension, embedding quality, and index schema are not specified by the toolbox contract.

Relevant contracts:

- [`IngestionToolbox::extractEntitySet`](../../include/toolbox/ingestion_toolbox.h) returns nodes, edges, and chunks; callers needing vector records must use this rather than `extractEntities()`.
- [`ToolboxBuilder::withVectorWriter` and `buildWithBridges`](../../include/toolbox/toolbox_builder.h) configure an optional vector sink and may wire a RAG bridge.
- [`ContentToolboxBridge::ingest`](../../include/toolbox/content_toolbox_bridge.h) runs content ingestion first, then toolbox enrichment, and returns both entity and vector outputs.

## Data flow

1. `ContentToolboxBridge::ingest` sends raw bytes through `ContentManager::ingestRawBlob` and retrieves assembled text.
2. If no text is available, the bridge returns success with the stored content identifiers and skips toolbox enrichment.
3. For available text, the bridge calls `extractEntitySet`; its `chunks` become `BridgeResult::vectors`.
4. If a vector writer was configured and the vector list is non-empty, the bridge calls `writeVectors`.
5. A vector-sink failure increments `vectorWriteFailuresTotal()` and is logged, but is a soft failure: the bridge result can still have `ok == true`. Check the sink-failure counter as well as `ok`.

The corresponding implementation is [`content_toolbox_bridge.cpp`](./content_toolbox_bridge.cpp); sink failures and empty-content behavior have integration coverage in [`tests/toolbox/README.md`](../../tests/toolbox/README.md).

## Integration invariants

- Treat `BridgeResult::vectors` as workflow output, not proof that vectors were durably persisted.
- `sinks_written` is true if at least one configured graph or vector sink write succeeds; it does not mean every configured sink succeeded.
- An absent vector writer is permitted. In that case the bridge may return vector records without persisting them through that sink.
- Empty or unextractable text can produce an `ok` result with no vectors. Distinguish this from sink failure using result fields and the bridge's separate counters.
- The bridge does not validate compatibility between an embedding model and the vector index. The integrating service must enforce its own model/version/dimension and collection-schema contract.

## Evaluation

Test the complete integration with a deterministic workflow and test vector writer. Assert that expected records reach the writer, writer errors increment `vectorWriteFailuresTotal()`, absent writers do not cause a hard failure, and empty text skips enrichment. Measure embedding quality and model-specific latency in the subsystem that owns the embedding implementation; do not use `BM_BridgeProxy_EntitySetLatency` as an embedding-quality or bridge-latency result because it measures `IngestionToolbox::extractEntitySet()` without `ContentToolboxBridge` or `ContentManager`.

## References

- [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- [`IMPLEMENTATION.md`](./IMPLEMENTATION.md)
- [`TRUST.md`](./TRUST.md)
- [`include/toolbox/content_toolbox_bridge.h`](../../include/toolbox/content_toolbox_bridge.h)

> Exact model backend not verifiable from final document text alone.
