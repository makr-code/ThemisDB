# Sprachassistent - Integrationsanleitung für Whisper.cpp und Piper TTS

**Version:** 1.0  
**Datum:** Dezember 2025  
**Status:** Implementierungsanleitung

---

## Übersicht

Dieses Dokument bietet Schritt-für-Schritt-Anleitungen zur Integration tatsächlicher Whisper.cpp- und Piper TTS-Modelle mit dem ThemisDB Sprachassistenten.

---

## Voraussetzungen

### Systemanforderungen

- **CMake** 3.20 oder höher
- **C++20** kompatibler Compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **Git** zum Klonen der Repositories
- **ONNX Runtime** für Piper TTS (optional, kann gebündelt werden)

### Optional (für GPU-Beschleunigung)

- **CUDA Toolkit** 11.x oder 12.x (NVIDIA GPUs)
- **cuBLAS** (kommt mit CUDA)

---

## Schritt 1: Whisper.cpp klonen und bauen

### 1.1 Whisper.cpp klonen

```bash
cd /pfad/zu/ThemisDB
mkdir -p external
cd external
git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
```

### 1.2 Whisper.cpp bauen

**Nur CPU:**
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Mit GPU (CUDA):**
```bash
mkdir build && cd build
cmake .. -DWHISPER_CUBLAS=ON
cmake --build . --config Release
```

### 1.3 Whisper-Modelle herunterladen

```bash
cd /pfad/zu/ThemisDB/external/whisper.cpp

# Base-Modell herunterladen (empfohlen für den Start)
bash ./models/download-ggml-model.sh base

# Oder andere Modelle:
# bash ./models/download-ggml-model.sh tiny    # Schnellstes, am wenigsten genau
# bash ./models/download-ggml-model.sh small   # Gute Balance
# bash ./models/download-ggml-model.sh medium  # Bessere Genauigkeit
# bash ./models/download-ggml-model.sh large-v3 # Beste Genauigkeit
```

---

## Schritt 2: Piper TTS klonen und bauen

### 2.1 Piper TTS klonen

```bash
cd /pfad/zu/ThemisDB/external
git clone https://github.com/rhasspy/piper.git
cd piper
```

### 2.2 Piper TTS bauen

**Abhängigkeiten installieren:**

Ubuntu/Debian:
```bash
sudo apt-get install libespeak-ng-dev libonnxruntime-dev
```

macOS:
```bash
brew install espeak-ng onnxruntime
```

**Piper bauen:**
```bash
cd src/cpp
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 2.3 Piper Stimmenmodelle herunterladen

```bash
cd /pfad/zu/ThemisDB
mkdir -p models/voices

# Deutsche Stimme herunterladen (Thorsten - Männlich)
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx \
     -O models/voices/de_DE-thorsten-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx.json \
     -O models/voices/de_DE-thorsten-medium.onnx.json

# Englische Stimme (Amy - US Weiblich)
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx \
     -O models/voices/en_US-amy-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx.json \
     -O models/voices/en_US-amy-medium.onnx.json
```

Weitere Stimmen verfügbar unter: https://huggingface.co/rhasspy/piper-voices/tree/main

---

## Schritt 3: ThemisDB mit Sprachassistent bauen

### 3.1 CMake konfigurieren

```bash
cd /pfad/zu/ThemisDB
mkdir -p build && cd build

cmake .. \
  -DTHEMIS_ENABLE_VOICE_ASSISTANT=ON \
  -DTHEMIS_ENABLE_WHISPER=ON \
  -DTHEMIS_ENABLE_PIPER_TTS=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DWHISPER_ROOT=/pfad/zu/ThemisDB/external/whisper.cpp \
  -DPIPER_ROOT=/pfad/zu/ThemisDB/external/piper/src/cpp \
  -DCMAKE_BUILD_TYPE=Release
```

### 3.2 ThemisDB bauen

```bash
cmake --build . --config Release -j$(nproc)
```

---

## Schritt 4: Sprachassistent konfigurieren

### 4.1 Konfigurationsdateien aktualisieren

`config/processors/stt.yaml` bearbeiten:

```yaml
processor:
  model:
    # Zeigen Sie auf Ihr heruntergeladenes Whisper-Modell
    path: "./models/ggml-base.bin"
    size: "base"
    auto_download: false
```

`config/processors/tts.yaml` bearbeiten:

```yaml
processor:
  model:
    # Zeigen Sie auf Ihre heruntergeladene Piper-Stimme
    path: "./models/voices/de_DE-thorsten-medium.onnx"
    engine: "piper"
    auto_download: false
```

`config/voice_assistant.yaml` bearbeiten:

```yaml
voice_assistant:
  enabled: true
  
  stt:
    model_path: "./models/ggml-base.bin"
    model_size: "base"
    language: "auto"
  
  tts:
    model_path: "./models/voices/de_DE-thorsten-medium.onnx"
    voice: "de_DE-thorsten-medium"
  
  llm:
    model_path: "./models/llama-2-7b-chat.gguf"
    n_ctx: 4096
```

---

## Schritt 5: Integration testen

### 5.1 ThemisDB Server starten

```bash
cd /pfad/zu/ThemisDB/build
./themis_server --config ../config/themis.yaml --enable-voice-assistant
```

### 5.2 STT (Speech-to-Text) testen

```bash
# Python-Beispiel verwenden
cd /pfad/zu/ThemisDB
python examples/voice_assistant_example.py
```

Oder curl verwenden:

```bash
# Test-Audio vorbereiten
base64 test_audio.wav > audio_base64.txt

# Transkriptions-API aufrufen
curl -X POST http://localhost:8080/api/v1/voice/transcribe \
  -H "Authorization: Bearer IHR_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"audio_base64\": \"$(cat audio_base64.txt)\", \"language\": \"de\"}"
```

### 5.3 TTS (Text-to-Speech) testen

```bash
curl -X POST http://localhost:8080/api/v1/voice/synthesize \
  -H "Authorization: Bearer IHR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"text": "Hallo, hier ist der ThemisDB Sprachassistent", "voice": "default", "return_base64": true}' \
  | jq -r '.audio_base64' | base64 -d > ausgabe.wav

# Generierte Audio abspielen
aplay ausgabe.wav  # Linux
afplay ausgabe.wav # macOS
```

---

## Fehlerbehebung

### Problem: "Whisper model not loaded"

**Lösung:**
1. Überprüfen Sie, ob die Modelldatei am konfigurierten Pfad existiert
2. Dateiberechtigungen prüfen
3. Sicherstellen, dass CMake die Whisper.cpp-Bibliothek während des Builds gefunden hat
4. Server-Logs auf detaillierte Fehlermeldungen überprüfen

### Problem: "Piper TTS synthesis failed"

**Lösung:**
1. ONNX-Modell und .json-Konfigurationsdateien überprüfen
2. Sicherstellen, dass ONNX Runtime installiert ist
3. ONNX-Modell-Kompatibilität prüfen (sollte Piper-Format sein)
4. Ausreichend verfügbaren Speicher überprüfen

---

## Performance-Optimierung

### GPU-Beschleunigung

Für NVIDIA GPUs mit CUDA-Unterstützung bauen:

```bash
cmake .. \
  -DTHEMIS_ENABLE_VOICE_ASSISTANT=ON \
  -DTHEMIS_ENABLE_WHISPER=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DWHISPER_CUBLAS=ON
```

Konfiguration aktualisieren:
```yaml
stt:
  performance:
    use_gpu: true
    gpu_device_id: 0
```

### Modellauswahl für Performance

| Modell | Geschwindigkeit | Genauigkeit | RAM | Anwendungsfall |
|--------|----------------|-------------|-----|----------------|
| tiny   | 4x RT | Gut | 1GB | Echtzeit, geringe Ressourcen |
| base   | 1x RT | Besser | 1GB | Ausgewogen (empfohlen) |
| small  | 0.5x RT | Hoch | 2GB | Hohe Genauigkeit erforderlich |
| medium | 0.3x RT | Sehr hoch | 5GB | Maximale Genauigkeit |
| large  | 0.2x RT | Beste | 10GB | Forschung/Archivierung |

*RT = Echtzeit (1x RT = 1 Minute Audio = 1 Minute Verarbeitung)*

---

## Produktions-Deployment

### Checkliste

- [ ] Modelle heruntergeladen und konfiguriert
- [ ] Build erfolgreich mit aktiviertem Sprachassistent abgeschlossen
- [ ] Konfigurationsdateien mit korrekten Pfaden aktualisiert
- [ ] API-Authentifizierung konfiguriert
- [ ] Speicherpfade für Aufzeichnungen konfiguriert
- [ ] Revisionskontrolle in ThemisDB aktiviert
- [ ] Transkription mit Beispiel-Audio getestet
- [ ] Synthese mit Beispieltext getestet
- [ ] Komplette Anrufaufzeichnungs-Pipeline getestet
- [ ] Lasttests abgeschlossen
- [ ] Monitoring konfiguriert
- [ ] Backup-Strategie vorhanden

---

## Support

Für Probleme oder Fragen:
- **Dokumentation:** [Sprachassistent Anleitung](sprachassistent_anleitung.md)
- **Whisper.cpp:** https://github.com/ggerganov/whisper.cpp
- **Piper TTS:** https://github.com/rhasspy/piper
- **ThemisDB:** GitHub Issues

---

## Lizenz

Integration verwendet MIT-lizenzierte Bibliotheken:
- Whisper.cpp: MIT-Lizenz
- Piper TTS: MIT-Lizenz
- ONNX Runtime: MIT-Lizenz

Siehe [Lizenzdokumentation](sprachassistent_lizenzen.md) für Details.
