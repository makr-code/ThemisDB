/**
 * @file shard_repair_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=3, M=43, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/shard_repair_api_handler.h"

#include "sharding/shard_repair_engine.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/input_validator.h"
#include "utils/audit_logger.h"  // W1-FIX(missing_audit_log): structured auth audit

#include <nlohmann/json.hpp>

#include <chrono>
#include <sstream>

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

uint64_t toUnixMs(const std::chrono::system_clock::time_point& time_point) {
    if (time_point.time_since_epoch().count() == 0) {
        return 0;
    }
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        time_point.time_since_epoch()).count());
}

std::string statusToString(sharding::ShardRepairStatus status) {
    switch (status) {
        case sharding::ShardRepairStatus::HEALTHY:
            return "healthy";
        case sharding::ShardRepairStatus::DEGRADED:
            return "degraded";
        case sharding::ShardRepairStatus::FAILED:
            return "failed";
        case sharding::ShardRepairStatus::REBUILDING:
            return "rebuilding";
    }
    return "unknown";
}

json repairJobToJson(const sharding::RepairJob& job) {
    return {
        {"job_id", job.job_id},
        {"shard_id", job.shard_id},
        {"document_id", job.document_id},
        {"collection", job.collection},
        {"is_full_scan", job.is_full_scan},
        {"submitted_at_unix_ms", toUnixMs(job.submitted_at)},
        {"completed_at_unix_ms", toUnixMs(job.completed_at)},
        {"documents_scanned", job.documents_scanned},
        {"documents_repaired", job.documents_repaired},
        {"documents_failed", job.documents_failed},
        {"completed", job.completed},
        {"success", job.success},
        {"error_message", job.error_message}
    };
}

json shardReportToJson(const sharding::ShardHealthReport& report) {
    return {
        {"shard_id", report.shard_id},
        {"status", statusToString(report.status)},
        {"documents_scanned", report.documents_scanned},
        {"documents_healthy", report.documents_healthy},
        {"documents_degraded", report.documents_degraded},
        {"documents_unrecoverable", report.documents_unrecoverable},
        {"last_scan_unix_ms", toUnixMs(report.last_scan)},
        {"last_repair_unix_ms", toUnixMs(report.last_repair)},
        {"last_error", report.last_error}
    };
}

json metricsToJson(const sharding::RepairMetrics& metrics) {
    return {
        {"total_scans", metrics.total_scans},
        {"total_repairs_attempted", metrics.total_repairs_attempted},
        {"total_repairs_successful", metrics.total_repairs_successful},
        {"total_repairs_failed", metrics.total_repairs_failed},
        {"total_documents_scanned", metrics.total_documents_scanned},
        {"avg_repair_time_ms", metrics.avg_repair_time_ms.count()},
        {"last_scan_time_unix_ms", toUnixMs(metrics.last_scan_time)},
        {"last_repair_time_unix_ms", toUnixMs(metrics.last_repair_time)},
        {"last_scan_workers", metrics.last_scan_workers}
    };
}

std::string extractJobId(std::string_view target) {
    constexpr std::string_view prefix{"/v1/admin/repair/jobs/"};
    if (target.rfind(prefix, 0) != 0) {
        return {};
    }
    auto job_id = target.substr(prefix.size());
    auto qpos = job_id.find('?');
    if (qpos != std::string_view::npos) {
        job_id = job_id.substr(0, qpos);
    }
    return std::string(job_id);
}

std::string buildDashboardHtml() {
    std::ostringstream html;
    html << "<!doctype html>\n"
         << "<html lang=\"en\">\n"
         << "<head><meta charset=\"utf-8\">\n"
         << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
         << "<title>Themis Repair Dashboard</title>\n"
         << "<style>"
         << ":root{--bg:#f4efe6;--fg:#1e2430;--panel:#fffaf2;--line:#d9ccb8;--accent:#a44a3f;--accent2:#2d6a73;--ok:#2e7d32;--warn:#a66a00;--bad:#b3261e;}"
         << "body{margin:0;font-family:Georgia,\"Times New Roman\",serif;background:linear-gradient(135deg,#f7f1e8,#ebe3d7);color:var(--fg)}"
         << "main{max-width:1200px;margin:0 auto;padding:24px}"
         << "h1{margin:0 0 8px;font-size:32px;letter-spacing:.02em}"
         << ".sub{margin:0 0 24px;color:#6a6f78}"
         << ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:16px}"
         << ".card{background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:16px;box-shadow:0 8px 24px rgba(0,0,0,.05)}"
         << ".metric{font-size:28px;font-weight:700;margin-top:8px}"
         << ".actions{display:flex;gap:12px;flex-wrap:wrap;margin:20px 0}"
         << "button{background:var(--accent);color:#fff;border:0;border-radius:999px;padding:10px 16px;cursor:pointer}"
         << "button.secondary{background:var(--accent2)}"
         << "input{padding:10px 12px;border-radius:999px;border:1px solid var(--line);min-width:220px;background:#fff}"
         << "table{width:100%;border-collapse:collapse;margin-top:16px;background:var(--panel);border-radius:16px;overflow:hidden}"
         << "th,td{text-align:left;padding:10px 12px;border-bottom:1px solid var(--line);font-size:14px}"
         << "th{background:#efe4d2}"
         << ".badge{display:inline-block;padding:4px 10px;border-radius:999px;font-size:12px;font-weight:700;text-transform:uppercase}"
         << ".healthy{background:#e5f3e6;color:var(--ok)}.degraded{background:#fff0cc;color:var(--warn)}.failed{background:#fde7e5;color:var(--bad)}.rebuilding{background:#dff1f4;color:var(--accent2)}"
         << "pre{background:#1f2430;color:#f1f5f9;padding:12px;border-radius:12px;overflow:auto}"
         << "#flash{min-height:20px;color:var(--accent);font-weight:700}"
         << "</style></head>\n"
         << "<body><main>"
         << "<h1>Repair Dashboard</h1>"
         << "<p class=\"sub\">Shard health, repair jobs, and anti-entropy triggers.</p>"
         << "<div id=\"flash\"></div>"
         << "<section class=\"grid\" id=\"summary\"></section>"
         << "<section class=\"actions\">"
         << "<button id=\"scanBtn\">Start Full Scan</button>"
         << "<input id=\"shardId\" placeholder=\"Shard ID for targeted repair\">"
         << "<button id=\"repairBtn\" class=\"secondary\">Queue Shard Repair</button>"
         << "<button id=\"refreshBtn\">Refresh</button>"
         << "</section>"
         << "<h2>Shard Health</h2><table><thead><tr><th>Shard</th><th>Status</th><th>Scanned</th><th>Healthy</th><th>Degraded</th><th>Unrecoverable</th><th>Last Error</th></tr></thead><tbody id=\"shards\"></tbody></table>"
         << "<h2>Active Jobs</h2><table><thead><tr><th>Job</th><th>Shard</th><th>Document</th><th>Full Scan</th><th>Submitted</th><th>Completed</th><th>Success</th></tr></thead><tbody id=\"jobs\"></tbody></table>"
         << "<h2>Raw JSON</h2><pre id=\"raw\">loading...</pre>"
         << "<script>"
         << "const summary=document.getElementById('summary');"
         << "const shards=document.getElementById('shards');"
         << "const jobs=document.getElementById('jobs');"
         << "const raw=document.getElementById('raw');"
         << "const flash=document.getElementById('flash');"
         << "function fmtTime(v){return v?new Date(v).toLocaleString():'-';}"
         << "function badge(status){return `<span class=\"badge ${status}\">${status}</span>`;}"
         << "function setFlash(msg){flash.textContent=msg;setTimeout(()=>{if(flash.textContent===msg)flash.textContent='';},4000);}"
         << "async function load(){const res=await fetch('/v1/admin/repair/health');const data=await res.json();raw.textContent=JSON.stringify(data,null,2);summary.innerHTML='';const cards=[['Status',data.status],['Engine',data.engine_running?'running':'stopped'],['Scans',data.metrics?.total_scans ?? 0],['Repairs OK',data.metrics?.total_repairs_successful ?? 0],['Repairs Failed',data.metrics?.total_repairs_failed ?? 0],['Active Jobs',(data.active_jobs||[]).length]];cards.forEach(([label,val])=>{const el=document.createElement('div');el.className='card';el.innerHTML=`<div>${label}</div><div class=\"metric\">${val}</div>`;summary.appendChild(el);});shards.innerHTML=(data.shards||[]).map(s=>`<tr><td>${s.shard_id||'-'}</td><td>${badge(s.status||'healthy')}</td><td>${s.documents_scanned}</td><td>${s.documents_healthy}</td><td>${s.documents_degraded}</td><td>${s.documents_unrecoverable}</td><td>${s.last_error||''}</td></tr>`).join('');jobs.innerHTML=(data.active_jobs||[]).map(j=>`<tr><td>${j.job_id}</td><td>${j.shard_id||'-'}</td><td>${j.document_id||'-'}</td><td>${j.is_full_scan?'yes':'no'}</td><td>${fmtTime(j.submitted_at_unix_ms)}</td><td>${j.completed?'yes':'no'}</td><td>${j.success?'yes':'no'}</td></tr>`).join('');}"
         << "async function post(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body||{})});const data=await res.json();if(!res.ok){throw new Error(data.message||data.error||'request failed');}return data;}"
         << "document.getElementById('refreshBtn').onclick=()=>load().catch(e=>setFlash(e.message));"
         << "document.getElementById('scanBtn').onclick=async()=>{const r=await post('/v1/admin/repair/scan',{});setFlash(`Full scan queued: ${r.job_id}`);load();};"
         << "document.getElementById('repairBtn').onclick=async()=>{const shardId=document.getElementById('shardId').value.trim();const r=await post('/v1/admin/repair',{shard_id:shardId});setFlash(`Repair queued: ${r.job_id}`);load();};"
         << "load().catch(e=>{raw.textContent=e.message;setFlash(e.message);});setInterval(()=>load().catch(()=>{}),10000);"
         << "</script></main></body></html>";
    return html.str();
}

} // namespace

ShardRepairApiHandler::ShardRepairApiHandler(
    std::shared_ptr<sharding::ShardRepairEngine> repair_engine,
    std::shared_ptr<themis::AuthMiddleware> auth)
    : repair_engine_(std::move(repair_engine))
    , auth_(std::move(auth)) {}

void ShardRepairApiHandler::setRepairEngine(
    std::shared_ptr<sharding::ShardRepairEngine> repair_engine) {
    repair_engine_ = std::move(repair_engine);
}

bool ShardRepairApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_scope,
    http::response<http::string_body>& out) const {
    if (!auth_ || !auth_->isEnabled()) {
        return true;
    }
    auto& auth = *auth_;

    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        out = makeErrorResponse(http::status::unauthorized,
                                "Missing Authorization header", req);
        return false;
    }

    auto token = AuthMiddleware::extractBearerToken(std::string(auth_header.data(), auth_header.size()));
    if (!token) {
        out = makeErrorResponse(http::status::unauthorized,
                                "Invalid Authorization header", req);
        return false;
    }

    auto ar = auth.authorize(*token, required_scope);
    // W1-FIX(missing_audit_log): every authorization decision must be recorded.
    // If a centralized AuditLogger is later injected, redirect there instead.
    if (ar.authorized) {
        THEMIS_INFO("[AUDIT] shard_repair authorize: scope='{}' user='{}' decision=ALLOW",
                    required_scope, ar.user_id);
    } else {
        THEMIS_WARN("[AUDIT] shard_repair authorize: scope='{}' user='{}' decision=DENY reason='{}'",
                    required_scope, ar.user_id, ar.reason);
    }
    if (!ar.authorized) {
        out = makeErrorResponse(http::status::forbidden,
                                "Insufficient scope: " + required_scope, req);
        return false;
    }

    return true;
}

http::response<http::string_body> ShardRepairApiHandler::handleHealth(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleShardRepairHealth");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:sharding:read", auth_resp)) {
        return auth_resp;
    }
    if (!repair_engine_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard repair engine not configured", req);
    }
    auto& repair_engine = *repair_engine_;
    auto metrics = repair_engine.getRepairMetrics();
    auto reports = repair_engine.getShardHealthReports();
    auto active_jobs = repair_engine.getActiveJobs();

    std::string overall = "healthy";
    for (const auto& report : reports) {
        if (report.status == sharding::ShardRepairStatus::FAILED) {
            overall = "failed";
            break;
        }
        if (report.status == sharding::ShardRepairStatus::DEGRADED ||
            report.status == sharding::ShardRepairStatus::REBUILDING) {
            overall = "degraded";
        }
    }

    json body = {
        {"status", overall},
        {"engine_running", repair_engine.isRunning()},
        {"metrics", metricsToJson(metrics)},
        {"active_jobs", json::array()},
        {"shards", json::array()}
    };

    for (const auto& job : active_jobs) {
        body["active_jobs"].push_back(repairJobToJson(job));
    }
    for (const auto& report : reports) {
        body["shards"].push_back(shardReportToJson(report));
    }

    return makeResponse(http::status::ok, body.dump(), "application/json", req);
}

http::response<http::string_body> ShardRepairApiHandler::handleTriggerRepair(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleShardRepairTrigger");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:sharding:write", auth_resp)) {
        return auth_resp;
    }
    if (!repair_engine_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard repair engine not configured", req);
    }
    auto& repair_engine = *repair_engine_;
    try {
        json body = req.body().empty() ? json::object() : json::parse(req.body());
        std::string job_id;
        std::string kind;
        if (body.contains("document_id") && body["document_id"].is_string()) {
            const std::string collection = body.value("collection", std::string{});
            
            // QW-46 Guard: Fail-closed collection name validation
            if (!collection.empty()) {
                utils::InputValidator validator;
                if (!validator.validateStringLength(collection, 256) || !validator.validatePathSegment(collection)) {
                    THEMIS_ERROR("QW-46 Guard: Invalid collection in handleTriggerRepair");
                    return makeErrorResponse(http::status::bad_request,
                        "Invalid collection: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
                }
            }
            
            job_id = repair_engine.triggerDocumentRepair(body["document_id"].get<std::string>(), collection);
            kind = "document";
        } else {
            const std::string shard_id = body.value("shard_id", std::string{});
            job_id = repair_engine.triggerRepair(shard_id);
            kind = shard_id.empty() ? "cluster" : "shard";
        }

        json response = {
            {"job_id", job_id},
            {"status", "queued"},
            {"kind", kind}
        };
        return makeResponse(http::status::accepted, response.dump(), "application/json", req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("Invalid JSON: ") + e.what(), req);
    }
}

http::response<http::string_body> ShardRepairApiHandler::handleTriggerFullScan(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleShardRepairFullScan");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:sharding:write", auth_resp)) {
        return auth_resp;
    }
    if (!repair_engine_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard repair engine not configured", req);
    }
    auto& repair_engine = *repair_engine_;
    const std::string job_id = repair_engine.triggerFullScan();
    json response = {
        {"job_id", job_id},
        {"status", "queued"},
        {"kind", "full_scan"}
    };
    return makeResponse(http::status::accepted, response.dump(), "application/json", req);
}

http::response<http::string_body> ShardRepairApiHandler::handleJobStatus(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleShardRepairJobStatus");
    http::response<http::string_body> auth_resp;
    if (!checkAuth(req, "admin:sharding:read", auth_resp)) {
        return auth_resp;
    }
    if (!repair_engine_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard repair engine not configured", req);
    }
    auto& repair_engine = *repair_engine_;
    const std::string job_id = extractJobId(std::string(req.target()));
    if (job_id.empty()) {
        return makeErrorResponse(http::status::bad_request, "Missing job id", req);
    }

    auto job = repair_engine.getJobStatus(job_id);
    auto status = (job.completed && !job.success && job.error_message == "Job not found")
        ? http::status::not_found
        : http::status::ok;
    return makeResponse(status, repairJobToJson(job).dump(), "application/json", req);
}

http::response<http::string_body> ShardRepairApiHandler::handleDashboard(
    const http::request<http::string_body>& req) {
    auto span = Tracer::startSpan("handleShardRepairDashboard");
    return makeResponse(http::status::ok, buildDashboardHtml(), "text/html; charset=utf-8", req);
}

http::response<http::string_body> ShardRepairApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const std::string& content_type,
    const http::request<http::string_body>& req) const {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, content_type);
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ShardRepairApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req) const {
    json body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, body.dump(), "application/json", req);
}

} // namespace server
} // namespace themis
