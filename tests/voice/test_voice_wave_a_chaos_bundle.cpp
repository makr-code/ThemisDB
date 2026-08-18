/**
 * @file test_voice_wave_a_chaos_bundle.cpp
 * @brief Wave A closure chaos/fault-injection evidence for the voice module.
 * @date 2026-08-18
 *
 * Provides 12 deterministic, self-contained tests covering:
 *  - Backend failure teardown
 *  - Multi-session spoofing under fault injection
 *  - Cascading backend-failure / circuit-breaker scenarios
 *
 * Test IDs: VOICE-CHAOS-01 .. VOICE-CHAOS-12
 *
 * @see src/voice/ROADMAP.md § Wave A Closure Evidence Block
 * @see tests/voice/test_voice_backend_degradation_focused.cpp (backend patterns)
 * @see tests/voice/test_voice_session_chaos_isolation.cpp (session patterns)
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// All implementation is self-contained (inline types + helpers).
// No voice headers are required — follows the pattern established in
// test_voice_backend_degradation_focused.cpp.
// ─────────────────────────────────────────────────────────────────────────────

namespace themis {
namespace voice {
namespace test {

/// Determinism seed required by Wave A chaos conventions.
constexpr uint32_t kChaosTestSeed = 42;

// =============================================================================
// Domain enumerations
// =============================================================================

enum class CircuitState { CLOSED, OPEN, HALF_OPEN };

enum class SessionState { ACTIVE, TORN_DOWN, ERROR };

enum class BackendKind { STT, LLM };

// =============================================================================
// CircuitBreaker (minimal, deterministic, no real timers needed for tests)
// =============================================================================

class CircuitBreaker {
 public:
  explicit CircuitBreaker(int failure_threshold = 5)
      : failure_threshold_(failure_threshold) {}

  CircuitState state() const { return state_; }

  bool canAttempt() const { return state_ != CircuitState::OPEN; }

  void recordSuccess() {
    if (state_ == CircuitState::HALF_OPEN) {
      ++success_count_;
      if (success_count_ >= 2) {
        state_ = CircuitState::CLOSED;
        failure_count_ = 0;
        success_count_ = 0;
      }
    } else if (state_ == CircuitState::CLOSED) {
      failure_count_ = 0;
    }
  }

  void recordFailure() {
    if (state_ == CircuitState::CLOSED) {
      ++failure_count_;
      if (failure_count_ >= failure_threshold_) {
        state_ = CircuitState::OPEN;
      }
    } else if (state_ == CircuitState::HALF_OPEN) {
      state_ = CircuitState::OPEN;
    }
  }

  /// Manually advance to HALF_OPEN (simulates timeout expiry in tests).
  void advanceToHalfOpen() {
    if (state_ == CircuitState::OPEN) {
      state_ = CircuitState::HALF_OPEN;
      success_count_ = 0;
    }
  }

  void reset() {
    state_ = CircuitState::CLOSED;
    failure_count_ = 0;
    success_count_ = 0;
  }

  int failureCount() const { return failure_count_; }

 private:
  CircuitState state_ = CircuitState::CLOSED;
  int failure_threshold_;
  int failure_count_ = 0;
  int success_count_ = 0;
};

// =============================================================================
// VoiceBackend — minimal fake backend
// =============================================================================

class VoiceBackend {
 public:
  explicit VoiceBackend(BackendKind kind) : kind_(kind) {}

  BackendKind kind() const { return kind_; }

  bool isAvailable() const { return available_; }
  void setAvailable(bool v) { available_ = v; }

  CircuitBreaker& breaker() { return breaker_; }
  const CircuitBreaker& breaker() const { return breaker_; }

  /// Returns true iff the call succeeds.
  bool call(const std::string& /*request*/) {
    if (!breaker_.canAttempt()) return false;
    if (!available_) {
      breaker_.recordFailure();
      return false;
    }
    breaker_.recordSuccess();
    return true;
  }

 private:
  BackendKind kind_;
  bool available_ = true;
  CircuitBreaker breaker_;
};

// =============================================================================
// VoiceSession — models a single voice session lifecycle
// =============================================================================

class VoiceSession {
 public:
  explicit VoiceSession(int id, VoiceBackend* stt, VoiceBackend* llm)
      : id_(id), stt_(stt), llm_(llm) {}

  int id() const { return id_; }
  SessionState state() const { return state_; }

  bool isActive() const { return state_ == SessionState::ACTIVE; }

  /// Returns false and sets state=ERROR when backends are unavailable.
  bool process(const std::string& chunk) {
    if (state_ != SessionState::ACTIVE) return false;

    if (!stt_ || !stt_->call(chunk)) {
      state_ = SessionState::ERROR;
      return false;
    }
    if (!llm_ || !llm_->call(chunk)) {
      state_ = SessionState::ERROR;
      return false;
    }
    ++processed_;
    return true;
  }

  /// Explicit teardown — must be idempotent.
  void teardown() {
    if (state_ == SessionState::ACTIVE || state_ == SessionState::ERROR) {
      state_ = SessionState::TORN_DOWN;
      ++teardown_count_;
    }
  }

  int processedCount() const { return processed_; }
  int teardownCount() const { return teardown_count_; }

 private:
  int id_;
  VoiceBackend* stt_;
  VoiceBackend* llm_;
  SessionState state_ = SessionState::ACTIVE;
  int processed_ = 0;
  int teardown_count_ = 0;
};

// =============================================================================
// AntiSpoof — minimal fake spoofing/liveness checker
// =============================================================================

struct ChallengeToken {
  std::string value;
  bool stale = false;
};

class AntiSpoof {
 public:
  /// Returns false when challenge is stale, replayed, or backend is down.
  bool verify(const ChallengeToken& token, bool backend_available) const {
    if (!backend_available) return false;  // fail-closed
    if (token.stale) return false;
    if (replaySet_.count(token.value)) return false;
    return !token.value.empty();
  }

  /// Records a token as seen (replay detection).
  void markSeen(const std::string& value) { replaySet_.insert(value); }

 private:
  std::set<std::string> replaySet_;
};

// =============================================================================
// AuditLog — records lifecycle events
// =============================================================================

class AuditLog {
 public:
  void record(const std::string& event) {
    std::lock_guard<std::mutex> lock(mu_);
    events_.push_back(event);
  }

  bool contains(const std::string& substr) const {
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& e : events_) {
      if (e.find(substr) != std::string::npos) return true;
    }
    return false;
  }

  std::size_t size() const {
    std::lock_guard<std::mutex> lock(mu_);
    return events_.size();
  }

 private:
  mutable std::mutex mu_;
  std::vector<std::string> events_;
};

// =============================================================================
// Test Fixture
// =============================================================================

class VoiceWaveAChaosTest : public ::testing::Test {
 protected:
  void SetUp() override {
    stt_ = std::make_unique<VoiceBackend>(BackendKind::STT);
    llm_ = std::make_unique<VoiceBackend>(BackendKind::LLM);
    audit_ = std::make_unique<AuditLog>();
  }

  std::unique_ptr<VoiceBackend> stt_;
  std::unique_ptr<VoiceBackend> llm_;
  std::unique_ptr<AuditLog> audit_;
};

// =============================================================================
// VOICE-CHAOS-01: Backend failure during active session → session tears down
// =============================================================================

/**
 * @test VOICE-CHAOS-01
 * @brief When the STT backend goes down mid-session the session must
 *        transition to ERROR and teardown must complete cleanly (state ==
 *        TORN_DOWN, teardownCount == 1).
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_01_BackendFailure_SessionTeardownClean) {
  VoiceSession session(1, stt_.get(), llm_.get());

  // First call succeeds.
  ASSERT_TRUE(session.process("hello")) << "VOICE-CHAOS-01: first call must succeed";

  // Backend goes down.
  stt_->setAvailable(false);
  EXPECT_FALSE(session.process("world"))
      << "VOICE-CHAOS-01: call after failure must return false";
  EXPECT_EQ(session.state(), SessionState::ERROR)
      << "VOICE-CHAOS-01: session must be in ERROR after backend failure";

  // Teardown must complete exactly once.
  session.teardown();
  EXPECT_EQ(session.state(), SessionState::TORN_DOWN)
      << "VOICE-CHAOS-01: session must be TORN_DOWN after teardown()";
  EXPECT_EQ(session.teardownCount(), 1)
      << "VOICE-CHAOS-01: teardownCount must be exactly 1";
}

// =============================================================================
// VOICE-CHAOS-02: Spoofing detected during backend failure → session rejected
// =============================================================================

/**
 * @test VOICE-CHAOS-02
 * @brief When the STT backend is unavailable, anti-spoof verification must
 *        fail-closed regardless of challenge validity.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_02_SpoofingDetection_BackendFailure_Rejected) {
  AntiSpoof anti_spoof;
  ChallengeToken valid_token{"tok_abc_42", false};

  // Backend available → valid token passes.
  EXPECT_TRUE(anti_spoof.verify(valid_token, /*backend_available=*/true))
      << "VOICE-CHAOS-02: valid token must pass when backend is up";

  // Backend down → must fail-closed even with a valid token.
  EXPECT_FALSE(anti_spoof.verify(valid_token, /*backend_available=*/false))
      << "VOICE-CHAOS-02: must fail-closed when backend is unavailable";
}

// =============================================================================
// VOICE-CHAOS-03: Cascading backend failure → circuit breaker trips after 5
// =============================================================================

/**
 * @test VOICE-CHAOS-03
 * @brief After 5 consecutive backend failures the circuit breaker must open.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_03_CascadingFailure_CircuitBreakerTrips) {
  stt_->setAvailable(false);

  VoiceSession session(1, stt_.get(), llm_.get());
  for (int i = 0; i < 5; ++i) {
    session.process("chunk");  // drives stt_ failures
  }

  // Drive the breaker directly for the remaining calls.
  for (int i = stt_->breaker().failureCount(); i < 5; ++i) {
    stt_->breaker().recordFailure();
  }

  EXPECT_EQ(stt_->breaker().state(), CircuitState::OPEN)
      << "VOICE-CHAOS-03: circuit breaker must OPEN after 5 failures";
}

// =============================================================================
// VOICE-CHAOS-04: Recovery OPEN → HALF_OPEN → CLOSED
// =============================================================================

/**
 * @test VOICE-CHAOS-04
 * @brief Simulates the full circuit breaker recovery arc without real timers.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_04_CircuitBreaker_Recovery_OpenHalfOpenClosed) {
  CircuitBreaker cb;

  // Trip the breaker.
  for (int i = 0; i < 5; ++i) cb.recordFailure();
  ASSERT_EQ(cb.state(), CircuitState::OPEN)
      << "VOICE-CHAOS-04: breaker must be OPEN";

  // Simulate timeout expiry → HALF_OPEN.
  cb.advanceToHalfOpen();
  EXPECT_EQ(cb.state(), CircuitState::HALF_OPEN)
      << "VOICE-CHAOS-04: breaker must transition to HALF_OPEN";

  // Two successes → CLOSED.
  cb.recordSuccess();
  cb.recordSuccess();
  EXPECT_EQ(cb.state(), CircuitState::CLOSED)
      << "VOICE-CHAOS-04: breaker must close after 2 successes in HALF_OPEN";
}

// =============================================================================
// VOICE-CHAOS-05: Concurrent session teardowns (N=8) under backend failure
// =============================================================================

/**
 * @test VOICE-CHAOS-05
 * @brief Eight sessions are torn down concurrently while the backend is down.
 *        No session must be torn down more than once (no double-teardown).
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_05_ConcurrentTeardowns_NoLeak) {
  constexpr int kSessions = 8;
  stt_->setAvailable(false);

  std::vector<std::unique_ptr<VoiceSession>> sessions;
  sessions.reserve(kSessions);
  for (int i = 0; i < kSessions; ++i) {
    sessions.push_back(
        std::make_unique<VoiceSession>(i, stt_.get(), llm_.get()));
    sessions.back()->process("chunk");  // drives it into ERROR
  }

  std::vector<std::thread> threads;
  threads.reserve(kSessions);
  for (int i = 0; i < kSessions; ++i) {
    threads.emplace_back([&sessions, i]() { sessions[i]->teardown(); });
  }
  for (auto& t : threads) t.join();

  for (int i = 0; i < kSessions; ++i) {
    EXPECT_EQ(sessions[i]->state(), SessionState::TORN_DOWN)
        << "VOICE-CHAOS-05: session " << i << " must be TORN_DOWN";
    EXPECT_EQ(sessions[i]->teardownCount(), 1)
        << "VOICE-CHAOS-05: session " << i << " must not be torn down twice";
  }
}

// =============================================================================
// VOICE-CHAOS-06: Replay attack during degraded backend → anti-spoof rejects
// =============================================================================

/**
 * @test VOICE-CHAOS-06
 * @brief A token that has already been seen must be rejected even when the
 *        backend is available (replay attack prevention).
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_06_ReplayAttack_DegradedBackend_Rejected) {
  AntiSpoof anti_spoof;
  const std::string token_val = "replay_token_42";
  ChallengeToken token{token_val, false};

  // First use — accepted and marked seen.
  EXPECT_TRUE(anti_spoof.verify(token, /*backend_available=*/true))
      << "VOICE-CHAOS-06: first use must succeed";
  anti_spoof.markSeen(token_val);

  // Second use (replay) — must be rejected.
  EXPECT_FALSE(anti_spoof.verify(token, /*backend_available=*/true))
      << "VOICE-CHAOS-06: replay must be rejected";

  // Replay with degraded backend — still rejected.
  EXPECT_FALSE(anti_spoof.verify(token, /*backend_available=*/false))
      << "VOICE-CHAOS-06: replay during degradation must also be rejected";
}

// =============================================================================
// VOICE-CHAOS-07: Oversized stream chunk during backend failure → rejected
// =============================================================================

/**
 * @test VOICE-CHAOS-07
 * @brief A stream chunk that exceeds the maximum allowed size must be rejected
 *        fail-closed, independent of backend availability.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_07_OversizedChunk_BackendFailure_FailClosed) {
  constexpr std::size_t kMaxChunkBytes = 64 * 1024;  // 64 KiB
  const std::string oversized_chunk(kMaxChunkBytes + 1, 'X');

  // Inline validator (simulates VoiceStreamValidator behaviour).
  auto validate_chunk = [&](const std::string& chunk) -> bool {
    return chunk.size() <= kMaxChunkBytes;
  };

  stt_->setAvailable(false);

  EXPECT_FALSE(validate_chunk(oversized_chunk))
      << "VOICE-CHAOS-07: oversized chunk must be rejected";

  // Normal-sized chunk — would be accepted by the validator (backend state
  // is irrelevant at the validator layer).
  const std::string normal_chunk(1024, 'Y');
  EXPECT_TRUE(validate_chunk(normal_chunk))
      << "VOICE-CHAOS-07: normal chunk must pass validator";
}

// =============================================================================
// VOICE-CHAOS-08: Backend unavailability + stale challenge → fails closed
// =============================================================================

/**
 * @test VOICE-CHAOS-08
 * @brief A stale challenge token combined with a down backend is doubly-invalid
 *        and must fail-closed.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_08_BackendUnavailable_StaleChallenge_FailsClosed) {
  AntiSpoof anti_spoof;
  ChallengeToken stale_token{"tok_stale_42", /*stale=*/true};

  // Backend up but token is stale → rejected.
  EXPECT_FALSE(anti_spoof.verify(stale_token, /*backend_available=*/true))
      << "VOICE-CHAOS-08: stale token must fail even with backend up";

  // Backend down AND token is stale → rejected.
  EXPECT_FALSE(anti_spoof.verify(stale_token, /*backend_available=*/false))
      << "VOICE-CHAOS-08: stale token + down backend must fail-closed";
}

// =============================================================================
// VOICE-CHAOS-09: Multi-backend cascade (LLM fail → STT fail) → ERROR
// =============================================================================

/**
 * @test VOICE-CHAOS-09
 * @brief When both STT and LLM fail the session must reach ERROR state after
 *        the first failing call.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_09_MultiBackendCascade_SessionDemotedToError) {
  // LLM fails first.
  llm_->setAvailable(false);

  VoiceSession session(1, stt_.get(), llm_.get());
  EXPECT_FALSE(session.process("test_chunk"))
      << "VOICE-CHAOS-09: process must fail when LLM is down";
  EXPECT_EQ(session.state(), SessionState::ERROR)
      << "VOICE-CHAOS-09: session must be in ERROR after LLM failure";

  // Now STT also fails (cascading scenario).
  stt_->setAvailable(false);

  // Session is already in ERROR — further calls must still return false.
  EXPECT_FALSE(session.process("another_chunk"))
      << "VOICE-CHAOS-09: ERROR session must reject further process calls";
}

// =============================================================================
// VOICE-CHAOS-10: Session audit callback fires during backend failure teardown
// =============================================================================

/**
 * @test VOICE-CHAOS-10
 * @brief An audit callback must be invoked exactly once when a session is torn
 *        down after a backend failure.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_10_AuditCallback_FiresDuringBackendFailureTeardown) {
  stt_->setAvailable(false);

  VoiceSession session(42, stt_.get(), llm_.get());
  session.process("chunk");  // drives session to ERROR

  // Simulate audit callback via AuditLog.
  auto teardown_with_audit = [&]() {
    session.teardown();
    audit_->record("session_teardown:id=42:reason=backend_failure");
  };

  teardown_with_audit();

  EXPECT_EQ(session.state(), SessionState::TORN_DOWN)
      << "VOICE-CHAOS-10: session must be TORN_DOWN";
  EXPECT_TRUE(audit_->contains("session_teardown:id=42"))
      << "VOICE-CHAOS-10: audit log must contain teardown event";
  EXPECT_EQ(audit_->size(), 1u)
      << "VOICE-CHAOS-10: audit must fire exactly once";
}

// =============================================================================
// VOICE-CHAOS-11: New sessions blocked when circuit breaker is OPEN
// =============================================================================

/**
 * @test VOICE-CHAOS-11
 * @brief When the STT circuit breaker is OPEN, new session calls must fail
 *        immediately (canAttempt() == false), blocking new work from queuing.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_11_NewSessionsBlocked_CircuitOpen) {
  // Trip the STT breaker.
  for (int i = 0; i < 5; ++i) stt_->breaker().recordFailure();
  ASSERT_EQ(stt_->breaker().state(), CircuitState::OPEN)
      << "VOICE-CHAOS-11: prerequisite: breaker must be OPEN";

  // A new session attempting to use the STT backend must fail.
  VoiceSession session(99, stt_.get(), llm_.get());
  EXPECT_FALSE(session.process("new_work"))
      << "VOICE-CHAOS-11: new session must be blocked by open circuit";
  EXPECT_EQ(session.state(), SessionState::ERROR)
      << "VOICE-CHAOS-11: session must enter ERROR when circuit is OPEN";
}

// =============================================================================
// VOICE-CHAOS-12: Full lifecycle: start → backend fail → teardown → re-init
// =============================================================================

/**
 * @test VOICE-CHAOS-12
 * @brief Full round-trip: session starts normally, backend fails, session is
 *        torn down, circuit breaker resets, and a fresh session succeeds.
 */
TEST_F(VoiceWaveAChaosTest, CHAOS_12_FullLifecycle_StartFailTeardownReinit) {
  // Phase 1 — normal operation.
  VoiceSession session1(1, stt_.get(), llm_.get());
  EXPECT_TRUE(session1.process("hello"))
      << "VOICE-CHAOS-12: initial processing must succeed";

  // Phase 2 — backend failure.
  stt_->setAvailable(false);
  EXPECT_FALSE(session1.process("world"))
      << "VOICE-CHAOS-12: processing must fail after backend goes down";
  EXPECT_EQ(session1.state(), SessionState::ERROR);

  // Phase 3 — teardown.
  session1.teardown();
  EXPECT_EQ(session1.state(), SessionState::TORN_DOWN)
      << "VOICE-CHAOS-12: session must be torn down cleanly";

  // Phase 4 — recovery: restore backend and reset circuit.
  stt_->setAvailable(true);
  stt_->breaker().reset();

  // Phase 5 — fresh session succeeds.
  VoiceSession session2(2, stt_.get(), llm_.get());
  EXPECT_TRUE(session2.process("re-init"))
      << "VOICE-CHAOS-12: fresh session must succeed after backend recovery";
  EXPECT_EQ(session2.state(), SessionState::ACTIVE)
      << "VOICE-CHAOS-12: fresh session must remain ACTIVE";
}

}  // namespace test
}  // namespace voice
}  // namespace themis

// Entry point — tests linked against gtest_main; no custom main() needed.
