/**
 * @file wasm_plugin_sandbox.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB WASM Plugin Sandbox – Implementation
//
// Provides memory-safe, capability-controlled isolation for untrusted WASM
// plugin code.  See include/themis/base/wasm_plugin_sandbox.h for the API.

#include "themis/base/wasm_plugin_sandbox.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace modules {

// =============================================================================
// WASM binary format constants
// =============================================================================

// WebAssembly binary format magic bytes: "\0asm"
static constexpr uint8_t kWasmMagic[4]   = {0x00, 0x61, 0x73, 0x6d};
static constexpr uint8_t kWasmVersion[4] = {0x01, 0x00, 0x00, 0x00}; // version 1

// WASM section IDs
static constexpr uint8_t kSectionCustom = 0;
static constexpr uint8_t kSectionImport = 2;
static constexpr uint8_t kSectionExport = 7;

// WASM external kinds
static constexpr uint8_t kExternalFunc   = 0;
static constexpr uint8_t kExternalTable  = 1;
static constexpr uint8_t kExternalMemory = 2;
static constexpr uint8_t kExternalGlobal = 3;

// Default WASM page size: 64 KiB
static constexpr size_t kWasmPageBytes = 65536;

// =============================================================================
// Helpers: LEB-128 unsigned integer decoder
// =============================================================================

/**
 * @brief Read a WASM unsigned LEB-128 integer.
 * @param data   Pointer to the first byte.
 * @param end    One-past-end of the available buffer.
 * @param out    Decoded value.
 * @return Number of bytes consumed, or 0 on error.
 */
static size_t readUleb128(const uint8_t *data, const uint8_t *end, uint64_t &out) noexcept {
    out             = 0;
    int shift       = 0;
    size_t consumed = 0;
    while (data < end) {
        uint8_t byte = *data++;
        consumed++;
        out |= static_cast<uint64_t>(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            return consumed;
        }
        shift += 7;
        if (shift >= 64) {
            return 0; // overflow guard
        }
    }
    return 0; // unterminated LEB-128
}

/**
 * @brief Read a WASM UTF-8 name (u32 length followed by bytes).
 * @return Number of bytes consumed, or 0 on error.
 */
static size_t readWasmName(const uint8_t *data, const uint8_t *end, std::string &out) noexcept {
    uint64_t len = 0;
    size_t hdr   = readUleb128(data, end, len);
    if (hdr == 0 || data + hdr + len > end) {
        return 0;
    }
    out.assign(reinterpret_cast<const char *>(data + hdr), static_cast<size_t>(len));
    return hdr + static_cast<size_t>(len);
}

// =============================================================================
// WasmModuleInfo::summary
// =============================================================================

std::string WasmModuleInfo::summary() const {
    std::ostringstream oss = {};
    oss << (valid ? "valid" : "invalid") << " wasm v" << wasm_version << " size=" << byte_size << "B"
        << " imports=" << imports.size() << " exports=" << exports.size();
    if (!module_name.empty()) {
        oss << " name=\"" << module_name << "\"";
    }
    return oss.str();
}

// =============================================================================
// WasmModuleValidator
// =============================================================================

/*static*/ const uint8_t *WasmModuleValidator::magicBytes() noexcept {
    return kWasmMagic;
}

/*static*/ WasmModuleInfo WasmModuleValidator::validate(const std::vector<uint8_t> &bytes) {
    WasmModuleInfo info;
    info.byte_size = bytes.size();

    if (bytes.size() < 8) {
        return info; // Too small to be a valid WASM binary
    }

    // Check magic
    if (std::memcmp(bytes.data(), kWasmMagic, 4) != 0) {
        return info;
    }

    // Check version
    if (std::memcmp(bytes.data() + 4, kWasmVersion, 4) != 0) {
        // Accept but note non-standard version
        info.wasm_version = static_cast<uint32_t>(bytes[4]) | (static_cast<uint32_t>(bytes[5]) << 8)
                            | (static_cast<uint32_t>(bytes[6]) << 16) | (static_cast<uint32_t>(bytes[7]) << 24);
    } else {
        info.wasm_version = 1;
    }

    info.valid = true;

    // Parse sections to collect imports and exports
    const uint8_t *p   = bytes.data() + 8;
    const uint8_t *end = bytes.data() + bytes.size();

    while (p < end) {
        if (p + 1 > end) {
            break;
        }
        uint8_t section_id = *p++;

        uint64_t section_size = 0;
        size_t sz             = readUleb128(p, end, section_size);
        if (sz == 0) {
            break;
        }
        p += sz;

        const uint8_t *section_end = p + static_cast<size_t>(section_size);
        if (section_end > end) {
            break;
        }

        if (section_id == kSectionImport) {
            uint64_t count = 0;
            size_t c       = readUleb128(p, section_end, count);
            if (c == 0) {
                p = section_end;
                continue;
            }
            const uint8_t *q = p + c;
            for (uint64_t i = 0; i < count && q < section_end; ++i) {
                std::string mod_name, fn_name;
                size_t mn = readWasmName(q, section_end, mod_name);
                if (mn == 0) {
                    break;
                }
                q += mn;
                size_t fn = readWasmName(q, section_end, fn_name);
                if (fn == 0) {
                    break;
                }
                q += fn;
                // Skip the import description (kind byte + descriptor)
                if (q >= section_end) {
                    break;
                }
                uint8_t kind = *q++;
                if (kind == kExternalFunc) {
                    // Function import: descriptor is a type index (LEB-128 u32)
                    uint64_t type_idx = 0;
                    size_t ti         = readUleb128(q, section_end, type_idx);
                    if (ti == 0) {
                        break;
                    }
                    q += ti;
                    info.imports.push_back(mod_name + "." + fn_name);
                } else if (kind == kExternalTable) {
                    // Table import: reftype (1 byte) + limits
                    if (q >= section_end) {
                        break;
                    }
                    q++; // skip reftype
                    if (q >= section_end) {
                        break;
                    }
                    uint8_t flags    = *q++;
                    uint64_t lim_min = 0;
                    size_t ms        = readUleb128(q, section_end, lim_min);
                    if (ms == 0) {
                        break;
                    }
                    q += ms;
                    if (flags & 0x01) { // has maximum
                        uint64_t lim_max = 0;
                        size_t xs        = readUleb128(q, section_end, lim_max);
                        if (xs == 0) {
                            break;
                        }
                        q += xs;
                    }
                    // Non-function: not host-callable; skip without recording
                } else if (kind == kExternalMemory) {
                    // Memory import: limits (flags byte + min LEB-128 + optional max LEB-128)
                    if (q >= section_end) {
                        break;
                    }
                    uint8_t flags    = *q++;
                    uint64_t lim_min = 0;
                    size_t ms        = readUleb128(q, section_end, lim_min);
                    if (ms == 0) {
                        break;
                    }
                    q += ms;
                    if (flags & 0x01) { // has maximum
                        uint64_t lim_max = 0;
                        size_t xs        = readUleb128(q, section_end, lim_max);
                        if (xs == 0) {
                            break;
                        }
                        q += xs;
                    }
                    // Non-function: not host-callable; skip without recording
                } else if (kind == kExternalGlobal) {
                    // Global import: valtype (1 byte) + mutability (1 byte)
                    if (q + 2 > section_end) {
                        break;
                    }
                    q += 2; // skip valtype + mutability
                    // Non-function: not host-callable; skip without recording
                } else {
                    // Unknown import kind; cannot safely skip — stop parsing
                    break;
                }
            }
        } else if (section_id == kSectionExport) {
            uint64_t count = 0;
            size_t c       = readUleb128(p, section_end, count);
            if (c == 0) {
                p = section_end;
                continue;
            }
            const uint8_t *q = p + c;
            for (uint64_t i = 0; i < count && q < section_end; ++i) {
                std::string exp_name = {};
                size_t en = readWasmName(q, section_end, exp_name);
                if (en == 0) {
                    break;
                }
                q += en;
                // kind byte + index LEB-128
                if (q >= section_end) {
                    break;
                }
                q++; // kind
                uint64_t idx = 0;
                size_t is    = readUleb128(q, section_end, idx);
                q += is;
                info.exports.push_back(exp_name);
            }
        } else if (section_id == kSectionCustom) {
            // Custom section: first field is a name
            const uint8_t *q = p;
            std::string custom_name = {};
            size_t cn = readWasmName(q, section_end, custom_name);
            if (cn > 0 && custom_name == "name") {
                // Parse the module name subsection (id=0)
                q += cn;
                while (q < section_end) {
                    if (q + 1 > section_end) {
                        break;
                    }
                    uint8_t sub_id    = *q++;
                    uint64_t sub_size = 0;
                    size_t ss         = readUleb128(q, section_end, sub_size);
                    if (ss == 0) {
                        break;
                    }
                    q += ss;
                    if (sub_id == 0 && sub_size > 0) {
                        // Module name subsection: one wasm name
                        readWasmName(q, section_end, info.module_name);
                        break;
                    }
                    q += static_cast<size_t>(sub_size);
                }
            }
        }

        p = section_end;
    }

    return info;
}

/*static*/ WasmModuleInfo WasmModuleValidator::validateFile(const std::string &path) {
    WasmModuleInfo info;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return info;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        return info;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(bytes.data()), size)) {
        return info;
    }

    return validate(bytes);
}

// =============================================================================
// WasmPluginSandbox – constructor / destructor
// =============================================================================

WasmPluginSandbox::WasmPluginSandbox(const Config &config) : config_(config) {}

WasmPluginSandbox::~WasmPluginSandbox() {
    unload();
}

// =============================================================================
// Runtime injection
// =============================================================================

void WasmPluginSandbox::setRuntime(std::unique_ptr<WasmRuntime> runtime) {
    runtime_ = std::move(runtime);
}

bool WasmPluginSandbox::hasRuntime() const noexcept {
    return runtime_ != nullptr;
}

std::string WasmPluginSandbox::engineName() const {
    if (!runtime_) {
        return std::string{};
    }
    return runtime_->engineName();
}

// =============================================================================
// Host-function allowlist
// =============================================================================

void WasmPluginSandbox::addHostFunction(WasmHostFunction fn) {
    host_fns_.push_back(std::move(fn));
}

void WasmPluginSandbox::clearHostFunctions() {
    host_fns_.clear();
}

size_t WasmPluginSandbox::hostFunctionCount() const noexcept {
    return host_fns_.size();
}

// =============================================================================
// Loading from file
// =============================================================================

bool WasmPluginSandbox::loadFromFile(const std::string &path) {
    last_error_.clear();
    load_warnings_.clear();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        // GAP-FIX sensitive_data_logging: keep the full path in last_error_ for the
        // caller but do not broadcast it to the log at ERROR level — file paths
        // are treated as sensitive per MODULE_GAPS.md.
        last_error_ = "Cannot open WASM file: " + path;
        spdlog::error("WasmPluginSandbox: cannot open WASM file (see lastError() for details)");
        return false;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        last_error_ = "Empty or unreadable file: " + path;
        return false;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(bytes.data()), size)) {
        last_error_ = "Failed to read WASM file: " + path;
        return false;
    }

    config_.wasm_path = path;
    return loadFromBytes(bytes, path);
}

// =============================================================================
// Loading from bytes
// =============================================================================

bool WasmPluginSandbox::loadFromBytes(const std::vector<uint8_t> &bytes, const std::string &module_name) {
    last_error_.clear();
    load_warnings_.clear();

    if (loaded_) {
        unload();
    }

    // --- 1. Validate WASM binary header -----------------------------------
    if (!validateWasmHeader(bytes)) {
        return false;
    }

    wasm_bytes_  = bytes;
    module_info_ = WasmModuleValidator::validate(bytes);

    if (!module_info_.module_name.empty()) {
        spdlog::debug("WasmPluginSandbox: loading '{}' ({} bytes)", module_info_.module_name, bytes.size());
    } else {
        spdlog::debug("WasmPluginSandbox: loading '{}' ({} bytes)", module_name, bytes.size());
    }

    // --- 2. Parse import/export sections and check allowlist --------------
    if (!parseImportsExports(bytes)) {
        return false;
    }
    if (!checkImportAllowlist()) {
        return false;
    }

    // --- 3. Allocate linear memory ----------------------------------------
    if (!allocateLinearMemory()) {
        return false;
    }

    // --- 4. Launch OS-level sandbox ---------------------------------------
    const std::string effective_name = module_info_.module_name.empty() ? module_name : module_info_.module_name;
    if (!launchOsSandbox(effective_name)) {
        return false;
    }

    // --- 5. Instantiate in the runtime (if one was injected) -------------
    if (runtime_) {
        bool ok = runtime_->instantiate(wasm_bytes_, host_fns_, linear_memory_.get(), linear_memory_size_);
        if (!ok) {
            last_error_ = "WASM runtime instantiation failed (" + runtime_->engineName() + ")";
            spdlog::error("WasmPluginSandbox: {}", last_error_);
            return false;
        }
        spdlog::info("WasmPluginSandbox: '{}' instantiated via {}", effective_name, runtime_->engineName());
    } else {
        load_warnings_.push_back("No WASM runtime injected – running in validation-only mode. "
                                 "callExport() will return an error until a runtime is provided.");
        spdlog::warn("WasmPluginSandbox: no runtime injected for '{}'; "
                     "validation-only mode",
                     effective_name);
    }

    loaded_ = true;
    // Initialise the fuel counter from the configured budget (UINT64_MAX when
    // max_instructions == 0 signals "unlimited").
    fuel_remaining_ = (config_.max_instructions == 0) ? UINT64_MAX : config_.max_instructions;
    spdlog::info("WasmPluginSandbox: '{}' loaded (imports={} exports={})", effective_name, module_info_.imports.size(),
                 module_info_.exports.size());
    return true;
}

// =============================================================================
// Unload
// =============================================================================

void WasmPluginSandbox::unload() {
    if (!loaded_) {
        return;
    }

    if (runtime_) {
        runtime_->destroy();
    }

    if (os_sandbox_ && os_sandbox_->isActive()) {
        os_sandbox_->shutdown();
    }
    os_sandbox_.reset();

    linear_memory_.reset();
    linear_memory_size_ = 0;
    wasm_bytes_.clear();
    module_info_ = {};
    loaded_      = false;

    spdlog::debug("WasmPluginSandbox: unloaded");
}

// =============================================================================
// callExport
// =============================================================================

WasmCallResult WasmPluginSandbox::callExport(const std::string &export_name, const std::vector<uint8_t> &args) {
    WasmCallResult result = {};

    if (!loaded_) {
        result.error = "No WASM module loaded";
        return result;
    }

    if (!runtime_) {
        result.error = "No WASM runtime available (validation-only mode); "
                       "inject a WasmRuntime before calling callExport()";
        stats_.calls_attempted++;
        stats_.calls_trapped++;
        return result;
    }

    // ── Fuel / instruction budget check ───────────────────────────────────
    // fuel_remaining_ == UINT64_MAX means unlimited (max_instructions == 0).
    if (fuel_remaining_ != UINT64_MAX) {
        if (fuel_remaining_ == 0) {
            result.error = "WASM fuel exhausted: sandbox instruction budget ("
                           + std::to_string(config_.max_instructions) + ") exceeded for export '" + export_name
                           + "'; reload the module to reset";
            spdlog::warn("WasmPluginSandbox: fuel exhausted for export '{}'", export_name);
            stats_.calls_attempted++;
            stats_.calls_trapped++;
            return result;
        }
        const uint64_t cost = (config_.fuel_check_interval > 0) ? config_.fuel_check_interval : 1u;
        fuel_remaining_     = (fuel_remaining_ >= cost) ? (fuel_remaining_ - cost) : 0u;
    }

    auto t0 = std::chrono::steady_clock::now();
    stats_.calls_attempted++;

    bool ok            = runtime_->call(export_name, args, result.output);
    auto t1            = std::chrono::steady_clock::now();
    result.duration_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());

    stats_.total_call_us += result.duration_us;

    if (ok) {
        result.success = true;
        stats_.calls_succeeded++;
    } else {
        result.error = "WASM trap or call error in export '" + export_name + "'";
        stats_.calls_trapped++;
        spdlog::warn("WasmPluginSandbox: call '{}' trapped in {}µs", export_name, result.duration_us);
    }

    return result;
}

// =============================================================================
// Linear memory accessors
// =============================================================================

const uint8_t *WasmPluginSandbox::linearMemory() const noexcept {
    return linear_memory_.get();
}

uint8_t *WasmPluginSandbox::linearMemory() noexcept {
    return linear_memory_.get();
}

size_t WasmPluginSandbox::linearMemorySize() const noexcept {
    return linear_memory_size_;
}

uint64_t WasmPluginSandbox::remainingFuel() const noexcept {
    return fuel_remaining_;
}

// =============================================================================
// Private helpers
// =============================================================================

bool WasmPluginSandbox::validateWasmHeader(const std::vector<uint8_t> &bytes) {
    if (bytes.size() < 8) {
        last_error_ = "Binary too small to be a valid WASM module (" + std::to_string(bytes.size()) + " bytes)";
        spdlog::error("WasmPluginSandbox: {}", last_error_);
        return false;
    }

    if (std::memcmp(bytes.data(), kWasmMagic, 4) != 0) {
        last_error_ = "Invalid WASM magic bytes – not a WebAssembly binary";
        spdlog::error("WasmPluginSandbox: {}", last_error_);
        return false;
    }

    // We accept the standard version 1 only.
    if (std::memcmp(bytes.data() + 4, kWasmVersion, 4) != 0) {
        uint32_t v  = static_cast<uint32_t>(bytes[4]) | (static_cast<uint32_t>(bytes[5]) << 8)
                      | (static_cast<uint32_t>(bytes[6]) << 16) | (static_cast<uint32_t>(bytes[7]) << 24);
        last_error_ = "Unsupported WASM binary version: " + std::to_string(v) + " (expected 1)";
        spdlog::error("WasmPluginSandbox: {}", last_error_);
        return false;
    }

    return true;
}

bool WasmPluginSandbox::parseImportsExports([[maybe_unused]] const std::vector<uint8_t> &bytes) {
    // The validator already does the heavy lifting; results are in module_info_.
    // This method is a hook for future extended validation.
    return true;
}

bool WasmPluginSandbox::checkImportAllowlist() {
    if (config_.allow_unregistered_imports) {
        return true;
    }
    if (module_info_.imports.empty()) {
        return true;
    }

    // GAP-FIX o_n_squared: build an unordered_set of allowed import keys so
    // that the membership test inside the imports loop is O(1) on average
    // rather than O(n) via std::find on a vector, reducing overall complexity
    // from O(imports * host_fns) to O(imports + host_fns).
    std::unordered_set<std::string> allowed_set = {};

    allowed_set.reserve(host_fns_.size());
    for (const auto &hf : host_fns_) {
        allowed_set.insert(hf.module_name + "." + hf.function_name);
    }

    std::vector<std::string> unknown = {};

    for (const auto &imp : module_info_.imports) {
        if (allowed_set.find(imp) == allowed_set.end()) {
            unknown.push_back(imp);
        }
    }

    if (!unknown.empty()) {
        std::ostringstream oss = {};
        oss << "WASM module requires " << unknown.size() << " unregistered host function(s): ";
        for (size_t i = 0; i < unknown.size(); ++i) {
            if (i) {
                oss << ", ";
            }
            oss << unknown[i];
        }
        last_error_ = oss.str();
        spdlog::error("WasmPluginSandbox: {}", last_error_);
        return false;
    }
    return true;
}

bool WasmPluginSandbox::allocateLinearMemory() {
    if (config_.linear_memory_pages == 0) {
        load_warnings_.push_back("linear_memory_pages=0: WASM module gets no linear memory");
        linear_memory_size_ = 0;
        return true;
    }

    linear_memory_size_ = static_cast<size_t>(config_.linear_memory_pages) * kWasmPageBytes;

    try {
        linear_memory_ = std::make_unique<uint8_t[]>(linear_memory_size_);
    } catch (const std::bad_alloc &) {
        last_error_ = "Failed to allocate WASM linear memory (" + std::to_string(linear_memory_size_ / 1024 / 1024)
                      + " MiB requested)";
        spdlog::error("WasmPluginSandbox: {}", last_error_);
        return false;
    }

    // Zero-initialise: WASM spec requires linear memory to start as all-zeros.
    std::memset(linear_memory_.get(), 0, linear_memory_size_);
    return true;
}

bool WasmPluginSandbox::launchOsSandbox(const std::string &module_name) {
    ModuleSandbox::Config os_cfg;
    os_cfg.max_memory_mb        = config_.max_memory_mb;
    os_cfg.max_cpu_time_seconds = config_.max_cpu_time_seconds;
    os_cfg.allow_network        = false;
    os_cfg.fs_access            = ModuleSandbox::FilesystemAccess::NONE;

    os_sandbox_ = std::make_unique<ModuleSandbox>(os_cfg);
    bool ok     = os_sandbox_->launch(module_name);
    if (!ok) {
        last_error_ = "OS sandbox launch failed: " + os_sandbox_->lastError();
        spdlog::error("WasmPluginSandbox: {}", last_error_);
        return false;
    }

    // GAP-FIX string_concat_loop: capture launchWarnings() into a local to
    // avoid re-calling the accessor on each iteration, and build the prefixed
    // string with append() instead of operator+ to skip one temporary per call.
    const auto& os_warnings = os_sandbox_->launchWarnings();
    load_warnings_.reserve(load_warnings_.size() + os_warnings.size());
    for (const auto &w : os_warnings) {
        load_warnings_.push_back(std::string("[OS sandbox] ").append(w));
        spdlog::debug("WasmPluginSandbox: OS sandbox warning: {}", w);
    }
    return true;
}

} // namespace modules
} // namespace themis
