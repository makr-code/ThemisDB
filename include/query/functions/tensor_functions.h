/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_functions.h                                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_functions.h
 * @brief AQL built-in functions for Tensor-Network operations.
 *
 * Provides the `tensor` category of AQL built-in functions:
 *
 * | Function           | Signature                                     | Description                              |
 * |--------------------|-----------------------------------------------|------------------------------------------|
 * | TENSOR_SIMILARITY  | (field_a, field_b) → Float                    | Cosine similarity in TT-compressed domain|
 * | TENSOR_NORM        | (field) → Float                               | Frobenius norm without decompression     |
 * | TENSOR_SLICE       | (field, dim: Int, idx: Int) → TT-field        | Subtensor along one mode                 |
 * | TENSOR_COMPRESS    | (field, eps: Float, rank: Int) → TT-field     | On-the-fly TT re-compression             |
 * | TENSOR_INFO        | (field) → Document                            | Metadata: rank, eps, compression_ratio   |
 *
 * All functions operate on tensors previously stored via
 * `TensorNetworkStorageEngine`.  Field arguments are passed as document
 * field paths (strings), and the engine is resolved via the global singleton
 * registered during server startup.
 *
 * ### Registration
 * Call `registerTensorFunctions(registry)` from `function_registry.cpp`.
 * Guard with `#ifdef THEMIS_ENABLE_TENSOR_NETWORK` (enabled by default).
 */

#pragma once

#include "query/functions/function_registry.h"

namespace themis {
namespace query {
namespace functions {

/**
 * @brief Register all TENSOR_* built-in functions with the AQL registry.
 *
 * @param registry  The FunctionRegistry singleton.
 */
void registerTensorFunctions(FunctionRegistry& registry);

} // namespace functions
} // namespace query
} // namespace themis
