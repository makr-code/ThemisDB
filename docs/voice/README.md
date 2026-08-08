# ThemisDB Voice Module - User Guide & API Reference

**Version:** v1.0-production  
**Last Updated:** 2026-08-08  
**Status:** Production Ready  
**Maturity Level:** 🟢 PRODUCTION-READY

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Core Concepts](#core-concepts)
3. [API Usage Examples](#api-usage-examples)
4. [Integration Paths](#integration-paths)
5. [Troubleshooting & FAQ](#troubleshooting--faq)
6. [Security & Best Practices](#security--best-practices)
7. [Performance Tuning](#performance-tuning)

---

## Quick Start

### 3-Step Guide to Your First Voice Command

```cpp
#include "voice/voice_session_manager.h"
#include "voice/voice_assistant.h"
#include <iostream>

using namespace themis::voice;

int main() {
    // Step 1: Create a session manager and new session
    VoiceSessionManager session_mgr;
    auto session = session_mgr.createSession("user123", "device-001");
    
    if (session.session_id.empty()) {
        std::cerr << "Failed to create session\n";
        return 1;
    }
    
    // Step 2: Create voice assistant and process a command
    VoiceAssistant assistant;
    std::string command = "what is the weather";
    
    auto response = assistant.processCommand(
        session.session_id,
        command,
        "en"  // language code
    );
    
    // Step 3: Get and display response
    std::cout << "Command: " << command << "\n";
    std::cout << "Response: " << response.response_text << "\n";
    std::cout << "Confidence: " << response.confidence << "%\n";
    
    return 0;
}
```

**Expected Output:**
```
Command: what is the weather
Response: I don't have real-time weather data, but you can check your local forecast.
Confidence: 87%
```

---

## Core Concepts

### Sessions

A **voice session** represents a continuous conversation between a user and ThemisDB Voice Assistant. Each session:
- Has a unique `session_id`
- Maintains conversation history and context
- Tracks user language preference
- Auto-expires after inactivity (default: 5 minutes)
- Is thread-safe for concurrent access

**Session Lifecycle:**
```
CREATE → ACTIVE → IDLE (on inactivity) → EXPIRED → TERMINATED
         ↑___________________|_________________↓
```

### Authentication

Voice authentication verifies the speaker's identity using:
- Biometric voice profiles (speaker verification)
- Liveness detection (anti-spoofing, prevents replay attacks)
- Multi-factor enrollment (3+ audio samples minimum)

### Streaming Audio

Real-time audio input via:
- **Browser WebSocket:** For web applications
- **Telephony (SIP/WebRTC):** For phone integration
- **File Upload:** For batch processing

### Intent Detection

Converts speech transcripts into structured intents:
- Natural language understanding (NLU)
- Command classification
- Parameter extraction
- Context-aware routing

---

## API Usage Examples

### Example 1: Basic Voice Session Creation and Command Processing

```cpp
#include "voice/voice_session_manager.h"
#include "voice/voice_assistant.h"
#include "voice/voice_error_handler.h"
#include <iostream>
#include <optional>

using namespace themis::voice;

int main() {
    try {
        // Initialize managers
        SessionTimeoutConfig config{
            .idle_timeout_ms = 5 * 60 * 1000,      // 5 minutes
            .max_session_duration_ms = 60 * 60 * 1000  // 1 hour
        };
        VoiceSessionManager session_mgr(config);
        VoiceAssistant assistant;
        
        // Create session
        auto session = session_mgr.createSession("alice@example.com", "browser-001");
        if (session.session_id.empty()) {
            std::cerr << "Session creation failed\n";
            return 1;
        }
        
        std::cout << "✓ Created session: " << session.session_id << "\n";
        std::cout << "  User: " << session.user_id << "\n";
        std::cout << "  Created at: " << session.created_at_ms << " ms\n";
        
        // Process a command
        auto cmd_result = assistant.processCommand(
            session.session_id,
            "remind me to call mom",
            "en"
        );
        
        std::cout << "✓ Processed command\n";
        std::cout << "  Intent: " << cmd_result.intent << "\n";
        std::cout << "  Response: " << cmd_result.response_text << "\n";
        std::cout << "  Confidence: " << cmd_result.confidence << "%\n";
        
        // Update session context
        json context_update;
        context_update["last_command"] = "remind me to call mom";
        context_update["device_type"] = "browser";
        
        if (session_mgr.updateSession(session.session_id, context_update)) {
            std::cout << "✓ Session context updated\n";
        }
        
    } catch (const VoiceException& e) {
        std::cerr << "Voice error: " << e.what() << " (code: " << static_cast<int>(e.code()) << ")\n";
        return 1;
    }
    
    return 0;
}
```

### Example 2: Streaming Audio Input (Browser WebSocket)

```cpp
#include "voice/voice_browser_streaming.h"
#include "voice/voice_session_manager.h"
#include <iostream>
#include <vector>

using namespace themis::voice;

int main() {
    // Initialize streaming manager
    VoiceStreamingManager streaming_mgr;
    
    // Callbacks for transcript results
    auto on_partial = [](const std::string& partial_text) {
        std::cout << "[Partial] " << partial_text << "\n";
    };
    
    auto on_final = [](const TranscriptResult& result) {
        std::cout << "[Final] " << result.full_text << " (confidence: " 
                  << result.confidence << "%)\n";
    };
    
    auto on_error = [](VoiceErrorCode error, const std::string& msg) {
        std::cerr << "[Error] " << msg << "\n";
    };
    
    // Create streaming session
    StreamID stream_id = streaming_mgr.createStream(
        "user456",
        "audio/wav",  // codec
        on_partial,
        on_final,
        on_error
    );
    
    if (stream_id.empty()) {
        std::cerr << "Failed to create stream\n";
        return 1;
    }
    
    std::cout << "✓ Created stream: " << stream_id << "\n";
    
    // Simulate audio frame capture from microphone
    std::vector<uint8_t> audio_frame(4096);  // 16kHz, 16-bit PCM, ~0.128s
    
    // Send audio frames to stream
    for (int i = 0; i < 5; ++i) {
        bool sent = streaming_mgr.sendAudioFrame(
            stream_id,
            audio_frame.data(),
            audio_frame.size(),
            16000,  // sample rate Hz
            16      // bits per sample
        );
        
        if (!sent) {
            std::cerr << "Failed to send audio frame " << i << "\n";
            break;
        }
        
        std::cout << "  Sent frame " << (i+1) << "\n";
    }
    
    // Close stream when done
    streaming_mgr.closeStream(stream_id);
    std::cout << "✓ Closed stream\n";
    
    return 0;
}
```

### Example 3: Telephony Integration (SIP/WebRTC)

```cpp
#include "voice/voice_telephony.h"
#include "voice/voice_session_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace themis::voice;

int main() {
    VoiceTelephonyManager telephony_mgr;
    VoiceSessionManager session_mgr;
    
    // Set up callbacks
    auto on_incoming = [&](const CallMetadata& call) {
        std::cout << "📞 Incoming call from: " << call.caller_number << "\n";
    };
    
    auto on_transcript_ready = [](const std::string& call_id, const std::string& transcript) {
        std::cout << "📝 Transcript ready for call " << call_id << ":\n";
        std::cout << "   " << transcript << "\n";
    };
    
    // Initialize telephony
    TelephonyConfig config{
        .protocol = "webrtc",
        .transport = "dtls-srtp",
        .enable_recording = true,
        .auto_transcribe = true
    };
    
    if (!telephony_mgr.initialize(config)) {
        std::cerr << "Failed to initialize telephony\n";
        return 1;
    }
    
    std::cout << "✓ Telephony initialized (WebRTC)\n";
    
    // Register callbacks
    telephony_mgr.onIncomingCall(on_incoming);
    telephony_mgr.onTranscriptReady(on_transcript_ready);
    
    std::cout << "Waiting for calls... (demo mode)\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    telephony_mgr.shutdown();
    std::cout << "✓ Telephony shutdown\n";
    
    return 0;
}
```

### Example 4: Error Handling and Retry Logic

```cpp
#include "voice/voice_assistant.h"
#include "voice/voice_error_handler.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace themis::voice;

struct RetryPolicy {
    int max_attempts = 3;
    std::chrono::milliseconds initial_backoff{100};
};

bool processWithRetry(
    VoiceAssistant& assistant,
    const std::string& session_id,
    const std::string& command,
    const RetryPolicy& policy
) {
    int attempt = 0;
    auto backoff = policy.initial_backoff;
    
    while (attempt < policy.max_attempts) {
        try {
            auto result = assistant.processCommand(session_id, command, "en");
            std::cout << "✓ Command succeeded: " << result.response_text << "\n";
            return true;
            
        } catch (const VoiceException& e) {
            attempt++;
            std::cerr << "Attempt " << attempt << " failed: " << e.what() << "\n";
            
            bool retryable = false;
            switch (e.code()) {
                case VoiceErrorCode::TIMEOUT:
                case VoiceErrorCode::NETWORK_ERROR:
                case VoiceErrorCode::STT_FAILED:
                    retryable = true;
                    break;
                default:
                    retryable = (attempt < policy.max_attempts);
            }
            
            if (!retryable || attempt >= policy.max_attempts) {
                std::cerr << "✗ Command failed after " << attempt << " attempts\n";
                return false;
            }
            
            std::cout << "  Retrying in " << backoff.count() << "ms...\n";
            std::this_thread::sleep_for(backoff);
            backoff *= 2;
        }
    }
    
    return false;
}

int main() {
    VoiceAssistant assistant;
    auto session = createSession("user789", "device-002");
    
    RetryPolicy policy{
        .max_attempts = 3,
        .initial_backoff = std::chrono::milliseconds(100)
    };
    
    if (processWithRetry(assistant, session.session_id, "play music", policy)) {
        std::cout << "✓ Successfully processed command\n";
    }
    
    return 0;
}
```

### Example 5: Concurrent Sessions

```cpp
#include "voice/voice_session_manager.h"
#include "voice/voice_assistant.h"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

using namespace themis::voice;

std::mutex output_mutex;

void processSessionInThread(
    VoiceSessionManager& session_mgr,
    VoiceAssistant& assistant,
    int session_num
) {
    std::string user_id = "user" + std::to_string(session_num);
    std::string device_id = "device" + std::to_string(session_num);
    
    auto session = session_mgr.createSession(user_id, device_id);
    
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "[Thread " << session_num << "] Created session: " 
                  << session.session_id << "\n";
    }
    
    for (int cmd = 1; cmd <= 3; ++cmd) {
        std::string command = "command number " + std::to_string(cmd);
        
        auto result = assistant.processCommand(
            session.session_id,
            command,
            "en"
        );
        
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[Thread " << session_num << "] Command " << cmd 
                      << ": " << result.intent << "\n";
        }
        
        session_mgr.touchSession(session.session_id);
    }
    
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "[Thread " << session_num << "] Session complete\n";
    }
}

int main() {
    VoiceSessionManager session_mgr;
    VoiceAssistant assistant;
    
    const int NUM_SESSIONS = 5;
    std::vector<std::thread> threads;
    
    std::cout << "Starting " << NUM_SESSIONS << " concurrent sessions...\n\n";
    
    for (int i = 1; i <= NUM_SESSIONS; ++i) {
        threads.emplace_back([&](int thread_num) {
            processSessionInThread(session_mgr, assistant, thread_num);
        }, i);
    }
    
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    auto analytics = session_mgr.getAnalytics();
    
    std::cout << "\n✓ All sessions complete\n";
    std::cout << "Session Statistics:\n";
    std::cout << "  Total sessions: " << analytics.total_sessions << "\n";
    std::cout << "  Active sessions: " << analytics.active_sessions << "\n";
    
    return 0;
}
```

---

## Integration Paths

### Path 1: Browser Web Application
- WebSocket real-time audio streaming
- Browser audio APIs for capture/playback
- Voice command processing with immediate feedback

### Path 2: Mobile Application
- Native audio APIs (iOS/Android)
- HTTP/WebSocket transport
- Battery-efficient audio buffering

### Path 3: Telephony (SIP/WebRTC)
- SIP signaling or WebRTC media
- Real-time transcription and recording
- Call recording with audit trail

### Path 4: CLI / Terminal
- Standard input/output
- File-based audio input
- Synchronous command processing

---

## Troubleshooting & FAQ

### Q: Session expires too quickly
**A:** Adjust session timeout configuration:
```cpp
SessionTimeoutConfig config{
    .idle_timeout_ms = 10 * 60 * 1000,  // 10 minutes
    .max_session_duration_ms = 2 * 60 * 60 * 1000  // 2 hours
};
```

### Q: Streaming audio is choppy
**A:** Increase stream buffer size and chunk timeout

### Q: How do I authenticate a user?
**A:** Use VoiceBiometricAuthenticator with voice biometrics

### Q: Transcript contains sensitive data
**A:** Use VoiceSecurityManager::redactPII() for automatic redaction

### Q: Enable recording and audit logging
**A:** Configure VoiceSecurityManager with recording consent

---

## Security & Best Practices

### Authentication
✅ Always authenticate users before creating sessions  
✅ Use biometric voice auth for sensitive operations  
❌ Never create sessions without authentication

### Audio Data
✅ Use TLS/DTLS for transport  
✅ Encrypt recordings (AES-256)  
❌ Never store audio in plain text

### Consent & Privacy
✅ Obtain explicit recording consent  
✅ Offer PII redaction by default  
✅ Honor GDPR/CCPA deletion requests  
❌ Never record without consent

### Error Handling
✅ Implement retry logic with backoff  
✅ Never expose stack traces to users  
❌ Don't log authentication tokens

---

## Performance Tuning

### Streaming Performance
- Optimal chunk size: 4096 bytes
- Concurrent streams: 100+
- Latency: 200-500ms (audio→transcript)

### Session Management
- Cleanup interval: 30s
- Idle timeout: 5 minutes
- Max session duration: 1 hour

### Model Caching
- Preload models at startup
- Cache size optimized for target hardware
- LRU eviction policy

---

## Additional Resources

- **[API Documentation](../../../include/voice/)** - Doxygen API reference
- **[Architecture Guide](../../voice/ARCHITECTURE.md)** - System design and components
- **[Production Requirements](../../voice/PRODUCTION_REQUIREMENTS.md)** - Deployment checklist
- **[Changelog](../../voice/CHANGELOG.md)** - Version history and breaking changes
- **[Roadmap](../../voice/ROADMAP.md)** - Feature planning and milestones

---

**Document Version:** v1.0-production (frozen 2026-08-08)  
**Level:** 4 (Public Documentation)  
**Source:** `docs/voice/README.md`  
**References:** Level 1: `src/voice/README.md`, `src/voice/ARCHITECTURE.md`
