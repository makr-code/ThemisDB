---
category: "🛡️ Security/Compliance"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 📊 Comprehensive Risk Assessment

Umfassende Risiko-Bewertung für Umwelt- und Anlagenrisiken.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht

ThemisDB bietet umfassende Risikobewertungsmodelle für Umweltrisiken und Anlagenrisiken, die vollständig mit deutschen Umwelt- und Sicherheitsvorschriften konform sind.

**Enterprise Feature**: Diese Risikobewertungsmodelle sind als Enterprise-Funktionen gekapselt und separat verfügbar.

**Core Feature**: Die 3D-Geospatial-Unterstützung (Point(x,y,z)) ist als Core-Feature in ThemisDB integriert.

## Headers

```cpp
// Enterprise Features
#include "enterprise/environmental_risk_models.h"
#include "enterprise/facility_risk_assessment.h"
#include "enterprise/arcgis_data_provider.h"
```

## Umweltrisikobewertung

### Unterstützte Risikoarten

#### 1. Wasserrisiken (WHG - Wasserhaushaltsgesetz)

**Hochwasserrisiko (HQ10, HQ100, HQ200)**
```cpp
WaterRiskParams params;
params.water_level_m = 180.0;  // Wasserpegel
params.flow_rate_m3s = 500.0;  // Durchfluss

auto result = assessor.assessFloodRisk(
    area_geometry,
    params,
    100  // HQ100 (100-jährliches Hochwasser)
);
```

**Grundwasserverschmutzung (WHG §48)**
```cpp
params.contamination_ppm = 15.0;
params.groundwater_depth_m = 5.0;

auto result = assessor.assessGroundwaterContaminationRisk(
    contamination_source,
    params,
    2  // Wasserschutzzone II
);
```

**Dürrerisiko**
```cpp
params.precipitation_mm = 450.0;  // Niederschlag < 500mm/Jahr
auto result = assessor.assessDroughtRisk(area, params, 30);
```

#### 2. Bodenrisiken (BBodSchG - Bundes-Bodenschutzgesetz)

**Bodenkontamination (BBodSchG §4)**
```cpp
SoilRiskParams soil_params;
soil_params.contamination_mg_kg = 500.0;
soil_params.soil_type = "sandy_loam";

auto result = assessor.assessSoilContaminationRisk(
    area,
    soil_params,
    "residential"  // Wohngebiet
);
```

**Erdrutschrisiko (3D Terrain)**
```cpp
std::vector<Coordinate> terrain = {
    {8.5, 50.0, 500.0},
    {8.51, 50.01, 450.0},
    {8.52, 50.02, 400.0}  // Steiler Hang
};

soil_params.slope_degrees = 35.0;
soil_params.soil_moisture_percent = 85.0;

auto result = assessor.assessLandslideRisk(terrain, soil_params);
```

**Bodenerosion (DIN 19708)**
```cpp
soil_params.erosion_rate_t_ha_year = 15.0;  // > 10 t/ha/Jahr kritisch
auto result = assessor.assessSoilErosionRisk(area, soil_params);
```

#### 3. Luftqualität (BImSchG, 39. BImSchV)

**Luftverschmutzung**
```cpp
AirQualityParams air_params;
air_params.pm10_ug_m3 = 65.0;      // Grenzwert: 50 µg/m³
air_params.pm25_ug_m3 = 35.0;      // Grenzwert: 25 µg/m³
air_params.no2_ug_m3 = 55.0;       // Grenzwert: 40 µg/m³

auto result = assessor.assessAirPollutionRisk(urban_area, air_params);
// result.violations enthält überschrittene Grenzwerte
```

**Smog-Risiko**
```cpp
auto result = assessor.assessSmogRisk(
    city_center,
    air_params,
    35.0  // Temperatur > 30°C erhöht Risiko
);
```

#### 4. Klimarisiken

**Hitzewelle (DWD-Kriterien)**
```cpp
ClimateRiskParams climate;
climate.temperature_c = 38.0;
climate.heat_index_c = 42.0;

auto result = assessor.assessHeatWaveRisk(
    area,
    climate,
    5  // 5 aufeinanderfolgende Tage > 30°C
);
```

**Sturmrisiko (Beaufort-Skala)**
```cpp
climate.wind_speed_ms = 35.0;  // Beaufort 12 (Orkan)
auto result = assessor.assessStormRisk(area, climate);
```

**Waldbrandrisiko (Canadian FWI)**
```cpp
climate.temperature_c = 32.0;
climate.precipitation_mm_h = 0.0;

auto result = assessor.assessForestFireRisk(
    forest_area,
    climate,
    0.85  // Vegetation dryness (0-1)
);
```

#### 5. Seismische Risiken

**Erdbebenrisiko (DIN EN 1998-1)**
```cpp
SeismicRiskParams seismic;
seismic.magnitude = 5.8;
seismic.peak_ground_acceleration_g = 0.25;
seismic.intensity_ems98 = 7;  // EMS-98 Intensität VII

Coordinate epicenter(8.5, 50.0, 10000.0);  // 10km Tiefe

auto result = assessor.assessEarthquakeRisk(
    epicenter,
    seismic,
    buildings_in_area
);
```

## Anlagenrisikobewertung

### Anlagentypen

ThemisDB unterstützt umfassende Risikobewertungen für:

- **Chemische Anlagen** (12. BImSchV, Seveso-III)
- **Energieanlagen** (EnWG)
- **Wasser/Abwasser** (WHG, AbwV)
- **Abfallentsorgung** (KrWG, DepV)
- **Lageranlagen** (AwSV, VAwS)
- **Kritische Infrastruktur** (KRITIS)
- **Transportinfrastruktur**

### Seveso-III Bewertung (Störfall-Verordnung)

#### Klassifizierung

```cpp
FacilityInventory facility;
facility.facility_type = FacilityType::CHEMICAL_PLANT;
facility.location = Coordinate(8.5, 50.0, 150.0);

// Gefahrstoffe definieren
FacilityInventory::HazardousSubstance ammonia;
ammonia.name = "Ammonia";
ammonia.cas_number = "7664-41-7";
ammonia.quantity_kg = 75000;  // 75 Tonnen
ammonia.hazard_statements = {"H221", "H280", "H314", "H400"};
ammonia.water_hazard_class = 2;

facility.substances.push_back(ammonia);

// Seveso-Klassifizierung
auto classification = assessor.classifySevesoFacility(facility.substances);

if (classification.is_seveso_facility) {
    std::cout << "Seveso facility: " 
              << (classification.is_upper_tier ? "Upper-tier" : "Lower-tier") 
              << std::endl;
    // Ausgabe: Seveso facility: Upper-tier
}
```

#### Dominoeffekt-Analyse (12. BImSchV §3 Abs. 5c)

```cpp
DominoEffectParams domino;
domino.primary_facility = chemical_plant.location;
domino.nearby_facilities = {refinery.location, power_plant.location};

// Explosionsparameter
domino.blast_overpressure_kpa = 50.0;  // 50 kPa Druckwelle
domino.thermal_radiation_kw_m2 = 15.0; // 15 kW/m² Wärmestrahlung
domino.toxic_concentration_ppm = 100.0;

// Wetterbedingungen
domino.wind_speed_ms = 5.0;
domino.wind_direction_deg = 270.0;  // West
domino.atmospheric_stability = 0.5;  // Pasquill D

auto result = assessor.assessDominoEffect(domino);

// Ergebnis enthält:
// - Betroffene Nachbaranlagen
// - Kaskadenwahrscheinlichkeit
// - Auswirkungsradien
```

#### Sicherheitsabstände (TA Abstand)

```cpp
double required_distance = assessor.calculateSafetyDistance(
    chemical_plant,
    "residential"  // Wohngebiet
);

// Prüfung: Sind Nachbarn zu nah?
for (const auto& neighbor : nearby_buildings) {
    double actual_distance = calculateDistance(
        chemical_plant.location,
        neighbor.location
    );
    
    if (actual_distance < required_distance) {
        std::cout << "Violation: " << neighbor.id 
                  << " too close (" << actual_distance << "m < "
                  << required_distance << "m)" << std::endl;
    }
}
```

### Lageranlagen-Bewertung (AwSV - Anlagenverordnung)

#### Tanklager-Risiko

```cpp
StorageTankParams tank;
tank.volume_m3 = 50.0;
tank.tank_type = "above_ground";
tank.material = "steel";
tank.age_years = 25;

// Sicherheitseinrichtungen
tank.has_double_wall = true;
tank.has_leak_detection = true;
tank.has_overflow_protection = true;
tank.bund_capacity_m3 = 55.0;  // 110% des Tankvolumens

// Inhalt
tank.substance_name = "Diesel";
tank.water_hazard_class = 2;  // WGK 2
tank.fill_level_percent = 85.0;

// Risikobewertung
auto result = assessor.assessStorageTankRisk(
    tank,
    true,  // In Wasserschutzzone
    2      // Zone II
);

if (!result.violations.empty()) {
    std::cout << "AwSV violations found:" << std::endl;
    for (const auto& violation : result.violations) {
        std::cout << "  - " << violation << std::endl;
    }
}
```

#### Rückhaltesystem-Prüfung

```cpp
auto containment = assessor.assessContainmentSystem(
    100.0,  // 100 m³ Lagervolumen
    105.0,  // 105 m³ Auffangwanne
    2       // WGK 2
);

if (!containment.is_adequate) {
    std::cout << "Containment inadequate!" << std::endl;
    std::cout << "Required: " << containment.required_volume_m3 << " m³" << std::endl;
    std::cout << "Provided: " << containment.provided_volume_m3 << " m³" << std::endl;
    std::cout << "Deficit: " << containment.deficit_m3 << " m³" << std::endl;
}
```

### Brand- und Explosionsrisiko

#### Explosionsradius-Berechnung (TNT-Äquivalent)

```cpp
// 10 Tonnen Propan
double radius = assessor.calculateExplosionRadius(
    10000,   // kg
    0.05,    // TNT-Äquivalenz 5%
    20.0     // 20 kPa Druckschwelle (schwere Gebäudeschäden)
);

std::cout << "Explosion impact radius: " << radius << " m" << std::endl;
// Ausgabe: ~250m für schwere Schäden
```

#### BLEVE-Analyse

```cpp
auto bleve = assessor.calculateBLEVEImpact(
    50.0,   // 50 m³ Druckbehälter
    80.0,   // 80% Füllstand
    15.0    // 15 bar
);

std::cout << "BLEVE Impact Zones:" << std::endl;
std::cout << "  Fireball: " << bleve.fireball_radius_m << " m" << std::endl;
std::cout << "  100% lethality: " 
          << bleve.thermal_radiation_100percent_lethality_m << " m" << std::endl;
std::cout << "  1% lethality: " 
          << bleve.thermal_radiation_1percent_lethality_m << " m" << std::endl;
std::cout << "  Severe damage: " 
          << bleve.blast_overpressure_severe_damage_m << " m" << std::endl;
```

### Toxische Freisetzung (VDI 3783)

#### Schadstoffausbreitung

```cpp
Coordinate release(8.5, 50.0, 25.0);  // 25m Höhe (Schornstein)

auto dispersion = assessor.assessToxicDispersion(
    release,
    "Chlorine",         // Chlorgas
    0.5,                // 0.5 kg/s Freisetzungsrate
    5.0,                // 5 m/s Windgeschwindigkeit
    270.0,              // Wind aus West
    'D'                 // Pasquill-Gifford Klasse D (neutral)
);

std::cout << "Affected area: " << dispersion.affected_area.area() << " km²" << std::endl;
std::cout << "Max downwind distance: " << dispersion.max_downwind_distance_m << " m" << std::endl;
std::cout << "Population at risk: " << dispersion.population_at_risk << std::endl;

// Isopleths (Konzentrations-Konturen)
for (size_t i = 0; i < dispersion.concentration_contours.size(); ++i) {
    std::cout << "  " << dispersion.concentration_levels_ppm[i] << " ppm: "
              << "contour area" << std::endl;
}
```

#### AEGL/ERPG Expositionsgrenzwerte

```cpp
auto exposure = assessor.calculateExposureLevel(
    "Ammonia",
    150.0,  // 150 ppm
    60.0    // 60 Minuten Exposition
);

std::cout << "Exposure level: " << exposure.level << std::endl;
std::cout << "Health effect: " << exposure.health_effect << std::endl;
if (exposure.life_threatening) {
    std::cout << "WARNING: Life-threatening concentration!" << std::endl;
}
```

### Kritische Infrastruktur (KRITIS)

#### KRITIS-Resilienz-Bewertung

```cpp
FacilityInventory hospital;
hospital.facility_type = FacilityType::HOSPITAL;
hospital.location = Coordinate(8.5, 50.0, 100.0);

auto result = assessor.assessKRITISResilience(
    hospital,
    "health"  // Gesundheitssektor
);

std::cout << "Resilience score: " << result.overall_risk_score << std::endl;
```

#### Kritikalitätsindex

```cpp
double criticality = assessor.calculateCriticalityIndex(
    power_plant,
    500000,  // Versorgt 500.000 Einwohner
    2        // 2 alternative Kraftwerke in der Region
);

if (criticality > 0.8) {
    std::cout << "Highly critical facility!" << std::endl;
}
```

### Regulatorische Compliance

#### Compliance-Prüfung

```cpp
auto compliance = assessor.checkRegulatoryCompliance(facility);

if (!compliance.is_compliant) {
    std::cout << "Compliance violations found:" << std::endl;
    
    for (const auto& violation : compliance.violations) {
        std::cout << "  ❌ " << violation << std::endl;
    }
    
    std::cout << "\nMissing permits:" << std::endl;
    for (const auto& permit : compliance.missing_permits) {
        std::cout << "  - " << permit << std::endl;
    }
    
    std::cout << "\nOverdue inspections:" << std::endl;
    for (const auto& inspection : compliance.overdue_inspections) {
        std::cout << "  - " << inspection << std::endl;
    }
}
```

#### Sicherheitsbericht (Seveso-III Art. 10)

```cpp
std::string safety_report = assessor.generateSafetyReport(
    facility,
    risk_result,
    "pdf"  // PDF-Format
);

// Speichern
std::ofstream out("safety_report.pdf", std::ios::binary);
out.write(safety_report.data(), safety_report.size());
```

#### Alarm- und Gefahrenabwehrplan

```cpp
auto emergency_plan = assessor.generateEmergencyResponsePlan(
    facility,
    risk_result
);

// Plan enthält:
// - Alarmierungskaskade
// - Evakuierungszonen
// - Notfallmaßnahmen
// - Kontaktinformationen
// - Lagepläne
```

## Integration mit ArcGIS

Alle Risikobewertungen können direkt nach ArcGIS exportiert werden:

```cpp
// 1. Risikobewertung durchführen
auto flood_risk = assessor.assessFloodRisk(area, params, 100);

// 2. Export nach ArcGIS
IArcGISDataProvider* provider = CreateArcGISDataProvider();
provider->connect("path=C:\\Data\\ThemisDB");

SpatialFeature feature;
feature.geometry = area;
feature.attributes.properties = flood_risk.detailed_findings;

// 3. In ArcGIS visualisieren
// Die Daten erscheinen automatisch in ArcGIS Pro/Server
```

## Anwendungsbeispiele

### Beispiel 1: Hochwasser-Risikokartierung

```cpp
// 1. Alle Anlagen in der Region abfragen
auto facilities = query_builder.findFacilitiesInWaterProtectionZone(2);

// 2. Hochwasserrisiko für jede Anlage bewerten
for (const auto& facility : facilities) {
    WaterRiskParams params;
    params.water_level_m = 185.0;  // HQ100 Wasserpegel
    
    auto risk = assessor.assessFloodRisk(
        facility.location,
        params,
        100
    );
    
    if (risk.severity >= RiskSeverity::HIGH) {
        // Hohe Gefährdung: Sofortmaßnahmen
        std::cout << "High flood risk: " << facility.facility_id << std::endl;
        
        // Export nach ArcGIS für Visualisierung
        exportToArcGIS(facility, risk);
    }
}
```

### Beispiel 2: Seveso-III Domino-Analyse

```cpp
// 1. Alle Seveso-Anlagen finden
auto seveso_facilities = query_builder.findSevesoFacilitiesNearby(
    incident_location,
    10.0  // 10 km Radius
);

// 2. Dominoeffekt simulieren
DominoEffectParams domino;
domino.primary_facility = incident_location;
domino.nearby_facilities = seveso_facilities;
domino.blast_overpressure_kpa = 100.0;

auto result = assessor.assessDominoEffect(domino);

// 3. Betroffene Nachbaranlagen identifizieren
for (const auto& facility_id : result.affected_entity_ids) {
    std::cout << "Affected: " << facility_id << std::endl;
    
    // Weitere Kaskadenanalyse
    // ...
}
```

### Beispiel 3: Multi-Hazard Assessment

```cpp
// Kombinierte Risikobewertung für Region
std::vector<EnvironmentalRiskType> risks = {
    EnvironmentalRiskType::FLOOD,
    EnvironmentalRiskType::EARTHQUAKE,
    EnvironmentalRiskType::INDUSTRIAL_HAZMAT
};

auto combined_risk = assessor.assessMultiHazardRisk(region, risks);

std::cout << "Combined risk score: " << combined_risk.risk_score << std::endl;
std::cout << "Dominant risk: " << getDominantRisk(combined_risk) << std::endl;
```

## Zusammenfassung

ThemisDB bietet:

✅ **20+ Umweltrisiken** (WHG, BBodSchG, BImSchG, etc.)
✅ **15+ Anlagenrisiken** (Seveso-III, AwSV, KRITIS, etc.)
✅ **Vollständige 3D-Unterstützung** für Höhenabhängige Risiken
✅ **Deutsche Vorschriften** (12. BImSchV, WHG, BBodSchG, etc.)
✅ **ArcGIS Integration** für Visualisierung
✅ **FEM-basierte Kaskadenanalyse**
✅ **Automatische Compliance-Prüfung**
