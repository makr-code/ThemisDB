# C++ Bibliotheken für Bildanalyse-AI (SwarmUI Engine)

**Datum:** Dezember 2025  
**Kategorie:** Plugin Development / Image Analysis  
**Roadmap:** v1.6.0 (2026)  
**Sprache:** Deutsch

---

## Übersicht

Dieses Dokument evaluiert verfügbare C++ Projekte und Bibliotheken für die Integration einer Bildanalyse-AI in ThemisDB, basierend auf der SwarmUI zugrundeliegenden Engine (Stable Diffusion).

---

## 1. ONNX Runtime (Empfehlung: ⭐⭐⭐⭐⭐)

### Beschreibung
Cross-platform, hochperformante Inference-Engine für ONNX-Modelle. Unterstützt Stable Diffusion, CLIP, Vision Transformers und weitere.

### Lizenz
**MIT License** ✅ **KOMPATIBEL mit ThemisDB**

- **Quelle**: https://github.com/microsoft/onnxruntime/blob/main/LICENSE
- **Typ**: Permissive Open Source
- **Kompatibilität**: ✅ Vollständig kompatibel mit ThemisDB MIT + Government Clause
- **Kommerzielle Nutzung**: ✅ Erlaubt
- **Weitergabe**: ✅ Ohne Einschränkungen
- **Patent Grant**: ✅ Ja (Microsoft gewährt Patent-Rechte)

### Vorteile
- ✅ **Breite Modellunterstützung**: Stable Diffusion, CLIP, ResNet, ViT, etc.
- ✅ **Cross-Platform**: Windows, Linux, macOS
- ✅ **Multi-Backend**: CPU, CUDA, DirectML, TensorRT, OpenVINO
- ✅ **Produktionsreif**: Von Microsoft entwickelt und maintained
- ✅ **GGML-kompatibel**: Kann mit llama.cpp-ähnlichen Modellen arbeiten
- ✅ **Quantisierung**: INT8, FP16 für reduzierte Speichernutzung
- ✅ **Lizenzsicher**: MIT License, öffentlicher Sektor bedenkenlos nutzbar

### Nachteile
- ❌ Etwas größere Binärgröße (~50MB)
- ❌ Modelle müssen zu ONNX konvertiert werden

### Integration in ThemisDB
```cpp
#include <onnxruntime_cxx_api.h>

class ONNXImageAnalysisBackend : public IImageAnalysisBackend {
private:
    Ort::Env env_;
    std::unique_ptr<Ort::Session> clip_session_;
    std::unique_ptr<Ort::Session> sd_session_;
    
public:
    std::vector<float> generateEmbedding(const std::vector<uint8_t>& image_data) override;
    std::string generateCaption(const std::vector<uint8_t>& image_data) override;
    std::vector<uint8_t> generateImage(const std::string& prompt) override;
};
```

### Verfügbare Modelle
- **CLIP**: `openai/clip-vit-base-patch32.onnx`
- **Stable Diffusion**: `stable-diffusion-v1-5.onnx`
- **ResNet**: `resnet50-v2-7.onnx`
- **Vision Transformer**: `vit-base-patch16-224.onnx`

### vcpkg Integration
```json
{
  "name": "onnxruntime",
  "version": "1.17.0",
  "features": ["cuda", "tensorrt"]
}
```

### Performance
- **Inferenz**: ~50-200ms für CLIP embedding (GPU)
- **GPU-Speicher**: 2-8GB je nach Modell
- **CPU-Fallback**: 2-10x langsamer

---

## 2. OpenCV DNN Module (Empfehlung: ⭐⭐⭐⭐)

### Beschreibung
Deep Learning Modul in OpenCV. Unterstützt verschiedene Frameworks (TensorFlow, PyTorch, Caffe, ONNX).

### Lizenz
**Apache 2.0 License** ✅ **KOMPATIBEL mit ThemisDB**

- **Quelle**: https://github.com/opencv/opencv/blob/4.x/LICENSE
- **Typ**: Permissive Open Source
- **Kompatibilität**: ✅ Vollständig kompatibel mit ThemisDB MIT + Government Clause
- **Kommerzielle Nutzung**: ✅ Erlaubt
- **Weitergabe**: ✅ Mit Apache 2.0 Notice
- **Patent Grant**: ✅ Ja (expliziter Patent-Schutz)
- **Trademark**: ⚠️ OpenCV-Name geschützt, aber Nutzung erlaubt

### Vorteile
- ✅ **Bereits integriert**: OpenCV ist wahrscheinlich bereits dependency
- ✅ **Leichtgewichtig**: Keine zusätzlichen großen Dependencies
- ✅ **CPU-optimiert**: Gute CPU-Performance
- ✅ **Einfache API**: Einfach zu verwenden
- ✅ **Lizenzsicher**: Apache 2.0, ideal für öffentlichen Sektor

### Nachteile
- ❌ **Eingeschränkte GPU-Unterstützung**: Nur CUDA backend
- ❌ **Weniger Modelle**: Nicht alle State-of-the-Art Modelle verfügbar
- ❌ **Langsamere Updates**: Weniger frequent aktualisiert

### Integration in ThemisDB
```cpp
#include <opencv2/dnn/dnn.hpp>

class OpenCVImageAnalysisBackend : public IImageAnalysisBackend {
private:
    cv::dnn::Net clip_net_;
    cv::dnn::Net detection_net_;
    
public:
    std::vector<float> generateEmbedding(const std::vector<uint8_t>& image_data) override {
        cv::Mat img = cv::imdecode(image_data, cv::IMREAD_COLOR);
        cv::Mat blob = cv::dnn::blobFromImage(img, 1.0/255.0, cv::Size(224, 224));
        clip_net_.setInput(blob);
        cv::Mat output = clip_net_.forward();
        return std::vector<float>(output.begin<float>(), output.end<float>());
    }
};
```

### Verfügbare Modelle
- **ResNet**: Classification
- **YOLO**: Object Detection
- **MobileNet-SSD**: Efficient detection
- **EAST**: Text detection

### vcpkg Integration
```json
{
  "name": "opencv",
  "version": "4.8.0",
  "features": ["dnn", "cuda"]
}
```

### Performance
- **Inferenz**: ~100-500ms (CPU), ~30-100ms (CUDA)
- **GPU-Speicher**: 1-4GB
- **CPU-Nutzung**: Optimiert für Intel/ARM CPUs

---

## 3. llama.cpp (Vision Branch) (Empfehlung: ⭐⭐⭐⭐⭐)

### Beschreibung
Erweiterte Version von llama.cpp mit Unterstützung für Vision-Language Modelle (LLaVA, MiniGPT-4, etc.).

### Lizenz
**MIT License** ✅ **KOMPATIBEL mit ThemisDB**

- **Quelle**: https://github.com/ggerganov/llama.cpp/blob/master/LICENSE
- **Typ**: Permissive Open Source
- **Kompatibilität**: ✅ Vollständig kompatibel mit ThemisDB MIT + Government Clause
- **Kommerzielle Nutzung**: ✅ Erlaubt
- **Weitergabe**: ✅ Ohne Einschränkungen
- **Patent Grant**: ⚠️ Keine explizite Patent-Klausel (aber MIT-typisch)
- **Besonderheit**: GGML (ggerganov's ML library) ebenfalls MIT

### Vorteile
- ✅ **Unified LLM+Vision**: Ein Framework für Text und Vision
- ✅ **GGML Quantisierung**: Extrem speichereffizient (4-bit, 5-bit, 8-bit)
- ✅ **CPU-optimiert**: Läuft gut auf CPU
- ✅ **Kleines Footprint**: Minimale Dependencies
- ✅ **Integration mit LLM**: Nahtlose Integration mit bestehendem LLM-System
- ✅ **Lizenzsicher**: MIT License, identisch mit ThemisDB-Basis

### Nachteile
- ❌ **Begrenzte Modelle**: Nur Vision-Language Modelle (LLaVA, etc.)
- ❌ **Experimentell**: Vision Support noch in Entwicklung
- ❌ **Kein reines Bildgeneration**: Fokus auf Image Understanding

### Integration in ThemisDB
```cpp
#include "llama.h"
#include "clip.h"

class LlamaCppVisionBackend : public IImageAnalysisBackend {
private:
    llama_model* llm_model_;
    clip_ctx* clip_ctx_;
    
public:
    std::string generateCaption(const std::vector<uint8_t>& image_data) override {
        // Load image into CLIP
        clip_image_u8 img = load_image_from_bytes(image_data);
        
        // Encode image
        float* image_embd = clip_encode_image(clip_ctx_, img);
        
        // Generate caption with LLM
        return llama_generate_with_image(llm_model_, image_embd);
    }
};
```

### Verfügbare Modelle
- **LLaVA 1.5**: 7B, 13B (image understanding)
- **LLaVA 1.6**: Improved version
- **MiniGPT-4**: Visual question answering
- **CogVLM**: Chinese vision-language model

### Integration
```bash
git submodule add https://github.com/ggerganov/llama.cpp.git third_party/llama.cpp
cd third_party/llama.cpp
git checkout vision-support
```

### Performance
- **Inferenz**: ~500-2000ms (CPU 7B model)
- **RAM**: 4-16GB je nach Quantisierung
- **GPU-Beschleunigung**: Über CUDA/Metal/Vulkan

---

## 4. TensorRT (Empfehlung: ⭐⭐⭐⭐)

### Beschreibung
NVIDIA's hochoptimierte Inference-Engine für GPUs.

### Lizenz
**NVIDIA Proprietary (Apache 2.0 für OSS Komponenten)** ⚠️ **TEILWEISE KOMPATIBEL**

- **Quelle**: https://developer.nvidia.com/tensorrt (Custom EULA)
- **Typ**: Proprietary mit Open Source Komponenten
- **Kompatibilität**: ⚠️ **Eingeschränkt kompatibel** - Beachte NVIDIA TensorRT EULA
- **Kommerzielle Nutzung**: ✅ Erlaubt (mit EULA-Akzeptanz)
- **Weitergabe**: ⚠️ Runtime darf verteilt werden, SDK nicht
- **Patent Grant**: ✅ Implizit durch NVIDIA
- **Besonderheit**: TensorRT OSS Komponenten sind Apache 2.0

**⚠️ WICHTIG für öffentlichen Sektor:**
- TensorRT Runtime (libnvinfer.so) darf frei verteilt werden
- TensorRT SDK Lizenz muss akzeptiert werden (kostenlos, aber proprietär)
- Empfehlung: Als **optionale Dependency** markieren, nicht als Pflicht
- Alternative: ONNX Runtime mit TensorRT Execution Provider

### Vorteile
- ✅ **Höchste Performance**: Beste GPU-Performance für NVIDIA GPUs
- ✅ **Optimierung**: Automatische Kernel-Fusion, FP16/INT8
- ✅ **Production-Ready**: Industriestandard für Inference
- ✅ **Modellunterstützung**: Breite Palette an Modellen

### Nachteile
- ❌ **NVIDIA-exklusiv**: Nur für NVIDIA GPUs
- ❌ **Komplex**: Steile Lernkurve
- ❌ **Modell-Konvertierung**: Aufwändige Engine-Konvertierung
- ❌ **Lizenz**: Proprietäre EULA (nicht vollständig Open Source)

### Integration in ThemisDB
```cpp
#include <NvInfer.h>

class TensorRTImageAnalysisBackend : public IImageAnalysisBackend {
private:
    nvinfer1::IRuntime* runtime_;
    nvinfer1::ICudaEngine* engine_;
    nvinfer1::IExecutionContext* context_;
    
public:
    std::vector<float> generateEmbedding(const std::vector<uint8_t>& image_data) override;
};
```

### Verfügbare Modelle
- Alle ONNX/PyTorch Modelle nach Konvertierung
- Optimierte Versionen von Stable Diffusion, CLIP, etc.

### vcpkg Integration
```bash
# Manuell installieren (NVIDIA SDK erforderlich)
# https://developer.nvidia.com/tensorrt
```

### Performance
- **Inferenz**: ~20-50ms (RTX 3090)
- **GPU-Speicher**: 2-8GB optimiert
- **Speedup**: 2-5x schneller als ONNX Runtime

---

## 5. OpenVINO (Empfehlung: ⭐⭐⭐)

### Beschreibung
Intel's Optimization Toolkit für Deep Learning Inferenz auf Intel-Hardware.

### Lizenz
**Apache 2.0 License** ✅ **KOMPATIBEL mit ThemisDB**

- **Quelle**: https://github.com/openvinotoolkit/openvino/blob/master/LICENSE
- **Typ**: Permissive Open Source
- **Kompatibilität**: ✅ Vollständig kompatibel mit ThemisDB MIT + Government Clause
- **Kommerzielle Nutzung**: ✅ Erlaubt
- **Weitergabe**: ✅ Mit Apache 2.0 Notice
- **Patent Grant**: ✅ Ja (expliziter Patent-Schutz)
- **Besonderheit**: Intel gewährt umfassende Patent-Rechte

### Vorteile
- ✅ **Intel-optimiert**: Beste Performance auf Intel CPUs/GPUs
- ✅ **Heterogenous**: CPU, iGPU, VPU, FPGA Support
- ✅ **Model Zoo**: Viele vorkompilierte Modelle
- ✅ **Quantisierung**: INT8 Optimierung
- ✅ **Lizenzsicher**: Apache 2.0, ideal für öffentlichen Sektor

### Nachteile
- ❌ **Intel-fokussiert**: Weniger optimal auf AMD/ARM
- ❌ **Größe**: Große SDK-Installation
- ❌ **Komplexität**: Steile Lernkurve

### Integration in ThemisDB
```cpp
#include <openvino/openvino.hpp>

class OpenVINOImageAnalysisBackend : public IImageAnalysisBackend {
private:
    ov::Core core_;
    std::shared_ptr<ov::Model> model_;
    ov::CompiledModel compiled_model_;
    
public:
    std::vector<float> generateEmbedding(const std::vector<uint8_t>& image_data) override;
};
```

### Verfügbare Modelle
- **OpenVINO Model Zoo**: Hunderte vortrainierte Modelle
- CLIP, ResNet, MobileNet, YOLO, etc.

### vcpkg Integration
```json
{
  "name": "openvino",
  "version": "2023.2.0"
}
```

### Performance
- **Inferenz**: ~50-150ms (Intel CPU)
- **RAM**: 2-8GB
- **Intel iGPU**: Gute Acceleration auf Laptops

---

## 6. ncnn (Empfehlung: ⭐⭐⭐)

### Beschreibung
Tencent's leichtgewichtiges Neural Network Framework, optimiert für mobile Geräte.

### Lizenz
**BSD 3-Clause License** ✅ **KOMPATIBEL mit ThemisDB**

- **Quelle**: https://github.com/Tencent/ncnn/blob/master/LICENSE.txt
- **Typ**: Permissive Open Source
- **Kompatibilität**: ✅ Vollständig kompatibel mit ThemisDB MIT + Government Clause
- **Kommerzielle Nutzung**: ✅ Erlaubt
- **Weitergabe**: ✅ Mit BSD Notice
- **Patent Grant**: ⚠️ Keine explizite Patent-Klausel
- **Besonderheit**: Von Tencent entwickelt und maintained

### Vorteile
- ✅ **Leichtgewicht**: Sehr kleine Binärgröße (~500KB)
- ✅ **Mobile-optimiert**: Läuft gut auf ARM/embedded
- ✅ **Vulkan Backend**: Cross-platform GPU support
- ✅ **Einfache API**: Sehr einfach zu integrieren
- ✅ **Lizenzsicher**: BSD 3-Clause, sehr permissiv

### Nachteile
- ❌ **Eingeschränkte Modelle**: Weniger SOTA-Modelle verfügbar
- ❌ **Dokumentation**: Hauptsächlich Chinesisch
- ❌ **Weniger Features**: Keine advanced features wie dynamic shapes

### Integration in ThemisDB
```cpp
#include "net.h"

class NCNNImageAnalysisBackend : public IImageAnalysisBackend {
private:
    ncnn::Net clip_net_;
    
public:
    std::vector<float> generateEmbedding(const std::vector<uint8_t>& image_data) override {
        ncnn::Mat in = ncnn::Mat::from_pixels(
            image_data.data(), ncnn::Mat::PIXEL_RGB, 224, 224
        );
        
        ncnn::Extractor ex = clip_net_.create_extractor();
        ex.input("input", in);
        
        ncnn::Mat out;
        ex.extract("output", out);
        
        return std::vector<float>(out.channel(0), out.channel(0) + out.w);
    }
};
```

### Verfügbare Modelle
- MobileNet, SqueezeNet
- YOLO (mobile versions)
- Einige CLIP Varianten

### vcpkg Integration
```json
{
  "name": "ncnn",
  "version": "20231027"
}
```

### Performance
- **Inferenz**: ~100-300ms (ARM CPU)
- **RAM**: <500MB
- **Vulkan**: Gute GPU-Beschleunigung

---

## 7. libtorch (PyTorch C++ Frontend) (Empfehlung: ⭐⭐⭐)

### Beschreibung
Offizielle C++ API für PyTorch.

### Lizenz
**BSD 3-Clause License (Modified)** ✅ **KOMPATIBEL mit ThemisDB**

- **Quelle**: https://github.com/pytorch/pytorch/blob/main/LICENSE
- **Typ**: Permissive Open Source (Modified BSD)
- **Kompatibilität**: ✅ Vollständig kompatibel mit ThemisDB MIT + Government Clause
- **Kommerzielle Nutzung**: ✅ Erlaubt
- **Weitergabe**: ✅ Mit BSD Notice
- **Patent Grant**: ✅ Ja (Facebook/Meta gewährt Patent-Rechte)
- **Besonderheit**: Zusätzliche Patent-Schutz-Klausel (ähnlich Apache 2.0)

### Vorteile
- ✅ **PyTorch Ecosystem**: Direkter Zugriff auf alle PyTorch Modelle
- ✅ **TorchScript**: Einfache Konvertierung von Python Modellen
- ✅ **Dynamic Graphs**: Flexible Modell-Architekturen
- ✅ **Breite Modellunterstützung**: Alle State-of-the-Art Modelle
- ✅ **Lizenzsicher**: BSD mit Patent-Schutz, sehr sicher für Unternehmen

### Nachteile
- ❌ **Große Binärgröße**: >200MB
- ❌ **Komplexe Dependencies**: Viele transitive Dependencies
- ❌ **Speicherhunger**: Höhere RAM-Nutzung als ONNX

### Integration in ThemisDB
```cpp
#include <torch/script.h>

class LibTorchImageAnalysisBackend : public IImageAnalysisBackend {
private:
    torch::jit::script::Module clip_module_;
    
public:
    std::vector<float> generateEmbedding(const std::vector<uint8_t>& image_data) override {
        // Load image tensor
        auto img_tensor = torch::from_blob(
            const_cast<uint8_t*>(image_data.data()),
            {1, 3, 224, 224},
            torch::kUInt8
        ).to(torch::kFloat32);
        
        // Inference
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(img_tensor);
        auto output = clip_module_.forward(inputs).toTensor();
        
        return std::vector<float>(output.data_ptr<float>(), 
                                 output.data_ptr<float>() + output.numel());
    }
};
```

### Verfügbare Modelle
- Alle PyTorch Hub Modelle
- Hugging Face Transformers (CLIP, ViT, etc.)
- Stable Diffusion via diffusers

### vcpkg Integration
```bash
# Download libtorch manually
wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.1.0%2Bcpu.zip
```

### Performance
- **Inferenz**: ~100-300ms (GPU)
- **GPU-Speicher**: 4-12GB
- **RAM**: 2-8GB

---

## Vergleichstabelle

| Bibliothek | Performance | Modellauswahl | Integration | Speicher | GPU-Support | Lizenz | Empfehlung |
|------------|-------------|---------------|-------------|----------|-------------|--------|------------|
| **ONNX Runtime** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ✅ MIT | **Best Overall** |
| **llama.cpp Vision** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ✅ MIT | **Best LLM Integration** |
| **OpenCV DNN** | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ✅ Apache 2.0 | **Best Simplicity** |
| **TensorRT** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⚠️ Proprietary | **Best NVIDIA Performance** |
| **OpenVINO** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ✅ Apache 2.0 | **Best Intel Performance** |
| **ncnn** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ✅ BSD 3-Clause | **Best Mobile/Embedded** |
| **libtorch** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | ✅ BSD (Modified) | **Best Research** |

---

## Lizenz-Kompatibilitätszusammenfassung

### ✅ Vollständig kompatibel (Empfohlen für öffentlichen Sektor)

Diese Bibliotheken sind **ohne Einschränkungen** mit ThemisDB MIT + Government Clause kompatibel:

1. **ONNX Runtime** - MIT License
   - ✅ Identische Lizenz-Philosophie wie ThemisDB
   - ✅ Patent Grant von Microsoft
   - ✅ Keine Trademark-Einschränkungen
   - ✅ **EMPFOHLEN** für Produktion

2. **llama.cpp (Vision)** - MIT License
   - ✅ Identische Lizenz wie ThemisDB
   - ✅ Nahtlose Integration möglich
   - ✅ **EMPFOHLEN** für LLM+Vision Unified Architecture

3. **OpenCV** - Apache 2.0 License
   - ✅ Sehr permissiv, kompatibel mit MIT
   - ✅ Expliziter Patent-Schutz
   - ✅ **EMPFOHLEN** als CPU-Fallback

4. **OpenVINO** - Apache 2.0 License
   - ✅ Intel gewährt umfassende Rechte
   - ✅ Ideal für Intel-Hardware
   - ✅ **EMPFOHLEN** für Intel-Optimierung

5. **ncnn** - BSD 3-Clause License
   - ✅ Sehr permissiv
   - ✅ Keine Einschränkungen
   - ✅ **EMPFOHLEN** für Embedded/Mobile

6. **libtorch** - BSD 3-Clause (Modified)
   - ✅ Zusätzlicher Patent-Schutz
   - ✅ Facebook/Meta gewährt Rechte
   - ✅ **EMPFOHLEN** für Forschung

### ⚠️ Teilweise kompatibel (Mit Vorsicht verwenden)

1. **TensorRT** - NVIDIA Proprietary (mit Apache 2.0 Komponenten)
   - ⚠️ Runtime darf verteilt werden
   - ⚠️ SDK Lizenz muss akzeptiert werden (kostenlos)
   - ⚠️ Nicht vollständig Open Source
   - **Empfehlung**: Als **optionale Dependency** integrieren
   - **Alternative**: ONNX Runtime mit TensorRT Execution Provider

### Lizenz-Hinweise für öffentlichen Sektor

**ThemisDB MIT License + Government Clause** bedeutet:

1. **Digitale Souveränität**: Öffentliche Verwaltung hat volle Kontrolle
2. **Keine Vendor Lock-in**: Alle empfohlenen Bibliotheken sind Open Source
3. **Patent-Schutz**: MIT + Apache 2.0 bieten guten Patent-Schutz
4. **Weitergabe**: Alle Änderungen müssen unter gleicher Lizenz veröffentlicht werden

**Empfehlung für öffentliche Auftraggeber:**

✅ **Primär verwenden**: ONNX Runtime, llama.cpp, OpenCV
- Alle MIT oder Apache 2.0
- Vollständig Open Source
- Keine proprietären Abhängigkeiten

⚠️ **Sekundär/Optional**: TensorRT (nur wenn NVIDIA GPU vorhanden)
- Als Performance-Optimierung
- Nicht als Kern-Dependency
- ONNX Runtime + TensorRT EP als Alternative

❌ **Vermeiden**: Proprietäre Lösungen ohne Open Source Alternative
- Keine Cloud-APIs (AWS Rekognition, Google Vision, etc.)
- Keine geschlossenen SDKs ohne Quellcode

---

## Empfehlung für ThemisDB

### Primärer Ansatz: **ONNX Runtime + llama.cpp Vision**

**Begründung:**
1. **ONNX Runtime** für dedizierte Bildanalyse (CLIP, Stable Diffusion)
   - Beste Cross-Platform-Unterstützung
   - Breite Hardware-Backend-Auswahl (CPU, CUDA, DirectML, TensorRT)
   - Produktionsreif und gut maintained

2. **llama.cpp Vision** für Vision-Language Modelle
   - Nahtlose Integration mit bestehendem LLM-System
   - Gemeinsame Memory-Infrastruktur
   - Speichereffizient durch GGML Quantisierung

3. **OpenCV DNN** als CPU-Fallback
   - Minimale Dependencies
   - Bereits in vielen Systemen vorhanden
   - Gute CPU-Performance

### Plugin-Architektur

```
┌─────────────────────────────────────────────────────┐
│           ThemisDB Core                             │
│  ┌────────────────────────────────────────────┐    │
│  │       Plugin Manager                        │    │
│  └────────────┬────────────────────────────────┘    │
│               │                                      │
│  ┌────────────┴────────────────────────────────┐    │
│  │     Image Analysis Backend Registry         │    │
│  └────┬───────┬───────┬──────────┬────────────┘    │
│       │       │       │          │                  │
│  ┌────▼──┐ ┌─▼────┐ ┌▼──────┐ ┌─▼────────┐        │
│  │ ONNX  │ │llama │ │OpenCV │ │TensorRT  │        │
│  │Runtime│ │.cpp  │ │ DNN   │ │(optional)│        │
│  │Plugin │ │Vision│ │Plugin │ │  Plugin  │        │
│  └───────┘ └──────┘ └───────┘ └──────────┘        │
│                                                      │
│  ┌──────────────────────────────────────────┐      │
│  │    Content Manager (Image Processor)     │      │
│  └──────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────┘
```

### Implementierungsschritte

1. **Phase 1**: Plugin Interface definieren
   - `IImageAnalysisBackend` interface
   - Standard Operationen: embedding, caption, detect, generate

2. **Phase 2**: ONNX Runtime Backend
   - CLIP Embeddings
   - Image Classification
   - Object Detection

3. **Phase 3**: llama.cpp Vision Backend
   - LLaVA Integration
   - Image Captioning
   - Visual Question Answering

4. **Phase 4**: Advanced Features
   - Stable Diffusion Image Generation
   - Image Segmentation
   - OCR Integration

---

## SwarmUI Integration

SwarmUI basiert auf **Stable Diffusion** (via diffusers/AUTOMATIC1111). Für ThemisDB:

### Option 1: ONNX Runtime mit Stable Diffusion ONNX
```cpp
// Stable Diffusion Pipeline in ONNX
class StableDiffusionPlugin : public IImageAnalysisBackend {
    // Text Encoder (CLIP)
    Ort::Session text_encoder_;
    
    // UNet (Denoising)
    Ort::Session unet_;
    
    // VAE Decoder
    Ort::Session vae_decoder_;
    
public:
    std::vector<uint8_t> generateImage(const std::string& prompt) override;
};
```

### Option 2: External Process Integration
```cpp
// SwarmUI als separate Process
class SwarmUIBridge : public IImageAnalysisBackend {
    // HTTP/gRPC Kommunikation mit SwarmUI
    std::string swarmui_url_;
    
public:
    std::vector<uint8_t> generateImage(const std::string& prompt) override {
        // POST request to SwarmUI API
        return http_client_.post(swarmui_url_ + "/generate", {{"prompt", prompt}});
    }
};
```

**Empfehlung**: Option 1 für embedded, Option 2 für externe SwarmUI-Instanz.

---

## Nächste Schritte

1. ✅ Dieses Dokument erstellen
2. ⏳ Plugin Interface implementieren (`include/plugins/image_analysis_interface.h`)
3. ⏳ ONNX Runtime Backend implementieren
4. ⏳ Integration Tests schreiben
5. ⏳ Dokumentation für Plugin-Entwickler
6. ⏳ Example Models bereitstellen

---

## Referenzen

- ONNX Runtime: https://github.com/microsoft/onnxruntime
- llama.cpp: https://github.com/ggerganov/llama.cpp
- OpenCV: https://github.com/opencv/opencv
- TensorRT: https://developer.nvidia.com/tensorrt
- OpenVINO: https://github.com/openvinotoolkit/openvino
- ncnn: https://github.com/Tencent/ncnn
- SwarmUI: https://github.com/mcmonkeyprojects/SwarmUI
- Stable Diffusion ONNX: https://github.com/huggingface/diffusers/tree/main/examples/community#stable-diffusion-onnx
