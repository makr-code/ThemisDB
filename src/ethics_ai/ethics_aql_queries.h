/**
 * @file ethics_aql_queries.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class EthicsAQLQueries {
public:
    /**
     * @brief ========== Argument Queries ==========
     * @return Return value.
     * @details Implements getArgumentById without additional internal calls.
     */
    
    static std::string getArgumentById() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER arg._key == @argument_id
            RETURN arg
        )";
    }
    
    /**
     * @brief Get Arguments By Philosophy.
     * @return Return value.
     * @details Implements getArgumentsByPhilosophy without additional internal calls.
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
     * @brief Get Arguments By Philosophy And Type.
     * @return Return value.
     * @details Implements getArgumentsByPhilosophyAndType without additional internal calls.
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
     * @brief Find Similar Dilemmas.
     * @return Return value.
     * @details Calls: VECTOR_COSINE_SIMILARITY().
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
     * @brief Search Arguments By Content.
     * @return Return value.
     * @details Calls: CONTAINS(), LOWER().
     */
    static std::string searchArgumentsByContent() {
        return R"(
            FOR arg IN ethics_arguments
            FILTER CONTAINS(LOWER(arg.content), LOWER(@search_text))
            LIMIT @limit
            RETURN arg
        )";
    }
    
    /**
     * @brief ========== Graph Traversal Queries ==========
     * @return Return value.
     * @details Calls: LENGTH().
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
     * @brief Get Supporting Arguments.
     * @return Return value.
     * @details Implements getSupportingArguments without additional internal calls.
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
     * @brief Get Countering Arguments.
     * @return Return value.
     * @details Implements getCounteringArguments without additional internal calls.
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
     * @brief Find Shortest Path.
     * @return Return value.
     * @details Implements findShortestPath without additional internal calls.
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
    
    /**
     * @brief ========== Decision Queries ==========
     * @return Return value.
     * @details Implements getDecisionById without additional internal calls.
     */
    
    static std::string getDecisionById() {
        return R"(
            FOR dec IN ethics_decisions
            FILTER dec._key == @decision_id
            RETURN dec
        )";
    }
    
    /**
     * @brief Get Decisions By Category.
     * @return Return value.
     * @details Implements getDecisionsByCategory without additional internal calls.
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
     * @brief Get Recent Debates.
     * @return Return value.
     * @details Implements getRecentDebates without additional internal calls.
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
     * @brief Find Consensus Decisions.
     * @return Return value.
     * @details Implements findConsensusDecisions without additional internal calls.
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
    
    /**
     * @brief ========== Best Practice Queries ==========
     * @return Return value.
     * @details Implements getBestPractices without additional internal calls.
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
     * @brief Get Philosophy Statistics.
     * @return Return value.
     * @details Implements getPhilosophyStatistics without additional internal calls.
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
    
    /**
     * @brief ========== Philosophy Profile Queries ==========
     * @return Return value.
     * @details Implements getPhilosophyProfile without additional internal calls.
     */
    
    static std::string getPhilosophyProfile() {
        return R"(
            FOR profile IN ethics_profiles
            FILTER profile._key == @school
            RETURN profile
        )";
    }
    
    /**
     * @brief List Philosophy Schools.
     * @return Return value.
     * @details Implements listPhilosophySchools without additional internal calls.
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
    
    /**
     * @brief ========== RAG Context Queries ==========
     * @return Return value.
     * @details Calls: VECTOR_COSINE_SIMILARITY().
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

