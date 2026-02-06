#include "server/merge_api_handler.h"
#include "transaction/merge_engine.h"
#include "transaction/snapshot_manager.h"
#include "analytics/diff_engine.h"
#include "cdc/changefeed.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;
using namespace transaction;

/**
 * @brief REST API handler for merge operations
 * 
 * Endpoints:
 * - POST /api/v1/merge - Perform three-way merge
 * - POST /api/v1/merge/preview - Preview merge without applying
 * - POST /api/v1/merge/by-tag - Merge using snapshot tags
 */
class MergeApiHandler {
public:
    MergeApiHandler(
        MergeEngine& merge_engine,
        SnapshotManager& snapshot_manager
    ) : merge_engine_(merge_engine),
        snapshot_manager_(snapshot_manager) {}

    /**
     * @brief POST /api/v1/merge
     * 
     * Request body:
     * {
     *   "base_sequence": 100,
     *   "source_sequence": 150,
     *   "target_sequence": 200,
     *   "strategy": "ours|theirs|manual|fast_forward",
     *   "fail_on_conflict": false,
     *   "manual_resolutions": [
     *     {"key": "users:1", "resolved_value": "Alice"}
     *   ]
     * }
     * 
     * Response:
     * {
     *   "success": true,
     *   "message": "Merge successful",
     *   "stats": {...},
     *   "conflicts": [...],
     *   "changes_applied": [...],
     *   "result_sequence": 250
     * }
     */
    json handleMerge(const json& request_body) {
        try {
            // Parse request
            if (!request_body.contains("base_sequence") ||
                !request_body.contains("source_sequence") ||
                !request_body.contains("target_sequence")) {
                return createErrorResponse("Missing required fields: base_sequence, source_sequence, target_sequence");
            }

            uint64_t base_sequence = request_body["base_sequence"];
            uint64_t source_sequence = request_body["source_sequence"];
            uint64_t target_sequence = request_body["target_sequence"];

            MergeEngine::MergeOptions options;
            
            // Parse strategy
            if (request_body.contains("strategy")) {
                std::string strategy = request_body["strategy"];
                if (strategy == "ours") {
                    options.strategy = MergeEngine::MergeStrategy::OURS;
                } else if (strategy == "theirs") {
                    options.strategy = MergeEngine::MergeStrategy::THEIRS;
                } else if (strategy == "fast_forward") {
                    options.strategy = MergeEngine::MergeStrategy::FAST_FORWARD;
                } else if (strategy == "manual") {
                    options.strategy = MergeEngine::MergeStrategy::MANUAL;
                } else {
                    return createErrorResponse("Invalid strategy. Must be: ours, theirs, manual, or fast_forward");
                }
            }

            // Parse other options
            if (request_body.contains("fail_on_conflict")) {
                options.fail_on_conflict = request_body["fail_on_conflict"];
            }

            // Parse manual resolutions
            if (request_body.contains("manual_resolutions")) {
                for (const auto& res_json : request_body["manual_resolutions"]) {
                    MergeEngine::ConflictResolution resolution;
                    resolution.key = res_json["key"];
                    if (res_json.contains("resolved_value")) {
                        resolution.resolved_value = res_json["resolved_value"].get<std::string>();
                    }
                    options.manual_resolutions.push_back(resolution);
                }
            }

            // Perform merge
            auto result = merge_engine_.merge(base_sequence, source_sequence, target_sequence, options);

            // Return result
            return result.toJson();

        } catch (const std::exception& e) {
            spdlog::error("Error handling merge request: {}", e.what());
            return createErrorResponse(fmt::format("Internal error: {}", e.what()));
        }
    }

    /**
     * @brief POST /api/v1/merge/preview
     * 
     * Request body:
     * {
     *   "base_sequence": 100,
     *   "source_sequence": 150,
     *   "target_sequence": 200
     * }
     * 
     * Response: Same as merge but without applying changes
     */
    json handleMergePreview(const json& request_body) {
        try {
            if (!request_body.contains("base_sequence") ||
                !request_body.contains("source_sequence") ||
                !request_body.contains("target_sequence")) {
                return createErrorResponse("Missing required fields: base_sequence, source_sequence, target_sequence");
            }

            uint64_t base_sequence = request_body["base_sequence"];
            uint64_t source_sequence = request_body["source_sequence"];
            uint64_t target_sequence = request_body["target_sequence"];

            auto result = merge_engine_.previewMerge(base_sequence, source_sequence, target_sequence);

            return result.toJson();

        } catch (const std::exception& e) {
            spdlog::error("Error handling merge preview request: {}", e.what());
            return createErrorResponse(fmt::format("Internal error: {}", e.what()));
        }
    }

    /**
     * @brief POST /api/v1/merge/by-tag
     * 
     * Request body:
     * {
     *   "base_tag": "v1.0.0",
     *   "source_tag": "feature-branch",
     *   "target_tag": "current",
     *   "strategy": "ours|theirs|manual|fast_forward",
     *   "fail_on_conflict": false,
     *   "manual_resolutions": [...]
     * }
     * 
     * Response: Same as merge
     */
    json handleMergeByTag(const json& request_body) {
        try {
            if (!request_body.contains("base_tag") ||
                !request_body.contains("source_tag") ||
                !request_body.contains("target_tag")) {
                return createErrorResponse("Missing required fields: base_tag, source_tag, target_tag");
            }

            std::string base_tag = request_body["base_tag"];
            std::string source_tag = request_body["source_tag"];
            std::string target_tag = request_body["target_tag"];

            MergeEngine::MergeOptions options;
            
            // Parse strategy
            if (request_body.contains("strategy")) {
                std::string strategy = request_body["strategy"];
                if (strategy == "ours") {
                    options.strategy = MergeEngine::MergeStrategy::OURS;
                } else if (strategy == "theirs") {
                    options.strategy = MergeEngine::MergeStrategy::THEIRS;
                } else if (strategy == "fast_forward") {
                    options.strategy = MergeEngine::MergeStrategy::FAST_FORWARD;
                } else if (strategy == "manual") {
                    options.strategy = MergeEngine::MergeStrategy::MANUAL;
                } else {
                    return createErrorResponse("Invalid strategy. Must be: ours, theirs, manual, or fast_forward");
                }
            }

            if (request_body.contains("fail_on_conflict")) {
                options.fail_on_conflict = request_body["fail_on_conflict"];
            }

            if (request_body.contains("manual_resolutions")) {
                for (const auto& res_json : request_body["manual_resolutions"]) {
                    MergeEngine::ConflictResolution resolution;
                    resolution.key = res_json["key"];
                    if (res_json.contains("resolved_value")) {
                        resolution.resolved_value = res_json["resolved_value"].get<std::string>();
                    }
                    options.manual_resolutions.push_back(resolution);
                }
            }

            auto result = merge_engine_.mergeByTag(base_tag, source_tag, target_tag, options);

            return result.toJson();

        } catch (const std::exception& e) {
            spdlog::error("Error handling merge by tag request: {}", e.what());
            return createErrorResponse(fmt::format("Internal error: {}", e.what()));
        }
    }

    /**
     * @brief GET /api/v1/merge/can-fast-forward
     * 
     * Query parameters:
     * - base_sequence
     * - source_sequence
     * - target_sequence
     * 
     * Response:
     * {
     *   "can_fast_forward": true
     * }
     */
    json handleCanFastForward(uint64_t base_sequence, uint64_t source_sequence, uint64_t target_sequence) {
        try {
            bool can_ff = merge_engine_.canFastForward(base_sequence, source_sequence, target_sequence);
            
            json response;
            response["can_fast_forward"] = can_ff;
            response["base_sequence"] = base_sequence;
            response["source_sequence"] = source_sequence;
            response["target_sequence"] = target_sequence;
            
            return response;

        } catch (const std::exception& e) {
            spdlog::error("Error checking fast-forward: {}", e.what());
            return createErrorResponse(fmt::format("Internal error: {}", e.what()));
        }
    }

private:
    MergeEngine& merge_engine_;
    SnapshotManager& snapshot_manager_;

    json createErrorResponse(const std::string& message) {
        json response;
        response["success"] = false;
        response["error"] = message;
        return response;
    }
};

} // namespace server
} // namespace themis
