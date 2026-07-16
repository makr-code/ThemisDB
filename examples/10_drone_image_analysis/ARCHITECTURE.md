> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Architecture - Drohnenbild-Analyse System

## 📋 Übersicht

Dieses Dokument beschreibt die System-Architektur des Drohnenbild-Analyse-Systems, ein hochmodernes KI-gestütztes System für Echtzeit-Bildverarbeitung und -Analyse.

## 🏗️ Gesamtarchitektur

### High-Level Komponenten

```
┌────────────────────────────────────────────────────────────────┐
│                    Presentation Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Map View     │  │ Timeline     │  │ Image Gallery        │ │
│  │ (Folium)     │  │ (Matplotlib) │  │ (PIL + Tkinter)      │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Statistics   │  │ Analysis     │  │ Export               │ │
│  │ Dashboard    │  │ Results      │  │ Reports              │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
└───────────────────────────┬────────────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────────────┐
│                    Application Layer                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Image        │  │ LLM          │  │ Object Detection     │ │
│  │ Preprocessing│  │ Integration  │  │ (YOLO/CLIP)          │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Geo-Spatial  │  │ Event        │  │ Similarity           │ │
│  │ Operations   │  │ Processing   │  │ Search (Vector)      │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
└───────────────────────────┬────────────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────────────┐
│                    Data Access Layer                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ ThemisDB     │  │ File System  │  │ Cache                │ │
│  │ Client       │  │ (Images)     │  │ (Redis/Memory)       │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────────────┐
│                    Storage Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Images       │  │ Metadata     │  │ Vector Embeddings    │ │
│  │ (Filesystem) │  │ (ThemisDB)   │  │ (ThemisDB)           │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Time-Series  │  │ Geo-Spatial  │  │ Graph Relations      │ │
│  │ (ThemisDB)   │  │ (ThemisDB)   │  │ (ThemisDB)           │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
```

## 🔄 Datenfluss

### 1. Bild-Upload Flow

```
┌─────────────┐
│ User Upload │
│ Image       │
└──────┬──────┘
       │
       ▼
┌─────────────────────┐
│ Validate & Store    │
│ - Format Check      │
│ - Size Limit        │
│ - Duplicate Check   │
└──────┬──────────────┘
       │
       ▼
┌─────────────────────┐
│ Extract Metadata    │
│ - EXIF Data         │
│ - GPS Coordinates   │
│ - Camera Info       │
└──────┬──────────────┘
       │
       ▼
┌─────────────────────┐
│ Image Processing    │
│ - Resize/Thumbnail  │
│ - Normalize         │
│ - Enhancement       │
└──────┬──────────────┘
       │
       ├──────────────────┐
       │                  │
       ▼                  ▼
┌─────────────┐    ┌──────────────┐
│ Object      │    │ LLM          │
│ Detection   │    │ Description  │
│ (YOLO/CLIP) │    │ (llama.cpp)  │
└──────┬──────┘    └──────┬───────┘
       │                  │
       └────────┬─────────┘
                ▼
       ┌────────────────┐
       │ Generate       │
       │ Embeddings     │
       │ (CLIP Vector)  │
       └────────┬───────┘
                │
                ▼
       ┌────────────────┐
       │ Store to       │
       │ ThemisDB       │
       │ - Metadata     │
       │ - Vectors      │
       │ - Geo-Spatial  │
       └────────────────┘
```

### 2. Suche/Abfrage Flow

```
┌──────────────┐
│ User Query   │
│ - Text       │
│ - Image      │
│ - Location   │
└──────┬───────┘
       │
       ├────────────────┬─────────────────┬─────────────────┐
       │                │                 │                 │
       ▼                ▼                 ▼                 ▼
┌──────────┐    ┌───────────┐    ┌──────────┐    ┌──────────┐
│ Text     │    │ Similarity│    │ Geo      │    │ Time     │
│ Search   │    │ (Vector)  │    │ Search   │    │ Range    │
└─────┬────┘    └─────┬─────┘    └────┬─────┘    └────┬─────┘
      │               │               │               │
      └───────────────┴───────────────┴───────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │ Merge & Rank    │
                    │ Results         │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ Enrich with     │
                    │ Additional Data │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ Return to User  │
                    └─────────────────┘
```

## 🧩 Komponenten-Details

### 1. Image Processing Pipeline

```python
class ImageProcessingPipeline:
    """Zentrale Bildverarbeitungs-Pipeline."""
    
    def __init__(self):
        self.stages = [
            ImageValidator(),
            MetadataExtractor(),
            ImageEnhancer(),
            ObjectDetector(),
            LLMDescriptor(),
            EmbeddingGenerator()
        ]
    
    async def process(self, image_path: str) -> dict:
        """Verarbeitet Bild durch alle Stages."""
        context = {"image_path": image_path}
        
        for stage in self.stages:
            try:
                context = await stage.process(context)
            except Exception as e:
                self._handle_stage_error(stage, e, context)
        
        return context

# Stages
class ImageValidator:
    """Stage 1: Validierung."""
    async def process(self, context: dict) -> dict:
        # Prüfe Format, Größe, etc.
        return context

class MetadataExtractor:
    """Stage 2: Metadaten-Extraktion."""
    async def process(self, context: dict) -> dict:
        # Extrahiere EXIF, GPS, etc.
        return context

class ImageEnhancer:
    """Stage 3: Bildverbesserung."""
    async def process(self, context: dict) -> dict:
        # Resize, Normalisierung, etc.
        return context

class ObjectDetector:
    """Stage 4: Objekt-Erkennung."""
    async def process(self, context: dict) -> dict:
        # YOLO/CLIP Detection
        return context

class LLMDescriptor:
    """Stage 5: LLM-Beschreibung."""
    async def process(self, context: dict) -> dict:
        # Generiere Bildbeschreibung
        return context

class EmbeddingGenerator:
    """Stage 6: Embedding-Generierung."""
    async def process(self, context: dict) -> dict:
        # CLIP Embeddings
        return context
```

### 2. LLM Integration Layer

```python
class LLMIntegrationLayer:
    """Abstraktions-Layer für verschiedene LLM-Backends."""
    
    def __init__(self, backend: str = "llama_cpp"):
        self.backend = self._initialize_backend(backend)
    
    def _initialize_backend(self, backend_name: str):
        """Initialisiert LLM-Backend."""
        if backend_name == "llama_cpp":
            return LlamaCppBackend()
        elif backend_name == "onnx":
            return ONNXBackend()
        elif backend_name == "transformers":
            return TransformersBackend()
        else:
            raise ValueError(f"Unknown backend: {backend_name}")
    
    async def generate_description(
        self,
        image_path: str,
        prompt: str = None
    ) -> str:
        """Generiert Bildbeschreibung."""
        return await self.backend.generate(image_path, prompt)
    
    async def answer_question(
        self,
        image_path: str,
        question: str
    ) -> str:
        """Beantwortet Frage zu Bild."""
        return await self.backend.answer(image_path, question)

class LlamaCppBackend:
    """Backend für llama.cpp."""
    
    def __init__(self):
        # Initialize llama.cpp
        pass
    
    async def generate(self, image_path: str, prompt: str) -> str:
        # Generate description using llama.cpp
        pass
```

### 3. Geo-Spatial Processing

```python
class GeoSpatialProcessor:
    """Verarbeitung geo-räumlicher Daten."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def find_nearby_images(
        self,
        lat: float,
        lon: float,
        radius_km: float
    ) -> list:
        """Findet Bilder in der Nähe."""
        query = {
            "location": {
                "$near": {
                    "$geometry": {
                        "type": "Point",
                        "coordinates": [lon, lat]
                    },
                    "$maxDistance": radius_km * 1000  # meters
                }
            }
        }
        
        return await self.client.query("images", query)
    
    async def get_coverage_area(
        self,
        image_id: str
    ) -> dict:
        """Berechnet Abdeckungsbereich eines Drohnenbilds."""
        image = await self.client.get("images", image_id)
        
        # Berechne Coverage basierend auf:
        # - Altitude
        # - Camera FOV
        # - Tilt angle
        
        altitude = image["location"]["altitude"]
        fov_horizontal = 84  # degrees (DJI Mavic 3)
        tilt = image["location"].get("tilt", -90)  # -90 = straight down
        
        coverage = self._calculate_coverage(altitude, fov_horizontal, tilt)
        
        return {
            "center": [image["location"]["lon"], image["location"]["lat"]],
            "radius_meters": coverage["radius"],
            "area_sqm": coverage["area"]
        }
    
    def _calculate_coverage(
        self,
        altitude: float,
        fov: float,
        tilt: float
    ) -> dict:
        """Berechnet Abdeckungsbereich."""
        import math
        
        # Simplified calculation
        fov_rad = math.radians(fov)
        ground_width = 2 * altitude * math.tan(fov_rad / 2)
        area = ground_width * ground_width  # Assuming square
        radius = ground_width / 2
        
        return {"radius": radius, "area": area}
```

### 4. Vector Search Engine

```python
class VectorSearchEngine:
    """Vector-basierte Ähnlichkeitssuche."""
    
    def __init__(self, themis_client):
        self.client = themis_client
        self.embedding_model = self._load_embedding_model()
    
    def _load_embedding_model(self):
        """Lädt CLIP Modell."""
        import clip
        import torch
        
        device = "cuda" if torch.cuda.is_available() else "cpu"
        model, preprocess = clip.load("ViT-B/32", device=device)
        
        return {"model": model, "preprocess": preprocess, "device": device}
    
    async def search_by_text(
        self,
        query_text: str,
        top_k: int = 10
    ) -> list:
        """Sucht Bilder anhand von Text-Beschreibung."""
        # Generiere Text-Embedding
        text_embedding = self._generate_text_embedding(query_text)
        
        # Suche ähnliche Bilder
        return await self.client.vector_search(
            "images",
            "embedding",
            text_embedding,
            top_k=top_k
        )
    
    async def search_by_image(
        self,
        image_path: str,
        top_k: int = 10
    ) -> list:
        """Sucht ähnliche Bilder."""
        # Generiere Image-Embedding
        image_embedding = self._generate_image_embedding(image_path)
        
        # Suche ähnliche Bilder
        return await self.client.vector_search(
            "images",
            "embedding",
            image_embedding,
            top_k=top_k
        )
    
    def _generate_text_embedding(self, text: str) -> list:
        """Generiert Embedding für Text."""
        import clip
        import torch
        
        model = self.embedding_model["model"]
        device = self.embedding_model["device"]
        
        text_token = clip.tokenize([text]).to(device)
        
        with torch.no_grad():
            text_features = model.encode_text(text_token)
            text_features /= text_features.norm(dim=-1, keepdim=True)
        
        return text_features.cpu().numpy().tolist()[0]
    
    def _generate_image_embedding(self, image_path: str) -> list:
        """Generiert Embedding für Bild."""
        import clip
        import torch
        from PIL import Image
        
        model = self.embedding_model["model"]
        preprocess = self.embedding_model["preprocess"]
        device = self.embedding_model["device"]
        
        image = preprocess(Image.open(image_path)).unsqueeze(0).to(device)
        
        with torch.no_grad():
            image_features = model.encode_image(image)
            image_features /= image_features.norm(dim=-1, keepdim=True)
        
        return image_features.cpu().numpy().tolist()[0]
```

### 5. Event Processing System

```python
class EventProcessingSystem:
    """Erkennt und verarbeitet Events aus Bildanalysen."""
    
    def __init__(self, themis_client):
        self.client = themis_client
        self.event_rules = []
    
    def add_rule(self, rule):
        """Fügt Event-Rule hinzu."""
        self.event_rules.append(rule)
    
    async def process_image_analysis(self, analysis: dict):
        """Verarbeitet Bildanalyse und triggert Events."""
        events = []
        
        for rule in self.event_rules:
            if rule.matches(analysis):
                event = await rule.create_event(analysis)
                events.append(event)
                await self._store_event(event)
        
        return events
    
    async def _store_event(self, event: dict):
        """Speichert Event in ThemisDB."""
        await self.client.create("events", event)

# Beispiel-Rule
class ConstructionProgressRule:
    """Rule für Baufortschritt-Erkennung."""
    
    def matches(self, analysis: dict) -> bool:
        """Prüft ob Baufortschritt erkannt wurde."""
        detected_objects = analysis.get("detected_objects", [])
        
        construction_objects = [
            "crane", "excavator", "building", "construction_site"
        ]
        
        return any(
            obj["classification"] in construction_objects
            for obj in detected_objects
        )
    
    async def create_event(self, analysis: dict) -> dict:
        """Erstellt Event."""
        return {
            "type": "construction_progress",
            "image_id": analysis["image_id"],
            "location": analysis["location"],
            "detected_at": datetime.now().isoformat(),
            "confidence": self._calculate_confidence(analysis),
            "description": "Construction activity detected"
        }
    
    def _calculate_confidence(self, analysis: dict) -> float:
        """Berechnet Konfidenz."""
        objects = analysis.get("detected_objects", [])
        if not objects:
            return 0.0
        
        return max(obj["confidence"] for obj in objects)
```

## 🔐 Sicherheits-Architektur

### Authentication Layer

```python
class AuthenticationLayer:
    """Authentifizierungs-Layer."""
    
    def __init__(self):
        self.session_manager = SessionManager()
    
    async def authenticate(self, username: str, password: str) -> dict:
        """Authentifiziert User."""
        # Hash password and verify
        pass
    
    async def check_permission(
        self,
        user_id: str,
        resource: str,
        action: str
    ) -> bool:
        """Prüft Berechtigung."""
        # Check RBAC permissions
        pass
```

### Data Encryption

```python
class EncryptionLayer:
    """Verschlüsselung für sensitive Daten."""
    
    def __init__(self, key: bytes):
        from cryptography.fernet import Fernet
        self.cipher = Fernet(key)
    
    def encrypt_metadata(self, metadata: dict) -> dict:
        """Verschlüsselt sensitive Metadaten."""
        sensitive_fields = ["gps_exact", "camera_serial", "pilot_id"]
        
        encrypted = metadata.copy()
        for field in sensitive_fields:
            if field in encrypted:
                value = str(encrypted[field]).encode()
                encrypted[field] = self.cipher.encrypt(value).decode()
        
        return encrypted
```

## 📊 Monitoring & Observability

### Metriken

- **Performance**: Verarbeitungszeit pro Bild, Throughput
- **Accuracy**: Erkennungsgenauigkeit, False Positives
- **System Health**: CPU/GPU Auslastung, Memory Usage
- **Business**: Anzahl verarbeiteter Bilder, Events

### Logging

```python
import logging
import structlog

# Structured Logging
logger = structlog.get_logger()

logger.info(
    "image_processed",
    image_id="img_123",
    processing_time_ms=1234,
    detected_objects=5,
    llm_confidence=0.89
)
```

## 🎯 Deployment-Szenarien

### 1. Single Machine (Development)
- Alles auf einem System
- Lokales ThemisDB
- CPU-only Inference

### 2. Small Scale (< 1000 Bilder/Tag)
- Separate ThemisDB Instance
- Single GPU für Inference
- Load Balancer optional

### 3. Production Scale (> 1000 Bilder/Tag)
- ThemisDB Cluster
- Multiple GPU Nodes
- Load Balancing + Auto-Scaling
- CDN für Bild-Serving

### 4. Edge Deployment
- Edge Computing Nodes
- Lokale Preprocessing
- Cloud-Sync für finale Analyse

## 🎓 Best Practices

1. **Modularität**
   - Loose Coupling zwischen Komponenten
   - Klare Interfaces
   - Dependency Injection

2. **Fehlerbehandlung**
   - Graceful Degradation
   - Retry-Logic mit Backoff
   - Circuit Breakers

3. **Performance**
   - Batch-Processing wo möglich
   - Caching häufiger Queries
   - Lazy Loading von Bildern

4. **Skalierbarkeit**
   - Stateless Services
   - Horizontal Scaling Ready
   - Message Queues für Async Work

## 📚 Weitere Dokumentation

- [LLM_INTEGRATION.md](LLM_INTEGRATION.md) - LLM Setup
- [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) - CV Pipeline
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
- [DEPLOYMENT.md](DEPLOYMENT.md) - Deployment Guide
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Fehlerbehandlung
