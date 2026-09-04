/**
 * @file bench_hybrid_vector_geo.cpp
 * @brief Simplified benchmarks for vector-geo operations (v1.3.0)
 * 
 * Note: Full hybrid vector-geo is a v1.4.0 feature. This demonstrates
 * basic vector similarity and geographic distance metrics separately.
 */

#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ===== Vector Math Benchmarks =====

static void BM_VectorDistance_Euclidean(benchmark::State& state) {
    std::vector<float> v1(state.range(0));
    std::vector<float> v2(state.range(0));
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto& val : v1) {
      val = dis(gen);
    }
    for (auto& val : v2) {
      val = dis(gen);
    }
    
    for (auto _ : state) {
        float dist = 0.0f;
        for (size_t i = 0; i < v1.size(); ++i) {
            float diff = v1[i] - v2[i];
            dist += diff * diff;
        }
        dist = std::sqrt(dist);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_VectorDistance_Euclidean)
    ->RangeMultiplier(2)
    ->Range(64, 1024)
    ->Complexity();

static void BM_VectorDistance_Cosine(benchmark::State& state) {
    std::vector<float> v1(state.range(0));
    std::vector<float> v2(state.range(0));
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Normalize vectors
    auto normalize = [](std::vector<float>& v) {
        float sum = 0.0f;
        for (float x : v) {
          sum += x * x;
        }
        float norm = std::sqrt(sum);
        if (norm > 1e-6) {
            for (float& x : v) {
              x /= norm;
            }
        }
    };
    
    for (auto& val : v1) {
      val = dis(gen);
    }
    for (auto& val : v2) {
      val = dis(gen);
    }
    normalize(v1);
    normalize(v2);
    
    for (auto _ : state) {
        float dot = 0.0f;
        for (size_t i = 0; i < v1.size(); ++i) {
            dot += v1[i] * v2[i];
        }
        benchmark::DoNotOptimize(dot);
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_VectorDistance_Cosine)
    ->RangeMultiplier(2)
    ->Range(64, 1024)
    ->Complexity();

static void BM_VectorNormalization(benchmark::State& state) {
    std::vector<float> v(state.range(0));
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto _ : state) {
        for (auto& val : v) {
          val = dis(gen);
        }
        
        float sum = 0.0f;
        for (float x : v) {
          sum += x * x;
        }
        float norm = std::sqrt(sum);
        if (norm > 1e-6) {
            for (float& x : v) {
              x /= norm;
            }
        }
        
        benchmark::DoNotOptimize(v[0]);
    }
}
BENCHMARK(BM_VectorNormalization)
    ->RangeMultiplier(2)
    ->Range(64, 4096);

// ===== Geographic Coordinate Benchmarks =====

struct Point {
    double lat, lon;
};

static double haversineDist(const Point& p1, const Point& p2) {
    const double R = 6371.0; // Earth radius in km
    
    double dlat = (p2.lat - p1.lat) * M_PI / 180.0;
    double dlon = (p2.lon - p1.lon) * M_PI / 180.0;
    
    double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
               std::cos(p1.lat * M_PI / 180.0) * std::cos(p2.lat * M_PI / 180.0) *
               std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
    
    double c = 2.0 * std::asin(std::sqrt(a));
    return R * c;
}

static void BM_GeoDistance_Haversine(benchmark::State& state) {
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    
    std::vector<Point> points(state.range(0));
    for (auto& p : points) {
        p.lat = lat_dis(gen);
        p.lon = lon_dis(gen);
    }
    
    for (auto _ : state) {
        double sum = 0.0;
        for (size_t i = 1; i < points.size(); ++i) {
            sum += haversineDist(points[0], points[i]);
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_GeoDistance_Haversine)
    ->Range(100, 10000);

static void BM_GeoPointInBoundingBox(benchmark::State& state) {
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    
    std::vector<Point> points(state.range(0));
    for (auto& p : points) {
        p.lat = lat_dis(gen);
        p.lon = lon_dis(gen);
    }
    
    // Define a bounding box
    double min_lat = 40.0, max_lat = 41.0;
    double min_lon = -74.0, max_lon = -73.0;
    
    for (auto _ : state) {
        int count = 0;
        for (const auto& p : points) {
            if (p.lat >= min_lat && p.lat <= max_lat &&
                p.lon >= min_lon && p.lon <= max_lon) {
                count++;
            }
        }
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(BM_GeoPointInBoundingBox)
    ->Range(100, 100000);

// ===== Combined Vector-Geo Simulation Benchmark =====

static void BM_VectorGeoFiltering(benchmark::State& state) {
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> vec_dis(0.0, 1.0);
    std::uniform_real_distribution<> lat_dis(-90.0, 90.0);
    std::uniform_real_distribution<> lon_dis(-180.0, 180.0);
    
    struct Item {
        std::vector<float> embedding;
        Point location;
    };
    
    std::vector<Item> items(state.range(0));
    for (auto& item : items) {
        item.embedding.resize(128);
        for (auto& v : item.embedding) {
          v = vec_dis(gen);
        }
        item.location.lat = lat_dis(gen);
        item.location.lon = lon_dis(gen);
    }
    
    // Query vector
    std::vector<float> query_vec(128);
    for (auto& v : query_vec) {
      v = vec_dis(gen);
    }
    
    // Query location
    Point query_loc = {40.5, -73.5};
    double max_distance_km = 50.0;
    
    for (auto _ : state) {
        std::vector<std::pair<double, int>> candidates;
        
        // Geo filter first
        for (size_t i = 0; i < items.size(); ++i) {
            if (haversineDist(query_loc, items[i].location) <= max_distance_km) {
                // Then compute vector distance
                double dot = 0.0;
                for (size_t j = 0; j < query_vec.size(); ++j) {
                    dot += query_vec[j] * items[i].embedding[j];
                }
                candidates.emplace_back(-dot, i); // negative for sorting
            }
        }
        
        benchmark::DoNotOptimize(candidates);
    }
}
BENCHMARK(BM_VectorGeoFiltering)
    ->Range(1000, 50000);

// ===== Main =====

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}
