/**
 * @file nlp_text_analyzer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "analytics/nlp_text_analyzer.h"
#include <stdexcept>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace analytics {

// ========== Constructor and Initialization ==========

NlpTextAnalyzer::NlpTextAnalyzer(const Config &config) : config_(config) {
    try {
        // Try to load stop words from YAML files if enabled
        if (config_.auto_load_stopwords && !config_.stopwords_directory.empty()) {
            try {
                size_t loaded = loadStopWordsFromDirectory(config_.stopwords_directory);
                if (loaded == 0) {
                    // Fallback to hard-coded stop words if YAML loading fails
                    initializeStopWords();
                }
            } catch (const std::exception &e) {
                // Log to stderr and fall back to hard-coded stop words
                std::cerr << "WARNING: NlpTextAnalyzer failed to load stopwords from " << config_.stopwords_directory
                          << ": " << e.what() << " - using hard-coded stopwords" << std::endl;
                initializeStopWords();
            }
        } else {
            // Use hard-coded stop words
            initializeStopWords();
        }

        initializeSentimentLexicon();
        initializeEntityPatterns();
        initializeLemmatizationData();
    } catch (const std::exception &e) {
        // Catch any exception during initialization to prevent crash
        std::cerr << "ERROR: NlpTextAnalyzer initialization failed: " << e.what() << std::endl;
        // Initialize with minimal defaults
        try {
            initializeStopWords();
            initializeSentimentLexicon();
            initializeEntityPatterns();
            initializeLemmatizationData();
        } catch (const std::exception& e) {
            std::cerr << "CRITICAL: NlpTextAnalyzer minimal initialization also failed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "CRITICAL: NlpTextAnalyzer minimal initialization also failed!" << std::endl;
        }
    }
}

void NlpTextAnalyzer::initializeStopWords() {
    // English stop words
    stopwords_[Language::ENGLISH]
        = {"a",    "an",   "the",   "and",   "or",   "but",   "in",     "on",    "at",   "to",    "for",  "of",
           "with", "by",   "from",  "as",    "is",   "was",   "are",    "were",  "been", "be",    "have", "has",
           "had",  "do",   "does",  "did",   "will", "would", "should", "could", "may",  "might", "must", "can",
           "this", "that", "these", "those", "i",    "you",   "he",     "she",   "it",   "we",    "they"};

    // German stop words
    stopwords_[Language::GERMAN] = {"der",  "die",   "das", "und",   "oder",   "aber", "in",    "auf", "an",   "zu",
                                    "von",  "mit",   "bei", "aus",   "für",    "als",  "ist",   "war", "sind", "waren",
                                    "sein", "haben", "hat", "hatte", "werden", "wird", "wurde", "ich", "du",   "er",
                                    "sie",  "es",    "wir", "ihr",   "ein",    "eine", "einer", "dem", "den",  "des"};

    // French stop words
    stopwords_[Language::FRENCH]
        = {"le",    "la",   "les", "un",    "une",  "des",  "du",  "de",    "et",   "ou",  "mais", "donc",  "ni",
           "car",   "à",    "en",  "par",   "pour", "dans", "sur", "avec",  "sans", "je",  "tu",   "il",    "elle",
           "nous",  "vous", "ils", "elles", "me",   "te",   "se",  "lui",   "leur", "est", "sont", "était", "été",
           "avoir", "ai",   "as",  "ont",   "pas",  "ne",   "ce",  "cette", "ces",  "qui", "que",  "quoi",  "où"};

    // Spanish stop words
    stopwords_[Language::SPANISH]
        = {"el",    "la",    "los",   "las", "un",   "una",     "unos", "unas",     "de",    "en",   "por",   "para",
           "con",   "sin",   "y",     "o",   "pero", "ni",      "que",  "es",       "son",   "era",  "están", "ser",
           "estar", "haber", "tener", "yo",  "tú",   "él",      "ella", "nosotros", "ellos", "se",   "le",    "les",
           "lo",    "no",    "sí",    "muy", "más",  "también", "ya",   "todo",     "otro",  "mismo"};

    // Italian stop words
    stopwords_[Language::ITALIAN]
        = {"il", "lo",  "la",  "i",     "gli", "le",    "un",  "uno",   "una", "di",   "da",   "in",     "con",
           "su", "per", "tra", "fra",   "e",   "o",     "ma",  "che",   "è",   "sono", "era",  "essere", "avere",
           "ho", "hai", "ha",  "hanno", "io",  "tu",    "lui", "lei",   "noi", "voi",  "loro", "mi",     "ti",
           "si", "ci",  "non", "no",    "sì",  "molto", "più", "anche", "già", "tutto"};

    // Dutch stop words
    stopwords_[Language::DUTCH]
        = {"de",   "het", "een", "en", "of",   "maar", "van",  "in",    "op",     "aan",   "te",  "voor",
           "met",  "bij", "uit", "na", "over", "door", "dat",  "die",   "dit",    "deze",  "ik",  "jij",
           "hij",  "zij", "wij", "ze", "zijn", "is",   "was",  "waren", "hebben", "heeft", "had", "worden",
           "niet", "ook", "nog", "al", "wel",  "hier", "daar", "nu",    "zo"};
}

void NlpTextAnalyzer::initializeSentimentLexicon() {
    // Basic positive words
    sentiment_lexicon_["good"]      = 0.5;
    sentiment_lexicon_["great"]     = 0.7;
    sentiment_lexicon_["excellent"] = 0.8;
    sentiment_lexicon_["wonderful"] = 0.7;
    sentiment_lexicon_["amazing"]   = 0.8;
    sentiment_lexicon_["best"]      = 0.6;
    sentiment_lexicon_["love"]      = 0.7;
    sentiment_lexicon_["like"]      = 0.4;
    sentiment_lexicon_["happy"]     = 0.6;
    sentiment_lexicon_["positive"]  = 0.5;

    // Basic negative words
    sentiment_lexicon_["bad"]      = -0.5;
    sentiment_lexicon_["terrible"] = -0.8;
    sentiment_lexicon_["horrible"] = -0.8;
    sentiment_lexicon_["awful"]    = -0.7;
    sentiment_lexicon_["worst"]    = -0.7;
    sentiment_lexicon_["hate"]     = -0.7;
    sentiment_lexicon_["dislike"]  = -0.4;
    sentiment_lexicon_["sad"]      = -0.5;
    sentiment_lexicon_["negative"] = -0.5;
    sentiment_lexicon_["problem"]  = -0.3;
    sentiment_lexicon_["issue"]    = -0.3;
    sentiment_lexicon_["error"]    = -0.4;
}

void NlpTextAnalyzer::initializeEntityPatterns() {
    // Simple patterns for named entity recognition
    // These are basic heuristics, not ML-based NER

    // Email addresses
    entity_patterns_.push_back({R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})", "EMAIL"});

    // URLs
    entity_patterns_.push_back({R"(https?://[^\s]+)", "URL"});

    // Dates (simple patterns)
    entity_patterns_.push_back({R"(\d{1,2}[./-]\d{1,2}[./-]\d{2,4})", "DATE"});

    // Numbers with units
    entity_patterns_.push_back({R"(\d+\s*(GB|MB|KB|TB|kg|km|m|cm))", "MEASUREMENT"});
}

// ========== Core Analysis Functions ==========

NlpTextAnalyzer::Language NlpTextAnalyzer::detectLanguage(std::string_view text) const {
    std::string lower = toLowerCase(text);

    // Simple heuristic-based language detection.
    // Each characteristic indicator word adds 1 to its language score so that
    // texts with more matching words win over single-word coincidences.
    std::map<Language, size_t> scores;

    // Helper: increment score for each indicator found
    auto count = [&](Language lang, const std::initializer_list<const char *> &indicators) {
        for (const char *ind : indicators) {
            if (lower.find(ind) != std::string::npos) {
                scores[lang]++;
            }
        }
    };

    // German indicators
    count(Language::GERMAN, {" der ", " die ", " das ", " und ", " nicht ", " mit ", " auf "});

    // English indicators
    count(Language::ENGLISH, {" the ", " and ", " not ", " this ", " that ", " with ", " for "});

    // French indicators (use only unambiguous words; avoid " la " shared with Spanish)
    count(Language::FRENCH, {" le ", " les ", " et ", " pas ", " sont ", " une ", " dans "});

    // Spanish indicators (use only unambiguous words; avoid " la " shared with French)
    count(Language::SPANISH, {" el ", " los ", " las ", " para ", " con ", " una ", " pero "});

    // Italian indicators
    count(Language::ITALIAN, {" gli ", " dello ", " della ", " sono ", " per ", " del ", " che "});

    // Dutch indicators
    count(Language::DUTCH, {" het ", " een ", " van ", " niet ", " zijn ", " met ", " naar "});

    // Return language with highest score
    if (scores.empty()) {
        return config_.default_language;
    }

    auto max_lang = std::max_element(scores.begin(), scores.end(),
                                     [](const auto &a, const auto &b) { return a.second < b.second; });

    return max_lang->first;
}

std::vector<Token> NlpTextAnalyzer::tokenize(std::string_view text) const {
    std::vector<Token> tokens;

    std::string current_word = {};
    size_t position = 0;
    Language lang   = detectLanguage(text);

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        if (std::isalnum(c) || c == '_' || c == '-') {
            current_word += c;
        } else {
            if (!current_word.empty()) {
                if (current_word.length() >= config_.min_word_length) {
                    Token token(current_word, position);
                    token.lemma = lemmatizeWord(current_word, lang);

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
        token.lemma = lemmatizeWord(current_word, lang);
        tokens.push_back(std::move(token));
    }

    token_count_ += tokens.size();
    return tokens;
}

std::vector<Keyword> NlpTextAnalyzer::extractKeywords(std::string_view text, size_t max_keywords) const {
    if (max_keywords == 0) {
        max_keywords = config_.max_keywords;
    }

    auto tokens   = tokenize(text);
    Language lang = detectLanguage(text);

    // Calculate term frequencies
    std::map<std::string, size_t> term_freqs;
    std::unordered_set<std::string> unique_terms;

    for (const auto &token : tokens) {
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
    std::vector<Keyword> keywords = {};

    size_t total_terms = tokens.size();

    for (const auto &[term, freq] : term_freqs) {
        double score = calculateTfIdf(term, term_freqs, total_terms);
        keywords.emplace_back(term, score, freq);
    }

    // Sort by score and limit
    std::sort(keywords.begin(), keywords.end());
    if (static_cast<int>(keywords.size()) > max_keywords) {
        keywords.resize(max_keywords);
    }

    return keywords;
}

std::vector<NamedEntity> NlpTextAnalyzer::extractEntities(std::string_view text) const {
    std::vector<NamedEntity> entities;

    // Pattern-based entity extraction
    for (const auto &pattern : entity_patterns_) {
        std::regex re(pattern.pattern, std::regex::icase);
        std::string text_str(text);

        auto words_begin = std::sregex_iterator(text_str.begin(), text_str.end(), re);
        auto words_end   = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            NamedEntity entity(match.str(), pattern.type, 0.8);
            entity.start_pos = match.position();
            entity.end_pos   = match.position() + match.length();
            entities.push_back(entity);
        }
    }

    // Heuristic-based: Capitalized words (potential proper nouns)
    auto tokens = tokenize(text);
    for (const auto &token : tokens) {
        if (isCapitalized(token.text) && token.text.length() > 2) {
            // Check if not at sentence start (simple check)
            if (token.position > 2) {
                NamedEntity entity(token.text, "PROPN", 0.5);
                entity.start_pos = token.position;
                entity.end_pos   = token.position + token.text.length();
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

    double total_score  = 0.0;
    size_t scored_words = 0;

    for (const auto &token : tokens) {
        std::string lower = toLowerCase(token.text);
        auto it           = sentiment_lexicon_.find(lower);
        if (it != sentiment_lexicon_.end()) {
            total_score += it->second;
            scored_words++;
        }
    }

    if (scored_words > 0) {
        result.score      = total_score / scored_words;
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

    auto tokens    = tokenize(text);
    auto sentences = splitSentences(text);

    metrics.word_count     = tokens.size();
    metrics.sentence_count = sentences.size();

    if (tokens.empty()) {
        return metrics;
    }

    // Calculate unique words
    std::unordered_set<std::string> unique;
    size_t total_length = 0;

    for (const auto &token : tokens) {
        std::string lower = toLowerCase(token.text);
        unique.insert(lower);
        total_length += token.text.length();

        // Count complex words (3+ syllables)
        if (countSyllables(token.text) >= 3) {
            metrics.complex_words++;
        }
    }

    metrics.unique_words        = unique.size();
    metrics.avg_word_length     = static_cast<double>(total_length) / tokens.size();
    metrics.avg_sentence_length = static_cast<double>(tokens.size()) / std::max<size_t>(1, sentences.size());
    metrics.lexical_diversity   = static_cast<double>(metrics.unique_words) / metrics.word_count;

    return metrics;
}

// ========== AQL Query Optimization Support ==========

double NlpTextAnalyzer::estimateQueryComplexity(std::string_view query_text) const {
    double complexity = 0.0;

    // Analyze query structure
    bool has_aggregation = containsAggregation(query_text);
    bool has_join        = containsJoin(query_text);
    bool has_subquery    = containsSubquery(query_text);

    // Base complexity from text metrics
    auto metrics = analyzeComplexity(query_text);
    complexity += std::min(0.3, metrics.word_count / 100.0);

    // Structural complexity
    if (has_aggregation) {
        complexity += 0.2;
    }
    if (has_join) {
        complexity += 0.3;
    }
    if (has_subquery) {
        complexity += 0.4;
    }

    // Count tables
    auto tables = extractTableNames(query_text);
    complexity += std::min(0.3, tables.size() * 0.1);

    analysis_count_++;
    return std::min(1.0, complexity);
}

std::map<std::string, std::string> NlpTextAnalyzer::extractQueryHints(std::string_view query_text) const {
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
        hints["aggregation"]      = "true";
        hints["index_preference"] = "covering";
    }

    if (containsJoin(query_text)) {
        hints["join_type"]        = "detected";
        hints["index_preference"] = "join_columns";
    }

    // Detect full-text search patterns
    if (lower.find("match") != std::string::npos || lower.find("search") != std::string::npos
        || lower.find("like") != std::string::npos) {
        hints["search_type"]      = "fulltext";
        hints["index_preference"] = "fulltext";
    }

    // Detect vector search patterns
    if (lower.find("vector") != std::string::npos || lower.find("similarity") != std::string::npos
        || lower.find("nearest") != std::string::npos) {
        hints["search_type"]      = "vector";
        hints["index_preference"] = "hnsw";
    }

    return hints;
}

std::vector<std::string> NlpTextAnalyzer::suggestIndexes(std::string_view query_text) const {
    std::vector<std::string> suggestions;
    std::string lower = toLowerCase(query_text);

    // Analyze query to suggest appropriate indexes
    if (lower.find("where") != std::string::npos) {
        suggestions.push_back("btree");
    }

    if (lower.find("match") != std::string::npos || lower.find("search") != std::string::npos) {
        suggestions.push_back("fulltext");
    }

    if (lower.find("vector") != std::string::npos || lower.find("similarity") != std::string::npos) {
        suggestions.push_back("hnsw");
    }

    if (lower.find("within") != std::string::npos || lower.find("distance") != std::string::npos
        || lower.find("geo") != std::string::npos) {
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
    // Delegate to morphological lemmatization which produces better base forms
    return lemmatizeWord(word, lang);
}

// ========== Full Morphological Lemmatization ==========

void NlpTextAnalyzer::initializeLemmatizationData() {
    // ---- English irregular forms ----
    auto &en = irregular_lemmas_[Language::ENGLISH];
    // Irregular verbs (conjugated -> base)
    en["am"]      = "be";
    en["is"]      = "be";
    en["are"]     = "be";
    en["was"]     = "be";
    en["were"]    = "be";
    en["been"]    = "be";
    en["being"]   = "be";
    en["has"]     = "have";
    en["had"]     = "have";
    en["having"]  = "have";
    en["does"]    = "do";
    en["did"]     = "do";
    en["done"]    = "do";
    en["doing"]   = "do";
    en["went"]    = "go";
    en["gone"]    = "go";
    en["goes"]    = "go";
    en["going"]   = "go";
    en["said"]    = "say";
    en["says"]    = "say";
    en["saying"]  = "say";
    en["got"]     = "get";
    en["gotten"]  = "get";
    en["gets"]    = "get";
    en["getting"] = "get";
    en["made"]    = "make";
    en["makes"]   = "make";
    en["making"]  = "make";
    en["knew"]    = "know";
    en["known"]   = "know";
    en["knows"]   = "know";
    en["took"]    = "take";
    en["taken"]   = "take";
    en["takes"]   = "take";
    en["saw"]     = "see";
    en["seen"]    = "see";
    en["sees"]    = "see";
    en["came"]    = "come";
    en["comes"]   = "come";
    en["coming"]  = "come";
    en["thought"] = "think";
    en["thinks"]  = "think";
    en["ran"]     = "run";
    en["runs"]    = "run";
    en["running"] = "run";
    en["fell"]    = "fall";
    en["fallen"]  = "fall";
    en["falls"]   = "fall";
    en["gave"]    = "give";
    en["given"]   = "give";
    en["gives"]   = "give";
    en["found"]   = "find";
    en["finds"]   = "find";
    en["told"]    = "tell";
    en["tells"]   = "tell";
    en["wrote"]   = "write";
    en["written"] = "write";
    en["writes"]  = "write";
    en["bought"]  = "buy";
    en["buys"]    = "buy";
    en["brought"] = "bring";
    en["brings"]  = "bring";
    en["built"]   = "build";
    en["builds"]  = "build";
    en["kept"]    = "keep";
    en["keeps"]   = "keep";
    en["left"]    = "leave";
    en["leaves"]  = "leave";
    en["leaving"] = "leave";
    en["lost"]    = "lose";
    en["loses"]   = "lose";
    en["losing"]  = "lose";
    en["met"]     = "meet";
    en["meets"]   = "meet";
    en["paid"]    = "pay";
    en["pays"]    = "pay";
    en["put"]     = "put";
    en["read"]    = "read";
    en["sent"]    = "send";
    en["sends"]   = "send";
    en["set"]     = "set";
    en["stood"]   = "stand";
    en["stands"]  = "stand";
    en["won"]     = "win";
    en["wins"]    = "win";
    en["sat"]     = "sit";
    en["sits"]    = "sit";
    en["cut"]     = "cut";
    en["cuts"]    = "cut";
    // Irregular nouns (plural -> singular)
    en["men"]       = "man";
    en["women"]     = "woman";
    en["children"]  = "child";
    en["people"]    = "person";
    en["feet"]      = "foot";
    en["teeth"]     = "tooth";
    en["mice"]      = "mouse";
    en["geese"]     = "goose";
    en["oxen"]      = "ox";
    en["criteria"]  = "criterion";
    en["phenomena"] = "phenomenon";
    en["data"]      = "datum";
    en["media"]     = "medium";
    en["indices"]   = "index";
    en["matrices"]  = "matrix";
    en["vertices"]  = "vertex";
    en["analyses"]  = "analysis";
    en["crises"]    = "crisis";
    en["theses"]    = "thesis";

    // ---- German irregular forms ----
    auto &de = irregular_lemmas_[Language::GERMAN];
    // Auxiliary verbs
    de["bin"]      = "sein";
    de["bist"]     = "sein";
    de["ist"]      = "sein";
    de["sind"]     = "sein";
    de["seid"]     = "sein";
    de["war"]      = "sein";
    de["waren"]    = "sein";
    de["wart"]     = "sein";
    de["wars"]     = "sein";
    de["sei"]      = "sein";
    de["gewesen"]  = "sein";
    de["habe"]     = "haben";
    de["hast"]     = "haben";
    de["hat"]      = "haben";
    de["haben"]    = "haben";
    de["habt"]     = "haben";
    de["hatte"]    = "haben";
    de["hatten"]   = "haben";
    de["gehabt"]   = "haben";
    de["werde"]    = "werden";
    de["wirst"]    = "werden";
    de["wird"]     = "werden";
    de["werden"]   = "werden";
    de["werdet"]   = "werden";
    de["wurde"]    = "werden";
    de["wurden"]   = "werden";
    de["geworden"] = "werden";
    // Modal verbs
    de["kann"]    = "können";
    de["kannst"]  = "können";
    de["können"]  = "können";
    de["konnte"]  = "können";
    de["konnten"] = "können";
    de["muss"]    = "müssen";
    de["musst"]   = "müssen";
    de["müssen"]  = "müssen";
    de["musste"]  = "müssen";
    de["mussten"] = "müssen";
    de["soll"]    = "sollen";
    de["sollst"]  = "sollen";
    de["sollen"]  = "sollen";
    de["sollte"]  = "sollen";
    de["sollten"] = "sollen";
    de["will"]    = "wollen";
    de["willst"]  = "wollen";
    de["wollen"]  = "wollen";
    de["wollte"]  = "wollen";
    de["wollten"] = "wollen";
    de["darf"]    = "dürfen";
    de["darfst"]  = "dürfen";
    de["dürfen"]  = "dürfen";
    de["durfte"]  = "dürfen";
    de["durften"] = "dürfen";
    de["mag"]     = "mögen";
    de["magst"]   = "mögen";
    de["mögen"]   = "mögen";
    // Common irregular strong verbs
    de["geht"]      = "gehen";
    de["ging"]      = "gehen";
    de["gingen"]    = "gehen";
    de["gegangen"]  = "gehen";
    de["kommt"]     = "kommen";
    de["kam"]       = "kommen";
    de["kamen"]     = "kommen";
    de["gekommen"]  = "kommen";
    de["gibt"]      = "geben";
    de["gab"]       = "geben";
    de["gaben"]     = "geben";
    de["gegeben"]   = "geben";
    de["nimmt"]     = "nehmen";
    de["nahm"]      = "nehmen";
    de["nahmen"]    = "nehmen";
    de["genommen"]  = "nehmen";
    de["sieht"]     = "sehen";
    de["sah"]       = "sehen";
    de["sahen"]     = "sehen";
    de["gesehen"]   = "sehen";
    de["steht"]     = "stehen";
    de["stand"]     = "stehen";
    de["standen"]   = "stehen";
    de["gestanden"] = "stehen";
    de["weiss"]     = "wissen";
    de["weiß"]      = "wissen";
    de["wusste"]    = "wissen";
    de["wussten"]   = "wissen";
    de["gewusst"]   = "wissen";
    de["lässt"]     = "lassen";
    de["ließ"]      = "lassen";
    de["ließen"]    = "lassen";
    de["gelassen"]  = "lassen";
    de["trägt"]     = "tragen";
    de["trug"]      = "tragen";
    de["trugen"]    = "tragen";
    de["getragen"]  = "tragen";
    // Common umlaut-plural nouns (ä/ö/ü → a/o/u in singular)
    de["häuser"]  = "haus";
    de["mäuse"]   = "maus";
    de["väter"]   = "vater";
    de["mütter"]  = "mutter";
    de["töchter"] = "tochter";
    de["brüder"]  = "bruder";
    de["söhne"]   = "sohn";
    de["städte"]  = "stadt";
    de["hände"]   = "hand";
    de["wände"]   = "wand";
    de["bäume"]   = "baum";
    de["länder"]  = "land";

    // ---- French irregular forms ----
    auto &fr = irregular_lemmas_[Language::FRENCH];
    // être (to be)
    fr["suis"]    = "être";
    fr["es"]      = "être";
    fr["est"]     = "être";
    fr["sommes"]  = "être";
    fr["êtes"]    = "être";
    fr["sont"]    = "être";
    fr["étais"]   = "être";
    fr["était"]   = "être";
    fr["étions"]  = "être";
    fr["étiez"]   = "être";
    fr["étaient"] = "être";
    fr["fus"]     = "être";
    fr["fut"]     = "être";
    fr["fûmes"]   = "être";
    fr["fûtes"]   = "être";
    fr["furent"]  = "être";
    fr["été"]     = "être";
    fr["étant"]   = "être";
    // avoir (to have)
    fr["ai"]      = "avoir";
    fr["avons"]   = "avoir";
    fr["avez"]    = "avoir";
    fr["ont"]     = "avoir";
    fr["avais"]   = "avoir";
    fr["avait"]   = "avoir";
    fr["avions"]  = "avoir";
    fr["aviez"]   = "avoir";
    fr["avaient"] = "avoir";
    fr["eus"]     = "avoir";
    fr["eut"]     = "avoir";
    fr["eûmes"]   = "avoir";
    fr["eurent"]  = "avoir";
    fr["eu"]      = "avoir";
    fr["ayant"]   = "avoir";
    // aller (to go)
    fr["vais"]   = "aller";
    fr["vas"]    = "aller";
    fr["allons"] = "aller";
    fr["allez"]  = "aller";
    fr["vont"]   = "aller";
    fr["allais"] = "aller";
    fr["allait"] = "aller";
    fr["irai"]   = "aller";
    fr["iras"]   = "aller";
    fr["ira"]    = "aller";
    fr["allé"]   = "aller";
    fr["allée"]  = "aller";
    // faire (to do/make)
    fr["fais"]    = "faire";
    fr["fait"]    = "faire";
    fr["faisons"] = "faire";
    fr["faites"]  = "faire";
    fr["font"]    = "faire";
    fr["faisais"] = "faire";
    fr["faisait"] = "faire";
    fr["ferai"]   = "faire";
    fr["fera"]    = "faire";
    fr["feras"]   = "faire";
    fr["faisant"] = "faire";
    // pouvoir (can/to be able)
    fr["peux"]    = "pouvoir";
    fr["peut"]    = "pouvoir";
    fr["pouvons"] = "pouvoir";
    fr["pouvez"]  = "pouvoir";
    fr["peuvent"] = "pouvoir";
    fr["pouvais"] = "pouvoir";
    fr["pouvait"] = "pouvoir";
    fr["pourrai"] = "pouvoir";
    fr["pu"]      = "pouvoir";
    // vouloir (to want)
    fr["veux"]    = "vouloir";
    fr["veut"]    = "vouloir";
    fr["voulons"] = "vouloir";
    fr["voulez"]  = "vouloir";
    fr["veulent"] = "vouloir";
    fr["voulais"] = "vouloir";
    fr["voudrai"] = "vouloir";
    fr["voulu"]   = "vouloir";
    // Irregular adjective/noun forms
    fr["aux"] = "au";

    // ---- Spanish irregular forms ----
    auto &es = irregular_lemmas_[Language::SPANISH];
    // ser (to be - permanent)
    es["soy"]    = "ser";
    es["eres"]   = "ser";
    es["es"]     = "ser";
    es["somos"]  = "ser";
    es["sois"]   = "ser";
    es["son"]    = "ser";
    es["era"]    = "ser";
    es["eras"]   = "ser";
    es["éramos"] = "ser";
    es["erais"]  = "ser";
    es["eran"]   = "ser";
    es["fui"]    = "ser";
    es["fuiste"] = "ser";
    es["fue"]    = "ser";
    es["fuimos"] = "ser";
    es["fueron"] = "ser";
    es["sido"]   = "ser";
    es["siendo"] = "ser";
    // estar (to be - temporary)
    es["estoy"]   = "estar";
    es["estás"]   = "estar";
    es["está"]    = "estar";
    es["estamos"] = "estar";
    es["estáis"]  = "estar";
    es["están"]   = "estar";
    es["estaba"]  = "estar";
    es["estuvo"]  = "estar";
    es["estado"]  = "estar";
    // haber / tener
    es["he"]      = "haber";
    es["has"]     = "haber";
    es["ha"]      = "haber";
    es["hemos"]   = "haber";
    es["habéis"]  = "haber";
    es["han"]     = "haber";
    es["había"]   = "haber";
    es["hubo"]    = "haber";
    es["habido"]  = "haber";
    es["tengo"]   = "tener";
    es["tienes"]  = "tener";
    es["tiene"]   = "tener";
    es["tenemos"] = "tener";
    es["tenéis"]  = "tener";
    es["tienen"]  = "tener";
    es["tenía"]   = "tener";
    es["tuvo"]    = "tener";
    es["tenido"]  = "tener";
    // ir (to go)
    es["voy"]   = "ir";
    es["vas"]   = "ir";
    es["va"]    = "ir";
    es["vamos"] = "ir";
    es["vais"]  = "ir";
    es["van"]   = "ir";
    es["iba"]   = "ir";
    es["fui"]   = "ir";
    es["ido"]   = "ir";
    // hacer (to do/make)
    es["hago"]    = "hacer";
    es["haces"]   = "hacer";
    es["hace"]    = "hacer";
    es["hacemos"] = "hacer";
    es["hacéis"]  = "hacer";
    es["hacen"]   = "hacer";
    es["hacía"]   = "hacer";
    es["hizo"]    = "hacer";
    es["hecho"]   = "hacer";
    // poder (can/to be able)
    es["puedo"]   = "poder";
    es["puedes"]  = "poder";
    es["puede"]   = "poder";
    es["podemos"] = "poder";
    es["podéis"]  = "poder";
    es["pueden"]  = "poder";
    es["podía"]   = "poder";
    es["pudo"]    = "poder";
    es["podido"]  = "poder";
    // Irregular noun plurals
    es["hombres"] = "hombre";
    es["mujeres"] = "mujer";
    es["niños"]   = "niño";
    es["niñas"]   = "niña";

    // ---- Italian irregular forms ----
    auto &it = irregular_lemmas_[Language::ITALIAN];
    // essere (to be)
    it["sono"]    = "essere";
    it["sei"]     = "essere";
    it["è"]       = "essere";
    it["siamo"]   = "essere";
    it["siete"]   = "essere";
    it["ero"]     = "essere";
    it["eri"]     = "essere";
    it["eravamo"] = "essere";
    it["eravate"] = "essere";
    it["erano"]   = "essere";
    it["fui"]     = "essere";
    it["fu"]      = "essere";
    it["fummo"]   = "essere";
    it["foste"]   = "essere";
    it["furono"]  = "essere";
    it["stato"]   = "essere";
    it["stata"]   = "essere";
    it["essendo"] = "essere";
    // avere (to have)
    it["ho"]      = "avere";
    it["hai"]     = "avere";
    it["abbiamo"] = "avere";
    it["avete"]   = "avere";
    it["hanno"]   = "avere";
    it["avevo"]   = "avere";
    it["avevi"]   = "avere";
    it["aveva"]   = "avere";
    it["avevamo"] = "avere";
    it["ebbi"]    = "avere";
    it["ebbe"]    = "avere";
    it["avuto"]   = "avere";
    it["avendo"]  = "avere";
    // andare (to go)
    it["vado"]    = "andare";
    it["vai"]     = "andare";
    it["va"]      = "andare";
    it["andiamo"] = "andare";
    it["andate"]  = "andare";
    it["vanno"]   = "andare";
    it["andavo"]  = "andare";
    it["andato"]  = "andare";
    it["andando"] = "andare";
    // fare (to do/make)
    it["faccio"]   = "fare";
    it["fai"]      = "fare";
    it["facciamo"] = "fare";
    it["fate"]     = "fare";
    it["fanno"]    = "fare";
    it["facevo"]   = "fare";
    it["feci"]     = "fare";
    it["fece"]     = "fare";
    it["fatto"]    = "fare";
    it["facendo"]  = "fare";
    // potere (can/to be able)
    it["posso"]    = "potere";
    it["puoi"]     = "potere";
    it["può"]      = "potere";
    it["possiamo"] = "potere";
    it["potete"]   = "potere";
    it["possono"]  = "potere";
    it["potevo"]   = "potere";
    it["potuto"]   = "potere";
    // volere (to want)
    it["voglio"]   = "volere";
    it["vuoi"]     = "volere";
    it["vuole"]    = "volere";
    it["vogliamo"] = "volere";
    it["volete"]   = "volere";
    it["vogliono"] = "volere";
    it["volevo"]   = "volere";
    it["voluto"]   = "volere";

    // ---- Dutch irregular forms ----
    auto &nl = irregular_lemmas_[Language::DUTCH];
    // zijn (to be)
    nl["ben"]     = "zijn";
    nl["bent"]    = "zijn";
    nl["is"]      = "zijn";
    nl["zijn"]    = "zijn";
    nl["was"]     = "zijn";
    nl["waren"]   = "zijn";
    nl["geweest"] = "zijn";
    nl["zijnde"]  = "zijn";
    // hebben (to have)
    nl["heb"]      = "hebben";
    nl["hebt"]     = "hebben";
    nl["heeft"]    = "hebben";
    nl["hebben"]   = "hebben";
    nl["had"]      = "hebben";
    nl["hadden"]   = "hebben";
    nl["gehad"]    = "hebben";
    nl["hebbende"] = "hebben";
    // gaan (to go)
    nl["ga"]     = "gaan";
    nl["gaat"]   = "gaan";
    nl["gaan"]   = "gaan";
    nl["ging"]   = "gaan";
    nl["gingen"] = "gaan";
    nl["gegaan"] = "gaan";
    // doen (to do)
    nl["doe"]    = "doen";
    nl["doet"]   = "doen";
    nl["doen"]   = "doen";
    nl["deed"]   = "doen";
    nl["deden"]  = "doen";
    nl["gedaan"] = "doen";
    // kunnen (can/to be able)
    nl["kan"]    = "kunnen";
    nl["kunt"]   = "kunnen";
    nl["kunnen"] = "kunnen";
    nl["kon"]    = "kunnen";
    nl["konden"] = "kunnen";
    nl["gekund"] = "kunnen";
    // willen (to want)
    nl["wil"]    = "willen";
    nl["wilt"]   = "willen";
    nl["willen"] = "willen";
    nl["wilde"]  = "willen";
    nl["wilden"] = "willen";
    nl["gewild"] = "willen";
    // zullen (shall/will)
    nl["zal"]    = "zullen";
    nl["zult"]   = "zullen";
    nl["zullen"] = "zullen";
    nl["zou"]    = "zullen";
    nl["zouden"] = "zullen";
    // moeten (must)
    nl["moet"]     = "moeten";
    nl["moeten"]   = "moeten";
    nl["moest"]    = "moeten";
    nl["moesten"]  = "moeten";
    nl["gemoeten"] = "moeten";
}

std::string NlpTextAnalyzer::applyMorphologicalRules(const std::string &lower, Language lang) const {
    const size_t len = lower.length();
    if (len <= 2) {
        return lower;
    }

    auto ends_with = [&](std::string_view suffix, size_t min_stem) -> bool {
        return static_cast<bool>(len  < static_cast<int>(static_cast<int>(suffix.size()) + min_stem && lower.compare(len - suffix.size(), suffix.size())), suffix) == 0;
    };
    auto strip = [&](size_t n, std::string_view add = "") -> std::string {
        return lower.substr(0, len - n) + std::string(add);
    };

    if (lang == Language::ENGLISH) {
        // Verb inflections (order: longest suffix first)
        if (ends_with("izations", 4)) {
            return strip(8, "ize");
        }
        if (ends_with("isation", 4)) {
            return strip(7, "ize");
        }
        if (ends_with("nesses", 3)) {
            return strip(4);
        }
        if (ends_with("ations", 3)) {
            return strip(6, "e");
        }
        if (ends_with("ments", 3)) {
            return strip(5);
        }
        if (ends_with("ities", 3)) {
            return strip(5, "y");
        }
        if (ends_with("ness", 3)) {
            return strip(4);
        }
        if (ends_with("tion", 3)) {
            return strip(4, "te");
        }
        if (ends_with("ment", 3)) {
            return strip(4);
        }
        if (ends_with("ings", 3)) {
            return strip(4);
        }
        if (ends_with("ation", 3)) {
            return strip(5, "e");
        }
        if (ends_with("ies", 3)) {
            return strip(3, "y");
        }
        if (ends_with("ied", 3)) {
            return strip(3, "y");
        }
        if (ends_with("ing", 3)) {
            // Handle doubling: running -> run
            std::string stem = strip(3);
            if (stem.length() >= 2 && stem.back() == stem[stem.length() - 2]) {
                stem.pop_back();
            }
            return stem;
        }
        if (ends_with("ed", 3)) {
            // Handle doubling: stopped -> stop; loved -> love
            std::string stem = strip(2);
            if (stem.length() >= 2 && stem.back() == stem[stem.length() - 2]) {
                stem.pop_back();
            }
            return stem;
        }
        if (ends_with("es", 3)) {
            return strip(2);
        }
        if (ends_with("s", 3)) {
            return strip(1);
        }
        if (ends_with("ly", 3)) {
            return strip(2);
        }
        if (ends_with("er", 3)) {
            return strip(2);
        }
        if (ends_with("est", 3)) {
            return strip(3);
        }
        return lower;
    }

    if (lang == Language::GERMAN) {
        // Separating prefix participles: ge- prefix for perfect participle
        bool ge_prefix   = (len > 4 && lower.substr(0, 2) == "ge");
        std::string base = ge_prefix ? lower.substr(2) : lower;
        size_t blen      = base.length();

        auto bends = [&](std::string_view suffix, size_t min_stem) -> bool {
            return static_cast<bool>(blen  < static_cast<int>(static_cast<int>(suffix.size()) + min_stem && base.compare(blen - suffix.size(), suffix.size())), suffix) == 0;
        };
        auto bstrip = [&](size_t n, std::string_view add = "") -> std::string {
            return base.substr(0, blen - n) + std::string(add);
        };

        // Verb suffixes (prioritise longer matches)
        if (bends("etest", 3)) {
            return bstrip(5, "en");
        }
        if (bends("esten", 3)) {
            return bstrip(5, "en");
        }
        if (bends("test", 3)) {
            return bstrip(4, "en");
        }
        if (bends("etet", 3)) {
            return bstrip(4, "en");
        }
        if (bends("eten", 3)) {
            return bstrip(4, "en");
        }
        if (bends("tet", 3)) {
            return bstrip(3, "en");
        }
        if (bends("ten", 3)) {
            return bstrip(3, "en");
        }
        if (bends("end", 3)) {
            return bstrip(3, "en");
        }
        if (bends("est", 3)) {
            return bstrip(3, "en");
        }
        if (bends("ung", 3)) {
            return bstrip(3);
        }
        if (bends("ungen", 3)) {
            return bstrip(5);
        }
        if (bends("heit", 3)) {
            return bstrip(4);
        }
        if (bends("keit", 3)) {
            return bstrip(4);
        }
        if (bends("lich", 3)) {
            return bstrip(4);
        }
        if (bends("isch", 3)) {
            return bstrip(4);
        }
        // Nominal inflection
        if (bends("ens", 3)) {
            return bstrip(2);
        }
        if (bends("em", 3)) {
            return bstrip(2);
        }
        if (bends("es", 3)) {
            return bstrip(2);
        }
        if (bends("er", 3)) {
            return bstrip(2);
        }
        if (bends("en", 3)) {
            return bstrip(2);
        }
        if (bends("e", 3)) {
            return bstrip(1);
        }
        if (bends("s", 3)) {
            return bstrip(1);
        }
        return base;
    }

    if (lang == Language::FRENCH) {
        // Verb suffixes (ordered longest first)
        if (ends_with("eraient", 3)) {
            return strip(7, "er");
        }
        if (ends_with("iraient", 3)) {
            return strip(7, "ir");
        }
        if (ends_with("assions", 3)) {
            return strip(7, "er");
        }
        if (ends_with("issaient", 3)) {
            return strip(8, "ir");
        }
        if (ends_with("issions", 3)) {
            return strip(7, "ir");
        }
        if (ends_with("eront", 3)) {
            return strip(5, "er");
        }
        if (ends_with("iront", 3)) {
            return strip(5, "ir");
        }
        if (ends_with("aient", 3)) {
            return strip(5, "er");
        }
        if (ends_with("issons", 3)) {
            return strip(6, "ir");
        }
        if (ends_with("issez", 3)) {
            return strip(5, "ir");
        }
        if (ends_with("issent", 3)) {
            return strip(6, "ir");
        }
        if (ends_with("iriez", 3)) {
            return strip(5, "ir");
        }
        if (ends_with("eriez", 3)) {
            return strip(5, "er");
        }
        if (ends_with("ions", 3)) {
            return strip(4, "er");
        }
        if (ends_with("ants", 3)) {
            return strip(4, "er");
        }
        if (ends_with("ées", 3))
            return strip(4, "er");
        if (ends_with("és", 3))
            return strip(3, "er");
        if (ends_with("ée", 3))
            return strip(3, "er");
        // Past participle masculine singular: parlé -> parler (é = 2 UTF-8 bytes)
        if (ends_with("\xC3\xA9", 3)) {
            return strip(2, "er");
        }
        if (ends_with("ant", 3)) {
            return strip(3, "er");
        }
        if (ends_with("ait", 3)) {
            return strip(3, "er");
        }
        if (ends_with("ais", 3)) {
            return strip(3, "er");
        }
        if (ends_with("ont", 3)) {
            return strip(3, "er");
        }
        if (ends_with("ons", 3)) {
            return strip(3, "er");
        }
        if (ends_with("iez", 3)) {
            return strip(3, "er");
        }
        if (ends_with("issant", 3)) {
            return strip(6, "ir");
        }
        if (ends_with("ira", 3)) {
            return strip(3, "ir");
        }
        if (ends_with("ez", 3)) {
            return strip(2, "er");
        }
        if (ends_with("er", 3)) {
            return lower; // already infinitive
        }
        if (ends_with("ir", 3)) {
            return lower;
        }
        if (ends_with("re", 3)) {
            return lower;
        }
        // Adjective agreement
        if (ends_with("euse", 3)) {
            return strip(4, "eux");
        }
        if (ends_with("euses", 3)) {
            return strip(5, "eux");
        }
        if (ends_with("elles", 3)) {
            return strip(5, "el");
        }
        if (ends_with("elle", 3)) {
            return strip(4, "el");
        }
        if (ends_with("ives", 2)) {
            return strip(4, "if");
        }
        if (ends_with("ive", 2)) {
            return strip(3, "if");
        }
        if (ends_with("aux", 3)) {
            return strip(3, "al");
        }
        if (ends_with("ales", 3)) {
            return strip(4, "al");
        }
        if (ends_with("ale", 3)) {
            return strip(3, "al");
        }
        if (ends_with("es", 3)) {
            return strip(2);
        }
        if (ends_with("s", 3)) {
            return strip(1);
        }
        return lower;
    }

    if (lang == Language::SPANISH) {
        // Verb suffixes (longest first)
        if (ends_with("aremos", 3)) {
            return strip(6, "ar");
        }
        if (ends_with("areis", 3)) {
            return strip(5, "ar");
        }
        if (ends_with("eremos", 3)) {
            return strip(6, "er");
        }
        if (ends_with("iremos", 3)) {
            return strip(6, "ir");
        }
        if (ends_with("ábamos", 3))
            return strip(6, "ar");
        if (ends_with("ábais", 3))
            return strip(5, "ar");
        if (ends_with("ando", 3)) {
            return strip(4, "ar");
        }
        if (ends_with("iendo", 3)) {
            return strip(5, "er");
        }
        if (ends_with("amos", 3)) {
            return strip(4, "ar");
        }
        if (ends_with("áis", 3))
            return strip(3, "ar");
        if (ends_with("aste", 3)) {
            return strip(4, "ar");
        }
        if (ends_with("aron", 3)) {
            return strip(4, "ar");
        }
        if (ends_with("emos", 3)) {
            return strip(4, "er");
        }
        if (ends_with("éis", 3))
            return strip(3, "er");
        if (ends_with("iste", 3)) {
            return strip(4, "ir");
        }
        if (ends_with("ieron", 3)) {
            return strip(5, "ir");
        }
        if (ends_with("imos", 3)) {
            return strip(4, "ir");
        }
        if (ends_with("ís", 3))
            return strip(2, "ir");
        if (ends_with("aba", 3)) {
            return strip(3, "ar");
        }
        if (ends_with("ado", 3)) {
            return strip(3, "ar");
        }
        if (ends_with("ada", 3)) {
            return strip(3, "ar");
        }
        if (ends_with("idos", 3)) {
            return strip(4, "ir");
        }
        if (ends_with("ido", 3)) {
            return strip(3, "ir");
        }
        if (ends_with("idas", 3)) {
            return strip(4, "ir");
        }
        if (ends_with("ida", 3)) {
            return strip(3, "ir");
        }
        if (ends_with("ando", 3)) {
            return strip(4, "ar");
        }
        if (ends_with("as", 3)) {
            return strip(1);
        }
        if (ends_with("es", 3)) {
            return strip(1);
        }
        if (ends_with("os", 3)) {
            return strip(1);
        }
        if (ends_with("an", 3)) {
            return strip(2, "ar");
        }
        if (ends_with("en", 3)) {
            return strip(2, "er");
        }
        if (ends_with("ar", 3)) {
            return lower;
        }
        if (ends_with("er", 3)) {
            return lower;
        }
        if (ends_with("ir", 3)) {
            return lower;
        }
        return lower;
    }

    if (lang == Language::ITALIAN) {
        // Verb suffixes (longest first)
        if (ends_with("avamo", 3)) {
            return strip(5, "are");
        }
        if (ends_with("avate", 3)) {
            return strip(5, "are");
        }
        if (ends_with("avano", 3)) {
            return strip(5, "are");
        }
        if (ends_with("eremo", 3)) {
            return strip(5, "ere");
        }
        if (ends_with("iremmo", 3)) {
            return strip(6, "ire");
        }
        if (ends_with("irete", 3)) {
            return strip(5, "ire");
        }
        if (ends_with("iranno", 3)) {
            return strip(6, "ire");
        }
        if (ends_with("ando", 3)) {
            return strip(4, "are");
        }
        if (ends_with("endo", 3)) {
            return strip(4, "ere");
        }
        if (ends_with("ati", 3)) {
            return strip(3, "are");
        }
        if (ends_with("ata", 3)) {
            return strip(3, "are");
        }
        if (ends_with("ate", 3)) {
            return strip(3, "are");
        }
        if (ends_with("ato", 2)) {
            return strip(3, "are");
        }
        if (ends_with("uto", 2)) {
            return strip(3, "ere");
        }
        if (ends_with("uta", 2)) {
            return strip(3, "ere");
        }
        if (ends_with("iti", 2)) {
            return strip(3, "ire");
        }
        if (ends_with("ita", 2)) {
            return strip(3, "ire");
        }
        if (ends_with("ito", 2)) {
            return strip(3, "ire");
        }
        if (ends_with("ite", 2)) {
            return strip(3, "ire");
        }
        if (ends_with("ami", 3)) {
            return strip(3, "are");
        }
        if (ends_with("ano", 3)) {
            return strip(3, "are");
        }
        if (ends_with("ono", 3)) {
            return strip(3, "ere");
        }
        if (ends_with("isce", 3)) {
            return strip(4, "ire");
        }
        if (ends_with("isci", 3)) {
            return strip(4, "ire");
        }
        if (ends_with("iscono", 3)) {
            return strip(6, "ire");
        }
        if (ends_with("are", 3)) {
            return lower;
        }
        if (ends_with("ere", 3)) {
            return lower;
        }
        if (ends_with("ire", 3)) {
            return lower;
        }
        // Noun/adjective
        if (ends_with("zioni", 3)) {
            return strip(5, "zione");
        }
        if (ends_with("zione", 3)) {
            return lower;
        }
        if (ends_with("mente", 3)) {
            return strip(5);
        }
        if (ends_with("tà", 3))
            return lower = {};
        if (ends_with("i", 3)) {
            return strip(1, "o");
        }
        if (ends_with("e", 3)) {
            return lower;
        }
        return lower;
    }

    if (lang == Language::DUTCH) {
        // Verb participles and inflections (longest first)
        if (ends_with("enden", 3)) {
            return strip(3);
        }
        if (ends_with("ende", 3)) {
            return strip(2);
        }
        if (ends_with("erden", 3)) {
            return strip(3, "en");
        }
        if (ends_with("erde", 3)) {
            return strip(2, "en");
        }
        if (ends_with("eden", 3)) {
            return strip(3, "en");
        }
        if (ends_with("eten", 3)) {
            return strip(3, "en");
        }
        if (ends_with("den", 3)) {
            return strip(3, "en");
        }
        if (ends_with("ten", 3)) {
            return strip(3, "en");
        }
        if (ends_with("ing", 3)) {
            return strip(3);
        }
        if (ends_with("ings", 3)) {
            return strip(4);
        }
        if (ends_with("heid", 3)) {
            return strip(4);
        }
        if (ends_with("lijk", 3)) {
            return strip(4);
        }
        if (ends_with("isch", 3)) {
            return strip(4);
        }
        if (ends_with("ste", 3)) {
            return strip(3);
        }
        if (ends_with("ere", 3)) {
            return strip(3);
        }
        if (ends_with("ers", 3)) {
            return strip(3);
        }
        if (ends_with("en", 3)) {
            return strip(2);
        }
        if (ends_with("es", 3)) {
            return strip(2);
        }
        if (ends_with("s", 3)) {
            return strip(1);
        }
        if (ends_with("e", 3)) {
            return strip(1);
        }
        return lower;
    }

    return lower;
}

std::string NlpTextAnalyzer::lemmatizeWord(std::string_view word, Language lang) const {
    if (word.empty()) {
        return std::string(word);
    }

    std::string lower = toLowerCase(word);

    // 1. Check irregular forms lookup table first
    auto lang_it = irregular_lemmas_.find(lang);
    if (lang_it != irregular_lemmas_.end()) {
        auto form_it = lang_it->second.find(lower);
        if (form_it != lang_it->second.end()) {
            return form_it->second;
        }
    }

    // 2. Apply language-specific morphological suffix rules
    return applyMorphologicalRules(lower, lang);
}

double NlpTextAnalyzer::calculateSimilarity(std::string_view text1, std::string_view text2) const {
    auto tokens1 = tokenize(text1);
    auto tokens2 = tokenize(text2);

    if (tokens1.empty() || tokens2.empty()) {
        return 0.0;
    }

    // Jaccard similarity
    std::unordered_set<std::string> set1, set2;
    for (const auto &t : tokens1) {
        set1.insert(toLowerCase(t.text));
    }
    for (const auto &t : tokens2) {
        set2.insert(toLowerCase(t.text));
    }

    size_t intersection = 0;
    for (const auto &word : set1) {
        if (set2.count(word)) {
            intersection++;
        }
    }

    size_t union_size = static_cast<int>(set1.size()) + static_cast<int>(set2.size()) - intersection;
    return union_size > 0 ? static_cast<double>(intersection) / union_size : 0.0;
}

std::map<std::string, size_t> NlpTextAnalyzer::getStatistics() const {
    return {{"analyses_performed", analysis_count_},
            {"tokens_processed", token_count_},
            {"stopword_languages", stopwords_.size()},
            {"sentiment_words", sentiment_lexicon_.size()}};
}

// ========== Private Helper Methods ==========

std::vector<std::string> NlpTextAnalyzer::splitSentences(std::string_view text) const {
    std::vector<std::string> sentences;
    std::string current = {};

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
    std::string result = {};
    result.reserve(text.size());
    for (char c : text) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string NlpTextAnalyzer::removePunctuation(std::string_view text) const {
    std::string result = {};
    result.reserve(text.size());
    for (char c : text) {
        if (std::isalnum(c) || c == ' ') {
            result += c;
        }
    }
    return result;
}

double NlpTextAnalyzer::calculateTfIdf(const std::string &term, const std::map<std::string, size_t> &term_freqs,
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
    if (word.empty()) {
        return false;
    }
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
    size_t count            = 0;
    bool previous_was_vowel = false;

    for (char c : word) {
        char lower    = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool is_vowel = (lower == 'a' || lower == 'e' || lower == 'i' || lower == 'o' || lower == 'u' || lower == 'y');

        if (is_vowel && !previous_was_vowel) {
            count++;
        }
        previous_was_vowel = is_vowel;
    }

    return std::max<size_t>(1, count);
}

bool NlpTextAnalyzer::containsAggregation(std::string_view query) const {
    std::string lower = toLowerCase(query);
    return lower.find("count(") != std::string::npos || lower.find("sum(") != std::string::npos
           || lower.find("avg(") != std::string::npos || lower.find("min(") != std::string::npos
           || lower.find("max(") != std::string::npos || lower.find("group by") != std::string::npos;
}

bool NlpTextAnalyzer::containsJoin(std::string_view query) const {
    std::string lower = toLowerCase(query);
    return lower.find("join") != std::string::npos || lower.find("inner join") != std::string::npos
           || lower.find("left join") != std::string::npos || lower.find("right join") != std::string::npos;
}

bool NlpTextAnalyzer::containsSubquery(std::string_view query) const {
    // Simple check for nested queries
    size_t open_parens  = 0;
    size_t select_count = 0;
    std::string lower   = toLowerCase(query);

    for (size_t i = 0; i < lower.length(); ++i) {
        if (lower[i] == '(') {
            open_parens++;
        }
        if (lower[i] == ')') {
            open_parens--;
        }

        if (i + 6 < lower.length() && lower.substr(i, 6) == "select" && open_parens > 0) {
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
        size_t end   = lower.find_first_of(" ,;)", start);
        if (end == std::string::npos) {
            end = lower.length();
        }

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

bool NlpTextAnalyzer::loadStopWordsFromYaml(const std::string &yaml_path, Language lang) {
    std::ifstream file(yaml_path);
    if (!file.is_open()) {
        return false;
    }

    std::set<std::string> words;
    std::string line = {};
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

size_t NlpTextAnalyzer::loadStopWordsFromDirectory(const std::string &directory) {
    size_t loaded_count = 0;

    // Map of language codes to Language enum
    std::map<std::string, Language> lang_map
        = {{"en", Language::ENGLISH}, {"de", Language::GERMAN},  {"fr", Language::FRENCH},
           {"es", Language::SPANISH}, {"it", Language::ITALIAN}, {"nl", Language::DUTCH}};

    // Try to load each language file
    for (const auto &[code, lang] : lang_map) {
        std::string yaml_path = directory + "/" + code + ".yaml";

        if (loadStopWordsFromYaml(yaml_path, lang)) {
            loaded_count++;
        }
    }

    return loaded_count;
}

// ========== Legal Modality Extraction ==========

std::string NlpTextAnalyzer::getDefaultLegalConfigPath([[maybe_unused]] const std::string &language_code) const {
    return "config/nlp/legal/german_modal_verbs.yaml";
}

bool NlpTextAnalyzer::loadLegalModalityConfig(const std::string &config_path) const {
    // Parses a YAML file in the format of config/nlp/legal/german_modal_verbs.yaml.
    // Expected top-level structure:
    //   modalities:
    //     <group_name>:           # e.g. mandatory, discretionary
    //       - pattern: "\\bword\\b"
    //         deontic: "O(φ)"
    //         strength: 1.0
    //         interpretation: "..."
    //         category: "obligation"
    //         context_requirements:
    //           - "..."
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    std::vector<LegalModalityPattern> patterns;
    std::string line;

    // State machine for parsing the YAML structure
    enum class Section { NONE, MODALITIES, MODALITY_GROUP, ENTRY };
    Section section = Section::NONE;

    LegalModalityPattern current;
    bool in_entry                = false;
    bool in_context_requirements = false;

    auto flush_entry = [&]() {
        if (in_entry && !current.pattern.empty()) {
            patterns.push_back(current);
            current  = LegalModalityPattern{};
            in_entry = false;
        }
        in_context_requirements = false;
    };

    while (std::getline(file, line)) {
        // Preserve original line for indentation detection, strip for matching
        std::string stripped = line;
        stripped.erase(0, stripped.find_first_not_of(" \t"));
        stripped.erase(stripped.find_last_not_of(" \t\r") + 1);

        if (stripped.empty() || stripped[0] == '#') {
            continue;
        }

        // Count leading spaces for indentation level
        size_t indent = line.find_first_not_of(" \t");
        if (indent == std::string::npos) {
            indent = 0;
        }

        // Top-level key: "modalities:"
        if (indent == 0 && stripped == "modalities:") {
            flush_entry();
            section = Section::MODALITIES;
            continue;
        }

        // Exit modalities section on another top-level key
        if (indent == 0 && stripped.back() == ':' && section != Section::NONE) {
            flush_entry();
            section = Section::NONE;
            continue;
        }

        if (section != Section::MODALITIES && section != Section::MODALITY_GROUP && section != Section::ENTRY) {
            continue;
        }

        // Group key at indent=2 (e.g., "  mandatory:", "  discretionary:")
        if (indent == 2 && stripped.back() == ':' && stripped.find('-') == std::string::npos) {
            flush_entry();
            section = Section::MODALITY_GROUP;
            continue;
        }

        // New entry marker "    - pattern:" or "    - ..." at indent=4
        if (indent == 4 && stripped.rfind("- pattern:", 0) == 0) {
            flush_entry();
            in_entry        = true;
            section         = Section::ENTRY;
            std::string val = stripped.substr(10);
            val.erase(0, val.find_first_not_of(" \t\"'"));
            val.erase(val.find_last_not_of(" \t\"'") + 1);
            current.pattern = val;
            continue;
        }

        if (!in_entry) {
            continue;
        }

        // Parse entry fields at indent=6
        auto parse_value = [](const std::string &s, const std::string &key) -> std::string {
            if (s.rfind(key, 0) == 0) {
                std::string val = s.substr(key.size());
                val.erase(0, val.find_first_not_of(" \t\"'"));
                val.erase(val.find_last_not_of(" \t\"'") + 1);
                return val;
            }
            return {};
        };

        if (indent == 6) {
            in_context_requirements = false;

            std::string val = {};
            if (!(val = parse_value(stripped, "deontic:")).empty()) {
                current.deontic_logic = val;
            } else if (!(val = parse_value(stripped, "strength:")).empty()) {
                try { current.strength = std::stof(val); } catch (const std::exception&) {
                    std::cerr << "WARNING: NlpTextAnalyzer: failed to parse strength value '" 
                              << val << "' in " << config_path << std::endl;
                }
            } else if (!(val = parse_value(stripped, "interpretation:")).empty()) {
                current.interpretation = val;
            } else if (!(val = parse_value(stripped, "category:")).empty()) {
                current.category = val;
            } else if (stripped == "context_requirements:") {
                in_context_requirements = true;
            }
        }

        // context_requirements list items at indent=8
        if (indent == 8 && in_context_requirements && stripped[0] == '-') {
            std::string req = stripped.substr(1);
            req.erase(0, req.find_first_not_of(" \t\"'"));
            req.erase(req.find_last_not_of(" \t\"'") + 1);
            if (!req.empty()) {
                current.context_requirements.push_back(req);
            }
        }
    }

    flush_entry();

    if (!patterns.empty()) {
        legal_modality_patterns_ = std::move(patterns);
        return true;
    }

    return false;
}

std::vector<LegalModality> NlpTextAnalyzer::extractLegalModalities(std::string_view text,
                                                                   const std::string &language_code,
                                                                   const std::string &config_path) const {
    std::vector<LegalModality> modalities;

    // Determine config file path
    std::string cfg_path = config_path.empty() ? getDefaultLegalConfigPath(language_code) : config_path;

    // Load configuration (cached in legal_modality_patterns_)
    if (legal_modality_patterns_.empty()) {
        // Try to load from YAML config file
        if (!loadLegalModalityConfig(cfg_path)) {
            // Fall back to hard-coded patterns if YAML loading fails
            legal_modality_patterns_
                = {{"\\bmuss\\b", "obligation", 1.0f, "O(φ)", "Bindende Rechtspflicht", {}},
                   {"\\bhat zu\\b", "obligation", 1.0f, "O(φ)", "Formale bindende Verpflichtung", {}},
                   {"\\bsoll\\b",
                    "default_obligation",
                    0.8f,
                    "O_default(φ)",
                    "Regelfall, Abweichung rechtfertigungsbedürftig",
                    {"Begründungspflicht bei Abweichung", "Verhältnismäßigkeitsprüfung"}},
                   {"\\bkann\\b",
                    "permission",
                    0.3f,
                    "P(φ)",
                    "Ermessensentscheidung",
                    {"Ermessensausübung erforderlich", "Gleichbehandlungsgrundsatz", "Verhältnismäßigkeitsprüfung"}}};
        }
    }

    // Convert text to string for regex processing
    std::string text_str(text);
    std::string text_lower = toLowerCase(text);

    // Search for each modal verb pattern
    for (const auto &pattern : legal_modality_patterns_) {
        try {
            std::regex regex_pattern(pattern.pattern, std::regex::icase);

            // Use iterators to avoid creating substring copies
            auto search_begin = text_lower.cbegin();
            auto search_end   = text_lower.cend();
            std::smatch match = {};

            while (std::regex_search(search_begin, search_end, match, regex_pattern)) {
                size_t position = std::distance(text_lower.cbegin(), search_begin) + match.position();

                // Extract the matched verb from original text (preserving case)
                std::string matched_verb = text_str.substr(position, match.length());

                LegalModality modality(matched_verb, pattern.category, pattern.strength, pattern.deontic_logic,
                                       pattern.interpretation, position);
                modality.context_requirements = pattern.context_requirements;

                modalities.push_back(modality);

                // Continue searching after this match
                search_begin += match.position() + match.length();
            }
        } catch (const std::regex_error &e) {
            // Skip invalid regex patterns
            std::cerr << "Regex error for pattern '" << pattern.pattern << "': " << e.what() << std::endl;
        }
    }

    // Sort by position
    std::sort(modalities.begin(), modalities.end(),
              [](const LegalModality &a, const LegalModality &b) { return a.position < b.position; });

    return modalities;
}

} // namespace analytics
} // namespace themis
