"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     135                                            ║
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
Datenmodelle für TSP-Anwendung
"""

from dataclasses import dataclass, field
from typing import List, Optional
import math


@dataclass
class City:
    """Repräsentiert eine Stadt im TSP."""
    
    id: str
    name: str
    x: float  # X-Koordinate (Breitengrad bei GPS)
    y: float  # Y-Koordinate (Längengrad bei GPS)
    country: str = ""
    
    def distance_to(self, other: 'City') -> float:
        """
        Berechnet euklidische Distanz zu anderer Stadt.
        
        Args:
            other: Andere Stadt
            
        Returns:
            Distanz in gleicher Einheit wie Koordinaten
        """
        dx = self.x - other.x
        dy = self.y - other.y
        return math.sqrt(dx * dx + dy * dy)
    
    def __str__(self) -> str:
        return f"{self.name} ({self.x:.2f}, {self.y:.2f})"
    
    def __hash__(self) -> int:
        return hash(self.id)
    
    def __eq__(self, other) -> bool:
        if not isinstance(other, City):
            return False
        return self.id == other.id


@dataclass
class Route:
    """Repräsentiert eine TSP-Route."""
    
    cities: List[City]
    total_distance: float
    algorithm: str
    computation_time: float = 0.0  # in Sekunden
    iterations: int = 0  # für iterative Algorithmen
    
    def __str__(self) -> str:
        city_names = " → ".join(c.name for c in self.cities)
        return (f"Route ({self.algorithm}): {city_names}\n"
                f"Distanz: {self.total_distance:.2f}\n"
                f"Zeit: {self.computation_time*1000:.2f} ms")
    
    def get_order(self) -> List[str]:
        """Gibt Reihenfolge der Stadt-IDs zurück."""
        return [city.id for city in self.cities]


@dataclass
class DistanceMatrix:
    """Vorberechnete Distanzmatrix für effiziente Lookups."""
    
    cities: List[City]
    _matrix: List[List[float]] = field(default_factory=list, init=False)
    _city_to_index: dict = field(default_factory=dict, init=False)
    
    def __post_init__(self):
        """Berechnet Distanzmatrix."""
        n = len(self.cities)
        self._matrix = [[0.0] * n for _ in range(n)]
        
        # Index-Mapping erstellen
        for i, city in enumerate(self.cities):
            self._city_to_index[city.id] = i
        
        # Distanzen berechnen (symmetrisch)
        for i in range(n):
            for j in range(i + 1, n):
                dist = self.cities[i].distance_to(self.cities[j])
                self._matrix[i][j] = dist
                self._matrix[j][i] = dist
    
    def get_distance(self, city1: City, city2: City) -> float:
        """
        Gibt Distanz zwischen zwei Städten zurück.
        
        Args:
            city1: Erste Stadt
            city2: Zweite Stadt
            
        Returns:
            Distanz
        """
        i = self._city_to_index[city1.id]
        j = self._city_to_index[city2.id]
        return self._matrix[i][j]
    
    def get_distance_by_index(self, i: int, j: int) -> float:
        """Gibt Distanz zwischen Städten an Indizes i und j zurück."""
        return self._matrix[i][j]
    
    def get_city_index(self, city: City) -> int:
        """Gibt Index einer Stadt zurück."""
        return self._city_to_index[city.id]
