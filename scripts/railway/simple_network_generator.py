"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            simple_network_generator.py                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     275                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Simple Railway Network Data Generator (No C++ compilation needed)

Generates a realistic German railway network with:
- Major stations with real GPS coordinates
- Track segments with speed profiles
- Signals, switches, level crossings
- All in pure Python (no external dependencies)
"""

import json
import math
import random
from datetime import datetime
from typing import List, Dict, Tuple

class SimpleNetworkGenerator:
    """Generates railway network data in pure Python"""
    
    # Major German stations with real coordinates
    STATIONS = [
        {"id": "FF", "name": "Frankfurt (Main) Hbf", "lat": 50.1070, "lon": 8.6632, "platforms": 24},
        {"id": "MH", "name": "Mannheim Hbf", "lat": 49.4793, "lon": 8.4695, "platforms": 12},
        {"id": "KA", "name": "Karlsruhe Hbf", "lat": 48.9934, "lon": 8.4010, "platforms": 14},
        {"id": "HN", "name": "Heidelberg Hbf", "lat": 49.4039, "lon": 8.6752, "platforms": 8},
        {"id": "DA", "name": "Darmstadt Hbf", "lat": 49.8728, "lon": 8.6303, "platforms": 10},
        {"id": "WI", "name": "Wiesbaden Hbf", "lat": 50.0708, "lon": 8.2437, "platforms": 9},
        {"id": "OF", "name": "Offenburg", "lat": 48.4732, "lon": 7.9404, "platforms": 6},
        {"id": "FR", "name": "Freiburg (Breisgau) Hbf", "lat": 47.9975, "lon": 7.8408, "platforms": 10},
        {"id": "BW", "name": "Bruchsal", "lat": 49.1278, "lon": 8.5980, "platforms": 5},
        {"id": "PF", "name": "Pforzheim Hbf", "lat": 48.8927, "lon": 8.7031, "platforms": 7},
    ]
    
    # Connections between stations (simplified)
    CONNECTIONS = [
        ("FF", "MH", 80, 230),   # Frankfurt - Mannheim (80km, max 230 km/h)
        ("MH", "HN", 20, 200),   # Mannheim - Heidelberg
        ("MH", "KA", 60, 200),   # Mannheim - Karlsruhe
        ("FF", "DA", 30, 160),   # Frankfurt - Darmstadt
        ("FF", "WI", 40, 160),   # Frankfurt - Wiesbaden
        ("DA", "MH", 45, 160),   # Darmstadt - Mannheim
        ("KA", "OF", 60, 250),   # Karlsruhe - Offenburg (High-speed)
        ("OF", "FR", 65, 200),   # Offenburg - Freiburg
        ("KA", "BW", 20, 160),   # Karlsruhe - Bruchsal
        ("BW", "PF", 25, 140),   # Bruchsal - Pforzheim
    ]
    
    def __init__(self):
        self.segments = []
        self.signals = []
        self.switches = []
        self.crossings = []
        
    def calculate_distance(self, lat1: float, lon1: float, lat2: float, lon2: float) -> float:
        """Calculate distance between two GPS coordinates (Haversine formula)"""
        R = 6371  # Earth radius in km
        
        lat1_rad = math.radians(lat1)
        lat2_rad = math.radians(lat2)
        delta_lat = math.radians(lat2 - lat1)
        delta_lon = math.radians(lon2 - lon1)
        
        a = math.sin(delta_lat/2)**2 + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon/2)**2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))
        
        return R * c
    
    def interpolate_point(self, lat1: float, lon1: float, lat2: float, lon2: float, fraction: float) -> Tuple[float, float]:
        """Interpolate GPS coordinate between two points"""
        lat = lat1 + (lat2 - lat1) * fraction
        lon = lon1 + (lon2 - lon1) * fraction
        return lat, lon
    
    def generate_segments(self, from_station: Dict, to_station: Dict, length_km: int, max_speed: int):
        """Generate track segments between two stations"""
        segment_length = 1.0  # 1 km per segment
        num_segments = int(length_km / segment_length)
        
        for i in range(num_segments):
            fraction_start = i / num_segments
            fraction_end = (i + 1) / num_segments
            
            lat_start, lon_start = self.interpolate_point(
                from_station["lat"], from_station["lon"],
                to_station["lat"], to_station["lon"],
                fraction_start
            )
            
            lat_end, lon_end = self.interpolate_point(
                from_station["lat"], from_station["lon"],
                to_station["lat"], to_station["lon"],
                fraction_end
            )
            
            # Determine speed based on position
            distance_from_start = i * segment_length
            distance_from_end = (num_segments - i) * segment_length
            
            # Reduce speed near stations
            if distance_from_start < 5 or distance_from_end < 5:
                speed_limit = 80
            else:
                speed_limit = max_speed
            
            # Randomize slightly for realism
            if random.random() < 0.1:  # 10% chance of temporary restriction
                speed_limit = min(speed_limit, random.choice([120, 140, 160]))
            
            segment = {
                "id": f"SEG_{from_station['id']}_{to_station['id']}_{i:03d}",
                "from_station": from_station["id"],
                "to_station": to_station["id"],
                "segment_index": i,
                "start_lat": lat_start,
                "start_lon": lon_start,
                "end_lat": lat_end,
                "end_lon": lon_end,
                "length_km": segment_length,
                "speed_limit_kmh": speed_limit,
                "electrified": True,
                "tracks": 2
            }
            
            self.segments.append(segment)
            
            # Add signals every 2-3 km
            if i % 2 == 0 and i < num_segments - 1:
                signal = {
                    "id": f"SIG_{from_station['id']}_{to_station['id']}_{i:03d}",
                    "segment_id": segment["id"],
                    "lat": lat_end,
                    "lon": lon_end,
                    "type": "main_signal",
                    "direction": "forward",
                    "current_aspect": "green"
                }
                self.signals.append(signal)
    
    def generate_switches(self):
        """Generate switches at major stations"""
        for station in self.STATIONS:
            # Number of switches based on platform count
            num_switches = max(2, station["platforms"] // 3)
            
            for i in range(num_switches):
                switch = {
                    "id": f"SW_{station['id']}_{i:02d}",
                    "station_id": station["id"],
                    "lat": station["lat"] + random.uniform(-0.002, 0.002),
                    "lon": station["lon"] + random.uniform(-0.002, 0.002),
                    "type": "simple",
                    "position": "normal",
                    "speed_normal_kmh": 200,
                    "speed_diverging_kmh": 60
                }
                self.switches.append(switch)
    
    def generate_level_crossings(self):
        """Generate level crossings (only on regional tracks)"""
        for segment in self.segments:
            # Only on slower tracks
            if segment["speed_limit_kmh"] <= 160 and random.random() < 0.05:  # 5% chance
                crossing = {
                    "id": f"LC_{segment['id']}",
                    "segment_id": segment["id"],
                    "lat": (segment["start_lat"] + segment["end_lat"]) / 2,
                    "lon": (segment["start_lon"] + segment["end_lon"]) / 2,
                    "type": "automatic",
                    "status": "open"
                }
                self.crossings.append(crossing)
    
    def generate(self) -> Dict:
        """Generate complete railway network"""
        print("Generating railway network...")
        
        # Generate segments for all connections
        for from_id, to_id, length, max_speed in self.CONNECTIONS:
            from_station = next(s for s in self.STATIONS if s["id"] == from_id)
            to_station = next(s for s in self.STATIONS if s["id"] == to_id)
            
            self.generate_segments(from_station, to_station, length, max_speed)
            print(f"  ✓ {from_station['name']} → {to_station['name']}: {length} km")
        
        # Generate infrastructure
        self.generate_switches()
        self.generate_level_crossings()
        
        # Build final structure
        network = {
            "metadata": {
                "name": "German Railway Network - Rhine-Main-Neckar Region",
                "version": "1.0.0",
                "generated": datetime.now().isoformat(),
                "generator": "simple_network_generator.py",
                "statistics": {
                    "stations": len(self.STATIONS),
                    "segments": len(self.segments),
                    "signals": len(self.signals),
                    "switches": len(self.switches),
                    "level_crossings": len(self.crossings),
                    "total_track_km": sum(s["length_km"] for s in self.segments)
                }
            },
            "stations": self.STATIONS,
            "track_segments": self.segments,
            "signals": self.signals,
            "switches": self.switches,
            "level_crossings": self.crossings
        }
        
        return network


def main():
    """Generate and save network data"""
    import os
    
    print("═══════════════════════════════════════════════════════")
    print("  Simple Railway Network Data Generator (Python)")
    print("═══════════════════════════════════════════════════════")
    print()
    
    generator = SimpleNetworkGenerator()
    network = generator.generate()
    
    # Create data directory
    os.makedirs("../../data", exist_ok=True)
    
    # Save to JSON
    output_file = "../../data/railway_network_base_germany.json"
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(network, f, indent=2, ensure_ascii=False)
    
    stats = network["metadata"]["statistics"]
    print()
    print("═══════════════════════════════════════════════════════")
    print("✓ Network generated successfully!")
    print("═══════════════════════════════════════════════════════")
    print(f"  Stations:        {stats['stations']}")
    print(f"  Track segments:  {stats['segments']}")
    print(f"  Signals:         {stats['signals']}")
    print(f"  Switches:        {stats['switches']}")
    print(f"  Level crossings: {stats['level_crossings']}")
    print(f"  Total track:     {stats['total_track_km']:.1f} km")
    print()
    print(f"  📁 Saved to: {output_file}")
    print("═══════════════════════════════════════════════════════")


if __name__ == "__main__":
    main()
