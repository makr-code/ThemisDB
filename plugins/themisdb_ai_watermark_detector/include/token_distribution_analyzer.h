/**
 * @file token_distribution_analyzer.h
 * @brief Statistical analysis of token distributions for watermark detection.
 *
 * Implements heuristics for detecting Claude's green/red list watermarking
 * and other statistical anomalies in token sequences.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace themisdb::watermark {

/**
 * @struct TokenStats
 * @brief Statistical metrics for a token sequence.
 */
struct TokenStats {
  uint64_t total_tokens = 0;      /**< Total number of tokens */
  uint64_t unique_tokens = 0;     /**< Number of unique token types */
  double entropy = 0.0;           /**< Shannon entropy of token distribution */
  double avg_token_freq = 0.0;    /**< Average frequency per token */
  double max_token_freq = 0.0;    /**< Maximum frequency of any token */
  double min_token_freq = 0.0;    /**< Minimum frequency of any token */
  double token_repetition_ratio = 0.0; /**< (total - unique) / total */
  double normalized_entropy = 0.0; /**< Entropy normalized to [0, 1] */

  /**
   * @brief Convert stats to string for logging.
   * @return Formatted statistics string
   */
  std::string to_string() const;
};

/**
 * @class TokenDistributionAnalyzer
 * @brief Analyzes token distributions to detect watermarks.
 *
 * Implements heuristics for Claude watermark detection:
 * - Green/red list pattern matching (token IDs in expected ranges)
 * - Entropy-based anomaly detection
 * - N-gram entrenchment analysis
 * - Frequency distribution shifts (Kullback-Leibler divergence)
 */
class TokenDistributionAnalyzer {
public:
  /**
   * @brief Initialize analyzer with tokenizer configuration.
   *
   * @param vocab_size Size of tokenizer vocabulary (e.g., 50257 for GPT-2)
   * @param green_list_fraction Fraction of tokens designated as "green" (default: 0.5)
   */
  explicit TokenDistributionAnalyzer(uint32_t vocab_size = 50000,
                                     float green_list_fraction = 0.5f);

  virtual ~TokenDistributionAnalyzer() = default;

  /**
   * @brief Analyze token sequence and extract statistical features.
   *
   * Computes entropy, frequency distributions, and watermark heuristic scores.
   *
   * @param token_ids Vector of token IDs from tokenizer
   * @return TokenStats with computed metrics
   */
  virtual TokenStats analyze_tokens(const std::vector<uint32_t>& token_ids);

  /**
   * @brief Estimate Claude watermark confidence from token distribution.
   *
   * Detects green/red list pattern typical of Claude's watermarking.
   * Returns confidence [0.0, 1.0] where:
   * - 0.0 = no watermark detected
   * - 1.0 = very strong watermark signal
   *
   * Algorithm:
   * 1. Classify tokens as "green" (designated by Claude) or "red" (not)
   * 2. Compute green-list bias (excess green tokens vs. random expectation)
   * 3. Apply length-dependent confidence adjustment
   * 4. Return confidence score
   *
   * @param token_ids Vector of token IDs
   * @param text_length_chars Character count (for length-dependent adjustment)
   * @return Confidence [0.0, 1.0]
   */
  virtual float estimate_claude_watermark(const std::vector<uint32_t>& token_ids,
                                          uint64_t text_length_chars);

  /**
   * @brief Detect n-gram "entrenchment" patterns.
   *
   * Computes how "locked-in" n-gram patterns are (entropy reduction
   * vs. random text). High entrenchment can indicate AI generation.
   *
   * @param token_ids Vector of token IDs
   * @param n N-gram order (e.g., 3 for trigrams)
   * @return Entrenchment score [0.0, 1.0]
   */
  virtual float compute_ngram_entrenchment(const std::vector<uint32_t>& token_ids,
                                           uint32_t n = 3);

  /**
   * @brief Compute Kullback-Leibler divergence from reference distribution.
   *
   * Measures how much the analyzed token distribution differs from
   * expected human-text distribution (reference). High KL-divergence
   * suggests AI generation.
   *
   * @param token_ids Vector of token IDs to analyze
   * @param reference_token_ids Reference distribution (human text, optional)
   * @return KL-divergence score [0.0, ∞), normalized to [0.0, 1.0]
   */
  virtual float compute_kl_divergence(const std::vector<uint32_t>& token_ids,
                                      const std::vector<uint32_t>& reference_token_ids = {});

  /**
   * @brief Check if token ID belongs to designated "green" list.
   *
   * Green tokens are those designated by Claude's watermarking algorithm.
   * They are selected in a hash-based, deterministic way from vocabulary.
   *
   * @param token_id Token ID to check
   * @return true if token is in green list
   */
  virtual bool is_green_token(uint32_t token_id) const;

  /**
   * @brief Get all heuristic scores for diagnosis.
   *
   * @param token_ids Token sequence to analyze
   * @param text_length_chars Character count
   * @return Map of heuristic name -> score [0.0, 1.0]
   */
  virtual std::unordered_map<std::string, float> get_all_heuristic_scores(
      const std::vector<uint32_t>& token_ids,
      uint64_t text_length_chars);

  /**
   * @brief Set green-list fraction (for A/B testing or calibration).
   *
   * @param fraction Fraction in [0.0, 1.0]
   * @throws std::invalid_argument if fraction not in valid range
   */
  virtual void set_green_list_fraction(float fraction);

  /**
   * @brief Get current green-list fraction.
   *
   * @return Green-list fraction in [0.0, 1.0]
   */
  virtual float get_green_list_fraction() const;

private:
  uint32_t vocab_size_;
  float green_list_fraction_;

  /**
   * @brief Compute Shannon entropy of frequency distribution.
   *
   * @param frequencies Map of value -> frequency
   * @return Entropy in nats (natural logarithm)
   */
  double compute_entropy(const std::unordered_map<uint32_t, uint64_t>& frequencies) const;

  /**
   * @brief Generate green-list bit mask from seed.
   *
   * Uses hash-based deterministic selection (consistent with Claude's approach).
   *
   * @param seed Seed value for hash function
   * @return Bit indicating if token in green list (0 or 1)
   */
  uint32_t compute_green_list_bit(uint32_t token_id) const;
};

} // namespace themisdb::watermark
