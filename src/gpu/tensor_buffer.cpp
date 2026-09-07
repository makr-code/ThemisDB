/**
 * @file tensor_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/tensor_buffer.h"

#include <cstring>
#include <stdexcept>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------
std::mutex GPUTensorBuffer::stats_mutex_;
GPUTensorBuffer::Stats GPUTensorBuffer::global_stats_;

// ---------------------------------------------------------------------------
// Shape helpers
// ---------------------------------------------------------------------------

size_t GPUTensorBuffer::Shape::numElements() const noexcept {
    if (dims.empty()) {
        return 0;
    }
    size_t n = 1;
    for (size_t d : dims) {
        n *= d;
    }
    return n;
}

size_t GPUTensorBuffer::Shape::elementBytes(DType dtype) noexcept {
    switch (dtype) {
        case DType::FLOAT32:
            return 4;
        case DType::FLOAT16:
            return 2;
        case DType::BFLOAT16:
            return 2;
        case DType::INT32:
            return 4;
        case DType::INT8:
            return 1;
        case DType::UINT8:
            return 1;
    }
    return 4;
}

size_t GPUTensorBuffer::Shape::totalBytes(DType dtype) const noexcept {
    return numElements() * elementBytes(dtype);
}

bool GPUTensorBuffer::Shape::operator==(const Shape &other) const noexcept {
    return dims == other.dims;
}

bool GPUTensorBuffer::Shape::operator!=(const Shape &other) const noexcept {
    return !(*this == other);
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

GPUTensorBuffer::GPUTensorBuffer(std::string name, const Shape &shape, DType dtype)
    : name_(std::move(name)), shape_(shape), dtype_(dtype), data_(shape.totalBytes(dtype), 0) {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    ++global_stats_.total_buffers_created;
    global_stats_.current_bytes += data_.size();
    if (global_stats_.current_bytes > global_stats_.peak_bytes)
        global_stats_.peak_bytes = global_stats_.current_bytes;
}

GPUTensorBuffer::GPUTensorBuffer(GPUTensorBuffer &&other) noexcept
    : name_(std::move(other.name_)), shape_(std::move(other.shape_)), dtype_(other.dtype_),
      data_(std::move(other.data_)) {}

GPUTensorBuffer &GPUTensorBuffer::operator=(GPUTensorBuffer &&other) noexcept {
    if (this != &other) {
        // Release current bytes from global stats
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            if (global_stats_.current_bytes >= data_.size()) {
                global_stats_.current_bytes -= data_.size();
            }
            ++global_stats_.total_buffers_freed;
        }
        name_  = std::move(other.name_);
        shape_ = std::move(other.shape_);
        dtype_ = other.dtype_;
        data_  = std::move(other.data_);
    }
    return *this;
}

GPUTensorBuffer::~GPUTensorBuffer() {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    if (global_stats_.current_bytes >= data_.size()) {
        global_stats_.current_bytes -= data_.size();
    }
    ++global_stats_.total_buffers_freed;
}

// ---------------------------------------------------------------------------
// totalBytes
// ---------------------------------------------------------------------------

size_t GPUTensorBuffer::totalBytes() const noexcept {
    return static_cast<int>(data_.size());
}

// ---------------------------------------------------------------------------
// fill
// ---------------------------------------------------------------------------

void GPUTensorBuffer::fill([[maybe_unused]] double value) {
    std::lock_guard<std::mutex> lk(mutex_);
    size_t elem_bytes = Shape::elementBytes(dtype_);
    size_t n          = shape_.numElements();

    for (size_t i = 0; i < n; ++i) {
        uint8_t *dest = data_.data() + i * elem_bytes;
        switch (dtype_) {
            case DType::FLOAT32: {
                float v = static_cast<float>(value);
                std::memcpy(dest, &v, 4);
                break;
            }
            case DType::FLOAT16: {
                // Encode as IEEE 754 half-precision (10-bit mantissa + 5-bit exponent).
                float f32 = static_cast<float>(value);
                uint32_t b32 = {};
                std::memcpy(&b32, &f32, 4);
                const uint32_t sign   = (b32 >> 31) & 0x1u;
                const int32_t exp32   = static_cast<int32_t>((b32 >> 23) & 0xFFu) - 127;
                const uint32_t mant32 = b32 & 0x7FFFFFu;
                uint16_t v = {};
                if (exp32 == 128) {
                    v = static_cast<uint16_t>((sign << 15) | 0x7C00u | (mant32 ? 0x0200u : 0));
                } else if (exp32 < -24) {
                    v = static_cast<uint16_t>(sign << 15);
                } else if (exp32 < -14) {
                    uint32_t shift  = static_cast<uint32_t>(-14 - exp32);
                    uint32_t mant16 = (mant32 | 0x800000u) >> (shift + 13);
                    v               = static_cast<uint16_t>((sign << 15) | mant16);
                } else if (exp32 > 15) {
                    v = static_cast<uint16_t>((sign << 15) | 0x7C00u);
                } else {
                    uint32_t exp16  = static_cast<uint32_t>(exp32 + 15);
                    uint32_t mant16 = mant32 >> 13;
                    uint32_t round  = mant32 & 0x1FFFu;
                    if (round > 0x1000u || (round == 0x1000u && (mant16 & 1))) {
                        ++mant16;
                    }
                    if (mant16 >= 0x400u) {
                        ++exp16;
                        mant16 = 0;
                    }
                    v = static_cast<uint16_t>((sign << 15) | (exp16 << 10) | (mant16 & 0x3FFu));
                }
                std::memcpy(dest, &v, 2);
                break;
            }
            case DType::BFLOAT16: {
                // BF16 = top 16 bits of the float32 bit pattern (with round-to-nearest).
                float f = static_cast<float>(value);
                uint32_t bits = {};
                std::memcpy(&bits, &f, 4);
                // Round to nearest even by adding 0x7FFF + ((bits >> 16) & 1).
                bits += 0x7FFFu + ((bits >> 16) & 1);
                uint16_t v = static_cast<uint16_t>(bits >> 16);
                std::memcpy(dest, &v, 2);
                break;
            }
            case DType::INT32: {
                int32_t v = static_cast<int32_t>(value);
                std::memcpy(dest, &v, 4);
                break;
            }
            case DType::INT8: {
                int8_t v = static_cast<int8_t>(value);
                std::memcpy(dest, &v, 1);
                break;
            }
            case DType::UINT8: {
                uint8_t v = static_cast<uint8_t>(value);
                *dest     = v;
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// copyFromHost / copyToHost
// ---------------------------------------------------------------------------

void GPUTensorBuffer::copyFromHost(const void *src, size_t bytes) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (bytes > static_cast<int>(data_.size())) {
        throw std::out_of_range("GPUTensorBuffer::copyFromHost: bytes > buffer size");
    }
    std::memcpy(data_.data(), src, bytes);
}

void GPUTensorBuffer::copyToHost(void *dst, size_t bytes) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (bytes > static_cast<int>(data_.size())) {
        throw std::out_of_range("GPUTensorBuffer::copyToHost: bytes > buffer size");
    }
    std::memcpy(dst, data_.data(), bytes);
}

// ---------------------------------------------------------------------------
// createView
// ---------------------------------------------------------------------------

GPUTensorBuffer::View GPUTensorBuffer::createView(const std::string &view_name, size_t offset_elements,
                                                  const Shape &view_shape) const {
    View v;
    v.name         = view_name;
    v.offset_bytes = offset_elements * Shape::elementBytes(dtype_);
    v.shape        = view_shape;
    v.dtype        = dtype_;

    std::lock_guard<std::mutex> lk(stats_mutex_);
    ++global_stats_.total_views_created;

    return v;
}

// ---------------------------------------------------------------------------
// Serialisation
// ---------------------------------------------------------------------------
//
// Format (little-endian):
//   [4]  magic: 0x54454E53 ("TENS")
//   [4]  dtype (uint32_t)
//   [4]  ndim  (uint32_t)
//   [4*ndim] dims (uint32_t each)
//   [4]  name length
//   [name_len] name bytes
//   [data_size] raw data bytes

static void write32(std::vector<uint8_t> &buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v));
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v >> 16));
    buf.push_back(static_cast<uint8_t>(v >> 24));
}

static uint32_t read32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8 | static_cast<uint32_t>(p[2]) << 16
           | static_cast<uint32_t>(p[3]) << 24;
}

std::vector<uint8_t> GPUTensorBuffer::serialize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<uint8_t> out = {};

    out.reserve(16 + 4 * shape_.dims.size() + static_cast<int>(name_.size()) + static_cast<int>(data_.size()) );

    write32(out, 0x54454E53u); // magic
    write32(out, static_cast<uint32_t>(dtype_));
    write32(out, static_cast<uint32_t>(shape_.dims.size()));
    for (size_t d : shape_.dims) {
        write32(out, static_cast<uint32_t>(d));
    }
    write32(out, static_cast<uint32_t>(name_.size()));
    out.insert(out.end(), name_.begin(), name_.end());
    out.insert(out.end(), data_.begin(), data_.end());
    return out;
}

GPUTensorBuffer GPUTensorBuffer::deserialize(const std::vector<uint8_t> &bytes) {
    try {
        const uint8_t *p   = bytes.data();
        const uint8_t *end = p + static_cast<int>(bytes.size()) ;

        auto need = [&]([[maybe_unused]] size_t n) {
            if (p + n > end) {
                throw std::runtime_error("GPUTensorBuffer::deserialize: truncated data");
            }
        };

        need(4);
        uint32_t magic = read32(p);
        p += 4;
        if (magic != 0x54454E53u) {
            throw std::runtime_error("GPUTensorBuffer::deserialize: bad magic");
        }

        need(4);
        DType dtype = static_cast<DType>(read32(p));
        p += 4;

        need(4);
        uint32_t ndim = read32(p);
        p += 4;

        Shape shape;
        for (uint32_t i = 0; i < ndim; ++i) {
            need(4);
            shape.dims.push_back(read32(p));
            p += 4;
        }

        need(4);
        uint32_t name_len = read32(p);
        p += 4;
        need(name_len);
        std::string name(reinterpret_cast<const char *>(p), name_len);
        p += name_len;

        size_t data_size = shape.totalBytes(dtype);
        need(data_size);

        GPUTensorBuffer buf(std::move(name), shape, dtype);
        std::memcpy(buf.data_.data(), p, data_size);
        return buf;
    } catch (const std::runtime_error &) {
        // Re-throw runtime errors (validation failures) as-is
        throw;
    } catch (const std::exception &e) {
        // Wrap other exceptions to preserve context
        throw std::runtime_error(std::string("GPUTensorBuffer::deserialize failed: ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// Global stats
// ---------------------------------------------------------------------------

GPUTensorBuffer::Stats GPUTensorBuffer::getGlobalStats() {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return global_stats_;
}

void GPUTensorBuffer::resetGlobalStats() {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    global_stats_ = Stats{};
}

} // namespace gpu
} // namespace themis
