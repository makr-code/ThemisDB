/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_processor.h                                    ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:24:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     110                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file geo_processor.h
 * @brief Geospatial Content Processor Plugin (GDAL-based)
 * 
 * Processes geospatial files (GeoJSON, Shapefile, GeoPackage, KML, GPX).
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "content_plugin_interface.h"
#include <mutex>
#include <atomic>

namespace themis {
namespace content {

/**
 * @brief Geospatial Processor Plugin
 * 
 * Uses GDAL/OGR for geospatial data processing.
 * Extracts:
 * - Geometry data (coordinates, bounding box)
 * - CRS (Coordinate Reference System)
 * - Feature properties/attributes
 * - Topology information
 * - Spatial indexes
 */
class GeoProcessor : public IContentProcessorPlugin {
public:
    GeoProcessor();
    ~GeoProcessor() override;
    
    // IContentProcessorPlugin interface
    PluginInfo getInfo() const override;
    bool initialize(const PluginConfig& config) override;
    void shutdown() override;
    bool canProcess(const std::string& mime_type) const override;
    
    ContentExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const ExtractionOptions& options = {}
    ) override;
    
    std::vector<ContentChunk> chunk(
        const ContentExtractionResult& result,
        int max_tokens,
        int overlap
    ) override;
    
    bool healthCheck() const override;
    json getStatistics() const override;
    
private:
    // Configuration
    std::string default_crs_ = "EPSG:4326";
    int max_features_ = 100000;
    bool simplify_geometry_ = false;
    double simplify_tolerance_ = 0.0001;
    bool generate_centroid_ = true;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> files_processed_{0};
    std::atomic<uint64_t> total_features_{0};
    std::atomic<uint64_t> errors_{0};
    
    bool initialized_ = false;
    
    // Internal methods
    GeoExtractionData parseGeoJSON(const std::vector<uint8_t>& blob);
    GeoExtractionData parseShapefile(const std::vector<uint8_t>& blob, const ExtractionOptions& options = {});
    GeoExtractionData parseGeoTIFF(const std::vector<uint8_t>& blob);
    GeoExtractionData parseKML(const std::vector<uint8_t>& blob);
    GeoExtractionData parseGPX(const std::vector<uint8_t>& blob);
    GeoExtractionData parseGeoPackage(const std::vector<uint8_t>& blob, const ExtractionOptions& options = {});
    
    std::pair<double, double> calculateCentroid(const GeoExtractionData& geo);
    double calculateArea(const GeoExtractionData& geo);
    double calculateLength(const GeoExtractionData& geo);
};

} // namespace content
} // namespace themis
