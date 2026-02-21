/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_buffer.cpp                                  ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     306                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "themis/gpu/tensor_buffer.h"

#include <cstring>
#include <stdexcept>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------
std::mutex          GPUTensorBuffer::stats_mutex_;
GPUTensorBuffer::Stats GPUTensorBuffer::global_stats_;

// ---------------------------------------------------------------------------
// Shape helpers
// ---------------------------------------------------------------------------

size_t GPUTensorBuffer::Shape::numElements() const noexcept {
    if (dims.empty()) return 0;
    size_t n = 1;
    for (size_t d : dims) n *= d;
    return n;
}

size_t GPUTensorBuffer::Shape::elementBytes(DType dtype) noexcept {
    switch (dtype) {
        case DType::FLOAT32: return 4;
        case DType::FLOAT16: return 2;
        case DType::INT32:   return 4;
        case DType::INT8:    return 1;
        case DType::UINT8:   return 1;
    }
    return 4;
}

size_t GPUTensorBuffer::Shape::totalBytes(DType dtype) const noexcept {
    return numElements() * elementBytes(dtype);
}

bool GPUTensorBuffer::Shape::operator==(const Shape& other) const noexcept {
    return dims == other.dims;
}

bool GPUTensorBuffer::Shape::operator!=(const Shape& other) const noexcept {
    return !(*this == other);
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

GPUTensorBuffer::GPUTensorBuffer(std::string name, const Shape& shape, DType dtype)
    : name_(std::move(name))
    , shape_(shape)
    , dtype_(dtype)
    , data_(shape.totalBytes(dtype), 0) {

    std::lock_guard<std::mutex> lk(stats_mutex_);
    ++global_stats_.total_buffers_created;
    global_stats_.current_bytes += data_.size();
    if (global_stats_.current_bytes > global_stats_.peak_bytes)
        global_stats_.peak_bytes = global_stats_.current_bytes;
}

GPUTensorBuffer::GPUTensorBuffer(GPUTensorBuffer&& other) noexcept
    : name_(std::move(other.name_))
    , shape_(std::move(other.shape_))
    , dtype_(other.dtype_)
    , data_(std::move(other.data_)) {}

GPUTensorBuffer& GPUTensorBuffer::operator=(GPUTensorBuffer&& other) noexcept {
    if (this != &other) {
        // Release current bytes from global stats
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            if (global_stats_.current_bytes >= data_.size())
                global_stats_.current_bytes -= data_.size();
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
    if (global_stats_.current_bytes >= data_.size())
        global_stats_.current_bytes -= data_.size();
    ++global_stats_.total_buffers_freed;
}

// ---------------------------------------------------------------------------
// totalBytes
// ---------------------------------------------------------------------------

size_t GPUTensorBuffer::totalBytes() const noexcept {
    return data_.size();
}

// ---------------------------------------------------------------------------
// fill
// ---------------------------------------------------------------------------

void GPUTensorBuffer::fill(double value) {
    std::lock_guard<std::mutex> lk(mutex_);
    size_t elem_bytes = Shape::elementBytes(dtype_);
    size_t n = shape_.numElements();

    for (size_t i = 0; i < n; ++i) {
        uint8_t* dest = data_.data() + i * elem_bytes;
        switch (dtype_) {
            case DType::FLOAT32: {
                float v = static_cast<float>(value);
                std::memcpy(dest, &v, 4);
                break;
            }
            case DType::FLOAT16: {
                // Store raw bits of float cast to 16-bit placeholder.
                uint16_t v = static_cast<uint16_t>(static_cast<float>(value));
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
                *dest = v;
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// copyFromHost / copyToHost
// ---------------------------------------------------------------------------

void GPUTensorBuffer::copyFromHost(const void* src, size_t bytes) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (bytes > data_.size())
        throw std::out_of_range("GPUTensorBuffer::copyFromHost: bytes > buffer size");
    std::memcpy(data_.data(), src, bytes);
}

void GPUTensorBuffer::copyToHost(void* dst, size_t bytes) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (bytes > data_.size())
        throw std::out_of_range("GPUTensorBuffer::copyToHost: bytes > buffer size");
    std::memcpy(dst, data_.data(), bytes);
}

// ---------------------------------------------------------------------------
// createView
// ---------------------------------------------------------------------------

GPUTensorBuffer::View GPUTensorBuffer::createView(const std::string& view_name,
                                                   size_t             offset_elements,
                                                   const Shape&       view_shape) const {
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

static void write32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v));
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v >> 16));
    buf.push_back(static_cast<uint8_t>(v >> 24));
}

static uint32_t read32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | static_cast<uint32_t>(p[1]) << 8
         | static_cast<uint32_t>(p[2]) << 16
         | static_cast<uint32_t>(p[3]) << 24;
}

std::vector<uint8_t> GPUTensorBuffer::serialize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<uint8_t> out;
    out.reserve(16 + 4 * shape_.dims.size() + name_.size() + data_.size());

    write32(out, 0x54454E53u);  // magic
    write32(out, static_cast<uint32_t>(dtype_));
    write32(out, static_cast<uint32_t>(shape_.dims.size()));
    for (size_t d : shape_.dims) write32(out, static_cast<uint32_t>(d));
    write32(out, static_cast<uint32_t>(name_.size()));
    out.insert(out.end(), name_.begin(), name_.end());
    out.insert(out.end(), data_.begin(), data_.end());
    return out;
}

GPUTensorBuffer GPUTensorBuffer::deserialize(const std::vector<uint8_t>& bytes) {
    const uint8_t* p   = bytes.data();
    const uint8_t* end = p + bytes.size();

    auto need = [&](size_t n) {
        if (p + n > end)
            throw std::runtime_error("GPUTensorBuffer::deserialize: truncated data");
    };

    need(4);
    uint32_t magic = read32(p); p += 4;
    if (magic != 0x54454E53u)
        throw std::runtime_error("GPUTensorBuffer::deserialize: bad magic");

    need(4);
    DType dtype = static_cast<DType>(read32(p)); p += 4;

    need(4);
    uint32_t ndim = read32(p); p += 4;

    Shape shape;
    for (uint32_t i = 0; i < ndim; ++i) {
        need(4);
        shape.dims.push_back(read32(p)); p += 4;
    }

    need(4);
    uint32_t name_len = read32(p); p += 4;
    need(name_len);
    std::string name(reinterpret_cast<const char*>(p), name_len);
    p += name_len;

    size_t data_size = shape.totalBytes(dtype);
    need(data_size);

    GPUTensorBuffer buf(std::move(name), shape, dtype);
    std::memcpy(buf.data_.data(), p, data_size);
    return buf;
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
