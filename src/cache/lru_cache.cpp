/**
 * @file lru_cache.cpp
 * @brief LRU cache implementation with move semantics
 * @version 0.1.0
 * @note Gap Fix: CWE-457, CWE-672
 */

#include "cache/lru_cache.h"
#include <utility>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace cache {

// Template implementation moved to header file (see lru_cache.hpp)
// This file provides specializations and explicit template instantiations if needed

} // namespace cache
} // namespace themis
