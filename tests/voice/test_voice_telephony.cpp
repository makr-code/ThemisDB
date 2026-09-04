#include <gtest/gtest.h>
#include "voice/voice_telephony.h"

#include <thread>
#include <vector>

namespace themis {
namespace voice {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static SipCallSession::Config makeSipConfig() {
    SipCallSession::Config cfg;
    cfg.from_uri       = "sip:alice@example.com";
    cfg.to_uri         = "sip:bob@example.com";
    cfg.codec          = AudioCodec::PCMU;
    cfg.sample_rate    = 8000;
    cfg.max_duration_s = 300;
    return cfg;
}

static WebRtcCallSession::Config makeWebRtcConfig() {
    WebRtcCallSession::Config cfg;
    cfg.session_id     = "test-session";
    cfg.user_id        = "test-user";
    cfg.preferred_codec = AudioCodec::OPUS;
    cfg.sample_rate    = 48000;
    cfg.max_duration_s = 300;
    return cfg;
}

/** Minimal 12-byte RTP packet with G.711 PCMU payload */
static std::vector<uint8_t> makePcmuRtpPacket(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> pkt(12, 0);
    pkt[0] = 0x80; // version=2
    pkt[1] = 0x00; // payload type = PCMU (0)
    pkt.insert(pkt.end(), payload.begin(), payload.end());
    return pkt;
}

static std::string minimalSdpOffer(const std::string& codec = "opus") {
    std::string pt   = (codec == "opus") ? "111" : "0";
    std::string rate = (codec == "opus") ? "48000" : "8000";
    return "v=0\r\n"
           "o=- 0 0 IN IP4 127.0.0.1\r\n"
           "s=Test\r\nt=0 0\r\n"
           "m=audio 9 RTP/AVP " + pt + "\r\n"
           "a=rtpmap:" + pt + " " + codec + "/" + rate + "\r\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession — construction & validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(SipCallSession, CreateSucceeds) {
    auto session = SipCallSession::create(makeSipConfig());
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->isActive());
    EXPECT_EQ(session->state(), CallState::IDLE);
}

TEST(SipCallSession, InvalidMaxDurationThrows) {
    auto cfg = makeSipConfig();
    cfg.max_duration_s = 0;
    EXPECT_THROW(SipCallSession::create(cfg), std::invalid_argument);
}

TEST(SipCallSession, ExcessiveMaxDurationThrows) {
    auto cfg = makeSipConfig();
    cfg.max_duration_s = 200000;
    EXPECT_THROW(SipCallSession::create(cfg), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession — lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(SipCallSession, StartReturnsNonEmptyCallId) {
    auto session = SipCallSession::create(makeSipConfig());
    auto id = session->start();
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(session->isActive());
    EXPECT_EQ(session->state(), CallState::ACTIVE);
}

TEST(SipCallSession, StartTwiceReturnsSameId) {
    auto session = SipCallSession::create(makeSipConfig());
    auto id1 = session->start();
    auto id2 = session->start();
    EXPECT_EQ(id1, id2);
}

TEST(SipCallSession, EndTerminatesCall) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    ASSERT_TRUE(session->isActive());
    session->end();
    EXPECT_FALSE(session->isActive());
    EXPECT_EQ(session->state(), CallState::TERMINATED);
}

TEST(SipCallSession, HoldAndUnhold) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    session->hold();
    EXPECT_EQ(session->state(), CallState::ON_HOLD);
    EXPECT_FALSE(session->isActive()); // ON_HOLD is not "active" in the strict sense
    session->unhold();
    EXPECT_EQ(session->state(), CallState::ACTIVE);
    EXPECT_TRUE(session->isActive());
}

TEST(SipCallSession, StateChangeCallbackFired) {
    auto session = SipCallSession::create(makeSipConfig());
    std::vector<CallState> states;
    session->onStateChange([&](CallState s) { states.push_back(s); });
    session->start();
    session->end();
    EXPECT_FALSE(states.empty());
    EXPECT_EQ(states.back(), CallState::TERMINATED);
}

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession — media ingestion
// ─────────────────────────────────────────────────────────────────────────────

TEST(SipCallSession, ReceiveRtpPacketReturnsTranscript) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();

    std::vector<uint8_t> payload(160, 0xFF); // 20 ms of silence (µ-law)
    auto pkt = makePcmuRtpPacket(payload);
    auto ct  = session->receiveRtpPacket(pkt);

    EXPECT_EQ(ct.call_id, session->callId());
    EXPECT_FALSE(ct.text.empty());
    EXPECT_EQ(session->rtpPacketsReceived(), 1u);
    EXPECT_GT(session->bytesReceived(), 0u);
}

TEST(SipCallSession, ReceiveRtpPacketDroppedWhenInactive) {
    auto session = SipCallSession::create(makeSipConfig());
    // Not started — should return empty
    std::vector<uint8_t> payload(160, 0x7F);
    auto pkt = makePcmuRtpPacket(payload);
    auto ct  = session->receiveRtpPacket(pkt);
    EXPECT_TRUE(ct.text.empty());
    EXPECT_EQ(session->rtpPacketsReceived(), 0u);
}

TEST(SipCallSession, ReceiveAudioFrame) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    std::vector<int16_t> pcm(160, 1000);
    auto ct = session->receiveAudioFrame(pcm);
    EXPECT_EQ(ct.call_id, session->callId());
    EXPECT_FALSE(ct.text.empty());
}

TEST(SipCallSession, ShortRtpPacketReturnsEmpty) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    std::vector<uint8_t> short_pkt(4, 0); // shorter than 12-byte RTP header
    auto ct = session->receiveRtpPacket(short_pkt);
    EXPECT_TRUE(ct.text.empty());
}

TEST(SipCallSession, InvalidRtpVersionRejected) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    std::vector<uint8_t> payload(160, 0xFF);
    auto pkt = makePcmuRtpPacket(payload);
    pkt[0] = 0x40; // RTP version 1
    auto ct = session->receiveRtpPacket(pkt);
    EXPECT_TRUE(ct.text.empty());
    EXPECT_EQ(session->rtpPacketsReceived(), 0u);
}

TEST(SipCallSession, EmptyRtpPayloadRejected) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    std::vector<uint8_t> pkt(12, 0);
    pkt[0] = 0x80;
    pkt[1] = 0x00;
    auto ct = session->receiveRtpPacket(pkt);
    EXPECT_TRUE(ct.text.empty());
    EXPECT_EQ(session->rtpPacketsReceived(), 0u);
}

TEST(SipCallSession, OddSizedLinearPayloadRejected) {
    auto cfg = makeSipConfig();
    cfg.codec = AudioCodec::G722;
    auto session = SipCallSession::create(cfg);
    session->start();
    std::vector<uint8_t> pkt(12, 0);
    pkt[0] = 0x80;
    pkt[1] = 0x09;
    pkt.push_back(0x01);
    pkt.push_back(0x02);
    pkt.push_back(0x03);
    auto ct = session->receiveRtpPacket(pkt);
    EXPECT_TRUE(ct.text.empty());
    EXPECT_EQ(session->rtpPacketsReceived(), 0u);
}

TEST(SipCallSession, TranscriptCallbackFired) {
    auto session = SipCallSession::create(makeSipConfig());
    int cb_count = 0;
    session->onTranscript([&](const CallTranscript&) { ++cb_count; });
    session->start();

    std::vector<uint8_t> payload(160, 0xA0);
    session->receiveRtpPacket(makePcmuRtpPacket(payload));
    session->receiveRtpPacket(makePcmuRtpPacket(payload));
    EXPECT_EQ(cb_count, 2);
}

TEST(SipCallSession, EmptyAudioFrameRejected) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    auto ct = session->receiveAudioFrame({});
    EXPECT_TRUE(ct.text.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession — DTMF
// ─────────────────────────────────────────────────────────────────────────────

TEST(SipCallSession, DtmfCallbackFired) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    std::vector<DtmfEvent> events;
    session->onDtmf([&](const DtmfEvent& e) { events.push_back(e); });

    DtmfEvent dtmf{'5', 120, 0};
    session->injectDtmf(dtmf);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].digit, '5');
}

TEST(SipCallSession, InvalidDtmfIgnored) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    int cb_count = 0;
    session->onDtmf([&](const DtmfEvent&) { ++cb_count; });

    session->injectDtmf({'Z', 0, 0});
    EXPECT_EQ(cb_count, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession — TTS synthesis
// ─────────────────────────────────────────────────────────────────────────────

TEST(SipCallSession, SynthesizeTtsReturnsPackets) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    auto pkts = session->synthesizeTts("Hello");
    EXPECT_FALSE(pkts.empty());
    // Each packet must at least carry the 12-byte RTP header
    for (const auto& p : pkts)
        EXPECT_GT(p.size(), 12u);
}

TEST(SipCallSession, SynthesizeTtsEmptyReturnsNothing) {
    auto session = SipCallSession::create(makeSipConfig());
    session->start();
    EXPECT_TRUE(session->synthesizeTts("").empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// SipCallSession — TTS backend injection (STUB #173)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Minimal ITtsBackend that returns N fixed-size payloads per synthesis call.
class FakeTtsBackend : public ITtsBackend {
public:
    explicit FakeTtsBackend(int frame_count = 2, int frame_bytes = 160)
        : frame_count_(frame_count), frame_bytes_(frame_bytes) {}

    std::vector<std::vector<uint8_t>> synthesize(
        const std::string& text, AudioCodec) override
    {
        if (text.empty()) return {};
        last_text_ = text;
        std::vector<std::vector<uint8_t>> frames;
        frames.reserve(static_cast<size_t>(frame_count_));
        for (int i = 0; i < frame_count_; ++i)
            frames.push_back(std::vector<uint8_t>(
                static_cast<size_t>(frame_bytes_), static_cast<uint8_t>(i)));
        return frames;
    }

    const std::string& lastText() const { return last_text_; }

private:
    int frame_count_;
    int frame_bytes_;
    std::string last_text_;
};

} // namespace

TEST(SipCallSession, SynthesizeTtsWithInjectedBackendDelegates) {
    auto session = SipCallSession::create(makeSipConfig());
    auto backend = std::make_shared<FakeTtsBackend>(3, 160);
    session->setTtsBackend(backend);
    session->start();

    auto pkts = session->synthesizeTts("Hello world");
    // 3 frames returned by backend → 3 RTP packets
    ASSERT_EQ(pkts.size(), 3u);
    for (const auto& p : pkts) {
        EXPECT_EQ(p.size(), 12u + 160u); // 12-byte RTP header + 160-byte payload
        EXPECT_EQ(p[0], 0x80u);          // RTP version=2
    }
    EXPECT_EQ(backend->lastText(), "Hello world");
}

TEST(SipCallSession, SynthesizeTtsInjectedBackendEmptyTextReturnsNothing) {
    auto session = SipCallSession::create(makeSipConfig());
    session->setTtsBackend(std::make_shared<FakeTtsBackend>());
    session->start();
    EXPECT_TRUE(session->synthesizeTts("").empty());
}

TEST(SipCallSession, SynthesizeTtsNullBackendReverts) {
    auto session = SipCallSession::create(makeSipConfig());
    session->setTtsBackend(std::make_shared<FakeTtsBackend>(1, 160));
    session->start();
    // Remove the backend; stub path should produce one packet with text in payload
    session->setTtsBackend(nullptr);
    auto pkts = session->synthesizeTts("fallback");
    ASSERT_FALSE(pkts.empty());
    // Stub path: text bytes follow the 12-byte header
    EXPECT_GT(pkts[0].size(), 12u);
}

TEST(WebRtcCallSession, CreateSucceeds) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->isActive());
    EXPECT_EQ(session->state(), CallState::IDLE);
}

TEST(WebRtcCallSession, InvalidMaxDurationThrows) {
    auto cfg = makeWebRtcConfig();
    cfg.max_duration_s = 0;
    EXPECT_THROW(WebRtcCallSession::create(cfg), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// WebRtcCallSession — signalling
// ─────────────────────────────────────────────────────────────────────────────

TEST(WebRtcCallSession, ProcessOfferReturnsAnswer) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    auto answer = session->processOffer(minimalSdpOffer("opus"));
    EXPECT_FALSE(answer.empty());
    EXPECT_NE(answer.find("opus"), std::string::npos); // opus chosen
    EXPECT_EQ(session->state(), CallState::CONNECTING);
}

TEST(WebRtcCallSession, ProcessEmptyOfferThrows) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    EXPECT_THROW(session->processOffer(""), std::runtime_error);
}

TEST(WebRtcCallSession, LocalIceCandidateCallbackFired) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    std::string ice_json;
    session->onLocalIceCandidate([&](const std::string& c) { ice_json = c; });
    session->processOffer(minimalSdpOffer());
    EXPECT_FALSE(ice_json.empty());
    EXPECT_NE(ice_json.find("candidate"), std::string::npos);
}

TEST(WebRtcCallSession, AddIceCandidateDoesNotThrow) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->processOffer(minimalSdpOffer());
    EXPECT_NO_THROW(session->addIceCandidate(
        R"({"candidate":"candidate:0 1 UDP 2122252543 192.168.1.1 9 typ host",)"
        R"("sdpMid":"audio","sdpMLineIndex":0})"));
}

// ─────────────────────────────────────────────────────────────────────────────
// WebRtcCallSession — lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(WebRtcCallSession, StartReturnsNonEmptyCallId) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    auto id = session->start();
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(session->isActive());
    EXPECT_EQ(session->state(), CallState::ACTIVE);
}

TEST(WebRtcCallSession, EndTerminatesCall) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->start();
    session->end();
    EXPECT_FALSE(session->isActive());
    EXPECT_EQ(session->state(), CallState::TERMINATED);
}

TEST(WebRtcCallSession, ReceiveAudioFrameReturnsTranscript) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->start();
    std::vector<int16_t> pcm(960, 500); // 20 ms at 48 kHz
    auto ct = session->receiveAudioFrame(pcm);
    EXPECT_EQ(ct.call_id, session->callId());
    EXPECT_FALSE(ct.text.empty());
    EXPECT_GT(session->bytesReceived(), 0u);
}

TEST(WebRtcCallSession, ReceiveAudioDroppedWhenInactive) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    std::vector<int16_t> pcm(960, 500);
    auto ct = session->receiveAudioFrame(pcm);
    EXPECT_TRUE(ct.text.empty());
}

TEST(WebRtcCallSession, TranscriptCallbackFired) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    int count = 0;
    session->onTranscript([&](const CallTranscript&) { ++count; });
    session->start();
    std::vector<int16_t> pcm(960, 300);
    session->receiveAudioFrame(pcm);
    session->receiveAudioFrame(pcm);
    EXPECT_EQ(count, 2);
}

TEST(WebRtcCallSession, DtmfCallbackFired) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->start();
    DtmfEvent fired{'#', 0, 0};
    session->onDtmf([&](const DtmfEvent& e) { fired = e; });
    DtmfEvent dtmf{'2', 100, 0};
    session->injectDtmf(dtmf);
    EXPECT_EQ(fired.digit, '2');
}

TEST(WebRtcCallSession, SdpNegotiatedAfterOffer) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->processOffer(minimalSdpOffer("opus"));
    EXPECT_FALSE(session->negotiatedSdp().empty());
}

TEST(WebRtcCallSession, SynthesizeTtsReturnsPackets) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->start();
    auto pkts = session->synthesizeTts("Test");
    EXPECT_FALSE(pkts.empty());
    for (const auto& p : pkts)
        EXPECT_GT(p.size(), 12u);
}

// ─────────────────────────────────────────────────────────────────────────────
// WebRtcCallSession — TTS backend injection (STUB #174)
// ─────────────────────────────────────────────────────────────────────────────

TEST(WebRtcCallSession, SynthesizeTtsWithInjectedBackendDelegates) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    auto backend = std::make_shared<FakeTtsBackend>(4, 240);
    session->setTtsBackend(backend);
    session->start();

    auto pkts = session->synthesizeTts("Opus test");
    ASSERT_EQ(pkts.size(), 4u);
    for (const auto& p : pkts) {
        EXPECT_EQ(p.size(), 12u + 240u);
        EXPECT_EQ(p[0], 0x80u);
        EXPECT_EQ(p[1], 111u); // Opus PT
    }
    EXPECT_EQ(backend->lastText(), "Opus test");
}

TEST(WebRtcCallSession, SynthesizeTtsInjectedBackendEmptyTextReturnsNothing) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->setTtsBackend(std::make_shared<FakeTtsBackend>());
    session->start();
    EXPECT_TRUE(session->synthesizeTts("").empty());
}

TEST(WebRtcCallSession, SynthesizeTtsNullBackendReverts) {
    auto session = WebRtcCallSession::create(makeWebRtcConfig());
    session->setTtsBackend(std::make_shared<FakeTtsBackend>(1, 240));
    session->start();
    session->setTtsBackend(nullptr);
    auto pkts = session->synthesizeTts("fallback");
    ASSERT_FALSE(pkts.empty());
    EXPECT_GT(pkts[0].size(), 12u);
    EXPECT_EQ(pkts[0][1], 111u); // still Opus PT in stub path
}

static IvrEngine buildTestIvr() {
    IvrEngine ivr("root");
    ivr.addNode({"root",    "Press 1 for sales, 2 for support.",
                 {{'1', "sales"}, {'2', "support"}}, "", false});
    ivr.addNode({"sales",   "Connecting to sales.",   {}, "", true});
    ivr.addNode({"support", "Connecting to support.", {}, "", true});
    return ivr;
}

TEST(IvrEngine, InitialPromptIsRootPrompt) {
    auto ivr = buildTestIvr();
    EXPECT_EQ(ivr.currentNodeId(), "root");
    EXPECT_FALSE(ivr.currentPrompt().empty());
}

TEST(IvrEngine, DtmfNavigatesToNextNode) {
    auto ivr = buildTestIvr();
    DtmfEvent d{'1', 200, 0};
    auto prompt = ivr.handleDtmf(d);
    EXPECT_EQ(ivr.currentNodeId(), "sales");
    EXPECT_EQ(prompt, "Connecting to sales.");
}

TEST(IvrEngine, UnknownDtmfReturnsEmpty) {
    auto ivr = buildTestIvr();
    DtmfEvent d{'9', 200, 0};
    auto prompt = ivr.handleDtmf(d);
    EXPECT_TRUE(prompt.empty());
    EXPECT_EQ(ivr.currentNodeId(), "root"); // stays on root
}

TEST(IvrEngine, TerminalNodeDetected) {
    auto ivr = buildTestIvr();
    EXPECT_FALSE(ivr.isTerminal());
    DtmfEvent d{'2', 100, 0};
    ivr.handleDtmf(d);
    EXPECT_TRUE(ivr.isTerminal());
}

TEST(IvrEngine, SpeechRouting) {
    IvrEngine ivr("root");
    ivr.addNode({"root", "Say your name.", {}, "name_node", false});
    ivr.addNode({"name_node", "Hello!", {}, "", true});

    auto prompt = ivr.handleSpeech("Alice");
    EXPECT_EQ(ivr.currentNodeId(), "name_node");
    EXPECT_EQ(prompt, "Hello!");
    EXPECT_TRUE(ivr.isTerminal());
}

TEST(IvrEngine, CollectResultContainsDtmf) {
    auto ivr = buildTestIvr();
    DtmfEvent d1{'1', 200, 0};
    DtmfEvent d2{'#', 100, 0};
    // First press navigates to "sales" (terminal); second press has no route but is still collected
    ivr.handleDtmf(d1);
    // Add a second digit manually — inject into the collected list via handleDtmf (no route from terminal)
    ivr.handleDtmf(d2);

    auto result = ivr.collectResult("test-call-1");
    EXPECT_EQ(result.call_id, "test-call-1");
    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.terminal_node_id, "sales");
    EXPECT_GE(result.dtmf_collected.size(), 1u);
}

TEST(IvrEngine, Reset) {
    auto ivr = buildTestIvr();
    DtmfEvent d{'1', 200, 0};
    ivr.handleDtmf(d);
    EXPECT_EQ(ivr.currentNodeId(), "sales");
    ivr.reset();
    EXPECT_EQ(ivr.currentNodeId(), "root");
    EXPECT_FALSE(ivr.isTerminal());
}

// ─────────────────────────────────────────────────────────────────────────────
// TelephonyBridge — SIP
// ─────────────────────────────────────────────────────────────────────────────

TEST(TelephonyBridge, AcceptSipCallReturnsCallId) {
    TelephonyBridge bridge;
    auto id = bridge.acceptSipCall(makeSipConfig());
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(bridge.activeSipCallCount(), 1u);
    EXPECT_EQ(bridge.activeCallCount(), 1u);
}

TEST(TelephonyBridge, DialSipReturnsCallId) {
    TelephonyBridge bridge;
    auto id = bridge.dialSip(makeSipConfig());
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(bridge.activeSipCallCount(), 1u);
}

TEST(TelephonyBridge, SipDisabledReturnsEmpty) {
    TelephonyBridge::Config cfg;
    cfg.enable_sip = false;
    TelephonyBridge bridge(cfg);
    auto id = bridge.acceptSipCall(makeSipConfig());
    EXPECT_TRUE(id.empty());
    EXPECT_EQ(bridge.activeSipCallCount(), 0u);
}

TEST(TelephonyBridge, RouteSipRtpReturnsTranscript) {
    TelephonyBridge bridge;
    auto id = bridge.acceptSipCall(makeSipConfig());
    std::vector<uint8_t> payload(160, 0xAA);
    auto ct = bridge.routeSipRtp(id, makePcmuRtpPacket(payload));
    EXPECT_EQ(ct.call_id, id);
    EXPECT_FALSE(ct.text.empty());
}

TEST(TelephonyBridge, RouteSipRtpUnknownIdReturnsEmpty) {
    TelephonyBridge bridge;
    std::vector<uint8_t> payload(160, 0xAA);
    auto ct = bridge.routeSipRtp("unknown-id", makePcmuRtpPacket(payload));
    EXPECT_TRUE(ct.text.empty());
}

TEST(TelephonyBridge, TerminateSipCall) {
    TelephonyBridge bridge;
    auto id = bridge.acceptSipCall(makeSipConfig());
    EXPECT_EQ(bridge.activeSipCallCount(), 1u);
    bridge.terminateSipCall(id);
    EXPECT_EQ(bridge.activeSipCallCount(), 0u);
}

TEST(TelephonyBridge, MaxConcurrentCallsEnforced) {
    TelephonyBridge::Config cfg;
    cfg.max_concurrent_calls = 2;
    TelephonyBridge bridge(cfg);

    auto id1 = bridge.acceptSipCall(makeSipConfig());
    auto id2 = bridge.acceptSipCall(makeSipConfig());
    auto id3 = bridge.acceptSipCall(makeSipConfig()); // over limit
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_TRUE(id3.empty());
    EXPECT_EQ(bridge.activeSipCallCount(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TelephonyBridge — WebRTC
// ─────────────────────────────────────────────────────────────────────────────

TEST(TelephonyBridge, AcceptWebRtcOfferReturnsAnswer) {
    TelephonyBridge bridge;
    CallID id;
    auto answer = bridge.acceptWebRtcOffer(makeWebRtcConfig(), minimalSdpOffer(), id);
    EXPECT_FALSE(answer.empty());
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(bridge.activeWebRtcCallCount(), 1u);
    EXPECT_EQ(bridge.activeCallCount(), 1u);
}

TEST(TelephonyBridge, WebRtcDisabledReturnsEmpty) {
    TelephonyBridge::Config cfg;
    cfg.enable_webrtc = false;
    TelephonyBridge bridge(cfg);
    CallID id;
    auto answer = bridge.acceptWebRtcOffer(makeWebRtcConfig(), minimalSdpOffer(), id);
    EXPECT_TRUE(answer.empty());
    EXPECT_TRUE(id.empty());
}

TEST(TelephonyBridge, RouteWebRtcAudioReturnsTranscript) {
    TelephonyBridge bridge;
    CallID id;
    bridge.acceptWebRtcOffer(makeWebRtcConfig(), minimalSdpOffer(), id);
    std::vector<int16_t> pcm(960, 200);
    auto ct = bridge.routeWebRtcAudio(id, pcm);
    EXPECT_EQ(ct.call_id, id);
    EXPECT_FALSE(ct.text.empty());
}

TEST(TelephonyBridge, RouteWebRtcAudioUnknownIdReturnsEmpty) {
    TelephonyBridge bridge;
    std::vector<int16_t> pcm(960, 200);
    auto ct = bridge.routeWebRtcAudio("no-such-call", pcm);
    EXPECT_TRUE(ct.text.empty());
}

TEST(TelephonyBridge, TerminateWebRtcCall) {
    TelephonyBridge bridge;
    CallID id;
    bridge.acceptWebRtcOffer(makeWebRtcConfig(), minimalSdpOffer(), id);
    EXPECT_EQ(bridge.activeWebRtcCallCount(), 1u);
    bridge.terminateWebRtcCall(id);
    EXPECT_EQ(bridge.activeWebRtcCallCount(), 0u);
}

TEST(TelephonyBridge, TerminateCallWorksForBothProtocols) {
    TelephonyBridge bridge;
    auto sip_id = bridge.acceptSipCall(makeSipConfig());
    CallID rtc_id;
    bridge.acceptWebRtcOffer(makeWebRtcConfig(), minimalSdpOffer(), rtc_id);
    EXPECT_EQ(bridge.activeCallCount(), 2u);

    bridge.terminateCall(sip_id);
    EXPECT_EQ(bridge.activeCallCount(), 1u);
    bridge.terminateCall(rtc_id);
    EXPECT_EQ(bridge.activeCallCount(), 0u);
}

TEST(TelephonyBridge, CallStateReturnsActiveForKnownCall) {
    TelephonyBridge bridge;
    auto id = bridge.acceptSipCall(makeSipConfig());
    EXPECT_EQ(bridge.callState(id), CallState::ACTIVE);
}

TEST(TelephonyBridge, CallStateReturnsIdleForUnknownCall) {
    TelephonyBridge bridge;
    EXPECT_EQ(bridge.callState("ghost-call"), CallState::IDLE);
}

TEST(TelephonyBridge, RouteIceCandidateDoesNotThrow) {
    TelephonyBridge bridge;
    CallID id;
    bridge.acceptWebRtcOffer(makeWebRtcConfig(), minimalSdpOffer(), id);
    EXPECT_NO_THROW(bridge.routeIceCandidate(
        id, R"({"candidate":"candidate:0 1 UDP 2122252543 192.168.1.1 9 typ host",)"
            R"("sdpMid":"audio","sdpMLineIndex":0})"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread safety smoke test
// ─────────────────────────────────────────────────────────────────────────────

TEST(TelephonyBridge, ConcurrentSipCallsAreThreadSafe) {
    TelephonyBridge bridge;
    constexpr int N = 20;
    std::vector<CallID> ids(N);

    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&bridge, &ids, i]() {
            ids[i] = bridge.acceptSipCall(makeSipConfig());
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    int active = 0;
    for (const auto& id : ids)
        if (!id.empty()) {
          ++active;
        }
    EXPECT_EQ(active, static_cast<int>(bridge.activeSipCallCount()));
}

} // namespace
} // namespace voice
} // namespace themis
