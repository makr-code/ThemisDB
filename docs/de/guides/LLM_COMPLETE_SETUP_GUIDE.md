---
category: "📋 Guides"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🧠 Kompletter LLM Setup und Inferencing Guide

**Vollständige Anleitung zum Laden eines LLM in ThemisDB und Durchführen von Inferencing an Beispieldaten**

---

## 📋 Inhaltsverzeichnis

- [Überblick](#-überblick)
- [Voraussetzungen](#-voraussetzungen)
- [Setup-Optionen](#-setup-optionen)
- [Schritt 1: ThemisDB mit LLM-Support bauen](#schritt-1-themisdb-mit-llm-support-bauen)
- [Schritt 2: Modell laden](#schritt-2-modell-laden)
- [Schritt 3: Inferencing durchführen](#schritt-3-inferencing-durchführen)
- [Schritt 4: RAG-Workflow mit Beispieldaten](#schritt-4-rag-workflow-mit-beispieldaten)
- [Erweiterte Themen](#-erweiterte-themen)
- [Troubleshooting](#-troubleshooting)
- [Weiterführende Dokumentation](#-weiterführende-dokumentation)

---

## 🎯 Überblick

ThemisDB bietet ab Version 1.3.0 eine **optionale eingebettete LLM-Engine** basierend auf **llama.cpp**. Diese Anleitung führt Sie durch:

1. ✅ **Build** von ThemisDB mit LLM-Support
2. ✅ **Laden** eines LLM-Modells (z.B. Llama, Mistral, Phi-3)
3. ✅ **Durchführen** von Inferencing-Anfragen
4. ✅ **RAG-Workflows** mit Beispieldaten (Retrieval-Augmented Generation)

### Was Sie lernen werden

- 🔧 Wie Sie ThemisDB mit aktiviertem LLM-Feature kompilieren
- 📦 Wie Sie GGUF-Modelle von Hugging Face herunterladen
- 🚀 Wie Sie Modelle in ThemisDB laden und verwalten
- 💬 Wie Sie Text-Generierung und Chat-Completion durchführen
- 🔍 Wie Sie RAG (Retrieval-Augmented Generation) implementieren
- ⚡ Wie Sie GPU-Beschleunigung nutzen

> **💡 Hinweis**: Das LLM-Feature ist **optional** und erfordert:
> - Build-Flag: `-DTHEMIS_ENABLE_LLM=ON`
> - Externe Abhängigkeit: llama.cpp (separat klonen)
> - Min. 8 GB RAM (16 GB empfohlen für größere Modelle)

---

## 🔧 Voraussetzungen

### Hardware

**Minimum (CPU-Only):**
- 8 GB RAM (für kleine Modelle wie Phi-3 Mini 3B)
- 4 CPU-Kerne
- 10 GB freier Speicherplatz

**Empfohlen (mit GPU):**
- 16 GB RAM
- NVIDIA GPU mit 8+ GB VRAM (für GPU-Beschleunigung)
- 8 CPU-Kerne
- 50 GB freier Speicherplatz

**Enterprise (große Modelle):**
- 32+ GB RAM
- NVIDIA GPU mit 24+ GB VRAM (für Modelle wie Llama-70B)
- 16+ CPU-Kerne
- 100+ GB freier Speicherplatz

### Software

- **CMake** 3.20+
- **C++ Compiler** mit C++20 Support (GCC 11+, Clang 13+, MSVC 2022)
- **Git**
- **vcpkg** (automatisch von ThemisDB-Skripten eingerichtet)
- **CUDA Toolkit** 11.8+ (optional, für GPU-Beschleunigung)

### Betriebssysteme

- ✅ Linux (Ubuntu 20.04+, Debian 11+)
- ✅ Windows 10/11 (mit MSVC 2022)
- ✅ macOS 12+ (mit Metal-Support)

---

## 🎮 Setup-Optionen

Es gibt drei Hauptoptionen für das Setup:

| Option | Vorteile | Ideal für |
|--------|----------|-----------|
| **🐳 Docker mit vLLM** | Einfachste Installation, kein Build nötig | Schneller Start, Produktion |
| **🔨 Source Build** | Volle Kontrolle, optimiert für Hardware | Entwicklung, Performance-Tuning |
| **📦 Hybrid (ThemisDB + Ollama)** | ThemisDB ohne LLM + externe LLM-API | Einfaches Setup, Flexibilität |

### Entscheidungshilfe

**Wählen Sie Docker**, wenn:
- Sie schnell starten möchten
- Sie keine Entwicklung an ThemisDB selbst machen
- Sie einen Produktions-Setup benötigen

**Wählen Sie Source Build**, wenn:
- Sie maximale Performance benötigen
- Sie spezifische Optimierungen vornehmen möchten
- Sie an ThemisDB entwickeln

**Wählen Sie Hybrid**, wenn:
- Sie ThemisDB bereits haben und nur LLM-Features testen möchten
- Sie verschiedene LLM-Backends ausprobieren wollen
- Sie Ollama bevorzugen

---

## Schritt 1: ThemisDB mit LLM-Support bauen

### Option A: Docker Setup (Empfohlen für Schnellstart)

```bash
# 1. Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Docker Compose mit vLLM verwenden
docker compose -f docker-compose-vllm.yml up -d

# 3. Status überprüfen
docker compose -f docker-compose-vllm.yml ps

# 4. Logs ansehen
docker compose -f docker-compose-vllm.yml logs -f themisdb
```

**Was passiert:**
- ThemisDB startet auf Port `8765` (Binary) und `8080` (HTTP)
- vLLM startet auf Port `8000` (optional für externe LLM-API)
- Automatisches Setup der Integration

> **📝 Siehe auch:** [Docker Deployment Guide](../deployment/DOCKER_DEPLOYMENT.md)

### Option B: Source Build (Linux/macOS)

```bash
# 1. Repository und Abhängigkeiten klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. llama.cpp klonen (erforderlich für LLM-Support)
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp
cd llama.cpp
git checkout b1698  # Stabiles Release
cd ..

# 3. Setup-Skript ausführen
./scripts/setup.sh

# 4. Build mit LLM-Support
mkdir -p build
cd build

# CPU-Only Build
cmake .. -DTHEMIS_ENABLE_LLM=ON

# GPU-beschleunigter Build (CUDA)
cmake .. -DTHEMIS_ENABLE_LLM=ON -DGGML_CUDA=ON

# Kompilieren (nutzt alle CPU-Kerne)
cmake --build . --config Release -j$(nproc)

# 5. Installation (optional)
sudo cmake --install .
```

**Build-Zeit:** 10-30 Minuten (abhängig von Hardware)

### Option C: Windows Build (PowerShell)

```powershell
# 1. Repository und Abhängigkeiten klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. llama.cpp klonen
if (!(Test-Path "llama.cpp")) {
    git clone https://github.com/ggerganov/llama.cpp.git llama.cpp
}

# 3. MSVC Build-Skript verwenden
powershell -File scripts/build-themis-server-llm.ps1

# 4. Executable testen
./build-msvc/bin/themis_server.exe --help
```

> **📝 Siehe auch:** 
> - [Build Strategy Guide](./guides_build_strategy.md)
> - [llama.cpp Integration](../llm/LLAMA_CPP_INTEGRATION.md)

### Build-Verifizierung

```bash
# Version und Features prüfen
./build/themis_server --version

# Sollte anzeigen:
# ThemisDB v1.3.0
# Features: LLM_SUPPORT=ON, CUDA_SUPPORT=ON

# Server starten (Test)
./build/themis_server --config config.yaml

# In anderem Terminal: Health-Check
curl http://localhost:8765/health
```

---

## Schritt 2: Modell laden

### 2.1 GGUF-Modell herunterladen

GGUF ist das bevorzugte Format für llama.cpp. Modelle sind auf Hugging Face verfügbar.

**Beliebte Modelle:**

| Modell | Größe | VRAM | Download |
|--------|-------|------|----------|
| **Phi-3 Mini** | 3B | 4 GB | [Link](https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf) |
| **Llama 3.2** | 3B | 4 GB | [Link](https://huggingface.co/meta-llama/Llama-3.2-3B-Instruct-gguf) |
| **Mistral 7B** | 7B | 8 GB | [Link](https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF) |
| **Llama 3.1** | 8B | 10 GB | [Link](https://huggingface.co/meta-llama/Llama-3.1-8B-Instruct-gguf) |

**Empfehlung für Einsteiger:** Phi-3 Mini (klein, schnell, gute Qualität)

```bash
# Ordner für Modelle erstellen
mkdir -p ~/models/gguf

# Beispiel: Phi-3 Mini herunterladen (Q4_K_M Quantisierung)
cd ~/models/gguf

# Mit wget
wget https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf

# ODER mit huggingface-cli (empfohlen)
pip install huggingface-hub
huggingface-cli download microsoft/Phi-3-mini-4k-instruct-gguf \
  Phi-3-mini-4k-instruct-q4.gguf \
  --local-dir ~/models/gguf
```

**Quantisierungs-Formate:**
- `Q4_K_M` - Guter Kompromiss (empfohlen)
- `Q5_K_M` - Höhere Qualität, mehr Speicher
- `Q8_0` - Beste Qualität, noch mehr Speicher
- `Q2_K` - Sehr klein, niedrigere Qualität

### 2.2 Modell in ThemisDB laden

Es gibt mehrere Wege, ein Modell zu laden:

#### Methode 1: REST API (empfohlen)

```bash
# Modell über REST API laden
curl -X POST http://localhost:8080/api/llm/load_model \
  -H "Content-Type: application/json" \
  -d '{
    "model_path": "/home/user/models/gguf/Phi-3-mini-4k-instruct-q4.gguf",
    "model_id": "phi3-mini",
    "context_size": 4096,
    "gpu_layers": 32,
    "options": {
      "temperature": 0.7,
      "top_p": 0.9,
      "top_k": 40
    }
  }'

# Antwort:
# {
#   "status": "success",
#   "model_id": "phi3-mini",
#   "loaded": true,
#   "memory_usage_mb": 2048
# }
```

**Parameter:**
- `model_path`: Absoluter Pfad zur GGUF-Datei
- `model_id`: Eindeutiger Identifier (frei wählbar)
- `context_size`: Kontext-Fenster (4096, 8192, 32768, etc.)
- `gpu_layers`: Anzahl Layer auf GPU (0 = CPU-only, -1 = alle)
- `options`: Sampling-Parameter

#### Methode 2: GraphQL

```graphql
mutation {
  loadLLMModel(
    modelPath: "/home/user/models/gguf/Phi-3-mini-4k-instruct-q4.gguf"
    modelId: "phi3-mini"
    contextSize: 4096
    gpuLayers: 32
  ) {
    status
    modelId
    loaded
    memoryUsageMb
  }
}
```

#### Methode 3: Konfigurationsdatei

```yaml
# config.yaml
llm:
  enabled: true
  models:
    - id: phi3-mini
      path: /home/user/models/gguf/Phi-3-mini-4k-instruct-q4.gguf
      context_size: 4096
      gpu_layers: 32
      preload: true
      
# Server mit Config starten
./themis_server --config config.yaml
```

### 2.3 Geladene Modelle anzeigen

```bash
# Liste aller geladenen Modelle
curl http://localhost:8080/api/llm/models

# Antwort:
# {
#   "models": [
#     {
#       "id": "phi3-mini",
#       "path": "/home/user/models/gguf/Phi-3-mini-4k-instruct-q4.gguf",
#       "loaded": true,
#       "memory_mb": 2048,
#       "gpu_layers": 32,
#       "context_size": 4096
#     }
#   ]
# }
```

> **📝 Siehe auch:** [LLM Loader Guide](../llm/LLM_LOADER_GUIDE.md)

---

## Schritt 3: Inferencing durchführen

### 3.1 Einfache Text-Generierung

```bash
# Simple completion
curl -X POST http://localhost:8080/api/llm/generate \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "phi3-mini",
    "prompt": "Erkläre mir die Vorteile von Multi-Model Datenbanken:",
    "max_tokens": 256,
    "temperature": 0.7
  }'

# Antwort:
# {
#   "model_id": "phi3-mini",
#   "prompt": "Erkläre mir die Vorteile von Multi-Model Datenbanken:",
#   "response": "Multi-Model Datenbanken bieten mehrere Vorteile:\n\n1. Flexibilität: Sie können verschiedene Datenmodelle (Relational, Dokument, Graph, Vector) in einer Datenbank vereinen.\n2. Einfachheit: Keine Notwendigkeit für mehrere separate Datenbanksysteme.\n3. Konsistenz: ACID-Transaktionen über alle Datenmodelle hinweg.\n4. Performance: Optimierte Abfragen für jeden Datentyp.\n5. Entwickler-Produktivität: Eine API für alle Datenmodelle.",
#   "tokens_generated": 142,
#   "latency_ms": 3847
# }
```

### 3.2 Chat-Completion (Konversation)

```bash
# Chat-API mit Kontext
curl -X POST http://localhost:8080/api/llm/chat \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "phi3-mini",
    "messages": [
      {
        "role": "system",
        "content": "Du bist ein hilfreicher Assistent für Datenbankfragen."
      },
      {
        "role": "user",
        "content": "Was ist ThemisDB?"
      }
    ],
    "max_tokens": 200,
    "temperature": 0.7
  }'

# Antwort:
# {
#   "model_id": "phi3-mini",
#   "response": "ThemisDB ist eine Multi-Model Datenbank mit nativer LLM-Integration...",
#   "finish_reason": "stop",
#   "tokens": 156,
#   "latency_ms": 2341
# }
```

### 3.3 Python-Client

```python
# ThemisDB Python Client installieren
# pip install themisdb-client

from themisdb import ThemisClient

# Client initialisieren
client = ThemisClient(host="localhost", port=8080)

# Text-Generierung
response = client.llm.generate(
    model_id="phi3-mini",
    prompt="Schreibe eine kurze Zusammenfassung über ACID-Transaktionen:",
    max_tokens=150,
    temperature=0.7
)

print(response.text)
# ACID-Transaktionen garantieren Atomicity, Consistency, Isolation und 
# Durability. Diese Eigenschaften stellen sicher, dass Datenbanktransaktionen 
# zuverlässig und konsistent ausgeführt werden...

print(f"Tokens: {response.tokens}, Latenz: {response.latency_ms}ms")
```

### 3.4 Streaming-Inferencing

Für lange Antworten ist Streaming nützlich:

```python
# Streaming-Generierung
for chunk in client.llm.generate_stream(
    model_id="phi3-mini",
    prompt="Erkläre ausführlich die Architektur von ThemisDB:",
    max_tokens=500
):
    print(chunk.text, end="", flush=True)
    
# ThemisDB verwendet eine mehrschichtige Architektur...
# [Text wird progressiv ausgegeben]
```

---

## Schritt 4: RAG-Workflow mit Beispieldaten

RAG (Retrieval-Augmented Generation) kombiniert Vektorsuche mit LLM-Generierung.

### 4.1 Beispieldaten vorbereiten

```python
from themisdb import ThemisClient
from sentence_transformers import SentenceTransformer

# Client und Embedding-Modell
client = ThemisClient(host="localhost", port=8080)
embedding_model = SentenceTransformer('all-MiniLM-L6-v2')

# Beispieldokumente
documents = [
    {
        "id": "doc1",
        "title": "ThemisDB Features",
        "content": "ThemisDB ist eine Multi-Model Datenbank mit ACID-Transaktionen, Graph-Support und nativer LLM-Integration."
    },
    {
        "id": "doc2",
        "title": "Vector Search",
        "content": "ThemisDB verwendet HNSW-Algorithmus für schnelle Vektorsuche und unterstützt GPU-Beschleunigung."
    },
    {
        "id": "doc3",
        "title": "LLM Integration",
        "content": "Die native LLM-Integration basiert auf llama.cpp und unterstützt Llama, Mistral und Phi-3 Modelle."
    }
]

# Embeddings generieren und Dokumente speichern
for doc in documents:
    # Embedding generieren
    embedding = embedding_model.encode(doc["content"]).tolist()
    
    # In ThemisDB speichern
    client.put_entity(
        entity_id=f"docs:{doc['id']}",
        data={
            "title": doc["title"],
            "content": doc["content"],
            "embedding": embedding
        }
    )
    
    # Vector-Index erstellen (einmalig)
    client.create_vector_index(
        table="docs",
        column="embedding",
        dimensions=384,  # all-MiniLM-L6-v2 Dimensionen
        metric="cosine"
    )

print("✓ Dokumente geladen und indexiert")
```

### 4.2 RAG-Query ausführen

```python
def rag_query(question: str, top_k: int = 3):
    """
    RAG-Pipeline: Retrieve -> Augment -> Generate
    """
    # 1. RETRIEVE: Relevante Dokumente suchen
    query_embedding = embedding_model.encode(question).tolist()
    
    results = client.vector_search(
        table="docs",
        vector=query_embedding,
        top_k=top_k,
        return_entities=True
    )
    
    # 2. AUGMENT: Kontext aufbauen
    context = "\n\n".join([
        f"Dokument {i+1}:\n{doc['content']}" 
        for i, doc in enumerate(results)
    ])
    
    # 3. GENERATE: LLM-Antwort mit Kontext
    prompt = f"""Beantworte die folgende Frage basierend auf den gegebenen Dokumenten.

Dokumente:
{context}

Frage: {question}

Antwort:"""

    response = client.llm.generate(
        model_id="phi3-mini",
        prompt=prompt,
        max_tokens=200,
        temperature=0.7
    )
    
    return {
        "question": question,
        "answer": response.text,
        "sources": [doc['title'] for doc in results],
        "latency_ms": response.latency_ms
    }

# RAG-Query durchführen
result = rag_query("Welche Features hat ThemisDB?")
print(f"Frage: {result['question']}")
print(f"Antwort: {result['answer']}")
print(f"Quellen: {', '.join(result['sources'])}")
print(f"Latenz: {result['latency_ms']}ms")
```

**Ausgabe:**
```
Frage: Welche Features hat ThemisDB?
Antwort: Basierend auf den Dokumenten bietet ThemisDB folgende Features:
1. Multi-Model Datenbank mit Unterstützung für verschiedene Datenmodelle
2. ACID-Transaktionen für Datenkonsistenz
3. Graph-Support für Beziehungen und Traversierungen
4. Native LLM-Integration basierend auf llama.cpp
5. Vector Search mit HNSW-Algorithmus
6. GPU-Beschleunigung für performante Vektorsuche
Quellen: ThemisDB Features, Vector Search
Latenz: 2547ms
```

### 4.3 Vollständiges RAG-Beispiel

Ein vollständiges RAG-Beispiel finden Sie hier:

📁 **[examples/07_vector_search_documents/](../../examples/07_vector_search_documents/)**

Dieses Beispiel zeigt:
- ✅ Dokument-Upload und Embedding-Generierung
- ✅ Vector-Index Management
- ✅ Hybride Suche (BM25 + Vector)
- ✅ RAG-Pipeline mit verschiedenen LLMs
- ✅ Tkinter-GUI für interaktive Queries

```bash
# Beispiel starten
cd examples/07_vector_search_documents
pip install -r requirements.txt
python main.py
```

> **📝 Siehe auch:** 
> - [Vector Search Features](../features/features_vector_ops.md)
> - [Semantic Cache](../features/features_semantic_cache.md)

---

## 🚀 Erweiterte Themen

### GPU-Beschleunigung optimieren

```bash
# Alle Layer auf GPU laden (-1)
curl -X POST http://localhost:8080/api/llm/load_model \
  -H "Content-Type: application/json" \
  -d '{
    "model_path": "/home/user/models/gguf/mistral-7b-q4.gguf",
    "model_id": "mistral-7b",
    "gpu_layers": -1
  }'

# GPU-Speicher überwachen
nvidia-smi -l 1
```

**Empfohlene GPU-Layer-Werte:**
- Kleine Modelle (3-7B): `-1` (alle Layer)
- Mittlere Modelle (13B): `32-40` Layer
- Große Modelle (70B): `20-30` Layer (abhängig von VRAM)

### Multi-LoRA Management

LoRA (Low-Rank Adaptation) ermöglicht Fine-Tuning ohne vollständiges Retraining:

```bash
# LoRA-Adapter laden
curl -X POST http://localhost:8080/api/llm/load_lora \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "phi3-mini",
    "lora_path": "/home/user/loras/phi3-german-lora.bin",
    "lora_id": "german-adapter",
    "scale": 1.0
  }'

# Mit LoRA inferieren
curl -X POST http://localhost:8080/api/llm/generate \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "phi3-mini",
    "lora_id": "german-adapter",
    "prompt": "Übersetze ins Deutsche: Hello World",
    "max_tokens": 50
  }'
```

> **📝 Siehe auch:** [LoRA Training Framework](../llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md)

### Prompt-Templates

```python
# Prompt-Template registrieren
client.llm.register_template(
    template_id="rag-query",
    template="""Basierend auf folgenden Dokumenten:

{context}

Beantworte die Frage: {question}

Antwort:""",
    variables=["context", "question"]
)

# Template verwenden
response = client.llm.generate_from_template(
    model_id="phi3-mini",
    template_id="rag-query",
    variables={
        "context": context_text,
        "question": "Was ist ThemisDB?"
    }
)
```

### Batch-Inferencing

Für viele Anfragen gleichzeitig:

```python
# Batch-Request
batch_prompts = [
    "Erkläre ACID",
    "Was ist ein Graph-Algorithmus?",
    "Wie funktioniert Vector Search?"
]

responses = client.llm.generate_batch(
    model_id="phi3-mini",
    prompts=batch_prompts,
    max_tokens=100
)

for prompt, response in zip(batch_prompts, responses):
    print(f"Q: {prompt}")
    print(f"A: {response.text}\n")
```

### Monitoring und Metriken

```bash
# LLM-Metriken abrufen
curl http://localhost:8080/api/llm/metrics

# Antwort:
# {
#   "total_requests": 1523,
#   "total_tokens_generated": 234567,
#   "avg_latency_ms": 2341,
#   "cache_hit_rate": 0.43,
#   "models": {
#     "phi3-mini": {
#       "requests": 1523,
#       "tokens": 234567,
#       "avg_latency_ms": 2341
#     }
#   }
# }
```

> **📝 Siehe auch:** [Monitoring & Testing Strategy](../llm/MONITORING_TESTING_STRATEGY.md)

---

## 🔧 Troubleshooting

### Problem: Modell lädt nicht

**Symptom:** `Failed to load model` Fehler

**Lösungen:**
1. Überprüfen Sie den Dateipfad:
   ```bash
   ls -lh /home/user/models/gguf/
   ```
2. Prüfen Sie Dateiberechtigungen:
   ```bash
   chmod 644 /home/user/models/gguf/*.gguf
   ```
3. Verifizieren Sie das GGUF-Format:
   ```bash
   file /home/user/models/gguf/phi3-mini.gguf
   # Sollte: "GGUF model file" anzeigen
   ```

### Problem: Out of Memory

**Symptom:** `CUDA out of memory` oder `malloc failed`

**Lösungen:**
1. Reduzieren Sie GPU-Layer:
   ```json
   {"gpu_layers": 20}  // Statt -1
   ```
2. Verwenden Sie stärker quantisiertes Modell:
   - Q4_K_M → Q2_K (kleiner, aber niedrigere Qualität)
3. Reduzieren Sie Context-Size:
   ```json
   {"context_size": 2048}  // Statt 4096
   ```

### Problem: Langsame Inferenz

**Symptom:** Generierung dauert sehr lange

**Lösungen:**
1. Aktivieren Sie GPU:
   ```json
   {"gpu_layers": -1}
   ```
2. Überprüfen Sie GPU-Nutzung:
   ```bash
   nvidia-smi
   # GPU-Utilization sollte >80% sein
   ```
3. Verwenden Sie kleineres Modell:
   - Phi-3 Mini (3B) statt Llama-70B
4. Aktivieren Sie KV-Cache:
   ```json
   {"cache_prompt": true}
   ```

### Problem: Schlechte Antwortqualität

**Symptom:** LLM generiert unsinnige oder irrelevante Antworten

**Lösungen:**
1. Adjustieren Sie Temperature:
   ```json
   {"temperature": 0.7}  // Höher = kreativer, niedriger = deterministischer
   ```
2. Verwenden Sie besseres Prompt-Template:
   ```
   System: Du bist ein hilfreicher Assistent...
   User: [Klare, spezifische Frage]
   ```
3. Laden Sie weniger quantisiertes Modell:
   - Q2_K → Q4_K_M → Q8_0 (größer, aber höhere Qualität)

### Problem: Build-Fehler

**Symptom:** Compilation schlägt fehl

**Lösungen:**
1. Verifizieren Sie llama.cpp ist geklont:
   ```bash
   ls -la llama.cpp/
   ```
2. Aktualisieren Sie vcpkg:
   ```bash
   cd vcpkg
   git pull
   ./bootstrap-vcpkg.sh
   ```
3. Cleanen und neu bauen:
   ```bash
   rm -rf build/
   mkdir build && cd build
   cmake .. -DTHEMIS_ENABLE_LLM=ON
   cmake --build . -j
   ```

> **📝 Ausführliches Troubleshooting:** [Known Issues](./guides_known_issues.md)

---

## 📚 Weiterführende Dokumentation

### Grundlagen
- 📖 [Quick Start Guide](./QUICK_START.md) - Erste Schritte mit ThemisDB
- 🔨 [Build Strategy Guide](./guides_build_strategy.md) - Detaillierte Build-Anweisungen
- 🐳 [Docker Deployment](../deployment/DOCKER_DEPLOYMENT.md) - Container-Setup

### LLM-spezifisch
- 🧠 [LLM Übersicht](../llm/README.md) - Alle LLM-Features
- 🔌 [Plugin Development](../llm/LLM_PLUGIN_DEVELOPMENT_GUIDE.md) - Eigene LLM-Plugins
- 📦 [llama.cpp Integration](../llm/LLAMA_CPP_INTEGRATION.md) - Technische Details
- 🎯 [Native LLM Concept](../llm/NATIVE_LLM_INTEGRATION_CONCEPT.md) - Architektur-Overview

### RAG & Vector Search
- 🔍 [Vector Operations](../features/features_vector_ops.md) - Vector Search Features
- 🧩 [Semantic Cache](../features/features_semantic_cache.md) - Embedding-Cache
- 📊 [Graph Features](../features/features_graph.md) - Graph-basierte RAG

### Beispiele
- 🎓 [Beispiel 07: Vector Search & RAG](../../examples/07_vector_search_documents/) - Vollständiges RAG-Beispiel
- 🚁 [Beispiel 10: Drohnenbild-Analyse](../../examples/10_drone_image_analysis/) - Komplexes LLM + Vision Beispiel
- 📚 [Alle Beispiele](../../examples/README.md) - Übersicht

### Benchmarks & Testing
- ⚡ [LLM/NLP Integration Quickstart](../../benchmarks/LLM_NLP_INTEGRATION_QUICKSTART.md) - Performance-Tests
- 📊 [LLM Integration Framework](../../benchmarks/LLM_NLP_INTEGRATION_FRAMEWORK.md) - Test-Framework

### Erweitert
- 🎮 [Ollama & vLLM Features](../llm/OLLAMA_VLLM_FEATURES.md) - Alternative Backends
- 🏢 [Enterprise LLM Feedback](../llm/LLM_FEEDBACK_ENTERPRISE.md) - Production Best Practices
- 🔐 [Thread Safety](../llm/THREAD_SAFETY_AND_SHARING.md) - Concurrency

---

## 🤝 Community & Support

### Hilfe bekommen

**Bei Fragen oder Problemen:**
- 🐛 [GitHub Issues](https://github.com/makr-code/ThemisDB/issues) - Bug Reports
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions) - Community Q&A
- 📖 [Dokumentation](https://makr-code.github.io/ThemisDB/) - Vollständige Docs

### Beitragen

Möchten Sie beitragen?
- 🤝 [Contributing Guide](../../../CONTRIBUTING.md) - Wie Sie beitragen können
- 📋 [LLM Roadmap](../llm/WEEK_4_6_IMPLEMENTATION_PLAN.md) - Geplante Features

---

## 📝 Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 1.0 | 2025-12-22 | Initiale Version des Guides |

---

**Viel Erfolg mit ThemisDB und LLM-Integration! 🚀**

Bei Fragen: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
