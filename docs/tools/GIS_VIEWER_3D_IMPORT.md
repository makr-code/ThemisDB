# 3D-Datenimport & Geo-Verortung - Erweitertes Konzept

## Übersicht

Themis.GISViewer unterstützt den Import und die Geo-Verortung von 3D-Daten aus verschiedenen Quellen:
- **Fabrikplanung**: CAD-Daten (AutoCAD, Revit, STEP, IFC)
- **Drohnenvermessung**: Photogrammetrie, LiDAR-Punktwolken
- **Geodaten-Austausch**: GeoJSON, CityGML, KML, Shapefile
- **3D-Modelle**: glTF, FBX, OBJ, COLLADA
- **BIM-Daten**: IFC (Industry Foundation Classes)

## 1. Unterstützte Datenformate

### 1.1 Fabrikplanung & CAD

| Format | Beschreibung | Verwendung | Plugin |
|--------|--------------|------------|--------|
| **IFC** | Industry Foundation Classes | BIM-Gebäudemodelle (Revit, ArchiCAD) | IFCImporterPlugin |
| **STEP** | ISO 10303 CAD Data Exchange | Maschinenbau, Fabrikplanung | STEPImporterPlugin |
| **DWG/DXF** | AutoCAD Zeichnungen | 2D/3D Grundrisse, Layouts | DWGImporterPlugin |
| **Revit** | Autodesk Revit Dateien | BIM-Modelle (via IFC Export) | IFCImporterPlugin |
| **SketchUp** | .skp Dateien | 3D-Architekturmodelle | SketchUpImporterPlugin |

### 1.2 Drohnenvermessung & Scan-Daten

| Format | Beschreibung | Verwendung | Plugin |
|--------|--------------|------------|--------|
| **LAS/LAZ** | LiDAR Punktwolken | Laser-Scanning, Drohnen-LiDAR | PointCloudPlugin |
| **E57** | ASTM E2807 Point Cloud | Multi-Vendor Punktwolken | PointCloudPlugin |
| **PLY** | Polygon File Format | Photogrammetrie-Meshes | MeshImporterPlugin |
| **OBJ/MTL** | Wavefront OBJ | 3D-Modelle aus Photogrammetrie | MeshImporterPlugin |
| **COLLADA** | .dae Files | 3D-Modelle mit Geo-Referenz | MeshImporterPlugin |

### 1.3 Geodaten-Austauschformate

| Format | Beschreibung | Verwendung | Plugin |
|--------|--------------|------------|--------|
| **GeoJSON** | JSON mit Geo-Koordinaten | Vektordaten, Gebäude-Footprints | GeoJSONImporterPlugin |
| **CityGML** | OGC City Model Standard | 3D-Stadtmodelle (LOD0-4) | CityGMLImporterPlugin |
| **KML/KMZ** | Google Earth Format | Geo-verortete 3D-Modelle | KMLImporterPlugin |
| **Shapefile** | ESRI Shapefile | 2D Vektordaten | ShapefileImporterPlugin |
| **GeoTIFF** | Geo-referenzierte Raster | Orthophotos, DEM | TerrainPlugin |
| **3D Tiles** | Cesium 3D Tiles | Massive 3D-Datensets | 3DTilesPlugin |

### 1.4 Standard 3D-Formate

| Format | Beschreibung | Verwendung | Plugin |
|--------|--------------|------------|--------|
| **glTF 2.0** | GL Transmission Format | PBR 3D-Modelle, optimiert | glTFImporterPlugin |
| **FBX** | Autodesk Filmbox | Animation, Rigging | FBXImporterPlugin |
| **USD** | Universal Scene Description | Film/VFX Pipelines | USDImporterPlugin |

## 2. Import-Pipeline Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                  Themis GIS Viewer Import System                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Input Sources                                          │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │  • CAD Files (IFC, STEP, DWG)                          │   │
│  │  • Drone Data (LAS, E57, Mesh)                         │   │
│  │  • GeoJSON / CityGML / KML                             │   │
│  │  • 3D Models (glTF, FBX, OBJ)                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Format Detection & Parser Selection                    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Format-Specific Importers (Plugins)                    │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │  • IFCImporter → BIM Hierarchy                         │   │
│  │  • PointCloudImporter → Downsampling, LOD              │   │
│  │  • GeoJSONImporter → Feature Parsing                   │   │
│  │  • CityGMLImporter → LOD0-4 Extraction                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Geo-Referencing Layer                                  │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │  • Coordinate Transformation (EPSG:4326, UTM, etc.)    │   │
│  │  • World Origin Alignment                              │   │
│  │  • Height Offset (Ellipsoid → Geoid)                   │   │
│  │  • Auto-Detection (Metadata, EXIF, XML)                │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Data Optimization                                       │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │  • Mesh Simplification (Decimation)                     │   │
│  │  • LOD Generation (Auto)                                │   │
│  │  • Nanite Conversion                                    │   │
│  │  • Texture Compression (BC7, ASTC)                      │   │
│  │  • Point Cloud Decimation                               │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  ThemisDB Storage                                        │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │  • Metadata (Tags, Attributes)                          │   │
│  │  • Geometry (Mesh Blobs, Compressed)                    │   │
│  │  • Spatial Index (Geo-Bounds)                           │   │
│  │  • Relationships (Building → Floors → Rooms)            │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Unreal Engine 5 Visualization                          │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │  • World Partition Streaming                            │   │
│  │  • Nanite Rendering                                     │   │
│  │  • Lumen Lighting                                       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 3. Geo-Referenzierung & Koordinatensysteme

### 3.1 Unterstützte Koordinatensysteme

- **WGS84 (EPSG:4326)**: Lat/Lon (Standard für GPS, GeoJSON)
- **Web Mercator (EPSG:3857)**: Google Maps, OSM
- **UTM Zones**: Universal Transverse Mercator (32N, 33N für Deutschland)
- **Gauss-Krüger**: DHDN (Deutschland)
- **ETRS89**: Europäisches Referenzsystem
- **Custom CRS**: Lokale Koordinatensysteme (Fabrik-intern)

### 3.2 Auto-Detection

```cpp
// Automatische Erkennung des Koordinatensystems
class FGeoReferenceDetector
{
public:
    // Aus Metadaten (GeoJSON, CityGML, IFC)
    static FCRS DetectFromMetadata(const FString& FilePath);
    
    // Aus Koordinaten-Range (Heuristik)
    static FCRS DetectFromBounds(FGeoBounds Bounds);
    
    // Aus EXIF-Tags (Drohnenfotos)
    static FCRS DetectFromEXIF(const FString& ImagePath);
};
```

### 3.3 Transformation Pipeline

```cpp
// Koordinaten-Transformation
class FGeoTransform
{
public:
    // EPSG:4326 (WGS84) → Unreal World Space
    static FVector TransformToWorld(
        FGeoLocation GeoPos,
        FGeoLocation WorldOrigin,
        FCRS SourceCRS = FCRS::WGS84
    );
    
    // UTM → WGS84 → Unreal
    static FVector TransformUTMToWorld(
        double Easting,
        double Northing,
        int32 UTMZone,
        FGeoLocation WorldOrigin
    );
    
    // Lokales Fabrik-Koordinatensystem → Geo → Unreal
    static FVector TransformLocalToWorld(
        FVector LocalPos,
        FGeoLocation AnchorPoint,  // Bekannter Geo-Punkt
        FRotator LocalRotation     // Nordausrichtung
    );
};
```

## 4. Format-Spezifische Importer

### 4.1 IFC Importer (BIM-Gebäude)

**Use Case**: Revit-Modelle, ArchiCAD-Gebäude, Fabriklayouts

```cpp
// IFCImporterPlugin/Source/IFCImporterPlugin/Public/IFCImporter.h

class FIFCImporter
{
public:
    struct FIFCImportSettings
    {
        // Geo-Referenzierung
        FGeoLocation SiteLocation;     // IfcSite.RefLatitude/RefLongitude
        bool bAutoDetectLocation = true;
        
        // LOD
        bool bImportGeometry = true;
        bool bImportStructure = true;   // Walls, Slabs, Beams
        bool bImportMEP = false;        // HVAC, Pipes (optional)
        
        // Optimization
        bool bGenerateLODs = true;
        bool bConvertToNanite = true;
        float MeshSimplificationRatio = 0.5f;
    };
    
    // Import IFC File
    static TArray<AActor*> ImportIFCFile(
        const FString& FilePath,
        const FIFCImportSettings& Settings
    );
    
private:
    // Parse IFC using Open CASCADE or IfcOpenShell
    static TSharedPtr<FIFCModel> ParseIFC(const FString& FilePath);
    
    // Extract Building Hierarchy
    static void ExtractHierarchy(
        TSharedPtr<FIFCModel> Model,
        TArray<FIFCBuilding>& OutBuildings
    );
    
    // Extract Geo-Reference from IfcSite
    static FGeoLocation ExtractSiteLocation(TSharedPtr<FIFCModel> Model);
};
```

**Beispiel IFC mit Geo-Referenz**:
```ifc
#1= IFCSITE('3vB2ZpPGD3ufhABS8s4z1e', $, 'Factory Site', $, $, 
    #2, $, 'FactoryComplex', .ELEMENT., 
    (52, 31, 12, 345678),  /* RefLatitude: 52°31'12.345678" N */
    (13, 24, 36, 901234),  /* RefLongitude: 13°24'36.901234" E */
    120.5,                 /* RefElevation */
    $
);
```

### 4.2 LiDAR Point Cloud Importer

**Use Case**: Drohnen-Vermessung, Laser-Scanning

```cpp
// PointCloudPlugin/Source/PointCloudPlugin/Public/PointCloudImporter.h

class FPointCloudImporter
{
public:
    struct FPointCloudImportSettings
    {
        // Geo-Referenzierung (meist in LAS Header)
        FCRS CoordinateSystem = FCRS::Auto;  // Auto-detect from LAS header
        
        // Downsampling
        float VoxelSize = 0.1f;              // 10cm voxel grid
        int32 MaxPoints = 10000000;          // 10M points max
        
        // Classification (LAS Standard)
        bool bImportGround = true;           // Class 2
        bool bImportBuildings = true;        // Class 6
        bool bImportVegetation = false;      // Class 3-5
        
        // Optimization
        bool bGenerateMesh = true;           // Poisson Surface Reconstruction
        bool bColorFromRGB = true;
    };
    
    // Import LAS/LAZ file
    static UPointCloudComponent* ImportLASFile(
        const FString& FilePath,
        const FPointCloudImportSettings& Settings
    );
    
private:
    // Parse LAS using LAStools or PDAL
    static TArray<FLiDARPoint> ParseLAS(const FString& FilePath);
    
    // Extract Geo-Reference from LAS Header
    static FGeoReference ExtractGeoReference(const FLASHeader& Header);
    
    // Downsample using Octree
    static TArray<FLiDARPoint> DownsampleOctree(
        const TArray<FLiDARPoint>& Points,
        float VoxelSize
    );
    
    // Poisson Mesh Reconstruction (optional)
    static UStaticMesh* ReconstructMesh(const TArray<FLiDARPoint>& Points);
};

// LiDAR Point Structure
struct FLiDARPoint
{
    FVector Position;       // X, Y, Z
    FColor Color;           // RGB
    uint8 Classification;   // LAS Classification Code
    float Intensity;
    uint8 ReturnNumber;
};
```

**LAS Header Geo-Referenz**:
```cpp
struct FLASHeader
{
    // Variable Length Records (VLRs) contain CRS info
    uint32 PointDataRecordFormat;
    
    // Bounding Box (in file's coordinate system)
    double MinX, MaxX;
    double MinY, MaxY;
    double MinZ, MaxZ;
    
    // Scale/Offset for coordinate compression
    double XScale, YScale, ZScale;
    double XOffset, YOffset, ZOffset;
    
    // GeoKeys (from VLR) - EPSG Code
    uint16 ProjectedCSTypeGeoKey;  // e.g., 32633 for UTM Zone 33N
};
```

### 4.3 GeoJSON Importer

**Use Case**: Gebäude-Footprints, Gebiets-Grenzen, Points of Interest

```cpp
// GeoJSONImporterPlugin/Source/GeoJSONImporterPlugin/Public/GeoJSONImporter.h

class FGeoJSONImporter
{
public:
    struct FGeoJSONImportSettings
    {
        // Feature Selection
        TArray<FString> FeatureTypes;    // ["Building", "Road", "Park"]
        
        // 3D Extrusion
        bool bExtrudePolygons = true;
        FString HeightProperty = "height";  // JSON property name
        float DefaultHeight = 10.0f;
        
        // Materials
        TMap<FString, UMaterialInterface*> MaterialMapping;
    };
    
    // Import GeoJSON file
    static TArray<AActor*> ImportGeoJSONFile(
        const FString& FilePath,
        const FGeoJSONImportSettings& Settings
    );
    
private:
    static TArray<FGeoJSONFeature> ParseGeoJSON(const FString& JSON);
    static AActor* CreateActorFromFeature(const FGeoJSONFeature& Feature);
};
```

**Beispiel GeoJSON mit 3D-Gebäude**:
```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [
            [13.404954, 52.520008],
            [13.405154, 52.520008],
            [13.405154, 52.520208],
            [13.404954, 52.520208],
            [13.404954, 52.520008]
          ]
        ]
      },
      "properties": {
        "name": "Factory Building A",
        "height": 25.5,
        "building": "industrial",
        "material": "concrete",
        "roof_type": "flat"
      }
    }
  ]
}
```

### 4.4 CityGML Importer

**Use Case**: 3D-Stadtmodelle, LoD (Level of Detail) 0-4

```cpp
// CityGMLImporterPlugin/Source/CityGMLImporterPlugin/Public/CityGMLImporter.h

class FCityGMLImporter
{
public:
    enum class ECityGMLLOD
    {
        LOD0,  // Footprint (2D Polygon)
        LOD1,  // Block Model (Extrusion ohne Details)
        LOD2,  // Roof + Facade (detaillierte Außenhülle)
        LOD3,  // Architectural Model (Fenster, Türen)
        LOD4   // Interior Model (Räume, Möbel)
    };
    
    struct FCityGMLImportSettings
    {
        TArray<ECityGMLLOD> LODsToImport = { ECityGMLLOD::LOD2 };
        bool bImportTextures = true;
        bool bImportSemantics = true;  // Wall, Roof, Ground surfaces
    };
    
    static TArray<AActor*> ImportCityGMLFile(
        const FString& FilePath,
        const FCityGMLImportSettings& Settings
    );
    
private:
    // Parse CityGML XML (using libcitygml or custom parser)
    static TSharedPtr<FCityGMLModel> ParseCityGML(const FString& FilePath);
    
    // Extract Building with specific LOD
    static UStaticMesh* ExtractBuildingLOD(
        const FCityGMLBuilding& Building,
        ECityGMLLOD LOD
    );
};
```

**CityGML LOD Beispiel**:
```xml
<core:CityModel>
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:32633">
      <gml:lowerCorner>366000 5806000 0</gml:lowerCorner>
      <gml:upperCorner>367000 5807000 100</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  
  <core:cityObjectMember>
    <bldg:Building gml:id="Building_1">
      <!-- LOD2: Detailed Roof + Facade -->
      <bldg:lod2Solid>
        <gml:Solid>
          <!-- Geometry hier -->
        </gml:Solid>
      </bldg:lod2Solid>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>
```

## 5. WPF Control Panel Integration

### 5.1 Import UI

```xml
<!-- NewImportTab in MainWindow.xaml -->
<TabItem Header="Daten-Import">
    <Grid Margin="10">
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
        </Grid.RowDefinitions>
        
        <!-- File Selection -->
        <GroupBox Grid.Row="0" Header="Datei auswählen">
            <StackPanel>
                <Grid Margin="5">
                    <Grid.ColumnDefinitions>
                        <ColumnDefinition Width="*"/>
                        <ColumnDefinition Width="Auto"/>
                    </Grid.ColumnDefinitions>
                    
                    <TextBox Grid.Column="0" Text="{Binding ImportFilePath}" IsReadOnly="True"/>
                    <Button Grid.Column="1" Content="Durchsuchen..." Command="{Binding BrowseImportFileCommand}"/>
                </Grid>
                
                <TextBlock Text="{Binding DetectedFormat, StringFormat='Erkanntes Format: {0}'}" Margin="5"/>
            </StackPanel>
        </GroupBox>
        
        <!-- Import Settings -->
        <GroupBox Grid.Row="1" Header="Import-Einstellungen" Margin="0,10,0,0">
            <StackPanel>
                <!-- Geo-Referenzierung -->
                <CheckBox Content="Geo-Koordinaten automatisch erkennen" 
                          IsChecked="{Binding AutoDetectGeoReference}" Margin="5"/>
                
                <StackPanel Margin="5" IsEnabled="{Binding AutoDetectGeoReference, Converter={StaticResource InverseBoolConverter}}">
                    <TextBlock Text="Manuell verorten:"/>
                    <Grid Margin="5,5,5,0">
                        <Grid.ColumnDefinitions>
                            <ColumnDefinition Width="Auto"/>
                            <ColumnDefinition Width="*"/>
                        </Grid.ColumnDefinitions>
                        <Grid.RowDefinitions>
                            <RowDefinition/>
                            <RowDefinition/>
                        </Grid.RowDefinitions>
                        
                        <TextBlock Grid.Row="0" Grid.Column="0" Text="Breitengrad:" VerticalAlignment="Center" Margin="0,0,10,5"/>
                        <TextBox Grid.Row="0" Grid.Column="1" Text="{Binding ManualLatitude}" Margin="0,0,0,5"/>
                        
                        <TextBlock Grid.Row="1" Grid.Column="0" Text="Längengrad:" VerticalAlignment="Center" Margin="0,0,10,0"/>
                        <TextBox Grid.Row="1" Grid.Column="1" Text="{Binding ManualLongitude}"/>
                    </Grid>
                </StackPanel>
                
                <!-- Optimization -->
                <CheckBox Content="LODs automatisch generieren" IsChecked="{Binding GenerateLODs}" Margin="5"/>
                <CheckBox Content="Zu Nanite konvertieren" IsChecked="{Binding ConvertToNanite}" Margin="5"/>
                
                <Button Content="Importieren" Command="{Binding StartImportCommand}" Margin="5,10,5,5"/>
            </StackPanel>
        </GroupBox>
        
        <!-- Import Progress -->
        <GroupBox Grid.Row="2" Header="Import-Status" Margin="0,10,0,0">
            <StackPanel>
                <ProgressBar Value="{Binding ImportProgress}" Maximum="100" Height="20" Margin="5"/>
                <TextBlock Text="{Binding ImportStatusMessage}" Margin="5"/>
                <TextBlock Text="{Binding ImportedObjectCount, StringFormat='Importierte Objekte: {0}'}" Margin="5"/>
            </StackPanel>
        </GroupBox>
    </Grid>
</TabItem>
```

### 5.2 Import ViewModel

```csharp
// ViewModels/ImportViewModel.cs
public partial class ImportViewModel : ObservableObject
{
    private readonly IUnrealEngineConnector _unrealConnector;
    private readonly IThemisDBService _themisDBService;
    
    [ObservableProperty]
    private string _importFilePath = "";
    
    [ObservableProperty]
    private string _detectedFormat = "Kein Format erkannt";
    
    [ObservableProperty]
    private bool _autoDetectGeoReference = true;
    
    [ObservableProperty]
    private double _manualLatitude = 52.520008;
    
    [ObservableProperty]
    private double _manualLongitude = 13.404954;
    
    [ObservableProperty]
    private bool _generateLODs = true;
    
    [ObservableProperty]
    private bool _convertToNanite = true;
    
    [ObservableProperty]
    private double _importProgress = 0.0;
    
    [ObservableProperty]
    private string _importStatusMessage = "Bereit";
    
    [ObservableProperty]
    private int _importedObjectCount = 0;
    
    [RelayCommand]
    private void BrowseImportFile()
    {
        var dialog = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "Alle unterstützten Formate|*.ifc;*.las;*.laz;*.e57;*.geojson;*.gml;*.kml;*.glb;*.gltf;*.fbx;*.obj;*.ply|" +
                     "IFC Dateien (*.ifc)|*.ifc|" +
                     "LiDAR (*.las, *.laz)|*.las;*.laz|" +
                     "GeoJSON (*.geojson, *.json)|*.geojson;*.json|" +
                     "CityGML (*.gml)|*.gml|" +
                     "KML (*.kml)|*.kml|" +
                     "3D Modelle (*.glb, *.gltf, *.fbx, *.obj)|*.glb;*.gltf;*.fbx;*.obj|" +
                     "Alle Dateien (*.*)|*.*"
        };
        
        if (dialog.ShowDialog() == true)
        {
            ImportFilePath = dialog.FileName;
            DetectedFormat = DetectFileFormat(ImportFilePath);
        }
    }
    
    [RelayCommand]
    private async Task StartImportAsync()
    {
        if (!_unrealConnector.IsConnected)
        {
            ImportStatusMessage = "Fehler: Nicht mit Unreal Engine verbunden";
            return;
        }
        
        ImportStatusMessage = $"Importiere {Path.GetFileName(ImportFilePath)}...";
        ImportProgress = 0;
        
        var importSettings = new Dictionary<string, object>
        {
            { "FilePath", ImportFilePath },
            { "Format", DetectedFormat },
            { "AutoDetectGeoReference", AutoDetectGeoReference },
            { "ManualLatitude", ManualLatitude },
            { "ManualLongitude", ManualLongitude },
            { "GenerateLODs", GenerateLODs },
            { "ConvertToNanite", ConvertToNanite }
        };
        
        await _unrealConnector.SendCommandAsync("ImportGeoData", importSettings);
        
        // Poll for progress
        while (ImportProgress < 100)
        {
            await Task.Delay(500);
            var response = await _unrealConnector.ReceiveDataAsync();
            var progress = JsonSerializer.Deserialize<ImportProgressUpdate>(response);
            
            if (progress != null)
            {
                ImportProgress = progress.Progress;
                ImportStatusMessage = progress.Message;
                ImportedObjectCount = progress.ObjectCount;
            }
        }
        
        ImportStatusMessage = $"Import abgeschlossen: {ImportedObjectCount} Objekte";
    }
    
    private string DetectFileFormat(string filePath)
    {
        var extension = Path.GetExtension(filePath).ToLowerInvariant();
        return extension switch
        {
            ".ifc" => "IFC (Industry Foundation Classes)",
            ".las" or ".laz" => "LiDAR Point Cloud",
            ".e57" => "E57 Point Cloud",
            ".geojson" or ".json" => "GeoJSON",
            ".gml" => "CityGML",
            ".kml" or ".kmz" => "KML (Google Earth)",
            ".glb" or ".gltf" => "glTF 2.0",
            ".fbx" => "Autodesk FBX",
            ".obj" => "Wavefront OBJ",
            ".ply" => "Stanford PLY",
            _ => "Unbekanntes Format"
        };
    }
}

public class ImportProgressUpdate
{
    public double Progress { get; set; }
    public string Message { get; set; } = "";
    public int ObjectCount { get; set; }
}
```

## 6. ThemisDB Storage Schema für 3D-Daten

### 6.1 Erweiterte Tabellen

```sql
-- Imported 3D Models Table
CREATE TABLE imported_models (
    _key VARCHAR PRIMARY KEY,
    name VARCHAR,
    source_file VARCHAR,
    import_date TIMESTAMP,
    format VARCHAR,  -- "IFC", "LAS", "GeoJSON", etc.
    
    -- Geo-Reference
    location GEOPOINT,        -- Center point
    bounds GEOPOLYGON,        -- Bounding box (2D footprint)
    elevation_min DOUBLE,
    elevation_max DOUBLE,
    crs VARCHAR,              -- e.g., "EPSG:32633"
    
    -- Metadata
    tags MAP<STRING, STRING>, -- Custom metadata
    properties JSON,          -- Format-specific properties
    
    -- Relationships
    parent_site_id VARCHAR,   -- Link to site/project
    
    -- Storage
    geometry_blob BLOB,       -- Compressed mesh data
    lod_levels JSON,          -- LOD metadata
    texture_refs ARRAY<STRING>
);

-- Point Clouds Table
CREATE TABLE point_clouds (
    _key VARCHAR PRIMARY KEY,
    name VARCHAR,
    source_file VARCHAR,
    
    -- Geo-Reference
    bounds GEOPOLYGON,
    elevation_min DOUBLE,
    elevation_max DOUBLE,
    
    -- Point Cloud Stats
    point_count BIGINT,
    density DOUBLE,           -- Points per m²
    has_rgb BOOLEAN,
    has_intensity BOOLEAN,
    has_classification BOOLEAN,
    
    -- Storage (Octree structure)
    octree_root BLOB,
    octree_depth INT
);

-- BIM Elements (from IFC)
CREATE TABLE bim_elements (
    _key VARCHAR PRIMARY KEY,
    ifc_guid VARCHAR UNIQUE,
    ifc_type VARCHAR,         -- "IfcWall", "IfcSlab", etc.
    
    -- Hierarchy
    building_id VARCHAR,
    storey_id VARCHAR,
    space_id VARCHAR,
    
    -- Geometry
    geometry_blob BLOB,
    
    -- Properties
    properties JSON,          -- IFC Property Sets
    materials JSON            -- Material assignments
);
```

### 6.2 AQL Query Beispiele

```sql
-- Finde alle importierten Modelle in einem Gebiet
FOR model IN imported_models
  FILTER GEO_CONTAINS(
    GEO_POLYGON([[13.4, 52.52], [13.41, 52.52], [13.41, 52.53], [13.4, 52.53], [13.4, 52.52]]),
    model.location
  )
  RETURN {
    name: model.name,
    format: model.format,
    import_date: model.import_date
  }

-- Suche Punktwolken mit hoher Dichte
FOR pc IN point_clouds
  FILTER pc.density > 1000  -- > 1000 Points/m²
  SORT pc.point_count DESC
  LIMIT 10
  RETURN pc

-- BIM Query: Alle Wände eines Gebäudes
FOR element IN bim_elements
  FILTER element.building_id == "Building_123"
  AND element.ifc_type == "IfcWall"
  RETURN {
    id: element._key,
    properties: element.properties
  }
```

## 7. Nächste Schritte

### Implementierungs-Priorität

1. **Phase 2A: GeoJSON Importer** (1 Woche)
   - Einfachstes Format
   - Sofort nutzbar für OSM-Export
   - Polygon-Extrusion

2. **Phase 2B: IFC Importer** (2 Wochen)
   - Wichtig für Fabrikplanung
   - Open CASCADE Integration
   - BIM-Hierarchie

3. **Phase 2C: LiDAR Importer** (2 Wochen)
   - LAStools/PDAL Integration
   - Octree-Decimation
   - Optional: Mesh-Rekonstruktion

4. **Phase 2D: CityGML Importer** (1 Woche)
   - 3D-Stadtmodelle
   - LOD-Support

## 8. Dependencies & Libraries

### C++ Libraries (Unreal Plugins)

```cpp
// ThemisGISViewer/Plugins/GeoDataImporters/GeoDataImporters.Build.cs

PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "Json",
    "JsonUtilities"
});

// Third-Party Libraries
if (Target.Platform == UnrealTargetPlatform.Win64)
{
    // IFC: Open CASCADE Technology
    PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "OpenCASCADE", "lib", "TKernel.lib"));
    
    // LiDAR: LAStools (LASlib)
    PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "LAStools", "lib", "LASlib.lib"));
    
    // CityGML: libcitygml
    PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "libcitygml", "lib", "citygml.lib"));
    
    // GDAL for coordinate transformations
    PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "GDAL", "lib", "gdal.lib"));
}
```

---

**Status**: Konzept erweitert ✅  
**Nächster Schritt**: GeoJSON Importer Implementation  
**Version**: 0.2.0  
**Datum**: Dezember 2024
