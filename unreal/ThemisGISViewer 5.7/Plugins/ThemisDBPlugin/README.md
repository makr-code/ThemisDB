# ThemisDB Plugin

## Übersicht

Unreal Engine 5 Plugin für die Integration mit ThemisDB - einem Multi-Model Database System für Geodaten.

## Features

- ✅ **HTTP Client**: Async REST API Calls
- ✅ **Blueprint Support**: Alle Funktionen Blueprint-zugänglich
- ✅ **Geo Utilities**: Koordinaten-Transformation (Geo ↔ Unreal World)
- ✅ **Typed Structures**: FOSMBuilding, FGeoLocation, FTerrainData
- ✅ **Async Queries**: Non-blocking Datenabfragen

## Installation

Dieses Plugin ist bereits im ThemisGISViewer-Projekt enthalten unter:
```
Plugins/ThemisDBPlugin/
```

## Verwendung

### C++ Beispiel

```cpp
#include "ThemisDBClient.h"

void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Create client
    UThemisDBClient* Client = NewObject<UThemisDBClient>();
    Client->Initialize("http://localhost:8765");
    
    // Query buildings
    FGeoLocation SW(52.5, 13.4);
    FGeoLocation NE(52.55, 13.45);
    
    FOnBuildingsReceived OnComplete;
    OnComplete.BindDynamic(this, &AMyActor::OnBuildingsReceived);
    
    Client->QueryBuildingsAsync(SW, NE, OnComplete);
}

void AMyActor::OnBuildingsReceived(const TArray<FOSMBuilding>& Buildings)
{
    UE_LOG(LogTemp, Log, TEXT("Received %d buildings"), Buildings.Num());
    
    for (const FOSMBuilding& Building : Buildings)
    {
        // Spawn procedural building
        SpawnBuilding(Building);
    }
}
```

### Blueprint Beispiel

1. **ThemisDB Client erstellen**:
   - `Create ThemisDBClient` Node
   - `Initialize` (ServerURL: "http://localhost:8765")

2. **Gebäude abfragen**:
   - `Query Buildings Async` Node
   - Input: SouthWest, NorthEast (FGeoLocation)
   - Output: OnComplete Delegate → Array<FOSMBuilding>

3. **Koordinaten umrechnen**:
   - `Geo To World` (FGeoLocation → FVector)
   - `World To Geo` (FVector → FGeoLocation)

## API Reference

### UThemisDBClient

#### Initialize
```cpp
void Initialize(const FString& ServerURL, int32 Port = 8765)
```
Initialisiert Verbindung zum ThemisDB Server.

#### QueryBuildingsAsync
```cpp
void QueryBuildingsAsync(
    FGeoLocation SouthWest,
    FGeoLocation NorthEast,
    FOnBuildingsReceived OnComplete
)
```
Lädt Gebäude in einer Bounding Box.

#### ExecuteAQLAsync
```cpp
void ExecuteAQLAsync(
    const FString& AQLQuery,
    FOnQueryComplete OnComplete
)
```
Führt beliebige AQL-Query aus.

### UThemisDBBlueprintLibrary

#### GeoToWorld
```cpp
static FVector GeoToWorld(FGeoLocation GeoLocation, FGeoLocation WorldOrigin)
```
Konvertiert Geo-Koordinaten in Unreal World Space.

**Beispiel**:
```cpp
FGeoLocation BerlinMitte(52.520008, 13.404954);
FGeoLocation WorldOrigin(52.5, 13.4);
FVector WorldPos = UThemisDBBlueprintLibrary::GeoToWorld(BerlinMitte, WorldOrigin);
```

#### WorldToGeo
```cpp
static FGeoLocation WorldToGeo(FVector WorldLocation, FGeoLocation WorldOrigin)
```
Konvertiert Unreal World Space in Geo-Koordinaten.

#### GeoDistance
```cpp
static float GeoDistance(FGeoLocation A, FGeoLocation B)
```
Berechnet Distanz zwischen zwei Geo-Punkten (Haversine-Formel).

## Datenstrukturen

### FGeoLocation
```cpp
USTRUCT(BlueprintType)
struct FGeoLocation
{
    UPROPERTY(BlueprintReadWrite)
    double Latitude;   // Breitengrad
    
    UPROPERTY(BlueprintReadWrite)
    double Longitude;  // Längengrad
    
    UPROPERTY(BlueprintReadWrite)
    double Altitude;   // Höhe (Meter)
};
```

### FOSMBuilding
```cpp
USTRUCT(BlueprintType)
struct FOSMBuilding
{
    UPROPERTY(BlueprintReadWrite)
    FString ID;
    
    UPROPERTY(BlueprintReadWrite)
    TArray<FGeoLocation> Footprint;  // Grundriss-Polygon
    
    UPROPERTY(BlueprintReadWrite)
    float Height;  // Gebäudehöhe (Meter)
    
    UPROPERTY(BlueprintReadWrite)
    FString BuildingType;  // z.B. "residential", "commercial"
    
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> Tags;  // OSM Tags
};
```

## ThemisDB AQL Queries

### Gebäude in Radius
```sql
FOR building IN osm_buildings
  FILTER GEO_DISTANCE(
    building.location,
    GEO_POINT(@longitude, @latitude)
  ) <= @radius
  RETURN building
```

### Höchste Gebäude
```sql
FOR building IN osm_buildings
  FILTER building.height >= @minHeight
  SORT building.height DESC
  LIMIT @limit
  RETURN building
```

## Performance-Tipps

1. **Batch Requests**: Lieber eine große Query als viele kleine
2. **Caching**: Statische Daten (Gebäude) client-seitig cachen
3. **LOD**: Nur benötigte Details laden (Footprint vs. Full 3D Model)
4. **Async**: Alle Queries asynchron ausführen (nie blocking)

## Troubleshooting

### "Request failed"
- ThemisDB Server läuft? (`http://localhost:8765/health`)
- Firewall blockiert Port 8765?
- CORS aktiviert in ThemisDB Config?

### "No buildings received"
- Bounding Box korrekt? (SouthWest < NorthEast)
- OSM-Daten in ThemisDB importiert?
- AQL Query testen via Postman/cURL

## Nächste Schritte

1. **OSMImporterPlugin**: Automatischer OSM-Import
2. **Streaming**: Progressive Loading für große Gebiete
3. **WebSocket**: Real-time Updates von ThemisDB

---

**Version**: 0.1.0  
**License**: MIT  
**Author**: ThemisDB Team
