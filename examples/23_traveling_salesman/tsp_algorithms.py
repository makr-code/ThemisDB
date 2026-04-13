"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tsp_algorithms.py                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     251                                            ║
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
TSP-Algorithmen Implementierung
"""

import time
import math
from typing import List, Tuple
from itertools import permutations
from models import City, Route, DistanceMatrix


class TSPSolver:
    """Sammlung von TSP-Lösungsalgorithmen."""
    
    @staticmethod
    def brute_force(cities: List[City]) -> Route:
        """
        Brute Force - Exakte Lösung durch Testen aller Permutationen.
        Nur für kleine n geeignet (n ≤ 10).
        
        Args:
            cities: Liste der Städte
            
        Returns:
            Optimale Route
        """
        if len(cities) < 3:
            raise ValueError("Mindestens 3 Städte erforderlich")
        
        start_time = time.time()
        dist_matrix = DistanceMatrix(cities)
        
        # Erste Stadt fixieren, Rest permutieren
        first_city = cities[0]
        other_cities = cities[1:]
        
        best_route = None
        best_distance = float('inf')
        
        # Teste alle Permutationen
        for perm in permutations(other_cities):
            route_cities = [first_city] + list(perm)
            
            # Berechne Distanz
            distance = 0.0
            for i in range(len(route_cities) - 1):
                distance += dist_matrix.get_distance(route_cities[i], route_cities[i+1])
            # Zurück zum Start
            distance += dist_matrix.get_distance(route_cities[-1], first_city)
            
            if distance < best_distance:
                best_distance = distance
                best_route = route_cities + [first_city]
        
        computation_time = time.time() - start_time
        
        return Route(
            cities=best_route,
            total_distance=best_distance,
            algorithm="Brute Force",
            computation_time=computation_time
        )
    
    @staticmethod
    def nearest_neighbor(cities: List[City], start_city: City = None) -> Route:
        """
        Nearest Neighbor (Greedy) - Wähle immer die nächste unbesuchte Stadt.
        Schnell, aber oft suboptimal.
        
        Args:
            cities: Liste der Städte
            start_city: Optionale Startstadt (sonst erste Stadt)
            
        Returns:
            Route
        """
        if len(cities) < 3:
            raise ValueError("Mindestens 3 Städte erforderlich")
        
        start_time = time.time()
        dist_matrix = DistanceMatrix(cities)
        
        if start_city is None:
            start_city = cities[0]
        
        unvisited = set(cities) - {start_city}
        route = [start_city]
        current = start_city
        total_distance = 0.0
        
        while unvisited:
            # Finde nächste Stadt
            nearest = min(unvisited, key=lambda city: dist_matrix.get_distance(current, city))
            
            distance = dist_matrix.get_distance(current, nearest)
            total_distance += distance
            
            route.append(nearest)
            current = nearest
            unvisited.remove(nearest)
        
        # Zurück zum Start
        total_distance += dist_matrix.get_distance(current, start_city)
        route.append(start_city)
        
        computation_time = time.time() - start_time
        
        return Route(
            cities=route,
            total_distance=total_distance,
            algorithm="Nearest Neighbor",
            computation_time=computation_time
        )
    
    @staticmethod
    def two_opt(cities: List[City], initial_route: Route = None) -> Route:
        """
        2-Opt Heuristik - Verbessert Route durch Kantentausch.
        Startet mit Nearest Neighbor wenn keine initiale Route gegeben.
        
        Args:
            cities: Liste der Städte
            initial_route: Optionale initiale Route
            
        Returns:
            Verbesserte Route
        """
        if len(cities) < 3:
            raise ValueError("Mindestens 3 Städte erforderlich")
        
        start_time = time.time()
        dist_matrix = DistanceMatrix(cities)
        
        # Initiale Route
        if initial_route is None:
            initial_route = TSPSolver.nearest_neighbor(cities)
        
        route = initial_route.cities[:-1]  # Ohne letzte Stadt (= erste)
        improved = True
        iterations = 0
        
        while improved:
            improved = False
            iterations += 1
            
            for i in range(1, len(route) - 1):
                for j in range(i + 1, len(route)):
                    # 2-opt: Remove edges (route[i-1], route[i]) and (route[j], route[(j+1)%n])
                    # Add edges (route[i-1], route[j]) and (route[i], route[(j+1)%n])
                    # This is done by reversing the segment from i to j
                    
                    n = len(route)
                    j_next = (j + 1) % n
                    
                    # Calculate old distance
                    old_dist = (dist_matrix.get_distance(route[i-1], route[i]) +
                               dist_matrix.get_distance(route[j], route[j_next]))
                    
                    # Calculate new distance after reversal
                    new_dist = (dist_matrix.get_distance(route[i-1], route[j]) +
                               dist_matrix.get_distance(route[i], route[j_next]))
                    
                    if new_dist < old_dist:
                        # 2-opt Swap: Reverse segment from i to j (inclusive)
                        route[i:j+1] = reversed(route[i:j+1])
                        improved = True
                        break
                
                if improved:
                    break
        
        # Berechne finale Distanz
        total_distance = 0.0
        for i in range(len(route) - 1):
            total_distance += dist_matrix.get_distance(route[i], route[i+1])
        total_distance += dist_matrix.get_distance(route[-1], route[0])
        
        # Füge Startstadt am Ende hinzu
        final_route = route + [route[0]]
        
        computation_time = time.time() - start_time
        
        return Route(
            cities=final_route,
            total_distance=total_distance,
            algorithm="2-Opt",
            computation_time=computation_time,
            iterations=iterations
        )
    
    @staticmethod
    def multi_start_nearest_neighbor(cities: List[City], num_starts: int = None) -> Route:
        """
        Multi-Start Nearest Neighbor - Probiert verschiedene Startstädte.
        
        Args:
            cities: Liste der Städte
            num_starts: Anzahl verschiedener Starts (default: alle)
            
        Returns:
            Beste gefundene Route
        """
        if len(cities) < 3:
            raise ValueError("Mindestens 3 Städte erforderlich")
        
        if num_starts is None:
            num_starts = len(cities)
        
        start_time = time.time()
        
        best_route = None
        best_distance = float('inf')
        
        for i, start_city in enumerate(cities[:num_starts]):
            route = TSPSolver.nearest_neighbor(cities, start_city)
            
            if route.total_distance < best_distance:
                best_distance = route.total_distance
                best_route = route
        
        computation_time = time.time() - start_time
        
        return Route(
            cities=best_route.cities,
            total_distance=best_route.total_distance,
            algorithm=f"Multi-Start NN ({num_starts})",
            computation_time=computation_time
        )
