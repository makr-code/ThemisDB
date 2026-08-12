/**
 * @file process_community_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_community_detector.h
 * Module:  include/process/
 * Purpose: Leiden-style greedy modularity-based community detection
 *          over process model graphs (P4 – GraphRAG, Edge 2024).
 */

#pragma once

#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// ProcessCommunity
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Represents a detected community of related process nodes.
 *
 * Communities are identified by greedy modularity optimisation (Louvain-style)
 * and can be serialised to RocksDB for later use by the LightRAG retrieval
 * layer.
 */
struct ProcessCommunity {
    std::string community_id;            ///< "community_<N>"
    std::vector<std::string> node_ids;   ///< IDs of nodes in the community
    std::string label;                   ///< Short label (first 3 node names joined)
    std::string report;                  ///< Full report (LLM stub or node descriptions)
    float modularity_score{0.f};         ///< Local modularity contribution
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessCommunityDetector
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Detects semantic communities in process model graphs.
 *
 * Uses a two-phase Louvain-style greedy modularity optimisation algorithm:
 *   - Phase 1: Local node reassignment to maximise modularity gain.
 *   - Phase 2: Coarsening – build super-nodes and repeat Phase 1.
 *
 * The `resolution` parameter scales the null-model term @c ki*kj/(2m) to
 * produce finer (resolution > 1.0) or coarser (resolution < 1.0) partitions.
 *
 * @par Performance target
 * @c detect() must complete in < 500 ms for 500-node graphs.
 *
 * @par Storage key scheme
 * Communities are persisted as:
 * @code
 *   proc:community:<model_id>:<community_id>  →  JSON
 * @endcode
 */
class ProcessCommunityDetector {
public:
    /**
     * @brief Construct a ProcessCommunityDetector with a RocksDB backend.
     *
     * @param db Reference to a RocksDBWrapper instance that will be used for storing and
     *           retrieving detected community data. The ProcessCommunityDetector does not own
     *           this reference; the caller is responsible for keeping the RocksDBWrapper
     *           alive for the entire lifetime of this detector instance.
     *
     * @note Thread-safe: Multiple ProcessCommunityDetector instances can be created with the
     *       same RocksDB backend, and their operations will properly synchronize via RocksDB's
     *       internal locking mechanisms.
     *
     * @note The constructor does not perform any I/O; community detection is triggered
     *       lazily when detect() is called.
     *
     * @see detect() to run community detection on a process model
     * @see generateReport() to create human-readable community descriptions
     */
    explicit ProcessCommunityDetector(RocksDBWrapper& db);

    /**
     * @brief Run greedy modularity-based community detection on a process model.
     *
     * Loads the @c ProcessModelRecord from RocksDB (key @c proc:model:<model_id>),
     * extracts its @c normalized["nodes"] and @c normalized["edges"], builds an
     * adjacency list, and runs the Louvain-style algorithm.
     *
     * @param model_id    Process model identifier.
     * @param resolution  Resolution parameter (default 1.0).  Values > 1.0
     *                    favour smaller communities; < 1.0 favour larger ones.
     * @return Detected communities sorted by size descending.
     */
    [[nodiscard]] std::vector<ProcessCommunity> detect(
        std::string_view model_id,
        float resolution = 1.0f
    ) const;

    /**
     * @brief Generate a text report for a community.
     *
     * @note
     * @code
     * // STUB/SIMULATION NOTE:
     * // Purpose: Generates community report without a real LLM call
     * // Activation: Always (LLM endpoint integration not yet implemented)
     * // Production Delta: Real implementation calls llm_endpoint with community
     * //                   node descriptions
     * // Removal Plan: Replace with HTTP LLM call when LLM integration is wired (Q4 2026)
     * @endcode
     *
     * @param community     The community to describe.
     * @param model_id      Model the community belongs to.
     * @param llm_endpoint  Ignored in the stub – reserved for LLM URL.
     * @param language      Output language ("de" or "en").
     * @return Text report built from node names/descriptions.
     */
    [[nodiscard]] std::string generateReport(
        const ProcessCommunity& community,
        std::string_view model_id,
        std::string_view llm_endpoint,
        std::string_view language = "de"
    ) const;

    /**
     * @brief Persist a list of communities to RocksDB.
     *
     * Each community is stored under:
     * @code
     *   proc:community:<model_id>:<community_id>
     * @endcode
     *
     * @return true if all communities were stored successfully.
     */
    bool persistCommunities(
        std::string_view model_id,
        const std::vector<ProcessCommunity>& communities
    );

    /**
     * @brief Load all communities for a model from RocksDB.
     *
     * @return All communities for @p model_id; empty vector if none found.
     */
    [[nodiscard]] std::vector<ProcessCommunity> loadCommunities(
        std::string_view model_id
    ) const;

private:
    RocksDBWrapper& db_;
};

} // namespace process
} // namespace themis
