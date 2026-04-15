"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            db_real_data_integration.py                        ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:07:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     545                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Deutsche Bahn Real Data Integration

Importiert echte Daten von:
1. GovData.de - Schienennetz (Geodaten, Shapefiles)
2. DB API Marketplace - Echtzeit-Fahrplandaten, Stationsdaten

APIs:
- Timetables API (Fahrplan)
- Station Data API (Bahnhöfe)
- StaDa (Station Data + Facilities)
- Betriebsstellen (Operational Points)
- FaSta (Facility Status)

Siehe:
- https://www.govdata.de/suche/daten/schienennetz-deutsche-bahnddea3
- https://developers.deutschebahn.com/db-api-marketplace/apis/
"""

import requests
import json
import os
import zipfile
import shutil
from pathlib import Path
from typing import Dict, List, Optional
from datetime import datetime, timedelta
import xml.etree.ElementTree as ET

class DBRealDataIntegration:
    """Integration mit echten Deutsche Bahn Datenquellen"""
    
    # DB API Marketplace Endpoints
    DB_API_BASE = "https://apis.deutschebahn.com"
    
    # GovData.de Schienennetz
    GOVDATA_SCHIENENNETZ_URL = "https://download-data.deutschebahn.com/static/datasets/schienennetz/"
    
    def __init__(self, db_api_key: Optional[str] = None, cache_dir: str = "./data/db_cache"):
        """
        Initialize DB Real Data Integration
        
        Args:
            db_api_key: Deutsche Bahn API Key (from https://developers.deutschebahn.com/)
            cache_dir: Directory for caching downloaded data
        """
        self.db_api_key = db_api_key or os.environ.get('DB_API_KEY')
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        
        self.headers = {
            'Accept': 'application/json',
        }
        if self.db_api_key:
            self.headers['Authorization'] = f'Bearer {self.db_api_key}'
    
    # =========================================================================
    # GovData.de Schienennetz Integration
    # =========================================================================
    
    def download_schienennetz_geodata(self):
        """
        Download Schienennetz Geodaten von GovData.de
        
        Enthält:
        - Strecken (LineStrings mit Kilometrierung)
        - Betriebsstellen (Bahnhöfe, Haltepunkte, Abzweigstellen)
        - Geschwindigkeiten
        - Elektrifizierung
        
        Format: Shapefile (SHP)
        """
        print("Downloading Schienennetz Geodaten from GovData.de...")
        
        # URLs (Stand: 2024)
        datasets = {
            "strecken": "schienennetz-strecken.zip",
            "betriebsstellen": "schienennetz-betriebsstellen.zip",
            "geschwindigkeiten": "schienennetz-geschwindigkeiten.zip"
        }
        
        downloaded = {}
        
        for name, filename in datasets.items():
            url = self.GOVDATA_SCHIENENNETZ_URL + filename
            output_file = self.cache_dir / filename
            
            if output_file.exists():
                print(f"  ✓ {name} bereits gecached: {output_file}")
                downloaded[name] = output_file
                continue
            
            try:
                print(f"  Downloading {name}...")
                response = requests.get(url, stream=True, timeout=60)
                response.raise_for_status()
                
                with open(output_file, 'wb') as f:
                    for chunk in response.iter_content(chunk_size=8192):
                        f.write(chunk)
                
                print(f"  ✓ Downloaded: {output_file}")
                downloaded[name] = output_file
                
            except requests.exceptions.RequestException as e:
                print(f"  ✗ Error downloading {name}: {e}")
                print(f"    Hinweis: Manuelle Download-URL: {url}")
        
        return downloaded
    
    def extract_schienennetz_shapefiles(self):
        """Extrahiere Shapefiles aus ZIP-Archiven"""
        print("\nExtracting Shapefiles...")
        
        shp_dir = self.cache_dir / "shapefiles"
        shp_dir.mkdir(exist_ok=True)
        
        for zip_file in self.cache_dir.glob("*.zip"):
            extract_dir = shp_dir / zip_file.stem
            
            if extract_dir.exists():
                print(f"  ✓ Already extracted: {extract_dir}")
                continue
            
            try:
                print(f"  Extracting {zip_file.name}...")
                with zipfile.ZipFile(zip_file, 'r') as zip_ref:
                    zip_ref.extractall(extract_dir)
                print(f"  ✓ Extracted to: {extract_dir}")
            except Exception as e:
                print(f"  ✗ Error extracting {zip_file.name}: {e}")
        
        return shp_dir
    
    def parse_shapefile_to_json(self, shapefile_path: Path) -> List[Dict]:
        """
        Parse Shapefile und konvertiere zu JSON
        
        Requires: pyshp (pip install pyshp)
        """
        try:
            import shapefile
        except ImportError:
            print("ERROR: pyshp nicht installiert. Installieren mit: pip install pyshp")
            return []
        
        print(f"\nParsing Shapefile: {shapefile_path}")
        
        try:
            sf = shapefile.Reader(str(shapefile_path))
            features = []
            
            for i, shape_rec in enumerate(sf.shapeRecords()):
                shape = shape_rec.shape
                record = shape_rec.record
                
                # GeoJSON-ähnliche Struktur
                feature = {
                    "type": "Feature",
                    "geometry": {
                        "type": self._shapetype_to_geojson(shape.shapeType),
                        "coordinates": shape.points
                    },
                    "properties": dict(zip([f[0] for f in sf.fields[1:]], record))
                }
                
                features.append(feature)
                
                if (i + 1) % 1000 == 0:
                    print(f"  Parsed {i + 1} features...")
            
            print(f"  ✓ Parsed {len(features)} features")
            return features
            
        except Exception as e:
            print(f"  ✗ Error parsing shapefile: {e}")
            return []
    
    def _shapetype_to_geojson(self, shapetype: int) -> str:
        """Convert Shapefile type to GeoJSON type"""
        types = {
            1: "Point",
            3: "LineString",
            5: "Polygon",
            8: "MultiPoint",
            11: "PointZ",
            13: "LineStringZ",
            15: "PolygonZ"
        }
        return types.get(shapetype, "Unknown")
    
    # =========================================================================
    # DB API Marketplace Integration
    # =========================================================================
    
    def get_all_stations(self, limit: int = 5000) -> List[Dict]:
        """
        Hole alle Bahnhöfe von StaDa API (Station Data)
        
        API: /stada/v2/stations
        
        Returns:
            List of stations with:
            - number (EVA number)
            - name
            - location (lat, lon)
            - category (1-7)
            - facilities
        """
        print("\nFetching stations from DB StaDa API...")
        
        if not self.db_api_key:
            print("  ✗ DB API Key required. Get one from: https://developers.deutschebahn.com/")
            return self._load_fallback_stations()
        
        try:
            url = f"{self.DB_API_BASE}/stada/v2/stations"
            params = {
                'limit': limit
            }
            
            response = requests.get(url, headers=self.headers, params=params, timeout=30)
            response.raise_for_status()
            
            data = response.json()
            stations = data.get('result', [])
            
            print(f"  ✓ Fetched {len(stations)} stations")
            
            # Cache für Offline-Nutzung
            cache_file = self.cache_dir / 'db_stations.json'
            with open(cache_file, 'w', encoding='utf-8') as f:
                json.dump(stations, f, ensure_ascii=False, indent=2)
            
            return stations
            
        except requests.exceptions.RequestException as e:
            print(f"  ✗ API Error: {e}")
            return self._load_fallback_stations()
    
    def get_timetable(self, eva_number: str, date: Optional[str] = None) -> Dict:
        """
        Hole Fahrplan für Bahnhof von Timetables API
        
        API: /timetables/v1/plan/{eva}/{date}/{hour}
        
        Args:
            eva_number: EVA-Nummer des Bahnhofs (z.B. "8000105" für Frankfurt Hbf)
            date: Datum im Format "YYMMDD" (default: heute)
        
        Returns:
            Timetable data with arrivals/departures
        """
        print(f"\nFetching timetable for station {eva_number}...")
        
        if not self.db_api_key:
            print("  ✗ DB API Key required")
            return {}
        
        if not date:
            date = datetime.now().strftime("%y%m%d")
        
        try:
            # Hole Fahrplan für aktuelle Stunde
            hour = datetime.now().hour
            url = f"{self.DB_API_BASE}/timetables/v1/plan/{eva_number}/{date}/{hour:02d}"
            
            response = requests.get(url, headers=self.headers, timeout=30)
            response.raise_for_status()
            
            # Parse XML response
            root = ET.fromstring(response.content)
            
            trains = []
            for s in root.findall('.//s'):  # s = stop
                train = {
                    'train_number': s.get('tl', ''),  # train line
                    'category': s.get('c', ''),       # category (ICE, IC, etc.)
                    'arrival': s.find('.//ar').get('pt') if s.find('.//ar') is not None else None,
                    'departure': s.find('.//dp').get('pt') if s.find('.//dp') is not None else None,
                    'platform': s.find('.//ar').get('pp') if s.find('.//ar') is not None else 
                                s.find('.//dp').get('pp') if s.find('.//dp') is not None else None
                }
                trains.append(train)
            
            print(f"  ✓ Found {len(trains)} trains at {eva_number}")
            return {'eva': eva_number, 'date': date, 'hour': hour, 'trains': trains}
            
        except Exception as e:
            print(f"  ✗ Error: {e}")
            return {}
    
    def get_betriebsstellen(self) -> List[Dict]:
        """
        Hole Betriebsstellen von Betriebsstellen API
        
        API: /betriebsstellen/v1/betriebsstellen
        
        Betriebsstellen = Operational Points:
        - Bahnhöfe
        - Haltepunkte
        - Abzweigstellen
        - Überleitstellen
        - Blockstellen
        """
        print("\nFetching Betriebsstellen from DB API...")
        
        if not self.db_api_key:
            print("  ✗ DB API Key required")
            return []
        
        try:
            url = f"{self.DB_API_BASE}/betriebsstellen/v1/betriebsstellen"
            
            response = requests.get(url, headers=self.headers, timeout=30)
            response.raise_for_status()
            
            data = response.json()
            betriebsstellen = data if isinstance(data, list) else []
            
            print(f"  ✓ Fetched {len(betriebsstellen)} Betriebsstellen")
            
            # Cache
            cache_file = self.cache_dir / 'db_betriebsstellen.json'
            with open(cache_file, 'w', encoding='utf-8') as f:
                json.dump(betriebsstellen, f, ensure_ascii=False, indent=2)
            
            return betriebsstellen
            
        except Exception as e:
            print(f"  ✗ Error: {e}")
            return []
    
    def _load_fallback_stations(self) -> List[Dict]:
        """Lade gecachte Stations-Daten"""
        cache_file = self.cache_dir / 'db_stations.json'
        
        if cache_file.exists():
            print(f"  ℹ Loading cached stations from {cache_file}")
            with open(cache_file, 'r', encoding='utf-8') as f:
                return json.load(f)
        else:
            print("  ℹ No cached data available")
            return []
    
    # =========================================================================
    # Data Conversion for ThemisDB
    # =========================================================================
    
    def convert_stations_to_themisdb(self, stations: List[Dict]) -> List[Dict]:
        """
        Konvertiere DB API Stations zu ThemisDB Format
        
        Input: StaDa API format
        Output: ThemisDB Entity format
        """
        print("\nConverting stations to ThemisDB format...")
        
        themis_stations = []
        
        for station in stations:
            # StaDa liefert manchmal verschiedene Formate
            eva = station.get('number') or station.get('evaNumber', '')
            name = station.get('name', 'Unknown')
            
            # Location
            location = {}
            if 'location' in station:
                loc = station['location']
                location = {
                    'lat': loc.get('latitude', 0.0),
                    'lon': loc.get('longitude', 0.0),
                    'altitude': 0.0  # Nicht in API enthalten
                }
            
            # Category (1-7)
            category_map = {
                1: "Fernverkehrsbahnhof",
                2: "Regionalbahnhof",
                3: "Nahverkehrsbahnhof",
                4: "Haltepunkt",
                5: "Haltestelle",
                6: "Kleinstation",
                7: "Verkehrshalt"
            }
            category_num = station.get('category', 7)
            category = category_map.get(category_num, "Unbekannt")
            
            themis_station = {
                "_key": f"station:{eva}",
                "type": "station",
                "eva_number": eva,
                "name": name,
                "location": location,
                "category": category,
                "operator": "DB Station&Service AG",
                "has_parking": station.get('hasParking', False),
                "has_bicycle_parking": station.get('hasBicycleParking', False),
                "has_public_fac": station.get('hasPublicFacilities', False),
                "has_lockers": station.get('hasLockerSystem', False),
                "has_taxi_rank": station.get('hasTaxiRank', False),
                "has_traveller_needs": station.get('hasTravelNecessities', False),
                "has_steps_free_access": station.get('hasStepFreeAccess', False),
                "db_info_available": station.get('hasDBInformation', False),
                "local_public_transport": station.get('hasLocalPublicTransport', False)
            }
            
            themis_stations.append(themis_station)
        
        print(f"  ✓ Converted {len(themis_stations)} stations")
        return themis_stations
    
    def export_for_themisdb(self, output_file: str = "data/db_real_data.json"):
        """
        Exportiere alle gesammelten Daten im ThemisDB Format
        
        Output: JSON file bereit für import_railway_network.py
        """
        print("\n" + "="*60)
        print("Exporting Real DB Data for ThemisDB")
        print("="*60)
        
        # Sammle alle Daten
        stations = self.get_all_stations()
        themis_stations = self.convert_stations_to_themisdb(stations)
        
        # Betriebsstellen (optional)
        # betriebsstellen = self.get_betriebsstellen()
        
        # Erstelle Export
        export_data = {
            "metadata": {
                "generated_at": datetime.now().isoformat(),
                "source": "Deutsche Bahn API Marketplace + GovData.de",
                "data_quality": "real_db_data",
                "stations_count": len(themis_stations)
            },
            "stations": themis_stations,
            "track_segments": [],  # TODO: Parse from Schienennetz Shapefiles
            "signals": [],
            "switches": [],
            "level_crossings": []
        }
        
        # Schreibe JSON
        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(export_data, f, ensure_ascii=False, indent=2)
        
        print(f"\n✓ Exported to: {output_path}")
        print(f"  Stations: {len(themis_stations)}")
        print("\nNext steps:")
        print(f"  1. python scripts/railway/import_railway_network.py {output_file}")
        print(f"  2. python scripts/railway/train_simulator.py --network {output_file}")

def main():
    """Main function"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Deutsche Bahn Real Data Integration',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Download Schienennetz Geodaten
  python db_real_data_integration.py --download-geodata
  
  # Fetch Stations from DB API
  python db_real_data_integration.py --fetch-stations --api-key YOUR_KEY
  
  # Export all data for ThemisDB
  python db_real_data_integration.py --export --api-key YOUR_KEY
  
  # Get Timetable for Frankfurt Hbf
  python db_real_data_integration.py --timetable 8000105 --api-key YOUR_KEY

API Key:
  Get your free API key from:
  https://developers.deutschebahn.com/
        """
    )
    
    parser.add_argument('--api-key', help='DB API Key (or set DB_API_KEY env var)')
    parser.add_argument('--download-geodata', action='store_true',
                       help='Download Schienennetz Geodaten from GovData.de')
    parser.add_argument('--fetch-stations', action='store_true',
                       help='Fetch stations from DB StaDa API')
    parser.add_argument('--timetable', metavar='EVA',
                       help='Fetch timetable for station (EVA number)')
    parser.add_argument('--export', action='store_true',
                       help='Export all data for ThemisDB')
    parser.add_argument('--output', default='data/db_real_data.json',
                       help='Output file (default: data/db_real_data.json)')
    
    args = parser.parse_args()
    
    # Create integration instance
    integration = DBRealDataIntegration(db_api_key=args.api_key)
    
    # Execute commands
    if args.download_geodata:
        integration.download_schienennetz_geodata()
        integration.extract_schienennetz_shapefiles()
    
    if args.fetch_stations:
        stations = integration.get_all_stations()
        print(f"\nSample station: {json.dumps(stations[0], indent=2, ensure_ascii=False)}")
    
    if args.timetable:
        timetable = integration.get_timetable(args.timetable)
        print(f"\nTimetable: {json.dumps(timetable, indent=2, ensure_ascii=False)}")
    
    if args.export:
        integration.export_for_themisdb(args.output)
    
    if not any([args.download_geodata, args.fetch_stations, args.timetable, args.export]):
        parser.print_help()

if __name__ == "__main__":
    main()
