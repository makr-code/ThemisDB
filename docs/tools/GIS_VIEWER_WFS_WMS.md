# WFS/WMS Integration - OGC Web Services

## Übersicht

Themis.GISViewer unterstützt den direkten Zugriff auf OGC-konforme Web Services:
- **WFS** (Web Feature Service): Vektordaten (Gebäude, Straßen, POIs)
- **WMS** (Web Map Service): Rasterdaten (Orthophotos, Karten)
- **WMTS** (Web Map Tile Service): Gekachelte Karten (effizient)
- **WCS** (Web Coverage Service): Höhendaten, DEM

## 1. OGC Web Services Übersicht

### 1.1 WFS - Web Feature Service

**Zweck**: Download von Vektor-Geodaten im GML/GeoJSON Format

**Versionen**:
- WFS 1.0.0 (alt)
- WFS 1.1.0 (Standard)
- WFS 2.0.0 (aktuell)
- WFS 3.0 (OGC API - Features, neu)

**Operationen**:
- `GetCapabilities` - Verfügbare Layer & Funktionen
- `DescribeFeatureType` - Schema eines Layers
- `GetFeature` - Download von Features (mit Filtern)

**Output-Formate**:
- GML (Geography Markup Language)
- GeoJSON (bevorzugt)
- Shapefile
- CSV

**Beispiel-Server**:
- **Deutschland**: https://sg.geodatenzentrum.de/wfs_geobasis
- **NRW**: https://www.wfs.nrw.de/geobasis/wfs_nw_alkis_vereinfacht
- **Berlin**: https://gdi.berlin.de/services/wfs/alkis

### 1.2 WMS - Web Map Service

**Zweck**: Download von Karten-Bildern (Raster)

**Versionen**:
- WMS 1.1.1 (Standard)
- WMS 1.3.0 (aktuell)

**Operationen**:
- `GetCapabilities` - Verfügbare Layer
- `GetMap` - Download eines Karten-Ausschnitts als Bild
- `GetFeatureInfo` - Attribute an einem Punkt abfragen

**Output-Formate**:
- PNG (mit Transparenz)
- JPEG
- GeoTIFF (georeferenziert)

**Beispiel-Server**:
- **Deutschland**: https://sgx.geodatenzentrum.de/wms_basemapde
- **OSM WMS**: https://ows.terrestris.de/osm/service

### 1.3 WMTS - Web Map Tile Service

**Zweck**: Vorberechnete Kacheln (schneller als WMS)

**Tile-Schema**:
- Google Maps / OSM: XYZ (Zoom, TileX, TileY)
- TMS (Tile Map Service)

**Beispiel**:
```
https://server.com/wmts?
  SERVICE=WMTS&
  REQUEST=GetTile&
  VERSION=1.0.0&
  LAYER=osm&
  STYLE=default&
  TILEMATRIXSET=GoogleMapsCompatible&
  TILEMATRIX=15&
  TILEROW=10995&
  TILECOL=17582&
  FORMAT=image/png
```

### 1.4 WCS - Web Coverage Service

**Zweck**: Raster-Geodaten mit Werten (z.B. Höhendaten)

**Use Case**: 
- DEM (Digital Elevation Model)
- Orthophotos mit Geo-Referenz

## 2. Architektur

```
┌─────────────────────────────────────────────────────────────┐
│              WPF Control Panel                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  WFS/WMS Server Manager UI                           │   │
│  │  - Server-Liste (Favoriten)                          │   │
│  │  - Layer-Browser                                     │   │
│  │  - Bbox Selection (Map Widget)                       │   │
│  │  - Filter-Builder (CQL)                              │   │
│  └──────────────────────────────────────────────────────┘   │
└────────────────────┬────────────────────────────────────────┘
                     │ IPC (Named Pipes)
                     ↓
┌─────────────────────────────────────────────────────────────┐
│           Unreal Engine 5 (ThemisGISViewer)                 │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  OGCWebServicesPlugin (C++)                          │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  • WFSClient                                         │   │
│  │    - GetCapabilities Parser                          │   │
│  │    - GetFeature (GeoJSON/GML)                        │   │
│  │    - CQL Filter Support                              │   │
│  │                                                       │   │
│  │  • WMSClient                                         │   │
│  │    - GetMap (PNG/JPEG)                               │   │
│  │    - Texture Streaming                               │   │
│  │    - Terrain Overlay                                 │   │
│  │                                                       │   │
│  │  • WMTSClient                                        │   │
│  │    - Tile Download                                   │   │
│  │    - Tile Caching                                    │   │
│  │    - LOD-basiertes Streaming                         │   │
│  └──────────────────────────────────────────────────────┘   │
└────────────────────┬────────────────────────────────────────┘
                     │ HTTP/HTTPS
                     ↓
┌─────────────────────────────────────────────────────────────┐
│           OGC Web Services (External)                        │
│  • WFS: Vektordaten (GeoJSON/GML)                           │
│  • WMS: Rasterdaten (PNG/JPEG)                              │
│  • WMTS: Tile-Server                                        │
└─────────────────────────────────────────────────────────────┘
```

## 3. Unreal Engine Plugin: OGCWebServicesPlugin

### 3.1 WFS Client Implementation

```cpp
// OGCWebServicesPlugin/Source/OGCWebServicesPlugin/Public/WFSClient.h

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "WFSClient.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnWFSFeaturesReceived, const TArray<FGeoJSONFeature>&, Features);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnWFSCapabilitiesReceived, const FWFSCapabilities&, Capabilities);

/**
 * WFS Server Configuration
 */
USTRUCT(BlueprintType)
struct OGCWEBSERVICESPLUGIN_API FWFSServerConfig
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WFS")
    FString ServerURL = "https://sg.geodatenzentrum.de/wfs_geobasis";
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WFS")
    FString Version = "2.0.0";
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WFS")
    FString Username = "";  // Optional: HTTP Basic Auth
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WFS")
    FString Password = "";
};

/**
 * WFS Layer Info
 */
USTRUCT(BlueprintType)
struct OGCWEBSERVICESPLUGIN_API FWFSLayer
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString Name;
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString Title;
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString Abstract;
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    TArray<FString> SupportedFormats;  // GeoJSON, GML, etc.
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString DefaultCRS;  // e.g., "EPSG:4326"
};

/**
 * WFS Capabilities (from GetCapabilities)
 */
USTRUCT(BlueprintType)
struct OGCWEBSERVICESPLUGIN_API FWFSCapabilities
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString ServiceTitle;
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString ServiceAbstract;
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    TArray<FWFSLayer> Layers;
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    TArray<FString> SupportedOperations;
};

/**
 * WFS Query Parameters
 */
USTRUCT(BlueprintType)
struct OGCWEBSERVICESPLUGIN_API FWFSQuery
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString TypeName;  // Layer name
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString OutputFormat = "application/json";  // GeoJSON
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString CRS = "EPSG:4326";
    
    // Bounding Box Filter
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FVector2D BBoxMin = FVector2D(0, 0);
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FVector2D BBoxMax = FVector2D(0, 0);
    
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    bool bUseBBox = false;
    
    // CQL Filter (Common Query Language)
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    FString CQLFilter = "";
    
    // Result Limit
    UPROPERTY(BlueprintReadWrite, Category = "WFS")
    int32 MaxFeatures = 1000;
};

/**
 * WFS Client
 */
UCLASS(BlueprintType)
class OGCWEBSERVICESPLUGIN_API UWFSClient : public UObject
{
    GENERATED_BODY()
    
public:
    UWFSClient();
    
    /**
     * Initialize WFS Client with server config
     */
    UFUNCTION(BlueprintCallable, Category = "OGC|WFS")
    void Initialize(const FWFSServerConfig& Config);
    
    /**
     * Get Capabilities (available layers)
     */
    UFUNCTION(BlueprintCallable, Category = "OGC|WFS")
    void GetCapabilitiesAsync(FOnWFSCapabilitiesReceived OnComplete);
    
    /**
     * Get Features (download vector data)
     */
    UFUNCTION(BlueprintCallable, Category = "OGC|WFS")
    void GetFeaturesAsync(
        const FWFSQuery& Query,
        FOnWFSFeaturesReceived OnComplete
    );
    
    /**
     * Get Features in Bounding Box
     */
    UFUNCTION(BlueprintCallable, Category = "OGC|WFS")
    void GetFeaturesInBBoxAsync(
        const FString& LayerName,
        FVector2D BBoxMin,
        FVector2D BBoxMax,
        FOnWFSFeaturesReceived OnComplete
    );

private:
    FWFSServerConfig ServerConfig;
    FHttpModule* HttpModule;
    
    // Build WFS GetCapabilities URL
    FString BuildCapabilitiesURL() const;
    
    // Build WFS GetFeature URL
    FString BuildGetFeatureURL(const FWFSQuery& Query) const;
    
    // Parse GetCapabilities XML response
    FWFSCapabilities ParseCapabilitiesXML(const FString& XML) const;
    
    // Parse GetFeature GeoJSON response
    TArray<FGeoJSONFeature> ParseGeoJSON(const FString& JSON) const;
    
    // HTTP Callbacks
    void HandleCapabilitiesResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bWasSuccessful,
        FOnWFSCapabilitiesReceived Callback
    );
    
    void HandleFeaturesResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bWasSuccessful,
        FOnWFSFeaturesReceived Callback
    );
};
```

### 3.2 WFS Client Implementation (.cpp)

```cpp
// OGCWebServicesPlugin/Source/OGCWebServicesPlugin/Private/WFSClient.cpp

#include "WFSClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "XmlParser.h"
#include "Json.h"

UWFSClient::UWFSClient()
{
    HttpModule = &FHttpModule::Get();
}

void UWFSClient::Initialize(const FWFSServerConfig& Config)
{
    ServerConfig = Config;
    UE_LOG(LogTemp, Log, TEXT("WFS Client initialized: %s"), *Config.ServerURL);
}

FString UWFSClient::BuildCapabilitiesURL() const
{
    return FString::Printf(TEXT("%s?SERVICE=WFS&REQUEST=GetCapabilities&VERSION=%s"),
        *ServerConfig.ServerURL,
        *ServerConfig.Version
    );
}

FString UWFSClient::BuildGetFeatureURL(const FWFSQuery& Query) const
{
    FString URL = FString::Printf(
        TEXT("%s?SERVICE=WFS&REQUEST=GetFeature&VERSION=%s&TYPENAME=%s&OUTPUTFORMAT=%s"),
        *ServerConfig.ServerURL,
        *ServerConfig.Version,
        *Query.TypeName,
        *Query.OutputFormat
    );
    
    // Add BBox filter
    if (Query.bUseBBox)
    {
        URL += FString::Printf(
            TEXT("&BBOX=%f,%f,%f,%f,%s"),
            Query.BBoxMin.X,  // MinLon
            Query.BBoxMin.Y,  // MinLat
            Query.BBoxMax.X,  // MaxLon
            Query.BBoxMax.Y,  // MaxLat
            *Query.CRS
        );
    }
    
    // Add CQL Filter
    if (!Query.CQLFilter.IsEmpty())
    {
        URL += TEXT("&CQL_FILTER=") + FGenericPlatformHttp::UrlEncode(Query.CQLFilter);
    }
    
    // Add MaxFeatures
    if (Query.MaxFeatures > 0)
    {
        URL += FString::Printf(TEXT("&COUNT=%d"), Query.MaxFeatures);
    }
    
    return URL;
}

void UWFSClient::GetCapabilitiesAsync(FOnWFSCapabilitiesReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
    Request->SetURL(BuildCapabilitiesURL());
    Request->SetVerb(TEXT("GET"));
    
    // Add Basic Auth if configured
    if (!ServerConfig.Username.IsEmpty())
    {
        FString AuthValue = FBase64::Encode(ServerConfig.Username + ":" + ServerConfig.Password);
        Request->SetHeader(TEXT("Authorization"), TEXT("Basic ") + AuthValue);
    }
    
    Request->OnProcessRequestComplete().BindUObject(
        this,
        &UWFSClient::HandleCapabilitiesResponse,
        OnComplete
    );
    
    Request->ProcessRequest();
    UE_LOG(LogTemp, Log, TEXT("WFS GetCapabilities request sent"));
}

void UWFSClient::GetFeaturesAsync(
    const FWFSQuery& Query,
    FOnWFSFeaturesReceived OnComplete)
{
    TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
    Request->SetURL(BuildGetFeatureURL(Query));
    Request->SetVerb(TEXT("GET"));
    
    if (!ServerConfig.Username.IsEmpty())
    {
        FString AuthValue = FBase64::Encode(ServerConfig.Username + ":" + ServerConfig.Password);
        Request->SetHeader(TEXT("Authorization"), TEXT("Basic ") + AuthValue);
    }
    
    Request->OnProcessRequestComplete().BindUObject(
        this,
        &UWFSClient::HandleFeaturesResponse,
        OnComplete
    );
    
    Request->ProcessRequest();
    UE_LOG(LogTemp, Log, TEXT("WFS GetFeature request: %s"), *Query.TypeName);
}

void UWFSClient::GetFeaturesInBBoxAsync(
    const FString& LayerName,
    FVector2D BBoxMin,
    FVector2D BBoxMax,
    FOnWFSFeaturesReceived OnComplete)
{
    FWFSQuery Query;
    Query.TypeName = LayerName;
    Query.BBoxMin = BBoxMin;
    Query.BBoxMax = BBoxMax;
    Query.bUseBBox = true;
    
    GetFeaturesAsync(Query, OnComplete);
}

void UWFSClient::HandleCapabilitiesResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful,
    FOnWFSCapabilitiesReceived Callback)
{
    FWFSCapabilities Capabilities;
    
    if (bWasSuccessful && Response.IsValid())
    {
        FString XML = Response->GetContentAsString();
        Capabilities = ParseCapabilitiesXML(XML);
        UE_LOG(LogTemp, Log, TEXT("WFS Capabilities received: %d layers"), Capabilities.Layers.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WFS GetCapabilities failed"));
    }
    
    Callback.ExecuteIfBound(Capabilities);
}

void UWFSClient::HandleFeaturesResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful,
    FOnWFSFeaturesReceived Callback)
{
    TArray<FGeoJSONFeature> Features;
    
    if (bWasSuccessful && Response.IsValid())
    {
        FString JSON = Response->GetContentAsString();
        Features = ParseGeoJSON(JSON);
        UE_LOG(LogTemp, Log, TEXT("WFS Features received: %d"), Features.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WFS GetFeature failed"));
    }
    
    Callback.ExecuteIfBound(Features);
}

FWFSCapabilities UWFSClient::ParseCapabilitiesXML(const FString& XML) const
{
    FWFSCapabilities Capabilities;
    
    // Parse XML using Unreal's FXmlFile
    TSharedRef<FXmlFile> XmlFile = MakeShareable(new FXmlFile(XML, EConstructMethod::ConstructFromBuffer));
    
    if (XmlFile->IsValid())
    {
        FXmlNode* RootNode = XmlFile->GetRootNode();
        
        // Parse Service Metadata
        FXmlNode* ServiceNode = RootNode->FindChildNode(TEXT("ows:ServiceIdentification"));
        if (ServiceNode)
        {
            FXmlNode* TitleNode = ServiceNode->FindChildNode(TEXT("ows:Title"));
            if (TitleNode)
                Capabilities.ServiceTitle = TitleNode->GetContent();
        }
        
        // Parse FeatureTypeList
        FXmlNode* FeatureTypeListNode = RootNode->FindChildNode(TEXT("FeatureTypeList"));
        if (FeatureTypeListNode)
        {
            TArray<FXmlNode*> FeatureTypeNodes = FeatureTypeListNode->GetChildrenNodes();
            for (FXmlNode* FeatureTypeNode : FeatureTypeNodes)
            {
                FWFSLayer Layer;
                
                FXmlNode* NameNode = FeatureTypeNode->FindChildNode(TEXT("Name"));
                if (NameNode)
                    Layer.Name = NameNode->GetContent();
                
                FXmlNode* TitleNode = FeatureTypeNode->FindChildNode(TEXT("Title"));
                if (TitleNode)
                    Layer.Title = TitleNode->GetContent();
                
                Capabilities.Layers.Add(Layer);
            }
        }
    }
    
    return Capabilities;
}

TArray<FGeoJSONFeature> UWFSClient::ParseGeoJSON(const FString& JSON) const
{
    TArray<FGeoJSONFeature> Features;
    
    // Parse JSON (same as GeoJSON importer)
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSON);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        const TArray<TSharedPtr<FJsonValue>>* FeaturesArray;
        if (JsonObject->TryGetArrayField(TEXT("features"), FeaturesArray))
        {
            for (const TSharedPtr<FJsonValue>& FeatureValue : *FeaturesArray)
            {
                FGeoJSONFeature Feature;
                // Parse feature (geometry, properties)
                // ... (implementation similar to GeoJSON importer)
                Features.Add(Feature);
            }
        }
    }
    
    return Features;
}
```

### 3.3 WMS Client Implementation

```cpp
// OGCWebServicesPlugin/Source/OGCWebServicesPlugin/Public/WMSClient.h

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Engine/Texture2D.h"
#include "WMSClient.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnWMSMapReceived, UTexture2D*, MapTexture);

/**
 * WMS GetMap Parameters
 */
USTRUCT(BlueprintType)
struct OGCWEBSERVICESPLUGIN_API FWMSGetMapRequest
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    FString ServerURL = "https://sgx.geodatenzentrum.de/wms_basemapde";
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    FString Layers = "de_basemapde_web_raster_grau";
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    FString Format = "image/png";
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    FString CRS = "EPSG:4326";
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    int32 Width = 1024;
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    int32 Height = 1024;
    
    // Bounding Box (Lon/Lat)
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    FVector2D BBoxMin = FVector2D(13.0, 52.0);
    
    UPROPERTY(BlueprintReadWrite, Category = "WMS")
    FVector2D BBoxMax = FVector2D(14.0, 53.0);
};

/**
 * WMS Client
 */
UCLASS(BlueprintType)
class OGCWEBSERVICESPLUGIN_API UWMSClient : public UObject
{
    GENERATED_BODY()
    
public:
    /**
     * Get Map Image from WMS Server
     */
    UFUNCTION(BlueprintCallable, Category = "OGC|WMS")
    void GetMapAsync(
        const FWMSGetMapRequest& Request,
        FOnWMSMapReceived OnComplete
    );

private:
    FHttpModule* HttpModule;
    
    FString BuildGetMapURL(const FWMSGetMapRequest& Request) const;
    
    void HandleMapResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bWasSuccessful,
        FOnWMSMapReceived Callback
    );
    
    UTexture2D* CreateTextureFromPNG(const TArray<uint8>& PNGData);
};
```

## 4. WPF Control Panel - WFS/WMS Manager UI

### 4.1 XAML UI

```xml
<!-- NewTab in MainWindow.xaml -->
<TabItem Header="WFS/WMS Server">
    <Grid Margin="10">
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
        </Grid.RowDefinitions>
        
        <!-- Server Configuration -->
        <GroupBox Grid.Row="0" Header="Server-Konfiguration">
            <StackPanel>
                <Grid Margin="5">
                    <Grid.ColumnDefinitions>
                        <ColumnDefinition Width="Auto"/>
                        <ColumnDefinition Width="*"/>
                    </Grid.ColumnDefinitions>
                    <Grid.RowDefinitions>
                        <RowDefinition/>
                        <RowDefinition/>
                        <RowDefinition/>
                    </Grid.RowDefinitions>
                    
                    <TextBlock Grid.Row="0" Grid.Column="0" Text="Service-Typ:" Margin="0,0,10,5"/>
                    <ComboBox Grid.Row="0" Grid.Column="1" SelectedItem="{Binding SelectedServiceType}" Margin="0,0,0,5">
                        <ComboBoxItem Content="WFS (Vektordaten)"/>
                        <ComboBoxItem Content="WMS (Rasterkarten)"/>
                        <ComboBoxItem Content="WMTS (Kacheln)"/>
                    </ComboBox>
                    
                    <TextBlock Grid.Row="1" Grid.Column="0" Text="Server-URL:" Margin="0,0,10,5"/>
                    <TextBox Grid.Row="1" Grid.Column="1" Text="{Binding ServerURL}" Margin="0,0,0,5"/>
                    
                    <TextBlock Grid.Row="2" Grid.Column="0" Text="Version:" Margin="0,0,10,0"/>
                    <TextBox Grid.Row="2" Grid.Column="1" Text="{Binding ServiceVersion}"/>
                </Grid>
                
                <Button Content="Capabilities laden" Command="{Binding LoadCapabilitiesCommand}" Margin="5"/>
            </StackPanel>
        </GroupBox>
        
        <!-- Layer Selection -->
        <GroupBox Grid.Row="1" Header="Verfügbare Layer" Margin="0,10,0,0">
            <DataGrid ItemsSource="{Binding AvailableLayers}" 
                      SelectedItem="{Binding SelectedLayer}"
                      AutoGenerateColumns="False"
                      Height="200">
                <DataGrid.Columns>
                    <DataGridTextColumn Header="Name" Binding="{Binding Name}" Width="200"/>
                    <DataGridTextColumn Header="Titel" Binding="{Binding Title}" Width="*"/>
                    <DataGridTextColumn Header="Format" Binding="{Binding DefaultFormat}" Width="100"/>
                </DataGrid.Columns>
            </DataGrid>
        </GroupBox>
        
        <!-- Query Parameters -->
        <GroupBox Grid.Row="2" Header="Abfrage-Parameter" Margin="0,10,0,0">
            <StackPanel>
                <GroupBox Header="Bounding Box">
                    <Grid Margin="5">
                        <Grid.ColumnDefinitions>
                            <ColumnDefinition Width="Auto"/>
                            <ColumnDefinition Width="*"/>
                            <ColumnDefinition Width="Auto"/>
                            <ColumnDefinition Width="*"/>
                        </Grid.ColumnDefinitions>
                        <Grid.RowDefinitions>
                            <RowDefinition/>
                            <RowDefinition/>
                        </Grid.RowDefinitions>
                        
                        <TextBlock Grid.Row="0" Grid.Column="0" Text="Min Lon:" Margin="0,0,5,5"/>
                        <TextBox Grid.Row="0" Grid.Column="1" Text="{Binding BBoxMinLon}" Margin="0,0,10,5"/>
                        <TextBlock Grid.Row="0" Grid.Column="2" Text="Max Lon:" Margin="0,0,5,5"/>
                        <TextBox Grid.Row="0" Grid.Column="3" Text="{Binding BBoxMaxLon}" Margin="0,0,0,5"/>
                        
                        <TextBlock Grid.Row="1" Grid.Column="0" Text="Min Lat:" Margin="0,0,5,0"/>
                        <TextBox Grid.Row="1" Grid.Column="1" Text="{Binding BBoxMinLat}" Margin="0,0,10,0"/>
                        <TextBlock Grid.Row="1" Grid.Column="2" Text="Max Lat:" Margin="0,0,5,0"/>
                        <TextBox Grid.Row="1" Grid.Column="3" Text="{Binding BBoxMaxLat}"/>
                    </Grid>
                </GroupBox>
                
                <GroupBox Header="Filter (CQL)" Margin="0,10,0,0">
                    <TextBox Text="{Binding CQLFilter}" 
                             Height="60" 
                             TextWrapping="Wrap" 
                             AcceptsReturn="True"
                             VerticalScrollBarVisibility="Auto"/>
                </GroupBox>
                
                <StackPanel Orientation="Horizontal" Margin="0,10,0,0">
                    <TextBlock Text="Max. Features:" VerticalAlignment="Center" Margin="0,0,10,0"/>
                    <TextBox Text="{Binding MaxFeatures}" Width="100"/>
                </StackPanel>
                
                <Button Content="Daten laden" Command="{Binding LoadDataCommand}" Margin="0,10,0,0"/>
                
                <TextBlock Text="{Binding LoadStatus}" Margin="0,10,0,0"/>
            </StackPanel>
        </GroupBox>
    </Grid>
</TabItem>
```

### 4.2 ViewModel

```csharp
// ViewModels/WFSWMSViewModel.cs

public partial class WFSWMSViewModel : ObservableObject
{
    private readonly IUnrealEngineConnector _unrealConnector;
    
    [ObservableProperty]
    private string _selectedServiceType = "WFS (Vektordaten)";
    
    [ObservableProperty]
    private string _serverURL = "https://sg.geodatenzentrum.de/wfs_geobasis";
    
    [ObservableProperty]
    private string _serviceVersion = "2.0.0";
    
    [ObservableProperty]
    private ObservableCollection<WFSLayer> _availableLayers = new();
    
    [ObservableProperty]
    private WFSLayer? _selectedLayer;
    
    [ObservableProperty]
    private double _bBoxMinLon = 13.3;
    
    [ObservableProperty]
    private double _bBoxMaxLon = 13.5;
    
    [ObservableProperty]
    private double _bBoxMinLat = 52.4;
    
    [ObservableProperty]
    private double _bBoxMaxLat = 52.6;
    
    [ObservableProperty]
    private string _cqlFilter = "";
    
    [ObservableProperty]
    private int _maxFeatures = 1000;
    
    [ObservableProperty]
    private string _loadStatus = "Bereit";
    
    [RelayCommand]
    private async Task LoadCapabilitiesAsync()
    {
        if (!_unrealConnector.IsConnected)
        {
            LoadStatus = "Fehler: Nicht verbunden";
            return;
        }
        
        LoadStatus = "Lade Capabilities...";
        
        await _unrealConnector.SendCommandAsync("WFS_GetCapabilities", new Dictionary<string, object>
        {
            { "ServerURL", ServerURL },
            { "Version", ServiceVersion }
        });
        
        var response = await _unrealConnector.ReceiveDataAsync();
        var capabilities = JsonSerializer.Deserialize<WFSCapabilitiesResponse>(response);
        
        if (capabilities != null)
        {
            AvailableLayers.Clear();
            foreach (var layer in capabilities.Layers)
            {
                AvailableLayers.Add(new WFSLayer
                {
                    Name = layer.Name,
                    Title = layer.Title,
                    DefaultFormat = "GeoJSON"
                });
            }
            
            LoadStatus = $"{AvailableLayers.Count} Layer verfügbar";
        }
    }
    
    [RelayCommand]
    private async Task LoadDataAsync()
    {
        if (SelectedLayer == null)
        {
            LoadStatus = "Fehler: Kein Layer ausgewählt";
            return;
        }
        
        LoadStatus = $"Lade Daten von {SelectedLayer.Name}...";
        
        await _unrealConnector.SendCommandAsync("WFS_GetFeatures", new Dictionary<string, object>
        {
            { "ServerURL", ServerURL },
            { "LayerName", SelectedLayer.Name },
            { "BBoxMinLon", BBoxMinLon },
            { "BBoxMinLat", BBoxMinLat },
            { "BBoxMaxLon", BBoxMaxLon },
            { "BBoxMaxLat", BBoxMaxLat },
            { "CQLFilter", CQLFilter },
            { "MaxFeatures", MaxFeatures }
        });
        
        // Receive progress updates
        var response = await _unrealConnector.ReceiveDataAsync();
        var result = JsonSerializer.Deserialize<WFSLoadResult>(response);
        
        if (result != null)
        {
            LoadStatus = $"Geladen: {result.FeatureCount} Features";
        }
    }
}

public class WFSLayer
{
    public string Name { get; set; } = "";
    public string Title { get; set; } = "";
    public string DefaultFormat { get; set; } = "";
}

public class WFSCapabilitiesResponse
{
    public List<WFSLayer> Layers { get; set; } = new();
}

public class WFSLoadResult
{
    public int FeatureCount { get; set; }
    public string Message { get; set; } = "";
}
```

## 5. Beispiel-Server & Use Cases

### 5.1 Deutschland - Offizielle Geodatenserver

```csharp
// Predefined Server List
public static class GermanGeoServers
{
    public static List<WFSServerConfig> GetPredefinedServers()
    {
        return new List<WFSServerConfig>
        {
            new()
            {
                Name = "BKG - Verwaltungsgebiete",
                ServerURL = "https://sg.geodatenzentrum.de/wfs_vg250",
                Layers = new[] { "vg250_gem", "vg250_krs", "vg250_lan" }
            },
            new()
            {
                Name = "NRW - ALKIS Gebäude",
                ServerURL = "https://www.wfs.nrw.de/geobasis/wfs_nw_alkis_vereinfacht",
                Layers = new[] { "ave:GebaeudeBauwerk" }
            },
            new()
            {
                Name = "Berlin - Stadtstruktur",
                ServerURL = "https://gdi.berlin.de/services/wfs/stadtstruktur",
                Layers = new[] { "fis:re_stadtstr" }
            }
        };
    }
}
```

### 5.2 Use Case: Berlin Gebäude laden

```cpp
// In Unreal Blueprint or C++

UWFSClient* WFSClient = NewObject<UWFSClient>();

FWFSServerConfig Config;
Config.ServerURL = "https://fbinter.stadt-berlin.de/fb/wfs/geometry/senstadt/re_stadtstruktur";
Config.Version = "2.0.0";

WFSClient->Initialize(Config);

// Load Buildings in Berlin Mitte
FWFSQuery Query;
Query.TypeName = "fis:re_stadtstr";
Query.BBoxMin = FVector2D(13.3, 52.5);  // Southwest
Query.BBoxMax = FVector2D(13.5, 52.6);  // Northeast
Query.bUseBBox = true;
Query.MaxFeatures = 5000;

WFSClient->GetFeaturesAsync(Query, [](const TArray<FGeoJSONFeature>& Features)
{
    UE_LOG(LogTemp, Log, TEXT("Received %d buildings from WFS"), Features.Num());
    
    for (const FGeoJSONFeature& Feature : Features)
    {
        // Spawn Procedural Building in Unreal
        SpawnBuildingFromGeoJSON(Feature);
    }
});
```

## 6. Performance & Caching

### 6.1 Tile Caching

```cpp
// Cache WMS tiles to disk
class FWMSTileCache
{
public:
    // Cache directory
    FString CacheDir = FPaths::ProjectSavedDir() / TEXT("WMSCache");
    
    // Generate cache key from request
    FString GetCacheKey(const FWMSGetMapRequest& Request)
    {
        return FMD5::HashAnsiString(*FString::Printf(
            TEXT("%s_%s_%d_%d_%f_%f_%f_%f"),
            *Request.ServerURL,
            *Request.Layers,
            Request.Width,
            Request.Height,
            Request.BBoxMin.X,
            Request.BBoxMin.Y,
            Request.BBoxMax.X,
            Request.BBoxMax.Y
        ));
    }
    
    // Check if tile exists in cache
    bool IsCached(const FString& CacheKey)
    {
        FString FilePath = CacheDir / CacheKey + TEXT(".png");
        return FPaths::FileExists(FilePath);
    }
    
    // Load from cache
    UTexture2D* LoadFromCache(const FString& CacheKey)
    {
        FString FilePath = CacheDir / CacheKey + TEXT(".png");
        TArray<uint8> PNGData;
        
        if (FFileHelper::LoadFileToArray(PNGData, *FilePath))
        {
            return CreateTextureFromPNG(PNGData);
        }
        
        return nullptr;
    }
    
    // Save to cache
    void SaveToCache(const FString& CacheKey, const TArray<uint8>& PNGData)
    {
        FString FilePath = CacheDir / CacheKey + TEXT(".png");
        FFileHelper::SaveArrayToFile(PNGData, *FilePath);
    }
};
```

## 7. Nächste Schritte

1. **Phase 2E: WFS/WMS Plugin** (2 Wochen)
   - [x] Konzept erstellt
   - [ ] WFSClient Implementation
   - [ ] WMSClient Implementation
   - [ ] WPF UI Integration
   - [ ] Tile Caching
   - [ ] Beispiel-Server Integration

2. **Phase 2F: WMTS Plugin** (1 Woche)
   - [ ] Tile-basiertes Streaming
   - [ ] LOD-Management
   - [ ] OpenStreetMap Integration

---

**Status**: WFS/WMS Konzept komplett ✅  
**Nächster Schritt**: Implementation WFSClient  
**Version**: 0.3.0  
**Datum**: Dezember 2024
