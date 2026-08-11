/**
 * @file voice_telephony.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace voice {

// ─────────────────────────────────────────────────────────────────────────────
// Common call data types
// ─────────────────────────────────────────────────────────────────────────────

/// Opaque call identifier (UUID-style)
using CallID = std::string;

/**
 * @brief Audio codec identifier used in SIP/WebRTC negotiation.
 */
enum class AudioCodec {
    PCMU,   ///< G.711 µ-law, 8 kHz (RTP payload 0)
    PCMA,   ///< G.711 A-law, 8 kHz (RTP payload 8)
    G722,   ///< G.722, 8 kHz clock / 16 kHz audio (RTP payload 9)
    OPUS,   ///< Opus, variable rate (RTP dynamic payload)
};

/**
 * @brief Current state of a telephony call.
 */
enum class CallState {
    IDLE,        ///< Not started
    CONNECTING,  ///< SIP INVITE / WebRTC offer sent, awaiting answer
    ACTIVE,      ///< Media flowing
    ON_HOLD,     ///< Call held (no media)
    TERMINATING, ///< BYE / close-offer sent
    TERMINATED,  ///< Call ended
    ERROR,       ///< Unrecoverable error
};

/**
 * @brief Direction of the call.
 */
enum class CallDirection { INBOUND, OUTBOUND };

/**
 * @brief Decoded DTMF event (RFC 4733).
 */
struct DtmfEvent {
    char    digit;          ///< '0'-'9', '*', '#', 'A'-'D'
    int     duration_ms;    ///< Tone duration in milliseconds
    int64_t timestamp_ms;   ///< Wall-clock time of event
};

/**
 * @brief Incremental STT result forwarded from the voice pipeline.
 *
 * Mirrors the fields used by VoiceStreamingSession to keep APIs consistent.
 */
struct CallTranscript {
    CallID      call_id;
    std::string text;
    bool        is_final    = false;
    float       confidence  = 0.0f;
    int64_t     timestamp_ms = 0;
};

/**
 * @brief IVR menu node for building voice-driven interaction flows.
 */
struct IvrNode {
    std::string id;             ///< Unique node identifier
    std::string prompt_text;    ///< TTS text played to the caller
    std::unordered_map<char, std::string> dtmf_routes; ///< digit → next node id
    std::string speech_route;   ///< node id for non-DTMF speech input
    bool        is_terminal = false; ///< True if this node ends the flow
};

/**
 * @brief Result of a completed IVR interaction.
 */
struct IvrResult {
    CallID      call_id;
    std::string terminal_node_id;   ///< ID of the node that concluded the session
    std::vector<DtmfEvent> dtmf_collected; ///< All DTMF events captured
    std::string speech_transcript;  ///< Aggregated spoken input, if any
    bool        completed = false;  ///< True if a terminal node was reached
};

// ─────────────────────────────────────────────────────────────────────────────
// ITtsBackend — injectable TTS encoder interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Injectable text-to-speech backend interface.
 *
 * Implement this interface to supply a real TTS encoder (e.g. a G.711 µ-law
 * PCMU encoder for SIP, or an Opus encoder for WebRTC).
 *
 * ## Contract
 * - `synthesize()` must be thread-safe.
 * - Each returned inner `vector<uint8_t>` represents one RTP payload frame
 *   (without RTP header); the session wrapper adds the correct RTP header.
 * - An empty outer vector indicates synthesis failure or unsupported text.
 *
 * @see SipCallSession::setTtsBackend()
 * @see WebRtcCallSession::setTtsBackend()
 */
class ITtsBackend {
public:
    virtual ~ITtsBackend() = default;

    /**
     * @brief Synthesise @p text into a sequence of encoded audio frames.
     *
     * @param text   UTF-8 text to synthesise.
     * @param codec  Target codec (used to select the encoder).
     * @return       Encoded audio payload frames (one per 20 ms); empty on
     *               failure or when the text is empty.
     */
    virtual std::vector<std::vector<uint8_t>> synthesize(
        const std::string& text,
        AudioCodec         codec) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages one SIP call session within the ThemisDB voice pipeline.
 *
 * Handles SIP signalling lifecycle (INVITE → 200 OK → ACK → BYE), RTP
 * media packetisation/depacketisation, DTMF decoding (RFC 4733), and
 * incremental STT forwarding.
 *
 * ## Typical inbound flow
 * ```cpp
 * auto session = SipCallSession::create(config);
 * session->onTranscript([](auto& t){ // process transcript
 * });
 * session->onDtmf([](auto& d){ // handle digit
 * });
 * session->start();                 // send 200 OK
 * session->receiveRtpPacket(pkt);   // called per arriving RTP packet
 * session->end();                   // sends BYE
 * ```
 */
class SipCallSession {
public:
    /**
     * @brief SIP call session configuration.
     */
    struct Config {
        std::string call_id;            ///< SIP Call-ID header value
        std::string from_uri;           ///< Calling party SIP URI
        std::string to_uri;             ///< Called party SIP URI
        CallDirection direction = CallDirection::INBOUND;
        AudioCodec  codec = AudioCodec::PCMU;
        uint32_t    sample_rate = 8000; ///< Hz (8000 for G.711, 16000 for G.722)
        std::string language = "en";    ///< BCP-47 language code for STT
        bool        enable_tts = true;  ///< Synthesise TTS responses
        bool        record_audio = false; ///< Store raw RTP audio for audit
        uint32_t    max_duration_s = 3600; ///< Auto-terminate after this many seconds
    };

    /**
     * @brief Factory: construct and return a new SIP call session.
     *
     * @param config  Session configuration.
     * @return Owning pointer to the session.
     * @throws std::invalid_argument if config fields are invalid.
     */
    static std::unique_ptr<SipCallSession> create(Config config);

    ~SipCallSession();

    // Non-copyable
    SipCallSession(const SipCallSession&)            = delete;
    SipCallSession& operator=(const SipCallSession&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the session (allocate RTP buffer, begin STT pipeline).
     * @return The session's unique CallID.
     */
    CallID start();

    /**
     * @brief Terminate the session and flush buffered audio.
     *
     * Sends SIP BYE and blocks until final transcript is delivered.
     */
    void end();

    /**
     * @brief Put the call on hold (stop processing incoming RTP).
     */
    void hold();

    /**
     * @brief Resume a held call.
     */
    void unhold();

    /** @brief True while the call is active (CONNECTING or ACTIVE state). */
    bool isActive() const noexcept;

    /** @brief Current call state. */
    CallState state() const noexcept;

    // ── Media ingestion ───────────────────────────────────────────────────────

    /**
     * @brief Feed an RTP packet from the network.
     *
     * Strips the RTP header, decodes the payload with the negotiated codec,
     * appends PCM to the STT pipeline, and returns any incremental transcript.
     *
     * @param rtp_packet  Raw RTP packet bytes (header + payload).
     * @return Incremental CallTranscript, or empty if no new hypothesis.
     * @note Malformed RTP headers, empty payloads, and misaligned linear-audio
     *       payloads are rejected fail-closed.
     */
    CallTranscript receiveRtpPacket(const std::vector<uint8_t>& rtp_packet);

    /**
     * @brief Feed a decoded PCM audio frame directly (bypass RTP parsing).
     *
     * Useful when the SIP stack has already decoded the audio.
     *
     * @param pcm_samples  16-bit signed PCM samples (little-endian).
     * @return Incremental CallTranscript.
     * @note Empty or over-limit PCM frames are rejected fail-closed.
     */
    CallTranscript receiveAudioFrame(const std::vector<int16_t>& pcm_samples);

    /**
     * @brief Inject a DTMF event (e.g. decoded from RFC 4733 RTP event packet).
     */
    void injectDtmf(const DtmfEvent& event);

    // ── Output ────────────────────────────────────────────────────────────────

    /**
     * @brief Synthesise @p text to speech and return RTP-encoded audio.
     *
     * When a TTS backend has been injected via setTtsBackend(), delegates
     * synthesis to that backend and wraps each returned frame with a minimal
     * G.711 RTP header.  Without an injected backend the previous stub
     * behaviour (raw text bytes in an RTP packet) is retained for compatibility.
     *
     * @param text  Text to speak to the caller.
     * @return RTP packets (one per 20 ms audio frame), empty on error.
     */
    std::vector<std::vector<uint8_t>> synthesizeTts(const std::string& text);

    /**
     * @brief Inject a TTS backend for real G.711 audio synthesis.
     *
     * When set, `synthesizeTts()` delegates to this backend.  Pass `nullptr`
     * to revert to the stub (raw-text) fallback.
     *
     * Thread safety: must be called before the first `synthesizeTts()` call.
     */
    void setTtsBackend(std::shared_ptr<ITtsBackend> backend);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    using TranscriptCb = std::function<void(const CallTranscript&)>;
    using DtmfCb       = std::function<void(const DtmfEvent&)>;
    using StateCb      = std::function<void(CallState)>;
    using ErrorCb      = std::function<void(const std::string&)>;

    void onTranscript(TranscriptCb cb);
    void onDtmf(DtmfCb cb);
    void onStateChange(StateCb cb);
    void onError(ErrorCb cb);

    // ── Session info ──────────────────────────────────────────────────────────

    CallID            callId()          const noexcept;
    const Config&     config()          const noexcept;
    int64_t           startedAtMs()     const noexcept;
    size_t            bytesReceived()   const noexcept;
    size_t            rtpPacketsReceived() const noexcept;

private:
    explicit SipCallSession(Config config);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// WebRtcCallSession
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages one WebRTC peer-connection session within the voice pipeline.
 *
 * Handles the WebRTC offer/answer exchange (SDP), DTLS-SRTP media, Opus
 * audio decoding, ICE candidate processing, DTMF, and STT forwarding.
 *
 * ## Typical browser-initiated flow
 * ```cpp
 * auto session = WebRtcCallSession::create(config);
 * session->onTranscript([](auto& t){ // process
 * });
 * auto answer_sdp = session->processOffer(offer_sdp);
 * session->start();
 * session->addIceCandidate(ice_json);
 * session->receiveAudioFrame(pcm);   // called by DTLS-SRTP stack
 * session->end();
 * ```
 */
class WebRtcCallSession {
public:
    /**
     * @brief WebRTC call session configuration.
     */
    struct Config {
        std::string session_id;         ///< Application-level session identifier
        std::string user_id;            ///< Optional user ID for personalisation
        AudioCodec  preferred_codec = AudioCodec::OPUS;
        uint32_t    sample_rate     = 48000; ///< Hz (Opus standard)
        std::string language        = "en";
        bool        enable_tts      = false;
        bool        record_audio    = false;
        uint32_t    max_duration_s  = 3600;
        bool        require_dtls    = true;  ///< Reject connections without DTLS-SRTP
    };

    /**
     * @brief Factory: construct and return a new WebRTC session.
     * @throws std::invalid_argument if config is invalid.
     */
    static std::unique_ptr<WebRtcCallSession> create(Config config);

    ~WebRtcCallSession();

    // Non-copyable
    WebRtcCallSession(const WebRtcCallSession&)            = delete;
    WebRtcCallSession& operator=(const WebRtcCallSession&) = delete;

    // ── Signalling ────────────────────────────────────────────────────────────

    /**
     * @brief Process an SDP offer from the remote peer.
     *
     * Selects the best matching codec from the offer, generates an SDP answer,
     * and prepares ICE/DTLS parameters.
     *
     * @param sdp_offer  SDP string from the remote WebRTC client.
     * @return SDP answer string to be returned to the peer.
     * @throws std::runtime_error if SDP cannot be parsed or no codec matches.
     */
    std::string processOffer(const std::string& sdp_offer);

    /**
     * @brief Add a remote ICE candidate (from the trickle-ICE exchange).
     *
     * @param candidate_json  JSON object with "candidate", "sdpMid",
     *                        "sdpMLineIndex" keys (as produced by the browser).
     */
    void addIceCandidate(const std::string& candidate_json);

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the session after ICE/DTLS negotiation completes.
     * @return The session's unique CallID.
     */
    CallID start();

    /**
     * @brief Terminate the WebRTC connection and flush STT buffers.
     */
    void end();

    /** @brief True while the connection is active. */
    bool isActive() const noexcept;

    /** @brief Current call state. */
    CallState state() const noexcept;

    // ── Media ingestion ───────────────────────────────────────────────────────

    /**
     * @brief Feed a decoded PCM audio frame from the DTLS-SRTP stack.
     *
     * @param pcm_samples  16-bit signed PCM at the negotiated sample rate.
     * @return Incremental CallTranscript.
     */
    CallTranscript receiveAudioFrame(const std::vector<int16_t>& pcm_samples);

    /**
     * @brief Inject a DTMF event decoded from the media track.
     */
    void injectDtmf(const DtmfEvent& event);

    // ── Output ────────────────────────────────────────────────────────────────

    /**
     * @brief Synthesise @p text and return Opus-encoded RTP packets.
     *
     * When a TTS backend has been injected via setTtsBackend(), delegates
     * synthesis to that backend and wraps each returned frame with a minimal
     * Opus RTP header (PT=111).  Without an injected backend the previous stub
     * behaviour (raw text bytes in an RTP packet) is retained for compatibility.
     */
    std::vector<std::vector<uint8_t>> synthesizeTts(const std::string& text);

    /**
     * @brief Inject a TTS backend for real Opus audio synthesis.
     *
     * When set, `synthesizeTts()` delegates to this backend.  Pass `nullptr`
     * to revert to the stub (raw-text) fallback.
     *
     * Thread safety: must be called before the first `synthesizeTts()` call.
     */
    void setTtsBackend(std::shared_ptr<ITtsBackend> backend);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    using TranscriptCb = std::function<void(const CallTranscript&)>;
    using DtmfCb       = std::function<void(const DtmfEvent&)>;
    using StateCb      = std::function<void(CallState)>;
    using ErrorCb      = std::function<void(const std::string&)>;
    using IceCandidateCb = std::function<void(const std::string& candidate_json)>;

    void onTranscript(TranscriptCb cb);
    void onDtmf(DtmfCb cb);
    void onStateChange(StateCb cb);
    void onError(ErrorCb cb);
    /** @brief Called when the local ICE candidate is ready to be sent to the peer. */
    void onLocalIceCandidate(IceCandidateCb cb);

    // ── Session info ──────────────────────────────────────────────────────────

    CallID            callId()        const noexcept;
    const Config&     config()        const noexcept;
    int64_t           startedAtMs()   const noexcept;
    size_t            bytesReceived() const noexcept;
    std::string       negotiatedSdp() const noexcept; ///< SDP answer used

private:
    explicit WebRtcCallSession(Config config);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// IvrEngine
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief State-machine IVR engine driven by DTMF and/or speech input.
 *
 * Nodes are added with addNode(); the active node is advanced via
 * handleDtmf() or handleSpeech().  The TTS prompt for each node is
 * returned so the caller can pipe it to the call's synthesizeTts().
 *
 * ## Example
 * ```cpp
 * IvrEngine ivr("root");
 * ivr.addNode({"root",  "Press 1 for sales, 2 for support.",
 *              {{'1', "sales"}, {'2', "support"}}, "", false});
 * ivr.addNode({"sales", "Connecting to sales.", {}, "", true});
 * ivr.addNode({"support","Connecting to support.", {}, "", true});
 *
 * auto prompt = ivr.currentPrompt(); // "Press 1 for sales…"
 * ivr.handleDtmf({'1', 200, ts});    // navigate to "sales"
 * bool done = ivr.isTerminal();      // true
 * ```
 */
class IvrEngine {
public:
    /**
     * @brief Construct an IVR engine with the given root node ID.
     * @param root_node_id  ID of the node to start from.
     */
    explicit IvrEngine(std::string root_node_id);

    /**
     * @brief Register an IVR node.  Duplicate IDs overwrite the previous entry.
     */
    void addNode(IvrNode node);

    /**
     * @brief Handle a DTMF digit, potentially advancing the current node.
     *
     * @param event  Decoded DTMF event.
     * @return Prompt text of the new node, or empty if no route matched.
     */
    std::string handleDtmf(const DtmfEvent& event);

    /**
     * @brief Handle spoken input, using the node's speech_route if set.
     *
     * @param text  Transcribed speech.
     * @return Prompt text of the new node, or empty if no speech route.
     */
    std::string handleSpeech(const std::string& text);

    /** @brief TTS prompt for the currently active node. */
    std::string currentPrompt() const;

    /** @brief ID of the currently active node. */
    std::string currentNodeId() const;

    /** @brief True if the current node is a terminal node. */
    bool isTerminal() const;

    /**
     * @brief Collect the full IVR result for the given call.
     */
    IvrResult collectResult(const CallID& call_id) const;

    /** @brief Reset to the root node. */
    void reset();

private:
    std::string root_node_id_;
    std::string current_node_id_;
    std::unordered_map<std::string, IvrNode> nodes_;
    std::vector<DtmfEvent>  collected_dtmf_;
    std::string             collected_speech_;
};

// ─────────────────────────────────────────────────────────────────────────────
// TelephonyBridge configuration (namespace-scope for GCC 13 compatibility)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Global TelephonyBridge configuration.
 */
struct TelephonyBridgeConfig {
    size_t   max_concurrent_calls   = 200; ///< Soft limit across all protocols
    uint32_t default_max_duration_s = 3600;
    bool     enable_sip    = true;
    bool     enable_webrtc = true;
    bool     record_audio  = false; ///< Default recording policy
};

// ─────────────────────────────────────────────────────────────────────────────
// TelephonyBridge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Central coordinator for all telephony (SIP + WebRTC) call sessions.
 *
 * Manages the pool of active SipCallSession and WebRtcCallSession objects,
 * enforces concurrency limits, and provides unified routing for audio and
 * signalling events.
 *
 * @note Thread Safety: All public methods are thread-safe.
 */
class TelephonyBridge {
public:
    using Config = TelephonyBridgeConfig;

    explicit TelephonyBridge(Config config = Config{});
    ~TelephonyBridge() = default;

    // Non-copyable
    TelephonyBridge(const TelephonyBridge&)            = delete;
    TelephonyBridge& operator=(const TelephonyBridge&) = delete;

    // ── SIP call management ───────────────────────────────────────────────────

    /**
     * @brief Register an inbound SIP call and start its session.
     *
     * @param config  Call configuration derived from the SIP INVITE.
     * @return CallID of the new session, or empty string when the concurrency
     *         limit has been reached.
     */
    CallID acceptSipCall(SipCallSession::Config config);

    /**
     * @brief Initiate an outbound SIP call.
     * @return CallID of the new session.
     */
    CallID dialSip(SipCallSession::Config config);

    /**
     * @brief Route an incoming RTP packet to the correct SIP session.
     *
     * @return Incremental transcript, or empty if call not found.
     */
    CallTranscript routeSipRtp(const CallID&                call_id,
                                const std::vector<uint8_t>& rtp_packet);

    /**
     * @brief Terminate a SIP call by ID.
     */
    void terminateSipCall(const CallID& call_id);

    // ── WebRTC call management ────────────────────────────────────────────────

    /**
     * @brief Create a new WebRTC session and process the SDP offer.
     *
     * @param config      Session configuration.
     * @param sdp_offer   SDP offer from the browser.
     * @param out_call_id Set to the new CallID on success.
     * @return SDP answer string, or empty string on failure.
     */
    std::string acceptWebRtcOffer(WebRtcCallSession::Config config,
                                   const std::string&        sdp_offer,
                                   CallID&                   out_call_id);

    /**
     * @brief Forward a remote ICE candidate to the correct WebRTC session.
     */
    void routeIceCandidate(const CallID&      call_id,
                            const std::string& candidate_json);

    /**
     * @brief Route a decoded audio frame to the correct WebRTC session.
     *
     * @return Incremental transcript, or empty if call not found.
     */
    CallTranscript routeWebRtcAudio(const CallID&                  call_id,
                                     const std::vector<int16_t>&    pcm_samples);

    /**
     * @brief Terminate a WebRTC call by ID.
     */
    void terminateWebRtcCall(const CallID& call_id);

    // ── Unified access ────────────────────────────────────────────────────────

    /**
     * @brief Terminate any call (SIP or WebRTC) by ID.
     */
    void terminateCall(const CallID& call_id);

    /** @brief Total number of currently active calls (SIP + WebRTC). */
    size_t activeCallCount() const noexcept;

    /** @brief Number of active SIP calls. */
    size_t activeSipCallCount() const noexcept;

    /** @brief Number of active WebRTC calls. */
    size_t activeWebRtcCallCount() const noexcept;

    /**
     * @brief Return the state of any call by ID.
     * @return CallState::IDLE if the call is not found.
     */
    CallState callState(const CallID& call_id) const;

private:
    Config config_;
    mutable std::mutex sip_mutex_;
    mutable std::mutex webrtc_mutex_;

    std::unordered_map<std::string, std::unique_ptr<SipCallSession>>    sip_calls_;
    std::unordered_map<std::string, std::unique_ptr<WebRtcCallSession>> webrtc_calls_;
};

} // namespace voice
} // namespace themis
