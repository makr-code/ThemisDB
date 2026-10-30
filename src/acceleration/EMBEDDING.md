---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding and vector-similarity paths

## Purpose and scope

This page distinguishes model inference that produces embeddings from vector similarity that compares already-produced vectors. The acceleration module provides dispatch surfaces for both, but it does not define an embedding model, tokenize text, or own the vector index. See [API.md](./API.md) for the public request types and [IMPLEMENTATION.md](./IMPLEMENTATION.md) for the dispatch flow.

## Embedding inference request

`AiInferenceRequest` carries a host-side FP32 tensor (`input_data`, `input_elements`, `input_shape`), a `model_path`, a `task_tag` such as `"embedding"`, and a preferred precision. The caller must supply valid storage for the duration of the synchronous `run()` call. The dispatcher probes available providers and attempts the preferred precision where supported, falling back to FP32 on that candidate when available.

Do not treat the generic CPU fallback as an embedding model. In `dispatchCpuFallback()`, non-similarity work validates that the input pointer is non-null and the element count is non-zero, then returns an identity copy of the input tensor. It does not load `model_path` or generate an embedding. The ONNX Runtime path requires non-empty input and a model path; actual model output and provider availability depend on the linked runtime and model. A successful identity-copy result is therefore not evidence that an embedding model ran.

## Vector similarity request

For vector-similarity tasks, the request supplies:

| Field | Contract |
|---|---|
| `input_data` | Query matrix in host memory |
| `input_elements` | Must equal `similarity_num_queries * similarity_dim` |
| `similarity_corpus` | Non-null matrix of `similarity_num_vectors` vectors |
| `similarity_num_queries`, `similarity_num_vectors`, `similarity_dim` | Each must be greater than zero |
| `similarity_top_k` | Must be greater than zero; execution clamps `k` to the corpus size |
| `similarity_metric` | Distance metric; dispatch also derives metric selection from supported task tags |

The result contains flattened `topk_indices` and `topk_distances`, with `k` entries per query. Invalid pointers, zero dimensions/counts, a query element-count mismatch, or zero `top_k` return a failed result with an explanatory error. A missing distance or TopK launcher also fails the dispatch.

When CUDA is compiled in, the GPU similarity path initializes the CUDA vector backend and uses its ANN dispatch table. Initialization or kernel failure routes to the CPU similarity path. Without CUDA, the request is sent directly to that CPU path. This is not a claim that all embedding or inference tasks use CUDA.

## Integration and validation

1. Generate or load the query embedding in the owning model/inference component.
2. Keep query and corpus data alive and laid out as contiguous FP32 matrices for the call; use the same dimension and metric on both paths.
3. Check `success` and preserve `error` diagnostics. Do not consume `topk_*` vectors after a failed result.
4. Compare the selected backend's indices/distances against CPU behavior on representative dimensions, batch sizes, and metrics.
5. Run the focused AI dispatcher and vector similarity tests from [tests/acceleration/README.md](../../tests/acceleration/README.md); hardware claims require a hardware-backed benchmark run.

## Source references

- [AI inference request and result](../../include/acceleration/ai_hardware_dispatcher.h)
- [Similarity validation and GPU dispatch](./ai_hardware_dispatcher.cpp)
- [Resource-managed similarity fallback](./vllm_resource_manager.cpp)
- [Focused test inventory](../../tests/acceleration/README.md)

**Exact model backend not verifiable from final document text alone.**
