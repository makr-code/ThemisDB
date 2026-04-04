# Voice Troubleshooting Guide

The `voice` module provides voice interaction capabilities for ThemisDB, including voice-based database queries, audio preprocessing, speaker authentication, batch audio processing, accessibility features, and LLM-powered voice assistants.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `VoiceAssistant: model not loaded` | ASR model not downloaded | Download ASR model |
| `AudioPreprocessing: ffmpeg not found` | ffmpeg not installed | Install `ffmpeg` |
| `VoiceAuthenticator: no voiceprint` | User has no enrolled voiceprint | Enroll voice first |
| Speech recognition inaccurate | Wrong language model | Set `voice.asr.language` correctly |
| `VoiceBatchProcessor: queue full` | Batch processing overloaded | Increase batch worker count |
| `VoiceAccessibility: tts not available` | TTS model not configured | Set `voice.tts.model_path` |
| `VoiceAudioStorage: disk full` | Audio files not rotated | Enable `voice.storage.rotation` |
| Long silence causes premature cutoff | VAD threshold too aggressive | Tune `voice.vad.silence_threshold_db` |
| `VoiceErrorHandler: unrecognised command` | Command not in grammar | Add command to grammar |
| Voice response latency too high | LLM model too large | Use smaller TTS/LLM model |

## Common Issues

### Issue 1: ASR Model Not Loaded

**Description:** Voice assistant cannot transcribe speech because the ASR model is not available.

**Symptoms:**
- Log: `VoiceAssistant: ASR model not found at /var/lib/themisdb/models/whisper-base.en.gguf`
- Voice queries return error

**Cause:** ASR model not downloaded or path misconfigured.

**Solution:**
```bash
# Download Whisper model
themisdb-admin voice download-model --model whisper-base-en

# Or download manually
wget https://models.themisdb.com/voice/whisper-base.en.gguf \
  -O /var/lib/themisdb/models/whisper-base.en.gguf
```
```yaml
voice:
  asr:
    model_path: /var/lib/themisdb/models/whisper-base.en.gguf
    language: en
    model_size: base               # "tiny" | "base" | "small" | "medium" | "large"
```

---

### Issue 2: Audio Preprocessing Fails Without ffmpeg

**Description:** Audio ingestion fails because ffmpeg is not available for format conversion.

**Symptoms:**
- Log: `AudioPreprocessing: ffmpeg not found; only raw PCM supported`
- MP3, OGG, M4A audio formats cannot be processed

**Cause:** ffmpeg not installed.

**Solution:**
```bash
# Install ffmpeg
apt install ffmpeg

# Verify
ffmpeg -version
```
```yaml
voice:
  audio:
    ffmpeg_path: /usr/bin/ffmpeg
    supported_formats: [wav, mp3, ogg, m4a, flac, webm]
    target_sample_rate: 16000      # 16kHz optimal for ASR
    target_channels: 1             # mono
    normalize_audio: true
```

---

### Issue 3: Speaker Authentication Fails for Enrolled User

**Description:** A user that is enrolled for voice authentication cannot authenticate.

**Symptoms:**
- Error: `VoiceAuthenticator: voiceprint match score=0.62 (threshold=0.80)`
- User is repeatedly denied

**Cause:** Audio quality is poor; background noise affecting voiceprint match.

**Solution:**
```yaml
voice:
  authenticator:
    match_threshold: 0.70           # lower from 0.80 if environment is noisy
    min_audio_duration_ms: 2000     # require at least 2 seconds
    denoise_before_match: true      # apply noise reduction
    max_attempts: 3
    lockout_duration_ms: 300000
```
```bash
# Re-enroll user in better audio conditions
themisdb-admin voice enroll \
  --user alice \
  --audio /tmp/alice_clean.wav \
  --replace-existing
```

---

### Issue 4: Voice Activity Detection Cuts Off Mid-Speech

**Description:** The voice assistant stops recording before the user finishes speaking.

**Symptoms:**
- Transcription is always truncated
- Log: `VoiceAssistant: VAD detected silence after 500ms; processing`

**Cause:** Voice Activity Detection silence threshold too aggressive; threshold set too high.

**Solution:**
```yaml
voice:
  vad:
    enabled: true
    silence_threshold_db: -40       # lower from -30 (less aggressive)
    min_speech_duration_ms: 500
    max_silence_duration_ms: 1500   # wait longer for pauses
    end_of_speech_timeout_ms: 2000
```

---

### Issue 5: TTS Not Available for Voice Responses

**Description:** Voice assistant cannot speak responses because TTS model is missing.

**Symptoms:**
- Log: `VoiceAccessibility: TTS model not configured`
- Text responses only; no audio output

**Cause:** TTS model path not set.

**Solution:**
```yaml
voice:
  tts:
    enabled: true
    model_path: /var/lib/themisdb/models/piper-en-us.onnx
    voice: en_US-amy-medium        # voice model variant
    sample_rate: 22050
    speed: 1.0                     # 1.0 = normal speed
```
```bash
# Download TTS model
themisdb-admin voice download-model --model piper-en-us
```

---

### Issue 6: Voice Audio Storage Fills Disk

**Description:** Recorded audio files accumulate and fill disk.

**Symptoms:**
- Disk usage: `/var/lib/themisdb/voice/` > 10 GB
- Log: `VoiceAudioStorage: write failed: ENOSPC`

**Cause:** No audio file rotation or retention policy.

**Solution:**
```yaml
voice:
  storage:
    path: /var/lib/themisdb/voice/
    rotation:
      enabled: true
      retention_days: 7            # keep audio for 7 days
      max_size_gb: 5
      compress: true
    auto_delete_transcribed: true  # delete after successful transcription
```

---

### Issue 7: LLM Voice Assistant Response Too Slow

**Description:** LLM-powered voice responses have high latency.

**Symptoms:**
- Voice response takes 5-8 seconds
- Users experience poor interaction quality

**Cause:** LLM model too large for voice interaction; full response generation before TTS.

**Solution:**
```yaml
voice:
  llm:
    model_path: /var/lib/themisdb/models/voice-llm.Q4_K_M.gguf
    streaming_tts: true            # stream TTS as tokens are generated
    max_response_tokens: 100       # short responses for voice
    temperature: 0.3               # low temperature for deterministic voice responses
    response_timeout_ms: 3000
```

---

### Issue 8: Batch Processor Queue Full

**Description:** Bulk voice file processing queue is full.

**Symptoms:**
- Log: `VoiceBatchProcessor: queue full (max=50); rejecting job`
- Batch audio processing jobs are rejected

**Cause:** Insufficient worker threads; queue too small.

**Solution:**
```yaml
voice:
  batch:
    worker_threads: 4              # increase from 1
    queue_size: 200
    job_timeout_ms: 600000
    priority_queue: true
```

## Diagnostic Commands

```bash
# Voice module health
themisdb-admin voice status

# ASR model info
themisdb-admin voice asr-info

# TTS model info
themisdb-admin voice tts-info

# Test transcription
themisdb-admin voice transcribe --audio /tmp/test.wav

# Enrolled users
themisdb-admin voice enrolled-users list

# Audio storage usage
du -sh /var/lib/themisdb/voice/

# Live voice metrics
curl -s http://localhost:9100/metrics | grep themisdb_voice

# Tail voice logs
journalctl -u themisdb -f | grep -E "voice|asr|tts|vad|authenticat|audio"
```

## Configuration Reference

```yaml
voice:
  enabled: false
  asr:
    model_path: ""
    language: en
  tts:
    enabled: false
    model_path: ""
  vad:
    enabled: true
    silence_threshold_db: -35
    max_silence_duration_ms: 1000
  authenticator:
    enabled: false
    match_threshold: 0.75
  storage:
    path: /var/lib/themisdb/voice/
    rotation:
      enabled: true
      retention_days: 7
  batch:
    worker_threads: 2
    queue_size: 100
```

## Known Limitations

- Whisper ASR models require significant CPU or GPU resources; `large` model requires 10+ GB VRAM.
- Speaker authentication is not suitable for high-security use cases; combine with multi-factor authentication.
- TTS Piper models require an ONNX runtime; see Exporters module for ONNX setup.
- Voice batch processor does not support real-time streaming input; it processes pre-recorded files only.
- Background noise in audio degrades ASR accuracy significantly; recommend headset or quiet environment.

## Related Documentation

- [Voice Module ROADMAP](../../src/voice/ROADMAP.md)
- [Content Troubleshooting](./content_troubleshooting.md)
- [LLM Troubleshooting](./llm_troubleshooting.md)
- [RAG Troubleshooting](./rag_troubleshooting.md)
