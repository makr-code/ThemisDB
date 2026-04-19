# Voice Module - Header Interfaces

## Module Purpose

The Voice module headers define the interface contracts for voice/audio interaction capabilities in ThemisDB. These headers provide the public API for integrating natural language voice commands, speech-to-text transcription, text-to-speech synthesis, phone call recording, meeting protocol generation, and voice-based analytics.

## Scope

**In Scope:**
- Voice assistant interface definitions
- Session and conversation management structures
- Phone call and meeting metadata structures
- Configuration structures for STT/TTS/LLM components
- Voice command processing interfaces
- Audio format and storage interfaces

**Out of Scope:**
- Concrete STT/TTS processor implementations (see `content/` module)
- LLM inference engine implementations (see `llm/` module)
- Audio codec implementations (external libraries)
- Network protocol implementations
- Database storage implementations

## Header Files

### voice_assistant.h
**Location:** `/include/voice/voice_assistant.h`

Core interface for the Voice Assistant system that integrates STT, TTS, and LLM capabilities.

**Key Components:**

#### 1. VoiceSession Structure

Represents an ongoing voice interaction session with conversation history.

```cpp
struct VoiceSession {
    std::string session_id;           // Unique session identifier
    std::string user_id;              // Associated user ID
    int64_t created_at;               // Session creation timestamp (ms)
    int64_t last_activity;            // Last interaction timestamp (ms)
    json context;                     // Conversation context (JSON object)
    std::vector<std::string> history; // Conversation history (alternating User/Assistant)
};
```

**Design Rationale:**
- `session_id`: Enables session continuity across multiple interactions
- `user_id`: Links sessions to user profiles for personalization
- `created_at` / `last_activity`: Support session timeout and analytics
- `context`: Flexible JSON structure for arbitrary session state
- `history`: Full conversation log for context-aware responses

**Usage Pattern:**
```cpp
// Create new session
VoiceSession session;
session.session_id = generate_session_id();
session.user_id = "user123";
session.created_at = get_timestamp_ms();
session.last_activity = session.created_at;
session.context = {{"language", "en"}, {"topic", "sales"}};

// Add to history
session.history.push_back("User: Show me last month's sales");
session.history.push_back("Assistant: Total sales were $2.3M");
```

**Session Management:**
- Sessions persist across multiple voice commands
- Context maintained for conversational coherence
- History enables "what did I ask about earlier?" queries
- Timeout can be implemented based on `last_activity`

#### 2. PhoneCallMetadata Structure

Metadata for phone call recordings.

```cpp
struct PhoneCallMetadata {
    std::string call_id;              // Unique call identifier
    std::string caller_number;        // Caller phone number (E.164 format)
    std::string callee_number;        // Callee phone number (E.164 format)
    int64_t start_time;               // Call start timestamp (ms since epoch)
    int64_t end_time;                 // Call end timestamp (ms since epoch)
    int64_t duration_ms;              // Call duration in milliseconds
    std::string call_type;            // "inbound", "outbound", "conference"
    json custom_fields;               // Additional custom metadata (JSON)
};
```

**Design Rationale:**
- Standard telephony metadata fields
- Flexible `custom_fields` for industry-specific data
- Supports all call types (inbound, outbound, conference)
- E.164 format for international phone numbers
- Millisecond precision for accurate billing/analytics

**Usage Pattern:**
```cpp
PhoneCallMetadata metadata;
metadata.call_id = "call_20260115_001234";
metadata.caller_number = "+14155552671";  // E.164 format
metadata.callee_number = "+14155558900";
metadata.start_time = 1736899200000;      // Unix timestamp ms
metadata.end_time = 1736899380000;        // 3 minutes later
metadata.duration_ms = metadata.end_time - metadata.start_time;
metadata.call_type = "inbound";
metadata.custom_fields = {
    {"department", "sales"},
    {"campaign_id", "Q1_2026"},
    {"customer_id", "CUST_789"}
};
```

**Common Use Cases:**
- Customer service call logging
- Sales call tracking
- Compliance recording (finance, healthcare)
- Call center analytics
- Quality assurance monitoring

#### 3. MeetingMetadata Structure

Metadata for meeting recordings.

```cpp
struct MeetingMetadata {
    std::string meeting_id;           // Unique meeting identifier
    std::string title;                // Meeting title/subject
    int64_t start_time;               // Meeting start timestamp (ms)
    int64_t end_time;                 // Meeting end timestamp (ms)
    std::vector<std::string> participants; // Participant identifiers (email/ID)
    std::string organizer;            // Meeting organizer identifier
    json custom_fields;               // Additional custom metadata (JSON)
};
```

**Design Rationale:**
- Calendar-compatible structure
- Supports multiple participants
- Flexible participant identifiers (email, username, ID)
- Custom fields for meeting type, room, project, etc.

**Usage Pattern:**
```cpp
MeetingMetadata metadata;
metadata.meeting_id = "mtg_20260115_quarterly_review";
metadata.title = "Q4 2025 Business Review";
metadata.start_time = 1736942400000;  // Meeting start
metadata.end_time = 1736946000000;    // 1 hour later
metadata.participants = {
    "alice@example.com",
    "bob@example.com",
    "carol@example.com",
    "david@example.com"
};
metadata.organizer = "alice@example.com";
metadata.custom_fields = {
    {"meeting_type", "quarterly_review"},
    {"room", "Conference Room A"},
    {"project", "Q4_2025_Review"},
    {"recording_consent", true}
};
```

**Meeting Types:**
- Corporate meetings (board, team, project)
- Client meetings
- Training sessions
- Interviews
- Presentations

#### 4. VoiceAssistant::Config Structure

Configuration for Voice Assistant components.

```cpp
struct Config {
    // STT (Speech-to-Text) Configuration
    std::string stt_model_path;       // Path to Whisper model file
    std::string stt_model_size;       // Model size: "tiny", "base", "small", "medium", "large"
    std::string stt_language;         // Language code or "auto" for detection

    // TTS (Text-to-Speech) Configuration
    std::string tts_model_path;       // Path to TTS model file
    std::string tts_voice;            // Voice ID from TTS model
    float tts_speed;                  // Speech rate (0.5 - 2.0, default 1.0)

    // LLM Configuration
    std::string llm_model_path;       // Path to LLM model (GGUF format)
    int llm_n_ctx;                    // Context window size (default 4096)
    int llm_n_gpu_layers;             // GPU layers (0 = CPU only)

    // Storage Configuration
    std::string storage_path;         // Base path for voice data storage
    bool enable_revision_control;     // Enable revision history (default true)
    bool compress_audio;              // Compress stored audio (default true)
    std::string audio_format;         // Storage format: "ogg", "mp3", "mp4", "wav"
};
```

**Design Rationale:**
- Grouped by component (STT, TTS, LLM, Storage)
- Sensible defaults for common use cases
- Flexible storage options (compressed vs. uncompressed)
- GPU acceleration support for LLM
- Multiple audio format support

**Configuration Examples:**

**Real-Time Configuration (Low Latency):**
```cpp
VoiceAssistant::Config config;
config.stt_model_path = "/models/whisper-tiny.bin";
config.stt_model_size = "tiny";
config.stt_language = "en";
config.tts_model_path = "/models/tts-en-fast.bin";
config.tts_voice = "default";
config.tts_speed = 1.2f;  // Slightly faster
config.llm_model_path = "/models/mistral-7b-instruct-q4.gguf";
config.llm_n_ctx = 2048;
config.llm_n_gpu_layers = 32;  // Use GPU
config.storage_path = "/var/themisdb/voice/";
config.enable_revision_control = false;  // Skip for speed
config.compress_audio = true;
config.audio_format = "ogg";
```

**High-Quality Configuration (Offline Processing):**
```cpp
VoiceAssistant::Config config;
config.stt_model_path = "/models/whisper-large-v3.bin";
config.stt_model_size = "large";
config.stt_language = "auto";  // Auto-detect
config.tts_model_path = "/models/tts-en-hq.bin";
config.tts_voice = "professional";
config.tts_speed = 1.0f;
config.llm_model_path = "/models/mistral-7b-instruct.gguf";
config.llm_n_ctx = 8192;  // Larger context
config.llm_n_gpu_layers = 48;
config.storage_path = "/archive/voice/";
config.enable_revision_control = true;
config.compress_audio = false;  // Keep uncompressed
config.audio_format = "wav";
```

**Multi-Language Configuration:**
```cpp
VoiceAssistant::Config config;
config.stt_model_path = "/models/whisper-medium-multilingual.bin";
config.stt_model_size = "medium";
config.stt_language = "auto";  // Auto-detect language
config.tts_model_path = "/models/tts-multilingual.bin";
config.tts_voice = "neutral";
config.tts_speed = 1.0f;
config.llm_model_path = "/models/mistral-7b-instruct-multilingual.gguf";
config.llm_n_ctx = 4096;
config.llm_n_gpu_layers = 0;  // CPU for broader compatibility
config.storage_path = "/var/themisdb/voice/";
config.enable_revision_control = true;
config.compress_audio = true;
config.audio_format = "opus";  // Best for multi-language
```

#### 5. VoiceAssistant Class

Main interface for voice interaction capabilities.

```cpp
class VoiceAssistant {
public:
    VoiceAssistant(const Config& config);
    ~VoiceAssistant();

    // Lifecycle management
    bool initialize();
    void shutdown();

    // Voice command processing
    std::vector<uint8_t> processVoiceCommand(
        const std::vector<uint8_t>& audio_data,
        const std::string& session_id
    );

    std::string processTextCommand(
        const std::string& text,
        const std::string& session_id
    );

    // Phone call recording
    json recordPhoneCall(
        const std::vector<uint8_t>& audio_data,
        const PhoneCallMetadata& metadata
    );

    // Meeting protocol generation
    json generateMeetingProtocol(
        const std::vector<uint8_t>& audio_data,
        const MeetingMetadata& metadata
    );

    // Audio utilities
    std::vector<uint8_t> convertAudioFormat(
        const std::vector<uint8_t>& audio_data,
        const std::string& target_format
    );

    // Storage
    std::string storeRecording(
        const std::vector<uint8_t>& audio_data,
        const std::string& transcript,
        const json& metadata
    );

    // Session management
    VoiceSession getSession(const std::string& session_id);
    void updateSession(const std::string& session_id, const json& context);

    // Statistics
    json getStatistics() const;
};
```

**Method Details:**

**initialize()**
- Initializes all sub-components (STT, TTS, LLM)
- Loads models into memory
- Prepares resources for processing
- Returns true on success, false on failure

**processVoiceCommand()**
- **Input**: Raw audio data (WAV format recommended), session ID
- **Output**: Audio response (WAV format)
- **Flow**: Audio → STT → LLM → TTS → Audio
- **Use Case**: Interactive voice assistant

**processTextCommand()**
- **Input**: Text command, session ID
- **Output**: Text response
- **Flow**: Text → LLM → Text
- **Use Case**: Testing, API integration, accessibility

**recordPhoneCall()**
- **Input**: Audio recording, call metadata
- **Output**: JSON with transcript, summary, segments, document ID
- **Features**: Speaker diarization, timestamps, summarization
- **Use Case**: Call center, compliance recording

**generateMeetingProtocol()**
- **Input**: Audio recording, meeting metadata
- **Output**: JSON with protocol, action items, decisions
- **Features**: Speaker identification, key point extraction, action item detection
- **Use Case**: Meeting minutes, project management

**convertAudioFormat()**
- **Input**: Audio data, target format
- **Output**: Converted audio data
- **Formats**: wav, ogg, mp3, mp4
- **Use Case**: Format standardization, compression

**storeRecording()**
- **Input**: Audio data, transcript, metadata
- **Output**: Document ID in ThemisDB
- **Features**: Revision control, compression, indexing
- **Use Case**: Long-term storage, searchable archives

## API Usage Patterns

### Pattern 1: Simple Voice Query

```cpp
#include "voice/voice_assistant.h"

// Initialize assistant
VoiceAssistant::Config config;
config.stt_model_path = "/models/whisper-base.bin";
config.tts_model_path = "/models/tts-en.bin";
config.llm_model_path = "/models/mistral-7b.gguf";

VoiceAssistant assistant(config);
if (!assistant.initialize()) {
    std::cerr << "Initialization failed" << std::endl;
    return 1;
}

// Process voice command
std::vector<uint8_t> audio_input = load_audio_file("user_query.wav");
std::string session_id = "user123_session1";

std::vector<uint8_t> audio_response =
    assistant.processVoiceCommand(audio_input, session_id);

// Save or play response
save_audio_file("assistant_response.wav", audio_response);
```

### Pattern 2: Phone Call Processing

```cpp
#include "voice/voice_assistant.h"

// Record phone call
PhoneCallMetadata metadata;
metadata.call_id = generate_call_id();
metadata.caller_number = "+14155552671";
metadata.callee_number = "+14155558900";
metadata.call_type = "inbound";
metadata.start_time = get_timestamp_ms();

// Get audio from telephony system
std::vector<uint8_t> call_recording = get_call_audio();

metadata.end_time = get_timestamp_ms();
metadata.duration_ms = metadata.end_time - metadata.start_time;

// Process call
json result = assistant.recordPhoneCall(call_recording, metadata);

// Extract information
if (result["success"].get<bool>()) {
    std::string transcript = result["transcript"];
    std::string summary = result["summary"];
    std::string doc_id = result["document_id"];

    // Update CRM
    update_crm(metadata.caller_number, {
        {"last_call_id", doc_id},
        {"last_call_summary", summary}
    });
}
```

### Pattern 3: Meeting Protocol

```cpp
#include "voice/voice_assistant.h"

// Generate meeting protocol
MeetingMetadata metadata;
metadata.meeting_id = "mtg_" + generate_id();
metadata.title = "Sprint Planning";
metadata.participants = {
    "alice@example.com",
    "bob@example.com",
    "carol@example.com"
};
metadata.organizer = "alice@example.com";
metadata.start_time = get_timestamp_ms();

// Record meeting
std::vector<uint8_t> meeting_audio = record_meeting();

metadata.end_time = get_timestamp_ms();

// Generate protocol
json protocol = assistant.generateMeetingProtocol(meeting_audio, metadata);

// Extract information
std::string transcript = protocol["transcript"]["full_text"];
json action_items = protocol["action_items"];
json decisions = protocol["decisions"];

// Send meeting minutes
email_participants(metadata.participants, protocol);

// Create tasks from action items
for (const auto& item : action_items) {
    create_task(
        item["assignee"],
        item["description"],
        item["due_date"]
    );
}
```

### Pattern 4: Conversational AI

```cpp
#include "voice/voice_assistant.h"

// Multi-turn conversation
VoiceAssistant assistant(config);
assistant.initialize();

std::string session_id = generate_session_id();

// Turn 1
std::string response1 = assistant.processTextCommand(
    "What were our sales last month?",
    session_id
);
std::cout << "Assistant: " << response1 << std::endl;
// Output: "Sales last month were $2.3 million, up 15% from November."

// Turn 2 (context-aware)
std::string response2 = assistant.processTextCommand(
    "What about the month before?",
    session_id
);
std::cout << "Assistant: " << response2 << std::endl;
// Output: "In November, sales were $2.0 million."

// Turn 3 (follow-up)
std::string response3 = assistant.processTextCommand(
    "Which products contributed most?",
    session_id
);
std::cout << "Assistant: " << response3 << std::endl;
// Output: "Top 3 products: Product A ($450K), Product B ($380K), Product C ($320K)."
```

## Threading and Concurrency

### Thread Safety

The `VoiceAssistant` class uses internal mutexes for session management:

```cpp
// Thread-safe session operations
std::map<std::string, VoiceSession> sessions_;
std::mutex sessions_mutex_;
```

**Safe Operations:**
- Multiple threads can process different sessions concurrently
- Session creation and updates are synchronized
- Statistics retrieval is thread-safe

**Unsafe Operations:**
- Single VoiceAssistant instance should not process same session from multiple threads
- Model initialization is not thread-safe (call `initialize()` once)

**Recommended Pattern:**
```cpp
// Single assistant, multiple sessions
VoiceAssistant assistant(config);
assistant.initialize();

// Thread pool for concurrent processing
ThreadPool pool(4);

for (const auto& request : incoming_requests) {
    pool.submit([&]() {
        auto response = assistant.processVoiceCommand(
            request.audio,
            request.session_id  // Different session per thread
        );
        send_response(request.client_id, response);
    });
}
```

## Memory Management

### Resource Usage

**STT Model:**
- Tiny: ~150 MB RAM
- Base: ~300 MB RAM
- Small: ~1 GB RAM
- Medium: ~3 GB RAM
- Large: ~6 GB RAM

**TTS Model:**
- Typical: 100-500 MB RAM

**LLM Model:**
- 7B parameter (Q4): ~4 GB RAM
- 13B parameter (Q4): ~8 GB RAM
- 70B parameter (Q4): ~40 GB RAM

**Audio Buffers:**
- Input buffer: ~10 MB (60 seconds @ 16kHz mono)
- Output buffer: ~10 MB
- Working memory: ~50 MB

**Total Minimum:**
- Base configuration: ~5 GB RAM
- Recommended: 8+ GB RAM
- High-performance: 16+ GB RAM with GPU

### Memory Optimization Tips

```cpp
// Use smaller models for limited memory
config.stt_model_size = "tiny";
config.llm_n_ctx = 2048;  // Smaller context

// Enable GPU offloading to reduce RAM
config.llm_n_gpu_layers = 32;

// Process in batches for large files
std::vector<uint8_t> chunk = get_audio_chunk(60000);  // 60 seconds
auto result = assistant.processVoiceCommand(chunk, session_id);
```

## Error Handling

All major operations return status information:

**Success Example:**
```json
{
  "success": true,
  "call_id": "call_123",
  "transcript": "...",
  "document_id": "doc_456"
}
```

**Error Example:**
```json
{
  "success": false,
  "error": "STT transcription failed",
  "error_code": "STT_ERROR"
}
```

**Error Handling Pattern:**
```cpp
json result = assistant.recordPhoneCall(audio, metadata);

if (!result["success"].get<bool>()) {
    std::string error = result["error"];
    std::cerr << "Call recording failed: " << error << std::endl;

    // Handle specific errors
    if (result.contains("error_code")) {
        std::string code = result["error_code"];
        if (code == "STT_ERROR") {
            // Retry with different settings
        } else if (code == "STORAGE_ERROR") {
            // Check disk space
        }
    }

    return false;
}

// Success - process result
std::string doc_id = result["document_id"];
```

## Performance Considerations

### Latency Breakdown

**processVoiceCommand():**
- STT: 100-500ms (model dependent)
- LLM: 50-200ms (model + query dependent)
- TTS: 50-100ms
- **Total**: 200-800ms

**recordPhoneCall():**
- STT with diarization: 1-3x real-time
- LLM summarization: 100-500ms
- Storage: 50-200ms
- **Total**: Proportional to audio length

**generateMeetingProtocol():**
- STT with diarization: 1-3x real-time
- LLM analysis: 500-2000ms
- Action item extraction: 200-500ms
- **Total**: Proportional to audio length + 1-3 seconds

### Throughput Optimization

**Use GPU Acceleration:**
```cpp
config.llm_n_gpu_layers = 32;  // Offload to GPU
```

**Batch Processing:**
```cpp
// Process multiple calls in parallel
std::vector<std::future<json>> futures;
for (const auto& call : call_batch) {
    futures.push_back(std::async([&]() {
        return assistant.recordPhoneCall(call.audio, call.metadata);
    }));
}
```

**Model Selection:**
- Real-time: Use tiny/base models
- Batch: Use medium/large models
- Balance: Use small model with GPU

## Dependencies

### Internal Dependencies
- `content::STTProcessor` - Speech-to-text processing
- `content::TTSProcessor` - Text-to-speech synthesis
- `llm::LlamaWrapper` - LLM inference wrapper
- `nlohmann::json` - JSON data structures

### External Dependencies
- Whisper.cpp - STT inference
- llama.cpp - LLM inference
- Audio codecs (libvorbis, libmp3lame, libopus)

## Version Compatibility

**Current Version:** v1.0.0

**Compatibility:**
- Whisper.cpp: v1.5.0+
- llama.cpp: v0.2.0+
- ThemisDB: v1.5.0+

**Breaking Changes:**
- None in v1.0.0

**Planned Breaking Changes:**
- v2.0.0: Async API will replace some synchronous methods

## Testing

### Unit Test Coverage
- Session management: 95%
- Audio format conversion: 90%
- Configuration validation: 100%
- Error handling: 85%

### Integration Tests
- End-to-end voice command: Yes
- Phone call recording: Yes
- Meeting protocol: Yes
- Multi-session handling: Yes

### Test Utilities

```cpp
// Test with mock audio
#include "voice/voice_assistant.h"
#include "tests/voice/mock_audio.h"

auto test_audio = MockAudio::generate_speech("Hello ThemisDB", "en");
auto response = assistant.processVoiceCommand(test_audio, "test_session");
assert(!response.empty());
```

## See Also

- [Voice Module Source](../../src/voice/README.md) - Implementation documentation
- [Content Module](../content/README.md) - STT/TTS processors
- [LLM Module](../llm/README.md) - Language model integration
- [API Documentation](../../docs/api/voice_api.md) - REST API reference
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features

---

*Last Updated: April 2026*
*Module Version: v1.0.0*
*Next Review: v1.1.0 Release*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
