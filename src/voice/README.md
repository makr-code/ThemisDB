# Voice Module

Voice/audio interface capabilities for natural language interaction with ThemisDB.

## Module Purpose

Implements a production-ready voice assistant stack for ThemisDB: speech-to-text transcription, natural language query parsing, AQL query generation, TTS response generation, session handling, and streaming voice interaction.

## Subsystem Scope

**In scope:** Speech-to-text integration (Whisper), voice-to-AQL query generation, TTS response synthesis, audio preprocessing, voice session management, and browser streaming integration.

**Out of scope:** Audio recording hardware drivers and model training/fine-tuning pipelines.

## Relevant Interfaces

- `voice_assistant.cpp` — end-to-end orchestration (STT/LLM/TTS/session)
- `audio_preprocessing.cpp` — audio cleanup and normalization pipeline
- `voice_intent_detector.cpp` — intent detection for voice commands
- `voice_session_manager.cpp` — voice session lifecycle and history

## Current Delivery Status

**Maturity:** 🟢 Production-Ready (v1.1.0) — VoiceAssistant orchestration, Whisper-based STT, llama.cpp-based LLM/TTS integration, phone-call transcription, meeting protocol generation, WebSocket streaming, and telephony bridge are implemented.

## Architecture Overview

The Voice module provides a comprehensive voice assistant system that combines Speech-to-Text (STT), Text-to-Speech (TTS), and Large Language Model (LLM) capabilities to enable natural language interaction with ThemisDB. It supports voice commands, phone call recording/transcription, meeting protocol generation, and voice-based analytics.

### Core Components

#### 1. VoiceAssistant (Main Orchestrator)
- **Purpose**: Central coordinator for all voice interaction functionality
- **Location**: `src/voice/voice_assistant.cpp`
- **Responsibilities**:
  - Manages STT/TTS/LLM integration
  - Handles session state and conversation history
  - Coordinates audio processing pipelines
  - Manages storage and retrieval of voice data
  - Provides voice-based database query interface

#### 2. LLM Integration
- **Purpose**: Natural language understanding and response generation
- **Location**: `src/voice/voice_assistant_llm.cpp`
- **Features**:
  - Uses EmbeddedLLM for unified llama.cpp integration
  - Conversational AI with context awareness
  - Query intent recognition
  - Response generation and formatting
  - Summary and key point extraction

### Integration Architecture

```
Voice Command Flow:
┌─────────────┐
│ Audio Input │
└──────┬──────┘
       │
       v
┌─────────────────┐
│  STT Processor  │ (content::STTProcessor)
│  - Whisper AI   │
│  - Diarization  │
│  - Timestamps   │
└────────┬────────┘
         │
         v
    ┌─────────┐
    │  Text   │
    └────┬────┘
         │
         v
┌──────────────────┐
│  LLM Wrapper     │ (llm::LlamaWrapper)
│  - Intent Parse  │
│  - Query Gen     │
│  - Response Gen  │
└────────┬─────────┘
         │
         v
    ┌──────────┐
    │ Response │
    └────┬─────┘
         │
         v
┌─────────────────┐
│  TTS Processor  │ (content::TTSProcessor)
│  - Voice Synth  │
│  - Audio Format │
└────────┬────────┘
         │
         v
┌──────────────┐
│ Audio Output │
└──────────────┘
```

## Features

### 1. Voice Command Processing
**Purpose**: Interactive voice-based database queries and commands

**Capabilities:**
- Natural language query interpretation
- Multi-turn conversation support
- Context-aware responses
- Session management with history
- Voice authentication (speaker verification via STT)

**Example Interaction:**
```
User: "Show me sales data from last month"
Assistant: "I found 1,247 sales records from December 2025. 
            Total revenue was $2.3 million, up 15% from November."

User: "What were the top products?"
Assistant: "The top 3 products were: Product A with $450K, 
            Product B with $380K, and Product C with $320K."
```

**Implementation (schematic):**
```cpp
// Process voice command
VoiceAssistant assistant(config);
assistant.initialize();

std::vector<uint8_t> audio_input = record_from_microphone();
std::string session_id = "user123_session1";

std::vector<uint8_t> audio_response = 
    assistant.processVoiceCommand(audio_input, session_id);

play_audio(audio_response);
```

Note: The snippet is an architectural usage sketch; exact method signatures depend on the current `VoiceAssistant` API.

### 2. Phone Call Recording & Transcription
**Purpose**: Record, transcribe, and analyze phone conversations

**Features:**
- Real-time or batch transcription
- Speaker diarization (who spoke when)
- Timestamp alignment
- Automatic summarization
- Sentiment analysis integration
- PCI-compliant storage options

**Use Cases:**
- Customer service call logging
- Sales call analysis
- Compliance recording
- Quality assurance
- Call center analytics

**Implementation:**
```cpp
// Record phone call
PhoneCallMetadata metadata;
metadata.call_id = "call_20260115_001234";
metadata.caller_number = "+1-555-0100";
metadata.callee_number = "+1-555-0200";
metadata.call_type = "inbound";
metadata.start_time = get_timestamp();

std::vector<uint8_t> recording = get_call_audio();
metadata.end_time = get_timestamp();

json result = assistant.recordPhoneCall(recording, metadata);

// Result includes:
// - Full transcript with timestamps
// - Speaker identification
// - Call summary
// - Key points extracted
// - Document ID in ThemisDB
```

**Output Structure:**
```json
{
  "success": true,
  "call_id": "call_20260115_001234",
  "transcript": "Full conversation text...",
  "language": "en",
  "confidence": 0.92,
  "duration_ms": 180000,
  "metadata": {
    "caller": "+1-555-0100",
    "callee": "+1-555-0200",
    "call_type": "inbound"
  },
  "segments": [
    {
      "text": "Hello, how can I help you?",
      "start_ms": 0,
      "end_ms": 2500,
      "speaker": "Speaker 1",
      "confidence": 0.95
    }
  ],
  "summary": "Customer inquiry about product availability...",
  "document_id": "themisdb://calls/2026/01/15/001234"
}
```

### 3. Meeting Protocol Generation
**Purpose**: Generate structured meeting minutes from recordings

**Features:**
- Automatic transcript generation
- Speaker identification and labeling
- Key decision extraction
- Action item detection
- Attendee tracking
- Structured output formats

**Use Cases:**
- Corporate meeting minutes
- Board meeting records
- Project planning sessions
- Team retrospectives
- Client meetings

**Implementation:**
```cpp
// Generate meeting protocol
MeetingMetadata metadata;
metadata.meeting_id = "mtg_20260115_quarterly_review";
metadata.title = "Q4 2025 Business Review";
metadata.organizer = "alice@example.com";
metadata.participants = {
    "alice@example.com",
    "bob@example.com", 
    "carol@example.com"
};
metadata.start_time = get_timestamp();

std::vector<uint8_t> recording = get_meeting_audio();
metadata.end_time = get_timestamp();

json protocol = assistant.generateMeetingProtocol(recording, metadata);
```

**Protocol Structure:**
```json
{
  "meeting_id": "mtg_20260115_quarterly_review",
  "title": "Q4 2025 Business Review",
  "date": "2026-01-15",
  "duration_ms": 3600000,
  "organizer": "alice@example.com",
  "participants": [
    "alice@example.com",
    "bob@example.com",
    "carol@example.com"
  ],
  "transcript": {
    "segments": [/* detailed segments */]
  },
  "summary": "Team reviewed Q4 performance...",
  "key_points": [
    "Revenue exceeded target by 12%",
    "New product launch scheduled for Q2",
    "Three new hires approved"
  ],
  "action_items": [
    {
      "description": "Prepare Q1 budget proposal",
      "assignee": "bob@example.com",
      "due_date": "2026-01-30"
    }
  ],
  "decisions": [
    "Approved $500K marketing budget for new campaign"
  ]
}
```

### 4. Audio Data Storage & Retrieval
**Purpose**: Efficient storage and versioning of audio recordings

**Features:**
- Revision-safe storage in ThemisDB
- Audio format conversion (WAV, OGG, MP3, MP4)
- Compression support
- Metadata indexing
- Full-text search on transcripts
- Audio embedding generation (for similarity search)

**Storage Architecture:**
```cpp
// Store recording with revision control
std::string doc_id = assistant.storeRecording(
    audio_data,           // Raw or compressed audio
    transcript,           // Text transcription
    metadata             // Additional metadata
);

// Creates revision-controlled document:
// - Audio binary data (compressed if enabled)
// - Transcript text (searchable)
// - Metadata (caller, duration, language, etc.)
// - Timestamps and speaker labels
// - Generated summaries and key points
```

**Storage Options:**
```cpp
VoiceAssistant::Config config;
config.storage_path = "/var/themisdb/voice/";
config.enable_revision_control = true;  // Version history
config.compress_audio = true;           // Save storage space
config.audio_format = "ogg";            // Compressed format

// Supported formats:
// - "wav" : Uncompressed PCM (highest quality)
// - "ogg" : Ogg Vorbis (balanced)
// - "mp3" : MPEG Audio Layer 3 (widely compatible)
// - "mp4" : AAC in MP4 container (modern standard)
```

### 5. Voice-Based Analytics
**Purpose**: Extract insights from voice data at scale

**Capabilities:**
- Sentiment analysis on conversations
- Topic modeling and clustering
- Speaker behavior patterns
- Call volume analytics
- Conversation flow analysis
- Keyword trending
- Custom metric extraction

**Analytics Queries:**
```cpp
// Example: Analyze customer sentiment trends
std::string query = R"(
    FOR call IN voice_calls
        FILTER call.date >= @start_date
        LET sentiment = SENTIMENT_ANALYZE(call.transcript)
        COLLECT date = DATE_TRUNC(call.date, 'day')
        AGGREGATE avg_sentiment = AVG(sentiment.score)
        SORT date
        RETURN {date, avg_sentiment}
)";

// Example: Find common action items
std::string query = R"(
    FOR meeting IN voice_meetings
        FOR item IN meeting.action_items
        COLLECT description = item.description
        AGGREGATE count = LENGTH(item)
        SORT count DESC
        LIMIT 10
        RETURN {description, count}
)";
```

### 6. Natural Language Understanding (NLU)
**Purpose**: Parse and understand user intent from voice input

**NLU Pipeline:**
1. **Speech Recognition**: Audio → Text (STT)
2. **Intent Classification**: Text → Intent Category
3. **Entity Extraction**: Identify data elements (dates, names, numbers)
4. **Context Resolution**: Apply conversation history
5. **Query Generation**: Intent → Database Query (AQL)
6. **Response Formatting**: Results → Natural Language

**Intent Categories:**
- **Query Intent**: User wants data retrieval
- **Command Intent**: User wants to modify data
- **Question Intent**: User wants explanation/help
- **Conversation Intent**: Small talk or clarification

**Example NLU Flow:**
```
Input: "How many customers signed up last week?"

1. STT: Audio → "How many customers signed up last week?"
2. Intent: QUERY_INTENT (data retrieval)
3. Entities:
   - Metric: "count"
   - Object: "customers"  
   - Time: "last week"
   - Action: "signed up"
4. Context: None needed (standalone query)
5. Query Generation:
   FOR c IN customers
       FILTER c.signup_date >= DATE_SUBTRACT(DATE_NOW(), 1, 'week')
       COLLECT WITH COUNT INTO count
       RETURN count
6. Execute: Result = 147
7. Response: "147 customers signed up last week."
8. TTS: Text → Audio
```

## Configuration

### VoiceAssistant Configuration
```cpp
VoiceAssistant::Config config;

// STT (Speech-to-Text) Configuration
config.stt_model_path = "/models/whisper-base.bin";
config.stt_model_size = "base";  // tiny, base, small, medium, large
config.stt_language = "auto";     // auto-detect or specific: en, de, fr, etc.

// TTS (Text-to-Speech) Configuration
config.tts_model_path = "/models/tts-en.bin";
config.tts_voice = "default";     // Voice ID from TTS model
config.tts_speed = 1.0f;          // Speech rate (0.5 - 2.0)

// LLM Configuration
config.llm_model_path = "/models/mistral-7b-instruct.gguf";
config.llm_n_ctx = 4096;          // Context window size
config.llm_n_gpu_layers = 0;      // GPU acceleration (0 = CPU only)

// Storage Configuration
config.storage_path = "/var/themisdb/voice/";
config.enable_revision_control = true;
config.compress_audio = true;
config.audio_format = "ogg";
```

### Model Selection Guide

**STT Models (Whisper):**
- **tiny**: 39M params, fast, lower accuracy (~60% WER)
- **base**: 74M params, balanced, good accuracy (~45% WER)
- **small**: 244M params, slower, better accuracy (~35% WER)
- **medium**: 769M params, slow, high accuracy (~25% WER)
- **large**: 1550M params, very slow, highest accuracy (~20% WER)

**Recommended:**
- Real-time applications: tiny or base
- Offline processing: small or medium
- Maximum accuracy: large (requires GPU)

**TTS Models:**
- Configurable via content::TTSProcessor
- Supports multiple voices per model
- Adjustable speech rate and pitch

**LLM Models:**
- Must support instruction following
- Recommended: Mistral 7B, Llama 2 7B, or larger
- Context window ≥ 2048 tokens for conversation history

## API Reference

### VoiceAssistant Class

#### Constructor
```cpp
VoiceAssistant(const Config& config);
```
Create voice assistant with configuration.

#### Initialization
```cpp
bool initialize();
```
Initialize all components (STT, TTS, LLM). Returns true on success.

#### Shutdown
```cpp
void shutdown();
```
Clean shutdown of all resources.

#### Voice Command Processing
```cpp
std::vector<uint8_t> processVoiceCommand(
    const std::vector<uint8_t>& audio_data,
    const std::string& session_id
);
```
Process audio input and return audio response.

**Parameters:**
- `audio_data`: Raw audio data (WAV format, 16kHz mono recommended)
- `session_id`: Session identifier for conversation continuity

**Returns:** Audio response as WAV data

#### Text Command Processing
```cpp
std::string processTextCommand(
    const std::string& text,
    const std::string& session_id
);
```
Process text input (useful for testing/API). Returns text response.

#### Phone Call Recording
```cpp
json recordPhoneCall(
    const std::vector<uint8_t>& audio_data,
    const PhoneCallMetadata& metadata
);
```
Record and transcribe phone call with metadata.

**Returns:** JSON with transcript, summary, segments, and document ID

#### Meeting Protocol Generation
```cpp
json generateMeetingProtocol(
    const std::vector<uint8_t>& audio_data,
    const MeetingMetadata& metadata
);
```
Generate structured meeting protocol from recording.

**Returns:** JSON with protocol, action items, decisions

#### Audio Format Conversion
```cpp
std::vector<uint8_t> convertAudioFormat(
    const std::vector<uint8_t>& audio_data,
    const std::string& target_format
);
```
Convert audio between formats (wav, ogg, mp3, mp4).

#### Storage Operations
```cpp
std::string storeRecording(
    const std::vector<uint8_t>& audio_data,
    const std::string& transcript,
    const json& metadata
);
```
Store recording in ThemisDB with revision control.

**Returns:** Document ID in ThemisDB

#### Session Management
```cpp
VoiceSession getSession(const std::string& session_id);
void updateSession(const std::string& session_id, const json& context);
```
Manage conversation sessions and context.

#### Statistics
```cpp
json getStatistics() const;
```
Get usage statistics (calls processed, storage used, etc.).

## Data Structures

### VoiceSession
```cpp
struct VoiceSession {
    std::string session_id;           // Unique session ID
    std::string user_id;              // User identifier
    int64_t created_at;               // Session creation timestamp
    int64_t last_activity;            // Last interaction timestamp
    json context;                     // Conversation context (JSON)
    std::vector<std::string> history; // Conversation history
};
```

### PhoneCallMetadata
```cpp
struct PhoneCallMetadata {
    std::string call_id;              // Unique call identifier
    std::string caller_number;        // Caller phone number
    std::string callee_number;        // Callee phone number
    int64_t start_time;               // Call start timestamp (ms)
    int64_t end_time;                 // Call end timestamp (ms)
    int64_t duration_ms;              // Call duration
    std::string call_type;            // "inbound", "outbound", "conference"
    json custom_fields;               // Additional metadata
};
```

### MeetingMetadata
```cpp
struct MeetingMetadata {
    std::string meeting_id;           // Unique meeting identifier
    std::string title;                // Meeting title
    int64_t start_time;               // Meeting start timestamp
    int64_t end_time;                 // Meeting end timestamp
    std::vector<std::string> participants; // Participant emails/IDs
    std::string organizer;            // Meeting organizer
    json custom_fields;               // Additional metadata
};
```

## Integration Examples

### Example 1: Voice-Enabled Query Interface
```cpp
#include "voice/voice_assistant.h"

// Setup
VoiceAssistant::Config config;
config.stt_model_path = "/models/whisper-base.bin";
config.tts_model_path = "/models/tts-en.bin";
config.llm_model_path = "/models/mistral-7b.gguf";

VoiceAssistant assistant(config);
if (!assistant.initialize()) {
    std::cerr << "Failed to initialize voice assistant" << std::endl;
    return 1;
}

// Process voice queries
std::string session_id = generate_session_id();

while (true) {
    // Record user voice
    auto audio_input = record_audio(5000); // 5 seconds
    
    // Process and get response
    auto audio_output = assistant.processVoiceCommand(audio_input, session_id);
    
    // Play response
    play_audio(audio_output);
}
```

### Example 2: Call Center Integration
```cpp
#include "voice/voice_assistant.h"

class CallCenterIntegration {
public:
    CallCenterIntegration(VoiceAssistant& assistant) 
        : assistant_(assistant) {}
    
    void handleIncomingCall(
        const std::string& caller_number,
        std::vector<uint8_t> recording
    ) {
        // Prepare metadata
        PhoneCallMetadata metadata;
        metadata.call_id = generate_call_id();
        metadata.caller_number = caller_number;
        metadata.callee_number = get_agent_number();
        metadata.call_type = "inbound";
        metadata.start_time = get_current_time();
        
        // Process call
        json result = assistant_.recordPhoneCall(recording, metadata);
        
        // Store in CRM
        if (result["success"]) {
            update_crm({
                {"customer_phone", caller_number},
                {"transcript", result["transcript"]},
                {"summary", result["summary"]},
                {"sentiment", analyze_sentiment(result["transcript"])},
                {"document_id", result["document_id"]}
            });
        }
        
        // Check for follow-up actions
        if (result.contains("action_items")) {
            create_follow_up_tasks(result["action_items"]);
        }
    }
    
private:
    VoiceAssistant& assistant_;
};
```

### Example 3: Meeting Room Assistant
```cpp
#include "voice/voice_assistant.h"

class MeetingRoomAssistant {
public:
    void recordMeeting(
        const std::string& room_id,
        const std::string& title,
        const std::vector<std::string>& participants
    ) {
        // Start recording
        auto audio_stream = start_room_recording(room_id);
        
        MeetingMetadata metadata;
        metadata.meeting_id = generate_meeting_id();
        metadata.title = title;
        metadata.participants = participants;
        metadata.organizer = get_organizer_from_calendar(title);
        metadata.start_time = get_current_time();
        
        // Wait for meeting to end
        wait_for_meeting_end(room_id);
        
        metadata.end_time = get_current_time();
        auto recording = audio_stream.finalize();
        
        // Generate protocol
        json protocol = assistant_.generateMeetingProtocol(recording, metadata);
        
        // Distribute to participants
        email_meeting_minutes(participants, protocol);
        
        // Store in document management system
        store_in_dms(protocol);
        
        // Create calendar entries for action items
        for (const auto& item : protocol["action_items"]) {
            create_calendar_reminder(
                item["assignee"],
                item["description"],
                item["due_date"]
            );
        }
    }
    
private:
    VoiceAssistant assistant_;
};
```

## Performance Considerations

### Real-Time Processing
- **STT Latency**: 100-500ms per second of audio (model-dependent)
- **LLM Latency**: 50-200ms per token (model size dependent)
- **TTS Latency**: 50-100ms per second of speech

**Optimization Tips:**
- Use smaller STT models (tiny/base) for real-time
- Enable GPU acceleration for LLM (n_gpu_layers > 0)
- Stream TTS output (start playing before complete)
- Use connection pooling for database queries

### Batch Processing
- Phone call transcription: 1-10x real-time (faster than recording)
- Meeting protocol generation: 2-5x real-time
- Bulk storage operations: 1000+ recordings/minute

### Storage Efficiency
- **Uncompressed WAV**: ~10MB per minute (16kHz mono, 16-bit)
- **OGG Vorbis**: ~1MB per minute (quality 5)
- **MP3**: ~1MB per minute (128kbps)
- **Transcripts**: ~1-2KB per minute of speech

**Recommendations:**
- Use OGG/MP3 for long-term storage
- Keep WAV only for high-quality archives
- Enable compression for phone calls (ogg)
- Use higher quality for important meetings (mp3/mp4)

## Security & Privacy

### Data Protection
- **Audio Encryption**: Support for at-rest encryption
- **Transcript Redaction**: PII removal capabilities
- **Access Control**: Role-based access to recordings
- **Audit Logging**: Track all access to voice data

### Compliance Features
- **GDPR**: Right to deletion, data export
- **HIPAA**: PHI protection in healthcare context
- **PCI-DSS**: Credit card data masking in transcripts
- **SOC 2**: Audit trail for all operations

### Privacy Best Practices
```cpp
// Enable PII redaction
json options;
options["redact_pii"] = true;
options["redact_credit_cards"] = true;
options["redact_phone_numbers"] = true;

auto result = assistant.recordPhoneCall(audio, metadata, options);

// Result transcript will have redacted fields:
// "My credit card number is [REDACTED]"
// "Call me at [REDACTED]"
```

## Accessibility Features

### Voice Interface Benefits
- **Hands-Free Operation**: Ideal for driving, cooking, multitasking
- **Visual Impairment Support**: Complete audio-only interaction
- **Physical Accessibility**: No typing or mouse required
- **Language Support**: Multi-language STT/TTS

### Inclusive Design
- Adjustable speech rate (0.5x to 2.0x)
- Multiple voice options (male/female, accents)
- Clear error messages spoken aloud
- Conversational repair strategies

## Dependencies

### Internal Dependencies
- **content::STTProcessor**: Speech-to-text transcription
- **content::TTSProcessor**: Text-to-speech synthesis
- **llm::LlamaWrapper**: LLM inference and text generation
- **llm::EmbeddedLLM**: Unified LLM integration layer

### External Dependencies
- **Whisper.cpp**: STT model inference
- **llama.cpp**: LLM model inference
- **Audio codecs**: libvorbis, libmp3lame, libopus
- **JSON**: nlohmann/json for structured data

## Testing

### Unit Tests
- STT processor initialization
- TTS processor initialization
- LLM integration functionality
- Session management
- Audio format conversion

### Integration Tests
- End-to-end voice command flow
- Phone call recording pipeline
- Meeting protocol generation
- Storage and retrieval operations

### Test Audio Samples
Located in `tests/voice/audio_samples/`:
- `test_query.wav`: Sample voice query
- `test_conversation.wav`: Multi-turn dialogue
- `test_call.wav`: Sample phone call recording
- `test_meeting.wav`: Sample meeting audio

## Troubleshooting

### Common Issues

**Issue**: STT initialization fails
```
Error: "Failed to load Whisper model"
Solution: Check model path and file permissions
         Verify model format matches Whisper.cpp version
```

**Issue**: LLM responses are slow
```
Cause: Running large model on CPU
Solution: Enable GPU acceleration:
         config.llm_n_gpu_layers = 32;  // Adjust based on VRAM
```

**Issue**: Audio format not supported
```
Error: "Unsupported audio format"
Solution: Convert to supported format:
         - WAV (preferred)
         - OGG Vorbis
         - MP3
         - MP4/AAC
```

**Issue**: Session state lost
```
Cause: Sessions not persisted
Solution: Implement session persistence:
         - Store sessions in Redis
         - Use ThemisDB for session state
         - Configure session timeout
```

## See Also

- [Content Module](../content/README.md) - STT/TTS processors
- [LLM Module](../llm/README.md) - Language model integration
- [Voice API Handler](../api/voice_api_handler.cpp.md) - REST API endpoints
- [Voice Header](../../include/voice/README.md) - Interface definitions
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features

---

*Last Updated: April 2026*  
*Module Version: v1.0.0*  
*Next Review: v1.1.0 Release*

## Scientific References

1. Radford, A., Kim, J. W., Xu, T., Brockman, G., McLeavey, C., & Sutskever, I. (2023). **Robust Speech Recognition via Large-Scale Weak Supervision**. *Proceedings of the 40th International Conference on Machine Learning (ICML)*, 28492–28518. https://arxiv.org/abs/2212.04356

2. Graves, A., Mohamed, A., & Hinton, G. (2013). **Speech Recognition with Deep Recurrent Neural Networks**. *Proceedings of the 2013 IEEE International Conference on Acoustics, Speech and Signal Processing (ICASSP)*, 6645–6649. https://doi.org/10.1109/ICASSP.2013.6638947

3. Bahdanau, D., Cho, K., & Bengio, Y. (2015). **Neural Machine Translation by Jointly Learning to Align and Translate**. *Proceedings of ICLR 2015*. https://arxiv.org/abs/1409.0473

4. Jurafsky, D., & Martin, J. H. (2023). **Speech and Language Processing (3rd ed. draft)**. Prentice Hall. https://web.stanford.edu/~jurafsky/slp3/

5. Guo, W., Su, S., & Xu, R. (2021). **Recent Advances of Conformer-Based Speech Recognition**. *arXiv preprint*. https://arxiv.org/abs/2105.08206
