/**
 * @file ethics_aql_queries.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief AQL Query Templates for Ethics AI Plugin
 * 
 * This class provides AQL query templates that work with ThemisDB's
 * BaseEntity model and QueryEngine. All data is stored as BaseEntity
 * instances and queried using AQL.
 * 
 * Collections:
 * - ethics_arguments: Ethical arguments as BaseEntity
 * - ethics_decisions: Decision records
 * - ethics_debates: Debate sessions
 * - ethics_profiles: Philosophy profiles
 */
class EthicsAQLQueries {
public:
    // ========== Argument Queries ==========
    
    /**
     * @brief Get argument by ID
     */
    static std::string getArgumentById() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER arg._key == @argument_id
            RETURN arg
        )";
    }
    
    /**
     * @brief Get arguments by philosophy school
     * Parameters: `school`, `limit`
     */
    static std::string getArgumentsByPhilosophy() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER arg.philosophy_school == @school
            LIMIT @limit
            RETURN arg
        )";
    }
    
    /**
     * @brief Get arguments by philosophy with type filter
     * Parameters: `school`, `argument_types` (array), `limit`
     */
    static std::string getArgumentsByPhilosophyAndType() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER arg.philosophy_school == @school
            FILTER arg.argument_type IN @argument_types
            LIMIT @limit
            RETURN arg
        )";
    }
    
    /**
     * @brief Find similar dilemmas using vector similarity
     * Parameters: `query_vector`, `threshold`, `limit`
     */
    static std::string findSimilarDilemmas() {
        return R"(
            FOR doc IN ethics_dilemmas
            LET similarity = VECTOR_COSINE_SIMILARITY(doc.embedding, @query_vector)
            FILTER similarity >= @threshold
            SORT similarity DESC
            LIMIT @limit
            RETURN {
                id: doc._key,
                description: doc.description,
                category: doc.category,
                similarity: similarity
            }
        )";
    }
    
    /**
     * @brief Search arguments by content (fulltext)
     * Parameters: `search_text`, `limit`
     */
    static std::string searchArgumentsByContent() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER CONTAINS(LOWER(arg.content), LOWER(@search_text))
            LIMIT @limit
            RETURN arg
        )";
    }
    
    // ========== Graph Traversal Queries ==========
    
    /**
     * @brief Traverse argument chains (graph traversal)
     * Parameters: `start_id`, `max_depth`
     * 
     * Note: Requires graph edges stored as:
     * - FROM: ethics_arguments/arg_001
     * - TO: ethics_arguments/arg_002
     * - Edge type: "supports", "counters", "rebuts", "synthesizes"
     */
    static std::string traverseArgumentChain() {
        return R"(
            FOR v, e, p IN 1..@max_depth OUTBOUND @start_id
            GRAPH 'ethics_arguments_graph'
            RETURN {
                vertex: v,
                edge: e,
                path: p.vertices[*]._key,
                depth: LENGTH(p.vertices) - 1
            }
        )";
    }
    
    /**
     * @brief Get supporting arguments (outbound "supports" edges)
     * Parameters: `argument_id`
     */
    static std::string getSupportingArguments() {
        return R"(
            FOR v IN 1..1 OUTBOUND @argument_id
            GRAPH 'ethics_arguments_graph'
            FILTER e._type == 'supports'
            RETURN v
        )";
    }
    
    /**
     * @brief Get countering arguments (outbound "counters" edges)
     * Parameters: `argument_id`
     */
    static std::string getCounteringArguments() {
        return R"(
            FOR v IN 1..1 OUTBOUND @argument_id
            GRAPH 'ethics_arguments_graph'
            FILTER e._type == 'counters'
            RETURN v
        )";
    }
    
    /**
     * @brief Find shortest path between two arguments
     * Parameters: `start_id`, `end_id`, `max_depth`
     */
    static std::string findShortestPath() {
        return R"(
            FOR v, e IN OUTBOUND SHORTEST_PATH
            @start_id TO @end_id
            GRAPH 'ethics_arguments_graph'
            OPTIONS {maxDepth: @max_depth}
            RETURN {vertex: v, edge: e}
        )";
    }
    
    // ========== Decision Queries ==========
    
    /**
     * @brief Get decision by ID
     * Parameters: `decision_id`
     */
    static std::string getDecisionById() {
        return R"(
            FOR dec IN ethics_decisions
            FILTER dec._key == @decision_id
            RETURN dec
        )";
    }
    
    /**
     * @brief Get decisions by category
     * Parameters: `category`, `min_confidence`, `limit`
     */
    static std::string getDecisionsByCategory() {
        return R"(
            FOR dec IN ethics_decisions
            FILTER dec.category == @category
            FILTER dec.confidence >= @min_confidence
            SORT dec.created_at DESC
            LIMIT @limit
            RETURN dec
        )";
    }
    
    /**
     * @brief Get recent debates by category
     * Parameters: `category`, `since_timestamp`, `limit`
     */
    static std::string getRecentDebates() {
        return R"(
            FOR debate IN ethics_debates
            FILTER debate.category == @category
            FILTER debate.created_at >= @since_timestamp
            SORT debate.created_at DESC
            LIMIT @limit
            RETURN debate
        )";
    }
    
    /**
     * @brief Find consensus decisions (high agreement among philosophies)
     * Parameters: `min_consensus`, `limit`
     */
    static std::string findConsensusDecisions() {
        return R"(
            FOR dec IN ethics_decisions
            FILTER dec.consensus_level >= @min_consensus
            SORT dec.consensus_level DESC, dec.created_at DESC
            LIMIT @limit
            RETURN dec
        )";
    }
    
    // ========== Best Practice Queries ==========
    
    /**
     * @brief Get best practice arguments (high quality + satisfaction)
     * Parameters: `min_quality`, `min_satisfaction`, `limit`
     */
    static std::string getBestPractices() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER arg.quality_score >= @min_quality
            FILTER arg.satisfaction_score >= @min_satisfaction
            SORT arg.quality_score DESC
            LIMIT @limit
            RETURN arg
        )";
    }
    
    /**
     * @brief Aggregate argument statistics by philosophy
     */
    static std::string getPhilosophyStatistics() {
        return R"(
            FOR arg IN ethics_arguments
            COLLECT school = arg.philosophy_school WITH COUNT INTO total
            RETURN {
                philosophy_school: school,
                argument_count: total
            }
        )";
    }
    
    // ========== Philosophy Profile Queries ==========
    
    /**
     * @brief Get philosophy profile by school
     * Parameters: `school`
     */
    static std::string getPhilosophyProfile() {
        return R"(
            FOR profile IN ethics_profiles
            FILTER profile._key == @school
            RETURN profile
        )";
    }
    
    /**
     * @brief List all philosophy schools
     */
    static std::string listPhilosophySchools() {
        return R"(
            FOR profile IN ethics_profiles
            RETURN {
                school: profile._key,
                name: profile.name,
                founder: profile.founder
            }
        )";
    }
    
    // ========== RAG Context Queries ==========
    
    /**
     * @brief Build RAG context: similar dilemmas + philosophy arguments + best practices
     * Parameters: `query_vector`, `schools` (array), `category`, `limit`
     */
    static std::string buildRAGContext() {
        return R"(
            LET similar_dilemmas = (
                FOR doc IN ethics_dilemmas
                LET similarity = VECTOR_COSINE_SIMILARITY(doc.embedding, @query_vector)
                FILTER similarity >= 0.65
                SORT similarity DESC
                LIMIT 5
                RETURN doc
            )
            
            LET philosophy_args = (
                FOR school IN @schools
                    FOR arg IN ethics_arguments
                    FILTER arg.philosophy_school == school
                    LIMIT 3
                    RETURN arg
            )
            
            LET best_practices = (
                FOR arg IN ethics_arguments
                FILTER arg.quality_score >= 0.8
                LIMIT 5
                RETURN arg
            )
            
            LET recent_debates = (
                FOR debate IN ethics_debates
                FILTER debate.category == @category
                SORT debate.created_at DESC
                LIMIT 5
                RETURN debate
            )
            
            LET consensus = (
                FOR dec IN ethics_decisions
                FILTER dec.consensus_level >= 0.7
                LIMIT 5
                RETURN dec
            )
            
            RETURN {
                similar_dilemmas: similar_dilemmas,
                philosophy_arguments: philosophy_args,
                best_practices: best_practices,
                recent_debates: recent_debates,
                consensus_decisions: consensus
            }
        )";
    }
};

} // namespace ethics
} // namespace plugins
} // namespace themis

