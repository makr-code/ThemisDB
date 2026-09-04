/**
 * @file coherence_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/coherence_evaluator.h"
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

namespace themis::rag::judge {

using json = nlohmann::json;

struct CoherenceEvaluator::Impl {
    Config config;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    ResponseParser parser;
    mutable std::mutex state_mutex;  // Protect shared state access
    
    // Calculate readability score (Flesch-like metric)
    double calculateReadability(const std::string& text) {
        if (text.empty()) {
          return 0.0;
        }
        
        // Count sentences
        std::regex sentence_regex(R"([^.!?]+[.!?])");
        auto sentences_begin = std::sregex_iterator(text.begin(), text.end(), sentence_regex);
        auto sentences_end = std::sregex_iterator();
        size_t sentence_count = std::distance(sentences_begin, sentences_end);
        
        if (sentence_count == 0) {
          return 0.5;
        }
        
        // Count words
        std::istringstream stream(text);
        std::string word = {};
        size_t word_count = 0;
        size_t syllable_count = 0;
        
        while (stream >> word) {
            word_count++;
            // Rough syllable estimation
            syllable_count += std::max(1, static_cast<int>(word.length() / 3));
        }
        
        if (word_count == 0) {
          return 0.5;
        }
        
        // Average sentence length
        double avg_sentence_length = static_cast<double>(word_count) / sentence_count;
        
        // Average syllables per word
        double avg_syllables = static_cast<double>(syllable_count) / word_count;
        
        // Simplified Flesch Reading Ease (normalized to 0-1)
        // Optimal: 10-20 words per sentence, 1.5-2 syllables per word
        double sentence_penalty = std::abs(avg_sentence_length - 15.0) / 15.0;
        double syllable_penalty = std::abs(avg_syllables - 1.75) / 1.75;
        
        double readability = 1.0 - std::min(1.0, (sentence_penalty + syllable_penalty) / 2.0);
        return std::max(0.0, readability);
    }
    
    // Check for transition words
    size_t countTransitionWords(const std::string& text) {
        std::vector<std::string> transitions = {
            "however", "moreover", "furthermore", "therefore", "thus",
            "consequently", "additionally", "meanwhile", "nevertheless",
            "in contrast", "on the other hand", "similarly", "likewise",
            "for example", "for instance", "in conclusion", "finally"
        };
        
        std::string text_lower = text;
        std::transform(text_lower.begin(), text_lower.end(), text_lower.begin(), ::tolower);
        
        size_t count = 0;
        for (const auto& transition : transitions) {
            size_t pos = 0;
            while ((pos = text_lower.find(transition, pos)) != std::string::npos) {
                count++;
                pos += transition.length();
            }
        }
        
        return count;
    }
};

CoherenceEvaluator::CoherenceEvaluator()
    : CoherenceEvaluator(Config{}) {
}

CoherenceEvaluator::CoherenceEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.3;
    llm_config.max_tokens = 512;
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    THEMIS_DEBUG("CoherenceEvaluator initialized");
}

CoherenceEvaluator::~CoherenceEvaluator() = default;

double CoherenceEvaluator::analyzeLogicalFlow(const std::string& answer) {
    if (answer.empty()) {
        return 0.0;
    }
    
    // Split into sentences
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    size_t sentence_count = std::distance(sentences_begin, sentences_end);
    
    if (sentence_count == 0) {
        return 0.0;
    }
    
    // Count transition words (indicates good flow)
    size_t transition_count = impl_->countTransitionWords(answer);
    
    // Check for logical connectors
    std::regex connector_regex(R"(\b(because|since|as|so|therefore|thus|hence)\b)", 
                               std::regex::icase);
    auto connectors_begin = std::sregex_iterator(answer.begin(), answer.end(), connector_regex);
    auto connectors_end = std::sregex_iterator();
    size_t connector_count = std::distance(connectors_begin, connectors_end);
    
    // Calculate flow score
    // Good flow: ~1 transition per 3-4 sentences, ~1 connector per 2-3 sentences
    double transition_ratio = static_cast<double>(transition_count) / sentence_count;
    double connector_ratio = static_cast<double>(connector_count) / sentence_count;
    
    double transition_score = std::min(1.0, transition_ratio * 3.0);  // Optimal: 0.33 transitions per sentence
    double connector_score = std::min(1.0, connector_ratio * 2.5);    // Optimal: 0.4 connectors per sentence
    
    double flow_score = (transition_score + connector_score) / 2.0;
    
    // Bonus for having multiple paragraphs (indicates structure)
    size_t paragraph_count = std::count(answer.begin(), answer.end(), '\n') + 1;
    if (paragraph_count > 1 && sentence_count > 5) {
        flow_score = std::min(1.0, flow_score * 1.1);
    }
    
    THEMIS_DEBUG("Logical flow score: {:.2f} (transitions={}, connectors={}, sentences={})",
                 flow_score, transition_count, connector_count, sentence_count);
    
    return flow_score;
}

double CoherenceEvaluator::assessStructure(const std::string& answer) {
    if (answer.empty()) {
        return 0.0;
    }
    
    // Check for structural elements
    size_t paragraph_count = std::count(answer.begin(), answer.end(), '\n') + 1;
    
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    size_t sentence_count = std::distance(sentences_begin, sentences_end);
    
    // Check for enumeration/listing (indicates organization)
    std::regex list_regex(R"(\b(first|second|third|finally|lastly|\d+\.))", std::regex::icase);
    auto list_begin = std::sregex_iterator(answer.begin(), answer.end(), list_regex);
    auto list_end = std::sregex_iterator();
    size_t list_marker_count = std::distance(list_begin, list_end);
    
    // Structure score components
    double structure_score = 0.0;
    
    // Paragraph organization (0-0.4)
    if (paragraph_count == 1) {
        structure_score += 0.2;  // Single paragraph is okay for short answers
    } else if (paragraph_count >= 2 && paragraph_count <= 5) {
        structure_score += 0.4;  // Multiple paragraphs = good organization
    } else if (paragraph_count > 5) {
        structure_score += 0.3;  // Too many might indicate fragmentation
    }
    
    // Sentence distribution (0-0.3)
    if (sentence_count >= 3 && sentence_count <= 20) {
        structure_score += 0.3;  // Good range
    } else if (sentence_count > 0) {
        structure_score += 0.15;  // Too short or too long
    }
    
    // Organization markers (0-0.3)
    if (list_marker_count > 0) {
        structure_score += std::min(0.3, list_marker_count * 0.1);
    }
    
    structure_score = std::min(1.0, structure_score);
    
    THEMIS_DEBUG("Structure score: {:.2f} (paragraphs={}, sentences={}, list_markers={})",
                 structure_score, paragraph_count, sentence_count, list_marker_count);
    
    return structure_score;
}

double CoherenceEvaluator::evaluateLinguisticQuality(const std::string& answer) {
    if (answer.empty()) {
        return 0.0;
    }
    
    // Calculate readability
    double readability = impl_->calculateReadability(answer);
    
    // Check for common grammar issues (simplified)
    std::string answer_lower = answer;
    std::transform(answer_lower.begin(), answer_lower.end(), answer_lower.begin(), ::tolower);
    
    size_t issue_count = 0;
    
    // Check for repeated words
    std::istringstream stream(answer_lower);
    std::string prev_word, word;
    stream >> prev_word;
    while (stream >> word) {
        if (word == prev_word && word.length() > 3) {
            issue_count++;
        }
        prev_word = word;
    }
    
    // Check for overly long sentences (>40 words)
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    
    for (auto it = sentences_begin; it != sentences_end; ++it) {
        std::string sentence = it->str();
        std::istringstream sent_stream(sentence);
        size_t word_count = 0;
        std::string w = {};
        while (sent_stream >> w) {
          word_count++;
        }
        
        if (word_count > 40) {
            issue_count++;
        }
    }
    
    // Calculate linguistic quality
    size_t total_sentences = std::distance(sentences_begin, sentences_end);
    double error_rate = total_sentences > 0 
        ? static_cast<double>(issue_count) / total_sentences 
        : 0.0;
    
    double grammar_score = 1.0 - std::min(1.0, error_rate);
    
    // Combine readability and grammar
    double linguistic_score = (readability * 0.6 + grammar_score * 0.4);
    
    THEMIS_DEBUG("Linguistic quality: {:.2f} (readability={:.2f}, grammar={:.2f}, issues={})",
                 linguistic_score, readability, grammar_score, issue_count);
    
    return linguistic_score;
}

std::vector<std::string> CoherenceEvaluator::detectContradictions(const std::string& answer) {
    std::vector<std::string> contradictions;
    
    if (!impl_->config.enable_contradiction_detection || answer.empty()) {
        return contradictions;
    }
    
    // Split into sentences
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    std::vector<std::string> sentences;
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    
    // Count matches for reserve
    size_t match_count = 0;
    for (auto it = sentences_begin; it != sentences_end; ++it) {
        ++match_count;
    }
    sentences.reserve(match_count);
    contradictions.reserve(std::max(size_t(1), match_count / 4));  // Expect ~25% contradictions
    
    for (auto it = sentences_begin; it != sentences_end; ++it) {
        sentences.push_back(it->str());
    }
    
    // Simple contradiction detection using negation patterns
    // In production, this would use NLI model to check for contradictions
    
    // Convert to set for O(1) lookup during word parsing
    // Complexity: O(n_sentences² × n_chars) instead of O(n_sentences² × n_negations × n_chars)
    std::set<std::string> negation_words_set = {
        "not", "no", "never", "cannot", "can't", "won't", "don't", "doesn't",
        "isn't", "aren't", "wasn't", "weren't", "however", "but", "although"
    };
    
    // Look for sentences with opposing negation patterns on similar topics
    for (size_t i = 0; i <static_cast<int>(sentences.size()); ++i) {
        for (size_t j = i + 1; j <static_cast<int>(sentences.size()); ++j) {
            std::string sent_i = sentences[i];
            std::string sent_j = sentences[j];
            
            std::transform(sent_i.begin(), sent_i.end(), sent_i.begin(), ::tolower);
            std::transform(sent_j.begin(), sent_j.end(), sent_j.begin(), ::tolower);
            
            // Check if sentences have negation words by parsing once
            // Optimization: extract words and check set membership O(log n) instead of O(n)
            bool i_has_negation = false;
            bool j_has_negation = false;
            
            // Parse sent_i words and check for negations
            if (!i_has_negation) {
                std::istringstream stream_i(sent_i);
                std::string word = {};
                while (stream_i >> word && !i_has_negation) {
                    // Remove punctuation from word end
                    while (!word.empty() && (word.back() < 'a' || word.back() > 'z')) {
                        word.pop_back();
                    }
                    if (negation_words_set.count(word)) {
                        i_has_negation = true;
                    }
                }
            }
            
            // Parse sent_j words and check for negations
            if (!j_has_negation) {
                std::istringstream stream_j(sent_j);
                std::string word = {};
                while (stream_j >> word && !j_has_negation) {
                    // Remove punctuation from word end
                    while (!word.empty() && (word.back() < 'a' || word.back() > 'z')) {
                        word.pop_back();
                    }
                    if (negation_words_set.count(word)) {
                        j_has_negation = true;
                    }
                }
            }
            
            // If one is negated and other isn't, check for common key terms
            if (i_has_negation != j_has_negation) {
                // Extract key terms
                std::set<std::string> terms_i, terms_j;
                std::istringstream stream_i(sent_i), stream_j(sent_j);
                std::string word = {};
                
                while (stream_i >> word) {
                    if (word.length() > 4) {
                      terms_i.insert(word);
                    }
                }
                while (stream_j >> word) {
                    if (word.length() > 4) {
                      terms_j.insert(word);
                    }
                }
                
                std::set<std::string> common;
                std::set_intersection(terms_i.begin(), terms_i.end(),
                                    terms_j.begin(), terms_j.end(),
                                    std::inserter(common, common.begin()));
                
                // If they share significant terms, might be contradiction
                if (static_cast<int>(common.size()) > = 2) {
                    contradictions.push_back(sentences[i] + " <-> " + sentences[j]);
                }
            }
        }
    }
    
    THEMIS_DEBUG("Detected {} potential contradictions",static_cast<int>(contradictions.size()));
    return contradictions;
}

CoherenceResult CoherenceEvaluator::evaluate(const std::string& answer) {
    CoherenceResult result = {};
    
    if (answer.empty()) {
        result.coherence_score = 0.0;
        result.explanation = "Empty answer.";
        return result;
    }
    
    // Step 1: Analyze logical flow
    result.logical_flow_score = analyzeLogicalFlow(answer);
    
    // Step 2: Assess structure
    result.structural_score = assessStructure(answer);
    
    // Step 3: Evaluate linguistic quality
    result.linguistic_score = evaluateLinguisticQuality(answer);
    
    // Step 4: Detect contradictions
    result.contradictions = detectContradictions(answer);
    result.has_contradictions = !result.contradictions.empty();
    
    // Calculate consistency score based on contradictions
    if (result.has_contradictions) {
        // Penalty based on number of contradictions
        double contradiction_penalty = std::min(1.0,static_cast<int>(result.contradictions.size()) * 0.3);
        result.consistency_score = 1.0 - contradiction_penalty;
    } else {
        result.consistency_score = 1.0;
    }
    
    // Step 5: Calculate overall coherence score
    result.coherence_score = 
        result.logical_flow_score * impl_->config.logical_flow_weight +
        result.structural_score * impl_->config.structural_weight +
        result.linguistic_score * impl_->config.linguistic_weight +
        result.consistency_score * impl_->config.consistency_weight;
    
    // Ensure score is in [0, 1]
    result.coherence_score = std::min(1.0, std::max(0.0, result.coherence_score));
    
    // Generate explanation
    std::ostringstream explanation = {};
    explanation << "Coherence Score: " << result.coherence_score << "\n";
    explanation << "Logical Flow: " << result.logical_flow_score << " (30%)\n";
    explanation << "Structure: " << result.structural_score << " (20%)\n";
    explanation << "Linguistic Quality: " << result.linguistic_score << " (20%)\n";
    explanation << "Consistency: " << result.consistency_score << " (30%)\n";
    
    if (result.has_contradictions) {
        explanation << "Warning: " <<static_cast<int>(result.contradictions.size()) << " potential contradiction(s) detected.\n";
    }
    
    if (result.coherence_score >= 0.8) {
        explanation << "Answer is well-structured and logically coherent.";
    } else if (result.coherence_score >= 0.6) {
        explanation << "Answer has reasonable coherence with some room for improvement.";
    } else {
        explanation << "Answer has coherence issues affecting readability.";
    }
    
    result.explanation = explanation.str();
    
    THEMIS_INFO("Coherence evaluation complete: score={:.2f}, flow={:.2f}, structure={:.2f}, linguistic={:.2f}, consistency={:.2f}", 
                result.coherence_score, result.logical_flow_score, result.structural_score,
                result.linguistic_score, result.consistency_score);
    
    return result;
}

} // namespace themis::rag::judge

