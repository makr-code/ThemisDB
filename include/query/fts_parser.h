#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis::query::fts {

enum class SearchNodeType : uint8_t {
  TERM = 0,
  PHRASE = 1,
  AND = 2,
  OR = 3,
  NOT = 4,
};

struct SearchNode {
  SearchNodeType type{SearchNodeType::TERM};
  std::string term;
  std::vector<SearchNode> children;
  float boost{1.0f};
  uint32_t proximity_distance{0};
  std::string field;

  // Optional execution annotations consumed by the scorer.
  std::optional<uint32_t> document_frequency;
  std::optional<uint32_t> document_term_frequency;
  std::optional<uint32_t> document_length_tokens;

  static SearchNode makeTerm(std::string value, float term_boost = 1.0f) {
    SearchNode node;
    node.type = SearchNodeType::TERM;
    node.term = std::move(value);
    node.boost = term_boost;
    return node;
  }

  static SearchNode makePhrase(std::string value, float term_boost = 1.0f) {
    SearchNode node;
    node.type = SearchNodeType::PHRASE;
    node.term = std::move(value);
    node.boost = term_boost;
    return node;
  }

  static SearchNode makeBoolean(SearchNodeType boolean_type,
                                std::vector<SearchNode> operands) {
    SearchNode node;
    node.type = boolean_type;
    node.children = std::move(operands);
    return node;
  }
};

}  // namespace themis::query::fts
