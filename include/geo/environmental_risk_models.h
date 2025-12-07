/**
 * @file environmental_risk_models.h
 * @brief Environmental Risk Assessment Models for Geospatial Analysis
 * 
 * This module provides comprehensive environmental risk assessment models
 * including climate risks, natural hazards, and regulatory compliance (e.g., WHG).
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "utils/geo/ewkb.h"
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace geo {
namespace risk {

/**
 * @brief Environmental Risk Categories (German Environmental Regulations)
 */
enum class EnvironmentalRiskType {
    // Water-related Risks (WHG - Wasserhaushaltsgesetz)
    FLOOD,                      // Hochwasser
    GROUNDWATER_CONTAMINATION,  // Grundwasserverschmutzung
    SURFACE_WATER_POLLUTION,    // Oberflächenwasserverschmutzung
    WATER_SCARCITY,             // Wasserknappheit
    DROUGHT,                    // Dürre
    
    // Soil-related Risks (BBodSchG - Bundes-Bodenschutzgesetz)
    SOIL_CONTAMINATION,         // Bodenkontamination
    SOIL_EROSION,              // Bodenerosion
    LANDSLIDE,                 // Erdrutsch
    SUBSIDENCE,                // Bodensenkung
    
    // Air Quality (BImSchG - Bundes-Immissionsschutzgesetz)
    AIR_POLLUTION,             // Luftverschmutzung
    SMOG,                      // Smog
    ACID_RAIN,                 // Saurer Regen
    
    // Climate Risks
    HEAT_WAVE,                 // Hitzewelle
    EXTREME_COLD,              // Extremkälte
    STORM,                     // Sturm
    TORNADO,                   // Tornado
    HAIL,                      // Hagel
    LIGHTNING,                 // Blitzschlag
    
    // Seismic Risks
    EARTHQUAKE,                // Erdbeben
    VOLCANIC_ACTIVITY,         // Vulkanaktivität
    
    // Forest/Vegetation Risks
    FOREST_FIRE,               // Waldbrand
    AVALANCHE,                 // Lawine
    
    // Industrial/Regulatory
    SEVESO_INCIDENT,           // Störfall nach Seveso-III-Richtlinie
    HAZMAT_SPILL,              // Gefahrstoffaustritt
    NUCLEAR_INCIDENT,          // Nuklearvorfall
    
    // Combined/Cascade
    CASCADE_EFFECT,            // Kaskadeneffekt
    MULTI_HAZARD               // Multi-Gefährdung
};

/**
 * @brief Risk Severity Levels (VDI 3780)
 */
enum class RiskSeverity {
    NEGLIGIBLE,     // Vernachlässigbar
    LOW,            // Gering
    MODERATE,       // Mäßig
    HIGH,           // Hoch
    VERY_HIGH,      // Sehr hoch
    EXTREME         // Extrem
};

/**
 * @brief Risk Probability Classes
 */
enum class RiskProbability {
    RARE,           // Selten (< 1% pro Jahr)
    UNLIKELY,       // Unwahrscheinlich (1-10%)
    POSSIBLE,       // Möglich (10-50%)
    LIKELY,         // Wahrscheinlich (50-90%)
    ALMOST_CERTAIN  // Fast sicher (> 90%)
};

/**
 * @brief Environmental Risk Assessment Result
 */
struct EnvironmentalRiskResult {
    EnvironmentalRiskType risk_type;
    RiskSeverity severity;
    RiskProbability probability;
    
    double risk_score;              // 0.0 - 1.0 (calculated from severity × probability)
    double affected_area_sqkm;      // Betroffene Fläche
    double estimated_damage_eur;    // Geschätzter Schaden in EUR
    
    // Spatial extent
    MBR affected_bbox;              // 2D bounding box
    std::optional<double> z_min;    // Min elevation affected
    std::optional<double> z_max;    // Max elevation affected
    
    // Temporal
    std::string assessment_time;    // ISO 8601 timestamp
    std::optional<std::string> forecast_valid_until;
    
    // Regulatory
    std::vector<std::string> violated_regulations;  // e.g., "WHG §62", "BImSchG §3"
    bool requires_notification;     // Meldepflicht
    bool requires_immediate_action; // Sofortmaßnahmen erforderlich
    
    // Affected entities
    std::vector<std::string> affected_entity_ids;
    size_t population_at_risk;      // Betroffene Bevölkerung
    
    // Detailed information
    nlohmann::json risk_factors;    // Risk-specific parameters
    nlohmann::json mitigation_measures; // Empfohlene Maßnahmen
    std::string description;
};

/**
 * @brief Water-related Risk Parameters (WHG)
 */
struct WaterRiskParams {
    double water_level_m;           // Wasserpegel (m über NN)
    double flow_rate_m3s;           // Durchfluss (m³/s)
    double precipitation_mm;        // Niederschlag (mm)
    double groundwater_depth_m;     // Grundwasserspiegel (m unter Gelände)
    double contamination_ppm;       // Schadstoffkonzentration (ppm)
    
    // Water protection zones (WHG Wasserschutzgebiete)
    std::optional<int> protection_zone; // 1, 2, 3, or null
};

/**
 * @brief Soil Risk Parameters (BBodSchG)
 */
struct SoilRiskParams {
    double contamination_mg_kg;     // Schadstoffkonzentration (mg/kg)
    double erosion_rate_t_ha_year;  // Erosionsrate (t/ha/Jahr)
    double slope_degrees;           // Hangneigung (Grad)
    double soil_moisture_percent;   // Bodenfeuchte (%)
    std::string soil_type;          // Bodenart
    double organic_matter_percent;  // Organische Substanz (%)
};

/**
 * @brief Air Quality Parameters (BImSchG, 39. BImSchV)
 */
struct AirQualityParams {
    double pm10_ug_m3;              // Feinstaub PM10 (µg/m³)
    double pm25_ug_m3;              // Feinstaub PM2.5 (µg/m³)
    double no2_ug_m3;               // Stickstoffdioxid (µg/m³)
    double o3_ug_m3;                // Ozon (µg/m³)
    double so2_ug_m3;               // Schwefeldioxid (µg/m³)
    double co_mg_m3;                // Kohlenmonoxid (mg/m³)
    
    // Grenzwerte nach 39. BImSchV
    bool exceeds_limits;
    std::vector<std::string> exceeded_pollutants;
};

/**
 * @brief Climate Risk Parameters
 */
struct ClimateRiskParams {
    double temperature_c;           // Temperatur (°C)
    double heat_index_c;            // Hitzeindex (°C)
    double wind_speed_ms;           // Windgeschwindigkeit (m/s)
    double precipitation_mm_h;      // Niederschlagsintensität (mm/h)
    double snow_load_kg_m2;         // Schneelast (kg/m²)
    int lightning_strikes_per_km2;  // Blitzeinschläge pro km²
};

/**
 * @brief Seismic Risk Parameters
 */
struct SeismicRiskParams {
    double magnitude;               // Magnitude (Richter-Skala)
    double peak_ground_acceleration_g; // Spitzenbeschleunigung (g)
    double epicenter_distance_km;   // Entfernung zum Epizentrum (km)
    int intensity_ems98;            // EMS-98 Intensität (I-XII)
};

/**
 * @brief Industrial Hazard Parameters (12. BImSchV, Seveso-III)
 */
struct IndustrialHazardParams {
    std::string hazard_type;        // "explosion", "toxic_release", "fire"
    double release_quantity_kg;     // Freigesetzte Menge (kg)
    std::string substance_name;     // Gefahrstoff
    std::string un_number;          // UN-Nummer
    double dispersion_radius_m;     // Ausbreitungsradius (m)
    
    // Seveso-III Classification
    bool seveso_upper_tier;         // Betriebsbereich obere Klasse
    std::vector<std::string> hazard_categories; // H-Sätze
};

/**
 * @brief Environmental Risk Assessor
 * 
 * Provides comprehensive environmental risk assessment methods
 * compliant with German environmental regulations.
 */
class EnvironmentalRiskAssessor {
public:
    EnvironmentalRiskAssessor() = default;
    ~EnvironmentalRiskAssessor() = default;
    
    // ========================================================================
    // Water Risks (WHG - Wasserhaushaltsgesetz)
    // ========================================================================
    
    /**
     * @brief Assess flood risk (HQ10, HQ100, HQ200)
     * 
     * @param area Area to assess
     * @param params Water risk parameters
     * @param return_period Return period in years (10, 100, 200)
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessFloodRisk(
        const GeometryInfo& area,
        const WaterRiskParams& params,
        int return_period = 100
    );
    
    /**
     * @brief Assess groundwater contamination risk (WHG §48)
     * 
     * @param point Location of potential contamination source
     * @param params Water risk parameters
     * @param protection_zone Water protection zone (1, 2, 3)
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessGroundwaterContaminationRisk(
        const GeometryInfo& point,
        const WaterRiskParams& params,
        int protection_zone = 2
    );
    
    /**
     * @brief Assess drought risk (meteorological, hydrological, agricultural)
     * 
     * @param area Area to assess
     * @param params Water risk parameters
     * @param duration_days Drought duration in days
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessDroughtRisk(
        const GeometryInfo& area,
        const WaterRiskParams& params,
        int duration_days = 30
    );
    
    // ========================================================================
    // Soil Risks (BBodSchG - Bundes-Bodenschutzgesetz)
    // ========================================================================
    
    /**
     * @brief Assess soil contamination risk (BBodSchG §4)
     * 
     * @param area Contaminated or potentially contaminated area
     * @param params Soil risk parameters
     * @param land_use Land use type ("residential", "industrial", "agricultural")
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessSoilContaminationRisk(
        const GeometryInfo& area,
        const SoilRiskParams& params,
        const std::string& land_use = "residential"
    );
    
    /**
     * @brief Assess landslide risk (3D terrain analysis)
     * 
     * @param terrain 3D terrain points
     * @param params Soil parameters
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessLandslideRisk(
        const std::vector<Coordinate>& terrain,
        const SoilRiskParams& params
    );
    
    /**
     * @brief Assess soil erosion risk (DIN 19708)
     * 
     * @param area Agricultural or exposed area
     * @param params Soil parameters
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessSoilErosionRisk(
        const GeometryInfo& area,
        const SoilRiskParams& params
    );
    
    // ========================================================================
    // Air Quality Risks (BImSchG, 39. BImSchV, TA Luft)
    // ========================================================================
    
    /**
     * @brief Assess air pollution risk (39. BImSchV)
     * 
     * @param area Area to assess
     * @param params Air quality parameters
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessAirPollutionRisk(
        const GeometryInfo& area,
        const AirQualityParams& params
    );
    
    /**
     * @brief Assess smog risk
     * 
     * @param area Urban area
     * @param params Air quality parameters
     * @param temperature Temperature in Celsius
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessSmogRisk(
        const GeometryInfo& area,
        const AirQualityParams& params,
        double temperature
    );
    
    // ========================================================================
    // Climate Risks
    // ========================================================================
    
    /**
     * @brief Assess heat wave risk (DWD criteria)
     * 
     * @param area Area to assess
     * @param params Climate parameters
     * @param consecutive_days Days with T > 30°C
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessHeatWaveRisk(
        const GeometryInfo& area,
        const ClimateRiskParams& params,
        int consecutive_days = 3
    );
    
    /**
     * @brief Assess storm risk (Beaufort scale, DWD warnings)
     * 
     * @param area Area to assess
     * @param params Climate parameters
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessStormRisk(
        const GeometryInfo& area,
        const ClimateRiskParams& params
    );
    
    /**
     * @brief Assess forest fire risk (Canadian FWI)
     * 
     * @param area Forest area
     * @param params Climate parameters
     * @param vegetation_dryness Vegetation dryness index (0-1)
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessForestFireRisk(
        const GeometryInfo& area,
        const ClimateRiskParams& params,
        double vegetation_dryness
    );
    
    // ========================================================================
    // Seismic Risks
    // ========================================================================
    
    /**
     * @brief Assess earthquake risk (DIN EN 1998-1, DIN 4149)
     * 
     * @param epicenter Earthquake epicenter (3D with depth)
     * @param params Seismic parameters
     * @param buildings_in_area Buildings to assess
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessEarthquakeRisk(
        const Coordinate& epicenter,
        const SeismicRiskParams& params,
        const std::vector<GeometryInfo>& buildings_in_area
    );
    
    // ========================================================================
    // Industrial Hazards (12. BImSchV, Seveso-III)
    // ========================================================================
    
    /**
     * @brief Assess industrial hazmat incident risk (12. BImSchV)
     * 
     * @param incident_location Incident location (3D with elevation)
     * @param params Industrial hazard parameters
     * @param nearby_facilities Nearby facilities
     * @param wind_direction Wind direction in degrees (0-360)
     * @param wind_speed Wind speed in m/s
     * @return Risk assessment result
     */
    EnvironmentalRiskResult assessIndustrialHazmatRisk(
        const Coordinate& incident_location,
        const IndustrialHazardParams& params,
        const std::vector<GeometryInfo>& nearby_facilities,
        double wind_direction = 0.0,
        double wind_speed = 5.0
    );
    
    /**
     * @brief Assess Seveso-III cascade effect (anlagenbezogene Störfälle)
     * 
     * @param primary_incident Primary incident location
     * @param seveso_facilities Nearby Seveso facilities
     * @param params Hazard parameters
     * @return Risk assessment result with cascade effects
     */
    EnvironmentalRiskResult assessSevesoIIICascadeRisk(
        const Coordinate& primary_incident,
        const std::vector<GeometryInfo>& seveso_facilities,
        const IndustrialHazardParams& params
    );
    
    // ========================================================================
    // Multi-Hazard Assessment
    // ========================================================================
    
    /**
     * @brief Assess combined/cascading environmental risks
     * 
     * @param area Area to assess
     * @param risk_types Risk types to consider
     * @return Combined risk assessment
     */
    EnvironmentalRiskResult assessMultiHazardRisk(
        const GeometryInfo& area,
        const std::vector<EnvironmentalRiskType>& risk_types
    );
    
    // ========================================================================
    // Regulatory Compliance
    // ========================================================================
    
    /**
     * @brief Check compliance with German environmental regulations
     * 
     * @param risk_result Risk assessment result
     * @return Violated regulations and required actions
     */
    nlohmann::json checkRegulatoryCompliance(
        const EnvironmentalRiskResult& risk_result
    );
    
    /**
     * @brief Generate risk report (VDI 3780 format)
     * 
     * @param risk_result Risk assessment result
     * @param report_format Format ("json", "pdf", "html")
     * @return Risk report
     */
    std::string generateRiskReport(
        const EnvironmentalRiskResult& risk_result,
        const std::string& report_format = "json"
    );
    
private:
    // Helper methods
    RiskSeverity calculateSeverity(double impact_score);
    RiskProbability calculateProbability(double frequency);
    double calculateRiskScore(RiskSeverity severity, RiskProbability probability);
    
    // Regulatory thresholds
    double getThresholdValue(const std::string& regulation, const std::string& parameter);
    bool exceedsThreshold(double value, const std::string& regulation, const std::string& parameter);
};

/**
 * @brief Risk Matrix (VDI 3780)
 * 
 * Maps severity and probability to risk levels.
 */
class RiskMatrix {
public:
    static RiskSeverity getRiskLevel(RiskSeverity severity, RiskProbability probability);
    static std::string getRiskColor(RiskSeverity severity);
    static std::string getRiskLevelDescription(RiskSeverity severity);
};

} // namespace risk
} // namespace geo
} // namespace themis
