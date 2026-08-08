/**
 * @file voice_browser_streaming.h
 * @brief Voice Streaming & WebSocket — Frozen API Contract for Phase 1.
 *
 * @version v1.0 frozen as of 2026-08-08
 *
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Design/API Contract Frozen (Phase 1)
 *
 * ## Stream Lifecycle State Machine (Frozen)
 * ```
 * [CREATE] → [CONNECTED] → [STREAMING] → [CLOSED]
 *               ↓             ↓
 *           [ERROR] ←────────┘
 * ```
 *
 * - **CONNECTED**: Session created, ready to receive audio
 * - **STREAMING**: Audio frames flowing, STT active
 * - **CLOSED**: Session terminated gracefully
 * - **ERROR**: Unrecoverable error (see error codes)
 *
 * ## Streaming Contract (Frozen)
 *
 * **Chunk Delivery Guarantees:**
 * - Chunks processed in strict order (FIFO)
 * - Chunks not duplicated; at-most-once semantics
 * - Chunks may be coalesced (multiple frames → single STT call)
 * - No chunk reordering; out-of-order arrival = frame error
 *
 * **Concurrent Session Limit:** >= 100 (frozen minimum)
 *
 * ## Error Codes (Voice Module — Streaming)
 * - 6900: Stream creation failed
 * - 6901: Stream not found / closed
 * - 6902: Audio frame too large for stream
 * - 6903: Stream timeout (no activity)
 * - 6904: Codec mismatch / unsupported format
 * - 6905: Origin/CORS validation failed
 * - 6906: STT engine error
 * - 6907-6999: Reserved for streaming errors
 *
 * ## Thread Safety
 * - VoiceStreamingSession is thread-safe for concurrent audio input
 * - VoiceStreamingManager is thread-safe for concurrent session creation
 * - Callbacks (onPartialTranscript, onFinalTranscript) run on internal worker threads
 */

/*
 * ThemisDB | File: voice_browser_streaming.h | Version: v1.0 FROZEN
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Status: Design/API Contract Frozen (Phase 1)
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace voice {

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

/// Opaque stream identifier
using StreamID = std::string;

/**
 * @brief Word-level timing information from STT.
 */
struct Word {
    std::string text;
    float       start_s = 0.0f;   ///< Start time in seconds relative to stream start
    float       end_s   = 0.0f;   ///< End time in seconds
    float       confidence = 0.0f;
};

/**
 * @brief Incremental STT result returned while audio is still flowing.
 */
struct PartialTranscript {
    StreamID    stream_id;
    std::string text;             ///< Text recognised so far (may change)
    bool        is_final = false; ///< True iff this is the conclusive result
    float       confidence = 0.0f;
    std::vector<Word> words;      ///< Word-level timing (if available)
    int64_t     timestamp_ms = 0; ///< Wall-clock time of this update
};

/**
 * @brief Definitive transcript + intent for a completed utterance.
 */
struct FinalTranscript {
    StreamID    stream_id;
    std::string text;
    float       confidence = 0.0f;
    std::vector<Word> words;
    std::string intent;           ///< NLU intent label (empty if not run)
    float       intent_confidence = 0.0f;
    int64_t     duration_ms = 0;  ///< Total utterance duration
};

/**
 * @brief Audio format description for incoming browser audio.
 *
 * Kept distinct from the storage-layer AudioFormat to avoid namespace-level
 * collisions between streaming transport metadata and persisted audio
 * inventory metadata.
 */
struct StreamAudioFormat {
    enum class Encoding { PCM16, OPUS, WEBM_OPUS };
    Encoding encoding      = Encoding::PCM16;
    uint32_t sample_rate   = 16000;  ///< Hz
    uint16_t channels      = 1;
    uint16_t bits_per_sample = 16;   ///< Only for PCM16
};

// ─────────────────────────────────────────────────────────────────────────────
// VoiceStreamingSession
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief WebSocket voice streaming session.
 *
 * Manages the lifecycle of one bidirectional voice stream from a browser
 * client: audio ingestion, incremental STT, optional NLU, and TTS synthesis
 * back to the browser.
 *
 * ## Typical flow
 * ```cpp
 * auto session = VoiceStreamingSession::create(config);
 *
 * // Attach callbacks before starting
 * session->onPartialTranscript([](auto& pt){ … });
 * session->onFinalTranscript([](auto& ft){ … });
 * session->onTtsChunk([](auto& chunk){ … }); // send back to browser
 *
 * session->start();
 *
 * // Called by the WebSocket on each incoming message:
 * session->sendAudioChunk(raw_audio_bytes);
 *
 * session->end(); // or auto-closes after max_duration_s
 * ```
 */
class VoiceStreamingSession {
public:
    /**
     * @brief Session configuration.
     */
    struct Config {
        std::string session_id;               ///< Auth/user session ID
        std::string user_id;                  ///< Optional user ID for personalisation
        StreamAudioFormat audio_format;       ///< Expected incoming browser audio format
        std::string language = "en";          ///< BCP-47 language code
        bool        run_nlu  = true;          ///< Extract intent after each utterance
        bool        enable_tts = false;       ///< Synthesise and stream TTS back to browser
        uint32_t    max_frame_bytes = 65536;  ///< Max accepted WebSocket frame size (64 KB)
        uint32_t    max_duration_s  = 600;    ///< Auto-close after this many seconds (10 min)
        bool        partial_results = true;   ///< Emit PartialTranscript as audio arrives
        /// Permitted WebRTC/HTTP request origins (e.g. "https://app.example.com").
        /// An empty list means all origins are allowed.  When non-empty, only
        /// exact string matches are accepted; pass the full scheme+host[:port].
        std::vector<std::string> origin_allowlist;
    };

    /**
     * @brief Factory: construct and return a new session.
     *
     * @param config  Session configuration.
     * @return Owning pointer to the new session.
     * @throws std::invalid_argument if config is invalid.
     */
    static std::unique_ptr<VoiceStreamingSession> create(Config config);

    ~VoiceStreamingSession();

    // Non-copyable
    VoiceStreamingSession(const VoiceStreamingSession&)            = delete;
    VoiceStreamingSession& operator=(const VoiceStreamingSession&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the session (allocate resources, begin STT pipeline).
     * @return The session's unique StreamID.
     */
    StreamID start();

    /**
     * @brief Terminate the session and flush any buffered audio.
     *
     * Blocks until the final transcript is delivered.
     */
    void end();

    /**
     * @brief True while the session is active (between start() and end()).
     */
    bool isActive() const noexcept;

    // ── Audio ingestion ───────────────────────────────────────────────────────

    /**
     * @brief Feed an audio chunk from the browser WebSocket frame.
     *
     * @param audio_chunk  Raw audio bytes (format specified in Config::audio_format).
     * @return PartialTranscript if Config::partial_results is true and STT has an
     *         incremental result; otherwise empty (is_final=false, text="").
     */
    PartialTranscript sendAudioChunk(const std::vector<uint8_t>& audio_chunk);

    /**
     * @brief Signal end-of-utterance (e.g. silence detected or VAD trigger).
     *
     * Forces the STT engine to finalise the current hypothesis.
     */
    void endOfUtterance();
    
    // Phase 3: Streaming Resilience
    
    /**
     * @brief Send a heartbeat/keep-alive ping (Phase 3).
     * 
     * Detects mid-stream connection loss via TCP keep-alive.
     * @return true if connection is alive; false if connection lost
     */
    bool sendHeartbeat() noexcept;
    
    /**
     * @brief Automatically reconnect with exponential backoff (Phase 3).
     * 
     * @param max_retries Maximum reconnection attempts (default 5)
     * @return true if reconnected successfully; false if gave up
     */
    bool reconnectWithBackoff(int max_retries = 5) noexcept;
    
    /**
     * @brief Resend unacknowledged chunks (Phase 3).
     * 
     * @param last_acked_sequence_num Sequence number of last acknowledged chunk
     * @return Number of chunks resent
     */
    size_t retryUnacknowledgedChunks(uint32_t last_acked_sequence_num) noexcept;
    
    /**
     * @brief Detect lost chunks via sequence gaps (Phase 3).
     * 
     * @return true if sequence gap detected; false if all chunks accounted for
     */
    bool detectSequenceGap() const noexcept;
    
    /**
     * @brief Pause/resume streaming if buffer critical (Phase 3).
     * 
     * @return true if paused due to buffer pressure; false if streaming normally
     */
    bool rebalanceBufferPressure() noexcept;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    using PartialTranscriptCb = std::function<void(const PartialTranscript&)>;
    using FinalTranscriptCb   = std::function<void(const FinalTranscript&)>;
    using TtsChunkCb          = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorCb             = std::function<void(const std::string& error)>;

    void onPartialTranscript(PartialTranscriptCb cb);
    void onFinalTranscript(FinalTranscriptCb cb);
    void onTtsChunk(TtsChunkCb cb);       ///< Called with TTS audio chunks when enable_tts=true
    void onError(ErrorCb cb);

    // ── STT backend injection ─────────────────────────────────────────────────

    /**
     * @brief Inject a real STT transcription backend.
     *
     * The function receives the current audio buffer, an `is_final` flag
     * (true when `endOfUtterance()` triggers the call), and the sequence
     * counter.  It must return a populated `PartialTranscript`; the session
     * will overwrite `stream_id` and, if `timestamp_ms` is zero, set it to
     * the current wall-clock time.
     *
     * When no function is injected the session falls back to the built-in
     * placeholder that returns `[partial#N:MBB]` text.
     *
     * Signature: (audio_buffer, is_final, seq) → PartialTranscript
     */
    using TranscribeFn = std::function<PartialTranscript(
        const std::vector<uint8_t>& audio,
        bool is_final,
        uint32_t seq)>;

    /// Inject an STT transcription backend.  Passing null resets to the
    /// built-in placeholder.
    void setTranscribeBackend(TranscribeFn fn);

    // ── Session info ──────────────────────────────────────────────────────────

    StreamID     streamId()    const noexcept;
    const Config& config()     const noexcept;
    int64_t      startedAtMs() const noexcept;
    size_t       bytesReceived()const noexcept;

    // ── Origin allowlist ──────────────────────────────────────────────────────

    /**
     * @brief Check whether @p origin is permitted by this session's allowlist.
     *
     * Returns true when Config::origin_allowlist is empty (no restriction) or
     * when @p origin is found verbatim in the allowlist.  The comparison is
     * case-sensitive; callers should normalise the origin to lowercase before
     * calling this method.
     *
     * @param origin  Full scheme+host[:port] of the connecting client
     *                (e.g. "https://app.example.com").
     */
    bool checkOrigin(const std::string& origin) const;

private:
    explicit VoiceStreamingSession(Config config);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// VoiceStreamingManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages the pool of active VoiceStreamingSession instances.
 *
 * Enforces the ≥100 concurrent-session requirement and provides lookup by
 * StreamID for the WebSocket message router.
 *
 * @note Thread Safety: All public methods are thread-safe.
 */
class VoiceStreamingManager {
public:
    explicit VoiceStreamingManager(size_t max_concurrent_sessions = 200);
    ~VoiceStreamingManager() = default;

    /**
     * @brief Create a new session and register it.
     *
     * @return StreamID of the created session, or empty string when the
     *         max_concurrent_sessions limit is reached.
     */
    StreamID createSession(VoiceStreamingSession::Config config);

    /**
     * @brief Route an incoming audio chunk to the correct session.
     *
     * @return PartialTranscript for the session, or empty if not found.
     */
    PartialTranscript routeAudio(const StreamID&             stream_id,
                                  const std::vector<uint8_t>& audio_chunk);

    /**
     * @brief Terminate and remove a session.
     */
    void closeSession(const StreamID& stream_id);

    /**
     * @brief Return the number of currently active sessions.
     */
    size_t activeSessionCount() const noexcept;

private:
    size_t max_sessions_;
    mutable std::mutex sessions_mutex_;
    std::unordered_map<std::string, std::unique_ptr<VoiceStreamingSession>> sessions_;
};

} // namespace voice
} // namespace themis
