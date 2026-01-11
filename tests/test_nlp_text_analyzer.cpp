#include <iostream>
#include <cassert>
#include "analytics/nlp_text_analyzer.h"

using namespace themis::analytics;

void test_tokenization() {
    std::cout << "Testing tokenization..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    auto tokens = analyzer.tokenize("The quick brown fox jumps over the lazy dog");
    
    assert(tokens.size() == 9);
    assert(tokens[0].text == "The");
    assert(tokens[1].text == "quick");
    
    std::cout << "  ✓ Tokenization passed" << std::endl;
}

void test_language_detection() {
    std::cout << "Testing language detection..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    auto lang_en = analyzer.detectLanguage("The quick brown fox jumps over the lazy dog");
    assert(lang_en == NlpTextAnalyzer::Language::ENGLISH);
    
    auto lang_de = analyzer.detectLanguage("Der schnelle braune Fuchs springt über den faulen Hund");
    assert(lang_de == NlpTextAnalyzer::Language::GERMAN);
    
    std::cout << "  ✓ Language detection passed" << std::endl;
}

void test_keyword_extraction() {
    std::cout << "Testing keyword extraction..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    auto keywords = analyzer.extractKeywords(
        "Database optimization query performance indexing search algorithms", 5);
    
    assert(!keywords.empty());
    assert(keywords.size() <= 5);
    
    std::cout << "  ✓ Keyword extraction passed" << std::endl;
}

void test_sentiment_analysis() {
    std::cout << "Testing sentiment analysis..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    auto positive = analyzer.analyzeSentiment("This is a great and wonderful product");
    assert(positive.polarity == SentimentResult::Polarity::POSITIVE);
    
    auto negative = analyzer.analyzeSentiment("This is a terrible and awful experience");
    assert(negative.polarity == SentimentResult::Polarity::NEGATIVE);
    
    std::cout << "  ✓ Sentiment analysis passed" << std::endl;
}

void test_query_complexity() {
    std::cout << "Testing query complexity estimation..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    // Simple query
    double simple = analyzer.estimateQueryComplexity("SELECT * FROM users WHERE id = 1");
    
    // Complex query with JOIN and aggregation
    double complex = analyzer.estimateQueryComplexity(
        "SELECT u.name, COUNT(*) FROM users u JOIN orders o ON u.id = o.user_id "
        "WHERE u.status = 'active' GROUP BY u.name HAVING COUNT(*) > 10");
    
    assert(complex > simple);
    
    std::cout << "  ✓ Query complexity estimation passed" << std::endl;
}

void test_query_hints() {
    std::cout << "Testing query hints extraction..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    auto hints = analyzer.extractQueryHints(
        "SELECT * FROM documents WHERE MATCH(content, 'database') ORDER BY score LIMIT 10");
    
    assert(hints.count("search_type") > 0);
    assert(hints.count("sorting") > 0);
    assert(hints.count("result_limit") > 0);
    
    std::cout << "  ✓ Query hints extraction passed" << std::endl;
}

void test_index_suggestions() {
    std::cout << "Testing index suggestions..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    auto fulltext = analyzer.suggestIndexes("SELECT * FROM articles WHERE MATCH(content, 'search')");
    assert(std::find(fulltext.begin(), fulltext.end(), "fulltext") != fulltext.end());
    
    auto vector = analyzer.suggestIndexes("SELECT * FROM images WHERE vector_similarity(embedding, ?) > 0.8");
    assert(std::find(vector.begin(), vector.end(), "hnsw") != vector.end());
    
    std::cout << "  ✓ Index suggestions passed" << std::endl;
}

void test_entity_extraction() {
    std::cout << "Testing named entity extraction..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    auto entities = analyzer.extractEntities(
        "Contact John Smith at john.smith@example.com or visit https://example.com. "
        "The meeting is on 2024-03-15.");
    
    assert(!entities.empty());
    
    // Should find email, URL, and date
    bool found_email = false;
    bool found_url = false;
    bool found_date = false;
    
    for (const auto& entity : entities) {
        if (entity.type == "EMAIL") found_email = true;
        if (entity.type == "URL") found_url = true;
        if (entity.type == "DATE") found_date = true;
    }
    
    assert(found_email);
    assert(found_url);
    assert(found_date);
    
    std::cout << "  ✓ Named entity extraction passed" << std::endl;
}

void test_text_complexity() {
    std::cout << "Testing text complexity analysis..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    auto metrics = analyzer.analyzeComplexity(
        "The cat sat on the mat. The dog ran in the park. Birds fly in the sky.");
    
    assert(metrics.word_count > 0);
    assert(metrics.sentence_count == 3);
    assert(metrics.unique_words > 0);
    assert(metrics.lexical_diversity > 0.0 && metrics.lexical_diversity <= 1.0);
    
    std::cout << "  ✓ Text complexity analysis passed" << std::endl;
}

void test_text_similarity() {
    std::cout << "Testing text similarity..." << std::endl;
    
    NlpTextAnalyzer analyzer;
    
    double sim1 = analyzer.calculateSimilarity(
        "database query optimization",
        "database query performance");
    
    double sim2 = analyzer.calculateSimilarity(
        "database query optimization",
        "machine learning algorithms");
    
    assert(sim1 > sim2); // First pair should be more similar
    
    std::cout << "  ✓ Text similarity passed" << std::endl;
}

int main() {
    std::cout << "\n=== NLP Text Analyzer Tests ===" << std::endl << std::endl;
    
    try {
        test_tokenization();
        test_language_detection();
        test_keyword_extraction();
        test_sentiment_analysis();
        test_query_complexity();
        test_query_hints();
        test_index_suggestions();
        test_entity_extraction();
        test_text_complexity();
        test_text_similarity();
        
        std::cout << "\n✅ All tests passed!" << std::endl;
        
        // Show some statistics
        NlpTextAnalyzer analyzer;
        analyzer.tokenize("Test text");
        auto stats = analyzer.getStatistics();
        
        std::cout << "\nAnalyzer Statistics:" << std::endl;
        for (const auto& [key, value] : stats) {
            std::cout << "  " << key << ": " << value << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
