/**
 * @file graph_summary_projection.h
 * @brief Versioned codec for trusted adapter-similarity graph projections.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tensor/tensor_mid_layer.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace themis::distributed_tensor {

/**
 * @brief Decode outcome for one graph-summary fragment.
 */
enum class GraphSummaryProjectionError {
  None,
  UnsupportedFormat,
  UnsupportedVersion,
  MalformedPayload,
  LimitExceeded,
  NonCanonicalPayload,
  ConflictingScope,
  ConflictingCandidate,
  ResourceFailure,
};

/**
 * @brief Result of decoding or merging graph-summary fragments.
 */
struct GraphSummaryProjectionResult {
  std::optional<tensor::TensorLayerSummary> summary;
  GraphSummaryProjectionError error = GraphSummaryProjectionError::None;

  /**
   * @brief Check whether the operation produced a summary.
   * @return True only when summary is present and error is None.
   */
  [[nodiscard]] bool ok() const noexcept {
    return summary.has_value() && error == GraphSummaryProjectionError::None;
  }
};

/**
 * @brief Encodes the explicitly supported graph-candidate payload format.
 *
 * This is a versioned wire format, not a C++ object-layout serialization.
 * It carries adapter similarity candidates only; generic tensor payloads and
 * unrecognized versions cannot be projected into graph evidence.
 */
class GraphSummaryProjectionCodec {
 public:
  static constexpr std::string_view kFormatId =
      "themis.tensor.adapter-similarity/v1";
  static constexpr std::size_t kMaxCandidateCount = 1024;
  static constexpr std::size_t kMaxTextBytes = 4096;
  static constexpr std::size_t kMaxPayloadBytes = 16 * 1024 * 1024;

  /**
   * @brief Encode one non-federated adapter-similarity summary.
   * @param summary Summary with a non-empty scope and unique, finite-scored
   *        adapter candidates; only Adapter or FingerprintSummary layers with
   *        Adapter ANN scope are accepted.
   * @return Canonical, big-endian, version-1 payload.
   * @throws std::invalid_argument if the summary is outside the format contract.
   * @throws std::bad_alloc if memory allocation fails.
   * @note Candidate scores must be finite cosine values in [-1, 1]. Candidate
   *       order is canonicalized by descending score and then adapter key.
   */
  [[nodiscard]] static std::vector<uint8_t> encode(
      const tensor::TensorLayerSummary& summary);

  /**
   * @brief Decode one bounded, versioned graph-summary payload.
   * @param bytes Complete payload bytes; trailing data is rejected.
   * @return A decoded summary, or a structured error for malformed,
   *         unsupported, non-canonical, oversized, or unallocatable input.
   * @note The summary contains advisory candidates only; graph truth must still
   *       be established by GraphTruthValidator.
   */
  [[nodiscard]] static GraphSummaryProjectionResult decode(
      std::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Merge per-fragment summaries into one deterministic federated view.
   * @param summaries Non-empty decoded summaries with one identical scope.
   * @return Merged candidates, deduplicated by adapter key, or a conflict error.
   * @note Duplicate IDs with different domain/model metadata are rejected;
   *       otherwise the highest score is retained.
   */
  [[nodiscard]] static GraphSummaryProjectionResult merge(
      std::span<const tensor::TensorLayerSummary> summaries) noexcept;

  /**
   * @brief Return a stable diagnostic code for a projection error.
   * @param error Error value returned by decode() or merge().
   * @return Static code suitable for structured logs and test assertions.
   */
  [[nodiscard]] static std::string_view errorCode(
      GraphSummaryProjectionError error) noexcept;
};

}  // namespace themis::distributed_tensor
