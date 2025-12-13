# Deutsche Bahn Real Data Integration

Integration von echten Daten der Deutschen Bahn für das Railway Monitoring System.

## Datenquellen

### 1. GovData.de - Schienennetz Geodaten

**URL**: https://www.govdata.de/suche/daten/schienennetz-deutsche-bahnddea3

**Inhalt**:
- **Streckennetz** (LineStrings mit Kilometrierung)
- **Betriebsstellen** (Bahnhöfe, Haltepunkte, Abzweigstellen)
- **Geschwindigkeiten** (je Streckenabschnitt)
- **Elektrifizierung** (15kV / 25kV / nicht elektrifiziert)

**Format**: Shapefile (SHP) + Metadaten
**Lizenz**: Datenlizenz Deutschland – Namensnennung – Version 2.0
**Update-Frequenz**: Halbjährlich

### 2. DB API Marketplace

**URL**: https://developers.deutschebahn.com/db-api-marketplace/apis/

**APIs**:

#### StaDa - Station Data API
- **Endpoint**: `/stada/v2/stations`
- **Inhalt**: ~5.400 Bahnhöfe in Deutschland
- **Daten**:
  - EVA-Nummer (eindeutige ID)
  - Name, Kategorie (1-7)
  - GPS-Koordinaten
  - Ausstattung (Parkplätze, Aufzüge, DB Info, etc.)
  - Barrierefreiheit
- **Authentifizierung**: API Key (kostenlos)

#### Timetables API
- **Endpoint**: `/timetables/v1/plan/{eva}/{date}/{hour}`
- **Inhalt**: Fahrplandaten (An-/Abfahrten)
- **Daten**:
  - Zugnummer, Zugtyp (ICE, IC, RE, etc.)
  - Ankunft/Abfahrt (geplant & aktuell)
  - Gleis
  - Verspätungen
  - Ausfälle
- **Format**: XML
- **Authentifizierung**: API Key

#### Betriebsstellen API
- **Endpoint**: `/betriebsstellen/v1/betriebsstellen`
- **Inhalt**: ~7.000 Betriebsstellen
- **Typen**:
  - Bf (Bahnhof)
  - Hp (Haltepunkt)
  - Abzw (Abzweigstelle)
  - Üst (Überleitstelle)
  - Bk (Blockstelle)
  - Anst (Anschlussstelle)
- **Daten**: Kürzel, Name, Typ, Status

#### FaSta - Facility Status API
- **Endpoint**: `/fasta/v1/facilities`
- **Inhalt**: Echtzeit-Status von Bahnhofsausstattung
- **Daten**:
  - Aufzüge (Betriebszustand)
  - Rolltreppen (Betriebszustand)
  - Bahnhofsinformationen

## Schnellstart

### Schritt 1: API Key registrieren

1. Gehe zu: https://developers.deutschebahn.com/
2. Registriere Account (kostenlos)
3. Erstelle App / API Key
4. Kopiere API Key

### Schritt 2: Installation

```bash
# Python Dependencies
pip install requests pyshp

# Environment Variable setzen
export DB_API_KEY="your_api_key_here"
```

### Schritt 3: Echte Daten holen

```bash
cd scripts/railway

# Variante A: Nur Stationsdaten (schnell, ~5.400 Bahnhöfe)
python db_real_data_integration.py --fetch-stations --api-key YOUR_KEY

# Variante B: Geodaten + Stations (vollständig, ~2 GB Download)
python db_real_data_integration.py --download-geodata
python db_real_data_integration.py --export --api-key YOUR_KEY

# Variante C: Fahrplandaten für einen Bahnhof
python db_real_data_integration.py --timetable 8000105 --api-key YOUR_KEY
# 8000105 = Frankfurt(Main)Hbf
```

### Schritt 4: Import in ThemisDB

```bash
# Import der echten Daten
python import_railway_network.py data/db_real_data.json
```

### Schritt 5: Live-Simulation mit echten Daten

```bash
# Simulator nutzt echte Bahnhofs-Positionen
python train_simulator.py \
    --network data/db_real_data.json \
    --trains 100 \
    --realistic-timetable
```

## Beispiel-Daten

### StaDa API Response (gekürzt)

```json
{
  "result": [
    {
      "number": 8000105,
      "name": "Frankfurt(Main)Hbf",
      "location": {
        "latitude": 50.1065,
        "longitude": 8.6619
      },
      "category": 1,
      "hasParking": true,
      "hasBicycleParking": true,
      "hasPublicFacilities": true,
      "hasLockerSystem": true,
      "hasTaxiRank": true,
      "hasTravelNecessities": true,
      "hasStepFreeAccess": "yes",
      "hasDBInformation": true,
      "hasLocalPublicTransport": true
    }
  ]
}
```

### Timetables API Response (XML)

```xml
<timetable station="Frankfurt(Main)Hbf">
  <s id="123456789">
    <tl f="ICE" t="p" c="ICE" n="508"/>
    <ar pt="2412131445" pp="7" ppth="München Hbf|Augsburg Hbf|..."/>
    <dp pt="2412131450" pp="7" ppth="...|Hamburg-Altona"/>
  </s>
</timetable>
```

## Konvertierung für ThemisDB

### Stations

```python
# Input: StaDa API
{
  "number": 8000105,
  "name": "Frankfurt(Main)Hbf",
  "category": 1,
  "location": {"latitude": 50.1065, "longitude": 8.6619}
}

# Output: ThemisDB Entity
{
  "_key": "station:8000105",
  "type": "station",
  "eva_number": "8000105",
  "name": "Frankfurt(Main)Hbf",
  "location": {"lat": 50.1065, "lon": 8.6619, "altitude": 98},
  "category": "Fernverkehrsbahnhof",
  "operator": "DB Station&Service AG",
  "facilities": {
    "parking": true,
    "bicycle_parking": true,
    "lockers": true,
    "step_free_access": true,
    "db_information": true
  }
}
```

### Tracks (aus Shapefile)

```python
# Input: Schienennetz Shapefile
{
  "geometry": {
    "type": "LineString",
    "coordinates": [[8.6619, 50.1065], [8.4689, 49.4792]]
  },
  "properties": {
    "STRNR": "3600",  # Streckennummer
    "VMAX": 200,      # Max Speed km/h
    "ELEKART": "15kV", # Electrification
    "KM_A": 0.0,      # Start km
    "KM_E": 73.5      # End km
  }
}

# Output: ThemisDB Edge
{
  "_key": "segment:3600_000_001",
  "_from": "track_point:3600_km0.0",
  "_to": "track_point:3600_km1.0",
  "type": "track_segment",
  "track_number": "3600",
  "max_speed_kmh": 200,
  "electrified": true,
  "voltage": "15kV_16.7Hz"
}
```

## Realistische Fahrplan-Integration

Mit echten Timetables API Daten können realistische Zugbewegungen simuliert werden:

```python
# Hole Fahrplan für heute
timetable = integration.get_timetable("8000105")  # Frankfurt Hbf

# Extrahiere Züge
for train in timetable['trains']:
    print(f"{train['category']} {train['train_number']}")
    print(f"  Arrival: {train['arrival']}, Departure: {train['departure']}")
    print(f"  Platform: {train['platform']}")

# Output:
# ICE 508
#   Arrival: 144500, Departure: 145000
#   Platform: 7
# IC 2314
#   Arrival: 151200, Departure: 151500
#   Platform: 11
```

### Simulator-Integration

```python
# In train_simulator.py
def generate_realistic_schedule(integration):
    """Generiere Fahrplan basierend auf echten DB Daten"""
    
    # Hole echte Fahrpläne für Top 10 Bahnhöfe
    major_stations = ["8000105", "8000261", "8011160"]  # FFM, MUC, HAM
    
    all_trains = []
    for eva in major_stations:
        timetable = integration.get_timetable(eva)
        for train_data in timetable['trains']:
            train = create_train_from_timetable(train_data)
            all_trains.append(train)
    
    return all_trains
```

## Datenqualität & Aktualisierung

### Stationsdaten (StaDa)
- **Aktualität**: Täglich aktualisiert
- **Vollständigkeit**: ~5.400 Stationen (alle DB Stationen)
- **Qualität**: Offizielle DB Daten, sehr hoch

### Geodaten (Schienennetz)
- **Aktualität**: Halbjährlich
- **Vollständigkeit**: Gesamtes DB Netz (~33.000 km)
- **Qualität**: Offizielle Vermessungsdaten

### Fahrplandaten (Timetables)
- **Aktualität**: Echtzeit (Live-Updates)
- **Vollständigkeit**: Alle Züge (ICE, IC, RE, RB)
- **Qualität**: Inkl. Verspätungen, Ausfälle

## Erweiterte Use Cases

### 1. Echtzeit-Verspätungen

```python
# Hole aktuelle Verspätungen
def get_current_delays():
    timetable = integration.get_timetable("8000105")
    
    delays = []
    for train in timetable['trains']:
        planned = train['scheduled_arrival']
        actual = train['actual_arrival']
        delay_min = (actual - planned) / 60
        
        if delay_min > 5:
            delays.append({
                'train': train['train_number'],
                'delay': delay_min
            })
    
    return delays
```

### 2. Netzauslastung

```python
# Analysiere Auslastung des Netzes
def analyze_network_utilization(tracks_data):
    """Berechne Züge pro Stunde pro Strecke"""
    
    utilization = {}
    for track in tracks_data:
        track_id = track['STRNR']
        capacity = track['VMAX'] / 10  # Simplified: trains/hour
        
        # Count trains on this track (from timetable)
        trains_count = count_trains_on_track(track_id)
        
        utilization[track_id] = {
            'capacity': capacity,
            'usage': trains_count,
            'utilization_percent': (trains_count / capacity) * 100
        }
    
    return utilization
```

### 3. Bahnhofs-Auslastung

```python
# Top 10 frequentierte Bahnhöfe
def get_busiest_stations(stations, timetables):
    traffic = {}
    
    for station in stations:
        eva = station['number']
        timetable = timetables.get(eva, {})
        trains_per_hour = len(timetable.get('trains', []))
        
        traffic[station['name']] = trains_per_hour
    
    # Sort by traffic
    sorted_stations = sorted(traffic.items(), key=lambda x: x[1], reverse=True)
    return sorted_stations[:10]
```

## API Rate Limits

### DB API Marketplace
- **Free Tier**: 1.000 Requests/Tag
- **Standard**: 10.000 Requests/Tag
- **Premium**: Unlimited

**Best Practices**:
- Cache Stationsdaten lokal (ändern sich selten)
- Fahrplandaten nur für aktive Simulationen
- Batch-Requests wo möglich

## Troubleshooting

### Problem: API Key funktioniert nicht

```bash
# Test API Key
curl -H "Authorization: Bearer YOUR_KEY" \
  https://apis.deutschebahn.com/stada/v2/stations?limit=1

# Expected: 200 OK with station data
# Error 401: Invalid API Key
```

### Problem: Shapefile kann nicht gelesen werden

```bash
# Installiere pyshp
pip install pyshp

# Prüfe Shapefile
python -c "import shapefile; sf = shapefile.Reader('data/strecken.shp'); print(len(sf))"
```

### Problem: Zu viele API Requests

```python
# Implementiere Caching
import functools
import time

@functools.lru_cache(maxsize=100)
def get_station_cached(eva):
    return integration.get_station(eva)
```

## Lizenz & Attribution

### GovData.de Daten
```
Datenlizenz Deutschland – Namensnennung – Version 2.0
Quelle: Deutsche Bahn AG
URL: https://www.govdata.de/
```

### DB API Daten
```
© Deutsche Bahn AG
API Marketplace: https://developers.deutschebahn.com/
Nutzungsbedingungen: https://developers.deutschebahn.com/terms
```

## Nächste Schritte

1. ✅ API Key registrieren
2. ✅ Echte Stationsdaten holen
3. ⏳ Shapefile-Parser implementieren (Strecken)
4. ⏳ Fahrplan-Integration in Simulator
5. ⏳ Echtzeit-Verspätungs-Feed
6. ⏳ Grafana Dashboard mit echten Daten

## Support

- **DB API Support**: https://developers.deutschebahn.com/support
- **GovData.de**: info@govdata.de
- **ThemisDB Issues**: https://github.com/makr-code/ThemisDB/issues
