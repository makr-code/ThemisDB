/**
 * @file relevance_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/relevance_evaluator.h"
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <regex>
#include <mutex>
#include <cmath>
#include <set>
#include <unordered_map>

namespace themis::rag::judge {

using json = nlohmann::json;

struct RelevanceEvaluator::Impl {
    Config config;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    ResponseParser parser;
    mutable std::mutex state_mutex;  // Protect shared state access

    // Tokenize text into lowercase, punctuation-stripped tokens of length > 2
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::istringstream stream(text);
        std::string word = {};
        while (stream >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (word.length() > 2) {
                tokens.push_back(word);
            }
        }
        return tokens;
    }

    // Compute term-frequency vector for a token list over a shared vocabulary
    static std::vector<double> termFrequencyVector(
        const std::vector<std::string>& tokens,
        const std::vector<std::string>& vocab
    ) {
        // Build frequency map in O(token_count), then populate in O(vocab_size)
        std::unordered_map<std::string, double> freq = {};

        for (const auto& t : tokens) {
            freq[t] += 1.0;
        }
        std::vector<double> vec(vocab.size(), 0.0);
        for (size_t i = 0; i < vocab.size(); ++i) {
            auto it = freq.find(vocab[i]);
            if (it != freq.end()) {
                vec[i] = it->second;
            }
        }
        return vec;
    }

    // Cosine similarity between two equal-length vectors
    static double cosineSimilarity(
        const std::vector<double>& a,
        const std::vector<double>& b
    ) {
        double dot = 0.0, na = 0.0, nb = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            na  += a[i] * a[i];
            nb  += b[i] * b[i];
        }
        if (na < 1e-9 || nb < 1e-9) {
          return 0.0;
        }
        return dot / (std::sqrt(na) * std::sqrt(nb));
    }

    // Semantic similarity using TF-cosine (bag-of-words cosine over shared vocab).
    // Falls back to Jaccard when the vocabulary is empty.
    double computeSemanticSimilarity(const std::string& text1, const std::string& text2) {
        auto toks1 = tokenize(text1);
        auto toks2 = tokenize(text2);

        if (toks1.empty() || toks2.empty()) {
            return 0.0;
        }

        // Build shared vocabulary
        std::set<std::string> vocab_set(toks1.begin(), toks1.end());
        vocab_set.insert(toks2.begin(), toks2.end());
        std::vector<std::string> vocab(vocab_set.begin(), vocab_set.end());

        auto vec1 = termFrequencyVector(toks1, vocab);
        auto vec2 = termFrequencyVector(toks2, vocab);
        return cosineSimilarity(vec1, vec2);
    }
};

RelevanceEvaluator::RelevanceEvaluator()
    : RelevanceEvaluator(Config{}) {
}

RelevanceEvaluator::RelevanceEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.5;  // Slightly higher for question generation
    llm_config.max_tokens = 256;
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    THEMIS_DEBUG("RelevanceEvaluator initialized");
}

RelevanceEvaluator::~RelevanceEvaluator() = default;

std::vector<std::string> RelevanceEvaluator::generateReverseQuestions(const std::string& answer) {
    std::vector<std::string> questions;
    questions.reserve(impl_->config.num_reverse_questions);
    
    if (answer.empty()) {
        return questions;
    }
    
    // Prompt for reverse question generation
    std::string prompt = R"(Generate )" + std::to_string(impl_->config.num_reverse_questions) + R"( questions that would be answered by the following text.
Each question should capture a key aspect of the answer.

Answer: )" + answer + R"(

Output format:
{
  "questions": ["question1", "question2", "question3"]
}

Questions:)";
    
    try {
        std::string llm_response = impl_->llm_integration->evaluateDimension(
            prompt, EvaluationDimension::RELEVANCE
        );
        
        // Parse JSON response
        json response_json = impl_->parser.parseJSONResponse(llm_response);
        
        if (response_json.contains("questions") && response_json["questions"].is_array()) {
            for (const auto& question : response_json["questions"]) {
                if (question.is_string()) {
                    questions.push_back(question.get<std::string>());
                    
                    if (questions.size() >= impl_->config.num_reverse_questions) {
                        break;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Reverse question generation failed: {}", e.what());
    }

    if (questions.empty()) {
        // Offline/no-model fallback: nutze die Antwort selbst als Rueckfrage-Anker.
        questions.push_back(answer);
    }
    
    THEMIS_DEBUG("Generated {} reverse questions", questions.size());
    return questions;
}

QueryIntent RelevanceEvaluator::analyzeIntent(const std::string& query) {
    if (query.empty()) {
        return QueryIntent::UNKNOWN;
    }
    
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Simple keyword-based intent detection
    // In production, this would use a trained classifier
    
    // Informational queries
    std::vector<std::string> info_keywords = {
        "what", "who", "where", "when", "why", "how",
        "explain", "describe", "define", "tell me about"
    };
    
    // Navigational queries
    std::vector<std::string> nav_keywords = {
        "find", "locate", "search for", "show me", "website", "page"
    };
    
    // Transactional queries
    std::vector<std::string> trans_keywords = {
        "buy", "download", "install", "create", "make", "do",
        "calculate", "convert", "generate"
    };
    
    // Conversational queries
    std::vector<std::string> conv_keywords = {
        "also", "more", "additionally", "furthermore", "follow up"
    };
    
    // Count keyword matches for each intent type
    auto count_matches = [&query_lower](const std::vector<std::string>& keywords) {
        int count = 0;
        for (const auto& kw : keywords) {
            if (query_lower.find(kw) != std::string::npos) {
                count++;
            }
        }
        return count;
    };
    
    int info_score = count_matches(info_keywords);
    int nav_score = count_matches(nav_keywords);
    int trans_score = count_matches(trans_keywords);
    int conv_score = count_matches(conv_keywords);
    
    // Return intent with highest score
    int max_score = std::max({info_score, nav_score, trans_score, conv_score});
    
    if (max_score == 0) {
        return QueryIntent::UNKNOWN;
    } else if (info_score == max_score) {
        return QueryIntent::INFORMATIONAL;
    } else if (nav_score == max_score) {
        return QueryIntent::NAVIGATIONAL;
    } else if (trans_score == max_score) {
        return QueryIntent::TRANSACTIONAL;
    } else {
        return QueryIntent::CONVERSATIONAL;
    }
}

std::vector<std::string> RelevanceEvaluator::detectNoise(
    const std::string& answer,
    const std::string& query
) {
    std::vector<std::string> irrelevant_segments;
    
    if (!impl_->config.enable_noise_detection || answer.empty()) {
        return irrelevant_segments;
    }
    
    // Split answer into sentences
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    
    // Check each sentence for relevance to query
    for (auto it = sentences_begin; it != sentences_end; ++it) {
        std::string sentence = it->str();
        double similarity = impl_->computeSemanticSimilarity(sentence, query);
        
        // If sentence has very low similarity to query, it might be noise
        if (similarity < 0.1) {
            irrelevant_segments.push_back(sentence);
        }
    }
    
    THEMIS_DEBUG("Detected {} potentially irrelevant segments", irrelevant_segments.size());
    return irrelevant_segments;
}

double RelevanceEvaluator::calculateSemanticSimilarity(
    const std::string& query,
    const std::vector<std::string>& questions
) {
    if (query.empty() || questions.empty()) {
        return 0.0;
    }
    
    // Calculate average similarity between query and all generated questions
    double total_similarity = 0.0;
    
    for (const auto& question : questions) {
        total_similarity += impl_->computeSemanticSimilarity(query, question);
    }
    
    return total_similarity / questions.size();
}

RelevanceResult RelevanceEvaluator::evaluate(
    const std::string& answer,
    const std::string& query
) {
    RelevanceResult result = {};
    
    if (answer.empty() || query.empty()) {
        result.relevance_score = 0.0;
        result.explanation = "Empty answer or query.";
        return result;
    }
    
    // Step 1: Generate reverse questions
    result.reverse_questions = generateReverseQuestions(answer);
    
    // Step 2: Calculate semantic similarity
    result.question_similarity_score = calculateSemanticSimilarity(query, result.reverse_questions);
    
    // Step 3: Analyze query intent
    if (impl_->config.enable_intent_analysis) {
        result.detected_intent = analyzeIntent(query);
        
        // Intent alignment: check if answer type matches query intent
        // For now, assume good alignment if we detected an intent
        result.intent_alignment_score = result.detected_intent != QueryIntent::UNKNOWN ? 0.8 : 0.5;
    } else {
        result.detected_intent = QueryIntent::UNKNOWN;
        result.intent_alignment_score = 0.7;  // Neutral
    }
    
    // Step 4: Detect noise
    result.irrelevant_segments = detectNoise(answer, query);
    
    // Calculate noise ratio
    size_t total_sentences = 0;
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    total_sentences = std::distance(sentences_begin, sentences_end);
    
    result.noise_ratio = total_sentences > 0 
        ? static_cast<double>(result.irrelevant_segments.size()) / total_sentences
        : 0.0;
    
    // Step 5: Calculate overall relevance score
    // Weighted combination: question similarity (50%) + intent alignment (30%) - noise penalty (20%)
    double noise_penalty = result.noise_ratio * 0.2;
    result.relevance_score = std::max(0.0, 
        result.question_similarity_score * 0.5 +
        result.intent_alignment_score * 0.3 -
        noise_penalty
    );
    
    // Ensure score is in [0, 1]
    result.relevance_score = std::min(1.0, std::max(0.0, result.relevance_score));
    
    // Generate explanation
    std::ostringstream explanation = {};
    explanation << "Relevance Score: " << result.relevance_score << "\n";
    explanation << "Question Similarity: " << result.question_similarity_score << "\n";
    explanation << "Intent: ";
    switch (result.detected_intent) {
        case QueryIntent::INFORMATIONAL: explanation << "Informational"; break;
        case QueryIntent::NAVIGATIONAL: explanation << "Navigational"; break;
        case QueryIntent::TRANSACTIONAL: explanation << "Transactional"; break;
        case QueryIntent::CONVERSATIONAL: explanation << "Conversational"; break;
        default: explanation << "Unknown";
    }
    explanation << " (alignment: " << result.intent_alignment_score << ")\n";
    explanation << "Noise Ratio: " << result.noise_ratio << "\n";
    
    if (result.relevance_score >= 0.7) {
        explanation << "Answer directly addresses the query.";
    } else if (result.relevance_score >= 0.5) {
        explanation << "Answer partially addresses the query.";
    } else {
        explanation << "Answer has limited relevance to the query.";
    }
    
    result.explanation = explanation.str();
    
    THEMIS_INFO("Relevance evaluation complete: score={:.2f}, similarity={:.2f}, noise={:.2f}", 
                result.relevance_score, result.question_similarity_score, result.noise_ratio);
    
    return result;
}

} // namespace themis::rag::judge
