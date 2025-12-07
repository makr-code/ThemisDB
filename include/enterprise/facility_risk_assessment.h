/**
 * @file facility_risk_assessment.h
 * @brief Facility-Specific Risk Assessment Models (Enterprise Feature)
 * 
 * Comprehensive risk assessment for industrial facilities, critical infrastructure,
 * and regulatory compliance (12. BImSchV, Seveso-III, WHG, AwSV, etc.).
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "utils/geo/ewkb.h"
#include "enterprise/environmental_risk_models.h"
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace geo {
namespace facility {

/**
 * @brief Facility Types (German Industrial Classification)
 */
enum class FacilityType {
    // Chemical/Petrochemical (12. BImSchV, Seveso-III)
    CHEMICAL_PLANT,                 // Chemische Anlage
    REFINERY,                       // Raffinerie
    PETROCHEMICAL_PLANT,           // Petrochemische Anlage
    PHARMACEUTICAL_PLANT,          // Pharmazeutische Anlage
    
    // Energy (EnWG - Energiewirtschaftsgesetz)
    POWER_PLANT_NUCLEAR,           // Kernkraftwerk
    POWER_PLANT_COAL,              // Kohlekraftwerk
    POWER_PLANT_GAS,               // Gaskraftwerk
    POWER_PLANT_HYDRO,             // Wasserkraftwerk
    POWER_PLANT_WIND,              // Windkraftanlage
    POWER_PLANT_SOLAR,             // Solarkraftwerk
    SUBSTATION_ELECTRICAL,         // Umspannwerk
    GAS_STORAGE,                   // Gasspeicher
    
    // Water/Wastewater (WHG, AbwV)
    WATER_TREATMENT_PLANT,         // Wasserwerk
    WASTEWATER_TREATMENT_PLANT,    // Kläranlage
    DRINKING_WATER_RESERVOIR,      // Trinkwasserbehälter
    RAINWATER_RETENTION_BASIN,     // Regenrückhaltebecken
    
    // Waste Management (KrWG, DepV)
    WASTE_INCINERATION_PLANT,      // Müllverbrennungsanlage
    LANDFILL,                      // Deponie
    RECYCLING_FACILITY,            // Recyclinganlage
    HAZARDOUS_WASTE_FACILITY,      // Sonderabfallanlage
    
    // Storage Facilities (AwSV, VAwS)
    FUEL_STORAGE_DEPOT,            // Tanklager
    CHEMICAL_STORAGE_WAREHOUSE,    // Chemikalienlager
    HAZMAT_STORAGE,                // Gefahrstofflager
    LPG_STORAGE,                   // Flüssiggaslager
    AMMONIA_STORAGE,               // Ammoniaklager
    
    // Critical Infrastructure (KRITIS)
    HOSPITAL,                      // Krankenhaus
    EMERGENCY_SERVICES,            // Rettungsdienst
    FIRE_STATION,                  // Feuerwehr
    POLICE_STATION,                // Polizeistation
    TELECOM_SWITCHING_CENTER,      // Telekommunikationsvermittlung
    DATA_CENTER,                   // Rechenzentrum
    
    // Transport Infrastructure
    AIRPORT,                       // Flughafen
    RAILWAY_STATION,               // Bahnhof
    HARBOR,                        // Hafen
    PIPELINE_STATION,              // Pipelinestation
    
    // Industrial Production
    STEEL_MILL,                    // Stahlwerk
    CEMENT_PLANT,                  // Zementwerk
    PAPER_MILL,                    // Papierfabrik
    FOOD_PROCESSING_PLANT,         // Lebensmittelverarbeitungsanlage
    
    // Other
    MINING_FACILITY,               // Bergbauanlage
    CONSTRUCTION_SITE,             // Baustelle
    GENERIC_INDUSTRIAL             // Sonstige Industrieanlage
};

/**
 * @brief Facility Hazard Classes (based on 12. BImSchV)
 */
enum class HazardClass {
    CLASS_I,        // Sehr hohe Gefahr (Seveso upper-tier)
    CLASS_II,       // Hohe Gefahr (Seveso lower-tier)
    CLASS_III,      // Mittlere Gefahr
    CLASS_IV,       // Geringe Gefahr
    NOT_CLASSIFIED  // Nicht eingestuft
};

/**
 * @brief Safety Management Systems
 */
enum class SafetyManagementLevel {
    NONE,               // Kein SMS
    BASIC,              // Basis-SMS
    ADVANCED,           // Erweitertes SMS
    ISO_45001,          // ISO 45001 zertifiziert
    SEVESO_COMPLIANT    // Seveso-III konform
};

/**
 * @brief Facility Risk Assessment Result
 */
struct FacilityRiskResult {
    std::string facility_id;
    FacilityType facility_type;
    HazardClass hazard_class;
    
    // Overall risk
    double overall_risk_score;      // 0.0 - 1.0
    risk::RiskSeverity severity;
    risk::RiskProbability probability;
    
    // Specific risks
    double fire_explosion_risk;     // Brandgefahr
    double toxic_release_risk;      // Freisetzung toxischer Stoffe
    double structural_failure_risk; // Konstruktionsversagen
    double environmental_impact_risk; // Umweltauswirkungen
    double cascade_risk;            // Dominoeffekt
    
    // Regulatory compliance
    std::vector<std::string> applicable_regulations;
    std::vector<std::string> violations;
    bool requires_emergency_plan;   // Alarm- und Gefahrenabwehrplan erforderlich
    bool requires_safety_report;    // Sicherheitsbericht erforderlich
    
    // Safety distances (Abstandsregelung)
    double required_safety_distance_m;
    std::vector<std::string> conflicting_neighbors; // Zu nahe Nachbarn
    
    // Affected area
    MBR impact_zone;
    std::optional<double> z_min_impact;
    std::optional<double> z_max_impact;
    
    // Recommendations
    std::vector<std::string> immediate_actions;
    std::vector<std::string> preventive_measures;
    nlohmann::json detailed_findings;
};

/**
 * @brief Facility Inventory Data
 */
struct FacilityInventory {
    std::string facility_id;
    FacilityType facility_type;
    GeometryInfo location;          // 3D location with elevation
    
    // Classification
    HazardClass hazard_class;
    bool seveso_facility;
    bool seveso_upper_tier;
    
    // Capacities
    double storage_capacity_m3;     // Lagerkapazität
    double production_capacity_t_year; // Produktionskapazität
    
    // Hazardous substances (AwSV, Seveso-III Anhang I)
    struct HazardousSubstance {
        std::string name;
        std::string cas_number;
        double quantity_kg;
        std::vector<std::string> hazard_statements; // H-Sätze
        std::string storage_class;  // LGK (Lagerklasse)
        double water_hazard_class;  // WGK (Wassergefährdungsklasse)
    };
    std::vector<HazardousSubstance> substances;
    
    // Infrastructure
    bool has_fire_suppression;      // Brandschutzanlage
    bool has_containment_system;    // Rückhaltesystem
    bool has_emergency_shutdown;    // NOT-AUS System
    bool has_gas_detection;         // Gaswarnanlage
    SafetyManagementLevel sms_level;
    
    // Personnel
    int employees_count;
    int night_shift_count;
    
    // Temporal aspects
    std::string commissioned_date;
    std::string last_inspection_date;
    std::string next_inspection_due;
    
    // Documentation
    bool has_emergency_plan;
    bool has_safety_report;
    std::string permit_number;      // Genehmigungsnummer
};

/**
 * @brief Storage Tank Risk Parameters (AwSV, VAwS)
 */
struct StorageTankParams {
    double volume_m3;
    std::string tank_type;          // "above_ground", "underground", "pressure"
    std::string material;           // "steel", "concrete", "plastic"
    int age_years;
    
    // Containment
    bool has_double_wall;
    bool has_leak_detection;
    bool has_overflow_protection;
    double bund_capacity_m3;        // Auffangwannenvolumen
    
    // Inspection status
    std::string last_inspection_date;
    bool inspection_passed;
    std::vector<std::string> defects;
    
    // Contents
    std::string substance_name;
    double water_hazard_class;      // WGK 1-3
    double fill_level_percent;
};

/**
 * @brief Seveso-III Threshold Quantities (Anhang I)
 */
struct SevesoThresholds {
    std::string hazard_category;    // e.g., "H1", "P5a"
    double lower_tier_quantity_t;
    double upper_tier_quantity_t;
    std::string description;
};

/**
 * @brief Domino Effect Parameters (12. BImSchV §3 Abs. 5c)
 */
struct DominoEffectParams {
    GeometryInfo primary_facility;
    std::vector<GeometryInfo> nearby_facilities;
    
    double blast_overpressure_kpa;  // Druckwelle
    double thermal_radiation_kw_m2; // Wärmestrahlung
    double toxic_concentration_ppm; // Schadstoffkonzentration
    
    double wind_speed_ms;
    double wind_direction_deg;
    double atmospheric_stability;   // Pasquill-Gifford class
};

/**
 * @brief Facility Risk Assessor
 * 
 * Comprehensive risk assessment for industrial facilities and critical infrastructure.
 */
class FacilityRiskAssessor {
public:
    FacilityRiskAssessor() = default;
    ~FacilityRiskAssessor() = default;
    
    // ========================================================================
    // General Facility Risk Assessment
    // ========================================================================
    
    /**
     * @brief Comprehensive facility risk assessment
     * 
     * @param facility Facility inventory data
     * @param nearby_facilities Nearby facilities for domino effect
     * @return Risk assessment result
     */
    FacilityRiskResult assessFacilityRisk(
        const FacilityInventory& facility,
        const std::vector<FacilityInventory>& nearby_facilities = {}
    );
    
    /**
     * @brief Quick risk screening (simplified assessment)
     * 
     * @param facility_type Type of facility
     * @param location Location with elevation
     * @param hazard_class Hazard classification
     * @return Preliminary risk score
     */
    double quickRiskScreening(
        FacilityType facility_type,
        const GeometryInfo& location,
        HazardClass hazard_class
    );
    
    // ========================================================================
    // Seveso-III Assessment (Störfall-Verordnung)
    // ========================================================================
    
    /**
     * @brief Check if facility falls under Seveso-III directive
     * 
     * @param substances Hazardous substances inventory
     * @return True if Seveso facility, tier level, and applicable categories
     */
    struct SevesoClassification {
        bool is_seveso_facility;
        bool is_upper_tier;
        std::vector<std::string> applicable_categories;
        std::vector<SevesoThresholds> exceeded_thresholds;
    };
    SevesoClassification classifySevesoFacility(
        const std::vector<FacilityInventory::HazardousSubstance>& substances
    );
    
    /**
     * @brief Assess domino effect risk (12. BImSchV §3 Abs. 5c)
     * 
     * @param params Domino effect parameters
     * @return Risk assessment with cascade effects
     */
    FacilityRiskResult assessDominoEffect(
        const DominoEffectParams& params
    );
    
    /**
     * @brief Calculate safety distances (TA Abstand)
     * 
     * @param facility Facility data
     * @param land_use_type Surrounding land use ("residential", "industrial", "mixed")
     * @return Required safety distance in meters
     */
    double calculateSafetyDistance(
        const FacilityInventory& facility,
        const std::string& land_use_type = "residential"
    );
    
    // ========================================================================
    // Storage Facility Assessment (AwSV, VAwS)
    // ========================================================================
    
    /**
     * @brief Assess storage tank risk (AwSV - Anlagenverordnung wassergefährdende Stoffe)
     * 
     * @param tank Tank parameters
     * @param in_water_protection_zone Is in water protection zone
     * @param protection_zone_level Water protection zone level (1-3)
     * @return Risk assessment result
     */
    FacilityRiskResult assessStorageTankRisk(
        const StorageTankParams& tank,
        bool in_water_protection_zone = false,
        int protection_zone_level = 0
    );
    
    /**
     * @brief Assess pipeline integrity risk
     * 
     * @param pipeline Pipeline geometry (LineString)
     * @param substance Transported substance
     * @param age_years Pipeline age
     * @param pressure_bar Operating pressure
     * @return Risk assessment result
     */
    FacilityRiskResult assessPipelineRisk(
        const GeometryInfo& pipeline,
        const std::string& substance,
        int age_years,
        double pressure_bar
    );
    
    /**
     * @brief Assess containment system adequacy (Rückhaltesystem)
     * 
     * @param storage_volume_m3 Total storage volume
     * @param containment_volume_m3 Containment/bund volume
     * @param water_hazard_class WGK (1-3)
     * @return True if adequate, required volume if not
     */
    struct ContainmentAssessment {
        bool is_adequate;
        double required_volume_m3;
        double provided_volume_m3;
        double deficit_m3;
        std::vector<std::string> violations;
    };
    ContainmentAssessment assessContainmentSystem(
        double storage_volume_m3,
        double containment_volume_m3,
        double water_hazard_class
    );
    
    // ========================================================================
    // Fire and Explosion Risk
    // ========================================================================
    
    /**
     * @brief Assess fire and explosion risk (vfdb-Richtlinien)
     * 
     * @param facility Facility data
     * @param fire_load_mj_m2 Fire load in MJ/m²
     * @param has_ignition_sources Has potential ignition sources
     * @return Risk assessment result
     */
    FacilityRiskResult assessFireExplosionRisk(
        const FacilityInventory& facility,
        double fire_load_mj_m2,
        bool has_ignition_sources = true
    );
    
    /**
     * @brief Calculate explosion impact zone (TNT equivalence method)
     * 
     * @param explosive_mass_kg Mass of explosive/flammable material (kg)
     * @param tnt_equivalence TNT equivalence factor
     * @param overpressure_threshold_kpa Threshold overpressure (default: 20 kPa)
     * @return Impact radius in meters
     */
    double calculateExplosionRadius(
        double explosive_mass_kg,
        double tnt_equivalence = 0.05,
        double overpressure_threshold_kpa = 20.0
    );
    
    /**
     * @brief Calculate BLEVE (Boiling Liquid Expanding Vapor Explosion) impact
     * 
     * @param vessel_volume_m3 Pressure vessel volume
     * @param fill_level_percent Fill level (%)
     * @param storage_pressure_bar Storage pressure
     * @return Impact zones for different effects
     */
    struct BLEVEImpact {
        double fireball_radius_m;
        double thermal_radiation_100percent_lethality_m;
        double thermal_radiation_1percent_lethality_m;
        double blast_overpressure_severe_damage_m;
        double blast_overpressure_glass_breakage_m;
    };
    BLEVEImpact calculateBLEVEImpact(
        double vessel_volume_m3,
        double fill_level_percent,
        double storage_pressure_bar
    );
    
    // ========================================================================
    // Toxic Release Assessment
    // ========================================================================
    
    /**
     * @brief Assess toxic gas dispersion (VDI 3783 Blatt 1/2)
     * 
     * @param release_point Release location (3D with height)
     * @param substance_name Chemical substance
     * @param release_rate_kg_s Release rate (kg/s)
     * @param wind_speed_ms Wind speed (m/s)
     * @param wind_direction_deg Wind direction (degrees)
     * @param atmospheric_stability Pasquill-Gifford class (A-F)
     * @return Concentration contours and affected areas
     */
    struct ToxicDispersionResult {
        std::vector<GeometryInfo> concentration_contours; // Isopleths
        std::vector<double> concentration_levels_ppm;
        MBR affected_area;
        double max_downwind_distance_m;
        size_t population_at_risk;
    };
    ToxicDispersionResult assessToxicDispersion(
        const Coordinate& release_point,
        const std::string& substance_name,
        double release_rate_kg_s,
        double wind_speed_ms,
        double wind_direction_deg,
        char atmospheric_stability = 'D'
    );
    
    /**
     * @brief Calculate AEGL/ERPG exposure levels
     * 
     * @param substance Chemical substance
     * @param concentration_ppm Concentration in ppm
     * @param exposure_time_min Exposure duration in minutes
     * @return Health effect classification
     */
    struct ExposureLevel {
        std::string level;          // "AEGL-1", "AEGL-2", "AEGL-3"
        std::string health_effect;  // Description
        bool life_threatening;
    };
    ExposureLevel calculateExposureLevel(
        const std::string& substance,
        double concentration_ppm,
        double exposure_time_min
    );
    
    // ========================================================================
    // Critical Infrastructure (KRITIS)
    // ========================================================================
    
    /**
     * @brief Assess KRITIS facility resilience (BSI IT-Grundschutz)
     * 
     * @param facility Facility data
     * @param sector KRITIS sector ("energy", "water", "health", "telecom")
     * @return Risk assessment with resilience metrics
     */
    FacilityRiskResult assessKRITISResilience(
        const FacilityInventory& facility,
        const std::string& sector
    );
    
    /**
     * @brief Calculate facility criticality index
     * 
     * @param facility Facility data
     * @param population_served Population served by facility
     * @param alternative_facilities Number of alternative facilities in region
     * @return Criticality score (0.0 - 1.0)
     */
    double calculateCriticalityIndex(
        const FacilityInventory& facility,
        size_t population_served,
        int alternative_facilities
    );
    
    // ========================================================================
    // Structural Integrity
    // ========================================================================
    
    /**
     * @brief Assess structural integrity risk (DIN standards)
     * 
     * @param facility Facility with 3D geometry
     * @param building_age_years Age of structure
     * @param last_inspection_date Last structural inspection
     * @param seismic_zone Seismic zone (0-3)
     * @return Risk assessment result
     */
    FacilityRiskResult assessStructuralIntegrity(
        const FacilityInventory& facility,
        int building_age_years,
        const std::string& last_inspection_date,
        int seismic_zone = 0
    );
    
    // ========================================================================
    // Regulatory Compliance
    // ========================================================================
    
    /**
     * @brief Check compliance with all applicable regulations
     * 
     * @param facility Facility data
     * @return Compliance report with violations
     */
    struct ComplianceReport {
        bool is_compliant;
        std::vector<std::string> applicable_regulations;
        std::vector<std::string> violations;
        std::vector<std::string> required_permits;
        std::vector<std::string> missing_permits;
        std::vector<std::string> overdue_inspections;
        nlohmann::json detailed_findings;
    };
    ComplianceReport checkRegulatoryCompliance(
        const FacilityInventory& facility
    );
    
    /**
     * @brief Generate safety report (Seveso-III Art. 10)
     * 
     * @param facility Facility data
     * @param risk_result Risk assessment result
     * @param format Output format ("json", "pdf", "xml")
     * @return Safety report
     */
    std::string generateSafetyReport(
        const FacilityInventory& facility,
        const FacilityRiskResult& risk_result,
        const std::string& format = "json"
    );
    
    /**
     * @brief Generate emergency response plan (Alarm- und Gefahrenabwehrplan)
     * 
     * @param facility Facility data
     * @param risk_result Risk assessment result
     * @return Emergency response plan
     */
    nlohmann::json generateEmergencyResponsePlan(
        const FacilityInventory& facility,
        const FacilityRiskResult& risk_result
    );
    
private:
    // Helper methods
    double calculateFireLoad(FacilityType type, double volume_m3);
    char determineAtmosphericStability(double wind_speed, double cloud_cover);
    double getSubstanceProperty(const std::string& substance, const std::string& property);
    
    // Regulatory thresholds
    SevesoThresholds getSevesoThreshold(const std::string& hazard_category);
    double getWHGThreshold(double water_hazard_class);
};

/**
 * @brief Facility Database Query Helper
 */
class FacilityQueryBuilder {
public:
    /**
     * @brief Find all Seveso facilities within radius
     */
    std::vector<FacilityInventory> findSevesoFacilitiesNearby(
        const Coordinate& center,
        double radius_km
    );
    
    /**
     * @brief Find facilities in water protection zone
     */
    std::vector<FacilityInventory> findFacilitiesInWaterProtectionZone(
        int zone_level = 2
    );
    
    /**
     * @brief Find critical infrastructure facilities
     */
    std::vector<FacilityInventory> findKRITISFacilities(
        const std::string& sector = ""
    );
};

} // namespace facility
} // namespace geo
} // namespace themis
