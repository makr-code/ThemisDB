---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Embedding and Vectorization

AI provenance note: Exact model backend not verifiable from final document text alone.

## Method

`DPRVectorizer` implements a dual-encoder interface: query text is sent to the
query encoder and passage text to the passage encoder. Both return fixed-size
vectors intended for dense passage retrieval. The configuration names separate
model paths, a device (`cpu` by default), passage batch size (32 by default),
embedding dimension (384 by default), maximum token length (512 by default),
and optional L2 normalization.

Text is tokenized, truncated to the configured token limit, and padded to that
length. The ONNX path supports common rank-2 and rank-3 outputs; for rank-3
outputs the first token is used, with mean pooling when that vector is nearly
zero. The configured output dimension bounds the copied model output. In the
hybrid helper, cosine similarity between query and passage vectors supplies
the dense score before sparse/dense fusion.

## Assumptions and operational constraints

- Query and passage model artifacts must be a compatible pair trained for
  matching dimensions and preprocessing. A syntactically loadable model is not
  evidence of retrieval relevance.
- `encodeQuery()` and `encodePassage()` require a non-empty input and an
  initialized vectorizer. Batch passage encoding accepts an empty batch.
- A single `DPRVectorizer` instance is documented as not thread-safe; use
  separate instances or external synchronization for concurrent calls.
- The base `IVectorizer` header says implementations must be thread-safe, which
  conflicts with the `DPRVectorizer` class contract. Until that interface
  discrepancy is resolved, follow the stricter DPR rule and externally
  synchronize each instance.
- The fallback embedding is deterministic and shape-preserving, but does not
  have the semantic properties of the configured DPR model.
- The vectorizer's unit tests create placeholder model files and check
  initialization, dimensions, batch shape, and error behavior. They do not
  measure semantic similarity or compare real encoder checkpoints.

## Reproducibility

For functional checks, build and run the focused DPR test registered by
`tests/rag/CMakeLists.txt`:

```bash
cmake --build <build-dir> --target module_rag_test_dpr_vectorizer_focused
ctest --test-dir <build-dir> -R '^test_dpr_vectorizer_RagFocusedTests$' --output-on-failure
```

For a model-quality comparison, record both model artifact hashes, tokenizer
and ONNX Runtime versions, device, dimensions, normalization, truncation,
batch size, dataset version, and relevance judgments. Compare query/passage
encoders with Recall@k or another declared retrieval metric on held-out
judgments. Do not infer model quality from the deterministic fallback or from
the placeholder-artifact unit tests.

## References

- [RAG research notes](./RESEARCH.md)
- [RAG retrieval modeling](./MODELING.md)
- `include/rag/dpr_vectorizer.h`
- `src/rag/dpr_vectorizer.cpp`
- `include/rag/vectorizer_interface.h`
- `tests/rag/test_dpr_vectorizer.cpp`
