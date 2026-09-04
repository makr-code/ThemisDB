/**
 * @file llamacpp_inference_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/llamacpp_inference_engine.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <regex>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {

LLMOutputValidator::LLMOutputValidator(const Config& config)
    : config_(config) {
    spdlog::debug("LLMOutputValidator initialized (min_len: {}, max_len: {}, require_utf8: {})",
                  config_.min_length, config_.max_length, config_.require_utf8);
}

LLMOutputValidator::LLMOutputValidator()
    : LLMOutputValidator(Config{})
{
}

ValidationResult LLMOutputValidator::validate(const std::string& text) {
    ValidationResult result;
    
    // Calculate basic metrics
    result.metrics.char_count = static_cast<int>(text.length());
    result.metrics.word_count = countWords(text);
    result.metrics.sentence_count = countSentences(text);
    result.metrics.avg_word_length = calculateAvgWordLength(text);
    result.metrics.newline_count = static_cast<int>(std::count(text.begin(), text.end(), '\n'));
    
    // Validation 1: Empty check
    if (text.empty()) {
        if (!config_.allow_empty) {
            result.is_valid = false;
            result.errors.push_back("Response is empty");
        }
        return result;
    }
    
    // Validation 2: Length checks
    if (result.metrics.char_count < config_.min_length) {
        result.is_valid = false;
        result.errors.push_back("Response too short (min: " + 
                               std::to_string(config_.min_length) + 
                               " chars, got: " + 
                               std::to_string(result.metrics.char_count) + ")");
    }
    
    if (result.metrics.char_count > config_.max_length) {
        result.warnings.push_back("Response exceeds max length (" + 
                                 std::to_string(config_.max_length) + " chars)");
    }
    
    // Validation 3: UTF-8 validation
    if (config_.require_utf8) {
        result.metrics.is_utf8_valid = isValidUTF8(text);
        if (!result.metrics.is_utf8_valid) {
            result.is_valid = false;
            result.errors.push_back("Invalid UTF-8 encoding detected");
        }
    }
    
    // Validation 4: Truncation detection
    if (config_.check_truncation) {
        result.metrics.is_truncated = detectTruncation(text);
        if (result.metrics.is_truncated) {
            result.warnings.push_back("Response may be truncated (incomplete sentence or sudden stop)");
        }
    }
    
    // Validation 5: Common error patterns
    if (hasCommonErrors(text)) {
        result.is_valid = false;
        result.errors.push_back("Response contains error patterns or placeholder text");
    }
    
    // Validation 6: Invalid control characters
    if (hasInvalidControlChars(text)) {
        result.warnings.push_back("Response contains unusual control characters");
    }
    
    // Validation 7: Repeating patterns
    if (hasRepeatingPatterns(text)) {
        result.warnings.push_back("Response contains unusual repeating patterns");
    }
    
    // Validation 8: Semantic coherence estimation
    if (config_.check_coherence && result.metrics.word_count > 5) {
        result.metrics.semantic_coherence = estimateCoherence(text);
        if (result.metrics.semantic_coherence < config_.min_coherence) {
            result.warnings.push_back("Low semantic coherence score: " + 
                                     std::to_string(result.metrics.semantic_coherence));
        }
    }
    
    return result;
}

ValidationResult LLMOutputValidator::validateWithTokens(
    const std::string& text,
    int token_count,
    int max_tokens
) {
    ValidationResult result = validate(text);
    
    result.metrics.token_count = token_count;
    
    // Check if hit token limit
    if (token_count >= max_tokens) {
        result.metrics.is_truncated = true;
        result.warnings.push_back("Token limit reached (" + 
                                 std::to_string(token_count) + "/" + 
                                 std::to_string(max_tokens) + " tokens)");
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════
// UTF-8 Validation
// ═══════════════════════════════════════════════════════════

bool LLMOutputValidator::isValidUTF8(const std::string& text) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text.c_str());
    size_t len = text.length();
    size_t i = 0;
    
    while (i < len) {
        unsigned char c = bytes[i];
        
        if (c <= 0x7F) {
            // 1-byte character (ASCII)
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte character
            if (i + 1 >= len) {
              return false;
            }
            if ((bytes[i + 1] & 0xC0) != 0x80) {
              return false;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte character
            if (i + 2 >= len) {
              return false;
            }
            if ((bytes[i + 1] & 0xC0) != 0x80) {
              return false;
            }
            if ((bytes[i + 2] & 0xC0) != 0x80) {
              return false;
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte character
            if (i + 3 >= len) {
              return false;
            }
            if ((bytes[i + 1] & 0xC0) != 0x80) {
              return false;
            }
            if ((bytes[i + 2] & 0xC0) != 0x80) {
              return false;
            }
            if ((bytes[i + 3] & 0xC0) != 0x80) {
              return false;
            }
            i += 4;
        } else {
            // Invalid UTF-8 start byte
            return false;
        }
    }
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// Truncation Detection
// ═══════════════════════════════════════════════════════════

bool LLMOutputValidator::detectTruncation(const std::string& text) {
    if (text.empty()) {
      return false;
    }
    
    // Check last characters
    std::string last_chars = text.substr(std::max(0, static_cast<int>(text.length()) - 50));
    
    // Heuristics for truncation:
    // 1. Ends mid-sentence (no period, question mark, exclamation)
    char last_char = text.back();
    bool ends_with_punctuation = (last_char == '.' || last_char == '!' || 
                                  last_char == '?' || last_char == '\n');
    
    // 2. Ends with incomplete word (no space before last word)
    bool ends_mid_word = false;
    if (text.length() > 1) {
        char second_last = text[text.length() - 2];
        ends_mid_word = !std::isspace(second_last) && std::isalpha(last_char);
    }
    
    // 3. Check for common truncation patterns
    bool has_truncation_pattern = (
        last_chars.find("...") != std::string::npos ||
        last_chars.find("[truncated]") != std::string::npos ||
        last_chars.find("(truncated)") != std::string::npos ||
        last_chars.find("Response limit reached") != std::string::npos
    );
    
    return !ends_with_punctuation || ends_mid_word || has_truncation_pattern;
}

// ═══════════════════════════════════════════════════════════
// Semantic Coherence Estimation (Simple Heuristics)
// ═══════════════════════════════════════════════════════════

double LLMOutputValidator::estimateCoherence(const std::string& text) {
    // Six surface-level heuristics for lightweight coherence estimation.
    // No external model is required.  The score is in [0, 1] where 1.0
    // means "likely coherent" and values approaching 0.0 indicate
    // strong evidence of incoherence (repetition, nonsense, extreme statistics).

    double score = 1.0;

    if (text.empty()) {
      return 0.0;
    }

    int word_count = countWords(text);
    if (word_count == 0) {
      return 0.0;
    }

    // Heuristic 1: Average word length (too short or too long is suspicious)
    double avg_word_len = calculateAvgWordLength(text);
    if (avg_word_len < 2.0 || avg_word_len > 15.0) {
        score *= 0.7;
    }

    // Heuristic 2: Sentence structure (ratio of words to sentences)
    int sentence_count = countSentences(text);
    if (sentence_count > 0) {
        double words_per_sentence = static_cast<double>(word_count) / sentence_count;
        if (words_per_sentence < 2.0 || words_per_sentence > 50.0) {
            score *= 0.8;
        }
    } else {
        // No sentences at all - very suspicious
        score *= 0.5;
    }

    // Heuristic 3: Character diversity (low diversity suggests repetition)
    // Note: This counts bytes, not UTF-8 characters, but is still useful for detecting
    // repetition patterns in both ASCII and UTF-8 text
    std::unordered_set<char> unique_chars(text.begin(), text.end());
    double char_diversity = static_cast<double>(unique_chars.size()) /
                           std::max(static_cast<size_t>(1), text.length());
    if (char_diversity < 0.05) {
        score *= 0.6;
    }

    // Heuristic 4: Word diversity (rough estimate)
    // Count approximate unique words (case-insensitive)
    // Limit to first 1000 words for performance on large texts
    std::unordered_set<std::string> words;
    std::istringstream iss(text);
    std::string word = {};
    int words_checked = 0;
    const int MAX_WORDS_TO_CHECK = 1000;

    while (iss >> word && words_checked < MAX_WORDS_TO_CHECK) {
        // Simple lowercase conversion (in-place for efficiency)
        for (char& c : word) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        words.insert(std::move(word));
        words_checked++;
    }

    if (word_count > 0) {
        // Calculate diversity based on checked words
        int effective_word_count = std::min(word_count, MAX_WORDS_TO_CHECK);
        double word_diversity = static_cast<double>(words.size()) / effective_word_count;
        if (word_diversity < 0.3) {
            score *= 0.7;  // Low word diversity
        }
    }

    // Heuristic 5: Consecutive sentence repetition.
    // Texts where two or more consecutive sentences are identical (after
    // normalisation) are a strong hallucination signal.
    {
        // Split text into sentences on '.', '!', '?'
        std::vector<std::string> sentences;
        std::string current = {};
        for (char c : text) {
            if (c == '.' || c == '!' || c == '?') {
                // Trim whitespace
                size_t a = current.find_first_not_of(" \t\r\n");
                if (a != std::string::npos) {
                    size_t b = current.find_last_not_of(" \t\r\n");
                    sentences.push_back(current.substr(a, b - a + 1));
                }
                current.clear();
            } else {
                current += c;
            }
        }
        int consec_dup = 0;
        for (size_t i = 1; i < sentences.size(); ++i) {
            if (sentences[i] == sentences[i - 1]) {
                ++consec_dup;
            }
        }
        if (!sentences.empty()) {
            double dup_ratio = static_cast<double>(consec_dup) / sentences.size();
            if (dup_ratio > 0.25) {
                score *= 0.5;  // >25% consecutive duplicate sentences
            } else if (dup_ratio > 0.10) {
                score *= 0.75;
            }
        }
    }

    // Heuristic 6: Bigram repetition ratio.
    // A high fraction of repeated bigrams indicates the model has entered a
    // repetitive loop, a common failure mode.
    {
        std::vector<std::string> tokens;
        {
            std::istringstream ts(text);
            std::string tok = {};
            while (ts >> tok) {
                for (char& c : tok) {
                  c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                tokens.push_back(std::move(tok));
            }
        }
        if (static_cast<int>(tokens.size()) > = 4) {
            std::unordered_map<std::string, int> bigram_count;
            int repeated = 0;
            for (size_t i = 0; i + 1 < tokens.size(); ++i) {
                // Increment count and check: if the bigram has already been seen
                // (new count > 1) this occurrence is a repetition.
                if (++bigram_count[tokens[i] + " " + tokens[i + 1]] > 1) {
                    ++repeated;
                }
            }
            double bigram_repeat_ratio = static_cast<double>(repeated) /
                                         static_cast<double>(tokens.size() - 1);
            if (bigram_repeat_ratio > 0.40) {
                score *= 0.5;  // >40% of bigrams are repeated
            } else if (bigram_repeat_ratio > 0.20) {
                score *= 0.75;
            }
        }
    }

    return std::max(0.0, std::min(1.0, score));
}

// ═══════════════════════════════════════════════════════════
// Error Pattern Detection
// ═══════════════════════════════════════════════════════════

bool LLMOutputValidator::hasCommonErrors(const std::string& text) {
    // Convert to lowercase for case-insensitive matching
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    // Common error patterns
    static const std::vector<std::string> error_patterns = {
        "error:",
        "exception:",
        "failed to",
        "could not",
        "unable to",
        "stub_response",
        "placeholder",
        "todo:",
        "fixme:",
        "not implemented",
        "[error]",
        "[warning]",
        "traceback",
        "stack trace"
    };
    
    for (const auto& pattern : error_patterns) {
        if (lower_text.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════
// Repeating Pattern Detection
// ═══════════════════════════════════════════════════════════

bool LLMOutputValidator::hasRepeatingPatterns(const std::string& text) {
    if (text.length() < 20) {
      return false;
    }
    
    // Check for exact repeated sequences of varying lengths
    for (size_t pattern_len = 5; pattern_len <= std::min(text.length() / 4, size_t(50)); ++pattern_len) {
        for (size_t i = 0; i + pattern_len * 2 <= text.length(); ++i) {
            std::string pattern = text.substr(i, pattern_len);
            std::string next = text.substr(i + pattern_len, pattern_len);
            
            if (pattern == next) {
                // Found immediate repetition
                // Check if it repeats more than twice
                size_t count = 2;
                size_t pos = i + pattern_len * 2;
                while (pos + pattern_len <= text.length()) {
                    if (text.substr(pos, pattern_len) == pattern) {
                        count++;
                        pos += pattern_len;
                    } else {
                        break;
                    }
                }
                
                if (count >= 3) {
                    spdlog::debug("Detected repeating pattern (len={}, count={}): {}",
                                 pattern_len, count, pattern.substr(0, 20));
                    return true;
                }
            }
        }
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════
// Invalid Control Characters
// ═══════════════════════════════════════════════════════════

bool LLMOutputValidator::hasInvalidControlChars(const std::string& text) {
    for (unsigned char c : text) {
        // Allow: tab (9), newline (10), carriage return (13), and printable chars (32-126)
        // Allow: extended ASCII (128-255) for UTF-8
        if (c < 32 && c != 9 && c != 10 && c != 13) {
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════
// Metrics Calculation Helpers
// ═══════════════════════════════════════════════════════════

int LLMOutputValidator::countWords(const std::string& text) {
    if (text.empty()) {
      return 0;
    }
    
    int count = 0;
    bool in_word = false;
    
    for (char c : text) {
        if (std::isspace(c)) {
            if (in_word) {
                count++;
                in_word = false;
            }
        } else {
            in_word = true;
        }
    }
    
    // Count last word if text doesn't end with whitespace
    if (in_word) {
      count++;
    }
    
    return count;
}

int LLMOutputValidator::countSentences(const std::string& text) {
    int count = 0;
    
    for (char c : text) {
        if (c == '.' || c == '!' || c == '?') {
            count++;
        }
    }
    
    // If no sentence-ending punctuation but has words, count as 1 sentence
    if (count == 0 && countWords(text) > 0) {
        count = 1;
    }
    
    return count;
}

double LLMOutputValidator::calculateAvgWordLength(const std::string& text) {
    if (text.empty()) {
      return 0.0;
    }
    
    int word_count = 0;
    int total_chars = 0;
    int current_word_len = 0;
    
    for (char c : text) {
        if (std::isspace(c)) {
            if (current_word_len > 0) {
                word_count++;
                total_chars += current_word_len;
                current_word_len = 0;
            }
        } else {
            current_word_len++;
        }
    }
    
    // Count last word
    if (current_word_len > 0) {
        word_count++;
        total_chars += current_word_len;
    }
    
    return (word_count > 0) ? static_cast<double>(total_chars) / word_count : 0.0;
}

} // namespace llm
} // namespace themis

