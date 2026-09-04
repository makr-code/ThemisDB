/**
 * @file aql_fewshot_example_library.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "aql/aql_fewshot_example_library.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "utils/logger.h"

namespace themis {
namespace aql {

AQLFewShotExampleLibrary::AQLFewShotExampleLibrary() {
    registerBuiltins_();
    THEMIS_DEBUG("AQLFewShotExampleLibrary initialized with {} built-in examples", examples_.size());
}

void AQLFewShotExampleLibrary::registerExample(const AQLFewShotExample &example) {
    if (example.id.empty()) {
        throw std::invalid_argument("AQLFewShotExample id must not be empty");
    }
    if (index_by_id_.count(example.id)) {
        throw std::invalid_argument("AQLFewShotExample id already registered: " + example.id);
    }
    index_by_id_[example.id] = examples_.size();
    examples_.push_back(example);
}

const std::vector<AQLFewShotExample> &AQLFewShotExampleLibrary::all() const {
    return examples_;
}

std::vector<AQLFewShotExample> AQLFewShotExampleLibrary::findByDomain(AQLExampleDomain domain) const {
    std::vector<AQLFewShotExample> result = {};

    for (const auto &ex : examples_) {
        if (ex.domain == domain) {
            result.push_back(ex);
        }
    }
    return result;
}

std::vector<AQLFewShotExample> AQLFewShotExampleLibrary::findByTag(const std::string &tag) const {
    std::string tag_lower = tag;
    std::transform(tag_lower.begin(), tag_lower.end(), tag_lower.begin(), ::tolower);

    std::vector<AQLFewShotExample> result = {};

    for (const auto &ex : examples_) {
        for (const auto &t : ex.tags) {
            std::string t_lower = t;
            std::transform(t_lower.begin(), t_lower.end(), t_lower.begin(), ::tolower);
            if (t_lower == tag_lower) {
                result.push_back(ex);
                break;
            }
        }
    }
    return result;
}

const AQLFewShotExample *AQLFewShotExampleLibrary::findById(const std::string &id) const {
    auto it = index_by_id_.find(id);
    if (it == index_by_id_.end()) {
        return nullptr;
    }
    return &examples_[it->second];
}

std::vector<AQLFewShotExample> AQLFewShotExampleLibrary::findRelevant(const std::string &nl_query, std::size_t n,
                                                                      std::optional<AQLExampleDomain> domain) const {
    // Build candidate list (optionally filtered by domain)
    std::vector<const AQLFewShotExample *> candidates = {};

    for (const auto &ex : examples_) {
        if (!domain.has_value() || ex.domain == domain.value()) {
            candidates.push_back(&ex);
        }
    }

    if (candidates.empty()) {
        return {};
    }

    // Score each candidate by relevance to nl_query.
    // When an embedding provider is available, use cosine similarity; otherwise
    // fall back to the existing Jaccard word-overlap metric.
    std::vector<std::pair<double, const AQLFewShotExample *>> scored;
    scored.reserve(candidates.size());

    if (embedding_provider_) {
        // Semantic path: embed the query once, then score each example.
        const std::vector<float> query_emb = embedding_provider_->embed(nl_query);
        for (const auto *c : candidates) {
            // Find the index of this example in examples_
            auto it    = index_by_id_.find(c->id);
            double sim = -1.0;
            if (it != index_by_id_.end()) {
                sim = computeRelevanceSemantic_(query_emb, it->second);
            }
            // Fall back to Jaccard if embedding failed (-1.0 means no embedding)
            if (sim < 0.0) {
                sim = computeRelevance_(nl_query, *c);
            }
            scored.emplace_back(sim, c);
        }
    } else {
        // Lexical path (default): Jaccard word-overlap
        for (const auto *c : candidates) {
            scored.emplace_back(computeRelevance_(nl_query, *c), c);
        }
    }

    // Sort by descending relevance
    std::sort(scored.begin(), scored.end(), [](const auto &a, const auto &b) { return a.first > b.first; });

    // Collect top-n results
    std::size_t count = std::min(n, scored.size());
    std::vector<AQLFewShotExample> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        result.push_back(*scored[i].second);
    }
    return result;
}

std::string AQLFewShotExampleLibrary::formatForPrompt(const std::vector<AQLFewShotExample> &examples) {
    if (examples.empty()) {
        return {};
    }
    std::ostringstream oss;
    oss << "Here are some examples of natural language queries and their AQL translations:\n\n";
    for (const auto &ex : examples) {
        oss << "Natural language: " << ex.nl_query << "\n";
        oss << "AQL: " << ex.aql_query << "\n\n";
    }
    return oss.str();
}

std::string AQLFewShotExampleLibrary::buildPromptSection(const std::string &nl_query, std::size_t n,
                                                         std::optional<AQLExampleDomain> domain) const {
    return formatForPrompt(findRelevant(nl_query, n, domain));
}

std::size_t AQLFewShotExampleLibrary::size() const {
    return examples_.size();
}

// ============================================================================
// Semantic ranking methods
// ============================================================================

void AQLFewShotExampleLibrary::setEmbeddingProvider(IEmbeddingProvider *provider) {
    embedding_provider_ = provider;
    // Invalidate the cache so new embeddings are computed with the new provider
    embedding_cache_.clear();
}

void AQLFewShotExampleLibrary::rebuildEmbeddingIndex() {
    if (!embedding_provider_) {
        return;
    }

    embedding_cache_.resize(examples_.size());
    for (std::size_t i = 0; i < examples_.size(); ++i) {
        embedding_cache_[i] = embedding_provider_->embed(examples_[i].nl_query);
    }
}

bool AQLFewShotExampleLibrary::ensureEmbedding_(std::size_t idx) const {
    if (!embedding_provider_) {
        return false;
    }
    if (idx >= examples_.size()) {
        return false;
    }

    // Grow cache if needed
    if (embedding_cache_.size() <= idx) {
        embedding_cache_.resize(examples_.size());
    }
    // Compute on demand if not yet cached
    if (embedding_cache_[idx].empty()) {
        embedding_cache_[idx] = embedding_provider_->embed(examples_[idx].nl_query);
    }
    return !embedding_cache_[idx].empty();
}

double AQLFewShotExampleLibrary::cosineSimilarity_(const std::vector<float> &a, const std::vector<float> &b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0;
    }

    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }
    const double denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    return (denom > 1e-12) ? dot / denom : 0.0;
}

double AQLFewShotExampleLibrary::computeRelevanceSemantic_(const std::vector<float> &query_embedding,
                                                           std::size_t example_index) const {
    if (query_embedding.empty()) {
        return -1.0;
    }
    if (!ensureEmbedding_(example_index)) {
        return -1.0;
    }
    return cosineSimilarity_(query_embedding, embedding_cache_[example_index]);
}

// ============================================================================
// Private helpers
// ============================================================================

double AQLFewShotExampleLibrary::computeRelevance_(const std::string &query, const AQLFewShotExample &example) {
    // Jaccard word-overlap similarity between query and example nl_query
    auto tokenize = [](const std::string &s) {
        std::unordered_set<std::string> tokens;
        std::istringstream iss(s);
        std::string token;
        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            // Strip trailing punctuation
            while (!token.empty() && !std::isalnum(static_cast<unsigned char>(token.back()))) {
                token.pop_back();
            }
            if (!token.empty()) {
                tokens.insert(token);
            }
        }
        return tokens;
    };

    auto q_tokens  = tokenize(query);
    auto ex_tokens = tokenize(example.nl_query);

    if (q_tokens.empty() || ex_tokens.empty()) {
        return 0.0;
    }

    std::size_t intersection = 0;
    for (const auto &t : q_tokens) {
        if (ex_tokens.count(t)) {
            ++intersection;
        }
    }

    std::size_t union_size = q_tokens.size() + ex_tokens.size() - intersection;
    return (union_size > 0) ? static_cast<double>(intersection) / static_cast<double>(union_size) : 0.0;
}

// ============================================================================
// Built-in example registry (30+ curated NL-to-AQL pairs)
// ============================================================================

void AQLFewShotExampleLibrary::registerBuiltins_() {
    // -----------------------------------------------------------------------
    // DOCUMENT examples
    // -----------------------------------------------------------------------
    examples_.push_back({"doc_all_documents",
                         "Find all documents",
                         "FOR d IN documents RETURN d",
                         AQLExampleDomain::DOCUMENT,
                         "Return every document in a collection",
                         {"document", "select", "all"}});
    index_by_id_["doc_all_documents"] = examples_.size() - 1;

    examples_.push_back({"doc_filter_city",
                         "Find all users in Seattle",
                         "FOR u IN users\n  FILTER u.city == \"Seattle\"\n  RETURN u",
                         AQLExampleDomain::DOCUMENT,
                         "Filter documents by a string field",
                         {"document", "filter", "string"}});
    index_by_id_["doc_filter_city"] = examples_.size() - 1;

    examples_.push_back({"doc_sort_limit",
                         "Get the top 10 products sorted by price descending",
                         "FOR p IN products\n  SORT p.price DESC\n  LIMIT 10\n  RETURN p",
                         AQLExampleDomain::DOCUMENT,
                         "Sort and limit results",
                         {"document", "sort", "limit"}});
    index_by_id_["doc_sort_limit"] = examples_.size() - 1;

    examples_.push_back({"doc_filter_numeric",
                         "Find users older than 30",
                         "FOR u IN users\n  FILTER u.age > 30\n  RETURN u",
                         AQLExampleDomain::DOCUMENT,
                         "Filter documents by a numeric comparison",
                         {"document", "filter", "numeric", "comparison"}});
    index_by_id_["doc_filter_numeric"] = examples_.size() - 1;

    examples_.push_back({"doc_insert",
                         "Insert a new user with name Alice and age 25",
                         "INSERT { name: \"Alice\", age: 25 } INTO users RETURN NEW",
                         AQLExampleDomain::DOCUMENT,
                         "Insert a new document",
                         {"document", "insert", "write"}});
    index_by_id_["doc_insert"] = examples_.size() - 1;

    examples_.push_back({"doc_update",
                         "Update the status of order 12345 to shipped",
                         "UPDATE \"12345\" WITH { status: \"shipped\" } IN orders RETURN NEW",
                         AQLExampleDomain::DOCUMENT,
                         "Update a document by key",
                         {"document", "update", "write"}});
    index_by_id_["doc_update"] = examples_.size() - 1;

    examples_.push_back({"doc_remove",
                         "Delete all inactive users",
                         "FOR u IN users\n  FILTER u.active == false\n  REMOVE u IN users",
                         AQLExampleDomain::DOCUMENT,
                         "Remove documents matching a filter",
                         {"document", "delete", "remove", "write"}});
    index_by_id_["doc_remove"] = examples_.size() - 1;

    examples_.push_back({"doc_return_fields",
                         "Get only the name and email of all users",
                         "FOR u IN users\n  RETURN { name: u.name, email: u.email }",
                         AQLExampleDomain::DOCUMENT,
                         "Project specific fields",
                         {"document", "projection", "return"}});
    index_by_id_["doc_return_fields"] = examples_.size() - 1;

    examples_.push_back(
        {"doc_multi_filter",
         "Find active premium users in New York",
         "FOR u IN users\n  FILTER u.active == true AND u.plan == \"premium\" AND u.city == \"New York\"\n  RETURN u",
         AQLExampleDomain::DOCUMENT,
         "Combine multiple filter conditions",
         {"document", "filter", "and", "multi-condition"}});
    index_by_id_["doc_multi_filter"] = examples_.size() - 1;

    // -----------------------------------------------------------------------
    // GRAPH examples
    // -----------------------------------------------------------------------
    examples_.push_back({"graph_outbound",
                         "Find all friends of user 42",
                         "FOR v, e IN 1..1 OUTBOUND \"users/42\" friends RETURN v",
                         AQLExampleDomain::GRAPH,
                         "Simple outbound graph traversal (depth 1)",
                         {"graph", "traversal", "outbound", "edges"}});
    index_by_id_["graph_outbound"] = examples_.size() - 1;

    examples_.push_back({"graph_deep",
                         "Find all nodes reachable from users/1 up to 3 hops",
                         "FOR v IN 1..3 OUTBOUND \"users/1\" edges RETURN v",
                         AQLExampleDomain::GRAPH,
                         "Deep graph traversal with max depth",
                         {"graph", "traversal", "depth", "hops"}});
    index_by_id_["graph_deep"] = examples_.size() - 1;

    examples_.push_back({"graph_shortest_path",
                         "Find the shortest path between users/1 and users/5",
                         "FOR v, e IN OUTBOUND SHORTEST_PATH \"users/1\" TO \"users/5\" edges RETURN v",
                         AQLExampleDomain::GRAPH,
                         "Shortest path query between two nodes",
                         {"graph", "shortest-path", "path"}});
    index_by_id_["graph_shortest_path"] = examples_.size() - 1;

    examples_.push_back({"graph_any_direction",
                         "Find all nodes connected to product/99 in any direction",
                         "FOR v IN 1..2 ANY \"product/99\" related_to RETURN v",
                         AQLExampleDomain::GRAPH,
                         "Bidirectional graph traversal",
                         {"graph", "traversal", "any", "bidirectional"}});
    index_by_id_["graph_any_direction"] = examples_.size() - 1;

    examples_.push_back({"graph_edge_filter",
                         "Find colleagues of user alice with collaboration strength above 5",
                         "FOR v, e IN 1..1 OUTBOUND \"users/alice\" works_with\n  FILTER e.strength > 5\n  RETURN v",
                         AQLExampleDomain::GRAPH,
                         "Graph traversal with edge property filter",
                         {"graph", "traversal", "edge-filter"}});
    index_by_id_["graph_edge_filter"] = examples_.size() - 1;

    examples_.push_back({"graph_with_path",
                         "Show the path from users/1 to users/10 including edge details",
                         "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
                         AQLExampleDomain::GRAPH,
                         "Graph traversal returning the full path object",
                         {"graph", "path", "traversal"}});
    index_by_id_["graph_with_path"] = examples_.size() - 1;

    // -----------------------------------------------------------------------
    // VECTOR examples
    // -----------------------------------------------------------------------
    examples_.push_back({"vector_ann_search",
                         "Find the 5 most similar documents to the given vector",
                         "FOR d IN documents\n  LET score = COSINE_SIMILARITY(d.embedding, @query_vector)\n  SORT "
                         "score DESC\n  LIMIT 5\n  RETURN d",
                         AQLExampleDomain::VECTOR,
                         "Approximate nearest-neighbour vector search by cosine similarity",
                         {"vector", "similarity", "ann", "cosine"}});
    index_by_id_["vector_ann_search"] = examples_.size() - 1;

    examples_.push_back({"vector_hybrid_search",
                         "Find documents similar to query vector that also have category electronics",
                         "FOR d IN documents\n  FILTER d.category == \"electronics\"\n  LET score = "
                         "COSINE_SIMILARITY(d.embedding, @query_vector)\n  SORT score DESC\n  LIMIT 10\n  RETURN d",
                         AQLExampleDomain::VECTOR,
                         "Hybrid vector + attribute filter search",
                         {"vector", "hybrid", "filter", "similarity"}});
    index_by_id_["vector_hybrid_search"] = examples_.size() - 1;

    examples_.push_back({"vector_l2_distance",
                         "Find the 3 nearest neighbours using Euclidean distance",
                         "FOR d IN embeddings\n  LET dist = L2_DISTANCE(d.vector, @query_vector)\n  SORT dist ASC\n  "
                         "LIMIT 3\n  RETURN { doc: d, distance: dist }",
                         AQLExampleDomain::VECTOR,
                         "Nearest-neighbour search using L2 / Euclidean distance",
                         {"vector", "l2", "euclidean", "nearest-neighbour"}});
    index_by_id_["vector_l2_distance"] = examples_.size() - 1;

    examples_.push_back({"vector_dot_product",
                         "Rank articles by relevance to query using dot product similarity",
                         "FOR a IN articles\n  LET score = DOT_PRODUCT(a.embedding, @query_vec)\n  SORT score DESC\n  "
                         "LIMIT 20\n  RETURN { article: a, score }",
                         AQLExampleDomain::VECTOR,
                         "Vector search ranked by dot-product score",
                         {"vector", "dot-product", "ranking"}});
    index_by_id_["vector_dot_product"] = examples_.size() - 1;

    // -----------------------------------------------------------------------
    // GEOSPATIAL examples
    // -----------------------------------------------------------------------
    examples_.push_back(
        {"geo_within_radius",
         "Find all restaurants within 5 km of coordinates 47.6, -122.3",
         "FOR r IN restaurants\n  FILTER GEO_DISTANCE(r.location, GEO_POINT(-122.3, 47.6)) <= 5000\n  RETURN r",
         AQLExampleDomain::GEOSPATIAL,
         "Radius search around a point",
         {"geo", "geospatial", "radius", "distance"}});
    index_by_id_["geo_within_radius"] = examples_.size() - 1;

    examples_.push_back({"geo_sort_nearest",
                         "List the 10 nearest stores to location 51.5, -0.1",
                         "FOR s IN stores\n  LET dist = GEO_DISTANCE(s.location, GEO_POINT(-0.1, 51.5))\n  SORT dist "
                         "ASC\n  LIMIT 10\n  RETURN { store: s, distance_m: dist }",
                         AQLExampleDomain::GEOSPATIAL,
                         "Find and sort nearest locations to a point",
                         {"geo", "nearest", "sort", "distance"}});
    index_by_id_["geo_sort_nearest"] = examples_.size() - 1;

    examples_.push_back({"geo_contains_polygon",
                         "Find all events that fall within the city polygon",
                         "FOR e IN events\n  FILTER GEO_CONTAINS(@city_polygon, e.location)\n  RETURN e",
                         AQLExampleDomain::GEOSPATIAL,
                         "Point-in-polygon containment check",
                         {"geo", "polygon", "contains", "spatial"}});
    index_by_id_["geo_contains_polygon"] = examples_.size() - 1;

    examples_.push_back({"geo_distance_filter",
                         "Find hotels within 2 km of the airport and with rating above 4",
                         "FOR h IN hotels\n  FILTER GEO_DISTANCE(h.location, GEO_POINT(@lon, @lat)) <= 2000\n     AND "
                         "h.rating > 4\n  RETURN h",
                         AQLExampleDomain::GEOSPATIAL,
                         "Combine geospatial radius filter with attribute filter",
                         {"geo", "filter", "radius", "combined"}});
    index_by_id_["geo_distance_filter"] = examples_.size() - 1;

    // -----------------------------------------------------------------------
    // TIMESERIES examples
    // -----------------------------------------------------------------------
    examples_.push_back({"ts_range_query",
                         "Get all sensor readings from the last 24 hours",
                         "FOR r IN sensor_readings\n  FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, \"day\")\n  "
                         "SORT r.timestamp ASC\n  RETURN r",
                         AQLExampleDomain::TIMESERIES,
                         "Time-range query for recent records",
                         {"timeseries", "range", "date", "filter"}});
    index_by_id_["ts_range_query"] = examples_.size() - 1;

    examples_.push_back({"ts_between",
                         "Find all orders placed between January 1 and January 31, 2025",
                         "FOR o IN orders\n  FILTER o.created_at >= \"2025-01-01T00:00:00Z\"\n     AND o.created_at <= "
                         "\"2025-01-31T23:59:59Z\"\n  RETURN o",
                         AQLExampleDomain::TIMESERIES,
                         "Date range between two ISO timestamps",
                         {"timeseries", "between", "date-range"}});
    index_by_id_["ts_between"] = examples_.size() - 1;

    examples_.push_back({"ts_hourly_avg",
                         "Compute hourly average temperature for the past week",
                         "FOR r IN temperatures\n  FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 7, \"day\")\n  "
                         "COLLECT hour = DATE_HOUR(r.timestamp),\n           day  = DATE_DAY(r.timestamp)\n  AGGREGATE "
                         "avg_temp = AVG(r.value)\n  RETURN { day, hour, avg_temp }",
                         AQLExampleDomain::TIMESERIES,
                         "Hourly aggregation of a time-series metric",
                         {"timeseries", "aggregation", "hourly", "average"}});
    index_by_id_["ts_hourly_avg"] = examples_.size() - 1;

    examples_.push_back({"ts_latest_per_device",
                         "Get the most recent reading for each device",
                         "FOR r IN sensor_readings\n  COLLECT device = r.device_id\n  AGGREGATE latest_ts = "
                         "MAX(r.timestamp)\n  FOR lr IN sensor_readings\n    FILTER lr.device_id == device AND "
                         "lr.timestamp == latest_ts\n    LIMIT 1\n    RETURN lr",
                         AQLExampleDomain::TIMESERIES,
                         "Latest value per group in a time-series collection",
                         {"timeseries", "latest", "per-device", "collect"}});
    index_by_id_["ts_latest_per_device"] = examples_.size() - 1;

    // -----------------------------------------------------------------------
    // AGGREGATION examples
    // -----------------------------------------------------------------------
    examples_.push_back({"agg_count_by_group",
                         "Count users by city",
                         "FOR u IN users\n  COLLECT city = u.city WITH COUNT INTO count\n  RETURN { city, count }",
                         AQLExampleDomain::AGGREGATION,
                         "Count documents grouped by a field",
                         {"aggregation", "collect", "count", "group-by"}});
    index_by_id_["agg_count_by_group"] = examples_.size() - 1;

    examples_.push_back({"agg_sum",
                         "Calculate the total revenue per product category",
                         "FOR o IN orders\n  COLLECT category = o.category\n  AGGREGATE total = SUM(o.amount)\n  SORT "
                         "total DESC\n  RETURN { category, total_revenue: total }",
                         AQLExampleDomain::AGGREGATION,
                         "Sum aggregation grouped by a field",
                         {"aggregation", "sum", "group-by", "revenue"}});
    index_by_id_["agg_sum"] = examples_.size() - 1;

    examples_.push_back({"agg_avg_rating",
                         "Calculate the average rating for each restaurant",
                         "FOR r IN reviews\n  COLLECT restaurant = r.restaurant_id\n  AGGREGATE avg_rating = "
                         "AVG(r.rating)\n  RETURN { restaurant, avg_rating }",
                         AQLExampleDomain::AGGREGATION,
                         "Average aggregation with grouping",
                         {"aggregation", "average", "group-by", "rating"}});
    index_by_id_["agg_avg_rating"] = examples_.size() - 1;

    examples_.push_back({"agg_having",
                         "Find cities with more than 100 users",
                         "FOR u IN users\n  COLLECT city = u.city WITH COUNT INTO count\n  FILTER count > 100\n  "
                         "RETURN { city, count }",
                         AQLExampleDomain::AGGREGATION,
                         "HAVING-style filter after aggregation",
                         {"aggregation", "having", "filter", "count"}});
    index_by_id_["agg_having"] = examples_.size() - 1;

    // -----------------------------------------------------------------------
    // GENERAL examples (multi-domain / join-like patterns)
    // -----------------------------------------------------------------------
    examples_.push_back({"gen_subquery_join",
                         "Find users who have at least one order with total above 500",
                         "FOR u IN users\n  LET orders = (\n    FOR o IN orders\n      FILTER o.user_id == u._id AND "
                         "o.total > 500\n      LIMIT 1\n      RETURN o\n  )\n  FILTER LENGTH(orders) > 0\n  RETURN u",
                         AQLExampleDomain::GENERAL,
                         "Subquery-based join pattern",
                         {"subquery", "join", "exists"}});
    index_by_id_["gen_subquery_join"] = examples_.size() - 1;

    examples_.push_back(
        {"gen_nested_return",
         "Get each user with their last 5 orders",
         "FOR u IN users\n  LET recent_orders = (\n    FOR o IN orders\n      FILTER o.user_id == u._id\n      SORT "
         "o.created_at DESC\n      LIMIT 5\n      RETURN o\n  )\n  RETURN { user: u, orders: recent_orders }",
         AQLExampleDomain::GENERAL,
         "Nested subquery to enrich each document with related data",
         {"subquery", "nested", "enrich"}});
    index_by_id_["gen_nested_return"] = examples_.size() - 1;

    examples_.push_back({"gen_fulltext_search",
                         "Search for documents containing the word database in the description",
                         "FOR d IN documents\n  FILTER CONTAINS(d.description, \"database\")\n  RETURN d",
                         AQLExampleDomain::GENERAL,
                         "Text containment search",
                         {"fulltext", "contains", "search", "text"}});
    index_by_id_["gen_fulltext_search"] = examples_.size() - 1;

    examples_.push_back({"gen_array_filter",
                         "Find products that have the tag sale in their tags array",
                         "FOR p IN products\n  FILTER \"sale\" IN p.tags\n  RETURN p",
                         AQLExampleDomain::GENERAL,
                         "Filter documents where an array field contains a value",
                         {"array", "in-array", "filter"}});
    index_by_id_["gen_array_filter"] = examples_.size() - 1;

    examples_.push_back({"gen_distinct",
                         "Get the distinct list of countries from the users collection",
                         "FOR u IN users\n  RETURN DISTINCT u.country",
                         AQLExampleDomain::GENERAL,
                         "DISTINCT values of a field",
                         {"distinct", "unique", "values"}});
    index_by_id_["gen_distinct"] = examples_.size() - 1;
}

} // namespace aql
} // namespace themis
