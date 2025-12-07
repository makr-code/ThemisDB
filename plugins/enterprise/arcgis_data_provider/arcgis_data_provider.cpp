/**
 * @file arcgis_data_provider.cpp
 * @brief Implementation of ArcGIS Data Provider (Enterprise Plugin)
 */

#include "enterprise/arcgis_data_provider.h"
#include "utils/geo/ewkb.h"
#include "storage/rocksdb_wrapper.h"
#include "index/spatial_index.h"
#include <sstream>

namespace themis {
namespace geo {
namespace arcgis {

// ============================================================================
// Helper Functions
// ============================================================================

nlohmann::json FieldDefinition::toJSON() const {
    return {
        {"name", name},
        {"type", type},
        {"alias", alias},
        {"nullable", nullable},
        {"length", length}
    };
}

nlohmann::json LayerMetadata::toJSON() const {
    nlohmann::json fields_json = nlohmann::json::array();
    for (const auto& field : fields) {
        fields_json.push_back(field.toJSON());
    }
    
    return {
        {"id", layer_id},
        {"name", name},
        {"description", description},
        {"geometryType", geometry_type},
        {"extent", {
            {"xmin", extent.minx},
            {"ymin", extent.miny},
            {"xmax", extent.maxx},
            {"ymax", extent.maxy}
        }},
        {"spatialReference", {{"wkid", srid}}},
        {"hasZ", has_z},
        {"hasM", has_m},
        {"fields", fields_json},
        {"hasFEMMetadata", has_fem_metadata},
        {"isTemporal", is_temporal},
        {"timeField", time_field.value_or("")},
        {"sourceTable", source_table},
        {"sourceModel", source_model}
    };
}

std::vector<uint8_t> SpatialFeature::toBinary(ExportFormat format) const {
    switch (format) {
        case ExportFormat::EWKB:
        case ExportFormat::WKB:
            return EWKBParser::serialize(geometry);
        default:
            throw std::runtime_error("Binary format not supported: " + std::to_string(static_cast<int>(format)));
    }
}

std::string SpatialFeature::toString(ExportFormat format) const {
    switch (format) {
        case ExportFormat::WKT:
            return EWKBParser::toWKT(geometry);
        case ExportFormat::GEOJSON:
            return EWKBParser::toGeoJSON(geometry);
        case ExportFormat::ESRI_JSON:
            return toEsriJSON().dump();
        default:
            throw std::runtime_error("String format not supported");
    }
}

nlohmann::json SpatialFeature::toEsriJSON() const {
    nlohmann::json result;
    
    // Geometry
    nlohmann::json geom_json;
    if (geometry.isPoint()) {
        geom_json["x"] = geometry.coords[0].x;
        geom_json["y"] = geometry.coords[0].y;
        if (geometry.hasZ()) {
            geom_json["z"] = geometry.coords[0].getZ();
        }
        geom_json["spatialReference"] = {{"wkid", srid}};
    } else {
        // For complex geometries, use GeoJSON-like structure
        geom_json = nlohmann::json::parse(EWKBParser::toGeoJSON(geometry));
    }
    
    result["geometry"] = geom_json;
    
    // Attributes
    result["attributes"] = attributes.properties;
    result["attributes"]["OBJECTID"] = attributes.object_id;
    
    if (attributes.timestamp) {
        result["attributes"]["timestamp"] = *attributes.timestamp;
    }
    
    if (attributes.fem_metadata) {
        result["attributes"]["fem_metadata"] = *attributes.fem_metadata;
    }
    
    return result;
}

// ============================================================================
// ThemisDB ArcGIS Data Provider Implementation
// ============================================================================

class ThemisArcGISDataProvider : public IArcGISDataProvider {
private:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<index::SpatialIndexManager> spatial_mgr_;
    bool connected_ = false;
    std::string connection_string_;
    
public:
    ThemisArcGISDataProvider() = default;
    ~ThemisArcGISDataProvider() override {
        disconnect();
    }
    
    const char* getName() const noexcept override {
        return "ThemisDB ArcGIS Data Provider";
    }
    
    const char* getVersion() const noexcept override {
        return "1.0.0";
    }
    
    bool connect(const std::string& connection_string) override {
        if (connected_) {
            disconnect();
        }
        
        connection_string_ = connection_string;
        
        // Parse connection string (simplified)
        // Format: "path=/path/to/db;cache_size=256"
        std::string db_path = "./themisdb_data";
        
        size_t path_pos = connection_string.find("path=");
        if (path_pos != std::string::npos) {
            size_t end_pos = connection_string.find(';', path_pos);
            if (end_pos == std::string::npos) end_pos = connection_string.length();
            db_path = connection_string.substr(path_pos + 5, end_pos - path_pos - 5);
        }
        
        // Open database
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 256;
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            return false;
        }
        
        spatial_mgr_ = std::make_unique<index::SpatialIndexManager>(*db_);
        connected_ = true;
        return true;
    }
    
    void disconnect() override {
        if (connected_) {
            spatial_mgr_.reset();
            db_.reset();
            connected_ = false;
        }
    }
    
    bool isConnected() const noexcept override {
        return connected_;
    }
    
    std::vector<LayerMetadata> getLayers() const override {
        if (!connected_) {
            throw std::runtime_error("Not connected to database");
        }
        
        std::vector<LayerMetadata> layers;
        
        // Scan database for tables with spatial indexes
        // This is a simplified implementation
        // In production, would query metadata tables
        
        LayerMetadata sample_layer;
        sample_layer.layer_id = "spatial_features";
        sample_layer.name = "Spatial Features";
        sample_layer.description = "Sample spatial features from ThemisDB";
        sample_layer.geometry_type = "Point";
        sample_layer.extent = MBR(-180, -90, 180, 90);
        sample_layer.srid = 4326;
        sample_layer.has_z = true;
        sample_layer.has_m = false;
        sample_layer.source_table = "spatial_features";
        sample_layer.source_model = "relational";
        
        // Add standard fields
        sample_layer.fields = {
            {"OBJECTID", "string", "Object ID", false, 256},
            {"name", "string", "Name", true, 256},
            {"description", "string", "Description", true, 1024},
            {"elevation", "double", "Elevation (m)", true, 0},
            {"created_at", "date", "Created At", true, 0}
        };
        
        layers.push_back(sample_layer);
        
        return layers;
    }
    
    std::optional<LayerMetadata> getLayerMetadata(const std::string& layer_id) const override {
        auto layers = getLayers();
        for (const auto& layer : layers) {
            if (layer.layer_id == layer_id) {
                return layer;
            }
        }
        return std::nullopt;
    }
    
    std::optional<MBR> getLayerExtent(const std::string& layer_id) const override {
        auto metadata = getLayerMetadata(layer_id);
        if (metadata) {
            return metadata->extent;
        }
        return std::nullopt;
    }
    
    size_t getFeatureCount(const std::string& layer_id, const std::string& where_clause) const override {
        if (!connected_) return 0;
        
        // Simplified: would query actual table
        return 0;
    }
    
    QueryResult queryFeatures(const std::string& layer_id, const SpatialQuery& query) const override {
        if (!connected_) {
            throw std::runtime_error("Not connected to database");
        }
        
        QueryResult result;
        result.srid = query.out_srid;
        result.timestamp = std::to_string(std::time(nullptr));
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute spatial query
        if (query.bounding_box && spatial_mgr_) {
            auto spatial_results = spatial_mgr_->searchIntersects(layer_id, *query.bounding_box);
            
            // Convert to SpatialFeatures
            for (const auto& sr : spatial_results) {
                SpatialFeature feature;
                
                // Create point geometry from MBR centroid
                GeometryInfo geom(GeometryType::Point);
                double cx = (sr.mbr.minx + sr.mbr.maxx) / 2.0;
                double cy = (sr.mbr.miny + sr.mbr.maxy) / 2.0;
                geom.coords.push_back(Coordinate(cx, cy));
                
                if (sr.z_min && sr.z_max) {
                    double cz = (*sr.z_min + *sr.z_max) / 2.0;
                    geom.coords[0].z = cz;
                    geom.has_z = true;
                }
                
                feature.geometry = geom;
                feature.attributes.object_id = sr.primary_key;
                feature.attributes.source_table = layer_id;
                feature.srid = result.srid;
                
                result.features.push_back(feature);
            }
        }
        
        result.total_count = result.features.size();
        
        // Calculate extent
        if (!result.features.empty()) {
            result.extent = result.features[0].geometry.computeMBR();
            for (size_t i = 1; i < result.features.size(); ++i) {
                auto mbr = result.features[i].geometry.computeMBR();
                result.extent.minx = std::min(result.extent.minx, mbr.minx);
                result.extent.miny = std::min(result.extent.miny, mbr.miny);
                result.extent.maxx = std::max(result.extent.maxx, mbr.maxx);
                result.extent.maxy = std::max(result.extent.maxy, mbr.maxy);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.query_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        return result;
    }
    
    std::optional<SpatialFeature> getFeatureById(
        const std::string& layer_id,
        const std::string& object_id,
        ExportFormat format
    ) const override {
        // Simplified implementation
        return std::nullopt;
    }
    
    void streamFeatures(
        const std::string& layer_id,
        const SpatialQuery& query,
        std::function<void(const std::vector<SpatialFeature>&)> callback
    ) const override {
        // Stream in batches
        const size_t batch_size = 100;
        SpatialQuery batch_query = query;
        
        for (size_t offset = 0; ; offset += batch_size) {
            batch_query.offset = offset;
            batch_query.limit = batch_size;
            
            auto result = queryFeatures(layer_id, batch_query);
            if (result.features.empty()) break;
            
            callback(result.features);
            
            if (result.features.size() < batch_size) break;
        }
    }
    
    bool supportsZ(const std::string& layer_id) const override {
        auto metadata = getLayerMetadata(layer_id);
        return metadata && metadata->has_z;
    }
    
    std::optional<std::pair<double, double>> getZRange(const std::string& layer_id) const override {
        // Would query actual data
        return std::nullopt;
    }
    
    bool hasFEMMetadata(const std::string& layer_id) const override {
        auto metadata = getLayerMetadata(layer_id);
        return metadata && metadata->has_fem_metadata;
    }
    
    std::optional<nlohmann::json> getFEMMetadata(
        const std::string& layer_id,
        const std::string& object_id
    ) const override {
        // Would query FEM metadata from database
        return std::nullopt;
    }
    
    std::vector<SpatialFeature> getRiskData(
        const std::string& layer_id,
        const std::string& risk_type,
        const GeometryInfo& area_geom
    ) const override {
        // Query risk-related features
        SpatialQuery query;
        query.bounding_box = area_geom.computeMBR();
        query.where_clause = "risk_type = '" + risk_type + "'";
        
        auto result = queryFeatures(layer_id, query);
        return result.features;
    }
    
    QueryResult queryByTime(
        const std::string& layer_id,
        const std::string& start_time,
        const std::string& end_time,
        const SpatialQuery& spatial_query
    ) const override {
        // Combine temporal and spatial query
        SpatialQuery query = spatial_query;
        query.time_start = start_time;
        query.time_end = end_time;
        
        return queryFeatures(layer_id, query);
    }
    
    nlohmann::json getRelatedFeatures(
        const std::string& layer_id,
        const std::string& object_id,
        const std::string& relation_type
    ) const override {
        // Multi-model integration
        // Would traverse graph relationships or query time-series
        return nlohmann::json::object();
    }
    
    nlohmann::json getCapabilities() const override {
        return {
            {"name", getName()},
            {"version", getVersion()},
            {"supportedFormats", {
                "EWKB", "WKB", "WKT", "GeoJSON", "ESRI_JSON"
            }},
            {"supports3D", true},
            {"supportsZ", true},
            {"supportsM", false},
            {"supportsTemporal", true},
            {"supportsFEM", true},
            {"supportsMultiModel", true},
            {"supportsStreaming", true},
            {"maxFeaturesPerQuery", 10000},
            {"supportedSpatialRelations", {
                "intersects", "contains", "within"
            }},
            {"riskAssessmentTypes", {
                "flood", "drought", "cascade", "seismic"
            }}
        };
    }
};

// ============================================================================
// Factory Implementation
// ============================================================================

std::unique_ptr<IArcGISDataProvider> ArcGISDataProviderFactory::createProvider() {
    return std::make_unique<ThemisArcGISDataProvider>();
}

ArcGISDataProviderFactory& ArcGISDataProviderFactory::instance() {
    static ArcGISDataProviderFactory factory;
    return factory;
}

} // namespace arcgis
} // namespace geo
} // namespace themis

// ============================================================================
// DLL Export Functions
// ============================================================================

extern "C" {

ARCGIS_PROVIDER_API themis::geo::arcgis::IArcGISDataProvider* CreateArcGISDataProvider() {
    return new themis::geo::arcgis::ThemisArcGISDataProvider();
}

ARCGIS_PROVIDER_API void DestroyArcGISDataProvider(themis::geo::arcgis::IArcGISDataProvider* provider) {
    delete provider;
}

ARCGIS_PROVIDER_API const char* GetProviderVersion() {
    return "1.0.0";
}

ARCGIS_PROVIDER_API const char* GetProviderCapabilities() {
    static std::string capabilities_json;
    auto provider = themis::geo::arcgis::ArcGISDataProviderFactory::createProvider();
    capabilities_json = provider->getCapabilities().dump();
    return capabilities_json.c_str();
}

} // extern "C"
