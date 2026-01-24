// Cost-based Query Optimizer implementation

#include "query/query_optimizer.h"
#include "query/adaptive_optimizer.h"
#include "index/secondary_index.h"
#include "storage/base_entity.h"
#include "analytics/nlp_text_analyzer.h"
#include "utils/expected.h"

#include <algorithm>
#include <numeric>
#include <memory>
#include <cmath>
#include <thread>

namespace themis {

// Lazy-initialized NLP analyzer (thread-safe in C++11+)
static themis::analytics::NlpTextAnalyzer& getOptimizerNlp() {
    static themis::analytics::NlpTextAnalyzer instance;
    return instance;
}

QueryOptimizer::QueryOptimizer(SecondaryIndexManager& secIdx) : secIdx_(secIdx) {}

QueryOptimizer::Plan QueryOptimizer::chooseOrderForAndQuery(const ConjunctiveQuery& q, size_t maxProbePerPred) const {
	Plan plan;
	plan.orderedPredicates.reserve(q.predicates.size());
	plan.details.reserve(q.predicates.size());

	// Schätzung je Prädikat
	for (const auto& p : q.predicates) {
		bool capped = false;
		size_t cnt = secIdx_.estimateCountEqual(q.table, p.column, p.value, maxProbePerPred, &capped);
		plan.details.push_back(Estimation{p, cnt, capped});
	}

	// Sortiere Prädikate nach (capped? maxProbe : count) aufsteigend
	std::vector<size_t> idx(q.predicates.size());
	std::iota(idx.begin(), idx.end(), 0);
	std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b){
		auto va = plan.details[a];
		auto vb = plan.details[b];
		auto ea = va.capped ? maxProbePerPred : va.estimatedCount;
		auto eb = vb.capped ? maxProbePerPred : vb.estimatedCount;
		if (ea != eb) return ea < eb;
		return va.pred.column < vb.pred.column; // stabile Ordnung
	});

	for (auto i : idx) plan.orderedPredicates.push_back(plan.details[i].pred);
	return plan;
}

// NLP-enhanced query optimization (PR #317 Phase 1)
QueryOptimizer::Plan QueryOptimizer::chooseOrderForAndQueryWithNLP(
    const ConjunctiveQuery& q,
    const std::string& original_query_text,
    size_t maxProbePerPred) const {
    
    // 1. Get base plan using traditional cost-based optimization
    Plan plan = chooseOrderForAndQuery(q, maxProbePerPred);
    
    // 2. Add NLP analysis if query text provided
    if (!original_query_text.empty()) {
        auto& nlp = getOptimizerNlp();
        // Estimate query complexity
        plan.nlp_complexity = nlp.estimateQueryComplexity(original_query_text);
        
        // Extract semantic hints
        plan.nlp_hints = nlp.extractQueryHints(original_query_text);
        
        // Get index suggestions
        plan.nlp_suggested_indexes = nlp.suggestIndexes(original_query_text);
        
        // Note: In future phases, we can use these hints to:
        // - Apply aggregation push-down if hints["aggregation"] is present
        // - Prefer specific index types from nlp_suggested_indexes
        // - Adjust cost estimates based on nlp_complexity
        // - Enable parallel execution for complex queries
    }
    
    return plan;
}

Result<std::vector<std::string>>
QueryOptimizer::executeOptimizedKeys(QueryEngine& engine, const ConjunctiveQuery& q, const Plan& plan) const {
	auto result = engine.executeAndKeysSequential(q.table, plan.orderedPredicates);
	if (!result.has_value()) {
		return Err<std::vector<std::string>>(
			errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			fmt::format("Optimized key execution failed")
		);
	}
	return Ok(result.value());
}

Result<std::vector<BaseEntity>>
QueryOptimizer::executeOptimizedEntities(QueryEngine& engine, const ConjunctiveQuery& q, const Plan& plan) const {
	auto result = engine.executeAndEntitiesSequential(q.table, plan.orderedPredicates);
	if (!result.has_value()) {
		return Err<std::vector<BaseEntity>>(
			errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
			fmt::format("Optimized entity execution failed")
		);
	}
	return Ok(result.value());
}

// ---------------- Vector+Geo Cost Model ----------------
QueryOptimizer::VectorGeoCostResult QueryOptimizer::chooseVectorGeoPlan(const VectorGeoCostInput& in) {
	// Tunable constants
	const double C_vec_base = 1.0;      // base cost per vector distance at dim=1
	const double C_spatial_eval = 0.15; // spatial predicate evaluation per candidate
	const double C_index_spatial = 0.02; // spatial index candidate fetch cost
	const double prefilterDiscountFactor = 0.65;

	// Handle vectorDim == 0 by using default 128
	size_t safeDim = in.vectorDim == 0 ? 128 : in.vectorDim;
	if (in.vectorDim == 0) {
		spdlog::warn("QueryOptimizer::chooseVectorGeoPlan: vectorDim is 0, using default {}", safeDim);
	}

	double dimScale = static_cast<double>(safeDim) / 128.0;
	double C_vec = C_vec_base * dimScale;
	std::size_t universe = in.spatialIndexEntries ? in.spatialIndexEntries : 100000; // fallback
	if (in.prefilterSize > 0 && in.prefilterSize < universe) universe = in.prefilterSize;

	std::size_t spatialCandidates = static_cast<std::size_t>(universe * in.bboxRatio);
	if (spatialCandidates == 0) spatialCandidates = 1;

	// Spatial-first cost
	double spatialPhaseCost = in.hasSpatialIndex ? spatialCandidates * C_index_spatial : universe * C_spatial_eval;
	double vectorPhaseCostSpatialFirst = spatialCandidates * C_vec;
	double costSpatialFirst = spatialPhaseCost + vectorPhaseCostSpatialFirst;

	// Vector-first cost
	double vectorSearchCost;
	if (in.hasVectorIndex) {
		vectorSearchCost = std::log(static_cast<double>(universe) + 1.0) * dimScale; // ANN approximation
	} else {
		vectorSearchCost = universe * C_vec; // brute-force
	}
	std::size_t vectorCandidates = in.hasVectorIndex ? (in.k * in.overfetch) : universe;
	double spatialEvalAfterVector = vectorCandidates * C_spatial_eval;
	double costVectorFirst = vectorSearchCost + spatialEvalAfterVector;

	if (in.prefilterSize > 0 && in.prefilterSize < in.spatialIndexEntries * 0.1) {
		costVectorFirst *= prefilterDiscountFactor;
		costSpatialFirst *= prefilterDiscountFactor;
	}

	VectorGeoPlan plan = costVectorFirst < costSpatialFirst ? VectorGeoPlan::VectorThenSpatial : VectorGeoPlan::SpatialThenVector;
	return {plan, costSpatialFirst, costVectorFirst};
}

// ---------------- Content+Geo Cost Model (extended heuristic) ----------------
QueryOptimizer::ContentGeoCostResult QueryOptimizer::estimateContentGeo(const ContentGeoCostInput& in) {
	// Tunable constants
	const double C_fulltext_base = 1.0;      // base cost for fulltext scoring batch
	const double C_spatial_eval = 0.12;      // spatial predicate per candidate (no index)
	const double C_spatial_index = 0.02;     // spatial index candidate fetch cost
	const double smallBBoxBoost = 0.7;       // discount when bboxRatio very small (<1%)

	std::size_t hits = in.fulltextHits ? in.fulltextHits : in.limit; // fallback
	if (hits == 0) hits = 1;

	// Fulltext-first plan cost: FT scan + spatial evaluation of hits
	double ftPhase = C_fulltext_base * std::log(static_cast<double>(hits) + 5.0);
	double spatialPhase = hits * (in.hasSpatialIndex ? C_spatial_index : C_spatial_eval) * in.bboxRatio;
	double costFulltextThenSpatial = ftPhase + spatialPhase;

	// Spatial-first plan cost: spatial candidate fetch + naive token checks on candidates
	// Approximate spatial candidates as hits * bboxRatio (worst-case) or hits if bboxRatio>1.
	double spatialCandidates = std::max(1.0, static_cast<double>(hits) * in.bboxRatio);
	double spatialFetch = spatialCandidates * (in.hasSpatialIndex ? C_spatial_index : C_spatial_eval);
	// Naive fulltext eval: tokenize + set compare ~ O(tokens + docTokens) ~ approximated by constant * candidates
	double ftEvalCandidates = spatialCandidates * 0.25; // constant factor
	double costSpatialThenFulltext = spatialFetch + ftEvalCandidates;

	if (in.bboxRatio < 0.01) {
		costSpatialThenFulltext *= smallBBoxBoost; // very selective bbox favors spatial-first
	}

	bool chooseFT = costFulltextThenSpatial <= costSpatialThenFulltext;
	return {costFulltextThenSpatial, costSpatialThenFulltext, chooseFT};
}

// ---------------- Graph Path Cost Model (rough estimate) ----------------
QueryOptimizer::GraphPathCostResult QueryOptimizer::estimateGraphPath(const GraphPathCostInput& in) {
	// Protect against exponential overflow
	constexpr double MAX_EXPANDED = 1e9; // Reasonable upper limit
	
	double expanded = 1.0; // start node
	for (size_t d = 1; d <= in.maxDepth; ++d) {
		double increment = std::pow(in.branchingFactor, static_cast<int>(d));
		
		// Check for overflow before adding
		if (expanded + increment > MAX_EXPANDED) {
			spdlog::warn("QueryOptimizer::estimateGraphPath: Node expansion would overflow, capping at {}", MAX_EXPANDED);
			expanded = MAX_EXPANDED;
			break;
		}
		expanded += increment;
	}
	if (in.hasSpatialConstraint) {
		expanded *= in.spatialSelectivity; // prune by spatial fraction
	}
	double timeMs = expanded * 0.02; // arbitrary scaling
	return {expanded, timeMs};
}

// ---------------- Adaptive & Distributed Optimization ----------------

void QueryOptimizer::enableAdaptiveOptimization(bool enable) {
	adaptive_enabled_ = enable;
	
	if (enable && !adaptive_stats_) {
		// Initialize adaptive components
		adaptive_stats_ = std::make_shared<AdaptiveQueryStats>();
		adaptive_selector_ = std::make_shared<AdaptivePlanSelector>();
		distributed_model_ = std::make_shared<DistributedQueryCostModel>();
		multi_index_optimizer_ = std::make_shared<MultiIndexOptimizer>();
		
		spdlog::info("QueryOptimizer: Adaptive optimization enabled");
	}
}

void QueryOptimizer::recordQueryExecution(
	const std::string& query_hash,
	size_t estimated_rows,
	size_t actual_rows,
	double execution_time_ms) {
	
	if (!adaptive_enabled_ || !adaptive_stats_) {
		return;
	}
	
	AdaptiveQueryStats::QueryExecution exec;
	exec.query_hash = query_hash;
	exec.estimated_rows = estimated_rows;
	exec.actual_rows = actual_rows;
	exec.execution_time_ms = execution_time_ms;
	exec.selectivity = estimated_rows > 0 ? 
		static_cast<double>(actual_rows) / estimated_rows : 1.0;
	exec.timestamp = std::chrono::system_clock::now();
	
	adaptive_stats_->recordExecution(exec);
	
	// Log significant misestimations
	if (adaptive_stats_->hasCardinalityMisestimation(query_hash)) {
		constexpr size_t HASH_DISPLAY_LENGTH = 8;
		spdlog::warn("QueryOptimizer: Cardinality misestimation detected for query {}", 
					 query_hash.substr(0, std::min(query_hash.size(), HASH_DISPLAY_LENGTH)));
	}
}

double QueryOptimizer::getAdaptiveAdjustment(const std::string& query_hash) const {
	if (!adaptive_enabled_ || !adaptive_stats_) {
		return 1.0;
	}
	
	return adaptive_stats_->getAdaptiveAdjustmentFactor(query_hash);
}

QueryOptimizer::DistributedPlan QueryOptimizer::optimizeForDistribution(
	const ConjunctiveQuery& q,
	const std::vector<std::string>& available_shards,
	bool enable_partition_pruning) const {
	
	DistributedPlan plan;
	plan.shard_ids = available_shards;
	
	if (!distributed_model_) {
		// Fallback: simple plan without optimization
		plan.recommended_parallelism = std::min(available_shards.size(), size_t(8));
		return plan;
	}
	
	// Build shard info for cost estimation
	std::vector<DistributedQueryCostModel::ShardInfo> shard_infos;
	for (const auto& shard_id : available_shards) {
		DistributedQueryCostModel::ShardInfo info;
		info.shard_id = shard_id;
		
		// TODO: Query actual shard metadata for accurate row estimation
		// For now, use a reasonable default estimate
		info.estimated_rows = 10000;
		
		// TODO: Measure actual network latency to each shard
		// For now, use default latency
		info.network_latency_ms = 1.0;
		
		// First shard assumed local as conservative default
		info.is_local = (shard_id == available_shards[0]);
		
		shard_infos.push_back(info);
	}
	
	// Partition pruning
	if (enable_partition_pruning) {
		std::vector<std::string> pruned_shards;
		for (const auto& info : shard_infos) {
			// TODO: Calculate actual selectivity from query predicates
			// For now, use conservative estimate that doesn't prune aggressively
			double selectivity = 0.5;
			
			if (!distributed_model_->shouldPrunePartition(info, available_shards.size(), selectivity)) {
				pruned_shards.push_back(info.shard_id);
			}
		}
		
		if (!pruned_shards.empty()) {
			plan.shard_ids = pruned_shards;
			plan.use_partition_pruning = true;
			spdlog::info("QueryOptimizer: Partition pruning reduced shards from {} to {}", 
						 available_shards.size(), pruned_shards.size());
		}
	}
	
	// Determine optimal parallelism
	size_t available_threads = std::thread::hardware_concurrency();
	plan.recommended_parallelism = distributed_model_->getOptimalParallelism(
		shard_infos, available_threads);
	
	// Enable NUMA awareness for large distributed queries
	if (plan.shard_ids.size() >= 4 && available_threads >= 8) {
		plan.enable_numa_awareness = true;
		
		// Get actual NUMA topology if available
		if (NumaAwareOptimizer::isNumaAvailable()) {
			// Use NumaAwareOptimizer to get actual NUMA placement
			NumaAwareOptimizer numa_opt;
			auto placement = numa_opt.getOptimalPlacement(0, plan.recommended_parallelism);
			plan.preferred_cpu_affinity = placement.cpu_affinity;
		} else {
			// Fallback: suggest sequential CPU affinity
			for (size_t i = 0; i < std::min(plan.recommended_parallelism, size_t(8)); ++i) {
				plan.preferred_cpu_affinity.push_back(static_cast<int>(i));
			}
		}
		
		spdlog::debug("QueryOptimizer: NUMA awareness enabled for distributed query");
	}
	
	// Determine join strategy for multi-shard queries
	if (plan.shard_ids.size() > 1) {
		// For simplicity, recommend broadcast for small result sets
		size_t estimated_results = 1000;  // Placeholder
		plan.join_strategy = estimated_results < 10000 ? "broadcast" : "repartition";
	}
	
	return plan;
}

QueryOptimizer::VectorWorkloadPlan QueryOptimizer::optimizeVectorWorkload(
	size_t k,
	size_t dataset_size,
	size_t dimension,
	double target_recall) const {
	
	VectorWorkloadPlan plan;
	
	// Use HNSW for large datasets
	if (dataset_size > 10000) {
		plan.index_type = "hnsw";
		
		// Adaptive ef_search based on k and dataset size
		// Formula: ef_search = max(k, k * log2(dataset_size / 1000))
		double log_factor = std::log2(static_cast<double>(dataset_size) / 1000.0);
		plan.recommended_ef_search = static_cast<int>(
			std::max(static_cast<double>(k), k * std::max(1.0, log_factor)));
		
		// Adjust for recall target
		if (target_recall > 0.97) {
			plan.recommended_ef_search = static_cast<int>(plan.recommended_ef_search * 1.5);
		} else if (target_recall < 0.93) {
			plan.recommended_ef_search = static_cast<int>(plan.recommended_ef_search * 0.7);
		}
		
		// Cap ef_search at reasonable bounds
		plan.recommended_ef_search = std::min(std::max(plan.recommended_ef_search, 16), 512);
		
		// Overfetch for post-filtering
		plan.recommended_k_overfetch = k * 2;  // 2x overfetch is typical
		plan.use_prefiltering = true;
		
	} else if (dataset_size > 1000) {
		plan.index_type = "ivf";
		plan.recommended_ef_search = static_cast<int>(k * 2);
		plan.recommended_k_overfetch = k;
		plan.use_prefiltering = false;
		
	} else {
		// Small dataset - use flat/brute force
		plan.index_type = "flat";
		plan.recommended_ef_search = 0;
		plan.recommended_k_overfetch = k;
		plan.use_prefiltering = false;
	}
	
	spdlog::debug("VectorWorkloadPlan: index_type={}, ef_search={}, k_overfetch={}, dataset_size={}", 
				  plan.index_type, plan.recommended_ef_search, plan.recommended_k_overfetch, dataset_size);
	
	return plan;
}

QueryOptimizer::GraphWorkloadPlan QueryOptimizer::optimizeGraphWorkload(
	size_t max_depth,
	size_t estimated_branching_factor,
	bool has_spatial_constraint) const {
	
	GraphWorkloadPlan plan;
	
	// Limit expansion based on branching factor with overflow protection
	size_t estimated_expansion = 1;
	constexpr size_t MAX_SAFE_EXPANSION = 1000000;  // 1M nodes max
	
	for (size_t d = 1; d <= max_depth; ++d) {
		// Check for potential overflow before multiplication
		if (estimated_expansion > MAX_SAFE_EXPANSION / estimated_branching_factor) {
			estimated_expansion = MAX_SAFE_EXPANSION;
			spdlog::warn("GraphWorkloadPlan: Estimated expansion capped at {} to prevent overflow", 
						 MAX_SAFE_EXPANSION);
			break;
		}
		estimated_expansion *= estimated_branching_factor;
		
		if (estimated_expansion >= MAX_SAFE_EXPANSION) {
			estimated_expansion = MAX_SAFE_EXPANSION;
			break;
		}
	}
	
	// If expansion would be too large, reduce depth or use bidirectional search
	if (estimated_expansion > 50000) {
		plan.use_bidirectional_search = true;
		plan.max_expansion_depth = max_depth;  // Bidirectional cuts search space
		spdlog::info("GraphWorkloadPlan: Using bidirectional search for large expansion (est={})", 
					 estimated_expansion);
	} else {
		plan.use_bidirectional_search = false;
		plan.max_expansion_depth = max_depth;
	}
	
	// Enable spatial pruning if constraint present
	plan.enable_spatial_pruning = has_spatial_constraint;
	
	// Parallelism based on expected work
	size_t hw_threads = std::thread::hardware_concurrency();
	if (estimated_expansion > 10000) {
		plan.recommended_parallelism = std::min(hw_threads, size_t(8));
	} else if (estimated_expansion > 1000) {
		plan.recommended_parallelism = std::min(hw_threads, size_t(4));
	} else {
		plan.recommended_parallelism = 1;  // Small graphs don't benefit from parallelism
	}
	
	spdlog::debug("GraphWorkloadPlan: max_depth={}, bidirectional={}, parallelism={}", 
				  plan.max_expansion_depth, plan.use_bidirectional_search, plan.recommended_parallelism);
	
	return plan;
}

} // namespace themis
