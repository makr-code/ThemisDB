"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     377                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client für Drohnenbild-Analyse System.

Dieser Client bietet Operationen für:
- Drohnenbilder speichern und abrufen
- Bildanalysen mit Vector-Search
- Zeitreihen-Events
- Geo-Queries
- Ähnliche Bilder finden
"""

import requests
import time
from typing import List, Optional, Dict, Any
from models import (
    DroneImage, ImageAnalysis, TimeSeriesEvent, DetectedObject,
    ImageProcessor, LLMIntegration
)


class DroneAnalysisClient:
    """Client für Drohnenbild-Analyse mit ThemisDB."""
    
    def __init__(self, host: str = "localhost", port: int = 8080, timeout: int = 30):
        """
        Initialisiert den Client.
        
        Args:
            host: ThemisDB Host
            port: ThemisDB Port
            timeout: Request Timeout in Sekunden
        """
        self.base_url = f"http://{host}:{port}"
        self.timeout = timeout
        self.session = requests.Session()
    
    def test_connection(self) -> bool:
        """Testet die Verbindung zu ThemisDB."""
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=5
            )
            return response.status_code == 200
        except Exception:
            return False
    
    # === Drohnenbild Operationen ===
    
    def create_image(self, image: DroneImage) -> bool:
        """Speichert ein Drohnenbild."""
        try:
            response = self.session.post(
                f"{self.base_url}/api/drone_images",
                json=image.to_dict(),
                timeout=self.timeout
            )
            return response.status_code == 201
        except Exception as e:
            print(f"Fehler beim Erstellen: {e}")
            return False
    
    def get_image(self, image_id: str) -> Optional[DroneImage]:
        """Lädt ein Drohnenbild."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/drone_images/{image_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return DroneImage.from_dict(response.json())
            return None
        except Exception as e:
            print(f"Fehler beim Laden: {e}")
            return None
    
    def list_images(self, limit: int = 100) -> List[DroneImage]:
        """Listet alle Drohnenbilder."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/drone_images",
                params={'limit': limit},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [DroneImage.from_dict(img) for img in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Auflisten: {e}")
            return []
    
    def delete_image(self, image_id: str) -> bool:
        """Löscht ein Drohnenbild."""
        try:
            response = self.session.delete(
                f"{self.base_url}/api/drone_images/{image_id}",
                timeout=self.timeout
            )
            return response.status_code == 204
        except Exception as e:
            print(f"Fehler beim Löschen: {e}")
            return False
    
    # === Bildanalyse Operationen ===
    
    def create_analysis(self, analysis: ImageAnalysis) -> bool:
        """Speichert eine Bildanalyse."""
        try:
            response = self.session.post(
                f"{self.base_url}/api/image_analyses",
                json=analysis.to_dict(),
                timeout=self.timeout
            )
            return response.status_code == 201
        except Exception as e:
            print(f"Fehler beim Erstellen der Analyse: {e}")
            return False
    
    def get_analysis(self, analysis_id: str) -> Optional[ImageAnalysis]:
        """Lädt eine Bildanalyse."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/image_analyses/{analysis_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return ImageAnalysis.from_dict(response.json())
            return None
        except Exception as e:
            print(f"Fehler beim Laden der Analyse: {e}")
            return None
    
    def get_analysis_by_image(self, image_id: str) -> Optional[ImageAnalysis]:
        """Lädt Analyse für ein bestimmtes Bild."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/image_analyses/by_image/{image_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return ImageAnalysis.from_dict(response.json())
            return None
        except Exception as e:
            print(f"Fehler beim Laden der Bildanalyse: {e}")
            return None
    
    # === Vector Search Operationen ===
    
    def search_similar_images(self, embedding: List[float], limit: int = 10) -> List[Dict[str, Any]]:
        """
        Sucht ähnliche Bilder basierend auf Embedding.
        
        Returns:
            Liste von Dicts mit 'image_id', 'analysis_id', 'similarity_score'
        """
        try:
            response = self.session.post(
                f"{self.base_url}/api/vector_search/similar_images",
                json={
                    'embedding': embedding,
                    'limit': limit
                },
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return []
        except Exception as e:
            print(f"Fehler bei der Vektorsuche: {e}")
            return []
    
    def search_by_text(self, query: str, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Sucht Bilder nach Textbeschreibung.
        
        In Produktion: Text würde zu Embedding konvertiert
        """
        try:
            response = self.session.post(
                f"{self.base_url}/api/vector_search/by_text",
                json={
                    'query': query,
                    'limit': limit
                },
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return []
        except Exception as e:
            print(f"Fehler bei der Textsuche: {e}")
            return []
    
    # === Geo-Query Operationen ===
    
    def search_by_location(self, lat: float, lon: float, radius_km: float = 1.0) -> List[DroneImage]:
        """Sucht Bilder in der Nähe einer Location."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/geo_search/nearby",
                params={
                    'lat': lat,
                    'lon': lon,
                    'radius_km': radius_km
                },
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [DroneImage.from_dict(img) for img in response.json()]
            return []
        except Exception as e:
            print(f"Fehler bei der Geo-Suche: {e}")
            return []
    
    def search_by_bbox(self, min_lat: float, min_lon: float, 
                       max_lat: float, max_lon: float) -> List[DroneImage]:
        """Sucht Bilder innerhalb einer Bounding Box."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/geo_search/bbox",
                params={
                    'min_lat': min_lat,
                    'min_lon': min_lon,
                    'max_lat': max_lat,
                    'max_lon': max_lon
                },
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [DroneImage.from_dict(img) for img in response.json()]
            return []
        except Exception as e:
            print(f"Fehler bei der BBox-Suche: {e}")
            return []
    
    # === Zeitreihen-Event Operationen ===
    
    def create_event(self, event: TimeSeriesEvent) -> bool:
        """Speichert ein Zeitreihen-Event."""
        try:
            response = self.session.post(
                f"{self.base_url}/api/events",
                json=event.to_dict(),
                timeout=self.timeout
            )
            return response.status_code == 201
        except Exception as e:
            print(f"Fehler beim Erstellen des Events: {e}")
            return False
    
    def get_events(self, event_type: Optional[str] = None) -> List[TimeSeriesEvent]:
        """Lädt Zeitreihen-Events."""
        try:
            params = {}
            if event_type:
                params['event_type'] = event_type
            
            response = self.session.get(
                f"{self.base_url}/api/events",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [TimeSeriesEvent.from_dict(evt) for evt in response.json()]
            return []
        except Exception as e:
            print(f"Fehler beim Laden der Events: {e}")
            return []
    
    # === Statistiken ===
    
    def get_statistics(self) -> Dict[str, Any]:
        """Lädt Statistiken über das System."""
        try:
            response = self.session.get(
                f"{self.base_url}/api/statistics",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {}
        except Exception as e:
            print(f"Fehler beim Laden der Statistiken: {e}")
            return {}
    
    # === Cache Management ===
    
    def clear_cache(self):
        """Leert den lokalen Cache."""
        # Implementierung für lokalen Cache
        pass


class AnalysisProcessor:
    """Helper-Klasse für Batch-Verarbeitung."""
    
    def __init__(self, client: DroneAnalysisClient):
        self.client = client
    
    def process_images_batch(self, images: List[DroneImage], 
                            callback=None) -> List[ImageAnalysis]:
        """
        Verarbeitet mehrere Bilder in einem Batch.
        
        Args:
            images: Liste von Bildern
            callback: Optional callback für Fortschritt
        
        Returns:
            Liste von Analysen
        """
        analyses = []
        total = len(images)
        
        for i, image in enumerate(images):
            start_time = time.time()
            
            # Objekterkennung
            detected_objects = ImageProcessor.detect_objects(image.image_path)
            
            # Embedding generieren
            embedding = ImageProcessor.generate_embedding(image.image_path)
            
            # Szene klassifizieren
            scene_classification = ImageProcessor.classify_scene(detected_objects)
            
            # Qualitätsscore
            quality_score = ImageProcessor.calculate_quality_score(detected_objects)
            
            # LLM Beschreibung
            llm_description = LLMIntegration.generate_description(
                detected_objects, scene_classification
            )
            
            processing_time = int((time.time() - start_time) * 1000)
            
            # Analyse erstellen
            analysis = ImageAnalysis(
                id=f"analysis_{image.id}",
                image_id=image.id,
                embedding=embedding,
                detected_objects=detected_objects,
                llm_description=llm_description,
                scene_classification=scene_classification,
                quality_score=quality_score,
                processing_time_ms=processing_time
            )
            
            analyses.append(analysis)
            
            if callback:
                callback(i + 1, total, image.id)
        
        return analyses
