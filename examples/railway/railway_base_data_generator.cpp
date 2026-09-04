/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            railway_base_data_generator.cpp                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     571                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Railway Base Data Generator
 * 
 * Generiert Basis-Streckendaten für das Deutsche Bahn Netz:
 * - Bahnhöfe mit echten Koordinaten
 * - Streckenabschnitte mit Geschwindigkeitsbeschränkungen
 * - Signale, Weichen, Bahnübergänge
 * - Topographie (Steigungen, Kurven)
 * 
 * Basiert auf:
 * - OpenStreetMap Railway-Daten
 * - Deutsche Bahn Open Data Portal
 * - Simulierte Daten wo nötig
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// Data Structures
// ============================================================================

struct GeoPoint {
    double lat = 0;
    double lon;
    double altitude;
};

struct Station {
    std::string eva_number;
    std::string name;
    GeoPoint location;
    int tracks;
    std::string category;
    std::string operator_name;
};

struct TrackSegment {
    std::string track_number;
    std::string segment_id;
    double start_km;
    double end_km;
    std::vector<GeoPoint> geometry;
    int max_speed_kmh;
    bool electrified;
    std::string track_class;
    double gradient_permille;
    int curve_radius_m;
};

struct Signal {
    std::string signal_id;
    std::string signal_type;
    std::string track_number;
    double km_position;
    GeoPoint location;
};

struct Switch {
    std::string switch_id;
    std::string switch_type;
    std::string track_number;
    double km_position;
    GeoPoint location;
    int design_speed_straight_kmh;
    int design_speed_diverging_kmh;
};

struct LevelCrossing {
    std::string crossing_id;
    std::string track_number;
    double km_position;
    GeoPoint location;
    std::string road_name;
    std::string protection_type;
};

// ============================================================================
// German Railway Network - Real Data
// ============================================================================

std::vector<Station> generateGermanStations() {
    return {
        // Major stations with real coordinates from OSM
        {"8000105", "Frankfurt(Main)Hbf", {50.1067, 8.6625, 98}, 24, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000261", "München Hbf", {48.1401, 11.5583, 518}, 32, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000244", "Mannheim Hbf", {49.4792, 8.4689, 97}, 12, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8011160", "Hamburg Hbf", {53.5528, 10.0067, 6}, 14, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8098160", "Berlin Hbf", {52.5250, 13.3694, 34}, 14, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000191", "Köln Hbf", {50.9432, 6.9589, 59}, 11, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000284", "Nürnberg Hbf", {49.4458, 11.0839, 309}, 22, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000207", "Hannover Hbf", {52.3769, 9.7419, 55}, 12, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000096", "Dortmund Hbf", {51.5176, 7.4589, 78}, 16, "Fernverkehrsbahnhof", "DB Station&Service"},
        {"8000098", "Dresden Hbf", {51.0408, 13.7320, 112}, 16, "Fernverkehrsbahnhof", "DB Station&Service"},
        
        // Intermediate stations
        {"8000156", "Fulda", {50.5544, 9.6836, 261}, 6, "Regionalbahnhof", "DB Station&Service"},
        {"8000026", "Augsburg Hbf", {48.3657, 10.8856, 493}, 9, "Regionalbahnhof", "DB Station&Service"},
        {"8000240", "Mainz Hbf", {50.0012, 8.2589, 82}, 8, "Regionalbahnhof", "DB Station&Service"},
        {"8000068", "Darmstadt Hbf", {49.8728, 8.6306, 143}, 7, "Regionalbahnhof", "DB Station&Service"},
        {"8000152", "Frankfurt Flughafen", {50.0531, 8.5706, 100}, 7, "Fernverkehrsbahnhof", "DB Station&Service"}
    };
}

// Speed restrictions database - based on real DB infrastructure standards
struct SpeedRestriction {
    std::string reason;
    int max_speed_kmh;
    double probability; // How often this occurs
};

std::vector<SpeedRestriction> getSpeedRestrictions() {
    return {
        {"curve_tight_500m", 100, 0.05},
        {"curve_800m", 130, 0.10},
        {"curve_1200m", 160, 0.15},
        {"gradient_steep_12", 120, 0.08},
        {"gradient_moderate_8", 140, 0.12},
        {"station_approach", 80, 1.00}, // Always near stations
        {"construction_zone", 50, 0.02},
        {"bridge_old", 100, 0.03},
        {"tunnel_single_track", 120, 0.04},
        {"level_crossing", 140, 0.10}
    };
}

// ============================================================================
// Track Network Generator with Realistic Speed Profiles
// ============================================================================

class TrackNetworkGenerator {
private:
    std::mt19937 rng = {};
    std::uniform_real_distribution<> uniform_dist{0.0, 1.0};
    std::uniform_real_distribution<> gradient_dist{-10.0, 10.0};
    std::uniform_int_distribution<> curve_radius_dist{500, 5000};
    std::vector<SpeedRestriction> speed_restrictions;
    
public:
    TrackNetworkGenerator() : rng(std::random_device{}()) {
        speed_restrictions = getSpeedRestrictions();
    }
    
    // Calculate distance between two geo points (Haversine formula)
    double calculateDistance(const GeoPoint& p1, const GeoPoint& p2) {
        const double R = 6371000; // Earth radius in meters
        double lat1 = p1.lat * M_PI / 180.0;
        double lat2 = p2.lat * M_PI / 180.0;
        double dlat = (p2.lat - p1.lat) * M_PI / 180.0;
        double dlon = (p2.lon - p1.lon) * M_PI / 180.0;
        
        double a = sin(dlat/2) * sin(dlat/2) +
                   cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2);
        double c = 2 * atan2(sqrt(a), sqrt(1-a));
        return R * c;
    }
    
    // Interpolate coordinates between two points
    std::vector<GeoPoint> interpolateRoute(const GeoPoint& from, const GeoPoint& to, int segments) {
        std::vector<GeoPoint> points = {};

        for (int i = 0; i <= segments; ++i) {
            double t = static_cast<double>(i) / segments;
            GeoPoint p;
            p.lat = from.lat + t * (to.lat - from.lat);
            p.lon = from.lon + t * (to.lon - from.lon);
            p.altitude = from.altitude + t * (to.altitude - from.altitude);
            points.push_back(p);
        }
        return points;
    }
    
    // Determine speed restriction for a segment
    int calculateSpeedLimit(int base_speed, double km_position, double total_length, 
                           double gradient, int curve_radius, bool near_station) {
        int speed = base_speed;
        
        // Station approach zones (5 km before/after)
        if (near_station) {
            speed = std::min(speed, 80);
        }
        
        // Gradient restrictions
        if (std::abs(gradient) > 12.0) {
            speed = std::min(speed, 120);
        } else if (std::abs(gradient) > 8.0) {
            speed = std::min(speed, 140);
        }
        
        // Curve restrictions
        if (curve_radius < 800) {
            speed = std::min(speed, 100);
        } else if (curve_radius < 1200) {
            speed = std::min(speed, 130);
        } else if (curve_radius < 2000) {
            speed = std::min(speed, 160);
        }
        
        // Random restrictions (construction, etc.)
        for (const auto& restriction : speed_restrictions) {
            if (restriction.reason == "construction_zone" && uniform_dist(rng) < restriction.probability) {
                speed = std::min(speed, restriction.max_speed_kmh);
            }
        }
        
        return speed;
    }
    
    // Generate detailed track segments with realistic speed profiles
    std::vector<TrackSegment> generateTrackSegments(
        const std::string& track_number,
        const Station& from,
        const Station& to,
        const std::string& line_type // "high_speed", "main_line", "regional"
    ) {
        std::vector<TrackSegment> segments;
        
        // Base speed limits by line type
        int base_speed = 200; // high_speed ICE lines
        if (line_type == "main_line") base_speed = 160; // IC/EC lines
        if (line_type == "regional") base_speed = 120; // RE/RB lines
        
        double total_distance = calculateDistance(from.location, to.location);
        int num_segments = static_cast<int>(total_distance / 1000); // 1 km segments
        if (num_segments < 1) {
          num_segments = 1;
        }
        
        auto route_points = interpolateRoute(from.location, to.location, num_segments);
        
        for (size_t i = 0; i < route_points.size() - 1; ++i) {
            TrackSegment seg;
            seg.track_number = track_number;
            seg.segment_id = std::to_string(static_cast<int>(i)) + "_" + 
                            std::to_string(static_cast<int>(i+1));
            seg.start_km = i * 1.0;
            seg.end_km = (i + 1) * 1.0;
            
            // Geometry
            seg.geometry.push_back(route_points[i]);
            seg.geometry.push_back(route_points[i+1]);
            
            // Calculate gradient from altitude difference
            double altitude_diff = route_points[i+1].altitude - route_points[i].altitude;
            seg.gradient_permille = (altitude_diff / 1000.0) * 1000;
            
            // Simulate curves (every 5-10 segments on average)
            seg.curve_radius_m = 10000; // Default: straight
            if (uniform_dist(rng) < 0.15) { // 15% chance of curve
                seg.curve_radius_m = curve_radius_dist(rng);
            }
            
            // Check if near station (first/last 5 km)
            bool near_station = (i < 5 || i >= num_segments - 5);
            
            // Calculate speed limit
            seg.max_speed_kmh = calculateSpeedLimit(
                base_speed, seg.start_km, total_distance / 1000.0,
                seg.gradient_permille, seg.curve_radius_m, near_station
            );
            
            seg.electrified = (line_type != "regional" || uniform_dist(rng) > 0.2);
            seg.track_class = (base_speed >= 160) ? "D4" : "D3";
            
            segments.push_back(seg);
        }
        
        return segments;
    }
    
    // Generate signals (main signals every 1.5-3 km, advance signals in between)
    std::vector<Signal> generateSignals(const std::string& track_number, double track_length_km) {
        std::vector<Signal> signals;
        std::uniform_real_distribution<> signal_spacing{1.5, 3.0};
        
        double km = 0;
        int signal_counter = 1;
        
        while (km < track_length_km) {
            // Main signal
            Signal main_sig;
            main_sig.signal_id = track_number + "_H" + std::to_string(signal_counter);
            main_sig.track_number = track_number;
            main_sig.km_position = km;
            main_sig.signal_type = "Hauptsignal";
            signals.push_back(main_sig);
            
            // Advance signal (Vorsignal) 1000m before main signal
            if (km >= 1.0) {
                Signal advance_sig;
                advance_sig.signal_id = track_number + "_V" + std::to_string(signal_counter);
                advance_sig.track_number = track_number;
                advance_sig.km_position = km - 1.0;
                advance_sig.signal_type = "Vorsignal";
                signals.push_back(advance_sig);
            }
            
            km += signal_spacing(rng);
            signal_counter++;
        }
        
        return signals;
    }
    
    // Generate switches at stations and junctions
    std::vector<Switch> generateSwitches(const std::string& track_number, 
                                         const std::vector<double>& junction_kms) {
        std::vector<Switch> switches;
        
        for (size_t i = 0; i < junction_kms.size(); ++i) {
            Switch sw;
            sw.switch_id = "W_" + track_number + "_" + std::to_string(i+1);
            sw.track_number = track_number;
            sw.km_position = junction_kms[i];
            sw.switch_type = "EW60-500-1:9"; // Standard DB switch type
            sw.design_speed_straight_kmh = 200;
            sw.design_speed_diverging_kmh = 60;
            
            switches.push_back(sw);
        }
        
        return switches;
    }
    
    // Generate level crossings (Bahnübergänge) - only on non-high-speed lines
    std::vector<LevelCrossing> generateLevelCrossings(const std::string& track_number,
                                                       double track_length_km,
                                                       const std::string& line_type) {
        std::vector<LevelCrossing> crossings;
        
        if (line_type == "high_speed") return crossings; // No level crossings on ICE lines
        
        std::uniform_real_distribution<> crossing_spacing{5.0, 15.0};
        double km = 2.0;
        int crossing_counter = 1;
        
        while (km < track_length_km - 2.0) {
            LevelCrossing lx;
            lx.crossing_id = "LX_" + track_number + "_" + std::to_string(crossing_counter);
            lx.track_number = track_number;
            lx.km_position = km;
            lx.road_name = "Landstraße L" + std::to_string(100 + crossing_counter);
            lx.protection_type = (crossing_counter % 2 == 0) ? 
                "automatic_half_barriers" : "automatic_full_barriers";
            
            crossings.push_back(lx);
            km += crossing_spacing(rng);
            crossing_counter++;
        }
        
        return crossings;
    }
};

// ============================================================================
// JSON Export
// ============================================================================

void exportToJSON(const std::string& filename,
                  const std::vector<Station>& stations,
                  const std::vector<TrackSegment>& segments,
                  const std::vector<Signal>& signals,
                  const std::vector<Switch>& switches,
                  const std::vector<LevelCrossing>& crossings) {
    
    json output;
    output["metadata"] = {
        {"generated_at", "2024-12-13T16:00:00Z"},
        {"generator", "railway_base_data_generator v1.0"},
        {"network", "Deutsche Bahn"},
        {"data_quality", "simulated_based_on_real_coordinates"},
        {"description", "Base railway network data with realistic speed profiles and infrastructure"}
    };
    
    // Stations
    output["stations"] = json::array();
    for (const auto& station : stations) {
        output["stations"].push_back({
            {"eva_number", station.eva_number},
            {"name", station.name},
            {"location", {
                {"lat", station.location.lat},
                {"lon", station.location.lon},
                {"altitude", station.location.altitude}
            }},
            {"tracks", station.tracks},
            {"category", station.category},
            {"operator", station.operator_name}
        });
    }
    
    // Track Segments with speed profiles
    output["track_segments"] = json::array();
    for (const auto& seg : segments) {
        json geometry = json::array();
        for (const auto& p : seg.geometry) {
            geometry.push_back({p.lon, p.lat, p.altitude});
        }
        
        output["track_segments"].push_back({
            {"track_number", seg.track_number},
            {"segment_id", seg.segment_id},
            {"start_km", seg.start_km},
            {"end_km", seg.end_km},
            {"geometry", {
                {"type", "LineString"},
                {"coordinates", geometry}
            }},
            {"max_speed_kmh", seg.max_speed_kmh},
            {"electrified", seg.electrified},
            {"track_class", seg.track_class},
            {"gradient_permille", seg.gradient_permille},
            {"curve_radius_m", seg.curve_radius_m}
        });
    }
    
    // Signals
    output["signals"] = json::array();
    for (const auto& sig : signals) {
        output["signals"].push_back({
            {"signal_id", sig.signal_id},
            {"signal_type", sig.signal_type},
            {"track_number", sig.track_number},
            {"km_position", sig.km_position}
        });
    }
    
    // Switches
    output["switches"] = json::array();
    for (const auto& sw : switches) {
        output["switches"].push_back({
            {"switch_id", sw.switch_id},
            {"switch_type", sw.switch_type},
            {"track_number", sw.track_number},
            {"km_position", sw.km_position},
            {"design_speed_straight_kmh", sw.design_speed_straight_kmh},
            {"design_speed_diverging_kmh", sw.design_speed_diverging_kmh}
        });
    }
    
    // Level Crossings
    output["level_crossings"] = json::array();
    for (const auto& lx : crossings) {
        output["level_crossings"].push_back({
            {"crossing_id", lx.crossing_id},
            {"track_number", lx.track_number},
            {"km_position", lx.km_position},
            {"road_name", lx.road_name},
            {"protection_type", lx.protection_type}
        });
    }
    
    // Write to file
    std::ofstream file(filename);
    file << output.dump(2);
    file.close();
    
    std::cout << "✓ Generated railway network data: " << filename << std::endl;
    std::cout << "  - Stations: " << stations.size() << std::endl;
    std::cout << "  - Track Segments: " << segments.size() << std::endl;
    std::cout << "  - Signals: " << signals.size() << std::endl;
    std::cout << "  - Switches: " << switches.size() << std::endl;
    std::cout << "  - Level Crossings: " << crossings.size() << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "Railway Base Data Generator" << std::endl;
    std::cout << "Generiert Deutsche Bahn Streckennetz mit:" << std::endl;
    std::cout << "- Echte Bahnhofskoordinaten (OSM)" << std::endl;
    std::cout << "- Realistische Geschwindigkeitsprofile" << std::endl;
    std::cout << "- Signalanlagen, Weichen, Bahnübergänge" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;
    
    TrackNetworkGenerator generator;
    
    // Generate stations
    auto stations = generateGermanStations();
    
    // Generate major routes
    std::vector<TrackSegment> all_segments;
    std::vector<Signal> all_signals;
    std::vector<Switch> all_switches;
    std::vector<LevelCrossing> all_crossings;
    
    // Route 1: Frankfurt - Mannheim (High-Speed ICE line 3600)
    std::cout << "Generating Frankfurt - Mannheim (High-Speed ICE)..." << std::endl;
    auto seg1 = generator.generateTrackSegments("3600", stations[0], stations[2], "high_speed");
    auto sig1 = generator.generateSignals("3600", 73.5);
    auto sw1 = generator.generateSwitches("3600", {0.5, 73.0});
    all_segments.insert(all_segments.end(), seg1.begin(), seg1.end());
    all_signals.insert(all_signals.end(), sig1.begin(), sig1.end());
    all_switches.insert(all_switches.end(), sw1.begin(), sw1.end());
    
    // Route 2: München - Augsburg (High-Speed)
    std::cout << "Generating München - Augsburg (High-Speed)..." << std::endl;
    auto seg2 = generator.generateTrackSegments("5300", stations[1], stations[11], "high_speed");
    auto sig2 = generator.generateSignals("5300", 61.0);
    auto sw2 = generator.generateSwitches("5300", {0.5, 60.5});
    all_segments.insert(all_segments.end(), seg2.begin(), seg2.end());
    all_signals.insert(all_signals.end(), sig2.begin(), sig2.end());
    all_switches.insert(all_switches.end(), sw2.begin(), sw2.end());
    
    // Route 3: Frankfurt - Fulda (Main Line)
    std::cout << "Generating Frankfurt - Fulda (Main Line)..." << std::endl;
    auto seg3 = generator.generateTrackSegments("3610", stations[0], stations[10], "main_line");
    auto sig3 = generator.generateSignals("3610", 103.0);
    auto sw3 = generator.generateSwitches("3610", {0.5, 102.5});
    auto lx3 = generator.generateLevelCrossings("3610", 103.0, "main_line");
    all_segments.insert(all_segments.end(), seg3.begin(), seg3.end());
    all_signals.insert(all_signals.end(), sig3.begin(), sig3.end());
    all_switches.insert(all_switches.end(), sw3.begin(), sw3.end());
    all_crossings.insert(all_crossings.end(), lx3.begin(), lx3.end());
    
    // Route 4: Köln - Dortmund (Main Line)
    std::cout << "Generating Köln - Dortmund (Main Line)..." << std::endl;
    auto seg4 = generator.generateTrackSegments("2600", stations[5], stations[8], "main_line");
    auto sig4 = generator.generateSignals("2600", 80.0);
    auto sw4 = generator.generateSwitches("2600", {0.5, 79.5});
    all_segments.insert(all_segments.end(), seg4.begin(), seg4.end());
    all_signals.insert(all_signals.end(), sig4.begin(), sig4.end());
    all_switches.insert(all_switches.end(), sw4.begin(), sw4.end());
    
    // Export to JSON
    exportToJSON("data/railway_network_base_germany.json",
                 stations, all_segments, all_signals, all_switches, all_crossings);
    
    std::cout << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "✓ COMPLETE!" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Data exported to: data/railway_network_base_germany.json" << std::endl;
    std::cout << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "1. Import into ThemisDB using import script" << std::endl;
    std::cout << "2. Run train simulator to generate telemetry" << std::endl;
    std::cout << "3. Visualize on map using Grafana/Leaflet" << std::endl;
    
    return 0;
}
