#include <gtest/gtest.h>

#include "tests/utils/fault_injector.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

namespace themis::test {

namespace {

struct VoiceFrame {
    std::vector<uint8_t> payload;
    std::string nonce;
    bool malformed = false;
};

class MockVoiceSessionManager {
public:
    int openSession() {
        std::lock_guard<std::mutex> lk(mutex_);
        const int id = next_session_id_++;
        sessions_.insert(id);
        return id;
    }

    void attachTimeoutInjector(TimeoutInjector* injector) {
        std::lock_guard<std::mutex> lk(mutex_);
        timeout_injector_ = injector;
    }

    bool processFrame(int session_id, const VoiceFrame& frame) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!sessions_.count(session_id)) {
            return false;
        }
        if (timeout_injector_ && timeout_injector_->shouldTimeout()) {
            closed_sessions_.insert(session_id);
            sessions_.erase(session_id);
            return false;
        }
        if (frame.malformed || frame.payload.size() > kMaxPayloadBytes ||
            !seen_nonces_.insert(frame.nonce).second) {
            closed_sessions_.insert(session_id);
            sessions_.erase(session_id);
            return false;
        }
        accepted_frames_++;
        return true;
    }

    void teardown(int session_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        sessions_.erase(session_id);
        closed_sessions_.insert(session_id);
    }

    [[nodiscard]] bool isClosed(int session_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        return closed_sessions_.count(session_id) > 0;
    }

    [[nodiscard]] size_t activeSessions() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return sessions_.size();
    }

    [[nodiscard]] size_t acceptedFrames() const {
        return accepted_frames_.load();
    }

private:
    static constexpr size_t kMaxPayloadBytes = 64 * 1024;

    int next_session_id_ = 1;
    TimeoutInjector* timeout_injector_ = nullptr;
    std::unordered_set<int> sessions_;
    std::unordered_set<int> closed_sessions_;
    std::unordered_set<std::string> seen_nonces_;
    std::atomic<size_t> accepted_frames_{0};
    mutable std::mutex mutex_;
};

VoiceFrame makeFrame(size_t bytes, std::string nonce, bool malformed = false) {
    VoiceFrame frame;
    frame.payload.assign(bytes, 0x5A);
    frame.nonce = std::move(nonce);
    frame.malformed = malformed;
    return frame;
}

}  // namespace

class VoiceAdversarialTest : public ::testing::Test {
protected:
    MockVoiceSessionManager manager_;
};

TEST_F(VoiceAdversarialTest, OversizedFrameFailsClosed) {
    const int session_id = manager_.openSession();

    EXPECT_FALSE(manager_.processFrame(session_id, makeFrame(80 * 1024, "oversized-1")));
    EXPECT_TRUE(manager_.isClosed(session_id));
    EXPECT_EQ(manager_.activeSessions(), 0u);
}

TEST_F(VoiceAdversarialTest, MalformedFrameFailsClosed) {
    const int session_id = manager_.openSession();

    EXPECT_FALSE(manager_.processFrame(session_id, makeFrame(1024, "bad-frame-1", true)));
    EXPECT_TRUE(manager_.isClosed(session_id));
    EXPECT_EQ(manager_.acceptedFrames(), 0u);
}

TEST_F(VoiceAdversarialTest, ReplayNonceIsRejectedDeterministically) {
    const int session_a = manager_.openSession();
    const int session_b = manager_.openSession();

    EXPECT_TRUE(manager_.processFrame(session_a, makeFrame(1024, "nonce-42")));
    EXPECT_FALSE(manager_.processFrame(session_b, makeFrame(1024, "nonce-42")));
    EXPECT_TRUE(manager_.isClosed(session_b));
    EXPECT_EQ(manager_.acceptedFrames(), 1u);
}

TEST_F(VoiceAdversarialTest, TimeoutInjectionClosesSession) {
    TimeoutInjector::TimeoutConfig cfg;
    cfg.target_component = "voice-auth";
    cfg.duration = 250ms;
    cfg.trigger_immediately = true;
    cfg.auto_recover = false;

    TimeoutInjector injector(cfg);
    manager_.attachTimeoutInjector(&injector);
    ASSERT_TRUE(injector.inject().success);

    const int session_id = manager_.openSession();
    EXPECT_FALSE(manager_.processFrame(session_id, makeFrame(2048, "timeout-1")));
    EXPECT_TRUE(manager_.isClosed(session_id));

    EXPECT_TRUE(injector.recover().success);
}

TEST_F(VoiceAdversarialTest, ConcurrentTeardownIsIdempotent) {
    std::vector<int> sessions = {};

    for (int i = 0; i < 16; ++i) {
        sessions.push_back(manager_.openSession());
    }

    std::vector<std::thread> threads = {};

    for (const int session_id : sessions) {
        threads.emplace_back([this, session_id]() {
            manager_.teardown(session_id);
            manager_.teardown(session_id);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(manager_.activeSessions(), 0u);
    for (const int session_id : sessions) {
        EXPECT_TRUE(manager_.isClosed(session_id));
    }
}

}  // namespace themis::test
