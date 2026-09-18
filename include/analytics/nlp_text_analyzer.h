/**
 * @file nlp_text_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

struct Token {
    std::string text;           ///< The token text
    size_t position;            ///< Position in original text
    std::string lemma;          ///< Base form of the word
    std::string pos_tag;        ///< Part-of-speech tag (NOUN, VERB, etc.)
    
    Token() = default;
    Token(std::string t, size_t pos) : text(std::move(t)), position(pos) {}
};

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

class NlpTextAnalyzer {
public:
    enum class Language {
        UNKNOWN,
        GERMAN,     // de
        ENGLISH,    // en
        FRENCH,     // fr
        SPANISH,    // es
        ITALIAN,    // it
        DUTCH,      // nl
    };

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
     * @brief Nlp Text Analyzer.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit NlpTextAnalyzer(const Config& config);
    
    NlpTextAnalyzer() : NlpTextAnalyzer(Config()) {}
    
    ~NlpTextAnalyzer() = default;


    /**
     * @brief Detect Language.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    Language detectLanguage(std::string_view text) const;

    /**
     * @brief Tokenize.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<Token> tokenize(std::string_view text) const;

    std::vector<Keyword> extractKeywords(std::string_view text, size_t max_keywords = 0) const;

    /**
     * @brief Extract Entities.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<NamedEntity> extractEntities(std::string_view text) const;

    /**
     * @brief Analyze Sentiment.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    SentimentResult analyzeSentiment(std::string_view text) const;

    /**
     * @brief Analyze Complexity.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    ComplexityMetrics analyzeComplexity(std::string_view text) const;

    std::vector<LegalModality> extractLegalModalities(
        std::string_view text,
        const std::string& language_code = "de",
        const std::string& config_path = "") const;


    /**
     * @brief Estimate Query Complexity.
     * @param[in] query_text Input parameter.
     * @return Return value.
     */
    double estimateQueryComplexity(std::string_view query_text) const;

    std::map<std::string, std::string> extractQueryHints(std::string_view query_text) const;

    /**
     * @brief Suggest Indexes.
     * @param[in] query_text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> suggestIndexes(std::string_view query_text) const;

    /**
     * @brief Normalize Query.
     * @param[in] query_text Input parameter.
     * @return Return value.
     */
    std::string normalizeQuery(std::string_view query_text) const;

    // ========== Utility Functions ==========

    bool isStopWord(std::string_view word, Language lang = Language::ENGLISH) const;

    std::string stemWord(std::string_view word, Language lang = Language::ENGLISH) const;

    std::string lemmatizeWord(std::string_view word, Language lang = Language::ENGLISH) const;

    /**
     * @brief Calculate Similarity.
     * @param[in] text1 Input parameter.
     * @param[in] text2 Input parameter.
     * @return Return value.
     */
    double calculateSimilarity(std::string_view text1, std::string_view text2) const;

    std::map<std::string, size_t> getStatistics() const;

    /**
     * @brief Load Stop Words From Yaml.
     * @param[in] yaml_path Path to the yaml.
     * @param[in] lang Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadStopWordsFromYaml(const std::string& yaml_path, Language lang);

    /**
     * @brief Load Stop Words From Directory.
     * @param[in] directory Input parameter.
     * @return Return value.
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

    
    /**
     * @brief Initialize Stop Words.
     */
    void initializeStopWords();
    /**
     * @brief Initialize Sentiment Lexicon.
     */
    void initializeSentimentLexicon();
    /**
     * @brief Initialize Entity Patterns.
     */
    void initializeEntityPatterns();
    
    /**
     * @brief Split Sentences.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> splitSentences(std::string_view text) const;
    /**
     * @brief To Lower Case.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string toLowerCase(std::string_view text) const;
    /**
     * @brief Remove Punctuation.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string removePunctuation(std::string_view text) const;
    
    double calculateTfIdf(const std::string& term,
                         const std::map<std::string, size_t>& term_freqs,
                         size_t total_terms) const;
    
    /**
     * @brief Is Capitalized.
     * @param[in] word Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCapitalized(std::string_view word) const;
    /**
     * @brief Is All Caps.
     * @param[in] word Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAllCaps(std::string_view word) const;
    /**
     * @brief Count Syllables.
     * @param[in] word Input parameter.
     * @return Return value.
     */
    size_t countSyllables(std::string_view word) const;
    
    // Query-specific helpers
    /**
     * @brief Contains Aggregation.
     * @param[in] query Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsAggregation(std::string_view query) const;
    /**
     * @brief Contains Join.
     * @param[in] query Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsJoin(std::string_view query) const;
    /**
     * @brief Contains Subquery.
     * @param[in] query Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsSubquery(std::string_view query) const;
    /**
     * @brief Extract Table Names.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractTableNames(std::string_view query) const;
    
    // Morphological lemmatization helpers
    /**
     * @brief Initialize Lemmatization Data.
     */
    void initializeLemmatizationData();
    /**
     * @brief Apply Morphological Rules.
     * @param[in] lower Input parameter.
     * @param[in] lang Input parameter.
     * @return Return value.
     */
    std::string applyMorphologicalRules(const std::string& lower,
                                        Language lang) const;

    // Legal modality helpers
    /**
     * @brief Load Legal Modality Config.
     * @param[in] config_path Path to the retention policy configuration file.
     * @return True when the operation succeeds.
     */
    bool loadLegalModalityConfig(const std::string& config_path) const;
    /**
     * @brief Get Default Legal Config Path.
     * @param[in] language_code Input parameter.
     * @return Return value.
     */
    std::string getDefaultLegalConfigPath(const std::string& language_code) const;
};

/**
 * @brief Language To String.
 * @param[in] lang Input parameter.
 * @return Return value.
 * @details Implements languageToString without additional internal calls.
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
