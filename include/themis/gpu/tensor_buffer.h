/**
 * @file tensor_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Data type tag for tensor elements.
 */
enum class DType {
    FLOAT32,   ///< 4 bytes
    FLOAT16,   ///< 2 bytes (IEEE 754 half-precision, host-side stored as uint16_t)
    BFLOAT16,  ///< 2 bytes (bfloat16: top 16 bits of float32, host-side as uint16_t)
    INT32,     ///< 4 bytes
    INT8,      ///< 1 byte
    UINT8,     ///< 1 byte
};

/**
 * @brief Multi-dimensional typed tensor backed by host-side storage.
 *
 * `GPUTensorBuffer` provides a safe, self-describing container for tensor
 * data that can be passed to GPU operations without exposing raw pointers.
 *
 * Design notes
 * ------------
 * - Storage is host-side (`std::vector<uint8_t>`) so the buffer is always
 *   usable without real GPU hardware.  A real implementation would add a
 *   `device_ptr_` member populated by `cudaMalloc` / `hipMalloc` and
 *   synchronised via `uploadToDevice()` / `downloadFromDevice()`.
 * - Named views describe logical sub-tensors (slices, heads, layers) without
 *   copying data.
 * - Serialisation / deserialisation supports checkpoint/restore workflows.
 * - Global statistics are tracked for memory-pressure monitoring.
 *
 * Thread safety: all instance methods except constructors are protected by
 * an internal per-instance mutex.  Global stats use a separate mutex.
 */
class GPUTensorBuffer {
public:
    // -----------------------------------------------------------------------
    // Shape
    // -----------------------------------------------------------------------
    struct Shape {
        std::vector<size_t> dims;

        /// Total number of elements (product of all dims).
        size_t numElements() const noexcept;

        /// Size in bytes of a single element of @p dtype.
        static size_t elementBytes(DType dtype) noexcept;

        /// Total bytes required for this shape and dtype.
        size_t totalBytes(DType dtype) const noexcept;

        bool operator==(const Shape& other) const noexcept;
        bool operator!=(const Shape& other) const noexcept;
    };

    // -----------------------------------------------------------------------
    // Named view (logical sub-tensor)
    // -----------------------------------------------------------------------
    struct View {
        std::string name;
        size_t      offset_bytes = 0;  ///< Byte offset within the parent buffer
        Shape       shape;
        DType       dtype = DType::FLOAT32;
    };

    // -----------------------------------------------------------------------
    // Global statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t   total_buffers_created = 0;
        size_t   total_buffers_freed   = 0;
        size_t   total_views_created   = 0;
        uint64_t peak_bytes            = 0;
        uint64_t current_bytes         = 0;
    };

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /**
     * @param name   Human-readable label (used in diagnostics and serialise).
     * @param shape  Tensor shape.
     * @param dtype  Element data type.
     */
    GPUTensorBuffer(std::string name, const Shape& shape, DType dtype);
    GPUTensorBuffer(GPUTensorBuffer&&) noexcept;
    GPUTensorBuffer& operator=(GPUTensorBuffer&&) noexcept;
    ~GPUTensorBuffer();

    // Non-copyable (tensors own their backing store).
    GPUTensorBuffer(const GPUTensorBuffer&) = delete;
    GPUTensorBuffer& operator=(const GPUTensorBuffer&) = delete;

    // -----------------------------------------------------------------------
    // Data access
    // -----------------------------------------------------------------------

    /**
     * @brief Fill the entire buffer with a constant scalar.
     *
     * The double @p value is cast to the buffer's DType.
     */
    void fill(double value);

    /**
     * @brief Copy @p bytes bytes from @p src into the buffer.
     *
     * @pre bytes <= totalBytes()
     */
    void copyFromHost(const void* src, size_t bytes);

    /**
     * @brief Copy @p bytes bytes from the buffer into @p dst.
     *
     * @pre bytes <= totalBytes()
     */
    void copyToHost(void* dst, size_t bytes) const;

    // -----------------------------------------------------------------------
    // Views
    // -----------------------------------------------------------------------

    /**
     * @brief Create a named logical view over a slice of this buffer.
     *
     * @param view_name       Label for the view.
     * @param offset_elements Element offset from the start of this buffer.
     * @param view_shape      Shape of the sub-tensor.
     * @return View descriptor.
     */
    View createView(const std::string& view_name,
                    size_t             offset_elements,
                    const Shape&       view_shape) const;

    // -----------------------------------------------------------------------
    // Properties
    // -----------------------------------------------------------------------
    const std::string& name()       const noexcept { return name_;  }
    const Shape&       shape()      const noexcept { return shape_; }
    DType              dtype()      const noexcept { return dtype_; }
    size_t             totalBytes() const noexcept;
    bool               isValid()    const noexcept { return !data_.empty(); }

    // Raw read-only access for testing / serialisation.
    const uint8_t* rawData()   const noexcept { return data_.data();  }
    size_t         rawBytes()  const noexcept { return data_.size();  }

    // -----------------------------------------------------------------------
    // Serialisation
    // -----------------------------------------------------------------------

    /**
     * @brief Serialise the buffer (header + raw bytes) for checkpointing.
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Reconstruct a GPUTensorBuffer from serialised bytes.
     *
     * @throws std::runtime_error on corrupt data.
     */
    static GPUTensorBuffer deserialize(const std::vector<uint8_t>& bytes);

    // -----------------------------------------------------------------------
    // Global stats
    // -----------------------------------------------------------------------
    static Stats getGlobalStats();
    static void  resetGlobalStats();

private:
    std::string          name_;
    Shape                shape_;
    DType                dtype_;
    std::vector<uint8_t> data_;   ///< host-side backing store
    mutable std::mutex   mutex_;

    static std::mutex stats_mutex_;
    static Stats      global_stats_;
};

} // namespace gpu
} // namespace themis
