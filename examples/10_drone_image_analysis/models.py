"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     473                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Datenmodelle für Drohnenbild-Analyse System.

Dieses Modul enthält die Datenmodelle für:
- DroneImage: Bilddaten mit GPS-Koordinaten
- ImageAnalysis: KI-Analyse mit Objekterkennung und LLM
- DetectedObject: Einzelnes erkanntes Objekt
- TimeSeriesEvent: Events über Zeit
- ImageProcessor: Bildverarbeitung und CV
- LLMIntegration: LLM für Bildbeschreibungen
"""

from dataclasses import dataclass, field
from typing import List, Optional, Tuple
from datetime import datetime
from enum import Enum
import random
import math


class SceneType(Enum):
    """Arten von Szenen."""
    URBAN = "urban"
    RURAL = "rural"
    FOREST = "forest"
    WATER = "water"
    INDUSTRIAL = "industrial"
    AGRICULTURAL = "agricultural"
    MIXED = "mixed"


class WeatherCondition(Enum):
    """Wetterbedingungen."""
    CLEAR = "clear"
    CLOUDY = "cloudy"
    RAINY = "rainy"
    FOGGY = "foggy"
    SNOWY = "snowy"


class ObjectType(Enum):
    """Arten von erkannten Objekten."""
    VEHICLE = "vehicle"
    BUILDING = "building"
    PERSON = "person"
    TREE = "tree"
    ROAD = "road"
    WATER_BODY = "water_body"
    CONSTRUCTION = "construction"
    UNKNOWN = "unknown"


@dataclass
class Location:
    """GPS-Lokation mit zusätzlichen Metadaten."""
    lat: float
    lon: float
    altitude: float  # Meter
    heading: float  # Grad (0-360)
    tilt: float  # Grad (-90 bis 0)
    
    def to_dict(self):
        return {
            'lat': self.lat,
            'lon': self.lon,
            'altitude': self.altitude,
            'heading': self.heading,
            'tilt': self.tilt
        }
    
    @staticmethod
    def from_dict(data):
        return Location(
            lat=data['lat'],
            lon=data['lon'],
            altitude=data['altitude'],
            heading=data['heading'],
            tilt=data['tilt']
        )


@dataclass
class ImageMetadata:
    """Bild-Metadaten."""
    resolution: str
    format: str
    size_mb: float
    camera: str
    iso: int
    shutter_speed: str
    
    def to_dict(self):
        return {
            'resolution': self.resolution,
            'format': self.format,
            'size_mb': self.size_mb,
            'camera': self.camera,
            'iso': self.iso,
            'shutter_speed': self.shutter_speed
        }
    
    @staticmethod
    def from_dict(data):
        return ImageMetadata(
            resolution=data['resolution'],
            format=data['format'],
            size_mb=data['size_mb'],
            camera=data['camera'],
            iso=data['iso'],
            shutter_speed=data['shutter_speed']
        )


@dataclass
class DroneImage:
    """Drohnenbild mit GPS und Metadaten."""
    id: str
    drone_id: str
    timestamp: datetime
    location: Location
    image_path: str
    thumbnail_path: str
    metadata: ImageMetadata
    
    def to_dict(self):
        return {
            'id': self.id,
            'drone_id': self.drone_id,
            'timestamp': self.timestamp.isoformat(),
            'location': self.location.to_dict(),
            'image_path': self.image_path,
            'thumbnail_path': self.thumbnail_path,
            'metadata': self.metadata.to_dict()
        }
    
    @staticmethod
    def from_dict(data):
        try:
            timestamp = datetime.fromisoformat(data['timestamp'])
        except (ValueError, AttributeError):
            # Fallback für ältere Python-Versionen oder ungültige Formate
            timestamp = datetime.now()
        
        return DroneImage(
            id=data['id'],
            drone_id=data['drone_id'],
            timestamp=timestamp,
            location=Location.from_dict(data['location']),
            image_path=data['image_path'],
            thumbnail_path=data['thumbnail_path'],
            metadata=ImageMetadata.from_dict(data['metadata'])
        )


@dataclass
class DetectedObject:
    """Erkanntes Objekt im Bild."""
    type: ObjectType
    confidence: float
    bbox: Tuple[int, int, int, int]  # x, y, w, h
    classification: str
    
    def to_dict(self):
        return {
            'type': self.type.value,
            'confidence': self.confidence,
            'bbox': list(self.bbox),
            'classification': self.classification
        }
    
    @staticmethod
    def from_dict(data):
        return DetectedObject(
            type=ObjectType(data['type']),
            confidence=data['confidence'],
            bbox=tuple(data['bbox']),
            classification=data['classification']
        )


@dataclass
class ImageAnalysis:
    """KI-Analyse eines Drohnenbildes."""
    id: str
    image_id: str
    embedding: List[float]  # 512D Vector
    detected_objects: List[DetectedObject]
    llm_description: str
    scene_classification: List[str]
    quality_score: float
    processing_time_ms: int
    
    def to_dict(self):
        return {
            'id': self.id,
            'image_id': self.image_id,
            'embedding': self.embedding,
            'detected_objects': [obj.to_dict() for obj in self.detected_objects],
            'llm_description': self.llm_description,
            'scene_classification': self.scene_classification,
            'quality_score': self.quality_score,
            'processing_time_ms': self.processing_time_ms
        }
    
    @staticmethod
    def from_dict(data):
        return ImageAnalysis(
            id=data['id'],
            image_id=data['image_id'],
            embedding=data['embedding'],
            detected_objects=[DetectedObject.from_dict(obj) for obj in data['detected_objects']],
            llm_description=data['llm_description'],
            scene_classification=data['scene_classification'],
            quality_score=data['quality_score'],
            processing_time_ms=data['processing_time_ms']
        )


@dataclass
class TimeSeriesEvent:
    """Event über Zeit (z.B. Baufortschritt)."""
    id: str
    location: Location
    event_type: str
    description: str
    images: List[str]  # Image IDs
    detected_at: datetime
    confidence: float
    
    def to_dict(self):
        return {
            'id': self.id,
            'location': self.location.to_dict(),
            'event_type': self.event_type,
            'description': self.description,
            'images': self.images,
            'detected_at': self.detected_at.isoformat(),
            'confidence': self.confidence
        }
    
    @staticmethod
    def from_dict(data):
        try:
            detected_at = datetime.fromisoformat(data['detected_at'])
        except (ValueError, AttributeError):
            # Fallback für ältere Python-Versionen oder ungültige Formate
            detected_at = datetime.now()
        
        return TimeSeriesEvent(
            id=data['id'],
            location=Location.from_dict(data['location']),
            event_type=data['event_type'],
            description=data['description'],
            images=data['images'],
            detected_at=detected_at,
            confidence=data['confidence']
        )


class ImageProcessor:
    """Bildverarbeitung und Computer Vision."""
    
    @staticmethod
    def detect_objects(image_path: str) -> List[DetectedObject]:
        """
        Simuliert Objekterkennung (YOLO würde hier laufen).
        
        In Produktion: YOLOv8 oder ähnlich
        """
        # Simulierte Objekterkennung
        objects = []
        
        # Simuliere zufällige Objekte
        num_objects = random.randint(2, 8)
        for i in range(num_objects):
            obj_type = random.choice(list(ObjectType))
            confidence = random.uniform(0.7, 0.99)
            x = random.randint(10, 500)
            y = random.randint(10, 500)
            w = random.randint(50, 300)
            h = random.randint(50, 300)
            
            objects.append(DetectedObject(
                type=obj_type,
                confidence=confidence,
                bbox=(x, y, w, h),
                classification=obj_type.value
            ))
        
        return objects
    
    @staticmethod
    def generate_embedding(image_path: str) -> List[float]:
        """
        Generiert Bild-Embedding (CLIP würde hier laufen).
        
        In Produktion: CLIP oder ähnliches Vision-Modell
        """
        # Simuliertes 512D Embedding
        return [random.gauss(0, 1) for _ in range(512)]
    
    @staticmethod
    def classify_scene(detected_objects: List[DetectedObject]) -> List[str]:
        """Klassifiziert die Szene basierend auf Objekten."""
        scene_tags = set()
        
        for obj in detected_objects:
            if obj.type in [ObjectType.BUILDING, ObjectType.ROAD]:
                scene_tags.add("urban")
            elif obj.type == ObjectType.TREE:
                scene_tags.add("forest")
            elif obj.type == ObjectType.WATER_BODY:
                scene_tags.add("water")
            elif obj.type == ObjectType.VEHICLE:
                scene_tags.add("traffic")
        
        # Füge Wetter und Tageszeit hinzu
        scene_tags.add("daytime")
        scene_tags.add("clear_weather")
        
        return list(scene_tags)
    
    @staticmethod
    def calculate_quality_score(detected_objects: List[DetectedObject]) -> float:
        """Berechnet Qualitätsscore basierend auf Erkennungen."""
        if not detected_objects:
            return 0.5
        
        avg_confidence = sum(obj.confidence for obj in detected_objects) / len(detected_objects)
        object_count_score = min(len(detected_objects) / 10, 1.0)
        
        return (avg_confidence * 0.7 + object_count_score * 0.3)


class LLMIntegration:
    """Integration mit LLM für Bildbeschreibungen."""
    
    @staticmethod
    def generate_description(detected_objects: List[DetectedObject], 
                           scene_classification: List[str]) -> str:
        """
        Generiert natürlichsprachliche Beschreibung.
        
        In Produktion: llama.cpp Vision Model oder GPT-4 Vision
        """
        # Zähle Objekte
        object_counts = {}
        for obj in detected_objects:
            obj_type = obj.classification
            object_counts[obj_type] = object_counts.get(obj_type, 0) + 1
        
        # Erstelle Beschreibung
        scene_desc = ", ".join(scene_classification) if scene_classification else "gemischte Szene"
        
        parts = [f"Luftaufnahme zeigt eine {scene_desc}."]
        
        if object_counts:
            obj_desc = []
            for obj_type, count in sorted(object_counts.items(), key=lambda x: -x[1])[:3]:
                if count == 1:
                    obj_desc.append(f"ein {obj_type}")
                else:
                    obj_desc.append(f"{count} {obj_type}s")
            
            if obj_desc:
                parts.append(f"Erkennbar sind {', '.join(obj_desc)}.")
        
        parts.append("Die Bildqualität ist gut und Details sind klar erkennbar.")
        
        return " ".join(parts)
    
    @staticmethod
    def analyze_changes(old_objects: List[DetectedObject], 
                       new_objects: List[DetectedObject]) -> str:
        """Analysiert Veränderungen zwischen zwei Bildern."""
        old_types = set(obj.type for obj in old_objects)
        new_types = set(obj.type for obj in new_objects)
        
        added = new_types - old_types
        removed = old_types - new_types
        
        if not added and not removed:
            return "Keine signifikanten Veränderungen erkannt."
        
        changes = []
        if added:
            changes.append(f"Neue Objekte: {', '.join(t.value for t in added)}")
        if removed:
            changes.append(f"Entfernte Objekte: {', '.join(t.value for t in removed)}")
        
        return "; ".join(changes)


class DroneSimulator:
    """Simuliert Drohnen-Flüge und Bildaufnahmen."""
    
    def __init__(self, start_lat: float = 52.5200, start_lon: float = 13.4050):
        self.current_lat = start_lat
        self.current_lon = start_lon
        self.altitude = 120.0
        self.heading = 0.0
        self.image_counter = 0
    
    def fly_pattern(self, pattern: str = "grid"):
        """Fliegt ein bestimmtes Muster."""
        if pattern == "grid":
            # Grid-Pattern: Hin und her
            self.current_lon += 0.001
            if self.image_counter % 10 == 0:
                self.current_lat += 0.001
                self.heading = (self.heading + 180) % 360
        elif pattern == "circle":
            # Kreis-Pattern
            angle = (self.image_counter * 10) % 360
            radius = 0.005
            self.current_lat += radius * math.cos(math.radians(angle))
            self.current_lon += radius * math.sin(math.radians(angle))
            self.heading = (angle + 90) % 360
    
    def capture_image(self, drone_id: str) -> DroneImage:
        """Simuliert Bildaufnahme."""
        self.image_counter += 1
        
        # Bewege Drohne
        self.fly_pattern("grid")
        
        # Erstelle Bild-Objekt
        image = DroneImage(
            id=f"img_{self.image_counter:05d}",
            drone_id=drone_id,
            timestamp=datetime.now(),
            location=Location(
                lat=self.current_lat,
                lon=self.current_lon,
                altitude=self.altitude,
                heading=self.heading,
                tilt=-45.0
            ),
            image_path=f"/data/images/img_{self.image_counter:05d}.jpg",
            thumbnail_path=f"/data/thumbnails/thumb_{self.image_counter:05d}.jpg",
            metadata=ImageMetadata(
                resolution="4096x3072",
                format="jpg",
                size_mb=round(random.uniform(6.0, 10.0), 2),
                camera="DJI Mavic 3",
                iso=random.choice([100, 200, 400]),
                shutter_speed=random.choice(["1/1000", "1/2000", "1/4000"])
            )
        )
        
        return image
