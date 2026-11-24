// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/graph_summary_projection.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace themis::distributed_tensor {
namespace {

constexpr std::array<uint8_t, 4> kMagic{'T', 'S', 'P', 'J'};
constexpr uint16_t kVersion = 1;
constexpr std::size_t kHeaderBytes = 16;

static_assert(sizeof(float) == sizeof(uint32_t));
static_assert(std::numeric_limits<float>::is_iec559);

[[nodiscard]] bool isSafeText(std::string_view text) noexcept {
  if (text.empty() || text.size() > GraphSummaryProjectionCodec::kMaxTextBytes) {
    return false;
  }

  for (std::size_t index = 0; index < text.size();) {
    const auto first = static_cast<uint8_t>(text[index]);
    if (first < 0x80) {
      if (first < 0x20 || first == 0x7f) {
        return false;
      }
      ++index;
      continue;
    }

    uint32_t code_point = 0;
    std::size_t continuation_count = 0;
    if (first >= 0xc2 && first <= 0xdf) {
      code_point = first & 0x1f;
      continuation_count = 1;
    } else if (first >= 0xe0 && first <= 0xef) {
      code_point = first & 0x0f;
      continuation_count = 2;
    } else if (first >= 0xf0 && first <= 0xf4) {
      code_point = first & 0x07;
      continuation_count = 3;
    } else {
      return false;
    }

    if (continuation_count > text.size() - index - 1) {
      return false;
    }
    for (std::size_t offset = 1; offset <= continuation_count; ++offset) {
      const auto continuation =
          static_cast<uint8_t>(text[index + offset]);
      if ((continuation & 0xc0) != 0x80) {
        return false;
      }
      code_point = (code_point << 6) | (continuation & 0x3f);
    }

    if ((continuation_count == 2 && code_point < 0x800) ||
        (continuation_count == 3 && code_point < 0x10000) ||
        (code_point >= 0xd800 && code_point <= 0xdfff) ||
        code_point > 0x10ffff ||
        (code_point >= 0x80 && code_point <= 0x9f)) {
      return false;
    }
    index += continuation_count + 1;
  }
  return true;
}

[[nodiscard]] bool isValidCandidate(
    const tensor::SimilarityResult& candidate) noexcept {
  return isSafeText(candidate.adapter_key) && isSafeText(candidate.domain) &&
         isSafeText(candidate.base_model_id) &&
         std::isfinite(candidate.score) && candidate.score >= -1.0f &&
         candidate.score <= 1.0f;
}

[[nodiscard]] bool isCanonicalLess(
    const tensor::SimilarityResult& lhs,
    const tensor::SimilarityResult& rhs) noexcept {
  if (lhs.score != rhs.score) {
    return lhs.score > rhs.score;
  }
  return lhs.adapter_key < rhs.adapter_key;
}

[[nodiscard]] bool isValidSummary(
    const tensor::TensorLayerSummary& summary) {
  const bool supported_layer =
      summary.layer_kind == tensor::TensorLayerKind::Adapter ||
      summary.layer_kind == tensor::TensorLayerKind::FingerprintSummary;
  if (!supported_layer ||
      summary.ann_scope_kind != index::AnnScopeKind::Adapter ||
      summary.federated || summary.participating_shards != 0 ||
      !isSafeText(summary.scope_key) ||
      summary.candidate_count != summary.similar_adapters.size() ||
      summary.similar_adapters.empty() ||
      summary.similar_adapters.size() >
          GraphSummaryProjectionCodec::kMaxCandidateCount) {
    return false;
  }

  std::map<std::string_view, bool> seen;
  for (const auto& candidate : summary.similar_adapters) {
    if (!isValidCandidate(candidate) ||
        !seen.emplace(candidate.adapter_key, true).second) {
      return false;
    }
  }
  return true;
}

void appendU16(std::vector<uint8_t>& output, uint16_t value) {
  output.push_back(static_cast<uint8_t>(value >> 8));
  output.push_back(static_cast<uint8_t>(value));
}

void appendU32(std::vector<uint8_t>& output, uint32_t value) {
  output.push_back(static_cast<uint8_t>(value >> 24));
  output.push_back(static_cast<uint8_t>(value >> 16));
  output.push_back(static_cast<uint8_t>(value >> 8));
  output.push_back(static_cast<uint8_t>(value));
}

void appendText(std::vector<uint8_t>& output, std::string_view text) {
  appendU32(output, static_cast<uint32_t>(text.size()));
  output.insert(output.end(), text.begin(), text.end());
}

[[nodiscard]] bool readU16(std::span<const uint8_t> bytes,
                           std::size_t& offset,
                           uint16_t& value) noexcept {
  if (offset > bytes.size() || bytes.size() - offset < 2) {
    return false;
  }
  value = static_cast<uint16_t>(
      (static_cast<uint16_t>(bytes[offset]) << 8) |
      static_cast<uint16_t>(bytes[offset + 1]));
  offset += 2;
  return true;
}

[[nodiscard]] bool readU32(std::span<const uint8_t> bytes,
                           std::size_t& offset,
                           uint32_t& value) noexcept {
  if (offset > bytes.size() || bytes.size() - offset < 4) {
    return false;
  }
  value = (static_cast<uint32_t>(bytes[offset]) << 24) |
          (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
          (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
          static_cast<uint32_t>(bytes[offset + 3]);
  offset += 4;
  return true;
}

[[nodiscard]] bool readText(std::span<const uint8_t> bytes,
                            std::size_t& offset,
                            std::string& value) {
  uint32_t length = 0;
  if (!readU32(bytes, offset, length) ||
      length > GraphSummaryProjectionCodec::kMaxTextBytes ||
      offset > bytes.size() || length > bytes.size() - offset) {
    return false;
  }
  value.assign(reinterpret_cast<const char*>(bytes.data() + offset), length);
  offset += length;
  return isSafeText(value);
}

[[nodiscard]] GraphSummaryProjectionResult failure(
    GraphSummaryProjectionError error) noexcept {
  return GraphSummaryProjectionResult{std::nullopt, error};
}

}  // namespace

std::vector<uint8_t> GraphSummaryProjectionCodec::encode(
    const tensor::TensorLayerSummary& summary) {
  if (!isValidSummary(summary)) {
    throw std::invalid_argument(
        "graph summary projection requires a non-federated adapter candidate summary");
  }

  auto candidates = summary.similar_adapters;
  std::sort(candidates.begin(), candidates.end(), isCanonicalLess);
  for (std::size_t index = 1; index < candidates.size(); ++index) {
    if (candidates[index - 1].adapter_key == candidates[index].adapter_key) {
      throw std::invalid_argument(
          "graph summary projection contains duplicate adapter keys");
    }
  }

  std::size_t payload_size = kHeaderBytes;
  const auto add_size = [&payload_size](std::size_t size) {
    if (size > kMaxPayloadBytes - payload_size) {
      throw std::invalid_argument(
          "graph summary projection exceeds the payload byte limit");
    }
    payload_size += size;
  };
  add_size(sizeof(uint32_t) + summary.scope_key.size());
  for (const auto& candidate : candidates) {
    add_size(sizeof(uint32_t) + candidate.adapter_key.size());
    add_size(sizeof(uint32_t) + candidate.domain.size());
    add_size(sizeof(uint32_t) + candidate.base_model_id.size());
    add_size(sizeof(uint32_t));
  }

  std::vector<uint8_t> output;
  output.reserve(payload_size);
  output.insert(output.end(), kMagic.begin(), kMagic.end());
  appendU16(output, kVersion);
  appendU16(output, 0);
  appendU32(output, static_cast<uint32_t>(candidates.size()));
  appendText(output, summary.scope_key);

  for (const auto& candidate : candidates) {
    appendText(output, candidate.adapter_key);
    appendText(output, candidate.domain);
    appendText(output, candidate.base_model_id);
    const float canonical_score = candidate.score == 0.0f ? 0.0f : candidate.score;
    appendU32(output, std::bit_cast<uint32_t>(canonical_score));
  }
  return output;
}

GraphSummaryProjectionResult GraphSummaryProjectionCodec::decode(
    std::span<const uint8_t> bytes) noexcept {
  if (bytes.size() > kMaxPayloadBytes) {
    return failure(GraphSummaryProjectionError::LimitExceeded);
  }
  if (bytes.size() < kHeaderBytes) {
    return failure(GraphSummaryProjectionError::MalformedPayload);
  }
  if (!std::equal(kMagic.begin(), kMagic.end(), bytes.begin())) {
    return failure(GraphSummaryProjectionError::UnsupportedFormat);
  }

  try {
    std::size_t offset = kMagic.size();
    uint16_t version = 0;
    uint16_t flags = 0;
    uint32_t candidate_count = 0;
    if (!readU16(bytes, offset, version) ||
        !readU16(bytes, offset, flags) ||
        !readU32(bytes, offset, candidate_count)) {
      return failure(GraphSummaryProjectionError::MalformedPayload);
    }
    if (version != kVersion) {
      return failure(GraphSummaryProjectionError::UnsupportedVersion);
    }
    if (flags != 0) {
      return failure(GraphSummaryProjectionError::MalformedPayload);
    }
    if (candidate_count == 0 || candidate_count > kMaxCandidateCount) {
      return failure(GraphSummaryProjectionError::LimitExceeded);
    }

    tensor::TensorLayerSummary summary;
    summary.layer_kind = tensor::TensorLayerKind::Adapter;
    summary.ann_scope_kind = index::AnnScopeKind::Adapter;
    if (!readText(bytes, offset, summary.scope_key)) {
      return failure(GraphSummaryProjectionError::MalformedPayload);
    }

    summary.similar_adapters.reserve(candidate_count);
    std::map<std::string, bool> seen_candidates;
    for (uint32_t index = 0; index < candidate_count; ++index) {
      tensor::SimilarityResult candidate;
      uint32_t score_bits = 0;
      if (!readText(bytes, offset, candidate.adapter_key) ||
          !readText(bytes, offset, candidate.domain) ||
          !readText(bytes, offset, candidate.base_model_id) ||
          !readU32(bytes, offset, score_bits)) {
        return failure(GraphSummaryProjectionError::MalformedPayload);
      }
      candidate.score = std::bit_cast<float>(score_bits);
      if (!isValidCandidate(candidate) ||
          (candidate.score == 0.0f && std::signbit(candidate.score))) {
        return failure(GraphSummaryProjectionError::MalformedPayload);
      }
      if (!seen_candidates.emplace(candidate.adapter_key, true).second) {
        return failure(GraphSummaryProjectionError::NonCanonicalPayload);
      }
      if (!summary.similar_adapters.empty()) {
        const auto& previous = summary.similar_adapters.back();
        if (isCanonicalLess(candidate, previous)) {
          return failure(GraphSummaryProjectionError::NonCanonicalPayload);
        }
      }
      summary.similar_adapters.push_back(std::move(candidate));
    }

    if (offset != bytes.size()) {
      return failure(GraphSummaryProjectionError::MalformedPayload);
    }
    summary.candidate_count = summary.similar_adapters.size();
    summary.routing_reason = "decoded versioned adapter-similarity projection";
    return GraphSummaryProjectionResult{std::move(summary),
                                        GraphSummaryProjectionError::None};
  } catch (...) {
    return failure(GraphSummaryProjectionError::ResourceFailure);
  }
}

GraphSummaryProjectionResult GraphSummaryProjectionCodec::merge(
    std::span<const tensor::TensorLayerSummary> summaries) noexcept {
  if (summaries.empty() || summaries.size() > kMaxCandidateCount) {
    return failure(GraphSummaryProjectionError::LimitExceeded);
  }

  try {
    std::map<std::string, tensor::SimilarityResult> candidates;
    std::string scope_key;
    for (const auto& summary : summaries) {
      if (!isValidSummary(summary)) {
        return failure(GraphSummaryProjectionError::MalformedPayload);
      }
      if (scope_key.empty()) {
        scope_key = summary.scope_key;
      } else if (scope_key != summary.scope_key) {
        return failure(GraphSummaryProjectionError::ConflictingScope);
      }

      for (const auto& candidate : summary.similar_adapters) {
        const auto [it, inserted] =
            candidates.emplace(candidate.adapter_key, candidate);
        if (inserted) {
          if (candidates.size() > kMaxCandidateCount) {
            return failure(GraphSummaryProjectionError::LimitExceeded);
          }
          continue;
        }
        if (it->second.domain != candidate.domain ||
            it->second.base_model_id != candidate.base_model_id) {
          return failure(GraphSummaryProjectionError::ConflictingCandidate);
        }
        if (candidate.score > it->second.score) {
          it->second.score = candidate.score;
        }
      }
    }
    if (candidates.empty()) {
      return failure(GraphSummaryProjectionError::MalformedPayload);
    }

    tensor::TensorLayerSummary merged;
    merged.scope_key = std::move(scope_key);
    merged.layer_kind = tensor::TensorLayerKind::Adapter;
    merged.ann_scope_kind = index::AnnScopeKind::Adapter;
    merged.federated = summaries.size() > 1;
    merged.participating_shards = summaries.size();
    merged.routing_reason = "merged manifest-verified adapter-similarity projections";
    merged.similar_adapters.reserve(candidates.size());
    for (auto& [_, candidate] : candidates) {
      merged.similar_adapters.push_back(std::move(candidate));
    }
    std::sort(merged.similar_adapters.begin(), merged.similar_adapters.end(),
              isCanonicalLess);
    merged.candidate_count = merged.similar_adapters.size();
    return GraphSummaryProjectionResult{std::move(merged),
                                        GraphSummaryProjectionError::None};
  } catch (...) {
    return failure(GraphSummaryProjectionError::ResourceFailure);
  }
}

std::string_view GraphSummaryProjectionCodec::errorCode(
    GraphSummaryProjectionError error) noexcept {
  switch (error) {
    case GraphSummaryProjectionError::None:
      return "NONE";
    case GraphSummaryProjectionError::UnsupportedFormat:
      return "UNSUPPORTED_FORMAT";
    case GraphSummaryProjectionError::UnsupportedVersion:
      return "UNSUPPORTED_VERSION";
    case GraphSummaryProjectionError::MalformedPayload:
      return "MALFORMED_PAYLOAD";
    case GraphSummaryProjectionError::LimitExceeded:
      return "LIMIT_EXCEEDED";
    case GraphSummaryProjectionError::NonCanonicalPayload:
      return "NON_CANONICAL_PAYLOAD";
    case GraphSummaryProjectionError::ConflictingScope:
      return "CONFLICTING_SCOPE";
    case GraphSummaryProjectionError::ConflictingCandidate:
      return "CONFLICTING_CANDIDATE";
    case GraphSummaryProjectionError::ResourceFailure:
      return "RESOURCE_FAILURE";
  }
  return "UNKNOWN";
}

}  // namespace themis::distributed_tensor
