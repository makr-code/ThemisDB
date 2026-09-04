/**
 * @file tnsr_task.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "tensor/tnsr_task.h"

#include "storage/tt_quantizer.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <stdexcept>

namespace themis {
namespace tensor {

namespace {
std::mutex                   g_reroute_serialize_mtx;
TNSRTask::RerouteSerializeFn g_reroute_serialize_fn;
} // namespace

void TNSRTask::setRerouteSerializeFn(RerouteSerializeFn fn) {
    std::lock_guard<std::mutex> lk(g_reroute_serialize_mtx);
    g_reroute_serialize_fn = std::move(fn);
}

void TNSRTask::clearRerouteSerializeFn() {
    std::lock_guard<std::mutex> lk(g_reroute_serialize_mtx);
    g_reroute_serialize_fn = nullptr;
}

TNSRTask::RerouteSerializeFn TNSRTask::getRerouteSerializeFn() {
    std::lock_guard<std::mutex> lk(g_reroute_serialize_mtx);
    return g_reroute_serialize_fn;
}

// ============================================================================
// TNSRTask — construction
// ============================================================================

TNSRTask::TNSRTask(std::shared_ptr<storage::TensorNetworkStorageEngine> engine,
                   storage::TensorTrainDecomposer                       decomposer)
    : engine_(std::move(engine)), decomposer_(std::move(decomposer)) {
    if (!engine_) {
        throw std::invalid_argument("TNSRTask: engine must not be null");
    }
}

bool TNSRTask::hasRerouteSerializeFn() {
    std::lock_guard<std::mutex> lock(g_reroute_serialize_mtx);
    return static_cast<bool>(g_reroute_serialize_fn);
}

// ============================================================================
// TNSRTask::run
// ============================================================================

TNSRReport TNSRTask::run(
    const std::vector<storage::TensorFieldKey>& index_key_range,
    const TNSRConfig& cfg)
{
    TNSRReport report;
    const auto t_start = std::chrono::steady_clock::now();

    for (const auto& field_key : index_key_range) {
        if (cancel_requested_.load(std::memory_order_acquire)) {
            break;
        }
        ++report.keys_processed;

        // ── 1. Fetch compressed form from storage ──────────────────────────
        const auto compressed_opt = engine_->getCompressed(field_key);
        if (!compressed_opt.has_value()) {
            continue; // Key not found — skip silently.
        }

        // ── 2. Deserialise TTTrain from QuantizedTrain ─────────────────────
        // TTQuantizer::dequantize reconstructs the floating-point TTTrain.
        storage::TTQuantizer quantizer;
        storage::TTTrain original_train;
        try {
            original_train = quantizer.dequantize(*compressed_opt);
        } catch (...) {
            ++report.error_count;
            continue;
        }
        if (original_train.cores.empty()) {
            ++report.error_count;
            continue;
        }

        // Original serialised size (used for savings computation).
        const std::size_t original_bytes = compressed_opt->serialize().size();
        const std::size_t original_max_rank = original_train.maxRank();

        // ── 3. Recompress: tighten bond dimensions ─────────────────────────
        storage::TensorTrainConfig tt_cfg;
        tt_cfg.eps = cfg.epsilon;
        storage::TTTrain recompressed;
        try {
            recompressed = decomposer_.recompress(original_train, tt_cfg);
        } catch (...) {
            ++report.error_count;
            continue;
        }

        // ── 4. Topology analysis via HissStructuralSearchEngine ────────────
        //
        // Fast path:
        // Skip HISS for structurally trivial trains where topology search is
        // unlikely to produce meaningful non-chain mutations.
        const bool trivial_topology =
            (recompressed.cores.size() < 3U) || (recompressed.maxRank() < 2U);
        if (trivial_topology) {
            ++report.topology_search_skipped_keys;
        } else {
            // Stub #252 resolved: RerouteSerializeFn bridge is fully wired.
            // When setRerouteSerializeFn() has been called, topology changes are
            // serialised to storage.  Without injection, bond-dimension reduction
            // (recompress) is still always durable; topology changes are advisory.
            TensorNetworkGraph tng = hiss_engine_.search(recompressed, cfg.hiss_config);
            std::size_t topo_changes_this_key = 0;
            for (const auto& edge : tng.edges()) {
                if (topo_changes_this_key >= cfg.max_topology_changes_per_run) {
                  break;
                }
                // Skip chain edges; only propose "reshaped" / "clustered" ones.
                if (edge.topology == "chain") {
                  continue;
                }
                // rerouteEdge is idempotent and harmless on the in-memory graph.
                const bool rerouted = tng.rerouteEdge(edge.from, edge.to, edge.topology);
                if (!rerouted) {
                    ++report.error_count;
                    continue;
                }
                ++topo_changes_this_key;
            }
            if (topo_changes_this_key > 0) {
                const auto serialize_fn = getRerouteSerializeFn();
                if (serialize_fn) {
                    try {
                        if (!serialize_fn(*engine_, field_key, tng, recompressed)) {
                            ++report.error_count;
                            continue;
                        }
                    } catch (...) {
                        ++report.error_count;
                        continue;
                    }
                }
            }
            report.topology_changes += topo_changes_this_key;
        }

        // ── 5. Decide whether to write back ────────────────────────────────
        const auto recompressed_qtz = quantizer.quantize(recompressed, compressed_opt->quant_type);
        const std::size_t new_bytes = recompressed_qtz.serialize().size();

        if (new_bytes >= original_bytes) {
            continue; // No size reduction — skip.
        }
        const std::size_t saved = original_bytes - new_bytes;
        if (saved < cfg.min_bytes_saved_to_commit) {
            continue; // Savings below threshold — skip.
        }

        // Reconstruct dense data for put() — required by the engine API.
        // For very large tensors this is the dominating cost; a future
        // optimisation (Q3 2028) will add a put_compressed() overload.
        const auto dense_data = recompressed.reconstruct();
        if (dense_data.empty()) {
            ++report.error_count;
            continue;
        }

        // put() re-decomposes from dense; that's intentional: the engine
        // enforces its own config (epsilon, quant_type) on every write.
        if (!engine_->put(field_key, dense_data, recompressed.mode_sizes)) {
            ++report.error_count;
            continue;
        }

        const auto new_max_rank = static_cast<std::int64_t>(recompressed.maxRank());
        report.rank_delta += static_cast<std::int64_t>(original_max_rank) - new_max_rank;
        report.bytes_saved += saved;
        ++report.keys_rewritten;
    }

    const auto t_end = std::chrono::steady_clock::now();
    report.duration_ms =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count()
        ) / 1000.0;

    return report;
}

} // namespace tensor
} // namespace themis

