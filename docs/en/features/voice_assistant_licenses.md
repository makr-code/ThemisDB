# Voice Assistant - License Information and On-Premise Deployment

**Version:** 1.0  
**Status:** Enterprise Feature  
**Date:** December 2025

---

## Overview

This document provides comprehensive license information for all libraries used in the ThemisDB Voice Assistant feature, with specific focus on open-source compliance and on-premise deployment suitability.

---

## Core Libraries

### 1. Whisper.cpp (Speech-to-Text)

| Attribute | Details |
|-----------|---------|
| **Repository** | https://github.com/ggerganov/whisper.cpp |
| **Purpose** | High-accuracy speech transcription |
| **License** | MIT License |
| **Copyright** | Copyright (c) 2022 Georgi Gerganov |
| **Open Source** | ✅ Yes |
| **Commercial Use** | ✅ Allowed |
| **On-Premise** | ✅ Fully supported |
| **Attribution** | Required (MIT terms) |

**Key Features:**
- C++ port of OpenAI's Whisper model
- No external API dependencies
- Runs entirely offline
- 100+ languages supported
- Multiple model sizes (tiny to large)

**Compliance:**
- MIT License is one of the most permissive open-source licenses
- Allows commercial use, modification, and distribution
- No copyleft requirements
- Can be used in proprietary software
- Only requires attribution in documentation

---

### 2. Piper TTS (Text-to-Speech)

| Attribute | Details |
|-----------|---------|
| **Repository** | https://github.com/rhasspy/piper |
| **Purpose** | Neural text-to-speech synthesis |
| **License** | MIT License |
| **Copyright** | Copyright (c) 2023 Michael Hansen |
| **Open Source** | ✅ Yes |
| **Commercial Use** | ✅ Allowed |
| **On-Premise** | ✅ Fully supported |
| **Attribution** | Required (MIT terms) |

**Key Features:**
- Neural TTS with ONNX models
- Multiple voice profiles
- High-quality synthesis
- No cloud dependencies
- Lightweight and fast

**Compliance:**
- MIT License
- Suitable for commercial deployment
- Voice models also open-source (mostly MIT/CC-BY)
- No usage restrictions

---

### 3. llama.cpp (LLM Inference)

| Attribute | Details |
|-----------|---------|
| **Repository** | https://github.com/ggerganov/llama.cpp |
| **Purpose** | Large Language Model inference |
| **License** | MIT License |
| **Copyright** | Copyright (c) 2023 Georgi Gerganov |
| **Open Source** | ✅ Yes |
| **Commercial Use** | ✅ Allowed |
| **On-Premise** | ✅ Fully supported |
| **Status** | Already integrated in ThemisDB v1.3.0+ |

**Key Features:**
- Efficient LLM inference in C++
- Support for LLaMA, Mistral, Phi-3, and more
- GGUF model format
- CPU and GPU acceleration
- No external API calls

**Compliance:**
- MIT License
- Already approved for ThemisDB use
- Core library is MIT (models have separate licenses)

---

### 4. ONNX Runtime (Neural Network Engine)

| Attribute | Details |
|-----------|---------|
| **Repository** | https://github.com/microsoft/onnxruntime |
| **Purpose** | Neural network inference engine (used by Piper) |
| **License** | MIT License |
| **Copyright** | Copyright (c) Microsoft Corporation |
| **Open Source** | ✅ Yes |
| **Commercial Use** | ✅ Allowed |
| **On-Premise** | ✅ Fully supported |

**Key Features:**
- High-performance ML inference
- Cross-platform support
- Hardware acceleration
- Industry standard

---

## License Summary

### All Core Libraries: MIT License ✅

The MIT License allows:
- ✅ Commercial use
- ✅ Modification
- ✅ Distribution
- ✅ Private use
- ✅ Sublicensing

The MIT License requires:
- ✅ License and copyright notice (attribution)

The MIT License does NOT require:
- ❌ Disclosure of source code
- ❌ Copyleft (sharing modifications)
- ❌ Trademark use
- ❌ Patent grant
- ❌ Liability or warranty

---

## On-Premise Deployment

### ✅ Fully Supported

All libraries are designed for and support on-premise deployment:

1. **No External Dependencies**
   - All processing happens locally
   - No API calls to external services
   - No internet connection required after model download

2. **Privacy & Compliance**
   - Data never leaves your infrastructure
   - GDPR/DSGVO compliant
   - Suitable for sensitive data (healthcare, government, finance)
   - No third-party data processing

3. **Cost Benefits**
   - No per-request API fees
   - No usage metering
   - One-time model download
   - Unlimited usage

4. **Control & Flexibility**
   - Full control over versions
   - Can customize and fine-tune models
   - No vendor lock-in
   - Offline capability

5. **Performance**
   - Low latency (no network calls)
   - Scalable (add more hardware)
   - GPU acceleration supported
   - Predictable performance

---

## Enterprise Suitability

### ✅ Approved for Enterprise Use

| Criterion | Status | Notes |
|-----------|--------|-------|
| **Open Source** | ✅ Yes | All MIT licensed |
| **Commercial Use** | ✅ Allowed | No restrictions |
| **On-Premise** | ✅ Supported | Designed for it |
| **Data Privacy** | ✅ Compliant | No external calls |
| **Vendor Lock-in** | ✅ None | Open standards |
| **Modifications** | ✅ Allowed | Can customize |
| **Distribution** | ✅ Allowed | Can bundle |
| **Support** | ✅ Available | Active communities |

---

## Model Licenses

**Important:** While the inference libraries are MIT licensed, AI models may have separate licenses.

### Whisper Models

- **Source:** OpenAI / HuggingFace (ggerganov/whisper.cpp)
- **License:** Apache 2.0 / MIT
- **Commercial Use:** ✅ Allowed
- **Models:** tiny, base, small, medium, large (all open)

### Piper Voice Models

- **Source:** rhasspy/piper-voices (HuggingFace)
- **License:** Mostly MIT, some CC-BY
- **Commercial Use:** ✅ Most allow commercial use
- **Voices:** 50+ languages, 100+ voices

### LLM Models (for llama.cpp)

Different models have different licenses:

| Model | License | Commercial Use |
|-------|---------|----------------|
| LLaMA 2 | Meta License | ✅ Allowed (<700M users) |
| Mistral | Apache 2.0 | ✅ Allowed |
| Phi-3 | MIT License | ✅ Allowed |
| Gemma | Google ToS | ✅ Allowed |

**Recommendation:** Use Mistral or Phi-3 for unrestricted commercial use.

---

## Attribution Requirements

As per MIT License terms, include the following attribution:

```
This software uses the following open-source libraries:

Whisper.cpp - Speech Recognition
Copyright (c) 2022 Georgi Gerganov
MIT License - https://github.com/ggerganov/whisper.cpp

Piper TTS - Text-to-Speech Synthesis  
Copyright (c) 2023 Michael Hansen
MIT License - https://github.com/rhasspy/piper

llama.cpp - LLM Inference
Copyright (c) 2023 Georgi Gerganov
MIT License - https://github.com/ggerganov/llama.cpp

ONNX Runtime - Neural Network Inference
Copyright (c) Microsoft Corporation
MIT License - https://github.com/microsoft/onnxruntime
```

---

## Compliance Checklist

For on-premise enterprise deployment:

- [x] All libraries are open-source
- [x] All libraries use permissive licenses (MIT)
- [x] Commercial use is explicitly allowed
- [x] On-premise deployment is supported
- [x] No external API dependencies
- [x] No usage restrictions or metering
- [x] Source code modifications allowed
- [x] Can be bundled with proprietary software
- [x] Attribution requirements are minimal
- [x] No copyleft obligations
- [x] No patent concerns
- [x] GDPR/DSGVO compliant architecture
- [x] Suitable for sensitive data processing
- [x] No vendor lock-in

---

## Legal Disclaimer

This document provides information about the licenses of third-party libraries used in the ThemisDB Voice Assistant feature. It is provided for informational purposes only and does not constitute legal advice. For specific legal questions, please consult with a qualified attorney.

**Last Updated:** April 2026  
**Review:** Recommended annually or when updating library versions

---

## References

- [MIT License](https://opensource.org/licenses/MIT)
- [Whisper.cpp Repository](https://github.com/ggerganov/whisper.cpp)
- [Piper TTS Repository](https://github.com/rhasspy/piper)
- [llama.cpp Repository](https://github.com/ggerganov/llama.cpp)
- [ONNX Runtime Repository](https://github.com/microsoft/onnxruntime)
- [Open Source Initiative](https://opensource.org/)
- [SPDX License List](https://spdx.org/licenses/)

---

## Contact

For questions about licenses or enterprise deployment:
- **Technical:** GitHub Issues
- **Commercial:** sales@themisdb.com
- **Legal:** legal@themisdb.com
