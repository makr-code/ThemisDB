// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/capability_matcher.h"
#include "sharding/shard_topology.h"

using namespace themis::sharding;

class CapabilityMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create matcher with default config
        CapabilityMatcher::Config config;
        config.keyword_weight = 0.5;
        config.semantic_weight = 0.3;
        config.domain_weight = 0.1;
        config.organization_weight = 0.05;
        config.region_weight = 0.03;
        config.data_type_weight = 0.02;
        config.use_tfidf = false;  // Use simple Jaccard for testing
        config.enable_embeddings = false;  // Disable for simple tests
        
        matcher = std::make_unique<CapabilityMatcher>(config);
        
        // Create test shards with capabilities
        setupTestShards();
    }
    
    void setupTestShards() {
        // Shard 1: Hamburg building department
        ShardInfo shard1;
        shard1.shard_id = "shard_hamburg_bauamt";
        shard1.primary_endpoint = "hamburg:8080";
        shard1.is_healthy = true;
        shard1.domain_capability.domains = {"construction", "law"};
        shard1.domain_capability.organizations = {"hamburg_bauamt"};
        shard1.domain_capability.regions = {"hamburg", "germany"};
        shard1.domain_capability.data_types = {"building_permits", "legal_documents"};
        shard1.domain_capability.keywords = {"baurecht", "building", "permit", "hamburg"};
        shards.push_back(shard1);
        
        // Shard 2: Bremen building department
        ShardInfo shard2;
        shard2.shard_id = "shard_bremen_bauamt";
        shard2.primary_endpoint = "bremen:8080";
        shard2.is_healthy = true;
        shard2.domain_capability.domains = {"construction", "law"};
        shard2.domain_capability.organizations = {"bremen_bauamt"};
        shard2.domain_capability.regions = {"bremen", "germany"};
        shard2.domain_capability.data_types = {"building_permits"};
        shard2.domain_capability.keywords = {"baurecht", "building", "permit", "bremen"};
        shards.push_back(shard2);
        
        // Shard 3: German law database
        ShardInfo shard3;
        shard3.shard_id = "shard_de_law";
        shard3.primary_endpoint = "law:8080";
        shard3.is_healthy = true;
        shard3.domain_capability.domains = {"law"};
        shard3.domain_capability.organizations = {};
        shard3.domain_capability.regions = {"germany"};
        shard3.domain_capability.data_types = {"legal_documents", "statutes"};
        shard3.domain_capability.keywords = {"law", "legal", "statute", "germany"};
        shards.push_back(shard3);
        
        // Shard 4: Berlin health department
        ShardInfo shard4;
        shard4.shard_id = "shard_berlin_health";
        shard4.primary_endpoint = "berlin:8080";
        shard4.is_healthy = true;
        shard4.domain_capability.domains = {"medicine", "health"};
        shard4.domain_capability.organizations = {"berlin_health"};
        shard4.domain_capability.regions = {"berlin", "germany"};
        shard4.domain_capability.data_types = {"medical_records", "health_permits"};
        shard4.domain_capability.keywords = {"health", "medical", "medicine", "berlin"};
        shards.push_back(shard4);
    }
    
    std::unique_ptr<CapabilityMatcher> matcher;
    std::vector<ShardInfo> shards;
};

TEST_F(CapabilityMatcherTest, ExtractKeywords) {
    std::string query = "Baurechtsakten Hamburg und Berlin";
    auto keywords = matcher->extractKeywords(query);
    
    EXPECT_FALSE(keywords.empty());
    // Should extract non-stopwords
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "baurechtsakten") != keywords.end());
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "hamburg") != keywords.end());
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "berlin") != keywords.end());
    // Should not contain stopwords
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "und") == keywords.end());
}

TEST_F(CapabilityMatcherTest, MatchHamburgBuildingQuery) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "Baurechtsakten Hamburg";
    query.keywords = {"baurechtsakten", "hamburg"};
    query.domains = {"construction"};
    query.regions = {"hamburg"};
    
    auto results = matcher->match(query, shards);
    
    ASSERT_FALSE(results.empty());
    // Hamburg building department should score highest
    EXPECT_EQ(results[0].shard_id, "shard_hamburg_bauamt");
    EXPECT_GT(results[0].score, 0.09);  // Weighted match should be clearly non-trivial
    
    // Bremen should be second (same domain but different region)
    ASSERT_GE(results.size(), 2);
    EXPECT_EQ(results[1].shard_id, "shard_bremen_bauamt");
    EXPECT_LT(results[1].score, results[0].score);
}

TEST_F(CapabilityMatcherTest, MatchLawQuery) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "Legal documents Germany";
    query.keywords = {"legal", "documents", "germany"};
    query.domains = {"law"};
    query.regions = {"germany"};
    
    auto results = matcher->match(query, shards);
    
    ASSERT_FALSE(results.empty());
    // Law database should score high
    bool found_law_shard = false;
    for (const auto& result : results) {
        if (result.shard_id == "shard_de_law") {
            found_law_shard = true;
            EXPECT_GT(result.score, 0.25);
            break;
        }
    }
    EXPECT_TRUE(found_law_shard);
}

TEST_F(CapabilityMatcherTest, NoMatchQuery) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "Financial reports Tokyo";
    query.keywords = {"financial", "reports", "tokyo"};
    query.domains = {"finance"};
    query.regions = {"japan"};
    
    auto results = matcher->match(query, shards);
    
    // Should return results but with low scores
    if (!results.empty()) {
        EXPECT_LT(results[0].score, 0.3);  // No good matches
    }
}

TEST_F(CapabilityMatcherTest, KeywordScoring) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "building permit";
    query.keywords = {"building", "permit"};
    
    auto results = matcher->match(query, shards);
    
    ASSERT_FALSE(results.empty());
    // Shards with building/permit keywords should score
    for (const auto& result : results) {
        if (result.shard_id == "shard_hamburg_bauamt" || 
            result.shard_id == "shard_bremen_bauamt") {
            EXPECT_GT(result.keyword_score, 0.0);
        }
    }
}

TEST_F(CapabilityMatcherTest, DomainScoring) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "construction documents";
    query.keywords = {"construction", "documents"};
    query.domains = {"construction"};
    
    auto results = matcher->match(query, shards);
    
    ASSERT_FALSE(results.empty());
    // Construction domain shards should have domain score
    for (const auto& result : results) {
        if (result.shard_id == "shard_hamburg_bauamt" || 
            result.shard_id == "shard_bremen_bauamt") {
            EXPECT_GT(result.domain_score, 0.0);
        }
    }
}

TEST_F(CapabilityMatcherTest, RegionScoring) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "hamburg records";
    query.keywords = {"hamburg", "records"};
    query.regions = {"hamburg"};
    
    auto results = matcher->match(query, shards);
    
    ASSERT_FALSE(results.empty());
    // Hamburg shard should have region score
    bool found_hamburg = false;
    for (const auto& result : results) {
        if (result.shard_id == "shard_hamburg_bauamt") {
            found_hamburg = true;
            EXPECT_GT(result.region_score, 0.0);
            break;
        }
    }
    EXPECT_TRUE(found_hamburg);
}

TEST_F(CapabilityMatcherTest, EmptyCapabilityFallback) {
    // Test shard with no domain capability but existing capabilities
    ShardInfo shard_empty;
    shard_empty.shard_id = "shard_empty";
    shard_empty.primary_endpoint = "empty:8080";
    shard_empty.is_healthy = true;
    shard_empty.capabilities = {"read", "write"};
    // domain_capability is empty
    
    std::vector<ShardInfo> test_shards = {shard_empty};
    
    CapabilityMatcher::QueryContext query;
    query.query_text = "read data";
    query.keywords = {"read", "data"};
    
    auto results = matcher->match(query, test_shards);
    
    // Should still match using capabilities as fallback keywords
    ASSERT_FALSE(results.empty());
    EXPECT_GT(results[0].score, 0.0);
}

TEST_F(CapabilityMatcherTest, SortedByScore) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "hamburg building";
    query.keywords = {"hamburg", "building"};
    query.regions = {"hamburg"};
    query.domains = {"construction"};
    
    auto results = matcher->match(query, shards);
    
    ASSERT_GE(results.size(), 2);
    // Results should be sorted by score descending
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].score, results[i].score);
    }
}

TEST_F(CapabilityMatcherTest, BuildIDF) {
    // Build IDF from shards
    matcher->buildIDF(shards);
    
    // Statistics should be updated
    auto stats = matcher->getStatistics();
    EXPECT_GT(stats["idf_cache_size"], 0);
    EXPECT_EQ(stats["total_shards"], shards.size());
}

TEST_F(CapabilityMatcherTest, Statistics) {
    CapabilityMatcher::QueryContext query;
    query.query_text = "test query";
    query.keywords = {"test", "query"};
    
    matcher->match(query, shards);
    
    auto stats = matcher->getStatistics();
    EXPECT_EQ(stats["total_matches"], 1);
    EXPECT_GE(stats["keyword_matches"], 0);
}
