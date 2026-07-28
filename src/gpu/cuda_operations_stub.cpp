/**
 * @file cuda_operations_stub.cpp
 * @brief CUDA operation stubs for CPU-only and CI builds.
 *
 * Provides no-op CPU fallbacks for CUDA kernels so that the GPU
 * module compiles and runs correctness tests without a CUDA device.
 */

// Fallback implementations for CUDA types when CUDA is not available
// This file provides minimal, non-device implementations to satisfy
// link-time dependencies on machines without the CUDA Toolkit.

#include "gpu/cuda_operations.h"
#include <stdexcept>
#include <thread>

namespace themis {
namespace gpu {

CudaStream::CudaStream(int device_id, int priority)
    : stream_handle_(nullptr), device_id_(device_id), is_moved_from_(false) {
    if (device_id < 0) {
        throw std::invalid_argument("device_id must be >= 0");
    }
}

CudaStream::~CudaStream() noexcept {
    // no-op for stub
}

CudaStream::CudaStream(CudaStream&& other) noexcept
    : stream_handle_(other.stream_handle_), device_id_(other.device_id_), is_moved_from_(false) {
    other.stream_handle_ = nullptr;
    other.device_id_ = -1;
    other.is_moved_from_ = true;
}

CudaStream& CudaStream::operator=(CudaStream&& other) noexcept {
    if (this == &other) return *this;
    stream_handle_ = other.stream_handle_;
    device_id_ = other.device_id_;
    is_moved_from_ = false;
    other.stream_handle_ = nullptr;
    other.device_id_ = -1;
    other.is_moved_from_ = true;
    return *this;
}

void* CudaStream::get_handle() const {
    if (is_moved_from_) throw std::logic_error("Cannot get handle from moved-from stream");
    return stream_handle_;
}

void CudaStream::synchronize() const {
    if (is_moved_from_) throw std::logic_error("Cannot synchronize moved-from stream");
    // no-op in stub
}

bool CudaStream::is_ready() const noexcept {
    if (is_moved_from_ || !stream_handle_) return true;
    return true;
}

bool CudaStream::is_valid() const noexcept { return !is_moved_from_ && stream_handle_ == nullptr ? true : !is_moved_from_; }

bool CudaStream::is_moved_from() const noexcept { return is_moved_from_; }

// --- CudaOperation ---

CudaOperation::CudaOperation(const CudaStream& stream, const std::string& name)
    : event_handle_(nullptr), stream_(&stream), name_(name), status_(Status::PENDING), is_moved_from_(false) {
    if (!stream.is_valid()) {
        throw std::invalid_argument("Invalid CUDA stream provided");
    }
}

CudaOperation::~CudaOperation() noexcept {
    // no-op
}

CudaOperation::CudaOperation(CudaOperation&& other) noexcept
    : event_handle_(other.event_handle_), stream_(other.stream_), name_(std::move(other.name_)),
      error_msg_(std::move(other.error_msg_)), status_(other.status_), is_moved_from_(false) {
    other.event_handle_ = nullptr;
    other.stream_ = nullptr;
    other.status_ = Status::MOVED_FROM;
    other.is_moved_from_ = true;
}

CudaOperation& CudaOperation::operator=(CudaOperation&& other) noexcept {
    if (this == &other) return *this;
    event_handle_ = other.event_handle_;
    stream_ = other.stream_;
    name_ = std::move(other.name_);
    error_msg_ = std::move(other.error_msg_);
    status_ = other.status_;
    is_moved_from_ = false;
    other.event_handle_ = nullptr;
    other.stream_ = nullptr;
    other.status_ = Status::MOVED_FROM;
    other.is_moved_from_ = true;
    return *this;
}

void CudaOperation::record_event() {
    if (is_moved_from_) throw std::logic_error("Cannot record event on moved-from operation");
    if (!stream_ || !stream_->is_valid()) throw std::logic_error("Invalid stream for event recording");
    status_ = Status::RUNNING;
    // immediate completion in stub
    status_ = Status::COMPLETED;
}

bool CudaOperation::wait(std::chrono::milliseconds timeout) {
    if (is_moved_from_) throw std::logic_error("Cannot wait on moved-from operation");
    status_ = Status::COMPLETED;
    return true;
}

CudaOperation::Status CudaOperation::get_status() const noexcept { return status_; }

void CudaOperation::mark_completed() noexcept { status_ = Status::COMPLETED; }

void CudaOperation::mark_failed(const std::string& error_msg) noexcept { status_ = Status::FAILED; error_msg_ = error_msg; }

const std::string& CudaOperation::get_name() const noexcept { static const std::string empty; return is_moved_from_ ? empty : name_; }

const std::string& CudaOperation::get_error() const noexcept { return error_msg_; }

int CudaOperation::get_device_id() const noexcept { return is_moved_from_ || !stream_ ? -1 : 0; }

bool CudaOperation::is_moved_from() const noexcept { return is_moved_from_; }

// --- CudaOperationBatch ---

CudaOperationBatch::CudaOperationBatch(const CudaStream& stream)
    : stream_(&stream), is_moved_from_(false) {
    if (!stream.is_valid()) {
        throw std::invalid_argument("Invalid CUDA stream provided");
    }
}

CudaOperationBatch::~CudaOperationBatch() noexcept {
    // best-effort in stub
}

CudaOperationBatch::CudaOperationBatch(CudaOperationBatch&& other) noexcept
    : stream_(other.stream_), operations_(std::move(other.operations_)), is_moved_from_(false) {
    other.stream_ = nullptr;
    other.is_moved_from_ = true;
}

CudaOperationBatch& CudaOperationBatch::operator=(CudaOperationBatch&& other) noexcept {
    if (this == &other) return *this;
    stream_ = other.stream_;
    operations_ = std::move(other.operations_);
    is_moved_from_ = false;
    other.stream_ = nullptr;
    other.is_moved_from_ = true;
    return *this;
}

void CudaOperationBatch::add_operation(CudaOperation&& op) {
    if (is_moved_from_) throw std::logic_error("Cannot add operation to moved-from batch");
    if (op.is_moved_from()) throw std::invalid_argument("Cannot add moved-from operation to batch");
    operations_.push_back(std::move(op));
}

bool CudaOperationBatch::wait_all(std::chrono::milliseconds timeout) {
    if (is_moved_from_) throw std::logic_error("Cannot wait on moved-from batch");
    for (auto& op : operations_) op.mark_completed();
    return true;
}

size_t CudaOperationBatch::size() const noexcept { return is_moved_from_ ? 0 : operations_.size(); }

bool CudaOperationBatch::all_completed() const noexcept { return is_moved_from_ ? true : true; }

size_t CudaOperationBatch::failed_count() const noexcept { return 0; }

bool CudaOperationBatch::is_valid() const noexcept { return !is_moved_from_ && stream_ != nullptr; }

bool CudaOperationBatch::is_moved_from() const noexcept { return is_moved_from_; }

} // namespace gpu
} // namespace themis
