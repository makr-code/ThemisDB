# Themis.GISViewer - Konzept mit Unreal Engine 5 Integration

## Übersicht

Themis.GISViewer ist ein hochperformantes GIS-Tool, das auf ThemisDB basiert und OpenStreetMap-Daten inklusive 3D-Modellen mit **Unreal Engine 5** rendert. Das System ermöglicht Echtzeit-Analysen durch modulare C++/Blueprint-Plugins und bietet eine WPF-Steuerkonsole.

## Vision

Ein photorealistisches, GPU-beschleunigtes GIS-System, das:
- **Visualisierung**: OSM-Daten mit fotorealistischen 3D-Modellen in Unreal Engine 5
- **Technologie**: Nanite, Lumen, World Partition für massive Open-World GIS-Daten
- **Analyse**: Echtzeit-Simulationen (Wind, Regen, Wasser, Geräusche, Katastrophen) via Niagara/Chaos
- **Erweiterbarkeit**: C++ Plugin-System + Blueprint-Scripting für Analysemodul
- **Steuerung**: WPF Control Panel für Parametrisierung und Konfiguration
- **Integration**: ThemisDB für Geodaten-Storage und räumliche Queries

## Systemarchitektur

### 1. Architektur-Übersicht

```
┌─────────────────────────────────────────────────────────────────┐
│         Themis.GISViewer.ControlPanel (WPF .NET 8)             │
│  ├─ UI für Parameter-Steuerung                                 │
│  ├─ Plugin-Manager & Konfiguration                             │
│  ├─ ThemisDB Query Interface                                   │
│  └─ Unreal Engine Communication (Named Pipes / UDP)            │
└────────────────────────┬────────────────────────────────────────┘
                         │ IPC (Pipes/Sockets)
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│           Themis GIS Viewer (Unreal Engine 5 Project)          │
├─────────────────────────────────────────────────────────────────┤
│  Unreal Engine 5 Core Systems                                  │
│  ├─ Nanite (Virtualized Geometry - Milliarden Polygone)       │
│  ├─ Lumen (Dynamic Global Illumination)                       │
│  ├─ World Partition (Streaming Open World)                    │
│  ├─ Niagara (VFX - Wind, Regen, Partikel)                     │
│  ├─ Chaos Physics (Destruktion, Simulation)                   │
│  └─ MetaSounds (3D Audio - Geräuschausbreitung)               │
├─────────────────────────────────────────────────────────────────┤
│  Custom C++ Plugins                                            │
│  ├─ ThemisDBPlugin (C++)                                       │
│  │   ├─ HTTP Client für ThemisDB REST API                     │
│  │   ├─ Async Geodaten-Streaming                              │
│  │   └─ Spatial Query Integration                             │
│  ├─ OSMImporterPlugin (C++)                                    │
│  │   ├─ OSM Parser (LibOSM Integration)                       │
│  │   ├─ Procedural Building Generator                         │
│  │   ├─ Road Network Splines                                  │
│  │   └─ Terrain from DEM                                      │
│  ├─ GISAnalysisFramework (C++)                                 │
│  │   ├─ IAnalysisModule Interface                             │
│  │   ├─ Plugin Loader & Registry                              │
│  │   └─ Data Exchange Layer                                   │
│  └─ Analysis Modules (Separate C++ Plugins)                    │
│      ├─ WindSimulationPlugin (Niagara-basiert)                │
│      ├─ WaterFlowPlugin (Chaos Fluids)                        │
│      ├─ SoundPropagationPlugin (MetaSounds)                   │
│      └─ DisasterSimulationPlugin (Chaos Destruction)          │
├─────────────────────────────────────────────────────────────────┤
│  Blueprint Layer                                                │
│  ├─ BP_GISWorldManager                                         │
│  ├─ BP_CameraController                                        │
│  ├─ BP_AnalysisVisualizer                                      │
│  └─ BP_PluginHost                                              │
└─────────────────────────────────────────────────────────────────┘
         ↓                    ↓                    ↓
   ┌──────────┐        ┌──────────┐        ┌──────────┐
   │ ThemisDB │        │ GPU      │        │ OSM API  │
   │ Server   │        │ (UE5     │        │ / Files  │
   │          │        │  Native) │        │          │
   └──────────┘        └──────────┘        └──────────┘
```

### 2. Unreal Engine 5 Integration

#### 2.1 Warum Unreal Engine 5?

**Vorteile für GIS-Anwendungen:**

1. **Nanite**: Virtualisierte Geometrie ermöglicht Milliarden von Polygonen
   - Perfekt für hochdetaillierte 3D-Stadtmodelle
   - Automatisches LOD (kein manuelles Management nötig)
   - Streaming von massiven Geometrie-Daten

2. **Lumen**: Echtzeit Global Illumination
   - Realistische Lichtverhältnisse für verschiedene Tageszeiten
   - Dynamische Schatten für Schatten-Analysen (Gebäude, Solar)
   - Keine Pre-Baking nötig

3. **World Partition**: Open World Streaming
   - Automatisches Laden/Entladen von Weltbereichen
   - Perfekt für große Städte/Regionen (100+ km²)
   - Data Layers für verschiedene Analyse-Szenarien

4. **Niagara**: VFX System
   - Partikel-basierte Wind-Visualisierung
   - Regen, Schnee, Nebel Simulationen
   - GPU-beschleunigte Partikel-Systeme (Millionen)

5. **Chaos Physics**: Physik-Engine
   - Destruktion (Erdbeben, Explosionen)
   - Fluid-Simulation (Hochwasser, Tsunami)
   - Cloth Simulation (Fahnen, Markisen)

6. **MetaSounds**: 3D Audio
   - Realistische Schallausbreitung
   - Lärm-Karten Visualisierung
   - Acoustic Ray-Tracing

#### 2.2 Projekt-Setup

**Unreal Engine Version**: 5.4+ (oder 5.5 wenn verfügbar)

**Project Settings**:
```ini
[Project]
ProjectName=ThemisGISViewer
EngineVersion=5.4

[Plugins]
EnabledPlugins=
  - ThemisDBPlugin
  - OSMImporterPlugin
  - GISAnalysisFramework
  - GeoreferencingPlugin (Unreal Standard)
  - LandscapePlugin
  - WaterPlugin
  - NiagaraPlugin
  - ChaosPlugin
  - MetaSoundsPlugin

[WorldPartition]
EnableWorldPartition=True
WorldOrigin=GeographicCoordinates
EPSG=4326  # WGS84

[Nanite]
EnableNanite=True
MaxTriangles=10000000  # 10M Tris pro Mesh

[Lumen]
EnableLumen=True
DynamicGI=True
```

### 3. ThemisDB Integration (C++ Plugin)

#### 3.1 ThemisDBPlugin Architektur

**C++ Klassen**:

```cpp
// ThemisDBPlugin/Source/ThemisDBPlugin/Public/ThemisDBClient.h
#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "ThemisDBClient.generated.h"

USTRUCT(BlueprintType)
struct FGeoLocation
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    double Latitude;
    
    UPROPERTY(BlueprintReadWrite)
    double Longitude;
    
    UPROPERTY(BlueprintReadWrite)
    double Altitude;
};

USTRUCT(BlueprintType)
struct FOSMBuilding
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    FString ID;
    
    UPROPERTY(BlueprintReadWrite)
    TArray<FGeoLocation> Footprint;
    
    UPROPERTY(BlueprintReadWrite)
    float Height;
    
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> Tags;
};

UCLASS(BlueprintType)
class THEMISDBPLUGIN_API UThemisDBClient : public UObject
{
    GENERATED_BODY()
    
public:
    // Initialize connection
    UFUNCTION(BlueprintCallable, Category = "ThemisDB")
    void Initialize(const FString& ServerURL, int32 Port);
    
    // Async query for buildings in bounding box
    UFUNCTION(BlueprintCallable, Category = "ThemisDB|Geo")
    void QueryBuildingsAsync(
        FGeoLocation SouthWest,
        FGeoLocation NorthEast,
        FOnBuildingsReceived OnComplete
    );
    
    // Async query for terrain data
    UFUNCTION(BlueprintCallable, Category = "ThemisDB|Geo")
    void QueryTerrainAsync(
        FGeoLocation Center,
        float RadiusKm,
        FOnTerrainReceived OnComplete
    );
    
    // Vector search for similar buildings
    UFUNCTION(BlueprintCallable, Category = "ThemisDB|Vector")
    void FindSimilarBuildingsAsync(
        const TArray<float>& EmbeddingVector,
        int32 K,
        FOnBuildingsReceived OnComplete
    );
    
    // AQL Query execution
    UFUNCTION(BlueprintCallable, Category = "ThemisDB|Query")
    void ExecuteAQLAsync(
        const FString& AQLQuery,
        FOnQueryComplete OnComplete
    );
    
private:
    FHttpModule* HttpModule;
    FString BaseURL;
    
    void HandleBuildingsResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bWasSuccessful,
        FOnBuildingsReceived Callback
    );
};
```

#### 3.2 Beispiel AQL Query für Gebäude

```cpp
// In Blueprint oder C++
FString AQLQuery = TEXT(R"(
    FOR building IN osm_buildings
      FILTER GEO_DISTANCE(
        building.location,
        GEO_POINT(@longitude, @latitude)
      ) <= @radius
      AND building.height >= @minHeight
      SORT building.height DESC
      LIMIT @limit
      RETURN {
        id: building._key,
        footprint: building.footprint,
        height: building.height,
        type: building.tags.building,
        material: building.tags.material
      }
)");

TMap<FString, FString> Parameters;
Parameters.Add("latitude", "52.520008");
Parameters.Add("longitude", "13.404954");
Parameters.Add("radius", "1000");  // 1km
Parameters.Add("minHeight", "20");
Parameters.Add("limit", "500");

ThemisDBClient->ExecuteAQLAsync(AQLQuery, [this](const TArray<FOSMBuilding>& Buildings) {
    // Spawn buildings in Unreal
    for (const auto& Building : Buildings) {
        SpawnProceduralBuilding(Building);
    }
});
```

### 4. OSM Importer Plugin

#### 4.1 Procedural Building Generation

```cpp
// OSMImporterPlugin/Source/OSMImporterPlugin/Public/ProceduralBuildingGenerator.h

UCLASS()
class AProceduralBuilding : public AActor
{
    GENERATED_BODY()
    
public:
    // Building data from ThemisDB
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FOSMBuilding BuildingData;
    
    // Procedural Mesh Component (or Static Mesh for Nanite)
    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent* ProceduralMesh;
    
    // Generate building geometry from footprint + height
    UFUNCTION(BlueprintCallable)
    void GenerateBuilding();
    
private:
    // Extrude footprint polygon to 3D mesh
    void ExtrudeFootprint(
        const TArray<FVector>& FootprintVertices,
        float Height,
        TArray<FVector>& OutVertices,
        TArray<int32>& OutTriangles,
        TArray<FVector>& OutNormals,
        TArray<FVector2D>& OutUVs
    );
    
    // Apply facade material based on building tags
    UMaterialInterface* SelectFacadeMaterial(const FString& BuildingType);
    
    // Add roof geometry
    void GenerateRoof(
        const TArray<FVector>& FootprintVertices,
        float Height,
        const FString& RoofType  // flat, gabled, hipped
    );
    
    // Convert to Nanite Static Mesh for performance
    void ConvertToNanite();
};
```

#### 4.2 Road Network Generation

```cpp
// Spline-based roads from OSM Ways
UCLASS()
class ARoadNetwork : public AActor
{
    GENERATED_BODY()
    
public:
    UPROPERTY(VisibleAnywhere)
    USplineComponent* RoadSpline;
    
    UPROPERTY(VisibleAnywhere)
    USplineMeshComponent* RoadMesh;
    
    // Generate road from OSM Way
    void GenerateRoadFromWay(const FOSMWay& Way);
    
    // Road types: motorway, trunk, primary, secondary, residential
    UPROPERTY(EditAnywhere)
    ERoadType RoadType;
    
    // Road width based on type
    float GetRoadWidth() const;
    
    // Apply road markings, traffic lights
    void ApplyRoadDetails();
};
```

### 5. GIS Analysis Framework (Plugin-System)

#### 5.1 IAnalysisModule Interface (C++)

```cpp
// GISAnalysisFramework/Source/GISAnalysisFramework/Public/IAnalysisModule.h

class GISANALYSISFRAMEWORK_API IAnalysisModule
{
public:
    virtual ~IAnalysisModule() = default;
    
    // Module metadata
    virtual FString GetModuleName() const = 0;
    virtual FString GetModuleVersion() const = 0;
    virtual FString GetDescription() const = 0;
    
    // Lifecycle
    virtual void Initialize(UWorld* World, UThemisDBClient* DBClient) = 0;
    virtual void Tick(float DeltaTime) = 0;
    virtual void Shutdown() = 0;
    
    // Parameter configuration (from WPF Control Panel)
    virtual void SetParameter(const FString& Name, const FString& Value) = 0;
    virtual TMap<FString, FString> GetParameters() const = 0;
    
    // Visualization output
    virtual void UpdateVisualization() = 0;
    
    // Data export for WPF Control Panel
    virtual FString ExportDataAsJSON() const = 0;
};

// Module Registry
UCLASS()
class UAnalysisModuleRegistry : public UObject
{
    GENERATED_BODY()
    
public:
    // Register module
    void RegisterModule(TSharedPtr<IAnalysisModule> Module);
    
    // Load module from DLL
    bool LoadModuleFromFile(const FString& ModulePath);
    
    // Get all loaded modules
    TArray<TSharedPtr<IAnalysisModule>> GetAllModules() const;
    
    // Find module by name
    TSharedPtr<IAnalysisModule> FindModule(const FString& Name) const;
    
private:
    TMap<FString, TSharedPtr<IAnalysisModule>> LoadedModules;
};
```

### 6. Analysis Modules (Beispiele)

#### 6.1 Wind Simulation Module (Niagara-basiert)

```cpp
// WindSimulationPlugin/Source/WindSimulationPlugin/Private/WindSimulationModule.h

class FWindSimulationModule : public IAnalysisModule
{
public:
    virtual void Initialize(UWorld* World, UThemisDBClient* DBClient) override
    {
        // Create Niagara System for wind particles
        WindNiagaraSystem = LoadObject<UNiagaraSystem>(
            nullptr,
            TEXT("/Game/VFX/NS_WindParticles")
        );
        
        WindNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            WindNiagaraSystem,
            FVector::ZeroVector
        );
        
        // Initialize wind grid (3D grid for CFD simulation)
        WindGrid = MakeShared<FWindGrid>(GridSize, CellSize);
        
        // Query buildings for wind obstacles
        DBClient->QueryBuildingsAsync(
            WorldBounds,
            FOnBuildingsReceived::CreateLambda([this](const TArray<FOSMBuilding>& Buildings) {
                for (const auto& Building : Buildings) {
                    WindGrid->AddObstacle(Building);
                }
            })
        );
    }
    
    virtual void Tick(float DeltaTime) override
    {
        // Update CFD simulation (can be GPU-accelerated via Compute Shader)
        WindGrid->Simulate(DeltaTime, WindSpeed, WindDirection);
        
        // Update Niagara particle velocities from wind grid
        UpdateNiagaraFromWindGrid();
    }
    
    virtual void UpdateVisualization() override
    {
        // Option 1: Particle flow lines
        if (VisualizationMode == EWindVisualizationMode::Particles) {
            // Already handled by Niagara
        }
        
        // Option 2: Vector field (arrows)
        else if (VisualizationMode == EWindVisualizationMode::VectorField) {
            DrawVectorField();
        }
        
        // Option 3: Heatmap overlay (speed)
        else if (VisualizationMode == EWindVisualizationMode::Heatmap) {
            UpdateHeatmapTexture();
        }
    }
    
private:
    UNiagaraComponent* WindNiagaraComponent;
    UNiagaraSystem* WindNiagaraSystem;
    TSharedPtr<FWindGrid> WindGrid;
    
    // Parameters
    float WindSpeed = 10.0f;  // m/s
    FVector WindDirection = FVector(1, 0, 0);
    EWindVisualizationMode VisualizationMode;
};
```

#### 6.2 Water Flow Module (Chaos Fluids)

```cpp
// WaterFlowPlugin/Source/WaterFlowPlugin/Private/WaterFlowModule.h

class FWaterFlowModule : public IAnalysisModule
{
public:
    virtual void Initialize(UWorld* World, UThemisDBClient* DBClient) override
    {
        // Query terrain heightmap
        DBClient->QueryTerrainAsync(
            WorldCenter,
            RadiusKm,
            FOnTerrainReceived::CreateLambda([this](const FTerrainData& Terrain) {
                CreateWaterSimulation(Terrain);
            })
        );
    }
    
    virtual void Tick(float DeltaTime) override
    {
        // Shallow Water Equations (2D Grid)
        for (int32 i = 0; i < WaterGrid.Width; ++i) {
            for (int32 j = 0; j < WaterGrid.Height; ++j) {
                // Compute water flow based on height differences
                FVector2D Flow = ComputeFlow(i, j);
                WaterGrid.SetVelocity(i, j, Flow);
                
                // Update water height
                float NewHeight = WaterGrid.GetHeight(i, j) + Flow.Size() * DeltaTime;
                WaterGrid.SetHeight(i, j, NewHeight);
            }
        }
        
        // Update Unreal Water System
        UpdateWaterBody();
    }
    
private:
    // Use Unreal's Water Plugin
    AWaterBody* WaterBody;
    FWaterSimulationGrid WaterGrid;
    
    // Rainfall input
    float RainfallIntensity = 0.0f;  // mm/h
    
    // Visualize flooded areas
    void UpdateFloodedAreas();
};
```

#### 6.3 Sound Propagation Module (MetaSounds)

```cpp
// SoundPropagationPlugin using MetaSounds

class FSoundPropagationModule : public IAnalysisModule
{
public:
    virtual void Initialize(UWorld* World, UThemisDBClient* DBClient) override
    {
        // Load sound sources from ThemisDB
        // (e.g., roads with traffic volume, industrial areas)
        DBClient->ExecuteAQLAsync(
            TEXT("FOR road IN osm_roads FILTER road.traffic_volume > 1000 RETURN road"),
            FOnQueryComplete::CreateLambda([this](const TArray<FJsonObject>& Results) {
                for (const auto& Road : Results) {
                    CreateSoundSource(Road);
                }
            })
        );
    }
    
    virtual void Tick(float DeltaTime) override
    {
        // Acoustic raytracing (can use Unreal's built-in Audio system)
        for (auto& Source : SoundSources) {
            // Trace rays from source
            TArray<FVector> ReceiverPoints = GetReceiverGrid();
            for (const FVector& Receiver : ReceiverPoints) {
                float dB = ComputeSoundLevel(Source, Receiver);
                NoiseMap.SetValue(Receiver, dB);
            }
        }
        
        UpdateNoiseMapVisualization();
    }
    
private:
    TArray<USoundSourceComponent*> SoundSources;
    FNoiseMap NoiseMap;
    
    // Compute sound attenuation through buildings
    float ComputeSoundLevel(const FSoundSource& Source, const FVector& Receiver);
    
    // Visualize as color-coded overlay
    void UpdateNoiseMapVisualization();
};
```

#### 6.4 Disaster Simulation Module (Chaos Destruction)

```cpp
// DisasterSimulationPlugin using Chaos Physics

class FDisasterSimulationModule : public IAnalysisModule
{
public:
    // Supported disaster types
    enum class EDisasterType {
        Earthquake,
        Flood,
        Fire,
        Explosion
    };
    
    virtual void Initialize(UWorld* World, UThemisDBClient* DBClient) override
    {
        // Query vulnerable buildings
        DBClient->QueryBuildingsAsync(
            WorldBounds,
            FOnBuildingsReceived::CreateLambda([this](const TArray<FOSMBuilding>& Buildings) {
                for (const auto& Building : Buildings) {
                    // Assign vulnerability based on building type, age, material
                    float Vulnerability = AssessVulnerability(Building);
                    VulnerabilityMap.Add(Building.ID, Vulnerability);
                    
                    // Convert to Chaos Destructible Mesh
                    CreateDestructibleBuilding(Building);
                }
            })
        );
    }
    
    virtual void Tick(float DeltaTime) override
    {
        if (DisasterActive) {
            switch (DisasterType) {
                case EDisasterType::Earthquake:
                    SimulateEarthquake(DeltaTime);
                    break;
                case EDisasterType::Flood:
                    SimulateFlood(DeltaTime);
                    break;
                case EDisasterType::Fire:
                    SimulateFire(DeltaTime);
                    break;
            }
        }
    }
    
private:
    void SimulateEarthquake(float DeltaTime)
    {
        // Apply ground motion to buildings
        for (auto& Building : DestructibleBuildings) {
            FVector GroundAcceleration = GetGroundMotion(Building->GetActorLocation());
            Building->AddForce(GroundAcceleration * Building->GetMass());
            
            // Check if building collapses
            if (Building->GetDamage() > VulnerabilityMap[Building->ID]) {
                Building->TriggerDestruction();
            }
        }
    }
    
    TArray<AGeometryCollectionActor*> DestructibleBuildings;
    TMap<FString, float> VulnerabilityMap;
    EDisasterType DisasterType;
    bool DisasterActive = false;
};
```

### 7. WPF Control Panel

#### 7.1 Architektur

```
Themis.GISViewer.ControlPanel/
├─ Themis.GISViewer.ControlPanel.csproj
├─ App.xaml
├─ MainWindow.xaml
├─ ViewModels/
│  ├─ MainViewModel.cs
│  ├─ PluginManagerViewModel.cs
│  ├─ WindSimulationViewModel.cs
│  ├─ WaterFlowViewModel.cs
│  └─ DisasterSimulationViewModel.cs
├─ Views/
│  ├─ PluginManagerView.xaml
│  ├─ WindControlView.xaml
│  ├─ WaterControlView.xaml
│  └─ DisasterControlView.xaml
├─ Services/
│  ├─ UnrealEngineConnector.cs  (IPC via Named Pipes)
│  ├─ ThemisDBService.cs
│  └─ PluginService.cs
└─ Models/
   ├─ PluginInfo.cs
   ├─ SimulationParameters.cs
   └─ AnalysisResult.cs
```

#### 7.2 IPC Communication (WPF ↔ Unreal)

```csharp
// UnrealEngineConnector.cs
public class UnrealEngineConnector
{
    private NamedPipeClientStream _pipeClient;
    private const string PipeName = "ThemisGISViewer_IPC";
    
    public async Task ConnectAsync()
    {
        _pipeClient = new NamedPipeClientStream(".", PipeName, PipeDirection.InOut);
        await _pipeClient.ConnectAsync();
    }
    
    // Send command to Unreal
    public async Task SendCommandAsync(string command, Dictionary<string, object> parameters)
    {
        var message = new {
            Command = command,
            Parameters = parameters,
            Timestamp = DateTime.UtcNow
        };
        
        var json = JsonSerializer.Serialize(message);
        var bytes = Encoding.UTF8.GetBytes(json);
        
        await _pipeClient.WriteAsync(bytes, 0, bytes.Length);
    }
    
    // Receive data from Unreal
    public async Task<string> ReceiveDataAsync()
    {
        var buffer = new byte[4096];
        var bytesRead = await _pipeClient.ReadAsync(buffer, 0, buffer.Length);
        return Encoding.UTF8.GetString(buffer, 0, bytesRead);
    }
}

// Example usage in ViewModel
public class WindSimulationViewModel : ViewModelBase
{
    private readonly UnrealEngineConnector _unrealConnector;
    
    public async Task SetWindSpeedAsync(double speed)
    {
        await _unrealConnector.SendCommandAsync("SetWindSpeed", new Dictionary<string, object> {
            { "ModuleName", "WindSimulation" },
            { "Speed", speed }
        });
    }
    
    public async Task SetWindDirectionAsync(double azimuth)
    {
        await _unrealConnector.SendCommandAsync("SetWindDirection", new Dictionary<string, object> {
            { "ModuleName", "WindSimulation" },
            { "Azimuth", azimuth }
        });
    }
}
```

#### 7.3 Beispiel UI (XAML)

```xml
<!-- WindControlView.xaml -->
<UserControl x:Class="Themis.GISViewer.ControlPanel.Views.WindControlView">
    <Grid>
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
        </Grid.RowDefinitions>
        
        <!-- Wind Speed -->
        <StackPanel Grid.Row="0" Margin="10">
            <TextBlock Text="Wind-Geschwindigkeit (m/s)" FontWeight="Bold"/>
            <Slider Value="{Binding WindSpeed, Mode=TwoWay}" 
                    Minimum="0" Maximum="50" 
                    TickFrequency="5" IsSnapToTickEnabled="True"/>
            <TextBlock Text="{Binding WindSpeed, StringFormat='{}{0:F1} m/s'}"/>
        </StackPanel>
        
        <!-- Wind Direction -->
        <StackPanel Grid.Row="1" Margin="10">
            <TextBlock Text="Wind-Richtung (Azimut °)" FontWeight="Bold"/>
            <Slider Value="{Binding WindDirection, Mode=TwoWay}" 
                    Minimum="0" Maximum="360" 
                    TickFrequency="45" IsSnapToTickEnabled="True"/>
            <TextBlock Text="{Binding WindDirection, StringFormat='{}{0:F0}°'}"/>
        </StackPanel>
        
        <!-- Visualization Mode -->
        <StackPanel Grid.Row="2" Margin="10">
            <TextBlock Text="Visualisierung" FontWeight="Bold"/>
            <ComboBox SelectedItem="{Binding VisualizationMode}">
                <ComboBoxItem Content="Partikel"/>
                <ComboBoxItem Content="Vektorfeld"/>
                <ComboBoxItem Content="Heatmap"/>
            </ComboBox>
        </StackPanel>
        
        <!-- Live Data -->
        <GroupBox Grid.Row="3" Header="Echtzeit-Daten" Margin="10">
            <StackPanel>
                <TextBlock Text="{Binding MaxWindSpeed, StringFormat='Max: {0:F1} m/s'}"/>
                <TextBlock Text="{Binding AverageWindSpeed, StringFormat='Durchschnitt: {0:F1} m/s'}"/>
                <TextBlock Text="{Binding ParticleCount, StringFormat='Partikel: {0:N0}'}"/>
            </StackPanel>
        </GroupBox>
    </Grid>
</UserControl>
```

### 8. Projekt-Struktur

```
ThemisGISViewer/
├─ ThemisGISViewer.uproject  (Unreal Engine 5 Project)
├─ Config/
│  ├─ DefaultEngine.ini
│  ├─ DefaultGame.ini
│  └─ ThemisDB.ini
├─ Content/
│  ├─ Maps/
│  │  ├─ MainWorld.umap  (World Partition enabled)
│  │  └─ TestCity.umap
│  ├─ Materials/
│  │  ├─ M_Building_Concrete.uasset
│  │  ├─ M_Building_Glass.uasset
│  │  ├─ M_Road_Asphalt.uasset
│  │  └─ M_Terrain.uasset
│  ├─ Blueprints/
│  │  ├─ BP_GISWorldManager.uasset
│  │  ├─ BP_CameraController.uasset
│  │  └─ BP_AnalysisVisualizer.uasset
│  ├─ VFX/
│  │  ├─ NS_WindParticles.uasset  (Niagara)
│  │  ├─ NS_Rain.uasset
│  │  └─ NS_Fire.uasset
│  └─ Audio/
│     ├─ MS_TrafficNoise.uasset  (MetaSound)
│     └─ MS_AmbientCity.uasset
├─ Plugins/
│  ├─ ThemisDBPlugin/
│  │  ├─ ThemisDBPlugin.uplugin
│  │  ├─ Source/
│  │  │  ├─ ThemisDBPlugin/
│  │  │  │  ├─ Public/
│  │  │  │  │  ├─ ThemisDBClient.h
│  │  │  │  │  └─ ThemisDBTypes.h
│  │  │  │  └─ Private/
│  │  │  │     └─ ThemisDBClient.cpp
│  │  └─ Resources/
│  ├─ OSMImporterPlugin/
│  │  ├─ OSMImporterPlugin.uplugin
│  │  ├─ Source/
│  │  │  ├─ OSMImporterPlugin/
│  │  │  │  ├─ Public/
│  │  │  │  │  ├─ ProceduralBuildingGenerator.h
│  │  │  │  │  ├─ RoadNetworkGenerator.h
│  │  │  │  │  └─ TerrainGenerator.h
│  │  │  │  └─ Private/
│  │  │  │     ├─ ProceduralBuildingGenerator.cpp
│  │  │  │     ├─ RoadNetworkGenerator.cpp
│  │  │  │     └─ TerrainGenerator.cpp
│  │  └─ ThirdParty/
│  │     └─ LibOSM/  (OSM parsing library)
│  ├─ GISAnalysisFramework/
│  │  ├─ GISAnalysisFramework.uplugin
│  │  ├─ Source/
│  │  │  ├─ GISAnalysisFramework/
│  │  │  │  ├─ Public/
│  │  │  │  │  ├─ IAnalysisModule.h
│  │  │  │  │  └─ AnalysisModuleRegistry.h
│  │  │  │  └─ Private/
│  │  │  │     └─ AnalysisModuleRegistry.cpp
│  ├─ WindSimulationPlugin/
│  ├─ WaterFlowPlugin/
│  ├─ SoundPropagationPlugin/
│  └─ DisasterSimulationPlugin/
├─ Source/
│  └─ ThemisGISViewer/
│     ├─ ThemisGISViewer.Build.cs
│     ├─ ThemisGISViewer.h
│     └─ ThemisGISViewer.cpp
└─ ControlPanel/  (WPF Project)
   └─ Themis.GISViewer.ControlPanel/
      ├─ Themis.GISViewer.ControlPanel.csproj
      ├─ App.xaml
      ├─ MainWindow.xaml
      └─ ...
```

### 9. Entwicklungs-Roadmap

#### Phase 1: Setup & Grundlagen (Wochen 1-3)
- [ ] Unreal Engine 5 Projekt erstellen
- [ ] ThemisDBPlugin Grundgerüst (C++)
- [ ] WPF Control Panel Projekt
- [ ] IPC-Kommunikation (Named Pipes) testen
- [ ] Einfaches Terrain rendern (Lumen + Nanite)

#### Phase 2: OSM Integration (Wochen 4-7)
- [ ] OSMImporterPlugin entwickeln
- [ ] Gebäude-Generierung (Procedural Meshes)
- [ ] Straßen-Netzwerk (Splines)
- [ ] ThemisDB Integration (Geodaten laden)
- [ ] World Partition Setup für große Städte

#### Phase 3: Plugin-Framework (Wochen 8-11)
- [ ] GISAnalysisFramework Plugin
- [ ] IAnalysisModule Interface
- [ ] Plugin Loader (DLL Loading)
- [ ] WPF ↔ Unreal Parameter-Synchronisation

#### Phase 4: Analysis Modules (Wochen 12-16)
- [ ] WindSimulationPlugin (Niagara)
- [ ] WaterFlowPlugin (Chaos Fluids)
- [ ] SoundPropagationPlugin (MetaSounds)
- [ ] DisasterSimulationPlugin (Chaos Destruction)

#### Phase 5: Polish & Optimization (Wochen 17-20)
- [ ] Performance-Optimierung (Nanite, World Partition)
- [ ] UI/UX Verbesserungen (WPF)
- [ ] Dokumentation & Tutorials
- [ ] Demo-Video (Berlin Stadtgebiet)
- [ ] Release v1.0

### 10. Technologie-Stack

#### Unreal Engine 5
- **Version**: 5.4+ (oder 5.5)
- **Rendering**: Nanite, Lumen, TSR (Temporal Super Resolution)
- **Physics**: Chaos Physics, Chaos Fluids
- **VFX**: Niagara
- **Audio**: MetaSounds
- **Scripting**: C++ (Plugins), Blueprints (Gameplay)

#### WPF Control Panel
- **.NET 8.0** (net8.0-windows)
- **UI Framework**: WPF
- **MVVM**: CommunityToolkit.Mvvm
- **DI**: Microsoft.Extensions.DependencyInjection
- **IPC**: Named Pipes / UDP Sockets

#### ThemisDB
- **Themis.AdminTools.Shared** (existing)
- **HTTP Client**: System.Net.Http
- **JSON**: System.Text.Json

#### 3rd-Party Libraries
- **LibOSM** (C++): OSM Parsing
- **GDAL** (optional): Geo-Koordinaten-Transformation
- **vcpkg**: C++ Dependency Management (für UE5 Plugins)

### 11. Performance-Überlegungen

#### Unreal Engine 5 Vorteile
- **Nanite**: Automatisches LOD → keine manuellen LOD-Stufen nötig
- **World Partition**: Streaming → Städte mit 100+ km² möglich
- **Lumen**: Echtzeit GI → keine Lightmap-Baking-Wartezeit
- **TSR**: Upscaling → 1080p → 4K mit minimaler Performance-Einbuße

#### Optimierungen
- **Instancing**: Vegetation, Straßenmöbel (100.000+ Instanzen)
- **HLOD**: Hierarchical LOD für ferne Stadtbereiche
- **Async Loading**: Tiles im Hintergrund laden
- **GPU Culling**: Frustum + Occlusion Culling auf GPU
- **Texture Streaming**: Nur sichtbare Texturen in VRAM

### 12. Konfiguration

#### appsettings.json (WPF Control Panel)
```json
{
  "ThemisDB": {
    "ApiUrl": "http://localhost:8765",
    "Timeout": 30
  },
  "UnrealEngine": {
    "IPCMethod": "NamedPipes",
    "PipeName": "ThemisGISViewer_IPC",
    "ExecutablePath": "C:/ThemisGISViewer/Binaries/Win64/ThemisGISViewer.exe"
  },
  "Map": {
    "InitialLocation": {
      "Latitude": 52.520008,
      "Longitude": 13.404954
    }
  },
  "Plugins": {
    "AutoLoad": ["WindSimulation", "WaterFlow", "SoundPropagation"]
  }
}
```

#### DefaultEngine.ini (Unreal)
```ini
[/Script/Engine.RendererSettings]
r.Nanite=True
r.Lumen.DiffuseIndirect=True
r.Lumen.Reflections=True
r.Shadow.Virtual.Enable=True

[/Script/Engine.WorldPartitionSettings]
bEnableWorldPartition=True
WorldPartitionGridSize=25600  # 256m cells

[ThemisDB]
ServerURL=http://localhost:8765
ConnectionTimeout=30

[Plugins.GISAnalysisFramework]
PluginDirectory=Plugins/AnalysisModules
```

### 13. Weiterführende Features

- **VR Support**: Immersive Stadt-Begehungen mit Meta Quest / Valve Index
- **Multi-User**: Kollaborative Analysen (mehrere Nutzer gleichzeitig)
- **Pixel Streaming**: Web-basierter Zugriff (Browser statt lokale Installation)
- **Zeitreisen**: Historische OSM-Daten visualisieren (Berlin 2000 vs. 2024)
- **KI-Integration**: Predictive Analytics (z.B. Traffic-Flow mit ML)
- **Cloud Rendering**: AWS/Azure für Rendering-Farm (massive Simulationen)

## Zusammenfassung

Durch die Integration von **Unreal Engine 5** profitiert Themis.GISViewer von:

✅ **Photorealistische Grafik** (Nanite + Lumen)  
✅ **Massive Skalierung** (World Partition für 100+ km²)  
✅ **Keine Low-Level-Rendering-Entwicklung** (UE5 übernimmt alles)  
✅ **Bewährte VFX/Physics-Systeme** (Niagara, Chaos)  
✅ **Aktive Community & Marketplace** (viele Assets verfügbar)  
✅ **C++ Plugin-System** für ThemisDB-Integration  
✅ **WPF Control Panel** für Nutzerfreundlichkeit  

Die Kombination aus **ThemisDB** (Geodaten-Backend) + **Unreal Engine 5** (Rendering/Simulation) + **WPF** (Steuerung) schafft ein professionelles, erweiterbares GIS-Tool für Echtzeit-Analysen.
