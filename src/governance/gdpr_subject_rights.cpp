/**
 * @file gdpr_subject_rights.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/gdpr_subject_rights.h"

#include <sstream>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ============================================================================
// ErasureReport helpers
// ============================================================================

std::unordered_map<std::string, std::string> ErasureReport::toSummaryMap() const {
    std::unordered_map<std::string, std::string> m;
    m["subject_id"]   = subject_id;
    m["regulation"]   = (regulation == Regulation::GDPR) ? "GDPR" : "CCPA";
    m["reason"]       = reason;
    m["operator_id"]  = operator_id;
    m["fully_erased"] = fully_erased ? "true" : "false";
    size_t ok         = 0;
    for (const auto &r : store_results) {
        if (r.success) {
            ++ok;
        }
    }
    m["stores_ok"]     = std::to_string(ok);
    m["stores_failed"] = std::to_string(store_results.size() - ok);
    return m;
}

// ============================================================================
// GdprSubjectRightsManager
// ============================================================================

GdprSubjectRightsManager::GdprSubjectRightsManager(TsaSigner tsa_signer) : tsa_signer_(std::move(tsa_signer)) {}

void GdprSubjectRightsManager::registerEraseTarget(std::shared_ptr<IGdprEraseTarget> target) {
    if (!target) {
        throw std::invalid_argument("GdprSubjectRightsManager: target must not be null");
    }
    if (target->storeId().empty()) {
        throw std::invalid_argument("GdprSubjectRightsManager: target storeId must not be empty");
    }
    std::lock_guard<std::mutex> lock(targets_mutex_);
    targets_.push_back(std::move(target));
}

size_t GdprSubjectRightsManager::targetCount() const {
    std::lock_guard<std::mutex> lock(targets_mutex_);
    return targets_.size();
}

std::mutex &GdprSubjectRightsManager::getSubjectMutex(const std::string &subject_id) {
    std::lock_guard<std::mutex> lock(subject_map_mutex_);
    return subject_mutexes_[subject_id];
}

ErasureReport GdprSubjectRightsManager::requestErasure(const std::string &subject_id, Regulation regulation,
                                                       const std::string &reason, const std::string &operator_id) {
    if (subject_id.empty()) {
        throw std::invalid_argument("GdprSubjectRightsManager::requestErasure: subject_id empty");
    }

    // Serialise concurrent erasure requests for the same subject
    std::lock_guard<std::mutex> subject_lock(getSubjectMutex(subject_id));

    THEMIS_INFO("GdprSubjectRights: ERASURE request subject='{}' regulation={} operator='{}'", subject_id,
                regulation == Regulation::GDPR ? "GDPR" : "CCPA", operator_id);

    // Snapshot targets under lock, then release for the actual erasure calls
    std::vector<std::shared_ptr<IGdprEraseTarget>> snapshot;
    {
        std::lock_guard<std::mutex> lock(targets_mutex_);
        snapshot = targets_;
    }

    ErasureReport report;
    report.subject_id  = subject_id;
    report.regulation  = regulation;
    report.reason      = reason;
    report.operator_id = operator_id;
    report.timestamp   = std::chrono::system_clock::now();

    bool all_ok = true;
    for (auto &target : snapshot) {
        try {
            auto res = target->eraseSubject(subject_id, regulation);
            if (!res.success) {
                all_ok = false;
                THEMIS_ERROR("GdprSubjectRights: erasure FAILED store='{}' subject='{}' error='{}'", target->storeId(),
                             subject_id, res.error_message);
            } else {
                THEMIS_INFO("GdprSubjectRights: erased store='{}' subject='{}' records={}", target->storeId(),
                            subject_id, res.records_erased);
            }
            report.store_results.push_back(std::move(res));
        } catch (const std::exception &e) {
            all_ok = false;
            StoreErasureResult err;
            err.store_id      = target->storeId();
            err.success       = false;
            err.error_message = e.what();
            THEMIS_ERROR("GdprSubjectRights: erasure EXCEPTION store='{}' subject='{}': {}", target->storeId(),
                         subject_id, e.what());
            report.store_results.push_back(std::move(err));
        }
    }

    report.fully_erased = all_ok;
    THEMIS_INFO("GdprSubjectRights: erasure COMPLETE subject='{}' fully_erased={}", subject_id, all_ok);
    return report;
}

PortabilityPackage GdprSubjectRightsManager::requestPortability(const std::string &subject_id,
                                                                const std::string &format) {
    if (subject_id.empty()) {
        throw std::invalid_argument("requestPortability: subject_id empty");
    }

    std::vector<std::shared_ptr<IGdprEraseTarget>> snapshot;
    {
        std::lock_guard<std::mutex> lock(targets_mutex_);
        snapshot = targets_;
    }

    THEMIS_INFO("GdprSubjectRights: PORTABILITY request subject='{}' format='{}'", subject_id, format);

    // Collect and concatenate exports from all targets
    std::string combined = {};
    if (format == "json") {
        combined   = "[";
        bool first = true;
        for (auto &target : snapshot) {
            try {
                auto data = target->exportSubjectData(subject_id, format);
                if (!data.empty()) {
                    if (!first) {
                        combined += ",";
                    }
                    combined += std::string(data.begin(), data.end());
                    first = false;
                }
            } catch (const std::exception &e) {
                THEMIS_WARN("GdprSubjectRights: export failed store='{}': {}", target->storeId(), e.what());
            }
        }
        combined += "]";
    } else {
        // CSV: concatenate with newline separator
        for (auto &target : snapshot) {
            try {
                auto data = target->exportSubjectData(subject_id, format);
                if (!data.empty()) {
                    combined += std::string(data.begin(), data.end());
                    if (combined.back() != '\n') {
                        combined += '\n';
                    }
                }
            } catch (...) {}
        }
    }

    PortabilityPackage pkg;
    pkg.subject_id = subject_id;
    pkg.format     = format;
    pkg.payload    = std::vector<uint8_t>(combined.begin(), combined.end());
    pkg.issued_at  = std::chrono::system_clock::now();

    // Sign with TSA if configured (eIDAS Art. 20 legal evidence)
    if (tsa_signer_ && !pkg.payload.empty()) {
        try {
            pkg.tsa_signature = tsa_signer_(pkg.payload);
            THEMIS_INFO("GdprSubjectRights: portability package TSA-signed subject='{}'", subject_id);
        } catch (const std::exception &e) {
            THEMIS_WARN("GdprSubjectRights: TSA signing failed: {}", e.what());
        }
    }

    return pkg;
}

} // namespace governance
} // namespace themis

