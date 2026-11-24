/**
 * @file distributed_planner.h
 * @brief Distributed tensor execution planner.
 *
 * Produces placement-aware execution plans for tensor operations across
 * a heterogeneous cluster, respecting bandwidth, memory, and affinity constraints.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/graph_summary_projection.h"
#include "tensor/tensor_summary_types.h"
#include "rag/graph_truth_validator.h"

#include <algorithm>
#include <cmath>
#include <exception>

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace distributed_tensor {


enum class RetrievalStrategy {
	SUMMARY_FIRST,
	EXACT,
	APPROXIMATE,
	LAZY,
	CACHED,
};

// Default planner implementation (declaration will follow after type definitions)

enum class RetrievalLocation {
	LOCAL_CACHE,
	HOT_TIER,
	WARM_TIER,
	COLD_TIER,
	ANY_TIER,
};

struct DistributedRetrievalPlan {
	std::string artifact_id;
	RetrievalStrategy retrieval_strategy = RetrievalStrategy::EXACT;
	RetrievalLocation retrieval_location = RetrievalLocation::ANY_TIER;
	std::vector<std::string> shard_ids_to_retrieve;
	bool parallel_retrieval = true;
	uint32_t max_parallel_streams = 4;
	uint64_t estimated_retrieval_time_ms = 0;
	float estimated_bandwidth_mbps = 100.0f;
	bool can_execute = true;
	bool degraded_mode = false;
	std::string execution_rationale;
	std::unordered_map<std::string, std::string> custom_parameters;
};

struct TensorDependency {
	std::string artifact_id;
	ArtifactLifecycleStage required_lifecycle_stage = ArtifactLifecycleStage::ACTIVE;
	bool is_required = true;
	uint32_t max_staleness_seconds = 0;
	bool allow_approximation = false;
};

// Default planner implementation (declaration only — implementation in .cc)
class DefaultDistributedTensorPlanner {
 public:
  DefaultDistributedTensorPlanner() = default;
  ~DefaultDistributedTensorPlanner() = default;

  DistributedRetrievalPlan plan_tensor_retrieval(
	  const ArtifactManifest& manifest,
	  const std::vector<TensorDependency>& dependencies,
	  RetrievalLocation preferred_location = RetrievalLocation::ANY_TIER) const noexcept;

  /**
   * @brief Optimize retrieval plan.
   * @param[in] plan Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  DistributedRetrievalPlan optimize_retrieval_plan(const DistributedRetrievalPlan& plan) const noexcept;

  bool is_tensor_available(const ArtifactManifest& manifest,
						   ArtifactLifecycleStage required_stage = ArtifactLifecycleStage::ACTIVE) const noexcept;

  /**
   * @brief Estimate retrieval cost.
   * @param[in] manifest Input parameter.
   * @param[in] strategy Input parameter.
   * @param[in] location Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  uint64_t estimate_retrieval_cost(const ArtifactManifest& manifest,
								   RetrievalStrategy strategy,
								   RetrievalLocation location) const noexcept;

 private:
  /**
   * @brief Select retrieval strategy.
   * @param[in] manifest Input parameter.
   * @param[in] dependencies Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  RetrievalStrategy select_retrieval_strategy(const ArtifactManifest& manifest,
											  const std::vector<TensorDependency>& dependencies) const noexcept;

  /**
   * @brief Calculate retrieval time.
   * @param[in] data_size Input parameter.
   * @param[in] bandwidth_mbps Input parameter.
   * @param[in] parallel_streams Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  uint64_t calculate_retrieval_time(uint64_t data_size,
									float bandwidth_mbps,
									uint32_t parallel_streams) const noexcept;
};

// Forward declarations for pluggable interfaces
class IFragmentFetcher;

struct RoutingSummary {
	std::string shard_id;
	tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::FRESH;
	float shard_relevance = 0.0f;
	float routing_confidence = 0.8f;
	std::string routing_reason;
	bool rejected_as_stale = false;
	bool rejected_as_unhealthy = false;
};

/**
 * @brief Identity, integrity, and resource contract for one exact shard read.
 */
struct FragmentLoadRequest {
	/// Stable shard identifier; must match the manifest placement and RPC frames.
	std::string shard_id;
	/// Manifest artifact identity; never a filesystem path.
	std::string artifact_id;
	bool expedited = false;
	/// Positive transport deadline in milliseconds.
	uint32_t timeout_ms = 5000;
	std::string codec_hint;
	/// Configured peer node ID selected by the artifact manifest placement.
	std::string target_node_id;
	/// Trusted lowercase SHA-256 digest from the manifest placement.
	std::string expected_content_hash;
	/// Maximum response size; defaults to 64 MiB and must remain bounded.
	uint64_t max_response_bytes = 64ULL * 1024ULL * 1024ULL;
};

struct FragmentLoadResult {
	std::string shard_id;
	std::string artifact_id;
	bool success = false;
	std::string error_reason;
	std::vector<uint8_t> fragment_data;
	std::string content_hash;
	float load_latency_ms = 0.0f;
	std::string loaded_at;
};

struct TensorRetrievalPlan {
	std::string correlation_id;
	std::vector<RoutingSummary> routing_summaries;
	std::vector<FragmentLoadRequest> fragment_requests;
	std::vector<FragmentLoadResult> fragment_results;
	bool all_summaries_fresh = true;
	std::size_t stale_shards_rejected = 0;
	/// Set only by the mutable graph-validation overload after sufficient graph evidence.
	bool passed_graph_validation = false;
	rag::GraphTruthValidationResult validation_result;
	std::string plan_reason;
	std::string fallback_routing;
	bool is_fallback = false;
};

/**
 * @brief Fetches complete shard fragments for manifest-bound planner requests.
 * @note Implementations must verify the returned bytes against the request's
 *       expected digest before marking a result successful. Batch results must
 *       preserve request order so the planner can bind each result to its
 *       corresponding request.
 */
class IFragmentFetcher {
 public:
	/**
	 * @brief Fetches exact bytes for one manifest-bound request.
	 * @param request Request containing the selected shard, artifact, peer,
	 *        expected manifest SHA-256, deadline, and response-size ceiling.
	 * @param correlation_id Optional identifier propagated to the transport.
	 * @return Result preserving request identity; success is true only after
	 *         complete transport and byte-digest verification; failures set
	 *         success=false and report an error reason.
	 * @note Failed bytes must not be exposed as trusted projection input.
	 */
	virtual ~IFragmentFetcher() = default;
	[[nodiscard]] virtual FragmentLoadResult fetchFragment(
		const FragmentLoadRequest& request,
		const std::string& correlation_id = {}) const noexcept = 0;

	/**
	 * @brief Fetches a batch of manifest-bound fragments.
	 * @param requests Ordered requests; each returned result corresponds to the
	 *        request at the same index.
	 * @param correlation_id Optional identifier propagated to every request.
	 * @return One result per input request, preserving input order; individual
	 *         transport or verification failures remain unsuccessful results.
	 */
	[[nodiscard]] virtual std::vector<FragmentLoadResult> fetchFragments(
		const std::vector<FragmentLoadRequest>& requests,
		const std::string& correlation_id = {}) const noexcept = 0;
};

class DistributedTensorPlanner {
 public:
	struct Config {
		uint32_t max_summary_ttl_seconds = 3600;
		float min_fragment_success_rate = 0.8f;
		bool enable_fallback_routing = true;
		uint32_t fragment_load_timeout_ms = 10000;
		bool strict_graph_validation = true;
	};

	DistributedTensorPlanner() = default;
	~DistributedTensorPlanner() = default;

	void setFragmentFetcher(std::shared_ptr<IFragmentFetcher> fetcher) noexcept { fragment_fetcher_ = std::move(fetcher); }
	void setGraphValidator(std::shared_ptr<rag::GraphTruthValidator> validator) noexcept { graph_validator_ = std::move(validator); }

	std::vector<RoutingSummary> planSummaryFirstRouting(
			const std::vector<tensor::ShardSummary>& summaries,
			const std::string& correlation_id = {},
			uint32_t freshness_ttl_s = 3600) noexcept {
		(void)correlation_id;
		(void)freshness_ttl_s;
		std::vector<RoutingSummary> out = {};

		out.reserve(summaries.size());
		for (const auto& s : summaries) {
			RoutingSummary r;
			r.shard_id = s.shard_id;
			r.freshness_state = s.freshness_state;
			r.shard_relevance = s.shard_relevance;
			r.routing_confidence = std::clamp(s.shard_relevance, 0.0f, 1.0f);
			r.routing_reason = "Summary-first routing";
			r.rejected_as_stale = (s.freshness_state != tensor::SummaryFreshnessState::FRESH);
			r.rejected_as_unhealthy = !s.shard_healthy;
			out.push_back(std::move(r));
		}
		return out;
	}

	std::vector<FragmentLoadRequest> planExactOnDemandLoading(
			const std::vector<RoutingSummary>& routing_summaries,
			const std::string& query_context = {},
			const std::string& correlation_id = {}) noexcept {
		(void)query_context;
		(void)correlation_id;
		std::vector<FragmentLoadRequest> reqs = {};

		for (const auto& r : routing_summaries) {
			if (r.rejected_as_stale || r.rejected_as_unhealthy) {
			  continue;
			}
			FragmentLoadRequest req;
			req.shard_id = r.shard_id;
			req.artifact_id = "";
			req.expedited = r.shard_relevance >= 0.9f;
			req.timeout_ms = config_.fragment_load_timeout_ms;
			reqs.push_back(std::move(req));
		}
		return reqs;
	}

	TensorRetrievalPlan buildRetrievalPlan(
			const std::vector<tensor::ShardSummary>& summaries,
			const std::string& query_context = {},
			const std::string& correlation_id = {}) noexcept {
		(void)query_context;
		TensorRetrievalPlan plan;
		plan.correlation_id = correlation_id;
		plan.routing_summaries = planSummaryFirstRouting(summaries, correlation_id, config_.max_summary_ttl_seconds);
		plan.fragment_requests = planExactOnDemandLoading(plan.routing_summaries, query_context, correlation_id);
		plan.stale_shards_rejected = 0;
		for (const auto& r : plan.routing_summaries) {
		  if (r.rejected_as_stale) ++plan.stale_shards_rejected;
		}
		plan.plan_reason = "Built by DistributedTensorPlanner";
		return plan;
	}

	/**
	 * @brief Build exact-load requests bound to a complete artifact manifest.
	 * @param summaries Summary-first routing candidates.
	 * @param manifest Valid manifest whose placement, peer, and SHA-256 digest
	 *        are bound to every requested shard.
	 * @param query_context Opaque caller context retained for API compatibility.
	 * @param correlation_id Identifier propagated to the transport.
	 * @return A plan with manifest-bound requests, or no requests and an explicit
	 *         reason when the manifest or any selected placement is invalid.
	 * @note No network request is permitted for an incomplete manifest or when
	 *       any selected summary lacks a complete placement.
	 */
	[[nodiscard]] TensorRetrievalPlan buildRetrievalPlan(
			const std::vector<tensor::ShardSummary>& summaries,
			const ArtifactManifest& manifest,
			const std::string& query_context = {},
			const std::string& correlation_id = {}) noexcept {
		auto plan = buildRetrievalPlan(summaries, query_context, correlation_id);
		if (!manifest.is_complete()) {
			plan.fragment_requests.clear();
			plan.plan_reason = "Artifact manifest is invalid or incomplete";
			return plan;
		}

		for (auto& request : plan.fragment_requests) {
			const auto placement = manifest.get_shard_placement(request.shard_id);
			if (!placement || placement->node_id.empty() ||
				placement->shard_content_hash.empty()) {
				plan.fragment_requests.clear();
				plan.plan_reason =
					"Selected shard has no complete manifest placement";
				return plan;
			}
			request.artifact_id = manifest.artifact_id();
			request.target_node_id = placement->node_id;
			request.expected_content_hash = placement->shard_content_hash;
			if (placement->shard_size_bytes > 0) {
				request.max_response_bytes = placement->shard_size_bytes;
			}
		}
		plan.plan_reason = "Built with manifest-bound shard identities and digests";
		return plan;
	}

	std::vector<FragmentLoadResult> executeFragmentLoads(const std::vector<FragmentLoadRequest>& requests, const std::string& correlation_id = {}) noexcept {
		if (!fragment_fetcher_) {
			std::vector<FragmentLoadResult> results = {};

			for (const auto& req : requests) {
				FragmentLoadResult r;
				r.shard_id = req.shard_id;
				r.artifact_id = req.artifact_id;
				r.success = false;
				r.error_reason = "No fragment fetcher configured";
				results.push_back(std::move(r));
			}
			return results;
		}
		return fragment_fetcher_->fetchFragments(requests, correlation_id);
	}

	/**
	 * @brief Project verified fragments and validate their candidates against graph truth.
	 * @param plan Retrieval plan with one complete, manifest-bound result per request.
	 * @param query Query used by the graph-truth layer.
	 * @param config Graph-truth validation policy.
	 * @return Validation result, or a structured fail-closed reason when identity,
	 *         digest metadata, versioned projection, or graph evidence is invalid.
	 * @note Only the explicit adapter-similarity/v1 payload format is accepted.
	 *       Transport success alone is insufficient; every returned evidence item
	 *       must be validated, meet the graph-score threshold, and name supporting
	 *       graph nodes before the plan is considered graph-validated.
	 */
	[[nodiscard]] rag::GraphTruthValidationResult validateAgainstGraphTruth(
		const TensorRetrievalPlan& plan,
		const std::string& query,
		const rag::GraphTruthValidatorConfig& config = {}) const noexcept {
		const auto failure = [&plan](std::string reason, std::string code) {
			rag::GraphTruthValidationResult result;
			result.routing_reason = std::move(reason);
			result.routing_reason_code = std::move(code);
			result.correlation_id = plan.correlation_id;
			result.fallback_mode = "FAIL_CLOSED";
			result.fallback_reason_code = result.routing_reason_code;
			return result;
		};

		if (!graph_validator_) {
			return failure("No graph validator configured", "NO_VALIDATOR");
		}
		if (plan.fragment_requests.empty() || plan.fragment_results.empty()) {
			return failure("No complete fragment results available for graph validation",
				"NO_FRAGMENTS");
		}
		if (plan.fragment_requests.size() != plan.fragment_results.size() ||
			plan.fragment_requests.size() > GraphSummaryProjectionCodec::kMaxCandidateCount) {
			return failure("Fragment requests and results are incomplete or exceed the projection limit",
				"FRAGMENT_RESULT_SET_INVALID");
		}
		if (query.empty() || config.max_evidence_candidates == 0 ||
			config.max_evidence_candidates > GraphSummaryProjectionCodec::kMaxCandidateCount ||
			!std::isfinite(config.min_graph_truth_score) ||
			config.min_graph_truth_score < 0.0 ||
			config.min_graph_truth_score > 1.0) {
			return failure("Graph truth validation input or configuration is invalid",
				"INVALID_GRAPH_VALIDATION_CONFIG");
		}

		try {
			std::vector<tensor::TensorLayerSummary> summaries;
			summaries.reserve(plan.fragment_results.size());
			std::set<std::pair<std::string, std::string>> seen_fragments;
			std::string artifact_id;

			for (std::size_t index = 0; index < plan.fragment_requests.size(); ++index) {
				const auto& request = plan.fragment_requests[index];
				const auto& result = plan.fragment_results[index];
				const bool valid_sha256 =
					request.expected_content_hash.size() == 64 &&
					std::all_of(request.expected_content_hash.begin(),
						request.expected_content_hash.end(), [](char value) {
							return (value >= '0' && value <= '9') ||
								(value >= 'a' && value <= 'f');
						});
				if (request.shard_id.empty() || request.artifact_id.empty() ||
					request.target_node_id.empty() ||
					!valid_sha256 || request.max_response_bytes == 0) {
					return failure("Fragment request lacks a complete manifest identity or SHA-256",
						"FRAGMENT_REQUEST_UNBOUND");
				}
				if (artifact_id.empty()) {
					artifact_id = request.artifact_id;
				} else if (artifact_id != request.artifact_id) {
					return failure("Graph projection cannot merge fragments from different artifacts",
						"FRAGMENT_ARTIFACT_MISMATCH");
				}
				if (!seen_fragments.emplace(request.shard_id, request.artifact_id).second) {
					return failure("Duplicate fragment identity in graph validation plan",
						"DUPLICATE_FRAGMENT");
				}
				if (!result.success || result.shard_id != request.shard_id ||
					result.artifact_id != request.artifact_id) {
					return failure("Fragment result does not match its manifest-bound request",
						"FRAGMENT_RESULT_IDENTITY_MISMATCH");
				}
				if (result.content_hash != request.expected_content_hash) {
					return failure("Fragment result does not report the verified manifest digest",
						"FRAGMENT_DIGEST_UNVERIFIED");
				}
				if (result.fragment_data.empty() ||
					result.fragment_data.size() > request.max_response_bytes) {
					return failure("Fragment payload is empty or exceeds its requested byte limit",
						"FRAGMENT_PAYLOAD_SIZE_INVALID");
				}

				auto decoded = GraphSummaryProjectionCodec::decode(
					std::span<const uint8_t>(result.fragment_data.data(),
						result.fragment_data.size()));
				if (!decoded.ok()) {
					const auto code =
						"FRAGMENT_PROJECTION_" +
						std::string(GraphSummaryProjectionCodec::errorCode(decoded.error));
					return failure("Fragment graph projection could not be decoded",
						code);
				}
				summaries.push_back(std::move(*decoded.summary));
			}

			const auto projected = GraphSummaryProjectionCodec::merge(
				std::span<const tensor::TensorLayerSummary>(summaries));
			if (!projected.ok()) {
				const auto code =
					"FRAGMENT_PROJECTION_" +
					std::string(GraphSummaryProjectionCodec::errorCode(projected.error));
				return failure("Fragment graph projections could not be merged",
					code);
			}

			auto validation = graph_validator_->validate(
				query, *projected.summary, config, plan.correlation_id);
			if (!hasSufficientGraphEvidence(validation,
					config.min_graph_truth_score)) {
				validation.routing_reason =
					"No graph-backed evidence met the configured score threshold";
				validation.routing_reason_code = "GRAPH_TRUTH_EVIDENCE_INSUFFICIENT";
				validation.fallback_mode = "FAIL_CLOSED";
				validation.fallback_reason_code =
					"GRAPH_TRUTH_EVIDENCE_INSUFFICIENT";
			}
			return validation;
		} catch (const std::exception&) {
			return failure("Graph truth validation failed with an exception",
				"GRAPH_VALIDATION_EXCEPTION");
		} catch (...) {
			return failure("Graph truth validation failed with a non-standard exception",
				"GRAPH_VALIDATION_EXCEPTION");
		}
	}

	/**
	 * @brief Validate graph truth and persist the outcome on a mutable plan.
	 * @param plan Plan to update with the validation result and pass state.
	 * @param query Query used by the graph-truth layer.
	 * @param config Graph-truth validation policy.
	 * @return The same structured result returned by the const overload.
	 * @note passed_graph_validation is true only when every returned evidence
	 *       item has graph support and meets the configured threshold. Failure
	 *       resets that flag and is stored in validation_result.
	 */
	[[nodiscard]] rag::GraphTruthValidationResult validateAgainstGraphTruth(
		TensorRetrievalPlan& plan,
		const std::string& query,
		const rag::GraphTruthValidatorConfig& config = {}) const noexcept {
		try {
			auto result = validateAgainstGraphTruth(
				static_cast<const TensorRetrievalPlan&>(plan), query, config);
			plan.passed_graph_validation =
				hasSufficientGraphEvidence(result, config.min_graph_truth_score);
			plan.validation_result = result;
			return result;
		} catch (...) {
			plan.passed_graph_validation = false;
			rag::GraphTruthValidationResult result;
			result.routing_reason = "Graph validation result could not be stored";
			result.routing_reason_code = "GRAPH_VALIDATION_RESULT_STORAGE_FAILED";
			result.correlation_id = plan.correlation_id;
			result.fallback_mode = "FAIL_CLOSED";
			result.fallback_reason_code = result.routing_reason_code;
			plan.validation_result = result;
			return result;
		}
	}

	bool isValidFinalPlan(const TensorRetrievalPlan& plan) const noexcept {
		if (plan.fragment_requests.empty() ||
		  plan.fragment_results.size() != plan.fragment_requests.size()) {
		  return false;
		}
		if (config_.strict_graph_validation &&
			(!plan.passed_graph_validation ||
			 !hasSufficientGraphEvidence(plan.validation_result, 0.0))) {
		  return false;
		}
		for (std::size_t i = 0; i < plan.fragment_requests.size(); ++i) {
		  const auto& request = plan.fragment_requests[i];
		  const auto& result = plan.fragment_results[i];
		  if (!result.success || result.shard_id != request.shard_id ||
			  result.artifact_id != request.artifact_id) {
			  return false;
		  }
		}
		return true;
	}

	void setConfig(const Config& c) noexcept { config_ = c; }
	Config config() const noexcept { return config_; }

 private:
	[[nodiscard]] static bool hasSufficientGraphEvidence(
		const rag::GraphTruthValidationResult& result,
		double minimum_graph_score) noexcept {
		return !result.evidences.empty() &&
			std::all_of(result.evidences.begin(), result.evidences.end(),
				[minimum_graph_score](const auto& evidence) {
					return evidence.validated &&
						std::isfinite(evidence.graph_score) &&
						evidence.graph_score >= minimum_graph_score &&
						!evidence.supporting_nodes.empty();
				});
	}

	std::shared_ptr<IFragmentFetcher> fragment_fetcher_;
	std::shared_ptr<rag::GraphTruthValidator> graph_validator_;
	Config config_;
};

// End of DistributedTensorPlanner

}  // namespace distributed_tensor
}  // namespace themis
