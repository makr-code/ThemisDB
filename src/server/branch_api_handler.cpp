/**
 * @file branch_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/branch_api_handler.h"
#include <sstream>
#include "utils/tracing.h"

namespace themis {
namespace server {

BranchApiHandler::BranchApiHandler(transaction::BranchManager& branch_manager)
    : branch_manager_(branch_manager) {
}

void BranchApiHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/branches - Create a new branch
    server.Post("/api/v1/branches", [this](const httplib::Request& req, httplib::Response& res) {
        handleCreateBranch(req, res);
    });
    
    // GET /api/v1/branches - List all branches
    server.Get("/api/v1/branches", [this](const httplib::Request& req, httplib::Response& res) {
        handleListBranches(req, res);
    });
    
    // GET /api/v1/branches/active - Get active branch
    server.Get("/api/v1/branches/active", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetActiveBranch(req, res);
    });
    
    // GET /api/v1/branches/:name - Get specific branch
    server.Get(R"(/api/v1/branches/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetBranch(req, res);
    });
    
    // POST /api/v1/branches/:name/switch - Switch to a branch
    server.Post(R"(/api/v1/branches/([^/]+)/switch)", [this](const httplib::Request& req, httplib::Response& res) {
        handleSwitchBranch(req, res);
    });
    
    // POST /api/v1/branches/merge/preview - Preview merge with full conflict details
    server.Post("/api/v1/branches/merge/preview", [this](const httplib::Request& req, httplib::Response& res) {
        handlePreviewMergeBranches(req, res);
    });

    // POST /api/v1/branches/merge/resolve - Resolve conflicts and complete merge
    server.Post("/api/v1/branches/merge/resolve", [this](const httplib::Request& req, httplib::Response& res) {
        handleResolveMergeBranches(req, res);
    });

    // POST /api/v1/branches/merge - Merge branches
    server.Post("/api/v1/branches/merge", [this](const httplib::Request& req, httplib::Response& res) {
        handleMergeBranches(req, res);
    });
    
    // DELETE /api/v1/branches/:name - Delete a branch
    server.Delete(R"(/api/v1/branches/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleDeleteBranch(req, res);
    });
    
    // GET /api/v1/branches/stats - Get branch statistics
    server.Get("/api/v1/branches/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStats(req, res);
    });
}

void BranchApiHandler::handleCreateBranch(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseJsonBody(req, body, res)) {
    auto span = Tracer::startSpan("handleCreateBranch");
        return;
    }
    
    // Extract parameters
    std::string branch_name = body.value("branch_name", "");
    std::string parent_branch = body.value("parent_branch", "");
    std::string description = body.value("description", "");
    std::string created_by = body.value("created_by", "system");
    
    if (branch_name.empty()) {
        sendError(res, 400, "branch_name is required");
        return;
    }
    
    // Parse options
    transaction::BranchManager::CreateBranchOptions options;
    if (body.contains("from_tag")) {
        options.from_tag = body["from_tag"];
    }
    if (body.contains("from_sequence")) {
        options.from_sequence = body["from_sequence"];
    }
    if (body.contains("from_timestamp")) {
        options.from_timestamp = body["from_timestamp"];
    }
    if (body.contains("set_active")) {
        options.set_active = body["set_active"];
    }
    
    // Create branch
    auto branch = branch_manager_.createBranch(
        branch_name,
        parent_branch,
        description,
        created_by,
        options
    );
    
    if (!branch.has_value()) {
        sendError(res, 400, "Failed to create branch. Branch may already exist or parent branch not found.");
        return;
    }
    
    sendJson(res, branch->toJson(), 201);
}

void BranchApiHandler::handleListBranches(const httplib::Request& req, httplib::Response& res) {
    // Parse query parameters
    size_t limit = 0;
    if (req.has_param("limit")) {
    auto span = Tracer::startSpan("handleListBranches");
        limit = std::stoull(req.get_param_value("limit"));
    }
    
    std::string sort_by = "name";
    if (req.has_param("sort_by")) {
        sort_by = req.get_param_value("sort_by");
    }
    
    bool ascending = true;
    if (req.has_param("ascending")) {
        ascending = req.get_param_value("ascending") == "true";
    }
    
    // Get branches
    auto branches = branch_manager_.listBranches(limit, sort_by, ascending);
    
    // Convert to JSON array
    json result = json::array();
    for (const auto& branch : branches) {
        result.push_back(branch.toJson());
    }
    
    sendJson(res, result);
}

void BranchApiHandler::handleGetBranch(const httplib::Request& req, httplib::Response& res) {
    std::string branch_name = req.matches[1];
    
    auto branch = branch_manager_.getBranch(branch_name);
    if (!branch.has_value()) {
    auto span = Tracer::startSpan("handleGetBranch");
        sendError(res, 404, "Branch not found");
        return;
    }
    
    sendJson(res, branch->toJson());
}

void BranchApiHandler::handleSwitchBranch(const httplib::Request& req, httplib::Response& res) {
    std::string branch_name = req.matches[1];
    
    bool success = branch_manager_.switchBranch(branch_name);
    if (!success) {
    auto span = Tracer::startSpan("handleSwitchBranch");
        sendError(res, 400, "Failed to switch branch. Branch may not exist.");
        return;
    }
    
    json result = {
        {"success", true},
        {"message", "Switched to branch: " + branch_name},
        {"active_branch", branch_name}
    };
    sendJson(res, result);
}

void BranchApiHandler::handleMergeBranches(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseJsonBody(req, body, res)) {
    auto span = Tracer::startSpan("handleMergeBranches");
        return;
    }
    
    std::string source_branch = body.value("source_branch", "");
    std::string target_branch = body.value("target_branch", "");
    
    if (source_branch.empty() || target_branch.empty()) {
        sendError(res, 400, "source_branch and target_branch are required");
        return;
    }
    
    // Parse merge options
    transaction::BranchManager::MergeOptions options;
    if (body.contains("fast_forward")) {
        options.fast_forward = body["fast_forward"];
    }
    if (body.contains("abort_on_conflict")) {
        options.abort_on_conflict = body["abort_on_conflict"];
    }
    if (body.contains("merge_strategy")) {
        options.merge_strategy = body["merge_strategy"];
    }
    
    // Perform merge
    auto result = branch_manager_.mergeBranches(source_branch, target_branch, options);
    
    sendJson(res, result.toJson(), result.success ? 200 : 409);
}

void BranchApiHandler::handleDeleteBranch(const httplib::Request& req, httplib::Response& res) {
    std::string branch_name = req.matches[1];
    
    // Check for force flag
    bool force = false;
    if (req.has_param("force")) {
    auto span = Tracer::startSpan("handleDeleteBranch");
        force = req.get_param_value("force") == "true";
    }
    
    bool success = branch_manager_.deleteBranch(branch_name, force);
    if (!success) {
        sendError(res, 400, "Failed to delete branch. Branch may be active or not fully merged.");
        return;
    }
    
    json result = {
        {"success", true},
        {"message", "Branch deleted: " + branch_name}
    };
    sendJson(res, result);
}

void BranchApiHandler::handleGetStats(const httplib::Request& /*req*/, httplib::Response& res) {
    auto stats = branch_manager_.getStats();
    sendJson(res, stats.toJson());
}

void BranchApiHandler::handleGetActiveBranch(const httplib::Request& /*req*/, httplib::Response& res) {
    auto span = Tracer::startSpan("handleGetActiveBranch");
    std::string active_branch = branch_manager_.getActiveBranch();
    
    json result = {
        {"active_branch", active_branch}
    };
    sendJson(res, result);
}

void BranchApiHandler::handlePreviewMergeBranches(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseJsonBody(req, body, res)) {
    auto span = Tracer::startSpan("handlePreviewMergeBranches");
        return;
    }

    std::string source_branch = body.value("source_branch", "");
    std::string target_branch = body.value("target_branch", "");
    std::string base_branch   = body.value("base_branch", "");

    if (source_branch.empty() || target_branch.empty()) {
        sendError(res, 400, "source_branch and target_branch are required");
        return;
    }

    auto result = branch_manager_.previewBranchMerge(source_branch, target_branch, base_branch);

    // Enrich the standard MergeEngine JSON with branch context
    json response = result.toJson();
    response["source_branch"] = source_branch;
    response["target_branch"] = target_branch;
    if (!base_branch.empty()) {
        response["base_branch"] = base_branch;
    }

    int status = 200;
    if (!result.success) {
        const auto& msg = result.message;
        if (msg.find("not found") != std::string::npos) {
            status = 404;
        } else if (msg.find("not initialized") != std::string::npos) {
            status = 503;
        } else {
            status = 409; // genuine merge conflict
        }
    }
    sendJson(res, response, status);
}

void BranchApiHandler::handleResolveMergeBranches(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseJsonBody(req, body, res)) {
    auto span = Tracer::startSpan("handleResolveMergeBranches");
        return;
    }

    std::string source_branch = body.value("source_branch", "");
    std::string target_branch = body.value("target_branch", "");
    std::string base_branch   = body.value("base_branch", "");

    if (source_branch.empty() || target_branch.empty()) {
        sendError(res, 400, "source_branch and target_branch are required");
        return;
    }

    // Parse per-key conflict resolutions
    std::vector<transaction::MergeEngine::ConflictResolution> resolutions;
    if (body.contains("resolutions") && body["resolutions"].is_array()) {
        for (const auto& r : body["resolutions"]) {
            transaction::MergeEngine::ConflictResolution res_item;
            res_item.key = r.value("key", "");
            if (res_item.key.empty()) {
                sendError(res, 400, "Each resolution must have a non-empty 'key' field");
                return;
            }
            if (r.contains("resolved_value")) {
                res_item.resolved_value = r["resolved_value"].get<std::string>();
            }
            resolutions.push_back(std::move(res_item));
        }
    }

    auto result = branch_manager_.resolveAndMergeBranches(
        source_branch, target_branch, resolutions, base_branch);

    json response = result.toJson();
    response["source_branch"] = source_branch;
    response["target_branch"] = target_branch;
    if (!base_branch.empty()) {
        response["base_branch"] = base_branch;
    }

    int status = 200;
    if (!result.success) {
        const auto& msg = result.message;
        if (msg.find("not found") != std::string::npos) {
            status = 404;
        } else if (msg.find("not initialized") != std::string::npos) {
            status = 503;
        } else {
            status = 409; // unresolved conflicts or merge failure
        }
    }
    sendJson(res, response, status);
}

void BranchApiHandler::sendJson(httplib::Response& res, const json& data, int status_code) {
    res.status = status_code;
    res.set_content(data.dump(2), "application/json");
}

void BranchApiHandler::sendError(httplib::Response& res, int status_code, const std::string& message) {
    json error = {
        {"error", message},
        {"status", status_code}
    };
    sendJson(res, error, status_code);
}

bool BranchApiHandler::parseJsonBody(const httplib::Request& req, json& out, httplib::Response& res) {
    try {
        out = json::parse(req.body);
        return true;
    } catch (const json::parse_error& e) {
        sendError(res, 400, std::string("Invalid JSON: ") + e.what());
        return false;
    }
}

} // namespace server
} // namespace themis
