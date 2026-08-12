#include <gtest/gtest.h>
#include "api/graphql_cache.h"
#include "security/output_encoding.h"
#include <thread>
#include <chrono>

using namespace themis::graphql;
using namespace themis::security;

// ============================================================================
// Cache Tests
// ============================================================================

TEST(GraphQLCache, BasicCacheOperations) {
    Cache<std::string> cache(10, std::chrono::seconds(60));
    
    // Put and get
    cache.put("key1", "value1");
    auto value = cache.get("key1");
    
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "value1");
}

TEST(GraphQLCache, CacheMiss) {
    Cache<std::string> cache(10, std::chrono::seconds(60));
    
    auto value = cache.get("nonexistent");
    EXPECT_EQ(value, nullptr);
}

TEST(GraphQLCache, CacheExpiration) {
    Cache<std::string> cache(10, std::chrono::seconds(1));  // 1 second TTL
    
    cache.put("key1", "value1");
    
    // Should be available immediately
    auto value1 = cache.get("key1");
    ASSERT_NE(value1, nullptr);
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired now
    auto value2 = cache.get("key1");
    EXPECT_EQ(value2, nullptr);
}

TEST(GraphQLCache, CacheEviction) {
    Cache<std::string> cache(3, std::chrono::seconds(60));  // Max 3 entries
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");
    
    // Cache is full, adding 4th should evict LRU
    cache.put("key4", "value4");
    
    // All should still be accessible for now (implementation-dependent)
    EXPECT_LE(cache.size(), 3);
}

TEST(GraphQLCache, CacheInvalidation) {
    Cache<std::string> cache(10, std::chrono::seconds(60));
    
    cache.put("key1", "value1");
    auto value1 = cache.get("key1");
    ASSERT_NE(value1, nullptr);
    
    cache.invalidate("key1");
    auto value2 = cache.get("key1");
    EXPECT_EQ(value2, nullptr);
}

TEST(GraphQLCache, CacheClear) {
    Cache<std::string> cache(10, std::chrono::seconds(60));
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    
    cache.clear();
    
    EXPECT_EQ(cache.get("key1"), nullptr);
    EXPECT_EQ(cache.get("key2"), nullptr);
    EXPECT_EQ(cache.size(), 0);
}

TEST(GraphQLCache, CacheStats) {
    Cache<std::string> cache(10, std::chrono::seconds(60));
    
    cache.put("key1", "value1");
    
    // Hit
    cache.get("key1");
    
    // Miss
    cache.get("key2");
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.hitRate(), 0.5);
}

TEST(GraphQLCache, QueryPlanCacheSingleton) {
    auto& cache = QueryPlanCache::instance();
    cache.clear();
    
    QueryPlanCache::QueryPlan plan;
    plan.depth = 3;
    plan.field_count = 5;
    plan.validation_passed = true;
    
    std::string query = "{ user { id name } }";
    cache.put(query, plan);
    
    auto cached = cache.get(query);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->depth, 3);
    EXPECT_EQ(cached->field_count, 5);
    EXPECT_TRUE(cached->validation_passed);
}

TEST(GraphQLCache, ResponseCacheSingleton) {
    auto& cache = ResponseCache::instance();
    cache.clear();
    
    ResponseCache::CachedResponse response;
    response.data = "{\"user\":{\"id\":\"123\"}}";
    response.etag = "abc123";
    
    std::string query = "{ user(id: \"123\") { id } }";
    cache.put(query, response);
    
    auto cached = cache.get(query);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->data, "{\"user\":{\"id\":\"123\"}}");
    EXPECT_EQ(cached->etag, "abc123");
}

TEST(GraphQLCache, InvalidatePatternSelectiveEviction) {
    auto& cache = ResponseCache::instance();
    cache.clear();

    // Entry tagged with "orders"
    ResponseCache::CachedResponse r1;
    r1.data = "{\"orders\":[]}";
    r1.collections = {"orders"};
    cache.put("{ orders { id } }", r1);

    // Entry tagged with "users"
    ResponseCache::CachedResponse r2;
    r2.data = "{\"users\":[]}";
    r2.collections = {"users"};
    cache.put("{ users { id } }", r2);

    // Entry tagged with both
    ResponseCache::CachedResponse r3;
    r3.data = "{\"orders\":[], \"users\":[]}";
    r3.collections = {"orders", "users"};
    cache.put("{ orders { id } users { id } }", r3);

    // Invalidate only "orders"
    cache.invalidatePattern("orders");

    // "orders"-tagged entries must be gone
    EXPECT_EQ(cache.get("{ orders { id } }"), nullptr);
    EXPECT_EQ(cache.get("{ orders { id } users { id } }"), nullptr);

    // "users"-only entry must still be present
    auto remaining = cache.get("{ users { id } }");
    ASSERT_NE(remaining, nullptr);
    EXPECT_EQ(remaining->data, "{\"users\":[]}");
}

TEST(GraphQLCache, InvalidatePatternNoMatchLeavesAllEntries) {
    auto& cache = ResponseCache::instance();
    cache.clear();

    ResponseCache::CachedResponse r;
    r.data = "{\"products\":[]}";
    r.collections = {"products"};
    cache.put("{ products { id } }", r);

    // Invalidating an unrelated collection must not evict anything
    cache.invalidatePattern("orders");

    auto remaining = cache.get("{ products { id } }");
    ASSERT_NE(remaining, nullptr);
}

TEST(GraphQLCache, InvalidatePatternPerformanceTarget) {
    // Performance target: invalidating one collection evicts ≤10% of entries
    // when 10 distinct collections are active.
    auto& cache = ResponseCache::instance();
    cache.clear();

    const int collections_count = 10;
    const int entries_per_collection = 10;  // 100 total entries

    for (int c = 0; c < collections_count; ++c) {
        std::string coll = "collection_" + std::to_string(c);
        for (int e = 0; e < entries_per_collection; ++e) {
            ResponseCache::CachedResponse r;
            r.data = "{\"data\":" + std::to_string(e) + "}";
            r.collections = {coll};
            cache.put("query_" + coll + "_" + std::to_string(e), r);
        }
    }

    size_t before = 0;
    static_cast<void>(before);
    // Count entries by querying all keys (approximate via put/get round-trip)
    // We know we inserted entries_per_collection * collections_count entries;
    // check that invalidating one collection removes exactly entries_per_collection.
    cache.invalidatePattern("collection_0");

    int evicted = 0;
    for (int e = 0; e < entries_per_collection; ++e) {
        if (!cache.get("query_collection_0_" + std::to_string(e))) {
            evicted++;
        }
    }
    EXPECT_EQ(evicted, entries_per_collection);

    // Remaining collections must all still be present
    int surviving = 0;
    for (int c = 1; c < collections_count; ++c) {
        std::string coll = "collection_" + std::to_string(c);
        for (int e = 0; e < entries_per_collection; ++e) {
            if (cache.get("query_" + coll + "_" + std::to_string(e))) {
                surviving++;
            }
        }
    }
    int total = collections_count * entries_per_collection;
    double eviction_ratio = static_cast<double>(evicted) / total;
    EXPECT_LE(eviction_ratio, 0.10);
    EXPECT_EQ(surviving, (collections_count - 1) * entries_per_collection);
}

// ============================================================================
// Output Encoding Tests
// ============================================================================

TEST(OutputEncoder, HTMLEncoding) {
    std::string input = "<script>alert('XSS')</script>";
    std::string encoded = OutputEncoder::encodeHTML(input);
    
    EXPECT_EQ(encoded, "&lt;script&gt;alert(&#x27;XSS&#x27;)&lt;&#x2F;script&gt;");
    EXPECT_TRUE(encoded.find("<script>") == std::string::npos);
}

TEST(OutputEncoder, HTMLEncodingSpecialChars) {
    std::string input = "& < > \" ' /";
    std::string encoded = OutputEncoder::encodeHTML(input);
    
    EXPECT_EQ(encoded, "&amp; &lt; &gt; &quot; &#x27; &#x2F;");
}

TEST(OutputEncoder, JavaScriptEncoding) {
    std::string input = "'; alert('XSS'); var x='";
    std::string encoded = OutputEncoder::encodeJavaScript(input);
    
    EXPECT_TRUE(encoded.find("\\'") != std::string::npos);
    EXPECT_TRUE(encoded.find("alert") != std::string::npos);
}

TEST(OutputEncoder, JavaScriptEncodingControlChars) {
    std::string input = "line1\nline2\ttab\rreturn";
    std::string encoded = OutputEncoder::encodeJavaScript(input);
    
    EXPECT_TRUE(encoded.find("\\n") != std::string::npos);
    EXPECT_TRUE(encoded.find("\\t") != std::string::npos);
    EXPECT_TRUE(encoded.find("\\r") != std::string::npos);
}

TEST(OutputEncoder, URLEncoding) {
    std::string input = "hello world!@#$%^&*()";
    std::string encoded = OutputEncoder::encodeURL(input);
    
    EXPECT_TRUE(encoded.find("%20") != std::string::npos);  // space
    EXPECT_TRUE(encoded.find("hello") != std::string::npos);
}

TEST(OutputEncoder, URLEncodingPreservesAlphanumeric) {
    std::string input = "abc123-_.~";
    std::string encoded = OutputEncoder::encodeURL(input);
    
    EXPECT_EQ(encoded, "abc123-_.~");  // These should not be encoded
}

TEST(OutputEncoder, JSONEncoding) {
    std::string input = "{ \"key\": \"value\nwith newline\" }";
    std::string encoded = OutputEncoder::encodeJSON(input);
    
    EXPECT_TRUE(encoded.find("\\\"") != std::string::npos);
    EXPECT_TRUE(encoded.find("\\n") != std::string::npos);
}

TEST(OutputEncoder, AttributeSanitization) {
    std::string input = "value with \"quotes\" and 'apostrophes'";
    std::string sanitized = OutputEncoder::sanitizeAttribute(input);
    
    EXPECT_TRUE(sanitized.find("\"") == std::string::npos);
    EXPECT_TRUE(sanitized.find("'") == std::string::npos);
    EXPECT_TRUE(sanitized.find("value") != std::string::npos);
}

TEST(OutputEncoder, EmptyStringHandling) {
    EXPECT_EQ(OutputEncoder::encodeHTML(""), "");
    EXPECT_EQ(OutputEncoder::encodeJavaScript(""), "");
    EXPECT_EQ(OutputEncoder::encodeURL(""), "");
    EXPECT_EQ(OutputEncoder::encodeJSON(""), "");
}

// ============================================================================
// CSP Builder Tests
// ============================================================================

TEST(CSPBuilder, BasicCSP) {
    std::string csp = CSPBuilder()
        .defaultSrc("'self'")
        .build();
    
    EXPECT_EQ(csp, "default-src 'self'");
}

TEST(CSPBuilder, MultipleDirectives) {
    std::string csp = CSPBuilder()
        .defaultSrc("'self'")
        .scriptSrc("'self' 'unsafe-inline'")
        .styleSrc("'self'")
        .build();
    
    EXPECT_TRUE(csp.find("default-src 'self'") != std::string::npos);
    EXPECT_TRUE(csp.find("script-src") != std::string::npos);
    EXPECT_TRUE(csp.find("style-src") != std::string::npos);
}

TEST(CSPBuilder, StrictAPIPolicy) {
    std::string csp = CSPBuilder::strictAPI();
    EXPECT_EQ(csp, "default-src 'none'");
}

TEST(CSPBuilder, StandardPolicy) {
    std::string csp = CSPBuilder::standard();
    
    EXPECT_TRUE(csp.find("default-src 'self'") != std::string::npos);
    EXPECT_TRUE(csp.find("script-src 'self'") != std::string::npos);
}

// ============================================================================
// Security Headers Tests
// ============================================================================

TEST(SecurityHeaders, APIHeadersIncludeCSP) {
    auto headers = SecurityHeaders::apiHeaders();
    
    EXPECT_TRUE(headers.find("Content-Security-Policy") != headers.end());
    EXPECT_TRUE(headers.find("X-Content-Type-Options") != headers.end());
    EXPECT_TRUE(headers.find("X-Frame-Options") != headers.end());
    EXPECT_EQ(headers["X-Frame-Options"], "DENY");
}

TEST(SecurityHeaders, APIHeadersIncludeHSTS) {
    auto headers = SecurityHeaders::apiHeaders();
    
    EXPECT_TRUE(headers.find("Strict-Transport-Security") != headers.end());
    EXPECT_TRUE(headers["Strict-Transport-Security"].find("max-age") != std::string::npos);
}

TEST(SecurityHeaders, WebHeadersLessRestrictive) {
    auto headers = SecurityHeaders::webHeaders();
    
    EXPECT_EQ(headers["X-Frame-Options"], "SAMEORIGIN");
    EXPECT_TRUE(headers.find("Content-Security-Policy") != headers.end());
}

TEST(SecurityHeaders, AllExpectedHeadersPresent) {
    auto headers = SecurityHeaders::apiHeaders();
    
    EXPECT_EQ(headers.size(), 6);  // Should have 6 standard headers
    EXPECT_TRUE(headers.find("X-XSS-Protection") != headers.end());
    EXPECT_TRUE(headers.find("Referrer-Policy") != headers.end());
}
