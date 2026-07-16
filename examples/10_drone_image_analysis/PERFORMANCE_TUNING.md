> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt.
> Für reproduzierbare Ergebnisse: CMake-Presets und aktuellen Teststand verwenden.

# Performance Tuning - Drohnenbild-Analyse

## 📋 Übersicht

Optimierungsstrategien für maximale Performance bei der Bildverarbeitung und -analyse.

## 🚀 GPU Optimization

### CUDA Setup

\`\`\`python
import torch

def setup_cuda():
    """Konfiguriert CUDA."""
    if torch.cuda.is_available():
        torch.backends.cudnn.benchmark = True
        torch.backends.cudnn.deterministic = False
        
        # Set memory allocator
        torch.cuda.empty_cache()
        
        print(f"GPU: {torch.cuda.get_device_name(0)}")
        print(f"Memory: {torch.cuda.get_device_properties(0).total_memory / 1e9:.1f} GB")
\`\`\`

### Model Quantization

\`\`\`python
def quantize_model(model, method: str = 'dynamic'):
    """Quantisiert Model."""
    if method == 'dynamic':
        # Dynamic quantization (inference only)
        return torch.quantization.quantize_dynamic(
            model,
            {torch.nn.Linear},
            dtype=torch.qint8
        )
    elif method == 'static':
        # Static quantization (needs calibration)
        model.qconfig = torch.quantization.get_default_qconfig('fbgemm')
        return torch.quantization.prepare(model)
\`\`\`

## 📦 Batch Processing

### Image Batching

\`\`\`python
class ImageBatcher:
    """Batch-Verarbeitung von Bildern."""
    
    def __init__(self, batch_size: int = 8):
        self.batch_size = batch_size
    
    def create_batches(self, images: list) -> list:
        """Erstellt Batches."""
        return [
            images[i:i + self.batch_size]
            for i in range(0, len(images), self.batch_size)
        ]
    
    async def process_batch(
        self,
        batch: list,
        processor
    ) -> list:
        """Verarbeitet Batch parallel."""
        tasks = [processor(img) for img in batch]
        return await asyncio.gather(*tasks)
\`\`\`

## 💾 Caching

### Image Cache

\`\`\`python
from functools import lru_cache
import hashlib

class ImageCache:
    """Cache für verarbeitete Bilder."""
    
    def __init__(self, max_size_mb: int = 1000):
        self.cache = {}
        self.max_size = max_size_mb * 1024 * 1024
        self.current_size = 0
    
    def get_hash(self, image_path: str) -> str:
        """Berechnet Hash."""
        with open(image_path, 'rb') as f:
            return hashlib.md5(f.read()).hexdigest()
    
    def get(self, image_path: str):
        """Holt aus Cache."""
        key = self.get_hash(image_path)
        return self.cache.get(key)
    
    def put(self, image_path: str, data: dict):
        """Speichert in Cache."""
        key = self.get_hash(image_path)
        self.cache[key] = data
\`\`\`

## 📊 Profiling

### Performance Profiler

\`\`\`python
import time
from contextlib import contextmanager

@contextmanager
def timer(name: str):
    """Context manager für Timing."""
    start = time.time()
    yield
    end = time.time()
    print(f"{name}: {(end - start) * 1000:.2f}ms")

# Usage
with timer("Image Processing"):
    processed = process_image(image)
\`\`\`

## 🎯 Benchmarks

### Expected Performance

| Operation | CPU (ms) | GPU (ms) |
|-----------|----------|----------|
| Image Load | 50 | - |
| Preprocessing | 20 | 5 |
| YOLO Detection | 200 | 15 |
| LLM Description | 2000 | 500 |
| **Total** | **2270** | **520** |

## 🎓 Best Practices

1. **GPU Usage**
   - Batch operations when possible
   - Keep data on GPU between operations
   - Use mixed precision (FP16)

2. **Memory**
   - Clear cache regularly
   - Use generators for large datasets
   - Monitor memory usage

3. **I/O**
   - Preload images in background
   - Use SSD for image storage
   - Compress thumbnails

## 📚 Weitere Dokumentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design
- [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) - CV Pipeline
- [DEPLOYMENT.md](DEPLOYMENT.md) - Deployment Guide
