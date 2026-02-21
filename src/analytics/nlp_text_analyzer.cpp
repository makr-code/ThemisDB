/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            nlp_text_analyzer.cpp                              ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     915                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e9770daf0  2026-02-12  Add Legal Modality Analyzer for German Administrative Law... ║
    • d66369dc5  2026-01-16  feat: Add ThemisDB Static Initialization Crash Analyzer ║
    • c2830e1d8  2026-01-11  Add NLP Text Analyzer with Full Pipeline Integration: Que... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "analytics/nlp_text_analyzer.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <cmath>
#include <unordered_set>
#include <fstream>
#include <iostream>

namespace themis {
namespace analytics {

// ========== Constructor and Initialization ==========

NlpTextAnalyzer::NlpTextAnalyzer(const Config& config)
    : config_(config)
{
    try {
        // Try to load stop words from YAML files if enabled
        if (config_.auto_load_stopwords && !config_.stopwords_directory.empty()) {
            try {
                size_t loaded = loadStopWordsFromDirectory(config_.stopwords_directory);
                if (loaded == 0) {
                    // Fallback to hard-coded stop words if YAML loading fails
                    initializeStopWords();
                }
            } catch (const std::exception& e) {
                // Log to stderr and fall back to hard-coded stop words
                std::cerr << "WARNING: NlpTextAnalyzer failed to load stopwords from " 
                          << config_.stopwords_directory << ": " << e.what() 
                          << " - using hard-coded stopwords" << std::endl;
                initializeStopWords();
            }
        } else {
            // Use hard-coded stop words
            initializeStopWords();
        }
        
        initializeSentimentLexicon();
        initializeEntityPatterns();
    } catch (const std::exception& e) {
        // Catch any exception during initialization to prevent crash
        std::cerr << "ERROR: NlpTextAnalyzer initialization failed: " << e.what() << std::endl;
        // Initialize with minimal defaults
        try {
            initializeStopWords();
            initializeSentimentLexicon();
            initializeEntityPatterns();
        } catch (...) {
            std::cerr << "CRITICAL: NlpTextAnalyzer minimal initialization also failed!" << std::endl;
        }
    }
}

void NlpTextAnalyzer::initializeStopWords() {
    // English stop words
    stopwords_[Language::ENGLISH] = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
        "of", "with", "by", "from", "as", "is", "was", "are", "were", "been",
        "be", "have", "has", "had", "do", "does", "did", "will", "would",
        "should", "could", "may", "might", "must", "can", "this", "that",
        "these", "those", "i", "you", "he", "she", "it", "we", "they"
    };
    
    // German stop words
    stopwords_[Language::GERMAN] = {
        "der", "die", "das", "und", "oder", "aber", "in", "auf", "an", "zu",
        "von", "mit", "bei", "aus", "für", "als", "ist", "war", "sind",
        "waren", "sein", "haben", "hat", "hatte", "werden", "wird", "wurde",
        "ich", "du", "er", "sie", "es", "wir", "ihr", "ein", "eine", "einer",
        "dem", "den", "des"
    };
}

void NlpTextAnalyzer::initializeSentimentLexicon() {
    // Basic positive words
    sentiment_lexicon_["good"] = 0.5;
    sentiment_lexicon_["great"] = 0.7;
    sentiment_lexicon_["excellent"] = 0.8;
    sentiment_lexicon_["wonderful"] = 0.7;
    sentiment_lexicon_["amazing"] = 0.8;
    sentiment_lexicon_["best"] = 0.6;
    sentiment_lexicon_["love"] = 0.7;
    sentiment_lexicon_["like"] = 0.4;
    sentiment_lexicon_["happy"] = 0.6;
    sentiment_lexicon_["positive"] = 0.5;
    
    // Basic negative words
    sentiment_lexicon_["bad"] = -0.5;
    sentiment_lexicon_["terrible"] = -0.8;
    sentiment_lexicon_["horrible"] = -0.8;
    sentiment_lexicon_["awful"] = -0.7;
    sentiment_lexicon_["worst"] = -0.7;
    sentiment_lexicon_["hate"] = -0.7;
    sentiment_lexicon_["dislike"] = -0.4;
    sentiment_lexicon_["sad"] = -0.5;
    sentiment_lexicon_["negative"] = -0.5;
    sentiment_lexicon_["problem"] = -0.3;
    sentiment_lexicon_["issue"] = -0.3;
    sentiment_lexicon_["error"] = -0.4;
}

void NlpTextAnalyzer::initializeEntityPatterns() {
    // Simple patterns for named entity recognition
    // These are basic heuristics, not ML-based NER
    
    // Email addresses
    entity_patterns_.push_back({
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})",
        "EMAIL"
    });
    
    // URLs
    entity_patterns_.push_back({
        R"(https?://[^\s]+)",
        "URL"
    });
    
    // Dates (simple patterns)
    entity_patterns_.push_back({
        R"(\d{1,2}[./-]\d{1,2}[./-]\d{2,4})",
        "DATE"
    });
    
    // Numbers with units
    entity_patterns_.push_back({
        R"(\d+\s*(GB|MB|KB|TB|kg|km|m|cm))",
        "MEASUREMENT"
    });
}

// ========== Core Analysis Functions ==========

NlpTextAnalyzer::Language NlpTextAnalyzer::detectLanguage(std::string_view text) const {
    std::string lower = toLowerCase(text);
    
    // Simple heuristic-based language detection
    // Count characteristic words per language
    std::map<Language, size_t> scores;
    
    // German indicators
    if (lower.find(" der ") != std::string::npos ||
        lower.find(" die ") != std::string::npos ||
        lower.find(" und ") != std::string::npos ||
        lower.find(" nicht ") != std::string::npos) {
        scores[Language::GERMAN] += 3;
    }
    
    // English indicators
    if (lower.find(" the ") != std::string::npos ||
        lower.find(" and ") != std::string::npos ||
        lower.find(" not ") != std::string::npos ||
        lower.find(" this ") != std::string::npos) {
        scores[Language::ENGLISH] += 3;
    }
    
    // French indicators  
    if (lower.find(" le ") != std::string::npos ||
        lower.find(" la ") != std::string::npos ||
        lower.find(" et ") != std::string::npos ||
        lower.find(" pas ") != std::string::npos) {
        scores[Language::FRENCH] += 3;
    }
    
    // Return language with highest score
    if (scores.empty()) {
        return config_.default_language;
    }
    
    auto max_lang = std::max_element(scores.begin(), scores.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return max_lang->first;
}

std::vector<Token> NlpTextAnalyzer::tokenize(std::string_view text) const {
    std::vector<Token> tokens;
    
    std::string current_word;
    size_t position = 0;
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        
        if (std::isalnum(c) || c == '_' || c == '-') {
            current_word += c;
        } else {
            if (!current_word.empty()) {
                if (current_word.length() >= config_.min_word_length) {
                    Token token(current_word, position);
                    token.lemma = toLowerCase(current_word);
                    
                    // Simple POS tagging based on patterns
                    if (isCapitalized(current_word)) {
                        token.pos_tag = "PROPN"; // Proper noun
                    } else if (isAllCaps(current_word)) {
                        token.pos_tag = "NOUN";
                    } else {
                        token.pos_tag = "WORD";
                    }
                    
                    tokens.push_back(std::move(token));
                }
                current_word.clear();
                position = i + 1;
            }
        }
    }
    
    // Don't forget last word
    if (!current_word.empty() && current_word.length() >= config_.min_word_length) {
        Token token(current_word, position);
        token.lemma = toLowerCase(current_word);
        tokens.push_back(std::move(token));
    }
    
    token_count_ += tokens.size();
    return tokens;
}

std::vector<Keyword> NlpTextAnalyzer::extractKeywords(std::string_view text, 
                                                      size_t max_keywords) const {
    if (max_keywords == 0) {
        max_keywords = config_.max_keywords;
    }
    
    auto tokens = tokenize(text);
    Language lang = detectLanguage(text);
    
    // Calculate term frequencies
    std::map<std::string, size_t> term_freqs;
    std::unordered_set<std::string> unique_terms;
    
    for (const auto& token : tokens) {
        std::string lower = toLowerCase(token.text);
        
        // Skip stop words
        if (config_.enable_stopwords && isStopWord(lower, lang)) {
            continue;
        }
        
        std::string term = config_.enable_stemming ? stemWord(lower, lang) : lower;
        term_freqs[term]++;
        unique_terms.insert(term);
    }
    
    // Calculate TF-IDF scores (simplified version)
    std::vector<Keyword> keywords;
    size_t total_terms = tokens.size();
    
    for (const auto& [term, freq] : term_freqs) {
        double score = calculateTfIdf(term, term_freqs, total_terms);
        keywords.emplace_back(term, score, freq);
    }
    
    // Sort by score and limit
    std::sort(keywords.begin(), keywords.end());
    if (keywords.size() > max_keywords) {
        keywords.resize(max_keywords);
    }
    
    return keywords;
}

std::vector<NamedEntity> NlpTextAnalyzer::extractEntities(std::string_view text) const {
    std::vector<NamedEntity> entities;
    
    // Pattern-based entity extraction
    for (const auto& pattern : entity_patterns_) {
        std::regex re(pattern.pattern, std::regex::icase);
        std::string text_str(text);
        
        auto words_begin = std::sregex_iterator(text_str.begin(), text_str.end(), re);
        auto words_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            NamedEntity entity(match.str(), pattern.type, 0.8);
            entity.start_pos = match.position();
            entity.end_pos = match.position() + match.length();
            entities.push_back(entity);
        }
    }
    
    // Heuristic-based: Capitalized words (potential proper nouns)
    auto tokens = tokenize(text);
    for (const auto& token : tokens) {
        if (isCapitalized(token.text) && token.text.length() > 2) {
            // Check if not at sentence start (simple check)
            if (token.position > 2) {
                NamedEntity entity(token.text, "PROPN", 0.5);
                entity.start_pos = token.position;
                entity.end_pos = token.position + token.text.length();
                entities.push_back(entity);
            }
        }
    }
    
    return entities;
}

SentimentResult NlpTextAnalyzer::analyzeSentiment(std::string_view text) const {
    SentimentResult result;
    
    auto tokens = tokenize(text);
    if (tokens.empty()) {
        return result;
    }
    
    double total_score = 0.0;
    size_t scored_words = 0;
    
    for (const auto& token : tokens) {
        std::string lower = toLowerCase(token.text);
        auto it = sentiment_lexicon_.find(lower);
        if (it != sentiment_lexicon_.end()) {
            total_score += it->second;
            scored_words++;
        }
    }
    
    if (scored_words > 0) {
        result.score = total_score / scored_words;
        result.confidence = std::min(1.0, static_cast<double>(scored_words) / 10.0);
        
        if (result.score > 0.2) {
            result.polarity = SentimentResult::Polarity::POSITIVE;
        } else if (result.score < -0.2) {
            result.polarity = SentimentResult::Polarity::NEGATIVE;
        } else {
            result.polarity = SentimentResult::Polarity::NEUTRAL;
        }
    }
    
    return result;
}

ComplexityMetrics NlpTextAnalyzer::analyzeComplexity(std::string_view text) const {
    ComplexityMetrics metrics;
    
    auto tokens = tokenize(text);
    auto sentences = splitSentences(text);
    
    metrics.word_count = tokens.size();
    metrics.sentence_count = sentences.size();
    
    if (tokens.empty()) {
        return metrics;
    }
    
    // Calculate unique words
    std::unordered_set<std::string> unique;
    size_t total_length = 0;
    
    for (const auto& token : tokens) {
        std::string lower = toLowerCase(token.text);
        unique.insert(lower);
        total_length += token.text.length();
        
        // Count complex words (3+ syllables)
        if (countSyllables(token.text) >= 3) {
            metrics.complex_words++;
        }
    }
    
    metrics.unique_words = unique.size();
    metrics.avg_word_length = static_cast<double>(total_length) / tokens.size();
    metrics.avg_sentence_length = static_cast<double>(tokens.size()) / 
                                   std::max<size_t>(1, sentences.size());
    metrics.lexical_diversity = static_cast<double>(metrics.unique_words) / 
                                metrics.word_count;
    
    return metrics;
}

// ========== AQL Query Optimization Support ==========

double NlpTextAnalyzer::estimateQueryComplexity(std::string_view query_text) const {
    double complexity = 0.0;
    
    // Analyze query structure
    bool has_aggregation = containsAggregation(query_text);
    bool has_join = containsJoin(query_text);
    bool has_subquery = containsSubquery(query_text);
    
    // Base complexity from text metrics
    auto metrics = analyzeComplexity(query_text);
    complexity += std::min(0.3, metrics.word_count / 100.0);
    
    // Structural complexity
    if (has_aggregation) complexity += 0.2;
    if (has_join) complexity += 0.3;
    if (has_subquery) complexity += 0.4;
    
    // Count tables
    auto tables = extractTableNames(query_text);
    complexity += std::min(0.3, tables.size() * 0.1);
    
    analysis_count_++;
    return std::min(1.0, complexity);
}

std::map<std::string, std::string> NlpTextAnalyzer::extractQueryHints(
    std::string_view query_text) const {
    
    std::map<std::string, std::string> hints;
    
    std::string lower = toLowerCase(query_text);
    
    // Detect query patterns
    if (lower.find("order by") != std::string::npos) {
        hints["sorting"] = "required";
    }
    
    if (lower.find("limit") != std::string::npos) {
        hints["result_limit"] = "yes";
    }
    
    if (containsAggregation(query_text)) {
        hints["aggregation"] = "true";
        hints["index_preference"] = "covering";
    }
    
    if (containsJoin(query_text)) {
        hints["join_type"] = "detected";
        hints["index_preference"] = "join_columns";
    }
    
    // Detect full-text search patterns
    if (lower.find("match") != std::string::npos ||
        lower.find("search") != std::string::npos ||
        lower.find("like") != std::string::npos) {
        hints["search_type"] = "fulltext";
        hints["index_preference"] = "fulltext";
    }
    
    // Detect vector search patterns
    if (lower.find("vector") != std::string::npos ||
        lower.find("similarity") != std::string::npos ||
        lower.find("nearest") != std::string::npos) {
        hints["search_type"] = "vector";
        hints["index_preference"] = "hnsw";
    }
    
    return hints;
}

std::vector<std::string> NlpTextAnalyzer::suggestIndexes(
    std::string_view query_text) const {
    
    std::vector<std::string> suggestions;
    std::string lower = toLowerCase(query_text);
    
    // Analyze query to suggest appropriate indexes
    if (lower.find("where") != std::string::npos) {
        suggestions.push_back("btree");
    }
    
    if (lower.find("match") != std::string::npos || 
        lower.find("search") != std::string::npos) {
        suggestions.push_back("fulltext");
    }
    
    if (lower.find("vector") != std::string::npos ||
        lower.find("similarity") != std::string::npos) {
        suggestions.push_back("hnsw");
    }
    
    if (lower.find("within") != std::string::npos ||
        lower.find("distance") != std::string::npos ||
        lower.find("geo") != std::string::npos) {
        suggestions.push_back("spatial");
    }
    
    if (containsJoin(query_text)) {
        suggestions.push_back("hash");
    }
    
    return suggestions;
}

std::string NlpTextAnalyzer::normalizeQuery(std::string_view query_text) const {
    std::string normalized = toLowerCase(query_text);
    
    // Remove extra whitespace
    std::regex ws_re(R"(\s+)");
    normalized = std::regex_replace(normalized, ws_re, " ");
    
    // Trim
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r") + 1);
    
    return normalized;
}

// ========== Utility Functions ==========

bool NlpTextAnalyzer::isStopWord(std::string_view word, Language lang) const {
    auto it = stopwords_.find(lang);
    if (it == stopwords_.end()) {
        return false;
    }
    
    std::string lower = toLowerCase(word);
    return it->second.find(lower) != it->second.end();
}

std::string NlpTextAnalyzer::stemWord(std::string_view word, Language lang) const {
    // Simple suffix-based stemming (Porter-like, very simplified)
    std::string stem = toLowerCase(word);
    
    if (stem.length() <= 3) {
        return stem;
    }
    
    // Remove common English suffixes
    if (lang == Language::ENGLISH) {
        if (stem.ends_with("ing")) {
            stem = stem.substr(0, stem.length() - 3);
        } else if (stem.ends_with("ed")) {
            stem = stem.substr(0, stem.length() - 2);
        } else if (stem.ends_with("s") && stem.length() > 3) {
            stem = stem.substr(0, stem.length() - 1);
        }
    }
    // Remove common German suffixes
    else if (lang == Language::GERMAN) {
        if (stem.ends_with("en")) {
            stem = stem.substr(0, stem.length() - 2);
        } else if (stem.ends_with("er")) {
            stem = stem.substr(0, stem.length() - 2);
        }
    }
    
    return stem;
}

double NlpTextAnalyzer::calculateSimilarity(std::string_view text1, 
                                            std::string_view text2) const {
    auto tokens1 = tokenize(text1);
    auto tokens2 = tokenize(text2);
    
    if (tokens1.empty() || tokens2.empty()) {
        return 0.0;
    }
    
    // Jaccard similarity
    std::unordered_set<std::string> set1, set2;
    for (const auto& t : tokens1) set1.insert(toLowerCase(t.text));
    for (const auto& t : tokens2) set2.insert(toLowerCase(t.text));
    
    size_t intersection = 0;
    for (const auto& word : set1) {
        if (set2.count(word)) {
            intersection++;
        }
    }
    
    size_t union_size = set1.size() + set2.size() - intersection;
    return union_size > 0 ? static_cast<double>(intersection) / union_size : 0.0;
}

std::map<std::string, size_t> NlpTextAnalyzer::getStatistics() const {
    return {
        {"analyses_performed", analysis_count_},
        {"tokens_processed", token_count_},
        {"stopword_languages", stopwords_.size()},
        {"sentiment_words", sentiment_lexicon_.size()}
    };
}

// ========== Private Helper Methods ==========

std::vector<std::string> NlpTextAnalyzer::splitSentences(std::string_view text) const {
    std::vector<std::string> sentences;
    std::string current;
    
    for (char c : text) {
        current += c;
        if (c == '.' || c == '!' || c == '?') {
            sentences.push_back(current);
            current.clear();
        }
    }
    
    if (!current.empty()) {
        sentences.push_back(current);
    }
    
    return sentences;
}

std::string NlpTextAnalyzer::toLowerCase(std::string_view text) const {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        result += std::tolower(static_cast<unsigned char>(c));
    }
    return result;
}

std::string NlpTextAnalyzer::removePunctuation(std::string_view text) const {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        if (std::isalnum(c) || c == ' ') {
            result += c;
        }
    }
    return result;
}

double NlpTextAnalyzer::calculateTfIdf(const std::string& term,
                                      const std::map<std::string, size_t>& term_freqs,
                                      size_t total_terms) const {
    auto it = term_freqs.find(term);
    if (it == term_freqs.end()) {
        return 0.0;
    }
    
    // TF (term frequency)
    double tf = static_cast<double>(it->second) / total_terms;
    
    // Simple IDF approximation (would need corpus for real IDF)
    // Here we use inverse frequency as a proxy
    double idf = std::log(static_cast<double>(term_freqs.size()) / it->second);
    
    return tf * idf;
}

bool NlpTextAnalyzer::isCapitalized(std::string_view word) const {
    return !word.empty() && std::isupper(static_cast<unsigned char>(word[0]));
}

bool NlpTextAnalyzer::isAllCaps(std::string_view word) const {
    if (word.empty()) return false;
    for (char c : word) {
        if (std::isalpha(c) && !std::isupper(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

size_t NlpTextAnalyzer::countSyllables(std::string_view word) const {
    // Very simple syllable counter
    // Real implementation would need phonetic rules
    size_t count = 0;
    bool previous_was_vowel = false;
    
    for (char c : word) {
        char lower = std::tolower(static_cast<unsigned char>(c));
        bool is_vowel = (lower == 'a' || lower == 'e' || lower == 'i' || 
                        lower == 'o' || lower == 'u' || lower == 'y');
        
        if (is_vowel && !previous_was_vowel) {
            count++;
        }
        previous_was_vowel = is_vowel;
    }
    
    return std::max<size_t>(1, count);
}

bool NlpTextAnalyzer::containsAggregation(std::string_view query) const {
    std::string lower = toLowerCase(query);
    return lower.find("count(") != std::string::npos ||
           lower.find("sum(") != std::string::npos ||
           lower.find("avg(") != std::string::npos ||
           lower.find("min(") != std::string::npos ||
           lower.find("max(") != std::string::npos ||
           lower.find("group by") != std::string::npos;
}

bool NlpTextAnalyzer::containsJoin(std::string_view query) const {
    std::string lower = toLowerCase(query);
    return lower.find("join") != std::string::npos ||
           lower.find("inner join") != std::string::npos ||
           lower.find("left join") != std::string::npos ||
           lower.find("right join") != std::string::npos;
}

bool NlpTextAnalyzer::containsSubquery(std::string_view query) const {
    // Simple check for nested queries
    size_t open_parens = 0;
    size_t select_count = 0;
    std::string lower = toLowerCase(query);
    
    for (size_t i = 0; i < lower.length(); ++i) {
        if (lower[i] == '(') open_parens++;
        if (lower[i] == ')') open_parens--;
        
        if (i + 6 < lower.length() && 
            lower.substr(i, 6) == "select" &&
            open_parens > 0) {
            select_count++;
        }
    }
    
    return select_count > 1;
}

std::vector<std::string> NlpTextAnalyzer::extractTableNames(std::string_view query) const {
    std::vector<std::string> tables;
    std::string lower = toLowerCase(query);
    
    // Very simple FROM clause parsing
    size_t from_pos = lower.find("from ");
    if (from_pos != std::string::npos) {
        size_t start = from_pos + 5;
        size_t end = lower.find_first_of(" ,;)", start);
        if (end == std::string::npos) end = lower.length();
        
        std::string table = lower.substr(start, end - start);
        // Trim
        table.erase(0, table.find_first_not_of(" \t"));
        table.erase(table.find_last_not_of(" \t") + 1);
        
        if (!table.empty()) {
            tables.push_back(table);
        }
    }
    
    return tables;
}

// ========== YAML Loading Functions ==========

bool NlpTextAnalyzer::loadStopWordsFromYaml(const std::string& yaml_path, Language lang) {
    std::ifstream file(yaml_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::set<std::string> words;
    std::string line;
    bool in_stopwords_section = false;
    
    // Simple YAML parser (only handles our specific format)
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Detect stopwords section
        if (line == "stopwords:") {
            in_stopwords_section = true;
            continue;
        }
        
        // Exit stopwords section on new top-level key
        if (in_stopwords_section && !line.empty() && line[0] != '-' && line.find(':') != std::string::npos) {
            break;
        }
        
        // Parse stop word (YAML list item: "  - word")
        if (in_stopwords_section && line.size() > 2 && line[0] == '-') {
            std::string word = line.substr(1);
            // Trim and remove quotes
            word.erase(0, word.find_first_not_of(" \t\"'"));
            word.erase(word.find_last_not_of(" \t\"'") + 1);
            
            if (!word.empty()) {
                words.insert(toLowerCase(word));
            }
        }
    }
    
    if (!words.empty()) {
        stopwords_[lang] = std::move(words);
        return true;
    }
    
    return false;
}

size_t NlpTextAnalyzer::loadStopWordsFromDirectory(const std::string& directory) {
    size_t loaded_count = 0;
    
    // Map of language codes to Language enum
    std::map<std::string, Language> lang_map = {
        {"en", Language::ENGLISH},
        {"de", Language::GERMAN},
        {"fr", Language::FRENCH},
        {"es", Language::SPANISH},
        {"it", Language::ITALIAN},
        {"nl", Language::DUTCH}
    };
    
    // Try to load each language file
    for (const auto& [code, lang] : lang_map) {
        std::string yaml_path = directory + "/" + code + ".yaml";
        
        if (loadStopWordsFromYaml(yaml_path, lang)) {
            loaded_count++;
        }
    }
    
    return loaded_count;
}

// ========== Legal Modality Extraction ==========

std::string NlpTextAnalyzer::getDefaultLegalConfigPath(const std::string& language_code) const {
    return "config/nlp/legal/german_modal_verbs.yaml";
}

bool NlpTextAnalyzer::loadLegalModalityConfig(const std::string& config_path) const {
    // TODO: Implement YAML parsing properly
    // For now, return false to use fallback patterns
    return false;
}

std::vector<LegalModality> NlpTextAnalyzer::extractLegalModalities(
    std::string_view text,
    const std::string& language_code,
    const std::string& config_path) const {
    
    std::vector<LegalModality> modalities;
    
    // Determine config file path
    std::string cfg_path = config_path.empty() 
        ? getDefaultLegalConfigPath(language_code) 
        : config_path;
    
    // Load configuration (cached in legal_modality_patterns_)
    if (legal_modality_patterns_.empty()) {
        // For now, use fallback hard-coded patterns
        // TODO: Fix YAML loading in future iteration
        legal_modality_patterns_ = {
            {"\\bmuss\\b", "obligation", 1.0f, "O(φ)", "Bindende Rechtspflicht", {}},
            {"\\bhat zu\\b", "obligation", 1.0f, "O(φ)", "Formale bindende Verpflichtung", {}},
            {"\\bsoll\\b", "default_obligation", 0.8f, "O_default(φ)", "Regelfall, Abweichung rechtfertigungsbedürftig", {"Begründungspflicht bei Abweichung", "Verhältnismäßigkeitsprüfung"}},
            {"\\bkann\\b", "permission", 0.3f, "P(φ)", "Ermessensentscheidung", {"Ermessensausübung erforderlich", "Gleichbehandlungsgrundsatz", "Verhältnismäßigkeitsprüfung"}}
        };
    }
    
    // Convert text to string for regex processing
    std::string text_str(text);
    std::string text_lower = toLowerCase(text);
    
    // Search for each modal verb pattern
    for (const auto& pattern : legal_modality_patterns_) {
        try {
            std::regex regex_pattern(pattern.pattern, std::regex::icase);
            
            // Use iterators to avoid creating substring copies
            auto search_begin = text_lower.cbegin();
            auto search_end = text_lower.cend();
            std::smatch match;
            
            while (std::regex_search(search_begin, search_end, match, regex_pattern)) {
                size_t position = std::distance(text_lower.cbegin(), search_begin) + match.position();
                
                // Extract the matched verb from original text (preserving case)
                std::string matched_verb = text_str.substr(position, match.length());
                
                LegalModality modality(
                    matched_verb,
                    pattern.category,
                    pattern.strength,
                    pattern.deontic_logic,
                    pattern.interpretation,
                    position
                );
                modality.context_requirements = pattern.context_requirements;
                
                modalities.push_back(modality);
                
                // Continue searching after this match
                search_begin += match.position() + match.length();
            }
        } catch (const std::regex_error& e) {
            // Skip invalid regex patterns
            std::cerr << "Regex error for pattern '" << pattern.pattern << "': " << e.what() << std::endl;
        }
    }
    
    // Sort by position
    std::sort(modalities.begin(), modalities.end(),
              [](const LegalModality& a, const LegalModality& b) {
                  return a.position < b.position;
              });
    
    return modalities;
}

} // namespace analytics
} // namespace themis
