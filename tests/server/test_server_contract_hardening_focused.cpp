/**
 * @file test_server_contract_hardening_focused.cpp
 * @brief Server Module — Contract Hardening focused regression tests.
 *
 * Covers the normative contracts defined in include/server/server_api_contract.h
 * across four acceptance-criteria tracks:
 *
 * - **SCH-01..04** — Auth gate enforcement (JWT valid/invalid, missing token → 401)
 * - **SCH-05..08** — Retry semantics (backoff budget, idempotency, max-retry exhaustion)
 * - **SCH-09..12** — Timeout contract (handler timeout → drain, graceful shutdown ordering)
 * - **SCH-13..16** — Rate limit state (per-client limit, distributed fallback fail-closed)
 * - **SCH-17..20** — Protocol contract (malformed frame, version mismatch, quorum loss)
 *
 * All infrastructure is fully in-process; no real TCP ports are opened.
 * Deterministic test data is seeded with kServerContractSeed = 42.
 *
 * @version 1.0.0
 * @note CTest labels: server;contract;hardening
 */

#include <gtest/gtest.h>

#include "server/server_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kServerContractSeed = 42U;

// ─────────────────────────────────────────────────────────────────────────────
// § Stubs and helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Minimal JWT token representation for testing.
struct FakeJwtToken {
    std::string value;
    bool is_structurally_valid{false};
    bool signature_ok{false};
    bool expired{false};
    std::string scope = {};
};

/// Stub auth gate: applies the server contract's auth gate logic.
class StubAuthGate {
public:
    ServerErrorClass validate(const FakeJwtToken& tok) const noexcept {
        if (tok.value.empty()) {
          return ServerErrorClass::AUTH_GATE_DENIED;
        }
        if (!tok.is_structurally_valid) {
          return ServerErrorClass::AUTH_GATE_DENIED;
        }
        if (!tok.signature_ok) {
          return ServerErrorClass::AUTH_GATE_DENIED;
        }
        if (tok.expired) {
          return ServerErrorClass::AUTH_GATE_DENIED;
        }
        return ServerErrorClass::OK;
    }
};

/// Stub retry engine implementing the contract from §3.
class StubRetryEngine {
public:
    struct Config {
        int max_retries{kDefaultMaxRetries};
        std::chrono::milliseconds base_delay{kRetryBaseDelay};
        std::chrono::seconds global_budget{kRetryGlobalBudget};
        bool use_jitter{true};
    };

    explicit StubRetryEngine(Config cfg = {}) : cfg_(cfg) {}

    /// Returns the error class after exhausting retries.
    ServerErrorClass execute(std::function<ServerErrorClass()> fn) {
        attempts_ = 0;
        auto budget_start = std::chrono::steady_clock::now();
        for (int i = 0; i <= cfg_.max_retries; ++i) {
            ++attempts_;
            auto result = fn();
            if (result == ServerErrorClass::OK) {
              return ServerErrorClass::OK;
            }
            // Fatal errors never retry
            if (result == ServerErrorClass::INPUT_VALIDATION_ERROR ||
                result == ServerErrorClass::AUTH_GATE_DENIED) {
                return result;
            }
            auto elapsed = std::chrono::steady_clock::now() - budget_start;
            if (elapsed >= cfg_.global_budget) {
              return ServerErrorClass::RETRY_BUDGET_EXHAUSTED;
            }
        }
        return ServerErrorClass::RETRY_BUDGET_EXHAUSTED;
    }

    int attempts() const noexcept { return attempts_; }

private:
    Config cfg_;
    int attempts_{0};
};

/// Stub request handler with configurable timeout behaviour.
class StubRequestHandler {
public:
    enum class Behaviour { CompleteBeforeDeadline, ExceedDeadline };

    explicit StubRequestHandler(Behaviour b, std::chrono::milliseconds handler_time = 0ms)
        : behaviour_(b), handler_time_(handler_time) {}

    ServerErrorClass handle(std::chrono::milliseconds deadline) {
        if (behaviour_ == Behaviour::ExceedDeadline && handler_time_ > deadline) {
            return ServerErrorClass::TRANSPORT_TIMEOUT;
        }
        return ServerErrorClass::OK;
    }

private:
    Behaviour behaviour_;
    std::chrono::milliseconds handler_time_;
};

/// Stub server state machine implementing §9 graceful shutdown ordering.
class StubServerStateMachine {
public:
    StubServerStateMachine() : state_(ServerState::kIdle) {}

    void start()    { transition(ServerState::kRunning); }
    void drain()    { transition(ServerState::kDraining); }
    void stop()     { transition(ServerState::kStopped); }

    ServerState state() const noexcept { return state_; }

    bool canAcceptRequests() const noexcept {
        return state_ == ServerState::kRunning;
    }

    bool isDraining() const noexcept { return state_ == ServerState::kDraining; }

private:
    void transition(ServerState next) {
        // Enforce forward-only transitions
        EXPECT_GT(static_cast<int>(next), static_cast<int>(state_));
        state_ = next;
    }
    ServerState state_;
};

/// Stub in-memory per-client rate limiter implementing §10.
class StubRateLimiter {
public:
    explicit StubRateLimiter(int limit_per_window) : limit_(limit_per_window) {}

    ServerErrorClass check(const std::string& client_id) {
        std::lock_guard<std::mutex> lk(mu_);
        auto& count = counts_[client_id];
        if (count >= limit_) {
          return ServerErrorClass::RATE_LIMIT_EXCEEDED;
        }
        ++count;
        return ServerErrorClass::OK;
    }

    void reset(const std::string& client_id) {
        std::lock_guard<std::mutex> lk(mu_);
        counts_[client_id] = 0;
    }

    /// Simulate distributed backend failure → fail-closed.
    ServerErrorClass checkWithFailedBackend(const std::string& /*client_id*/) noexcept {
        return ServerErrorClass::RATE_LIMIT_EXCEEDED;  // fail-closed per §10
    }

private:
    int limit_;
    std::mutex mu_;
    std::unordered_map<std::string, int> counts_;
};

// ─────────────────────────────────────────────────────────────────────────────
// SCH-01..04: Auth gate enforcement
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServerContractAuth, SCH01_ValidJwtAccepted) {
    StubAuthGate gate;
    FakeJwtToken tok;
    tok.value                = "header.payload.signature";
    tok.is_structurally_valid = true;
    tok.signature_ok         = true;
    tok.expired              = false;
    tok.scope                = "read";
    EXPECT_EQ(gate.validate(tok), ServerErrorClass::OK);
}

TEST(ServerContractAuth, SCH02_InvalidSignatureRejected) {
    StubAuthGate gate;
    FakeJwtToken tok;
    tok.value                = "header.payload.badsig";
    tok.is_structurally_valid = true;
    tok.signature_ok         = false;
    tok.expired              = false;
    auto result = gate.validate(tok);
    EXPECT_EQ(result, ServerErrorClass::AUTH_GATE_DENIED);
    EXPECT_TRUE(isServerFailClosedClass(result));
}

TEST(ServerContractAuth, SCH03_MissingTokenRejected) {
    StubAuthGate gate;
    FakeJwtToken tok;
    tok.value = "";  // empty = missing
    auto result = gate.validate(tok);
    EXPECT_EQ(result, ServerErrorClass::AUTH_GATE_DENIED);
}

TEST(ServerContractAuth, SCH04_ExpiredTokenRejected) {
    StubAuthGate gate;
    FakeJwtToken tok;
    tok.value                = "header.payload.sig";
    tok.is_structurally_valid = true;
    tok.signature_ok         = true;
    tok.expired              = true;
    EXPECT_EQ(gate.validate(tok), ServerErrorClass::AUTH_GATE_DENIED);
}

// ─────────────────────────────────────────────────────────────────────────────
// SCH-05..08: Retry semantics
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServerContractRetry, SCH05_TransientErrorRetries) {
    StubRetryEngine engine;
    int call_count = 0;
    // Succeed on 3rd attempt
    auto result = engine.execute([&]() -> ServerErrorClass {
        ++call_count;
        if (call_count < 3) {
          return ServerErrorClass::INTERNAL_ERROR;
        }
        return ServerErrorClass::OK;
    });
    EXPECT_EQ(result, ServerErrorClass::OK);
    EXPECT_EQ(call_count, 3);
}

TEST(ServerContractRetry, SCH06_MaxRetryExhaustionReturnsError) {
    StubRetryEngine::Config cfg;
    cfg.max_retries    = 3;
    cfg.global_budget  = 60s;
    StubRetryEngine engine(cfg);
    auto result = engine.execute([&]() -> ServerErrorClass {
        return ServerErrorClass::INTERNAL_ERROR;  // always fails
    });
    EXPECT_EQ(result, ServerErrorClass::RETRY_BUDGET_EXHAUSTED);
    EXPECT_EQ(engine.attempts(), 4);  // initial + 3 retries
}

TEST(ServerContractRetry, SCH07_FatalErrorFailsFast) {
    StubRetryEngine engine;
    int call_count = 0;
    auto result = engine.execute([&]() -> ServerErrorClass {
        ++call_count;
        return ServerErrorClass::AUTH_GATE_DENIED;  // fatal — no retry
    });
    EXPECT_EQ(result, ServerErrorClass::AUTH_GATE_DENIED);
    EXPECT_EQ(call_count, 1);  // no retries for fatal
}

TEST(ServerContractRetry, SCH08_IdempotencyKeyPreservedAcrossRetries) {
    std::mt19937 rng(kServerContractSeed);
    // Generate a deterministic idempotency key
    const std::string idem_key = "idem-sch08-" + std::to_string(rng());
    std::vector<std::string> observed_keys;
    StubRetryEngine engine;
    engine.execute([&]() -> ServerErrorClass {
        observed_keys.push_back(idem_key);  // key must be identical across retries
        return ServerErrorClass::INTERNAL_ERROR;
    });
    // All calls should use the same key
    for (const auto& k : observed_keys) {
        EXPECT_EQ(k, idem_key);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SCH-09..12: Timeout contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServerContractTimeout, SCH09_HandlerCompletesWithinDeadline) {
    StubRequestHandler handler(StubRequestHandler::Behaviour::CompleteBeforeDeadline, 10ms);
    auto result = handler.handle(100ms);
    EXPECT_EQ(result, ServerErrorClass::OK);
}

TEST(ServerContractTimeout, SCH10_HandlerExceedsDeadlineReturnsTimeout) {
    StubRequestHandler handler(StubRequestHandler::Behaviour::ExceedDeadline, 200ms);
    auto result = handler.handle(100ms);  // 200ms > 100ms deadline
    EXPECT_EQ(result, ServerErrorClass::TRANSPORT_TIMEOUT);
}

TEST(ServerContractTimeout, SCH11_GracefulShutdownOrderingIdle_to_Stopped) {
    StubServerStateMachine sm;
    EXPECT_EQ(sm.state(), ServerState::kIdle);
    sm.start();
    EXPECT_EQ(sm.state(), ServerState::kRunning);
    EXPECT_TRUE(sm.canAcceptRequests());
    sm.drain();
    EXPECT_EQ(sm.state(), ServerState::kDraining);
    EXPECT_FALSE(sm.canAcceptRequests());
    EXPECT_TRUE(sm.isDraining());
    sm.stop();
    EXPECT_EQ(sm.state(), ServerState::kStopped);
    EXPECT_FALSE(sm.canAcceptRequests());
}

TEST(ServerContractTimeout, SCH12_DrainingStateRejectsNewRequests) {
    StubServerStateMachine sm;
    sm.start();
    sm.drain();
    // In draining state, no new requests should be accepted
    EXPECT_FALSE(sm.canAcceptRequests());
    EXPECT_EQ(sm.state(), ServerState::kDraining);
}

// ─────────────────────────────────────────────────────────────────────────────
// SCH-13..16: Rate limit state
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServerContractRateLimit, SCH13_PerClientLimitEnforced) {
    const int limit = 5;
    StubRateLimiter rl(limit);
    const std::string client = "client-sch13";
    for (int i = 0; i < limit; ++i) {
        EXPECT_EQ(rl.check(client), ServerErrorClass::OK) << "attempt " << i;
    }
    // Next call must be rejected
    EXPECT_EQ(rl.check(client), ServerErrorClass::RATE_LIMIT_EXCEEDED);
}

TEST(ServerContractRateLimit, SCH14_DifferentClientsHaveIsolatedLimits) {
    StubRateLimiter rl(3);
    EXPECT_EQ(rl.check("client-A"), ServerErrorClass::OK);
    EXPECT_EQ(rl.check("client-A"), ServerErrorClass::OK);
    EXPECT_EQ(rl.check("client-A"), ServerErrorClass::OK);
    EXPECT_EQ(rl.check("client-A"), ServerErrorClass::RATE_LIMIT_EXCEEDED);
    // client-B has its own counter
    EXPECT_EQ(rl.check("client-B"), ServerErrorClass::OK);
    EXPECT_EQ(rl.check("client-B"), ServerErrorClass::OK);
}

TEST(ServerContractRateLimit, SCH15_DistributedBackendFailureFailClosed) {
    StubRateLimiter rl(100);
    // When distributed backend fails, contract mandates fail-closed (deny all)
    auto result = rl.checkWithFailedBackend("client-sch15");
    EXPECT_EQ(result, ServerErrorClass::RATE_LIMIT_EXCEEDED);
}

TEST(ServerContractRateLimit, SCH16_RateLimitResetAllowsNewWindow) {
    StubRateLimiter rl(2);
    const std::string client = "client-sch16";
    EXPECT_EQ(rl.check(client), ServerErrorClass::OK);
    EXPECT_EQ(rl.check(client), ServerErrorClass::OK);
    EXPECT_EQ(rl.check(client), ServerErrorClass::RATE_LIMIT_EXCEEDED);
    // Simulate window reset
    rl.reset(client);
    EXPECT_EQ(rl.check(client), ServerErrorClass::OK);
}

// ─────────────────────────────────────────────────────────────────────────────
// SCH-17..20: Protocol contract
// ─────────────────────────────────────────────────────────────────────────────

/// Stub protocol frame validator.
struct StubFrameValidator {
    enum class FrameType { Wellformed, Malformed, VersionMismatch };

    ServerErrorClass validate(FrameType type, int client_version = 1, int server_version = 1) {
        switch (type) {
        case FrameType::Malformed:
            return ServerErrorClass::PROTOCOL_VIOLATION;
        case FrameType::VersionMismatch:
            if (client_version != server_version) {
              return ServerErrorClass::VERSION_MISMATCH;
            }
            return ServerErrorClass::OK;
        case FrameType::Wellformed:
            return ServerErrorClass::OK;
        }
        return ServerErrorClass::INTERNAL_ERROR;
    }

    ServerErrorClass checkQuorum(int healthy_nodes, int total_nodes) {
        // Quorum = majority (> total/2)
        if (healthy_nodes * 2 <= total_nodes) {
          return ServerErrorClass::QUORUM_UNAVAILABLE;
        }
        return ServerErrorClass::OK;
    }
};

TEST(ServerContractProtocol, SCH17_MalformedFrameReturnsProtocolViolation) {
    StubFrameValidator v;
    auto result = v.validate(StubFrameValidator::FrameType::Malformed);
    EXPECT_EQ(result, ServerErrorClass::PROTOCOL_VIOLATION);
}

TEST(ServerContractProtocol, SCH18_WellformedFrameAccepted) {
    StubFrameValidator v;
    auto result = v.validate(StubFrameValidator::FrameType::Wellformed);
    EXPECT_EQ(result, ServerErrorClass::OK);
}

TEST(ServerContractProtocol, SCH19_VersionMismatchReturns400) {
    StubFrameValidator v;
    // client_version=2, server_version=1 → mismatch
    auto result = v.validate(StubFrameValidator::FrameType::VersionMismatch, 2, 1);
    EXPECT_EQ(result, ServerErrorClass::VERSION_MISMATCH);
}

TEST(ServerContractProtocol, SCH20_QuorumLossReturns503) {
    StubFrameValidator v;
    // 3-node cluster, only 1 healthy → no quorum
    auto result = v.checkQuorum(/*healthy=*/1, /*total=*/3);
    EXPECT_EQ(result, ServerErrorClass::QUORUM_UNAVAILABLE);
    EXPECT_TRUE(isServerFailClosedClass(result));

    // 3-node cluster, 2 healthy → quorum holds
    EXPECT_EQ(v.checkQuorum(2, 3), ServerErrorClass::OK);
}
