/*
 * ThemisDB | File: test_hash_util.cpp
 */

#include "utils/hash_util.h"

#include <array>
#include <cstdint>

namespace {

constexpr std::array<std::uint8_t, 3> kAbcBytes{{'a', 'b', 'c'}};
constexpr std::array<std::uint8_t, 0> kEmptyBytes{};

static_assert(themis::hash::fnv1a32(kAbcBytes.data(), kAbcBytes.size()) ==
              themis::hash::fnv1a32("abc", 3),
              "fnv1a32(uint8_t*) must stay constexpr-compatible and match char hashing");

static_assert(themis::hash::fnv1a64(kAbcBytes.data(), kAbcBytes.size()) ==
              themis::hash::fnv1a64("abc", 3),
              "fnv1a64(uint8_t*) must stay constexpr-compatible and match char hashing");

static_assert(themis::hash::fnv1a32(kEmptyBytes.data(), kEmptyBytes.size()) ==
              themis::hash::kFnv32OffsetBasis,
              "Empty uint8_t buffers must produce the 32-bit FNV offset basis");

static_assert(themis::hash::fnv1a64(kEmptyBytes.data(), kEmptyBytes.size()) ==
              themis::hash::kFnv64OffsetBasis,
              "Empty uint8_t buffers must produce the 64-bit FNV offset basis");

} // namespace
