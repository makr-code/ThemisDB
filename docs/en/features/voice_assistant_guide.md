# ThemisDB Voice Assistant - Complete Guide

**Version:** 1.0  
**Status:** Enterprise Feature  
**Author:** ThemisDB Team  
**Date:** December 2025

---

## Overview

ThemisDB Voice Assistant provides natural language voice interaction capabilities similar to Alexa or Siri, integrated directly into the database. It combines Speech-to-Text (STT), Text-to-Speech (TTS), and Large Language Models (LLM) to enable:

- **Voice Commands** - Query and control the database using natural language
- **Phone Call Recording** - Automatic transcription and storage of phone calls
- **Meeting Protocol Generation** - AI-powered meeting minutes and action items
- **Voice Assistant Conversations** - Interactive voice-based assistance

All recordings and transcriptions are stored securely in ThemisDB with full revision control and audit trails (Enterprise feature).

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Voice Assistant                        │
│  ┌───────────┐   ┌───────────┐   ┌─────────────┐       │
│  │    STT    │   │    TTS    │   │     LLM     │       │
│  │ (Whisper) │   │  (Piper)  │   │ (llama.cpp) │       │
│  └───────────┘   └───────────┘   └─────────────┘       │
└────────────────────┬────────────────────────────────────┘
                     │
         ┌───────────┴───────────┐
         │                       │
    ┌────▼────┐            ┌────▼────┐
    │   API   │            │   WS    │
    │/api/v1/ │            │  /ws/   │
    │ voice   │            │ voice   │
    └─────────┘            └─────────┘
         │                       │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │  ThemisDB Storage     │
         │  - Base Entities      │
         │  - Revision Control   │
         │  - Audit Logs         │
         └───────────────────────┘
```

---

## Features

### 1. Speech-to-Text (STT)

Powered by **Whisper.cpp** for high-accuracy transcription:

- Multi-language support (100+ languages with auto-detection)
- Timestamp generation for segments
- Speaker diarization (identify different speakers)
- Word-level confidence scores
- Real-time streaming transcription

**Supported Audio Formats:**
- MP3, WAV, OGG, FLAC, AAC, M4A, Opus, WMA

**Model Sizes:**
- `tiny` - 39M params, fast, good for real-time
- `base` - 74M params, balanced (default)
- `small` - 244M params, better accuracy
- `medium` - 769M params, high accuracy
- `large` - 1550M params, best accuracy

### 2. Text-to-Speech (TTS)

Powered by **Piper TTS** for natural-sounding voice synthesis:

- Multiple voice profiles (male/female, different accents)
- Adjustable speed and pitch
- Multiple output formats (WAV, MP3, OGG)
- High-quality neural synthesis
- Real-time streaming synthesis

**Available Voices:**
- English (US, UK, Australian)
- German
- Spanish
- French
- And more...

### 3. LLM Integration

Uses **llama.cpp** for natural language understanding:

- Conversation context management
- Meeting summary generation
- Key points extraction
- Action items identification
- Natural language query processing

---

## Quick Start

### 1. Enable Voice Assistant

Edit `config/voice_assistant.yaml`:

```yaml
voice_assistant:
  enabled: true
  
  stt:
    model_path: "./models/ggml-base.bin"
    model_size: "base"
    language: "auto"
  
  tts:
    model_path: "./models/tts-model.bin"
    voice: "default"
  
  llm:
    model_path: "./models/llama-2-7b-chat.gguf"
    n_ctx: 4096
```

### 2. Start ThemisDB Server

```bash
./themis_server --config config.yaml --enable-voice-assistant
```

### 3. Test Voice Command

```bash
curl -X POST http://localhost:8080/api/v1/voice/command \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "text": "What is the total revenue this month?",
    "session_id": "user123"
  }'
```

---

## API Reference

### Base URL

```
http://localhost:8080/api/v1/voice
```

### Authentication

All endpoints require Bearer token authentication:

```
Authorization: Bearer YOUR_JWT_TOKEN
```

### Endpoints

#### 1. Transcribe Audio

**POST** `/api/v1/voice/transcribe`

Convert audio to text.

**Request:**
```json
{
  "audio_base64": "BASE64_ENCODED_AUDIO",
  "language": "auto",
  "timestamps": true,
  "speaker_diarization": false
}
```

**Response:**
```json
{
  "success": true,
  "text": "Hello, this is a test transcription.",
  "language": "en",
  "confidence": 0.95,
  "duration_ms": 3000,
  "segments": [
    {
      "text": "Hello, this is a test transcription.",
      "start_ms": 0,
      "end_ms": 3000,
      "confidence": 0.95
    }
  ]
}
```

#### 2. Synthesize Speech

**POST** `/api/v1/voice/synthesize`

Convert text to speech.

**Request:**
```json
{
  "text": "Hello, how can I help you today?",
  "voice": "default",
  "speed": 1.0,
  "format": "wav",
  "return_base64": true
}
```

**Response:**
```json
{
  "success": true,
  "audio_base64": "BASE64_ENCODED_AUDIO",
  "mime_type": "audio/wav",
  "duration_ms": 2500
}
```

#### 3. Process Voice Command

**POST** `/api/v1/voice/command`

Process a voice or text command with LLM.

**Request (Text):**
```json
{
  "text": "Show me the top 10 customers by revenue",
  "session_id": "user123"
}
```

**Request (Audio):**
```json
{
  "audio_base64": "BASE64_ENCODED_AUDIO",
  "session_id": "user123"
}
```

**Response (Text):**
```json
{
  "success": true,
  "response": "Here are the top 10 customers by revenue...",
  "session_id": "user123"
}
```

**Response (Audio):**
```json
{
  "success": true,
  "audio_base64": "BASE64_ENCODED_AUDIO",
  "mime_type": "audio/wav",
  "session_id": "user123"
}
```

#### 4. Record Phone Call

**POST** `/api/v1/voice/call/record`

Record and transcribe a phone call.

**Request:**
```json
{
  "audio_base64": "BASE64_ENCODED_AUDIO",
  "call_id": "call-12345",
  "caller": "+1234567890",
  "callee": "+0987654321",
  "start_time": 1703000000000,
  "end_time": 1703003600000,
  "call_type": "inbound",
  "custom_fields": {
    "department": "Sales",
    "category": "Support"
  }
}
```

**Response:**
```json
{
  "success": true,
  "call_id": "call-12345",
  "transcript": "Full transcription text...",
  "language": "en",
  "confidence": 0.95,
  "duration_ms": 3600000,
  "segments": [...],
  "summary": "Customer called regarding...",
  "document_id": "recording:abc123",
  "metadata": {
    "caller": "+1234567890",
    "callee": "+0987654321",
    "call_type": "inbound"
  }
}
```

#### 5. Generate Meeting Protocol

**POST** `/api/v1/voice/meeting/protocol`

Generate a structured meeting protocol from audio recording.

**Request:**
```json
{
  "audio_base64": "BASE64_ENCODED_AUDIO",
  "meeting_id": "meeting-789",
  "title": "Q4 Planning Meeting",
  "start_time": 1703000000000,
  "end_time": 1703007200000,
  "organizer": "john.doe@company.com",
  "participants": [
    "john.doe@company.com",
    "jane.smith@company.com",
    "bob.jones@company.com"
  ],
  "custom_fields": {
    "project": "Phoenix",
    "location": "Conference Room A"
  }
}
```

**Response:**
```json
{
  "success": true,
  "meeting_id": "meeting-789",
  "title": "Q4 Planning Meeting",
  "transcript": "Full meeting transcript...",
  "summary": "The team discussed Q4 objectives...",
  "key_points": [
    "Launch new product in Q4",
    "Increase marketing budget by 20%",
    "Hire 3 new developers"
  ],
  "action_items": [
    {
      "description": "Prepare product launch plan",
      "status": "pending"
    },
    {
      "description": "Submit budget proposal",
      "status": "pending"
    }
  ],
  "segments": [...],
  "document_id": "recording:xyz789",
  "participants": [...],
  "duration_ms": 7200000
}
```

#### 6. Get Available Voices

**GET** `/api/v1/voice/voices`

List available TTS voices.

**Response:**
```json
{
  "voices": [
    {
      "id": "default",
      "name": "Default Voice",
      "language": "en",
      "gender": "neutral",
      "style": "professional"
    },
    {
      "id": "female_en",
      "name": "Female English",
      "language": "en",
      "gender": "female",
      "style": "friendly"
    }
  ]
}
```

#### 7. Get Supported Languages

**GET** `/api/v1/voice/languages`

List supported languages for STT/TTS.

**Response:**
```json
{
  "languages": [
    "en", "de", "es", "fr", "it", "pt", "ru", "zh", "ja", "ko"
  ]
}
```

#### 8. Get Statistics

**GET** `/api/v1/voice/stats`

Get voice assistant statistics.

**Response:**
```json
{
  "stt": {
    "transcriptions_completed": 1234,
    "total_audio_duration_ms": 3600000,
    "real_time_factor": 0.3
  },
  "tts": {
    "syntheses_completed": 567,
    "total_audio_duration_ms": 1800000
  },
  "llm": {
    "tokens_processed": 50000,
    "cache_hits": 1200,
    "avg_latency_ms": 150
  },
  "wake_word": {
    "total_chunks_processed": 8400,
    "total_detections": 12,
    "registered_wake_words": 3,
    "buffer_samples": 24000
  },
  "active_sessions": 5
}
```

#### 9. Health Check

**GET** `/api/v1/voice/health`

Check voice assistant health.

**Response:**
```json
{
  "status": "healthy",
  "voice_assistant": "available",
  "timestamp": 1703000000000
}
```

---

## Use Cases

### 1. Phone Call Recording System

Record and transcribe customer support calls automatically:

```python
import requests
import base64

# Read audio file
with open("call.mp3", "rb") as f:
    audio_data = f.read()
    audio_base64 = base64.b64encode(audio_data).decode()

# Record call
response = requests.post(
    "http://localhost:8080/api/v1/voice/call/record",
    headers={"Authorization": "Bearer YOUR_TOKEN"},
    json={
        "audio_base64": audio_base64,
        "call_id": "call-12345",
        "caller": "+1234567890",
        "callee": "+0987654321",
        "call_type": "inbound"
    }
)

result = response.json()
print(f"Transcript: {result['transcript']}")
print(f"Summary: {result['summary']}")
print(f"Document ID: {result['document_id']}")
```

### 2. Meeting Minutes Generation

Automatically generate meeting protocols:

```python
import requests
import base64

# Read meeting recording
with open("meeting.wav", "rb") as f:
    audio_data = f.read()
    audio_base64 = base64.b64encode(audio_data).decode()

# Generate protocol
response = requests.post(
    "http://localhost:8080/api/v1/voice/meeting/protocol",
    headers={"Authorization": "Bearer YOUR_TOKEN"},
    json={
        "audio_base64": audio_base64,
        "meeting_id": "meeting-789",
        "title": "Sprint Planning",
        "participants": [
            "alice@company.com",
            "bob@company.com"
        ]
    }
)

result = response.json()
print(f"Summary: {result['summary']}")
print(f"Key Points: {result['key_points']}")
print(f"Action Items: {result['action_items']}")
```

### 3. Voice-Controlled Database Queries

Query the database using natural language:

```python
import requests

response = requests.post(
    "http://localhost:8080/api/v1/voice/command",
    headers={"Authorization": "Bearer YOUR_TOKEN"},
    json={
        "text": "Show me the total sales for last month",
        "session_id": "user123"
    }
)

result = response.json()
print(f"Response: {result['response']}")
```

---

## Configuration

### STT Configuration

```yaml
stt:
  model:
    path: "./models/ggml-base.bin"
    size: "base"  # tiny, base, small, medium, large
    auto_download: true
  
  transcription:
    language: "auto"
    timestamps: true
    timestamp_granularity: "segment"
    word_confidence: false
  
  speaker_diarization:
    enabled: false
    num_speakers: 0  # 0 = auto-detect
  
  vad:
    enabled: true
    threshold: 0.5
```

### TTS Configuration

```yaml
tts:
  model:
    path: "./models/tts-model.bin"
    engine: "piper"
  
  synthesis:
    sample_rate: 22050
    speed: 1.0
    pitch: 1.0
    normalize: true
  
  output:
    format: "wav"
    quality: "medium"
```

### LLM Configuration

```yaml
llm:
  model_path: "./models/llama-2-7b-chat.gguf"
  n_ctx: 4096
  n_gpu_layers: 0  # 0 = CPU only
  temperature: 0.7
  top_p: 0.9
```

### Wake-Word Configuration

Enable hands-free activation so users can trigger the voice pipeline without pressing a button:

```yaml
voice_commands:
  wake_word_enabled: true
  wake_word: "hey themis"                # Primary wake word (kept for backward compat)
  wake_word_sensitivity: 0.5             # 0.0 = permissive, 1.0 = strict
  wake_word_buffer_length_ms: 1500       # Rolling audio buffer size (ms)
  wake_word_cooldown_ms: 1000            # Minimum ms between detections
  wake_word_vad_min_energy: 0.005        # RMS energy gate (silence suppressor)
  wake_word_sample_rate: 16000           # Expected PCM sample rate (Hz)
  wake_words:
    - id: "hey-themis"
      phrase: "hey themis"
    - id: "themis"
      phrase: "themis"
    - id: "database"
      phrase: "database"
```

**C++ API:**

```cpp
#include "voice/voice_assistant.h"

themis::voice::VoiceAssistant::Config cfg;
cfg.enable_wake_word = true;
cfg.wake_word_config.sensitivity       = 0.5f;
cfg.wake_word_config.cooldown_ms       = 1000;
cfg.wake_word_config.vad_min_energy    = 0.005f;
cfg.wake_word_config.continuous_listen = true;
// Override default wake words if needed
cfg.wake_words = {
    {"hey-themis", "hey themis"},
    {"themis",     "themis"}
};

themis::voice::VoiceAssistant va(cfg);

// Register a callback (optional – called synchronously on detection)
va.setWakeWordCallback([](const themis::voice::WakeWordDetectionResult& r) {
    // r.wake_word_id   – which word fired
    // r.confidence     – score in [0, 1]
    // r.detection_timestamp_ms – wall-clock ms
});

// Stream microphone chunks into the detector
while (capturing) {
    auto chunk = mic.readChunk();           // 16-bit LE PCM
    auto result = va.detectWakeWord(chunk);
    if (result.detected) {
        // Wake word confirmed – start the voice pipeline
        va.streamProcessVoiceCommand(recordUtterance(), session_id);
    }
}
```

**How detection works:**

The detector runs a two-stage pipeline on every audio chunk:

1. **VAD gate** – RMS energy is compared against `vad_min_energy`.  Chunks below the
   threshold are discarded immediately (near-zero CPU cost during silence).
2. **Keyword scoring** – Each registered phrase is scored using three audio features:
   phrase-length density, a spectral-centroid proxy (speech vs. broadband noise), and the
   crest factor (consonant-rich transients).  The best score must exceed `sensitivity` to
   confirm a detection.  No external model file is required.

**Statistics returned by `/api/v1/voice/stats`:**

```json
{
  "wake_word": {
    "total_chunks_processed": 8400,
    "total_detections": 12,
    "registered_wake_words": 3,
    "buffer_samples": 24000
  }
}
```

---

## Storage and Revision Control

All recordings and transcriptions are stored in ThemisDB with:

- **Revision Control** - Track changes over time
- **Audit Logs** - Who accessed/modified what and when
- **Encryption** - At-rest encryption for sensitive data
- **Compression** - Automatic audio compression (OGG/MP3)
- **Metadata** - Rich metadata for search and retrieval

**Storage Path:**
```
data/voice_recordings/
  ├── calls/
  │   ├── call-12345/
  │   │   ├── audio.ogg
  │   │   ├── transcript.txt
  │   │   └── metadata.json
  │   └── ...
  └── meetings/
      ├── meeting-789/
      │   ├── audio.ogg
      │   ├── protocol.md
      │   └── metadata.json
      └── ...
```

---

## Security

### Authentication

- JWT Bearer token required for all API endpoints
- Token validation on every request
- Session-based access control

### Privacy

- PII detection and optional redaction
- Configurable data retention policies
- Automatic cleanup of old recordings
- GDPR-compliant data handling

### Audit Logging

All voice operations are logged:
- Who initiated the request
- What operation was performed
- When it occurred
- What data was accessed/modified

---

## Performance

### STT Performance

| Model | Speed | Accuracy | Memory |
|-------|-------|----------|--------|
| tiny  | 4x RT | Good     | ~1 GB  |
| base  | 1x RT | Better   | ~1 GB  |
| small | 0.5x RT | High    | ~2 GB  |
| medium| 0.3x RT | Very High| ~5 GB |
| large | 0.2x RT | Best    | ~10 GB |

*RT = Real-time (1x RT means 1 minute audio = 1 minute processing)*

### TTS Performance

- ~50-100 characters/second synthesis
- Real-time streaming capable
- Low latency (<100ms for short phrases)

### LLM Performance

- Depends on model size and hardware
- GPU acceleration recommended
- ~20-50 tokens/second (typical)

---

## Troubleshooting

### Issue: STT model not found

**Solution:** Enable auto-download or manually download:
```bash
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin \
  -O models/ggml-base.bin
```

### Issue: High latency for transcription

**Solution:** Use smaller model (tiny/base) or enable GPU acceleration.

### Issue: Poor transcription quality

**Solution:** Use larger model (medium/large) or ensure audio quality is good.

---

## Enterprise Features

- **Horizontal Scaling** - Distribute voice processing across nodes
- **High Availability** - Redundant voice assistants
- **Advanced Analytics** - Call analytics, sentiment analysis
- **Custom Voice Training** - Train custom voices for your brand
- **Integration** - Integrate with PBX systems, CRM, etc.

---

## License Information

All core libraries used in the Voice Assistant are **open-source with MIT License**:

- **Whisper.cpp** (STT) - MIT License
- **Piper TTS** (TTS) - MIT License  
- **llama.cpp** (LLM) - MIT License
- **ONNX Runtime** - MIT License

✅ **Suitable for commercial and on-premise use**  
✅ **No external API dependencies**  
✅ **Privacy-preserving (all processing local)**

**[→ Complete License Documentation](voice_assistant_licenses.md)**

---

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
- Enterprise Support: sales@themisdb.com

---

## License

Voice Assistant is an **Enterprise Feature** of ThemisDB.

- Community Edition: Limited to basic STT/TTS functionality
- Enterprise Edition: Full features including phone call recording, meeting protocols, and advanced LLM integration

See [LICENSE](LICENSE) for details.
