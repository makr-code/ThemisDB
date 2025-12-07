/**
 * @file arcgis_data_provider.h
 * @brief ArcGIS Data Provider Interface (Enterprise DLL)
 * 
 * This module provides a data provider interface that allows ArcGIS to consume
 * geospatial data from ThemisDB. The provider exposes ThemisDB as a data source
 * for ArcGIS applications.
 * 
 * ## Architecture
 * - ThemisDB acts as data provider/source
 * - ArcGIS consumes data via standardized interfaces
 * - Support for 3D geometries (Point(x,y,z))
 * - Integration with FEM metadata for risk assessment
 * 
 * ## Use Cases
 * - Environmental risk assessment (floods, drought)
 * - Facility incident cascade effects (12. BImSchV)
 * - Real-time geospatial data streaming to ArcGIS
 * - Multi-model data integration (graph + geo + time-series)
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "utils/geo/ewkb.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace geo {
namespace arcgis {

/**
 * @brief Geometry format types supported for ArcGIS export
 */
enum class ExportFormat {
    EWKB,           // Extended Well-Known Binary (PostGIS)
    WKB,            // Well-Known Binary (OGC)
    WKT,            // Well-Known Text
    GEOJSON,        // GeoJSON
    ESRI_JSON,      // Esri JSON (ArcGIS native format)
    SHAPEFILE,      // Shapefile format
    GEOPACKAGE      // OGC GeoPackage
};

/**
 * @brief Feature attributes for ArcGIS
 */
struct FeatureAttributes {
    std::string object_id;              // Unique object ID
    nlohmann::json properties;          // All feature properties
    
    // FEM metadata (if available)
    std::optional<nlohmann::json> fem_metadata;
    
    // Temporal attributes
    std::optional<std::string> timestamp;
    std::optional<std::string> valid_from;
    std::optional<std::string> valid_to;
    
    // Source metadata
    std::string source_table;
    std::string source_model;  // "relational", "graph", "timeseries", "content"
};

/**
 * @brief Spatial feature for ArcGIS consumption
 */
struct SpatialFeature {
    GeometryInfo geometry;              // 3D geometry
    FeatureAttributes attributes;       // Feature attributes
    int srid = 4326;                    // Spatial Reference ID (default: WGS84)
    
    // Serialize to specific format
    std::vector<uint8_t> toBinary(ExportFormat format) const;
    std::string toString(ExportFormat format) const;
    nlohmann::json toEsriJSON() const;
};

/**
 * @brief Spatial query parameters from ArcGIS
 */
struct SpatialQuery {
    // Spatial filter
    std::optional<MBR> bounding_box;
    std::optional<GeometryInfo> filter_geometry;
    std::string spatial_rel = "intersects";  // "intersects", "contains", "within"
    
    // Attribute filter (SQL-like WHERE clause)
    std::string where_clause;
    
    // Temporal filter
    std::optional<std::string> time_start;
    std::optional<std::string> time_end;
    
    // 3D filter
    std::optional<double> z_min;
    std::optional<double> z_max;
    
    // Pagination
    int offset = 0;
    int limit = 1000;
    
    // Output format
    ExportFormat format = ExportFormat::ESRI_JSON;
    int out_srid = 4326;  // Output spatial reference
    
    // Fields to return
    std::vector<std::string> return_fields;  // Empty = all fields
    bool return_geometry = true;
    bool return_fem_metadata = false;
};

/**
 * @brief Query result for ArcGIS
 */
struct QueryResult {
    std::vector<SpatialFeature> features;
    size_t total_count;         // Total matching features (before limit)
    MBR extent;                 // Bounding box of result set
    int srid;                   // Spatial reference of results
    
    // Statistics
    double query_time_ms;
    std::string timestamp;
};

/**
 * @brief Layer metadata for ArcGIS layer discovery
 */
struct LayerMetadata {
    std::string layer_id;
    std::string name;
    std::string description;
    std::string geometry_type;  // "Point", "LineString", "Polygon", etc.
    MBR extent;                 // Layer extent
    int srid;
    bool has_z = false;         // 3D support
    bool has_m = false;         // Measure support
    
    // Field definitions
    std::vector<FieldDefinition> fields;
    
    // FEM integration
    bool has_fem_metadata = false;
    
    // Temporal support
    bool is_temporal = false;
    std::optional<std::string> time_field;
    
    // Source info
    std::string source_table;
    std::string source_model;
    
    nlohmann::json toJSON() const;
};

/**
 * @brief Field definition for layer schema
 */
struct FieldDefinition {
    std::string name;
    std::string type;       // "string", "integer", "double", "date", "geometry"
    std::string alias;      // Display name
    bool nullable = true;
    int length = 0;         // For string fields
    
    nlohmann::json toJSON() const;
};

/**
 * @brief ArcGIS Data Provider Interface
 * 
 * This interface is implemented by ThemisDB and exposed via DLL for ArcGIS
 * to consume geospatial data.
 */
class IArcGISDataProvider {
public:
    virtual ~IArcGISDataProvider() = default;
    
    /**
     * @brief Get provider name
     */
    virtual const char* getName() const noexcept = 0;
    
    /**
     * @brief Get provider version
     */
    virtual const char* getVersion() const noexcept = 0;
    
    /**
     * @brief Initialize provider with connection string
     * @param connection_string Connection string (e.g., "host=localhost;port=8080;db=mydb")
     * @return true if connection successful
     */
    virtual bool connect(const std::string& connection_string) = 0;
    
    /**
     * @brief Disconnect from data source
     */
    virtual void disconnect() = 0;
    
    /**
     * @brief Check if connected
     */
    virtual bool isConnected() const noexcept = 0;
    
    // ========================================================================
    // Layer Discovery
    // ========================================================================
    
    /**
     * @brief Get all available spatial layers
     * @return Vector of layer metadata
     */
    virtual std::vector<LayerMetadata> getLayers() const = 0;
    
    /**
     * @brief Get metadata for specific layer
     * @param layer_id Layer identifier (table name)
     * @return Layer metadata or nullopt if not found
     */
    virtual std::optional<LayerMetadata> getLayerMetadata(const std::string& layer_id) const = 0;
    
    /**
     * @brief Get layer extent (bounding box)
     * @param layer_id Layer identifier
     * @return Extent or nullopt if layer not found
     */
    virtual std::optional<MBR> getLayerExtent(const std::string& layer_id) const = 0;
    
    /**
     * @brief Get feature count for layer
     * @param layer_id Layer identifier
     * @param where_clause Optional WHERE clause filter
     * @return Feature count
     */
    virtual size_t getFeatureCount(
        const std::string& layer_id,
        const std::string& where_clause = ""
    ) const = 0;
    
    // ========================================================================
    // Data Query
    // ========================================================================
    
    /**
     * @brief Query features from layer
     * @param layer_id Layer identifier
     * @param query Query parameters
     * @return Query result
     */
    virtual QueryResult queryFeatures(
        const std::string& layer_id,
        const SpatialQuery& query
    ) const = 0;
    
    /**
     * @brief Get feature by object ID
     * @param layer_id Layer identifier
     * @param object_id Object ID
     * @param format Output format
     * @return Feature or nullopt if not found
     */
    virtual std::optional<SpatialFeature> getFeatureById(
        const std::string& layer_id,
        const std::string& object_id,
        ExportFormat format = ExportFormat::ESRI_JSON
    ) const = 0;
    
    /**
     * @brief Stream features (for large datasets)
     * @param layer_id Layer identifier
     * @param query Query parameters
     * @param callback Callback function for each feature batch
     */
    virtual void streamFeatures(
        const std::string& layer_id,
        const SpatialQuery& query,
        std::function<void(const std::vector<SpatialFeature>&)> callback
    ) const = 0;
    
    // ========================================================================
    // 3D Support
    // ========================================================================
    
    /**
     * @brief Check if layer supports 3D (Z coordinates)
     * @param layer_id Layer identifier
     * @return true if layer has Z coordinates
     */
    virtual bool supportsZ(const std::string& layer_id) const = 0;
    
    /**
     * @brief Get Z range for layer
     * @param layer_id Layer identifier
     * @return Min/Max Z values or nullopt
     */
    virtual std::optional<std::pair<double, double>> getZRange(
        const std::string& layer_id
    ) const = 0;
    
    // ========================================================================
    // FEM Metadata Integration
    // ========================================================================
    
    /**
     * @brief Check if layer has FEM metadata
     * @param layer_id Layer identifier
     * @return true if FEM metadata available
     */
    virtual bool hasFEMMetadata(const std::string& layer_id) const = 0;
    
    /**
     * @brief Get FEM metadata for feature
     * @param layer_id Layer identifier
     * @param object_id Object ID
     * @return FEM metadata or nullopt
     */
    virtual std::optional<nlohmann::json> getFEMMetadata(
        const std::string& layer_id,
        const std::string& object_id
    ) const = 0;
    
    // ========================================================================
    // Risk Assessment Data (for environmental analysis)
    // ========================================================================
    
    /**
     * @brief Get risk assessment data for area
     * @param layer_id Layer identifier (e.g., "flood_zones", "facilities")
     * @param risk_type Risk type ("flood", "drought", "cascade")
     * @param area_geom Area of interest
     * @return Risk features
     */
    virtual std::vector<SpatialFeature> getRiskData(
        const std::string& layer_id,
        const std::string& risk_type,
        const GeometryInfo& area_geom
    ) const = 0;
    
    // ========================================================================
    // Temporal Support
    // ========================================================================
    
    /**
     * @brief Query features by time range
     * @param layer_id Layer identifier
     * @param start_time Start timestamp (ISO 8601)
     * @param end_time End timestamp (ISO 8601)
     * @param spatial_query Optional spatial filter
     * @return Query result
     */
    virtual QueryResult queryByTime(
        const std::string& layer_id,
        const std::string& start_time,
        const std::string& end_time,
        const SpatialQuery& spatial_query = {}
    ) const = 0;
    
    // ========================================================================
    // Multi-Model Integration
    // ========================================================================
    
    /**
     * @brief Get related features from other models
     * 
     * For example, get graph relationships or time-series data
     * related to a spatial feature.
     * 
     * @param layer_id Layer identifier
     * @param object_id Object ID
     * @param relation_type Relation type ("graph", "timeseries", "content")
     * @return Related features as JSON
     */
    virtual nlohmann::json getRelatedFeatures(
        const std::string& layer_id,
        const std::string& object_id,
        const std::string& relation_type
    ) const = 0;
    
    // ========================================================================
    // Capabilities
    // ========================================================================
    
    /**
     * @brief Get provider capabilities
     * @return JSON describing supported features
     */
    virtual nlohmann::json getCapabilities() const = 0;
};

/**
 * @brief ArcGIS Data Provider Factory
 * 
 * Factory for creating provider instances.
 */
class ArcGISDataProviderFactory {
public:
    /**
     * @brief Create provider instance
     * @return Provider instance
     */
    static std::unique_ptr<IArcGISDataProvider> createProvider();
    
    /**
     * @brief Get singleton instance
     */
    static ArcGISDataProviderFactory& instance();
};

} // namespace arcgis
} // namespace geo
} // namespace themis

// ============================================================================
// DLL Export Macros (for Enterprise DLL)
// ============================================================================

#ifdef _WIN32
    #ifdef THEMIS_ARCGIS_PROVIDER_EXPORTS
        #define ARCGIS_PROVIDER_API __declspec(dllexport)
    #else
        #define ARCGIS_PROVIDER_API __declspec(dllimport)
    #endif
#else
    #define ARCGIS_PROVIDER_API __attribute__((visibility("default")))
#endif

extern "C" {
    /**
     * @brief DLL entry point - Create provider instance
     * @return Provider instance
     */
    ARCGIS_PROVIDER_API themis::geo::arcgis::IArcGISDataProvider* CreateArcGISDataProvider();
    
    /**
     * @brief Destroy provider instance
     * @param provider Provider to destroy
     */
    ARCGIS_PROVIDER_API void DestroyArcGISDataProvider(themis::geo::arcgis::IArcGISDataProvider* provider);
    
    /**
     * @brief Get provider version
     * @return Version string
     */
    ARCGIS_PROVIDER_API const char* GetProviderVersion();
    
    /**
     * @brief Get provider capabilities
     * @return JSON string describing capabilities
     */
    ARCGIS_PROVIDER_API const char* GetProviderCapabilities();
}
