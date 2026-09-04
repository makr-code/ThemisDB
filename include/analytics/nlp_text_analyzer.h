/**
 * @file nlp_text_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace analytics {

/**
 * @brief Token information from text analysis
 */
struct Token {
    std::string text;           ///< The token text
    size_t position;            ///< Position in original text
    std::string lemma;          ///< Base form of the word
    std::string pos_tag;        ///< Part-of-speech tag (NOUN, VERB, etc.)
    
    Token() = default;
    Token(std::string t, size_t pos) : text(std::move(t)), position(pos) {}
};

/**
 * @brief Named entity extracted from text
 */
struct NamedEntity {
    std::string text;           ///< Entity text
    std::string type;           ///< Entity type (PERSON, ORG, LOC, etc.)
    double confidence;          ///< Confidence score [0.0, 1.0]
    size_t start_pos;           ///< Start position in text
    size_t end_pos;             ///< End position in text
    
    NamedEntity(std::string t, std::string ty, double conf = 1.0)
        : text(std::move(t)), type(std::move(ty)), confidence(conf) 
        , start_pos(0), end_pos(0) {}
};

/**
 * @brief Keyword with relevance score
 */
struct Keyword {
    std::string text;           ///< Keyword text
    double score;               ///< TF-IDF or relevance score
    size_t frequency;           ///< Occurrence count
    
    Keyword() : text(""), score(0.0), frequency(0) {}
    Keyword(std::string t, double s, size_t f = 1)
        : text(std::move(t)), score(s), frequency(f) {}
        
    bool operator<(const Keyword& other) const {
        return score > other.score; // Higher score first
    }
};

/**
 * @brief Sentiment analysis result
 */
struct SentimentResult {
    enum class Polarity {
        NEGATIVE,
        NEUTRAL,
        POSITIVE
    };
    
    Polarity polarity;          ///< Overall sentiment
    double score;               ///< Sentiment score [-1.0, 1.0]
    double confidence;          ///< Confidence in analysis [0.0, 1.0]
    
    SentimentResult() : polarity(Polarity::NEUTRAL), score(0.0), confidence(0.5) {}
};

/**
 * @brief Text complexity metrics
 */
struct ComplexityMetrics {
    size_t word_count = 0;          ///< Total words
    size_t sentence_count;      ///< Total sentences
    size_t unique_words;        ///< Unique word count
    double avg_word_length;     ///< Average word length
    double avg_sentence_length; ///< Average sentence length
    double lexical_diversity;   ///< Unique words / total words
    size_t complex_words;       ///< Words with 3+ syllables
    
    ComplexityMetrics() : word_count(0), sentence_count(0), unique_words(0)
                       , avg_word_length(0.0), avg_sentence_length(0.0)
                       , lexical_diversity(0.0), complex_words(0) {}
};

/**
 * @brief Legal modality detected in text
 * 
 * Represents modal verbs with legal/normative semantics, particularly
 * for German administrative law (Verwaltungsrecht).
 */
struct LegalModality {
    std::string verb;                   ///< Modal verb (e.g., "muss", "soll", "kann")
    std::string category;               ///< Category: "obligation", "permission", "prohibition"
    float strength;                     ///< Normative strength [0.0, 1.0]
    std::string deontic_logic;          ///< Deontic logic notation (e.g., "O(φ)")
    std::string interpretation;         ///< Legal interpretation
    size_t position;                    ///< Position in text
    std::vector<std::string> context_requirements;  ///< Required checks/considerations
    
    LegalModality() : strength(0.0f), position(0) {}
    LegalModality(std::string v, std::string c, float s, std::string d, std::string i, size_t pos)
        : verb(std::move(v)), category(std::move(c)), strength(s)
        , deontic_logic(std::move(d)), interpretation(std::move(i)), position(pos) {}
};

/**
 * @brief Lightweight NLP text analyzer for query optimization
 * 
 * Provides basic NLP capabilities without heavy dependencies or
 * compute requirements. Designed for:
 * - AQL query analysis and optimization
 * - Execution plan cost estimation
 * - Text-based query pattern recognition
 * - Semantic query hints generation
 * 
 * @note This class is thread-safe for read operations after initialization.
 */
class NlpTextAnalyzer {
public:
    /**
     * @brief Supported languages for analysis
     */
    enum class Language {
        UNKNOWN,
        GERMAN,     // de
        ENGLISH,    // en
        FRENCH,     // fr
        SPANISH,    // es
        ITALIAN,    // it
        DUTCH,      // nl
    };

    /**
     * @brief Configuration options for NLP analyzer
     */
    struct Config {
        bool enable_stemming = true;        ///< Enable word stemming
        bool enable_stopwords = true;       ///< Remove stopwords
        size_t max_keywords = 10;           ///< Max keywords to extract
        size_t min_word_length = 3;         ///< Minimum word length
        Language default_language = Language::ENGLISH;
        
        // Stop words configuration
        std::string stopwords_directory = "config/nlp/stopwords";  ///< Directory for stop word YAML files
        bool auto_load_stopwords = true;    ///< Auto-load stop words from YAML files
        
        Config() = default;
    };

    /**
     * @brief Construct NLP analyzer with configuration
     */
    explicit NlpTextAnalyzer(const Config& config);
    
    /// @brief Default constructor using default Config
    NlpTextAnalyzer() : NlpTextAnalyzer(Config()) {}
    
    ~NlpTextAnalyzer() = default;

    // ========== Core Analysis Functions ==========

    /**
     * @brief Detect language of text
     * @param text Input text
     * @return Detected language
     */
    Language detectLanguage(std::string_view text) const;

    /**
     * @brief Tokenize text into words/tokens
     * @param text Input text
     * @return Vector of tokens
     */
    std::vector<Token> tokenize(std::string_view text) const;

    /**
     * @brief Extract keywords using TF-IDF approach
     * @param text Input text
     * @param max_keywords Maximum number of keywords (0 = use config)
     * @return Sorted keywords by relevance
     */
    std::vector<Keyword> extractKeywords(std::string_view text, size_t max_keywords = 0) const;

    /**
     * @brief Extract named entities (person, organization, location)
     * @param text Input text
     * @return Vector of named entities
     */
    std::vector<NamedEntity> extractEntities(std::string_view text) const;

    /**
     * @brief Analyze sentiment of text
     * @param text Input text
     * @return Sentiment analysis result
     */
    SentimentResult analyzeSentiment(std::string_view text) const;

    /**
     * @brief Calculate text complexity metrics
     * @param text Input text
     * @return Complexity metrics
     */
    ComplexityMetrics analyzeComplexity(std::string_view text) const;

    /**
     * @brief Extract legal modalities from text (e.g., German modal verbs)
     * @param text Input text (legal/administrative document)
     * @param language_code Language code (e.g., "de" for German)
     * @param config_path Optional path to YAML config (default: german_modal_verbs.yaml)
     * @return Vector of detected legal modalities with deontic semantics
     * 
     * Analyzes text for modal verbs with legal significance, particularly
     * for German administrative law (Verwaltungsrecht):
     * - "muss" (must) = Binding obligation (O(φ))
     * - "soll" (shall) = Default rule (O_default(φ))
     * - "kann" (may) = Discretionary permission (P(φ))
     */
    std::vector<LegalModality> extractLegalModalities(
        std::string_view text,
        const std::string& language_code = "de",
        const std::string& config_path = "") const;

    // ========== AQL Query Optimization Support ==========

    /**
     * @brief Estimate query complexity based on text analysis
     * @param query_text AQL query text
     * @return Complexity score [0.0, 1.0], higher = more complex
     */
    double estimateQueryComplexity(std::string_view query_text) const;

    /**
     * @brief Extract semantic hints for query optimization
     * @param query_text AQL query text
     * @return Map of hint_type -> hint_value
     */
    std::map<std::string, std::string> extractQueryHints(std::string_view query_text) const;

    /**
     * @brief Suggest index usage based on query text patterns
     * @param query_text AQL query text
     * @return Vector of suggested index types
     */
    std::vector<std::string> suggestIndexes(std::string_view query_text) const;

    /**
     * @brief Normalize query text for comparison
     * @param query_text Input query
     * @return Normalized form
     */
    std::string normalizeQuery(std::string_view query_text) const;

    // ========== Utility Functions ==========

    /**
     * @brief Check if word is a stop word
     */
    bool isStopWord(std::string_view word, Language lang = Language::ENGLISH) const;

    /**
     * @brief Stem a word to its base form
     */
    std::string stemWord(std::string_view word, Language lang = Language::ENGLISH) const;

    /**
     * @brief Lemmatize a word to its canonical dictionary form (full morphological lemmatization)
     *
     * Applies language-specific morphological rules and irregular-form lookup tables to
     * return the base (dictionary) form of a word for the given language.  Unlike
     * stemWord(), the result is always a valid word in the target language.
     *
     * Supported languages: ENGLISH, GERMAN, FRENCH, SPANISH, ITALIAN, DUTCH.
     * Falls back to lowercased input for UNKNOWN.
     *
     * @param word   Input word (case-insensitive)
     * @param lang   Target language (default: ENGLISH)
     * @return       Canonical lemma of the word
     */
    std::string lemmatizeWord(std::string_view word, Language lang = Language::ENGLISH) const;

    /**
     * @brief Calculate similarity between two texts
     * @param text1 First text
     * @param text2 Second text
     * @return Similarity score [0.0, 1.0]
     */
    double calculateSimilarity(std::string_view text1, std::string_view text2) const;

    /**
     * @brief Get statistics about analyzer state
     */
    std::map<std::string, size_t> getStatistics() const;

    /**
     * @brief Load stop words from YAML file for a specific language
     * @param yaml_path Path to YAML file
     * @param lang Language code
     * @return true if loaded successfully
     */
    bool loadStopWordsFromYaml(const std::string& yaml_path, Language lang);

    /**
     * @brief Load all stop word files from directory
     * @param directory Directory containing YAML files
     * @return Number of languages loaded
     */
    size_t loadStopWordsFromDirectory(const std::string& directory);

private:
    Config config_;
    
    // Stop word dictionaries per language
    std::unordered_map<Language, std::set<std::string>> stopwords_;
    
    // Sentiment lexicons (word -> score)
    std::unordered_map<std::string, double> sentiment_lexicon_;
    
    // Named entity patterns (simple regex-based)
    struct EntityPattern {
        std::string pattern = {};
        std::string type;
    };
    std::vector<EntityPattern> entity_patterns_;

    // Legal modality patterns (for German administrative law)
    struct LegalModalityPattern {
        std::string pattern;
        std::string category;
        float strength;
        std::string deontic_logic;
        std::string interpretation;
        std::vector<std::string> context_requirements;
    };
    mutable std::vector<LegalModalityPattern> legal_modality_patterns_;

    // Morphological lemmatization: per-language irregular-form maps (inflected -> lemma)
    std::unordered_map<Language, std::unordered_map<std::string, std::string>> irregular_lemmas_;

    // Statistics
    mutable size_t analysis_count_ = 0;
    mutable size_t token_count_ = 0;

    // ========== Private Helper Methods ==========
    
    void initializeStopWords();
    void initializeSentimentLexicon();
    void initializeEntityPatterns();
    
    std::vector<std::string> splitSentences(std::string_view text) const;
    std::string toLowerCase(std::string_view text) const;
    std::string removePunctuation(std::string_view text) const;
    
    double calculateTfIdf(const std::string& term,
                         const std::map<std::string, size_t>& term_freqs,
                         size_t total_terms) const;
    
    bool isCapitalized(std::string_view word) const;
    bool isAllCaps(std::string_view word) const;
    size_t countSyllables(std::string_view word) const;
    
    // Query-specific helpers
    bool containsAggregation(std::string_view query) const;
    bool containsJoin(std::string_view query) const;
    bool containsSubquery(std::string_view query) const;
    std::vector<std::string> extractTableNames(std::string_view query) const;
    
    // Morphological lemmatization helpers
    void initializeLemmatizationData();
    std::string applyMorphologicalRules(const std::string& lower,
                                        Language lang) const;

    // Legal modality helpers
    bool loadLegalModalityConfig(const std::string& config_path) const;
    std::string getDefaultLegalConfigPath(const std::string& language_code) const;
};

/**
 * @brief Helper function to convert language enum to string
 */
inline std::string_view languageToString(NlpTextAnalyzer::Language lang) {
    switch (lang) {
        case NlpTextAnalyzer::Language::GERMAN:  return "de";
        case NlpTextAnalyzer::Language::ENGLISH: return "en";
        case NlpTextAnalyzer::Language::FRENCH:  return "fr";
        case NlpTextAnalyzer::Language::SPANISH: return "es";
        case NlpTextAnalyzer::Language::ITALIAN: return "it";
        case NlpTextAnalyzer::Language::DUTCH:   return "nl";
        default: return "unknown";
    }
}

} // namespace analytics
} // namespace themis
