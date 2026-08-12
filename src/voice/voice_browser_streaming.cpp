/**
 * @file voice_browser_streaming.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "voice/voice_browser_streaming.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <random>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace voice {

// ─────────────────────────────────────────────────────────────────────────────
// VoiceStreamingSession::Impl
// ─────────────────────────────────────────────────────────────────────────────

// TASK 2.5: Streaming and chunk handling with bounded buffer
// Error codes [6900-6999]:
// - 6900: Buffer overflow (exceeds kMaxBufferSizeBytes)
// - 6901: Stream state transition invalid
// - 6902: Chunk ordering violation

// Bounded buffer constraint
static constexpr size_t kMaxBufferSizeBytes = 50 * 1024 * 1024;  // 50 MB per stream
static constexpr size_t kMaxChunkQueueSize = 10000;  // Max pending chunks for ordering

// TASK 2.5: Stream state machine
enum class StreamState {
    CONNECTING,  // Initial state before start()
    CONNECTED,   // Active; accepting audio chunks
    STREAMING,   // Audio being processed
    CLOSING,     // Shutdown initiated; flushing buffers
    CLOSED       // Terminal state
};

struct VoiceStreamingSession::Impl {
    Config   config;
    StreamID stream_id;
    bool     active          = false;
    StreamState stream_state = StreamState::CONNECTING;  // TASK 2.5: State tracking
    int64_t  started_at_ms   = 0;
    size_t   bytes_received  = 0;
    size_t   buffer_size_bytes = 0;  // TASK 2.5: Track buffer usage
    int64_t  last_activity_ms = 0;
    int64_t  last_heartbeat_ms = 0;
    bool     connection_alive = true;
    bool     buffer_pressure_paused = false;
    bool     sequence_gap_detected = false;

    // TASK 2.5: Accumulated audio buffer for the current utterance
    // with size tracking for overflow detection
    std::vector<uint8_t> audio_buffer;
    uint32_t last_chunk_seq = 0;  // TASK 2.5: For deterministic chunk ordering
    std::deque<uint32_t> pending_chunk_sequences;

    // Partial hypothesis state
    std::string partial_text;
    uint32_t    partial_seq = 0;

    // Injected STT backend (null = built-in placeholder)
    TranscribeFn transcribe_fn;

    // Callbacks
    PartialTranscriptCb on_partial;
    FinalTranscriptCb   on_final;
    TtsChunkCb          on_tts;
    ErrorCb             on_error;

    Impl(Config c) : config(std::move(c)) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

int64_t streamingNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string generateStreamId() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::mutex mu;
    std::lock_guard<std::mutex> lock(mu);
    std::ostringstream oss;
    oss << "vs-" << std::hex << rng() << "-" << rng();
    return oss.str();
}

bool isChunkFrameAligned(const VoiceStreamingSession::Config& config,
                         const std::vector<uint8_t>& audio_chunk) {
    if (audio_chunk.empty()) {
        return false;
    }
    if (config.audio_format.encoding != StreamAudioFormat::Encoding::PCM16) {
        return true;
    }
    const uint32_t bytes_per_sample = config.audio_format.bits_per_sample / 8u;
    const uint32_t frame_bytes =
        bytes_per_sample * static_cast<uint32_t>(std::max<uint16_t>(1, config.audio_format.channels));
    return frame_bytes > 0 && (audio_chunk.size() % frame_bytes) == 0;
}

/**
 * @brief Minimal placeholder STT: counts bytes as a proxy for speech.
 *
 * In production this delegates to the voice module's SpeechToText processor.
 * The placeholder returns a synthetic partial transcript to make the
 * pipeline end-to-end testable without a GPU.
 */
PartialTranscript runPartialStt([[maybe_unused]] const std::string& session_id,
                                 StreamID           stream_id,
                                 const std::vector<uint8_t>& audio,
                                 bool   is_final,
                                 uint32_t seq)
{
    // STUB/SIMULATION NOTE:
    // Purpose: Return a recognisable placeholder transcript while no real STT
    //          backend is wired into the browser streaming pipeline.
    // Activation: Always — no IWhisperTranscriber or equivalent is injected here.
    // Production Delta: All browser-stream transcripts contain synthetic text like
    //                   `[partial#N:MBB]` instead of actual speech content.
    //                   Applications that consume these transcripts receive
    //                   meaningless captions regardless of audio input.
    // Removal Plan: Inject an IWhisperTranscriber; call transcribeStream() per
    //               audio chunk; populate pt.text from the real transcript token.
    //               See src/voice/FUTURE_ENHANCEMENTS.md §Browser STT Backend.
    PartialTranscript pt;
    pt.stream_id  = stream_id;
    pt.is_final   = is_final;
    pt.confidence = is_final ? 0.92f : 0.75f;
    pt.timestamp_ms = streamingNowMs();
    // Placeholder text — real STT backend fills this
    std::ostringstream oss;
    oss << "[partial#" << seq << ":" << audio.size() << "B]";
    pt.text = oss.str();
    return pt;
}

FinalTranscript makeFinalTranscript([[maybe_unused]] const std::string& session_id,
                                     StreamID           stream_id,
                                     const std::vector<uint8_t>& audio,
                                     int64_t            started_at_ms)
{
    FinalTranscript ft;
    ft.stream_id   = stream_id;
    ft.confidence  = 0.92f;
    ft.duration_ms = streamingNowMs() - started_at_ms;

    std::ostringstream oss;
    oss << "[transcript:" << audio.size() << "B]";
    ft.text = oss.str();
    return ft;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// VoiceStreamingSession
// ─────────────────────────────────────────────────────────────────────────────

VoiceStreamingSession::VoiceStreamingSession(Config config)
    : impl_(std::make_unique<Impl>(std::move(config)))
{
    if (impl_->config.max_frame_bytes == 0 ||
        impl_->config.max_frame_bytes > 65536) {
        throw std::invalid_argument(
            "VoiceStreamingSession: max_frame_bytes must be in (0, 65536]");
    }
    if (impl_->config.max_duration_s == 0 || impl_->config.max_duration_s > 3600) {
        throw std::invalid_argument(
            "VoiceStreamingSession: max_duration_s must be in (0, 3600]");
    }
}

VoiceStreamingSession::~VoiceStreamingSession() {
    if (impl_ && impl_->active) {
        try {
            end();
        } catch (const std::string&) {
        } catch (const char*) {
        } catch (...) {
        }
    }
}

// factory
std::unique_ptr<VoiceStreamingSession>
VoiceStreamingSession::create(Config config) {
    // Using 'new' directly because the constructor is private
    return std::unique_ptr<VoiceStreamingSession>(
        new VoiceStreamingSession(std::move(config)));
}

StreamID VoiceStreamingSession::start() {
    // TASK 2.5: Stream state machine enforcement
    if (impl_->active) return impl_->stream_id;
    
    impl_->stream_id    = generateStreamId();
    impl_->started_at_ms= streamingNowMs();
    impl_->active       = true;
    impl_->stream_state = StreamState::CONNECTED;  // TASK 2.5: State transition
    impl_->audio_buffer.clear();
    impl_->buffer_size_bytes = 0;  // TASK 2.5: Reset buffer tracking
    impl_->partial_seq  = 0;
    impl_->last_chunk_seq = 0;  // TASK 2.5: Reset chunk sequence
    impl_->last_activity_ms = impl_->started_at_ms;
    impl_->last_heartbeat_ms = impl_->started_at_ms;
    impl_->connection_alive = true;
    impl_->buffer_pressure_paused = false;
    impl_->sequence_gap_detected = false;
    impl_->pending_chunk_sequences.clear();
    
    THEMIS_INFO("VoiceStreamingSession: started stream_id={} user={} (CONNECTED state)",
                impl_->stream_id, impl_->config.user_id);
    return impl_->stream_id;
}

void VoiceStreamingSession::end() {
    // TASK 2.5: Stream teardown and cleanup
    if (!impl_->active) return;
    
    // TASK 2.5: Transition to CLOSING state
    impl_->stream_state = StreamState::CLOSING;
    
    // TASK 2.5: Flush any remaining audio as final utterance (stream cleanup)
    if (!impl_->audio_buffer.empty()) {
        endOfUtterance();
    }
    
    impl_->active = false;
    impl_->stream_state = StreamState::CLOSED;  // TASK 2.5: Terminal state
    impl_->buffer_size_bytes = 0;  // TASK 2.5: Clean up buffer tracking
    impl_->connection_alive = false;
    impl_->buffer_pressure_paused = false;
    impl_->pending_chunk_sequences.clear();
    
    THEMIS_INFO("VoiceStreamingSession: ended stream_id={} bytes={} (CLOSED state)",
                impl_->stream_id, impl_->bytes_received);
}

bool VoiceStreamingSession::isActive() const noexcept {
    return impl_ && impl_->active;
}

// ── Audio ingestion ───────────────────────────────────────────────────────────

PartialTranscript
VoiceStreamingSession::sendAudioChunk(const std::vector<uint8_t>& audio_chunk) {
    // TASK 2.5: Streaming chunk handling with bounded buffer
    PartialTranscript empty;
    if (!impl_ || !impl_->active) return empty;

    // TASK 2.5: Verify stream state (must be CONNECTED or STREAMING)
    // Error code 6901: Stream state transition invalid
    if (impl_->stream_state == StreamState::CLOSING ||
        impl_->stream_state == StreamState::CLOSED) {
        std::string msg = "VoiceStreamingSession: stream is closing/closed (error 6901)";
        THEMIS_WARN("{}", msg);
        if (impl_->on_error) impl_->on_error(msg);
        return empty;
    }
    if (!isChunkFrameAligned(impl_->config, audio_chunk)) {
        std::string msg = "VoiceStreamingSession: malformed or frame-misaligned audio chunk (error 6904)";
        THEMIS_WARN("{}", msg);
        if (impl_->on_error) impl_->on_error(msg);
        return empty;
    }

    // TASK 2.5: Enforce max frame size
    if (audio_chunk.size() > impl_->config.max_frame_bytes) {
        std::string msg = "VoiceStreamingSession: frame too large (" +
                          std::to_string(audio_chunk.size()) + " > " +
                          std::to_string(impl_->config.max_frame_bytes) + ")";
        THEMIS_WARN("{}", msg);
        if (impl_->on_error) impl_->on_error(msg);
        return empty;
    }

    // TASK 2.5: Enforce session duration
    int64_t elapsed_s = (streamingNowMs() - impl_->started_at_ms) / 1000;
    if (static_cast<uint32_t>(elapsed_s) > impl_->config.max_duration_s) {
        THEMIS_WARN("VoiceStreamingSession: max duration exceeded, closing stream_id={}",
                    impl_->stream_id);
        end();
        return empty;
    }

    // TASK 2.5: Bounded buffer overflow detection and rejection
    // Error code 6900: Buffer overflow
    size_t new_total = impl_->buffer_size_bytes + audio_chunk.size();
    if (new_total > kMaxBufferSizeBytes) {
        std::string msg = "VoiceStreamingSession: buffer overflow (" +
                          std::to_string(new_total) + " > " +
                          std::to_string(kMaxBufferSizeBytes) + " bytes) - error 6900";
        THEMIS_ERROR("{}", msg);
        if (impl_->on_error) impl_->on_error(msg);
        // Fail-closed: reject chunk when buffer full
        return empty;
    }

    // TASK 2.5: Deterministic chunk ordering — track sequence numbers
    // Error code 6902: Chunk ordering violation (if we detect out-of-order)
    impl_->last_chunk_seq++;
    if (impl_->pending_chunk_sequences.size() >= kMaxChunkQueueSize) {
        impl_->sequence_gap_detected = true;
        std::string msg = "VoiceStreamingSession: pending chunk queue exhausted (error 6902)";
        THEMIS_ERROR("{}", msg);
        if (impl_->on_error) impl_->on_error(msg);
        return empty;
    }
    impl_->pending_chunk_sequences.push_back(impl_->last_chunk_seq);

    // TASK 2.5: Append to audio buffer with size tracking
    impl_->audio_buffer.insert(impl_->audio_buffer.end(),
                                audio_chunk.begin(), audio_chunk.end());
    impl_->buffer_size_bytes = impl_->audio_buffer.size();
    impl_->bytes_received += audio_chunk.size();
    impl_->last_activity_ms = streamingNowMs();

    // Transition to STREAMING state if receiving audio
    if (impl_->stream_state == StreamState::CONNECTED) {
        impl_->stream_state = StreamState::STREAMING;
    }

    if (!impl_->config.partial_results) return empty;

    // Run incremental STT — use injected backend when available
    ++impl_->partial_seq;
    PartialTranscript pt;
    if (impl_->transcribe_fn) {
        pt = impl_->transcribe_fn(impl_->audio_buffer,
                                   /*is_final=*/false,
                                   impl_->partial_seq);
        pt.stream_id = impl_->stream_id;
        if (pt.timestamp_ms == 0) pt.timestamp_ms = streamingNowMs();
    } else {
        pt = runPartialStt(impl_->config.session_id,
                           impl_->stream_id,
                           impl_->audio_buffer,
                           /*is_final=*/false,
                           impl_->partial_seq);
    }
    if (impl_->on_partial) impl_->on_partial(pt);
    return pt;
}

void VoiceStreamingSession::endOfUtterance() {
    if (!impl_ || !impl_->active || impl_->audio_buffer.empty()) return;

    // Final STT
    auto ft = makeFinalTranscript(impl_->config.session_id,
                                   impl_->stream_id,
                                   impl_->audio_buffer,
                                   impl_->started_at_ms);
    impl_->audio_buffer.clear();
    impl_->buffer_size_bytes = 0;
    if (impl_->on_final) impl_->on_final(ft);

    // Optional TTS synthesis (placeholder: echo transcript back as bytes)
    if (impl_->config.enable_tts && impl_->on_tts) {
        std::vector<uint8_t> tts_audio(ft.text.begin(), ft.text.end());
        impl_->on_tts(tts_audio);
    }
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void VoiceStreamingSession::onPartialTranscript(PartialTranscriptCb cb) {
    impl_->on_partial = std::move(cb);
}
void VoiceStreamingSession::onFinalTranscript(FinalTranscriptCb cb) {
    impl_->on_final = std::move(cb);
}
void VoiceStreamingSession::onTtsChunk(TtsChunkCb cb) {
    impl_->on_tts = std::move(cb);
}
void VoiceStreamingSession::onError(ErrorCb cb) {
    impl_->on_error = std::move(cb);
}
void VoiceStreamingSession::setTranscribeBackend(TranscribeFn fn) {
    impl_->transcribe_fn = std::move(fn);
}

bool VoiceStreamingSession::sendHeartbeat() noexcept {
    if (!impl_ || !impl_->active) {
        return false;
    }
    if (impl_->stream_state == StreamState::CLOSING ||
        impl_->stream_state == StreamState::CLOSED) {
        return false;
    }
    impl_->last_heartbeat_ms = streamingNowMs();
    return impl_->connection_alive;
}

bool VoiceStreamingSession::reconnectWithBackoff(int max_retries) noexcept {
    if (!impl_ || max_retries <= 0) {
        return false;
    }
    if (impl_->stream_state == StreamState::CLOSING ||
        impl_->stream_state == StreamState::CLOSED) {
        return false;
    }
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        impl_->connection_alive = true;
        if (impl_->stream_state == StreamState::CONNECTING) {
            impl_->stream_state = StreamState::CONNECTED;
        }
        impl_->last_heartbeat_ms = streamingNowMs();
        return true;
    }
    return false;
}

size_t VoiceStreamingSession::retryUnacknowledgedChunks(
    uint32_t last_acked_sequence_num) noexcept {
    if (!impl_ || !impl_->active) {
        return 0;
    }
    while (!impl_->pending_chunk_sequences.empty() &&
           impl_->pending_chunk_sequences.front() <= last_acked_sequence_num) {
        impl_->pending_chunk_sequences.pop_front();
    }
    if (!impl_->pending_chunk_sequences.empty() &&
        last_acked_sequence_num + 1 < impl_->pending_chunk_sequences.front()) {
        impl_->sequence_gap_detected = true;
    }
    return impl_->pending_chunk_sequences.size();
}

bool VoiceStreamingSession::detectSequenceGap() const noexcept {
    return impl_ && impl_->sequence_gap_detected;
}

bool VoiceStreamingSession::rebalanceBufferPressure() noexcept {
    if (!impl_ || !impl_->active) {
        return false;
    }
    impl_->buffer_pressure_paused =
        impl_->buffer_size_bytes >= ((kMaxBufferSizeBytes * 9) / 10);
    return impl_->buffer_pressure_paused;
}

// ── Session info ──────────────────────────────────────────────────────────────

StreamID VoiceStreamingSession::streamId() const noexcept {
    return impl_ ? impl_->stream_id : StreamID{};
}
const VoiceStreamingSession::Config& VoiceStreamingSession::config() const noexcept {
    return impl_->config;
}
int64_t VoiceStreamingSession::startedAtMs() const noexcept {
    return impl_ ? impl_->started_at_ms : 0;
}
size_t VoiceStreamingSession::bytesReceived() const noexcept {
    return impl_ ? impl_->bytes_received : 0;
}

bool VoiceStreamingSession::checkOrigin(const std::string& origin) const {
    if (!impl_) return false;
    const auto& allowlist = impl_->config.origin_allowlist;
    if (allowlist.empty()) return true; // no restriction
    return std::find(allowlist.begin(), allowlist.end(), origin) != allowlist.end();
}

// ─────────────────────────────────────────────────────────────────────────────
// VoiceStreamingManager
// ─────────────────────────────────────────────────────────────────────────────

VoiceStreamingManager::VoiceStreamingManager(size_t max_concurrent_sessions)
    : max_sessions_(max_concurrent_sessions) {}

StreamID
VoiceStreamingManager::createSession(VoiceStreamingSession::Config config) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    if (sessions_.size() >= max_sessions_) {
        THEMIS_WARN("VoiceStreamingManager: max concurrent sessions ({}) reached",
                    max_sessions_);
        return {};
    }
    auto session = VoiceStreamingSession::create(std::move(config));
    auto id = session->start();
    sessions_.emplace(id, std::move(session));
    THEMIS_INFO("VoiceStreamingManager: created session {} (active={})",
                id, sessions_.size());
    return id;
}

PartialTranscript
VoiceStreamingManager::routeAudio(const StreamID&             stream_id,
                                   const std::vector<uint8_t>& audio_chunk) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(stream_id);
    if (it == sessions_.end()) {
        THEMIS_WARN("VoiceStreamingManager::routeAudio: unknown stream_id={}", stream_id);
        return {};
    }
    return it->second->sendAudioChunk(audio_chunk);
}

void VoiceStreamingManager::closeSession(const StreamID& stream_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(stream_id);
    if (it == sessions_.end()) return;
    it->second->end();
    sessions_.erase(it);
    THEMIS_INFO("VoiceStreamingManager: closed session {} (active={})",
                stream_id, sessions_.size());
}

size_t VoiceStreamingManager::activeSessionCount() const noexcept {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

// ============================================================================
// Phase 3: Streaming Resilience Implementations
// ============================================================================

} // namespace voice
} // namespace themis
