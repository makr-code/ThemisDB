/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_optimization_standalone.cpp                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Standalone test for concurrent_cache and RocksDB metrics
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <cassert>

#define THEMIS_DEBUG(fmt, ...) std::cout << "[DEBUG] " << fmt << std::endl
#define THEMIS_INFO(fmt, ...) std::cout << "[INFO] " << fmt << std::endl
#define THEMIS_ERROR(fmt, ...) std::cerr << "[ERROR] " << fmt << std::endl

#include "utils/concurrent_cache.h"

using namespace themis;

int main() {
    std::cout << "=== Testing ConcurrentCache ===" << std::endl;
    
    // Test 1: Basic insert and get
    {
        ConcurrentCache<int, std::string> cache;
        cache.insert(1, "one");
        cache.insert(2, "two");
        
        auto val1 = cache.get(1);
        auto val2 = cache.get(2);
        auto val3 = cache.get(3);
        
        assert(val1.has_value() && val1.value() == "one");
        assert(val2.has_value() && val2.value() == "two");
        assert(!val3.has_value());
        
        std::cout << "✅ Test 1: Basic insert/get passed" << std::endl;
    }
    
    // Test 2: Concurrent inserts
    {
        ConcurrentCache<int, int> cache;
        constexpr int num_threads = 8;
        constexpr int items_per_thread = 1000;
        
        std::vector<std::thread> threads = {};

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&cache, t]() {
                for (int i = 0; i < items_per_thread; ++i) {
                    int key = t * items_per_thread + i;
                    cache.insert(key, key * 2);
                }
            });
        }
        
        for (auto& th : threads) {
            th.join();
        }
        
        assert(cache.size() == num_threads * items_per_thread);
        std::cout << "✅ Test 2: Concurrent inserts (" << cache.size() << " items) passed" << std::endl;
    }
    
    // Test 3: Concurrent read/write
    {
        ConcurrentCache<int, int> cache;
        
        // Pre-populate
        for (int i = 0; i < 1000; ++i) {
            cache.insert(i, i);
        }
        
        std::atomic<int> read_count{0};
        std::atomic<int> write_count{0};
        
        std::vector<std::thread> threads;
        
        // Readers
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&cache, &read_count]() {
                for (int i = 0; i < 5000; ++i) {
                    auto val = cache.get(i % 1000);
                    if (val.has_value()) {
                        read_count++;
                    }
                }
            });
        }
        
        // Writers
        for (int t = 0; t < 2; ++t) {
            threads.emplace_back([&cache, &write_count]() {
                for (int i = 1000; i < 2000; ++i) {
                    cache.insert(i, i * 3);
                    write_count++;
                }
            });
        }
        
        for (auto& th : threads) {
            th.join();
        }
        
        std::cout << "✅ Test 3: Concurrent read/write (reads: " << read_count 
                  << ", writes: " << write_count << ") passed" << std::endl;
    }
    
    std::cout << "\n=== All ConcurrentCache tests passed! ===" << std::endl;
    return 0;
}
