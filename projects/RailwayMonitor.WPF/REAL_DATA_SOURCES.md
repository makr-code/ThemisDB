# Reale Datenquellen für Bahnstrecken-Kostenanalyse

## Öffentlich verfügbare Datenquellen

### 1. Herstellungskosten Bahnstrecken

#### Deutsche Bahn / Bundesregierung
- **Quelle**: Bundesverkehrswegeplan (BVWP)
- **URL**: https://www.bmvi.de/bvwp
- **Inhalt**: 
  - Detaillierte Kostenansätze für Schieneninfrastruktur
  - Neubaustrecken, Ausbaustrecken
  - Tunnel, Brücken, Bahnhöfe
- **Format**: PDF, Excel
- **Aktualisierung**: Alle 5 Jahre

#### Eisenbahn-Bundesamt (EBA)
- **Quelle**: Planfeststellungsunterlagen
- **URL**: https://www.eba.bund.de
- **Inhalt**: Kostenaufstellungen genehmigter Projekte
- **Beispiele**:
  - Stuttgart 21: ~8,2 Mrd. € (10,4 km Tunnel)
  - NBS Wendlingen-Ulm: 3,8 Mrd. € (60 km)
  - NBS Karlsruhe-Basel: 6,5 Mrd. € (182 km)

#### Typische Kostensätze (Stand 2024)
```
Neubaustrecke Ebene:        10-15 Mio €/km
Neubaustrecke Hügel:        15-25 Mio €/km
Neubaustrecke Gebirge:      25-40 Mio €/km
Tunnel (eingleisig):        60-100 Mio €/km
Tunnel (zweigleisig):       80-150 Mio €/km
Brücke (Standard):          15-30 Mio €/km
Brücke (Talbrücke):         30-80 Mio €/km
Umbau/Ausbau:               3-8 Mio €/km
Elektrifizierung:           1,5-3 Mio €/km
ETCS Level 2:               0,5-1 Mio €/km
Lärmschutz:                 0,5-2 Mio €/km
```

### 2. Grundstückspreise / Bodenrichtwerte

#### BORIS (Bodenrichtwert-Informationssystem)
- **Quelle**: Gutachterausschüsse der Bundesländer
- **URLs**:
  - Baden-Württemberg: https://www.gutachterausschuesse-bw.de
  - Bayern: https://www.bodenrichtwerte-boris.de
  - Bundesweit: https://www.boris-d.de (geplant)
- **API**: Teilweise vorhanden (BORIS.NRW hat API)
- **Inhalt**: 
  - Bodenrichtwerte pro qm
  - Nach Nutzungsart (Ackerland, Wald, Bauland, etc.)
  - Aktuell alle 2 Jahre
- **Format**: WMS, WFS, REST API

#### OpenData-Portale
- **GovData.de**: https://www.govdata.de
  - Suchbegriff: "Bodenrichtwerte"
  - Format: CSV, GeoJSON, Shapefile
- **Statistisches Bundesamt**: https://www.destatis.de
  - Durchschnittliche Grundstückspreise nach Region
  - Format: Excel, Genesis-Datenbank

#### Typische Bodenrichtwerte (€/qm, Stand 2024)
```
Ackerland:              5-15 €/qm
Grünland:               3-10 €/qm
Wald:                   1-5 €/qm
Bauland (ländlich):     50-200 €/qm
Bauland (Stadt):        300-1.500 €/qm
Bauland (Großstadt):    1.000-10.000 €/qm
Industrieland:          100-500 €/qm
```

### 3. Gelände-/Topographie-Daten

#### DEM (Digital Elevation Model)
- **EU-DEM**: https://land.copernicus.eu/imagery-in-situ/eu-dem
  - 25m Auflösung
  - Kostenlos
  - Format: GeoTIFF
- **SRTM**: https://earthexplorer.usgs.gov
  - 30m Auflösung weltweit
  - Kostenlos
  - Format: HGT, GeoTIFF
- **ASTER GDEM**: https://asterweb.jpl.nasa.gov
  - 30m Auflösung
  - Kostenlos

#### Geoportal.de
- **URL**: https://www.geoportal.de
- **Inhalt**: DGM (Digitales Geländemodell) der Bundesländer
- **Auflösung**: 1m, 5m, 10m
- **Format**: XYZ, ASCII Grid, GeoTIFF

### 4. Schutzgebiete / Restriktionen

#### Natura 2000
- **Quelle**: Bundesamt für Naturschutz (BfN)
- **URL**: https://www.bfn.de/natura-2000
- **API**: WFS-Dienst verfügbar
- **Inhalt**: 
  - FFH-Gebiete
  - Vogelschutzgebiete
  - Naturschutzgebiete
- **Format**: Shapefile, GeoJSON

#### OpenStreetMap (OSM)
- **URL**: https://www.openstreetmap.org
- **API**: Overpass API
- **Inhalt**:
  - Existierende Bahnstrecken
  - Siedlungsgebiete
  - Gewässer
  - Schutzgebiete
- **Format**: XML, JSON

### 5. Verkehrsprognosen

#### Verkehrsverflechtungsprognose
- **Quelle**: Bundesverkehrsministerium
- **Inhalt**: Prognostizierte Verkehrsströme bis 2030/2040
- **Format**: PDF, Excel

## API-Integration Beispiele

### 1. BORIS NRW API
```http
GET https://www.boris.nrw.de/borisapi/api/v2.0/bodenrichtwertzonen
Authorization: Bearer {API_KEY}
Parameters:
  - bbox: Bounding Box (lat/lon)
  - nutzungsart: Ackerland, Grünland, Bauland
```

### 2. Overpass API (OSM)
```http
POST https://overpass-api.de/api/interpreter
Content-Type: application/x-www-form-urlencoded

data=[out:json];
(
  way["railway"="rail"]({{bbox}});
  node["railway"="station"]({{bbox}});
);
out geom;
```

### 3. EU-DEM Download
```bash
wget https://land.copernicus.eu/imagery-in-situ/eu-dem/eu-dem-v1.1/E40N30.tif
```

## Implementierungs-Empfehlungen

### Phase 1: Statische Daten (Sofort verfügbar)
1. Download BORIS-Daten als CSV/Shapefile
2. Download EU-DEM für Deutschland
3. Download Natura 2000 Shapefiles
4. Lokale Datenbank aufbauen

### Phase 2: API-Integration
1. BORIS API anbinden (wo verfügbar)
2. Overpass API für OSM-Daten
3. Caching-Layer einbauen

### Phase 3: Machine Learning
1. Kostenschätzung basierend auf historischen Projekten
2. Regression-Modelle für Preisprognosen
3. Berücksichtigung von Inflation

## Datenbank-Schema

```sql
-- Bodenrichtwerte
CREATE TABLE bodenrichtwerte (
    id SERIAL PRIMARY KEY,
    geometry GEOMETRY(Polygon, 4326),
    preis_pro_qm DECIMAL(10,2),
    nutzungsart VARCHAR(50),
    stichtag DATE,
    quelle VARCHAR(100)
);

-- Höhendaten (Raster)
CREATE TABLE elevation_tiles (
    id SERIAL PRIMARY KEY,
    tile_x INT,
    tile_y INT,
    zoom_level INT,
    elevation_data BYTEA, -- komprimiertes DEM
    min_elevation DECIMAL(6,2),
    max_elevation DECIMAL(6,2)
);

-- Schutzgebiete
CREATE TABLE protected_areas (
    id SERIAL PRIMARY KEY,
    geometry GEOMETRY(MultiPolygon, 4326),
    name VARCHAR(200),
    type VARCHAR(50), -- Natura2000, NSG, LSG
    restrictions TEXT
);

-- Kostensätze
CREATE TABLE construction_costs (
    id SERIAL PRIMARY KEY,
    terrain_type VARCHAR(50),
    construction_type VARCHAR(50), -- track, tunnel, bridge
    cost_per_km DECIMAL(12,2),
    currency VARCHAR(3),
    valid_from DATE,
    source VARCHAR(200)
);
```

## Service-Implementation

```csharp
public interface IRealDataProvider
{
    Task<decimal> GetLandPriceAsync(double lat, double lon);
    Task<decimal> GetConstructionCostAsync(string terrainType, string constructionType);
    Task<bool> IsProtectedAreaAsync(double lat, double lon);
    Task<double> GetElevationAsync(double lat, double lon);
}

public class RealDataProvider : IRealDataProvider
{
    private readonly HttpClient _httpClient;
    private readonly IMemoryCache _cache;
    private readonly string _borisApiKey;
    
    public async Task<decimal> GetLandPriceAsync(double lat, double lon)
    {
        // 1. Check cache
        var cacheKey = $"landprice_{lat:F4}_{lon:F4}";
        if (_cache.TryGetValue(cacheKey, out decimal cachedPrice))
            return cachedPrice;
        
        // 2. Query BORIS API
        var response = await _httpClient.GetAsync(
            $"https://www.boris.nrw.de/borisapi/api/v2.0/bodenrichtwertzonen?" +
            $"lat={lat}&lon={lon}");
        
        if (response.IsSuccessStatusCode)
        {
            var data = await response.Content.ReadFromJsonAsync<BorisResponse>();
            var price = data?.Preis ?? 10; // Fallback
            
            _cache.Set(cacheKey, price, TimeSpan.FromDays(30));
            return price;
        }
        
        // 3. Fallback auf statische Datenbank
        return await GetLandPriceFromDatabaseAsync(lat, lon);
    }
}
```

## Kostenberechnung Integration

```csharp
public async Task<RouteConstructionCost> CalculateRealCostsAsync(OptimalPath path)
{
    var totalCost = 0m;
    var breakdown = new CostBreakdown();
    
    foreach (var segment in path.Segments)
    {
        // 1. Grunderwerbskosten
        var landPrice = await _realDataProvider.GetLandPriceAsync(
            segment.Latitude, 
            segment.Longitude);
        var trackWidth = 15; // Meter (Bahnkörper)
        var landCost = landPrice * segment.LengthKm * 1000 * trackWidth;
        breakdown.LandAcquisition += landCost;
        
        // 2. Baukosten
        var terrain = await _terrainProvider.GetTerrainInfoAsync(
            segment.Latitude, 
            segment.Longitude);
        var constructionCost = await _realDataProvider.GetConstructionCostAsync(
            terrain.Type.ToString(), 
            "track");
        breakdown.Construction += constructionCost * (decimal)segment.LengthKm;
        
        // 3. Schutzgebiete (Zusatzkosten)
        if (await _realDataProvider.IsProtectedAreaAsync(
            segment.Latitude, 
            segment.Longitude))
        {
            breakdown.EnvironmentalMitigation += 5_000_000; // Ausgleichsmaßnahmen
        }
        
        totalCost += landCost + constructionCost * (decimal)segment.LengthKm;
    }
    
    // 4. Planungskosten (10%)
    breakdown.Planning = totalCost * 0.10m;
    
    // 5. Puffer (20%)
    breakdown.Contingency = totalCost * 0.20m;
    
    return new RouteConstructionCost
    {
        TotalCost = totalCost + breakdown.Planning + breakdown.Contingency,
        Breakdown = breakdown,
        Currency = "EUR",
        BasisYear = 2024,
        Confidence = CalculateConfidence(path)
    };
}
```

## Update-Strategie

1. **Täglich**: Bodenrichtwerte-Cache aktualisieren
2. **Wöchentlich**: Neue Schutzgebiete prüfen
3. **Monatlich**: Kostensätze aktualisieren (Inflation)
4. **Jährlich**: DEM-Daten aktualisieren

## Lizenz-Hinweise

- **BORIS-Daten**: Meist CC-BY-4.0
- **EU-DEM**: Kostenlos, Quellenangabe erforderlich
- **OSM**: ODbL-Lizenz
- **Natura 2000**: Frei verfügbar

## Kosten-Schätzung Genauigkeit

| Datenquelle | Genauigkeit | Aktualität |
|-------------|-------------|------------|
| BORIS       | ±10-20%     | 2 Jahre    |
| BVWP        | ±15-30%     | 5 Jahre    |
| DEM         | ±5m Höhe    | 5-10 Jahre |
| OSM         | Variabel    | Laufend    |

**Gesamtgenauigkeit Kostenprognose**: ±20-40% (typisch für Vorplanung)
