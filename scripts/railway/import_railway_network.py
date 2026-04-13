"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_railway_network.py                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:23:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     316                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Railway Network Data Importer for ThemisDB

Importiert Basis-Streckendaten in ThemisDB:
- Bahnhöfe als Graph Vertices
- Strecken als Graph Edges  
- Signale, Weichen als Vertices
- Verbindungen zwischen Infrastruktur-Elementen

Verwendet JSON-Ausgabe von railway_base_data_generator.cpp
"""

import json
import requests
import sys
from typing import Dict, List, Any
from datetime import datetime

class ThemisDBImporter:
    def __init__(self, themis_url: str = "http://localhost:8765"):
        self.themis_url = themis_url
        self.track_network = {}  # For graph connections
        self.stats = {
            "stations": 0,
            "track_segments": 0,
            "signals": 0,
            "switches": 0,
            "level_crossings": 0,
            "connections": 0,
            "errors": []
        }
    
    def import_data(self, data_file: str):
        """Import railway network data into ThemisDB"""
        print(f"Loading data from {data_file}...")
        
        with open(data_file, 'r') as f:
            data = json.load(f)
        
        print(f"Data loaded. Metadata: {data.get('metadata', {})}")
        
        # Store network data for graph connections
        self.track_network = {
            'stations': {s['eva_number']: s for s in data.get('stations', [])}
        }
        
        print("\nImporting into ThemisDB...")
        
        # Import in order (dependencies)
        self._import_stations(data.get('stations', []))
        self._import_track_segments(data.get('track_segments', []))
        self._import_signals(data.get('signals', []))
        self._import_switches(data.get('switches', []))
        self._import_level_crossings(data.get('level_crossings', []))
        
        self._print_stats()
    
    def _import_stations(self, stations: List[Dict]):
        """Import stations as graph vertices"""
        print("\n1. Importing stations...")
        
        for station in stations:
            entity = {
                "_key": f"station:{station['eva_number']}",
                "type": "station",
                **station
            }
            
            if self._put_entity(entity):
                self.stats["stations"] += 1
            
            if self.stats["stations"] % 5 == 0:
                print(f"   Imported {self.stats['stations']} stations...", end='\r')
        
        print(f"   ✓ Imported {self.stats['stations']} stations")
    
    def _import_track_segments(self, segments: List[Dict]):
        """Import track segments as graph edges"""
        print("\n2. Importing track segments...")
        
        for i, seg in enumerate(segments):
            # Create track points (vertices) for segment endpoints
            start_point = {
                "_key": f"track_point:{seg['track_number']}_km{seg['start_km']:.1f}",
                "type": "track_point",
                "track_number": seg['track_number'],
                "km": seg['start_km'],
                "location": {
                    "lat": seg['geometry']['coordinates'][0][1],
                    "lon": seg['geometry']['coordinates'][0][0],
                    "altitude": seg['geometry']['coordinates'][0][2]
                }
            }
            self._put_entity(start_point)
            
            end_point = {
                "_key": f"track_point:{seg['track_number']}_km{seg['end_km']:.1f}",
                "type": "track_point",
                "track_number": seg['track_number'],
                "km": seg['end_km'],
                "location": {
                    "lat": seg['geometry']['coordinates'][1][1],
                    "lon": seg['geometry']['coordinates'][1][0],
                    "altitude": seg['geometry']['coordinates'][1][2]
                }
            }
            self._put_entity(end_point)
            
            # Create segment as edge
            edge = {
                "_key": f"segment:{seg['track_number']}_{seg['segment_id']}",
                "_from": start_point["_key"],
                "_to": end_point["_key"],
                "type": "track_segment",
                **seg
            }
            
            if self._put_entity(edge):
                self.stats["track_segments"] += 1
            
            # Create graph edges for routing (Phase 2.4)
            # Connect stations to nearby track points
            self._create_station_connections(seg, start_point, end_point)
            
            if self.stats["track_segments"] % 50 == 0:
                print(f"   Imported {self.stats['track_segments']} segments...", end='\r')
        
        print(f"   ✓ Imported {self.stats['track_segments']} track segments")
    
    def _create_station_connections(self, segment: Dict, start_point: Dict, end_point: Dict):
        """Create graph edges between stations and track points for routing"""
        # This creates bidirectional edges for graph traversal
        # Simplified: Connect if track passes near station
        for station_eva, station in self.track_network.get('stations', {}).items():
            # Check if either endpoint is near this station (simplified distance check)
            start_dist = self._point_distance(
                start_point['location'], station.get('location', {})
            )
            end_dist = self._point_distance(
                end_point['location'], station.get('location', {})
            )
            
            # If track point within 2km of station, create connection
            if start_dist < 2.0:
                connection = {
                    "_key": f"conn:station_{station_eva}_track_{start_point['_key']}",
                    "_from": f"station:{station_eva}",
                    "_to": start_point["_key"],
                    "type": "station_track_connection",
                    "distance_km": round(start_dist, 3)
                }
                if self._put_entity(connection):
                    self.stats["connections"] += 1
            
            if end_dist < 2.0:
                connection = {
                    "_key": f"conn:station_{station_eva}_track_{end_point['_key']}",
                    "_from": f"station:{station_eva}",
                    "_to": end_point["_key"],
                    "type": "station_track_connection",
                    "distance_km": round(end_dist, 3)
                }
                if self._put_entity(connection):
                    self.stats["connections"] += 1
    
    def _point_distance(self, loc1: Dict, loc2: Dict) -> float:
        """Simple Haversine distance calculation between two points (km)"""
        if not loc1 or not loc2 or 'lat' not in loc1 or 'lat' not in loc2:
            return 999.9  # Invalid distance
        
        import math
        R = 6371  # Earth radius in km
        
        lat1, lon1 = math.radians(loc1['lat']), math.radians(loc1['lon'])
        lat2, lon2 = math.radians(loc2['lat']), math.radians(loc2['lon'])
        
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        
        a = math.sin(dlat/2)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon/2)**2
        c = 2 * math.asin(math.sqrt(a))
        
        return R * c
    
    def _import_signals(self, signals: List[Dict]):
        """Import signals as vertices"""
        print("\n3. Importing signals...")
        
        for signal in signals:
            entity = {
                "_key": f"signal:{signal['signal_id']}",
                "type": "signal",
                **signal,
                "current_aspect": "Hp0_rot",  # Default: red/stop
                "status": "operational"
            }
            
            if self._put_entity(entity):
                self.stats["signals"] += 1
            
            if self.stats["signals"] % 50 == 0:
                print(f"   Imported {self.stats['signals']} signals...", end='\r')
        
        print(f"   ✓ Imported {self.stats['signals']} signals")
    
    def _import_switches(self, switches: List[Dict]):
        """Import switches as vertices"""
        print("\n4. Importing switches...")
        
        for switch in switches:
            entity = {
                "_key": f"switch:{switch['switch_id']}",
                "type": "switch",
                **switch,
                "current_position": "straight",
                "locked": False,
                "status": "operational"
            }
            
            if self._put_entity(entity):
                self.stats["switches"] += 1
            
            if self.stats["switches"] % 20 == 0:
                print(f"   Imported {self.stats['switches']} switches...", end='\r')
        
        print(f"   ✓ Imported {self.stats['switches']} switches")
    
    def _import_level_crossings(self, crossings: List[Dict]):
        """Import level crossings as vertices"""
        print("\n5. Importing level crossings...")
        
        for crossing in crossings:
            entity = {
                "_key": f"crossing:{crossing['crossing_id']}",
                "type": "level_crossing",
                **crossing,
                "barrier_status": "open",
                "status": "operational"
            }
            
            if self._put_entity(entity):
                self.stats["level_crossings"] += 1
        
        print(f"   ✓ Imported {self.stats['level_crossings']} level crossings")
    
    def _put_entity(self, entity: Dict) -> bool:
        """PUT entity to ThemisDB"""
        key = entity["_key"]
        
        try:
            # Send entity directly as JSON (not double-encoded)
            response = requests.put(
                f"{self.themis_url}/entities/{key}",
                json=entity,
                timeout=5
            )
            return response.status_code in [200, 201]
        except Exception as e:
            print(f"[ERROR] Error importing {key}: {e}", file=sys.stderr)
            self.stats["errors"].append(f"Error importing {key}: {e}")
            return False
    
    def _print_stats(self):
        """Print import statistics"""
        print("\n" + "="*60)
        print("Import Complete!")
        print("="*60)
        print(f"Stations:        {self.stats['stations']}")
        print(f"Track Segments:  {self.stats['track_segments']}")
        print(f"Signals:         {self.stats['signals']}")
        print(f"Switches:        {self.stats['switches']}")
        print(f"Level Crossings: {self.stats['level_crossings']}")
        print(f"Graph Connections: {self.stats['connections']}")
        print(f"Errors:          {len(self.stats['errors'])}")
        
        if self.stats['errors']:
            print("\nErrors encountered:")
            for err in self.stats['errors'][:10]:
                print(f"  - {err}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python import_railway_network.py <data_file.json>")
        print("\nExample:")
        print("  python import_railway_network.py data/railway_network_base_germany.json")
        sys.exit(1)
    
    data_file = sys.argv[1]
    themis_url = sys.argv[2] if len(sys.argv) > 2 else "http://localhost:8765"
    
    importer = ThemisDBImporter(themis_url)
    importer.import_data(data_file)
