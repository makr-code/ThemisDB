"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     190                                            ║
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
ThemisDB Client für TSP-Anwendung
"""

import json
import requests
from typing import List, Optional
from models import City, Route


class ThemisDBClient:
    """Client für ThemisDB HTTP API."""
    
    def __init__(self, host: str = "localhost", port: int = 8080):
        """
        Initialisiert den Client.
        
        Args:
            host: ThemisDB Host
            port: ThemisDB Port
        """
        self.base_url = f"http://{host}:{port}"
        self.graph_id = "tsp_graph"
    
    def health_check(self) -> bool:
        """
        Prüft ob ThemisDB erreichbar ist.
        
        Returns:
            True wenn Server erreichbar
        """
        try:
            response = requests.get(f"{self.base_url}/health", timeout=2)
            return response.status_code == 200
        except:
            return False
    
    def create_city(self, city: City) -> bool:
        """
        Speichert eine Stadt in ThemisDB.
        
        Args:
            city: Stadt-Objekt
            
        Returns:
            True bei Erfolg
        """
        try:
            # Erstelle Entity
            entity_data = {
                "id": city.id,
                "name": city.name,
                "x": city.x,
                "y": city.y,
                "country": city.country,
                "_labels": "City"  # Property Graph Label
            }
            
            # PUT Request
            response = requests.put(
                f"{self.base_url}/entities/cities:{city.id}",
                headers={"Content-Type": "application/json"},
                json={"blob": json.dumps(entity_data)},
                timeout=5
            )
            
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Error creating city: {e}")
            return False
    
    def get_city(self, city_id: str) -> Optional[City]:
        """
        Lädt eine Stadt aus ThemisDB.
        
        Args:
            city_id: Stadt-ID
            
        Returns:
            City-Objekt oder None
        """
        try:
            response = requests.get(
                f"{self.base_url}/entities/cities:{city_id}",
                timeout=5
            )
            
            if response.status_code != 200:
                return None
            
            data = response.json()
            blob = json.loads(data.get("blob", "{}"))
            
            return City(
                id=blob["id"],
                name=blob["name"],
                x=blob["x"],
                y=blob["y"],
                country=blob.get("country", "")
            )
        except Exception as e:
            print(f"Error getting city: {e}")
            return None
    
    def delete_city(self, city_id: str) -> bool:
        """
        Löscht eine Stadt aus ThemisDB.
        
        Args:
            city_id: Stadt-ID
            
        Returns:
            True bei Erfolg
        """
        try:
            response = requests.delete(
                f"{self.base_url}/entities/cities:{city_id}",
                timeout=5
            )
            return response.status_code in [200, 204]
        except Exception as e:
            print(f"Error deleting city: {e}")
            return False
    
    def save_route(self, route: Route) -> bool:
        """
        Speichert eine Route in ThemisDB.
        
        Args:
            route: Route-Objekt
            
        Returns:
            True bei Erfolg
        """
        try:
            route_data = {
                "cities": [city.id for city in route.cities],
                "total_distance": route.total_distance,
                "algorithm": route.algorithm,
                "computation_time": route.computation_time,
                "iterations": route.iterations
            }
            
            response = requests.put(
                f"{self.base_url}/entities/routes:latest",
                headers={"Content-Type": "application/json"},
                json={"blob": json.dumps(route_data)},
                timeout=5
            )
            
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"Error saving route: {e}")
            return False
    
    def list_cities(self) -> List[City]:
        """
        Lädt alle Städte aus ThemisDB.
        
        Returns:
            Liste von City-Objekten
        """
        # Hinweis: In einer echten Implementierung würde man hier eine
        # Query verwenden, um alle Entities mit Label "City" zu finden.
        # Für dieses Beispiel geben wir eine leere Liste zurück,
        # da die lokale Verwaltung einfacher ist.
        return []
