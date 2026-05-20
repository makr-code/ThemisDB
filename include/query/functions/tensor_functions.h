/*
 * ThemisDB | File: tensor_functions.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
 * Tensor arguments can be supplied either directly as objects
 * `{data:[...], shape:[...], eps?:...}` or as field-path strings resolved
 * against `FunctionContext` (current document / variables).
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
