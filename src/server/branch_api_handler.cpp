/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            branch_api_handler.cpp                             ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:39:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/branch_api_handler.h"
#include <sstream>

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
        sendError(res, 404, "Branch not found");
        return;
    }
    
    sendJson(res, branch->toJson());
}

void BranchApiHandler::handleSwitchBranch(const httplib::Request& req, httplib::Response& res) {
    std::string branch_name = req.matches[1];
    
    bool success = branch_manager_.switchBranch(branch_name);
    if (!success) {
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

void BranchApiHandler::handleGetStats(const httplib::Request& req, httplib::Response& res) {
    auto stats = branch_manager_.getStats();
    sendJson(res, stats.toJson());
}

void BranchApiHandler::handleGetActiveBranch(const httplib::Request& req, httplib::Response& res) {
    std::string active_branch = branch_manager_.getActiveBranch();
    
    json result = {
        {"active_branch", active_branch}
    };
    sendJson(res, result);
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
