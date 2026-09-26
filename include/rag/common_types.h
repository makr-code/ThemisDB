// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>

namespace themis::rag {

/// @brief Represents a retrieved document with score and metadata.
///
/// Used across RAG modules for representing search results and re-ranking targets.
struct Document {
  uint32_t id;           ///< Document ID
  std::string content;   ///< Document text or truncated preview
  float retrieval_score; ///< Original retrieval score [0, 1]
};

}  // namespace themis::rag
