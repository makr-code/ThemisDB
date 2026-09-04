/**
 * @file admin_api.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/admin_api.h"
#include "sharding/shard_repair_engine.h"
#include "sharding/hardware_migration_manager.h"
#include "sharding/pki_shard_certificate.h"
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace sharding {

AdminAPI::AdminAPI(const Config& config)
    : config_(config) {
}

void AdminAPI::registerTopologyHandler([[maybe_unused]] RequestHandler handler) {
    topology_handler_ = handler;
}

void AdminAPI::registerRebalanceHandler([[maybe_unused]] RequestHandler handler) {
    rebalance_handler_ = handler;
}

void AdminAPI::registerHealthHandler([[maybe_unused]] RequestHandler handler) {
    health_handler_ = handler;
}

void AdminAPI::registerStatsHandler([[maybe_unused]] RequestHandler handler) {
    stats_handler_ = handler;
}

void AdminAPI::registerRepairHandler([[maybe_unused]] RequestHandler handler) {
    repair_handler_ = handler;
}

void AdminAPI::registerMigrateHardwareHandler([[maybe_unused]] RequestHandler handler) {
    migrate_hardware_handler_ = handler;
}

void AdminAPI::setMigrationManager(std::shared_ptr<HardwareMigrationManager> mgr) {
    migration_manager_ = std::move(mgr);
}

void AdminAPI::setRepairEngine(std::shared_ptr<ShardRepairEngine> engine) {
    repair_engine_ = std::move(engine);
}

nlohmann::json AdminAPI::handleRequest(const std::string& method, 
                                         const std::string& path,
                                         const nlohmann::json& body,
                                         const std::string& operator_cert) {
    // Authorize request
    if (!authorizeRequest(operator_cert)) {
        return createErrorResponse(403, "Unauthorized - invalid operator certificate");
    }

    // Audit log
    auditLog(method, path, operator_cert);

    // Route to appropriate handler
    if (path == Endpoints::TOPOLOGY && method == "GET") {
        if ([[maybe_unused]] topology_handler_) {
            return topology_handler_([[maybe_unused]] body);
        }
    } else if (path == Endpoints::SHARD_ADD && method == "POST") {
        if ([[maybe_unused]] topology_handler_) {
            return topology_handler_([[maybe_unused]] body);
        }
    } else if (path.find(Endpoints::SHARD_REMOVE) == 0 && method == "DELETE") {
        if ([[maybe_unused]] topology_handler_) {
            return topology_handler_([[maybe_unused]] body);
        }
    } else if (path == Endpoints::REBALANCE && method == "POST") {
        if ([[maybe_unused]] rebalance_handler_) {
            return rebalance_handler_([[maybe_unused]] body);
        }
    } else if (path.find(Endpoints::REBALANCE_STATUS) == 0 && method == "GET") {
        if ([[maybe_unused]] rebalance_handler_) {
            return rebalance_handler_([[maybe_unused]] body);
        }
    } else if (path == Endpoints::HEALTH && method == "GET") {
        nlohmann::json health_response;
        if ([[maybe_unused]] health_handler_) {
            health_response = health_handler_([[maybe_unused]] body);
        }
        // Enrich with per-shard repair health when a repair engine is attached
        nlohmann::json repair_health = buildRepairHealthJson();
        if (!repair_health.empty()) {
            health_response["repair"] = repair_health;
        }
        if (health_response.empty()) {
            return createErrorResponse(404, "Endpoint not found");
        }
        return health_response;
    } else if (path == Endpoints::STATS && method == "GET") {
        if ([[maybe_unused]] stats_handler_) {
            return stats_handler_([[maybe_unused]] body);
        }
    } else if (path == Endpoints::REPAIR && method == "POST") {
        if ([[maybe_unused]] repair_handler_) {
            return repair_handler_([[maybe_unused]] body);
        }
    } else if (path == Endpoints::REPAIR_SCAN && method == "POST") {
        if ([[maybe_unused]] repair_handler_) {
            nlohmann::json scan_body = body;
            scan_body["full_scan"] = true;
            return repair_handler_([[maybe_unused]] scan_body);
        }
    } else if (path.find(Endpoints::REPAIR_STATUS) == 0 && method == "GET") {
        if ([[maybe_unused]] repair_handler_) {
            // Extract job_id from path: /admin/repair/{job_id}
            std::string job_id = path.substr(std::string(Endpoints::REPAIR_STATUS).size());
            nlohmann::json status_body = body;
            status_body["job_id"] = job_id;
            return repair_handler_([[maybe_unused]] status_body);
        }
    } else if (path.find(Endpoints::MIGRATE_HARDWARE_PREFIX) == 0
               && path.find(Endpoints::MIGRATE_HARDWARE_SUFFIX) != std::string::npos
               && method == "POST") {
        // Extract shard_id from path: /api/v1/shards/{id}/migrate-hardware
        const std::string prefix = Endpoints::MIGRATE_HARDWARE_PREFIX;
        const std::string suffix = Endpoints::MIGRATE_HARDWARE_SUFFIX;
        std::string mid = path.substr(prefix.size());
        auto pos = mid.rfind(suffix);
        if (pos != std::string::npos) {
            std::string shard_id = mid.substr(0, pos);
            return handleMigrateHardware(shard_id, body);
        }
    }

    return createErrorResponse(404, "Endpoint not found");
}

bool AdminAPI::authorizeRequest(const std::string& operator_cert) {
    // Test/development mode: signature verification is disabled, but an
    // explicit non-empty operator identity is still required.
    if (!config_.require_signatures) {
        return !operator_cert.empty();
    }

    if (operator_cert.empty()) {
        return false;
    }

    // Parse the PEM certificate from the caller-supplied string.
    BIO* bio = BIO_new_mem_buf(operator_cert.data(),
                               static_cast<int>(operator_cert.size()));
    if (!bio) {
      return false;
    }

    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!cert) {
        return false;  // Not a valid PEM certificate.
    }

    // Verify certificate validity window (not-before / not-after vs. now).
    const int nb_cmp = X509_cmp_current_time(X509_get0_notBefore(cert));
    const int na_cmp = X509_cmp_current_time(X509_get0_notAfter(cert));
    if (nb_cmp > 0 || na_cmp < 0) {
        // Certificate is not yet valid or has expired.
        X509_free(cert);
        return false;
    }

    // If a CA certificate path is configured, verify the certificate chain.
    if (!config_.ca_cert_path.empty()) {
        X509_STORE* store = X509_STORE_new();
        bool chain_ok = false;
        if (store) {
            if (X509_STORE_load_locations(store, config_.ca_cert_path.c_str(), nullptr) == 1) {
                X509_STORE_CTX* ctx = X509_STORE_CTX_new();
                if (ctx && X509_STORE_CTX_init(ctx, store, cert, nullptr) == 1) {
                    chain_ok = (X509_verify_cert(ctx) == 1);
                }
                X509_STORE_CTX_free(ctx);
            }
            X509_STORE_free(store);
        }
        if (!chain_ok) {
            X509_free(cert);
            return false;
        }
    }

    // Check for admin-capability indicator: look for "admin" or "themis-admin"
    // in the Subject CN or SAN DNS names.
    bool has_admin_cap = false;

    // Check Subject CN.
    X509_NAME* subject = X509_get_subject_name(cert);
    if (subject) {
        char cn_buf[256] = {};
        X509_NAME_get_text_by_NID(subject, NID_commonName, cn_buf, sizeof(cn_buf));
        std::string cn(cn_buf);
        if (cn.find("admin") != std::string::npos) {
            has_admin_cap = true;
        }
    }

    // Check Subject Alternative Name DNS entries.
    if (!has_admin_cap) {
        GENERAL_NAMES* sans = static_cast<GENERAL_NAMES*>(
            X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
        if (sans) {
            for (int i = 0; i < sk_GENERAL_NAME_num(sans); ++i) {
                GENERAL_NAME* gn = sk_GENERAL_NAME_value(sans, i);
                if (gn->type == GEN_DNS) {
                    ASN1_STRING* dns = gn->d.dNSName;
                    std::string dns_str(
                        reinterpret_cast<const char*>(ASN1_STRING_get0_data(dns)),
                        ASN1_STRING_length(dns));
                    if (dns_str.find("admin") != std::string::npos) {
                        has_admin_cap = true;
                        break;
                    }
                }
            }
            GENERAL_NAMES_free(sans);
        }
    }

    X509_free(cert);

    // When no CA is configured, any structurally valid, non-expired certificate
    // with an admin indicator in CN/SAN is accepted (development / test mode).
    // In production, `ca_cert_path` should always be set.
    return has_admin_cap;
}

void AdminAPI::auditLog(const std::string& method, const std::string& path, const std::string& operator_cert) {
    if (!config_.enable_audit_log) {
      return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ofstream log_file(config_.audit_log_path, std::ios::app);
    if (log_file.is_open()) {
        log_file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                 << " | " << method 
                 << " | " << path
                 << " | " << operator_cert.substr(0, 20) << "..."
                 << std::endl;
    }
}

nlohmann::json AdminAPI::createErrorResponse(int code, const std::string& message) {
    return {
        {"success", false},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };
}

nlohmann::json AdminAPI::buildRepairHealthJson() const {
    if (!repair_engine_) {
        return nlohmann::json{};
    }

    static const char* kStatusStr[] = {"healthy", "degraded", "failed", "rebuilding"};

    auto reports = repair_engine_->getShardHealthReports();
    auto metrics  = repair_engine_->getRepairMetrics();

    nlohmann::json repair;

    // Overall cluster repair health
    std::string overall = "healthy";
    for (const auto& r : reports) {
        if (r.status == ShardRepairStatus::FAILED) {
            overall = "failed";
            break;
        }
        if (r.status == ShardRepairStatus::DEGRADED || r.status == ShardRepairStatus::REBUILDING) {
            overall = "degraded";
        }
    }
    repair["status"] = overall;
    repair["engine_running"] = repair_engine_->isRunning();
    repair["total_scans"] = metrics.total_scans;
    repair["repairs_attempted"] = metrics.total_repairs_attempted;
    repair["repairs_successful"] = metrics.total_repairs_successful;
    repair["repairs_failed"] = metrics.total_repairs_failed;
    repair["avg_repair_ms"] = metrics.avg_repair_time_ms.count();

    nlohmann::json shards = nlohmann::json::array();
    for (const auto& r : reports) {
        int status_idx = static_cast<int>(r.status);
        nlohmann::json shard_entry;
        shard_entry["shard_id"] = r.shard_id;
        shard_entry["status"] = kStatusStr[status_idx];
        shard_entry["documents_scanned"] = r.documents_scanned;
        shard_entry["documents_healthy"] = r.documents_healthy;
        shard_entry["documents_degraded"] = r.documents_degraded;
        shard_entry["documents_unrecoverable"] = r.documents_unrecoverable;
        if (!r.last_error.empty()) {
            shard_entry["last_error"] = r.last_error;
        }
        shards.push_back(std::move(shard_entry));
    }
    repair["shards"] = std::move(shards);

    return repair;
}

nlohmann::json AdminAPI::handleMigrateHardware(const std::string& shard_id,
                                                  const nlohmann::json& body) {
    // Custom handler takes precedence over the built-in path.
    if ([[maybe_unused]] migrate_hardware_handler_) {
        nlohmann::json req = body;
        req["shard_id"] = shard_id;
        return migrate_hardware_handler_([[maybe_unused]] req);
    }

    if (!migration_manager_) {
        return createErrorResponse(501, "Hardware migration manager not configured");
    }

    if (shard_id.empty()) {
        return createErrorResponse(400, "shard_id must not be empty");
    }

    std::string new_endpoint;
    if (body.contains("new_endpoint") && body["new_endpoint"].is_string()) {
        new_endpoint = body["new_endpoint"].get<std::string>();
    }
    if (new_endpoint.empty()) {
        return createErrorResponse(400, "new_endpoint is required");
    }

    auto result = migration_manager_->replaceEndpoint(shard_id, new_endpoint);

    if (!result.success) {
        return {
            {"success", false},
            {"shard_id", result.shard_id},
            {"old_endpoint", result.old_endpoint},
            {"new_endpoint", result.new_endpoint},
            {"message", result.message}
        };
    }

    return {
        {"success", true},
        {"shard_id", result.shard_id},
        {"old_endpoint", result.old_endpoint},
        {"new_endpoint", result.new_endpoint},
        {"message", result.message}
    };
}

} // namespace sharding
} // namespace themis

