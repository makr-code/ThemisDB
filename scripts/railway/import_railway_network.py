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
        self.stats = {
            "stations": 0,
            "track_segments": 0,
            "signals": 0,
            "switches": 0,
            "level_crossings": 0,
            "errors": []
        }
    
    def import_data(self, data_file: str):
        """Import railway network data into ThemisDB"""
        print(f"Loading data from {data_file}...")
        
        with open(data_file, 'r') as f:
            data = json.load(f)
        
        print(f"Data loaded. Metadata: {data.get('metadata', {})}")
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
            
            if self.stats["track_segments"] % 50 == 0:
                print(f"   Imported {self.stats['track_segments']} segments...", end='\r')
        
        print(f"   ✓ Imported {self.stats['track_segments']} track segments")
    
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
        blob = json.dumps(entity)
        
        try:
            response = requests.put(
                f"{self.themis_url}/entities/{key}",
                json={"blob": blob},
                timeout=5
            )
            return response.status_code in [200, 201]
        except Exception as e:
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
