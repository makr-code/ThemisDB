/**
 * @file tsa_api.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/tsa_api.h"

namespace themis {
namespace security {

// ============================================================================
// TSAResponse helpers
// ============================================================================

TSAResponse TSAResponse::fromToken(const TimestampToken& tok,
                                   std::chrono::milliseconds latency) {
    TSAResponse r;
    r.success           = tok.success;
    r.timestamp_utc     = tok.timestamp_utc;
    r.timestamp_unix_ms = tok.timestamp_unix_ms;
    r.serial_number     = tok.serial_number;
    r.policy_oid        = tok.policy_oid;
    r.hash_algorithm    = tok.hash_algorithm;
    r.token_der         = tok.token_der;
    r.token_b64         = tok.token_b64;
    r.tsa_name          = tok.tsa_name;
    r.tsa_serial        = tok.tsa_serial;
    r.tsa_cert          = tok.tsa_cert;
    r.has_accuracy      = tok.has_accuracy;
    r.accuracy_seconds  = tok.accuracy_seconds;
    r.accuracy_millis   = tok.accuracy_millis;
    r.accuracy_micros   = tok.accuracy_micros;
    r.ordering          = tok.ordering;
    r.error_message     = tok.error_message;
    r.http_status       = tok.status_code;
    r.pki_status        = tok.pki_status;
    r.request_latency   = latency;
    return r;
}

// ============================================================================
// TSAClientWrapper
// ============================================================================

TSAClientWrapper::TSAClientWrapper(std::unique_ptr<TimestampAuthority> authority,
                                   TSAEventHooks hooks)
    : authority_(std::move(authority)), hooks_(std::move(hooks)) {}

TSAClientWrapper::TSAClientWrapper(TSAClientWrapper&&) noexcept = default;
TSAClientWrapper& TSAClientWrapper::operator=(TSAClientWrapper&&) noexcept = default;

void TSAClientWrapper::fireError(const std::string& msg) {
    last_error_ = msg;
    if (hooks_.on_error) {
        hooks_.on_error(msg);
    }
}

TSAResponse TSAClientWrapper::requestTimestamp(const TSARequest& req) {
    auto t0 = std::chrono::steady_clock::now();

    TimestampToken tok;
    if (req.data_is_hash) {
        tok = authority_->getTimestampForHash(req.data);
    } else {
        tok = authority_->getTimestamp(req.data);
    }

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    if (!tok.success) {
        fireError(tok.error_message.empty() ? authority_->getLastError()
                                            : tok.error_message);
    }

    TSAResponse resp = TSAResponse::fromToken(tok, latency);

    if (hooks_.on_timestamp_issued) {
        hooks_.on_timestamp_issued(req, resp);
    }

    return resp;
}

bool TSAClientWrapper::verifyToken(const TSAVerifyRequest& req) {
    // Resolve the DER token: prefer token_der, fall back to token_b64.
    TimestampToken tok;
    if (!req.token_der.empty()) {
        tok = authority_->parseToken(req.token_der);
    } else if (!req.token_b64.empty()) {
        tok = authority_->parseToken(req.token_b64);
    } else {
        fireError("verifyToken: neither token_der nor token_b64 provided");
        if (hooks_.on_token_verified) {
            hooks_.on_token_verified(req, false);
        }
        return false;
    }

    bool valid = false;
    if (req.data_is_hash) {
        valid = authority_->verifyTimestampForHash(req.data, tok);
    } else {
        valid = authority_->verifyTimestamp(req.data, tok);
    }

    if (!valid) {
        last_error_ = authority_->getLastError();
    }

    if (hooks_.on_token_verified) {
        hooks_.on_token_verified(req, valid);
    }

    return valid;
}

bool TSAClientWrapper::isAvailable() {
    return authority_->isAvailable();
}

std::string TSAClientWrapper::getLastError() const {
    return last_error_;
}

// ============================================================================
// Factory
// ============================================================================

std::unique_ptr<ITSAClient> createTSAClient(TSAConfig config,
                                            TSAEventHooks hooks) {
    auto authority = std::make_unique<TimestampAuthority>(std::move(config));
    return std::make_unique<TSAClientWrapper>(std::move(authority),
                                             std::move(hooks));
}

} // namespace security
} // namespace themis
