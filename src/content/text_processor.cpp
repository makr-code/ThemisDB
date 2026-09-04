/**
 * @file text_processor.cpp
 * @brief Text content processor with tokenization, NLP, and linguistic analysis.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 84/100
 * @note Gap Summary: total=7; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=0, H=2, M=5, L=0
 * @note Status: Production Ready; Tokenization + NLP working; advanced linguistic analysis deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <regex>
#include <sstream>

#include "content/content_processor.h"
#include "content/language_detector.h"

namespace themis {
namespace content {

// ============================================================================
// TextProcessor Implementation
// ============================================================================

ExtractionResult TextProcessor::extract(const std::string &blob, const ContentType &content_type) {
    ExtractionResult result;
    result.ok = true;

    // Normalize text (UTF-8 handling, whitespace cleanup)
    result.text = normalizeText(blob);

    // Extract metadata based on content type
    result.metadata                          = json::object();
    result.metadata["original_size_bytes"]   = blob.size();
    result.metadata["normalized_size_bytes"] = result.text.size();
    result.metadata["mime_type"]             = content_type.mime_type;

    // For code files, detect language
    if (content_type.mime_type.find("text/x-") == 0 || content_type.mime_type == "application/javascript"
        || content_type.mime_type == "text/x-python" || content_type.mime_type == "text/x-c++src") {
        // Extract language from MIME type
        std::string lang = content_type.mime_type;
        if (auto pos = lang.find("text/x-"); pos != std::string::npos) {
            lang = lang.substr(pos + 7);
        } else if (lang == "application/javascript") {
            lang = "javascript";
        }

        result.metadata["language"] = lang;
        result.metadata["is_code"]  = true;

        // Simple line count
        auto line_count = static_cast<int64_t>(std::count(result.text.begin(), result.text.end(), '\n')) + 1;
        result.metadata["line_count"] = line_count;
    } else {
        result.metadata["is_code"] = false;
    }

    // Word and token count
    int token_count                = countTokens(result.text);
    result.metadata["token_count"] = token_count;

    // Sentence count (approximate)
    auto sentences                    = splitIntoSentences(result.text);
    result.metadata["sentence_count"] = sentences.size();

    // Multi-language detection and routing
    {
        LanguageDetector lang_detector;
        DetectedLanguage lang                    = lang_detector.detect(result.text);
        result.metadata["detected_language"]     = lang.code;
        result.metadata["language_name"]         = lang.name;
        result.metadata["language_confidence"]   = lang.confidence;
        result.metadata["language_routing_hint"] = LanguageDetector::routingHint(lang.code);
    }

    return result;
}

std::vector<json> TextProcessor::chunk(const ExtractionResult &extraction_result, int chunk_size, int overlap) {
    std::vector<json> chunks;

    const std::string &text = extraction_result.text;
    if (text.empty()) {
        return chunks;
    }

    // Split into sentences for better chunk boundaries
    auto sentences = splitIntoSentences(text);

    if (sentences.empty()) {
        // Fallback: create single chunk
        json chunk = {{"text", text},
                      {"seq_num", 0},
                      {"start_offset", 0},
                      {"end_offset", static_cast<int>(text.size())},
                      {"token_count", countTokens(text)}};
        chunks.push_back(chunk);
        return chunks;
    }

    // Group sentences into chunks of approximately chunk_size tokens
    std::vector<std::string> sentence_list = sentences;
    int seq_num                            = 0;
    size_t current_pos                     = 0;

    while (static_cast<size_t>(current_pos) < sentence_list.size()) {
        std::string chunk_text = {};
        int chunk_tokens       = 0;
        size_t chunk_start_idx = current_pos;
        size_t chunk_end_idx   = current_pos;

        // Add sentences until we reach chunk_size
        while (static_cast<size_t>(chunk_end_idx) < sentence_list.size()) {
            const std::string &sentence = sentence_list[chunk_end_idx];
            int sentence_tokens         = countTokens(sentence);

            if (chunk_tokens + sentence_tokens > chunk_size && chunk_tokens > 0) {
                // Would exceed chunk_size, stop here
                break;
            }

            chunk_text += sentence;
            if (chunk_end_idx < static_cast<int>(sentence_list.size()) - 1) {
                chunk_text += " "; // Add space between sentences
            }
            chunk_tokens += sentence_tokens;
            chunk_end_idx++;
        }

        if (chunk_text.empty()) {
            // Single sentence is larger than chunk_size, include it anyway
            chunk_text    = sentence_list[current_pos];
            chunk_tokens  = countTokens(chunk_text);
            chunk_end_idx = current_pos + 1;
        }

        // Find actual offsets in original text
        size_t start_offset = 0;
        for (size_t i = 0; i < chunk_start_idx; i++) {
            start_offset += sentence_list[i].size() + 1; // +1 for space
        }

        size_t end_offset = start_offset + chunk_text.size();

        json chunk = {{"text", chunk_text},
                      {"seq_num", seq_num},
                      {"start_offset", static_cast<int>(start_offset)},
                      {"end_offset", static_cast<int>(end_offset)},
                      {"token_count", chunk_tokens}};
        chunks.push_back(chunk);

        seq_num++;

        // Move to next chunk with overlap
        if (overlap > 0  && static_cast<size_t>(chunk_end_idx) < sentence_list.size()) {
            // Calculate how many sentences to overlap
            int overlap_sentences      = 0;
            int overlap_tokens_counted = 0;

            for (size_t i = chunk_end_idx; i > chunk_start_idx && overlap_tokens_counted < overlap; i--) {
                overlap_tokens_counted += countTokens(sentence_list[static_cast<int>(i - 1)]);
                overlap_sentences++;
            }

            current_pos = chunk_end_idx - overlap_sentences;
            if (current_pos <= chunk_start_idx) {
                current_pos = chunk_end_idx; // Avoid infinite loop
            }
        } else {
            current_pos = chunk_end_idx;
        }
    }

    return chunks;
}

std::vector<float> TextProcessor::generateEmbedding(const std::string &chunk_data) {
    // If a real embedding backend has been injected, delegate to it.
    if (embedding_fn_) {
        return embedding_fn_(chunk_data);
    }

    // PERMANENT FALLBACK NOTE:
    // Purpose: Provide a deterministic, zero-dependency embedding for unit
    //   tests and environments where no real embedding service is available.
    // Activation: Active when no EmbeddingFn is injected via
    //   setEmbeddingBackend().  Inject a real backend for semantically
    //   meaningful vectors:
    //     text_proc.setEmbeddingBackend([&model](const std::string& text) {
    //         return model.embed(text);   // e.g. ONNX all-mpnet-base-v2
    //     });
    // Behaviour: Uses a hash-based deterministic formula — output vectors are
    //   NOT semantically meaningful for similarity search.  Safe for tests.

    const int EMBEDDING_DIM = 768; // Standard for all-mpnet-base-v2
    std::vector<float> embedding(EMBEDDING_DIM, 0.0f);

    if (chunk_data.empty()) {
        return embedding; // Zero vector for empty input
    }

    // Simple deterministic hash-based embedding for testing
    // Each token influences multiple dimensions
    std::hash<std::string> hasher;

    // Split text into tokens
    std::istringstream iss(chunk_data);
    std::vector<std::string> tokens;
    std::string token = {};
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return embedding;
    }

    // Generate embedding components with better distribution
    for (size_t i = 0; i < tokens.size(); i++) {
        size_t token_hash = hasher(tokens[i]);

        // Use different hash seeds for better differentiation
        for (int seed = 0; seed < 3; seed++) {
            size_t combined_hash = token_hash ^ (i * 31) ^ (seed * 97);

            // Distribute token influence across dimensions
            for (int dim_offset = 0; dim_offset < 10; dim_offset++) {
                int dim = (combined_hash + dim_offset * 73) % EMBEDDING_DIM;

                // Add influence with varying weights based on position
                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);
                float phase  = static_cast<float>((combined_hash + dim) % 360) * 3.14159f / 180.0f;
                embedding[dim] += std::sin(phase) * weight;
            }
        }
    }

    // Normalize to unit vector (L2 normalization for cosine similarity)
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);

    if (norm > 1e-6f) {
        for (float &val : embedding) {
            val /= norm;
        }
    }

    return embedding;
}

// ============================================================================
// TextProcessor::setEmbeddingBackend (stub #6 injection API)
// ============================================================================

void TextProcessor::setEmbeddingBackend(EmbeddingFn fn) {
    embedding_fn_ = std::move(fn);
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string TextProcessor::normalizeText(const std::string &text) {
    std::string normalized = text;

    // Remove carriage returns (Windows line endings)
    normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'), normalized.end());

    // Normalize multiple spaces to single space
    std::regex multi_space("  +");
    normalized = std::regex_replace(normalized, multi_space, " ");

    // Trim leading/trailing whitespace
    auto start = normalized.find_first_not_of(" \t\n");
    auto end   = normalized.find_last_not_of(" \t\n");

    if (start == std::string::npos) {
        return ""; // All whitespace
    }

    return normalized.substr(start, end - start + 1);
}

int TextProcessor::countTokens(const std::string &text) {
    // Simple whitespace-based tokenizer
    // In production, use a proper tokenizer (e.g., tiktoken for GPT, WordPiece for BERT)

    if (text.empty()) {
        return 0;
    }

    std::istringstream iss(text);
    std::string token = {};
    int count = 0;

    while (iss >> token) {
        count++;
    }

    return count;
}

std::vector<std::string> TextProcessor::splitIntoSentences(const std::string &text) {
    std::vector<std::string> sentences;

    // Simple sentence splitter based on punctuation
    // In production, use a proper sentence tokenizer (e.g., spaCy, NLTK)

    std::regex sentence_regex(R"([^.!?]+[.!?]+)");
    auto sentences_begin = std::sregex_iterator(text.begin(), text.end(), sentence_regex);
    auto sentences_end   = std::sregex_iterator();

    for (auto it = sentences_begin; it != sentences_end; ++it) {
        std::string sentence = it->str();

        // Trim whitespace
        auto start = sentence.find_first_not_of(" \t\n");
        auto end   = sentence.find_last_not_of(" \t\n");

        if (start != std::string::npos) {
            sentence = sentence.substr(start, end - start + 1);
            if (!sentence.empty()) {
                sentences.push_back(sentence);
            }
        }
    }

    // If no sentences found with punctuation, treat entire text as one sentence
    if (sentences.empty() && !text.empty()) {
        sentences.push_back(text);
    }

    return sentences;
}

// ---------------------------------------------------------------------------
// computeMinHash — 128-permutation MinHash over 3-word shingles
// ---------------------------------------------------------------------------

/*static*/ std::vector<uint32_t> TextProcessor::computeMinHash(const std::string &text, size_t num_hashes) {
    std::vector<uint32_t> signature(num_hashes, UINT32_MAX);
    if (text.empty() || num_hashes == 0) {
        return signature;
    }

    // Tokenise into words (lowercase)
    std::vector<std::string> words;
    {
        std::istringstream iss(text);
        std::string w = {};
        while (iss >> w) {
            std::transform(w.begin(), w.end(), w.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            words.push_back(std::move(w));
        }
    }
    if (words.empty()) {
        return signature;
    }

    // Build 3-word shingles (fall back to unigrams for very short texts)
    std::vector<std::string> shingles = {};

    if (words.size() < 3) {
        shingles = words;
    } else {
        shingles.reserve(static_cast<int>(words.size()) - 2);
        for (size_t i = 0; i + 3 <= words.size(); ++i) {
            shingles.push_back(words[i] + " " + words[i + 1] + " " + words[i + 2]);
        }
    }

    // For each shingle, update each MinHash slot.
    // Hash function h_k(s) = FNV-1a(s) XOR-mixed with seed k.
    static constexpr uint32_t kFnvPrime  = 16777619u;
    static constexpr uint32_t kFnvOffset = 2166136261u;
    static constexpr uint32_t kSeedMix   = 2246822519u;

    for (const auto &shingle : shingles) {
        for (size_t k = 0; k < num_hashes; ++k) {
            // FNV-1a over the shingle bytes, seeded by k
            uint32_t h = kFnvOffset ^ (static_cast<uint32_t>(k) * 2654435761u);
            for (unsigned char c : shingle) {
                h ^= c;
                h *= kFnvPrime;
            }
            h ^= static_cast<uint32_t>(k) * kSeedMix;
            if (h < signature[k]) {
                signature[k] = h;
            }
        }
    }

    return signature;
}

} // namespace content
} // namespace themis
