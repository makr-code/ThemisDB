> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Image Processing - Drohnenbild-Analyse

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 📋 Übersicht

Computer Vision Pipeline für die Verarbeitung und Analyse von Drohnenbildern mit OpenCV, YOLO und CLIP.

## 🖼️ Bildvorverarbeitung

### Image Loading & Validation

\`\`\`python
import cv2
import numpy as np
from PIL import Image
import piexif

class ImagePreprocessor:
    """Bildvorverarbeitung."""
    
    SUPPORTED_FORMATS = ['.jpg', '.jpeg', '.png', '.tiff']
    MAX_SIZE_MB = 50
    
    def load_and_validate(self, image_path: str) -> dict:
        """Lädt und validiert Bild."""
        # Check format
        ext = os.path.splitext(image_path)[1].lower()
        if ext not in self.SUPPORTED_FORMATS:
            raise ValueError(f"Unsupported format: {ext}")
        
        # Check size
        size_mb = os.path.getsize(image_path) / (1024 * 1024)
        if size_mb > self.MAX_SIZE_MB:
            raise ValueError(f"File too large: {size_mb:.1f}MB")
        
        # Load
        image = cv2.imread(image_path)
        if image is None:
            raise ValueError("Failed to load image")
        
        return {
            'image': image,
            'path': image_path,
            'size_mb': size_mb,
            'dimensions': (image.shape[1], image.shape[0])
        }
\`\`\`

### EXIF Extraction

\`\`\`python
def extract_exif(image_path: str) -> dict:
    """Extrahiert EXIF-Daten."""
    try:
        img = Image.open(image_path)
        exif_dict = piexif.load(img.info.get('exif', b''))
        
        # GPS
        gps = exif_dict.get('GPS', {})
        lat = _convert_gps(gps.get(piexif.GPSIFD.GPSLatitude))
        lon = _convert_gps(gps.get(piexif.GPSIFD.GPSLongitude))
        alt = gps.get(piexif.GPSIFD.GPSAltitude, (0, 1))[0]
        
        # Camera
        exif = exif_dict.get('Exif', {})
        iso = exif.get(piexif.ExifIFD.ISOSpeedRatings, 0)
        
        return {
            'gps': {'lat': lat, 'lon': lon, 'altitude': alt},
            'camera': {
                'make': exif_dict.get('0th', {}).get(piexif.ImageIFD.Make, b'').decode(),
                'model': exif_dict.get('0th', {}).get(piexif.ImageIFD.Model, b'').decode(),
                'iso': iso
            }
        }
    except:
        return {}
\`\`\`

## 🎨 Image Enhancement

### Normalization

\`\`\`python
def normalize_image(image: np.ndarray) -> np.ndarray:
    """Normalisiert Bild."""
    # Convert to float
    image_float = image.astype(np.float32) / 255.0
    
    # Normalize per channel
    mean = np.array([0.485, 0.456, 0.406])
    std = np.array([0.229, 0.224, 0.225])
    
    normalized = (image_float - mean) / std
    
    return normalized
\`\`\`

### Contrast Enhancement

\`\`\`python
def enhance_contrast(image: np.ndarray) -> np.ndarray:
    """Verbessert Kontrast."""
    # CLAHE (Contrast Limited Adaptive Histogram Equalization)
    lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
    l, a, b = cv2.split(lab)
    
    clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8))
    l_enhanced = clahe.apply(l)
    
    enhanced = cv2.merge([l_enhanced, a, b])
    enhanced = cv2.cvtColor(enhanced, cv2.COLOR_LAB2BGR)
    
    return enhanced
\`\`\`

## 🎯 Object Detection

### YOLO Integration

\`\`\`python
import torch

class YOLODetector:
    """YOLO Object Detection."""
    
    def __init__(self, model_path: str = "yolov8n.pt"):
        from ultralytics import YOLO
        self.model = YOLO(model_path)
        self.confidence_threshold = 0.5
    
    def detect(self, image: np.ndarray) -> list:
        """Erkennt Objekte."""
        results = self.model(image, conf=self.confidence_threshold)
        
        detections = []
        for result in results:
            boxes = result.boxes
            for box in boxes:
                detections.append({
                    'class': result.names[int(box.cls)],
                    'confidence': float(box.conf),
                    'bbox': box.xyxy[0].tolist(),
                    'center': [(box.xyxy[0][0] + box.xyxy[0][2]) / 2,
                              (box.xyxy[0][1] + box.xyxy[0][3]) / 2]
                })
        
        return detections
\`\`\`

## 📊 Image Analysis

### Scene Classification

\`\`\`python
class SceneClassifier:
    """Klassifiziert Szenen."""
    
    def __init__(self):
        self.categories = {
            'urban': ['building', 'car', 'person', 'street'],
            'construction': ['crane', 'excavator', 'truck'],
            'agriculture': ['crop', 'tractor', 'field'],
            'water': ['boat', 'water', 'wave']
        }
    
    def classify(self, detections: list) -> str:
        """Klassifiziert Szene."""
        scores = {cat: 0 for cat in self.categories}
        
        for detection in detections:
            for category, keywords in self.categories.items():
                if any(kw in detection['class'].lower() for kw in keywords):
                    scores[category] += detection['confidence']
        
        return max(scores, key=scores.get)
\`\`\`

## 🎓 Best Practices

1. **Preprocessing**
   - Normalisiere Bilder vor Inference
   - Resize für konsistente Performance
   - Cache vorverarbeitete Bilder

2. **Object Detection**
   - Tune Confidence Threshold je nach Use Case
   - Verwende NMS (Non-Maximum Suppression)
   - Tracke Performance-Metriken

3. **Performance**
   - Batch-Processing für mehrere Bilder
   - GPU-Beschleunigung aktivieren
   - Model Quantization verwenden

## 📚 Weitere Dokumentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design
- [LLM_INTEGRATION.md](LLM_INTEGRATION.md) - LLM Setup
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
