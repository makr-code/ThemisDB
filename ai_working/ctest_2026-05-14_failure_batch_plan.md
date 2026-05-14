# CTest Failure Batch Plan (2026-05-14)

## Failing tests
- GrammarTest.ModelAwareConstructor_NullModel_SetsError
- GGUFMetadata.GMD09_injected_hmacfn_overrides_default_path
- GNNEmbeddingTest.NeighborAggregationImpactsEmbedding
- GNNEmbeddingTest.AggregationStrategies_ProduceDifferentEmbeddings

## Root-cause hypothesis
- Grammar: model-aware constructor checks grammar API availability before null model check, causing wrong error text.
- GGUFMetadata: injected HmacFn path enforces hex-SHA256 format, but test contract expects full override behavior.
- GNN embeddings: current feature/aggregation pipeline can produce colinear vectors, causing cosine similarity ~1 across differing neighborhoods/strategies.

## Change set
1. Update `src/llm/grammar.cpp`
   - Prioritize null model validation in model-aware ctor.
2. Update `src/storage/gguf_metadata.cpp`
   - For injected HmacFn, accept non-empty injected signatures verbatim for sign/verify override path.
3. Update `src/index/gnn_embeddings.cpp`
   - Add deterministic structural/strategy signals in neighbor aggregation path to avoid colinearity and preserve similarity bounds.

## Validation
- Build relevant target (`themis_tests`) and run focused CTest filters for the four failures.
- If green, continue full CTest monitoring and catch further regressions.
