/**
 * @file graph_validator.h
 * @brief Graph Truth Validation Layer — third layer of the hybrid retrieval stack.
 *
 * Validates tensor-layer candidates against the knowledge graph, assembles
 * evidence chains, and attaches provenance metadata before LLM/LoRA inference.
 *
 * Planned in: docs/EPIC1_GRAPH_VALIDATION.md
 * Sub-issue:   #5426
 */

#pragma once

#include "tensor_midlayer.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::retrieval {

/// Confidence level assigned to a piece of evidence after graph validation.
enum class EvidenceConfidence {
    Unverified,  ///< No graph node found
    Partial,     ///< Reachable but weakly connected
    Strong,      ///< Direct edge or close neighbourhood
    Definitive,  ///< First-class fact asserted in the knowledge base
};

/// A single evidence record returned by the graph layer.
struct GraphEvidence {
    std::uint64_t       candidate_id;
    EvidenceConfidence  confidence;
    std::vector<std::string> supporting_nodes; ///< Graph node IDs on the path
    std::string         provenance_id;         ///< Unique evidence receipt
    double              graph_score = 0.0;     ///< Structural similarity score
};

/// Query descriptor for the graph validator.
struct GraphQuery {
    std::vector<AnnCandidate> candidates; ///< Candidates from the tensor layer
    std::string               query_text; ///< Original natural-language query
    std::uint32_t             max_hops = 3;
    bool                      require_provenance = true;
};

/// Result produced by the graph truth layer.
struct GraphResult {
    std::vector<GraphEvidence> evidence;
    std::vector<AnnCandidate>  validated_candidates; ///< Filtered/reordered
    double                     latency_ms = 0.0;
    bool                       graph_available = true; ///< False on fallback
};

/// Configuration for the graph truth validator.
struct GraphValidatorConfig {
    std::string graph_endpoint;  ///< Connection string or directory
    std::uint32_t max_hops = 3;
    double        min_graph_score = 0.2;
    bool          strict_provenance = false; ///< Reject unverified candidates
    std::size_t   result_cache_ttl_s = 60;
};

/**
 * @brief Graph Truth Validation Layer interface.
 *
 * Validates and enriches retrieval candidates with graph-backed evidence,
 * provenance receipts, and confidence ratings.
 */
class IGraphValidator {
public:
    virtual ~IGraphValidator() = default;

    /// Validate candidates against the knowledge graph.
    virtual GraphResult validate(const GraphQuery& query) = 0;

    /// Return whether the graph backend is reachable.
    virtual bool isAvailable() const = 0;

    /// Look up a single provenance receipt by ID.
    virtual std::optional<GraphEvidence> lookupProvenance(
        const std::string& provenance_id) const = 0;
};

/// Factory: create a GraphValidator from configuration.
std::unique_ptr<IGraphValidator> makeGraphValidator(const GraphValidatorConfig& cfg);

} // namespace themis::retrieval
