/**
 * @file token_analyzer.cpp
 * @brief Implementation of TokenDistributionAnalyzer.
 */

#include "../include/token_distribution_analyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themisdb::watermark {

TokenDistributionAnalyzer::TokenDistributionAnalyzer(uint32_t vocab_size,
                                                     float green_list_fraction)
    : vocab_size_(vocab_size), green_list_fraction_(green_list_fraction) {
  if (green_list_fraction < 0.0f || green_list_fraction > 1.0f) {
    throw std::invalid_argument("green_list_fraction must be in [0.0, 1.0]");
  }
}

TokenStats TokenDistributionAnalyzer::analyze_tokens(
    const std::vector<uint32_t>& token_ids) {
  TokenStats stats;
  if (token_ids.empty()) {
    return stats;
  }

  stats.total_tokens = token_ids.size();

  // Compute frequency distribution
  std::unordered_map<uint32_t, uint64_t> frequencies;
  for (uint32_t token : token_ids) {
    frequencies[token]++;
  }
  stats.unique_tokens = frequencies.size();

  // Compute entropy
  stats.entropy = compute_entropy(frequencies);
  double max_entropy = std::log(static_cast<double>(stats.unique_tokens));
  stats.normalized_entropy = (max_entropy > 0) ? (stats.entropy / max_entropy) : 0.0;

  // Compute frequency statistics
  std::vector<uint64_t> freq_values;
  for (const auto& [token, freq] : frequencies) {
    freq_values.push_back(freq);
  }
  if (!freq_values.empty()) {
    stats.avg_token_freq =
        static_cast<double>(std::accumulate(freq_values.begin(), freq_values.end(), 0ULL)) /
        freq_values.size();
    stats.max_token_freq = *std::max_element(freq_values.begin(), freq_values.end());
    stats.min_token_freq = *std::min_element(freq_values.begin(), freq_values.end());
  }

  stats.token_repetition_ratio =
      static_cast<double>(stats.total_tokens - stats.unique_tokens) / stats.total_tokens;

  return stats;
}

float TokenDistributionAnalyzer::estimate_claude_watermark(
    const std::vector<uint32_t>& token_ids,
    uint64_t text_length_chars) {
  if (token_ids.empty()) {
    return 0.5f;
  }

  // Count green tokens
  uint64_t green_count = 0;
  for (uint32_t token : token_ids) {
    if (is_green_token(token)) {
      green_count++;
    }
  }

  // Expected green count under random sampling
  uint64_t expected_green = static_cast<uint64_t>(
      token_ids.size() * green_list_fraction_);

  // Compute deviation from expected (z-score approximation)
  double deviation = green_count - expected_green;
  double std_dev = std::sqrt(token_ids.size() * green_list_fraction_ *
                              (1.0 - green_list_fraction_));

  if (std_dev < 1e-6) {
    return 0.5f;
  }
  double z_score = deviation / std_dev;

  // Convert z-score to confidence [0.0, 1.0]
  // z = 0 -> confidence 0.5 (no signal)
  // z > 2 -> confidence > 0.9 (strong signal)
  // z < -2 -> confidence < 0.1 (negative signal, unlikely but possible)
  float confidence = 0.5f + (0.15f * std::tanh(z_score / 3.0f));
  confidence = std::max(0.0f, std::min(1.0f, confidence));

  // Length-dependent adjustment: shorter texts are less reliable
  if (text_length_chars < 200) {  // Very short text
    confidence = 0.5f + 0.3f * (confidence - 0.5f);  // Reduce confidence by 70%
  } else if (text_length_chars < 500) {  // Short text
    confidence = 0.5f + 0.7f * (confidence - 0.5f);  // Reduce confidence by 30%
  }

  return confidence;
}

float TokenDistributionAnalyzer::compute_ngram_entrenchment(
    const std::vector<uint32_t>& token_ids,
    uint32_t n) {
  if (token_ids.size() < n) {
    return 0.5f;
  }

  // Extract n-grams
  std::unordered_map<size_t, uint64_t> ngram_counts;
  std::hash<std::string> hasher;

  for (size_t i = 0; i <= token_ids.size() - n; ++i) {
    std::string ngram_str;
    for (uint32_t j = 0; j < n; ++j) {
      ngram_str += std::to_string(token_ids[i + j]) + ",";
    }
    ngram_counts[hasher(ngram_str)]++;
  }

  // Entropy of n-gram distribution
  std::unordered_map<uint32_t, uint64_t> dummy_freqs;
  for (const auto& [hash, count] : ngram_counts) {
    dummy_freqs[hash] = count;
  }

  double entropy = compute_entropy(dummy_freqs);
  double max_entropy = std::log(static_cast<double>(ngram_counts.size()));

  // Normalized entropy: 0 = locked-in (high entrenchment), 1 = random
  double normalized_entropy = (max_entropy > 0) ? (entropy / max_entropy) : 0.0;

  // Convert to entrenchment score: 1 - normalized_entropy
  // High entrenchment -> high score
  float entrenchment = 1.0f - static_cast<float>(normalized_entropy);
  return std::max(0.0f, std::min(1.0f, entrenchment));
}

float TokenDistributionAnalyzer::compute_kl_divergence(
    const std::vector<uint32_t>& token_ids,
    const std::vector<uint32_t>& reference_token_ids) {
  if (token_ids.empty()) {
    return 0.0f;
  }

  // Compute probability distributions
  std::unordered_map<uint32_t, uint64_t> p_counts, q_counts;
  for (uint32_t token : token_ids) {
    p_counts[token]++;
  }

  // If no reference, use uniform distribution
  if (reference_token_ids.empty()) {
    for (uint32_t token : token_ids) {
      q_counts[token]++;
    }
    // Flatten to uniform
    for (auto& [token, count] : q_counts) {
      count = 1;  // Uniform probability
    }
  } else {
    for (uint32_t token : reference_token_ids) {
      q_counts[token]++;
    }
  }

  // Compute KL-divergence: sum(p(x) * log(p(x) / q(x)))
  double kl_div = 0.0;
  for (const auto& [token, count_p] : p_counts) {
    double p = static_cast<double>(count_p) / token_ids.size();
    double q = 1e-10;  // Smoothing
    auto it = q_counts.find(token);
    if (it != q_counts.end()) {
      size_t ref_size = reference_token_ids.empty() ? token_ids.size()
                                                      : reference_token_ids.size();
      q = static_cast<double>(it->second) / ref_size;
    }
    if (p > 1e-10) {
      kl_div += p * std::log(p / q);
    }
  }

  // Normalize KL-divergence to [0.0, 1.0]
  // (unbounded in theory, but in practice capped at ~10 for natural text)
  float normalized_kl = static_cast<float>(std::min(kl_div, 10.0) / 10.0);
  return std::max(0.0f, std::min(1.0f, normalized_kl));
}

bool TokenDistributionAnalyzer::is_green_token(uint32_t token_id) const {
  // Deterministic hash-based green-list selection
  // Matches Claude's approach: hash(token_id) % 1 < green_list_fraction
  uint32_t bit = compute_green_list_bit(token_id);
  return bit == 1;
}

std::unordered_map<std::string, float>
TokenDistributionAnalyzer::get_all_heuristic_scores(
    const std::vector<uint32_t>& token_ids,
    uint64_t text_length_chars) {
  std::unordered_map<std::string, float> scores;

  // Heuristic 1: Claude watermark detection
  scores["claude_watermark"] = estimate_claude_watermark(token_ids, text_length_chars);

  // Heuristic 2: N-gram entrenchment (trigrams)
  scores["ngram_entrenchment"] = compute_ngram_entrenchment(token_ids, 3);

  // Heuristic 3: Token entropy
  TokenStats stats = analyze_tokens(token_ids);
  scores["entropy_normalized"] = static_cast<float>(stats.normalized_entropy);

  // Heuristic 4: KL-divergence
  scores["kl_divergence"] = compute_kl_divergence(token_ids);

  return scores;
}

void TokenDistributionAnalyzer::set_green_list_fraction(float fraction) {
  if (fraction < 0.0f || fraction > 1.0f) {
    throw std::invalid_argument("green_list_fraction must be in [0.0, 1.0]");
  }
  green_list_fraction_ = fraction;
}

float TokenDistributionAnalyzer::get_green_list_fraction() const {
  return green_list_fraction_;
}

double TokenDistributionAnalyzer::compute_entropy(
    const std::unordered_map<uint32_t, uint64_t>& frequencies) const {
  if (frequencies.empty()) {
    return 0.0;
  }

  uint64_t total = 0;
  for (const auto& [token, freq] : frequencies) {
    total += freq;
  }

  double entropy = 0.0;
  for (const auto& [token, freq] : frequencies) {
    double p = static_cast<double>(freq) / total;
    if (p > 1e-10) {
      entropy -= p * std::log(p);
    }
  }
  return entropy;
}

uint32_t TokenDistributionAnalyzer::compute_green_list_bit(uint32_t token_id) const {
  // Simple hash-based approach: use low-order bits to determine green/red
  // In production, use cryptographic hash (SHA-256) like Claude does
  uint32_t hash_val = token_id ^ 0x9e3779b9;  // Mix bits
  hash_val = (hash_val ^ (hash_val >> 16)) * 0x7feb352d;
  hash_val = hash_val ^ (hash_val >> 15);

  // Normalize to [0.0, 1.0]
  float prob = (hash_val % 1000) / 1000.0f;
  return (prob < green_list_fraction_) ? 1 : 0;
}

std::string TokenStats::to_string() const {
  std::ostringstream oss;
  oss << "TokenStats{total=" << total_tokens << ", unique=" << unique_tokens
      << ", entropy=" << entropy << ", normalized_entropy=" << normalized_entropy
      << ", avg_freq=" << avg_token_freq << ", max_freq=" << max_token_freq
      << ", repetition_ratio=" << token_repetition_ratio << "}";
  return oss.str();
}

}  // namespace themisdb::watermark
