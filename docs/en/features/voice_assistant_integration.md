# Voice Assistant - Integration Guide for Whisper.cpp and Piper TTS

**Version:** 1.0  
**Date:** December 2025  
**Status:** Implementation Guide

---

## Overview

This document provides step-by-step instructions for integrating actual Whisper.cpp and Piper TTS models with ThemisDB Voice Assistant.

---

## Prerequisites

### System Requirements

- **CMake** 3.20 or higher
- **C++20** compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **Git** for cloning repositories
- **ONNX Runtime** for Piper TTS (optional, can be bundled)

### Optional (for GPU acceleration)

- **CUDA Toolkit** 11.x or 12.x (NVIDIA GPUs)
- **cuBLAS** (comes with CUDA)

---

## Step 1: Clone and Build Whisper.cpp

### 1.1 Clone Whisper.cpp

```bash
cd /path/to/ThemisDB
mkdir -p external
cd external
git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
```

### 1.2 Build Whisper.cpp

**For CPU-only:**
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**For GPU (CUDA):**
```bash
mkdir build && cd build
cmake .. -DWHISPER_CUBLAS=ON
cmake --build . --config Release
```

**For GPU (HIP - AMD):**
```bash
mkdir build && cd build
cmake .. -DWHISPER_HIPBLAS=ON
cmake --build . --config Release
```

### 1.3 Download Whisper Models

```bash
cd /path/to/ThemisDB/external/whisper.cpp

# Download base model (recommended for starting)
bash ./models/download-ggml-model.sh base

# Or download other models:
# bash ./models/download-ggml-model.sh tiny    # Fastest, least accurate
# bash ./models/download-ggml-model.sh small   # Good balance
# bash ./models/download-ggml-model.sh medium  # Better accuracy
# bash ./models/download-ggml-model.sh large-v3 # Best accuracy
```

Models will be downloaded to `./models/` directory.

---

## Step 2: Clone and Build Piper TTS

### 2.1 Clone Piper TTS

```bash
cd /path/to/ThemisDB/external
git clone https://github.com/rhasspy/piper.git
cd piper
```

### 2.2 Build Piper TTS

**Install dependencies first:**

On Ubuntu/Debian:
```bash
sudo apt-get install libespeak-ng-dev libonnxruntime-dev
```

On macOS:
```bash
brew install espeak-ng onnxruntime
```

**Build Piper:**
```bash
cd src/cpp
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 2.3 Download Piper Voice Models

```bash
cd /path/to/ThemisDB
mkdir -p models/voices

# Download English voice (Amy - US Female)
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx \
     -O models/voices/en_US-amy-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx.json \
     -O models/voices/en_US-amy-medium.onnx.json

# Download German voice (Thorsten - Male)
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx \
     -O models/voices/de_DE-thorsten-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx.json \
     -O models/voices/de_DE-thorsten-medium.onnx.json
```

More voices available at: https://huggingface.co/rhasspy/piper-voices/tree/main

---

## Step 3: Build ThemisDB with Voice Assistant

### 3.1 Configure CMake

```bash
cd /path/to/ThemisDB
mkdir -p build && cd build

cmake .. \
  -DTHEMIS_ENABLE_VOICE_ASSISTANT=ON \
  -DTHEMIS_ENABLE_WHISPER=ON \
  -DTHEMIS_ENABLE_PIPER_TTS=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DWHISPER_ROOT=/path/to/ThemisDB/external/whisper.cpp \
  -DPIPER_ROOT=/path/to/ThemisDB/external/piper/src/cpp \
  -DCMAKE_BUILD_TYPE=Release
```

**For GPU acceleration (CUDA):**
```bash
cmake .. \
  -DTHEMIS_ENABLE_VOICE_ASSISTANT=ON \
  -DTHEMIS_ENABLE_WHISPER=ON \
  -DTHEMIS_ENABLE_PIPER_TTS=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DWHISPER_ROOT=/path/to/ThemisDB/external/whisper.cpp \
  -DPIPER_ROOT=/path/to/ThemisDB/external/piper/src/cpp \
  -DCMAKE_BUILD_TYPE=Release
```

### 3.2 Build ThemisDB

```bash
cmake --build . --config Release -j$(nproc)
```

---

## Step 4: Configure Voice Assistant

### 4.1 Update Configuration Files

Edit `config/processors/stt.yaml`:

```yaml
processor:
  model:
    # Point to your downloaded Whisper model
    path: "./models/ggml-base.bin"
    size: "base"
    auto_download: false
```

Edit `config/processors/tts.yaml`:

```yaml
processor:
  model:
    # Point to your downloaded Piper voice
    path: "./models/voices/en_US-amy-medium.onnx"
    engine: "piper"
    auto_download: false
```

Edit `config/voice_assistant.yaml`:

```yaml
voice_assistant:
  enabled: true
  
  stt:
    model_path: "./models/ggml-base.bin"
    model_size: "base"
    language: "auto"
  
  tts:
    model_path: "./models/voices/en_US-amy-medium.onnx"
    voice: "en_US-amy-medium"
  
  llm:
    model_path: "./models/llama-2-7b-chat.gguf"
    n_ctx: 4096
```

---

## Step 5: Test the Integration

### 5.1 Start ThemisDB Server

```bash
cd /path/to/ThemisDB/build
./themis_server --config ../config/themis.yaml --enable-voice-assistant
```

### 5.2 Test STT (Speech-to-Text)

```bash
# Using Python example
cd /path/to/ThemisDB
python examples/voice_assistant_example.py
```

Or use curl:

```bash
# Prepare test audio (example with a WAV file)
base64 test_audio.wav > audio_base64.txt

# Call transcribe API
curl -X POST http://localhost:8080/api/v1/voice/transcribe \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"audio_base64\": \"$(cat audio_base64.txt)\", \"language\": \"en\"}"
```

### 5.3 Test TTS (Text-to-Speech)

```bash
curl -X POST http://localhost:8080/api/v1/voice/synthesize \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"text": "Hello, this is ThemisDB Voice Assistant", "voice": "default", "return_base64": true}' \
  | jq -r '.audio_base64' | base64 -d > output.wav

# Play the generated audio
aplay output.wav  # Linux
afplay output.wav # macOS
```

### 5.4 Test Complete Pipeline

```python
import requests
import base64

# Read audio file
with open("call_recording.wav", "rb") as f:
    audio_base64 = base64.b64encode(f.read()).decode()

# Record and transcribe phone call
response = requests.post(
    "http://localhost:8080/api/v1/voice/call/record",
    headers={"Authorization": "Bearer YOUR_TOKEN"},
    json={
        "audio_base64": audio_base64,
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

---

## Troubleshooting

### Issue: "Whisper model not loaded"

**Solution:**
1. Verify model file exists at the configured path
2. Check file permissions
3. Ensure CMake found Whisper.cpp library during build
4. Check server logs for detailed error messages

### Issue: "Piper TTS synthesis failed"

**Solution:**
1. Verify ONNX model and .json config files exist
2. Ensure ONNX Runtime is installed
3. Check ONNX model compatibility (should be Piper format)
4. Verify sufficient memory available

### Issue: Build errors with Whisper.cpp

**Solution:**
```bash
# Ensure you have the latest version
cd external/whisper.cpp
git pull
cd build
rm -rf *
cmake .. -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
cmake --build . --config Release
```

### Issue: ONNX Runtime not found

**Solution:**

**Ubuntu/Debian:**
```bash
wget https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
tar xzf onnxruntime-linux-x64-1.16.3.tgz
sudo cp -r onnxruntime-linux-x64-1.16.3/include/* /usr/local/include/
sudo cp -r onnxruntime-linux-x64-1.16.3/lib/* /usr/local/lib/
sudo ldconfig
```

**macOS:**
```bash
brew install onnxruntime
```

---

## Performance Optimization

### GPU Acceleration

For NVIDIA GPUs, build with CUDA support:

```bash
cmake .. \
  -DTHEMIS_ENABLE_VOICE_ASSISTANT=ON \
  -DTHEMIS_ENABLE_WHISPER=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DWHISPER_CUBLAS=ON
```

Update config:
```yaml
stt:
  performance:
    use_gpu: true
    gpu_device_id: 0
```

### Model Selection for Performance

| Model | Speed | Accuracy | RAM | Use Case |
|-------|-------|----------|-----|----------|
| tiny  | 4x RT | Good | 1GB | Real-time, low-resource |
| base  | 1x RT | Better | 1GB | Balanced (recommended) |
| small | 0.5x RT | High | 2GB | High accuracy needed |
| medium| 0.3x RT | Very High | 5GB | Maximum accuracy |
| large | 0.2x RT | Best | 10GB | Research/archival |

*RT = Real-time (1x RT = 1 minute audio = 1 minute processing)*

---

## Production Deployment

### Checklist

- [ ] Models downloaded and configured
- [ ] Build completed successfully with voice assistant enabled
- [ ] Configuration files updated with correct paths
- [ ] API authentication configured
- [ ] Storage paths configured for recordings
- [ ] Revision control enabled in ThemisDB
- [ ] Tested transcription with sample audio
- [ ] Tested synthesis with sample text
- [ ] Tested complete call recording pipeline
- [ ] Load testing completed
- [ ] Monitoring configured
- [ ] Backup strategy in place

### Recommended Setup

**Development:**
- Whisper: base model
- Piper: single voice (English)
- CPU processing

**Production:**
- Whisper: small or medium model
- Piper: multiple voices (multi-language)
- GPU acceleration (if available)
- Load balancer for multiple instances
- Redis cache for frequent queries

---

## Support

For issues or questions:
- **Documentation:** [Voice Assistant Guide](voice_assistant_guide.md)
- **Whisper.cpp:** https://github.com/ggerganov/whisper.cpp
- **Piper TTS:** https://github.com/rhasspy/piper
- **ThemisDB:** GitHub Issues

---

## License

Integration uses MIT-licensed libraries:
- Whisper.cpp: MIT License
- Piper TTS: MIT License
- ONNX Runtime: MIT License

See [License Documentation](voice_assistant_licenses.md) for details.
