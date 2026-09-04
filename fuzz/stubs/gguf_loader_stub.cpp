/**
 * @file gguf_loader_stub.cpp
 * @brief Standalone fuzz stub for themis::llm::GGUFLoader
 *
 * Provides a minimal, dependency-free implementation of GGUFLoader for use in
 * the standalone fuzz_targets build (ENABLE_FUZZING=ON).  The stub replaces
 * spdlog logging with no-ops and omits RocksDB storage; the GGUF binary format
 * parsing (magic, version, metadata key-value pairs, tensor info) is retained
 * so that AFL++/libFuzzer exercises the real parsing paths.
 *
 * Activation: compiled only when THEMIS_FUZZ_STUBS is defined (set by
 *   fuzz/CMakeLists.txt for the project-linked harness targets).
 * Production Delta: no spdlog log calls; no RocksDB writes in loadToThemisDB().
 * Removal Plan: replace with the real library once the full project is
 *   available in the fuzz build environment (vcpkg + spdlog resolved).
 *
 * STUB/SIMULATION NOTE:
 * Purpose: allow gguf_loader_harness to compile in the standalone fuzz build
 * Activation: THEMIS_FUZZ_STUBS cmake define / ENABLE_FUZZING=ON path
 * Production Delta: logging suppressed; RocksDB ops are no-ops
 * Removal Plan: wire against real themisdb_llm once deps are available in fuzz env
 */

#include "llm/gguf_loader.h"

#include <cstring>
#include <fstream>
#include <vector>
#include <string>

namespace themis {
namespace llm {

// ── Constants ────────────────────────────────────────────────────────────────

static constexpr uint8_t kGGUFMagic[4] = {'G', 'G', 'U', 'F'};
static constexpr size_t  kMaxMetaKVCount = 1024;
static constexpr size_t  kMaxTensorCount = 512;
static constexpr size_t  kMaxStringLen   = 4096;

// ── Internal helpers (file-scoped) ───────────────────────────────────────────

/**
 * @brief Context holding raw file bytes for parsing.
 *
 * Holds a flat byte buffer and a cursor position so that the recursive
 * parse helpers can consume the buffer safely without unsafe pointer
 * arithmetic visible at call sites.
 */
struct ParseContext {
    const uint8_t* buf  = nullptr;  ///< Pointer to raw file bytes (not owned)
    size_t         size = 0;        ///< Total byte count
    size_t         pos  = 0;        ///< Current read cursor

    /** @return true when at least @p n bytes remain unread. */
    bool can_read(size_t n) const noexcept { return pos + n <= size; }

    /**
     * @brief Copy @p n bytes into @p out and advance the cursor.
     * @return false if insufficient bytes remain (buffer not modified).
     */
    bool read(void* out, size_t n) noexcept {
        if (!can_read(n)) {
          return false;
        }
        std::memcpy(out, buf + pos, n);
        pos += n;
        return true;
    }

    /** @brief Skip @p n bytes without reading. */
    bool skip(size_t n) noexcept {
        if (!can_read(n)) {
          return false;
        }
        pos += n;
        return true;
    }
};

static bool read_u8(ParseContext& ctx, uint8_t& v)   { return ctx.read(&v, 1); }
static bool read_u16(ParseContext& ctx, uint16_t& v) { return ctx.read(&v, 2); }
static bool read_i16(ParseContext& ctx, int16_t& v)  { return ctx.read(&v, 2); }
static bool read_u32(ParseContext& ctx, uint32_t& v) { return ctx.read(&v, 4); }
static bool read_i32(ParseContext& ctx, int32_t& v)  { return ctx.read(&v, 4); }
static bool read_u64(ParseContext& ctx, uint64_t& v) { return ctx.read(&v, 8); }
static bool read_i64(ParseContext& ctx, int64_t& v)  { return ctx.read(&v, 8); }

static bool read_f32(ParseContext& ctx, float& v) {
    return ctx.read(&v, sizeof(float));
}
static bool read_f64(ParseContext& ctx, double& v) {
    return ctx.read(&v, sizeof(double));
}

/**
 * @brief Read a GGUF length-prefixed UTF-8 string.
 *
 * Format: uint64 length, then length bytes (not NUL-terminated).
 * Capped at kMaxStringLen to prevent excessive allocations on fuzz inputs.
 */
static bool read_gguf_string(ParseContext& ctx, std::string& out) {
    uint64_t len = 0;
    if (!read_u64(ctx, len)) {
      return false;
    }
    if (len > kMaxStringLen)  return false;   // cap — treat as format error
    if (!ctx.can_read(static_cast<size_t>(len))) {
      return false;
    }
    out.assign(reinterpret_cast<const char*>(ctx.buf + ctx.pos),
               static_cast<size_t>(len));
    ctx.pos += static_cast<size_t>(len);
    return true;
}

/**
 * @brief Skip one GGUF metadata value (recursive for ARRAY).
 *
 * @param ctx    Parsing context.
 * @param vtype  GGUF value type tag.
 * @param depth  Current recursion depth (guards against crafted deep arrays).
 */
static bool skip_gguf_value(ParseContext& ctx, uint32_t vtype,
                             unsigned depth = 0) {
    if (depth > 4) return false;   // cap nesting

    switch (static_cast<GGUFValueType>(vtype)) {
    case GGUFValueType::UINT8:
    case GGUFValueType::INT8:
    case GGUFValueType::BOOL: {
        uint8_t v{}; return read_u8(ctx, v);
    }
    case GGUFValueType::UINT16:
    case GGUFValueType::INT16: {
        uint16_t v{}; return read_u16(ctx, v);
    }
    case GGUFValueType::UINT32:
    case GGUFValueType::INT32:
    case GGUFValueType::FLOAT32: {
        uint32_t v{}; return read_u32(ctx, v);
    }
    case GGUFValueType::UINT64:
    case GGUFValueType::INT64:
    case GGUFValueType::FLOAT64: {
        uint64_t v{}; return read_u64(ctx, v);
    }
    case GGUFValueType::STRING: {
        std::string s;
        return read_gguf_string(ctx, s);
    }
    case GGUFValueType::ARRAY: {
        uint32_t elem_type{};
        uint64_t count{};
        if (!read_u32(ctx, elem_type)) {
          return false;
        }
        if (!read_u64(ctx, count)) {
          return false;
        }
        static constexpr uint64_t kMaxArrayElems = 8192;
        if (count > kMaxArrayElems) {
          return false;
        }
        for (uint64_t i = 0; i < count; ++i) {
            if (!skip_gguf_value(ctx, elem_type, depth + 1)) {
              return false;
            }
        }
        return true;
    }
    default:
        return false;  // Unknown type — treat as format error
    }
}

// ── GGUFLoader member implementations ────────────────────────────────────────

GGUFLoader::GGUFLoader()
    : db_(nullptr), fd_(0), mmap_base_(nullptr), mmap_size_(0) {}

GGUFLoader::GGUFLoader(RocksDBWrapper* db)
    : db_(db), fd_(0), mmap_base_(nullptr), mmap_size_(0) {}

GGUFLoader::~GGUFLoader() noexcept {
    releaseResources();
}

void GGUFLoader::releaseResources() noexcept {
    fd_        = 0;
    mmap_base_ = nullptr;
    mmap_size_ = 0;
    file_buffer_.clear();
}

/**
 * @brief Parse a GGUF file from @p filepath.
 *
 * Reads the entire file into a buffer and then validates:
 *   1. Magic bytes (GGUF)
 *   2. Version (uint32, expected 1–4)
 *   3. Tensor count (uint64)
 *   4. Metadata key-value count (uint64), then each KV pair
 *   5. Tensor info entries (name, ndim, shape, dtype, offset)
 *
 * @return true on a well-formed file; false with last_error_ set otherwise.
 */
bool GGUFLoader::parseFile(const std::string& filepath) {
    releaseResources();
    filepath_ = filepath;
    last_error_.clear();

    // ── Read file into buffer ─────────────────────────────────────────────
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
    if (!ifs) {
        last_error_ = "Cannot open file: " + filepath;
        return false;
    }
    const auto fsz = ifs.tellg();
    if (fsz < 0) {
        last_error_ = "Failed to determine file size";
        return false;
    }
    ifs.seekg(0, std::ios::beg);
    file_buffer_.resize(static_cast<size_t>(fsz));
    if (!ifs.read(reinterpret_cast<char*>(file_buffer_.data()),
                  static_cast<std::streamsize>(fsz))) {
        last_error_ = "Failed to read file content";
        return false;
    }

    ParseContext ctx{file_buffer_.data(),
                     static_cast<size_t>(fsz), 0};

    // ── 1. Magic ─────────────────────────────────────────────────────────
    uint8_t magic[4]{};
    if (!ctx.read(magic, 4) ||
        std::memcmp(magic, kGGUFMagic, 4) != 0) {
        last_error_ = "Not a GGUF file: invalid magic bytes";
        return false;
    }

    // ── 2. Version ───────────────────────────────────────────────────────
    uint32_t version{};
    if (!read_u32(ctx, version)) {
        last_error_ = "Truncated GGUF header: missing version";
        return false;
    }
    if (version < 1 || version > 4) {
        last_error_ = "Unsupported GGUF version: " + std::to_string(version);
        return false;
    }

    // ── 3. Tensor count ──────────────────────────────────────────────────
    uint64_t n_tensors{};
    if (!read_u64(ctx, n_tensors)) {
        last_error_ = "Truncated GGUF header: missing tensor count";
        return false;
    }
    if (n_tensors > kMaxTensorCount) {
        last_error_ = "Too many tensors in GGUF header";
        return false;
    }

    // ── 4. Metadata KV count ─────────────────────────────────────────────
    uint64_t n_kv{};
    if (!read_u64(ctx, n_kv)) {
        last_error_ = "Truncated GGUF header: missing metadata count";
        return false;
    }
    if (n_kv > kMaxMetaKVCount) {
        last_error_ = "Too many metadata entries in GGUF header";
        return false;
    }

    // ── 5. Metadata KV pairs ─────────────────────────────────────────────
    for (uint64_t i = 0; i < n_kv; ++i) {
        std::string key;
        if (!read_gguf_string(ctx, key)) {
            last_error_ = "Truncated GGUF metadata key at entry " +
                          std::to_string(i);
            return false;
        }
        uint32_t vtype{};
        if (!read_u32(ctx, vtype)) {
            last_error_ = "Truncated GGUF metadata value type at entry " +
                          std::to_string(i);
            return false;
        }
        if (!skip_gguf_value(ctx, vtype)) {
            last_error_ = "Truncated or invalid GGUF metadata value at key: " +
                          key;
            return false;
        }
        // Store simple string values in the metadata map
        metadata_.config[key] = "(parsed)";
    }

    // ── 6. Tensor info ───────────────────────────────────────────────────
    metadata_.tensors.reserve(static_cast<size_t>(n_tensors));
    for (uint64_t i = 0; i < n_tensors; ++i) {
        TensorMetadata tm;
        if (!read_gguf_string(ctx, tm.name)) {
            last_error_ = "Truncated tensor name at index " + std::to_string(i);
            return false;
        }
        uint32_t ndim{};
        if (!read_u32(ctx, ndim)) {
            last_error_ = "Truncated tensor dimension count at index " +
                          std::to_string(i);
            return false;
        }
        static constexpr uint32_t kMaxDims = 8;
        if (ndim > kMaxDims) {
            last_error_ = "Invalid tensor ndim at index " + std::to_string(i);
            return false;
        }
        tm.shape.resize(static_cast<size_t>(ndim));
        for (uint32_t d = 0; d < ndim; ++d) {
            uint64_t dim_size{};
            if (!read_u64(ctx, dim_size)) {
                last_error_ = "Truncated tensor shape at index " +
                              std::to_string(i);
                return false;
            }
            tm.shape[static_cast<size_t>(d)] =
                static_cast<int64_t>(dim_size);
        }
        uint32_t dtype{};
        if (!read_u32(ctx, dtype)) {
            last_error_ = "Truncated tensor dtype at index " +
                          std::to_string(i);
            return false;
        }
        tm.type = static_cast<GGMLType>(dtype);
        uint64_t offset{};
        if (!read_u64(ctx, offset)) {
            last_error_ = "Truncated tensor offset at index " +
                          std::to_string(i);
            return false;
        }
        tm.offset = static_cast<size_t>(offset);
        metadata_.tensors.push_back(std::move(tm));
    }

    return true;
}

// ── Stub implementations of remaining non-inline members ─────────────────────

std::string GGUFLoader::loadToThemisDB(const std::string&) {
    // No-op in standalone fuzz build — RocksDB not available.
    return {};
}

void* GGUFLoader::mmapTensor(const std::string&) {
    return nullptr;
}

void GGUFLoader::unmapTensor(void*) {
    // No-op: mmapTensor always returns nullptr in this stub.
}

std::vector<uint8_t> GGUFLoader::getTensorData(const std::string&) {
    return {};
}

bool GGUFLoader::validateQuantizationMetadata(const std::string&) const {
    return false;
}

bool GGUFLoader::isFormatSupported(GGMLType) {
    return false;
}

bool GGUFLoader::parseHeader() { return false; }
bool GGUFLoader::parseMetadataKV() { return false; }
bool GGUFLoader::parseTensorInfo() { return false; }

size_t GGUFLoader::getDtypeSize(const std::string&) const { return 0; }
size_t GGUFLoader::getGGMLTypeSize(GGMLType) const { return 0; }

bool GGUFLoader::readString(size_t&, std::string&) { return false; }

bool GGUFLoader::readMetadataValue(size_t&, GGUFValueType, std::string&) {
    return false;
}

bool GGUFLoader::storeTensorInChunks(const std::string&,
                                     const TensorMetadata&,
                                     size_t) {
    return false;
}

size_t GGUFLoader::alignOffset(size_t offset, size_t alignment) {
    if (alignment == 0) {
      return offset;
    }
    return (offset + alignment - 1) & ~(alignment - 1);
}

// ── TensorMetadata ────────────────────────────────────────────────────────────

std::string TensorMetadata::type_string() const {
    switch (type) {
    case GGMLType::F32:  return "F32";
    case GGMLType::F16:  return "F16";
    case GGMLType::Q4_0: return "Q4_0";
    case GGMLType::Q4_1: return "Q4_1";
    case GGMLType::Q5_0: return "Q5_0";
    case GGMLType::Q5_1: return "Q5_1";
    case GGMLType::Q8_0: return "Q8_0";
    case GGMLType::Q8_1: return "Q8_1";
    case GGMLType::Q2_K: return "Q2_K";
    case GGMLType::Q3_K: return "Q3_K";
    case GGMLType::Q4_K: return "Q4_K";
    case GGMLType::Q5_K: return "Q5_K";
    case GGMLType::Q6_K: return "Q6_K";
    case GGMLType::Q8_K: return "Q8_K";
    case GGMLType::I8:   return "I8";
    case GGMLType::I16:  return "I16";
    case GGMLType::I32:  return "I32";
    default:             return "UNKNOWN";
    }
}

} // namespace llm
} // namespace themis
