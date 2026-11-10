/**
 * @file lora_safetensors_package.cpp
 * @brief Native safetensors header validation for distributable LoRA packages.
 */

#include "llm/lora_framework/lora_safetensors_package.h"

#include <nlohmann/json.hpp>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <memory>
#include <system_error>

namespace themis {
namespace llm {
namespace lora {

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

constexpr const char* kSafetensorsFileName = "adapter_model.safetensors";
constexpr const char* kAdapterConfigFileName = "adapter_config.json";

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

/// Returns the element size in bytes for whitelisted dtypes, 0 otherwise.
std::uint64_t dtypeSize(const std::string& dtype) {
    if (dtype == "F16" || dtype == "BF16") {
        return 2;
    }
    if (dtype == "F32") {
        return 4;
    }
    return 0;
}

SafetensorsInspection fail(SafetensorsInspection r, std::string msg) {
    r.valid = false;
    r.error = std::move(msg);
    r.tensors.clear();
    r.metadata.clear();
    return r;
}

struct EvpCtxDeleter {
    void operator()(EVP_MD_CTX* ctx) const noexcept { EVP_MD_CTX_free(ctx); }
};

/// Streams [offset, EOF) of @p path through SHA-256.
std::string sha256Range(const fs::path& path, std::uint64_t offset) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!in) {
        return {};
    }
    std::unique_ptr<EVP_MD_CTX, EvpCtxDeleter> ctx(EVP_MD_CTX_new());
    if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        return {};
    }
    std::array<char, 64 * 1024> buf{};
    while (in.read(buf.data(), static_cast<std::streamsize>(buf.size())) || in.gcount() > 0) {
        if (EVP_DigestUpdate(ctx.get(), buf.data(), static_cast<size_t>(in.gcount())) != 1) {
            return {};
        }
    }
    if (in.bad()) {
        return {};
    }
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int len = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &len) != 1) {
        return {};
    }
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(static_cast<size_t>(len) * 2);
    for (unsigned int i = 0; i < len; ++i) {
        out.push_back(kHex[digest[i] >> 4]);
        out.push_back(kHex[digest[i] & 0x0F]);
    }
    return out;
}

}  // namespace

bool SafetensorsPackageValidator::isForbiddenCheckpointExtension(const fs::path& path) {
    const std::string ext = toLower(path.extension().string());
    return ext == ".bin" || ext == ".pt" || ext == ".pth" || ext == ".pkl" ||
           ext == ".pickle" || ext == ".ckpt";
}

AdapterPackagePaths SafetensorsPackageValidator::resolvePackage(const fs::path& source) {
    AdapterPackagePaths out;
    std::error_code ec;
    if (source.empty()) {
        out.error = "empty package path";
        return out;
    }
    if (isForbiddenCheckpointExtension(source)) {
        out.error = "pickle-based checkpoint formats (.bin/.pt/.pth/.pkl/.pickle/.ckpt) are rejected; "
                    "provide a safetensors package";
        return out;
    }
    if (fs::is_directory(source, ec)) {
        out.package_dir = source;
        out.safetensors = source / kSafetensorsFileName;
    } else if (fs::is_regular_file(source, ec) && toLower(source.extension().string()) == ".safetensors") {
        out.package_dir = source.parent_path();
        out.safetensors = source;
    } else {
        out.error = "package path must be a directory or a .safetensors file: " + source.string();
        return out;
    }
    // Reject packages that ship pickle checkpoints alongside (a converter could pick them up).
    for (fs::directory_iterator it(out.package_dir, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && isForbiddenCheckpointExtension(it->path())) {
            out.error = "package contains a pickle-based checkpoint (" + it->path().filename().string() +
                        "); only safetensors packages are accepted";
            return out;
        }
    }
    if (ec) {
        out.error = "cannot list package directory: " + ec.message();
        return out;
    }
    if (!fs::is_regular_file(out.safetensors, ec)) {
        out.error = std::string("missing ") + kSafetensorsFileName + " in package";
        return out;
    }
    out.adapter_config = out.package_dir / kAdapterConfigFileName;
    if (!fs::is_regular_file(out.adapter_config, ec)) {
        out.error = std::string("missing ") + kAdapterConfigFileName + " in package";
        return out;
    }
    const auto cfg_size = fs::file_size(out.adapter_config, ec);
    if (ec || cfg_size == 0 || cfg_size > kMaxAdapterConfigBytes) {
        out.error = std::string(kAdapterConfigFileName) + " is empty, unreadable or too large";
        return out;
    }
    std::ifstream cfg_in(out.adapter_config, std::ios::binary);
    std::string cfg_text(static_cast<size_t>(cfg_size), '\0');
    if (!cfg_in.read(cfg_text.data(), static_cast<std::streamsize>(cfg_size))) {
        out.error = std::string("cannot read ") + kAdapterConfigFileName;
        return out;
    }
    const json cfg = json::parse(cfg_text, nullptr, /*allow_exceptions=*/false);
    if (cfg.is_discarded() || !cfg.is_object()) {
        out.error = std::string(kAdapterConfigFileName) + " is not a JSON object";
        return out;
    }
    // GGUF serving supports plain LoRA only; AdaLoRA must be exported as plain LoRA.
    const auto peft_type = cfg.find("peft_type");
    if (peft_type == cfg.end() || !peft_type->is_string() || toLower(peft_type->get<std::string>()) != "lora") {
        out.error = std::string(kAdapterConfigFileName) +
                    " must declare peft_type \"LORA\" (AdaLoRA must be exported as plain LoRA)";
        return out;
    }
    out.valid = true;
    return out;
}

SafetensorsInspection SafetensorsPackageValidator::inspectFile(const fs::path& path) {
    SafetensorsInspection r;
    std::error_code ec;
    const std::uint64_t file_size = fs::file_size(path, ec);
    if (ec) {
        return fail(std::move(r), "cannot stat safetensors file: " + ec.message());
    }
    if (file_size < 8) {
        return fail(std::move(r), "file too small for a safetensors header");
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return fail(std::move(r), "cannot open safetensors file");
    }
    std::array<unsigned char, 8> prefix{};
    if (!in.read(reinterpret_cast<char*>(prefix.data()), 8)) {
        return fail(std::move(r), "cannot read safetensors header length");
    }
    std::uint64_t n = 0;
    for (int i = 7; i >= 0; --i) {
        n = (n << 8) | prefix[static_cast<size_t>(i)];
    }
    if (n == 0 || n > kMaxHeaderBytes) {
        return fail(std::move(r), "safetensors header length out of bounds");
    }
    if (n > file_size - 8) {
        return fail(std::move(r), "safetensors header length exceeds file size");
    }
    r.header_length = n;
    r.data_offset = 8 + n;
    r.data_length = file_size - r.data_offset;

    std::string header(static_cast<size_t>(n), '\0');
    if (!in.read(header.data(), static_cast<std::streamsize>(n))) {
        return fail(std::move(r), "cannot read safetensors header");
    }
    const json h = json::parse(header, nullptr, /*allow_exceptions=*/false);
    if (h.is_discarded() || !h.is_object()) {
        return fail(std::move(r), "safetensors header is not a JSON object");
    }

    for (auto it = h.begin(); it != h.end(); ++it) {
        if (it.key() == "__metadata__") {
            if (!it->is_object()) {
                return fail(std::move(r), "__metadata__ must be an object");
            }
            for (auto m = it->begin(); m != it->end(); ++m) {
                if (!m->is_string()) {
                    return fail(std::move(r), "__metadata__ values must be strings (key '" + m.key() + "')");
                }
                r.metadata.emplace(m.key(), m->get<std::string>());
            }
            continue;
        }
        const json& t = *it;
        if (!t.is_object()) {
            return fail(std::move(r), "tensor entry '" + it.key() + "' is not an object");
        }
        const auto dt = t.find("dtype");
        const auto sh = t.find("shape");
        const auto off = t.find("data_offsets");
        if (dt == t.end() || !dt->is_string() || sh == t.end() || !sh->is_array() ||
            off == t.end() || !off->is_array() || off->size() != 2) {
            return fail(std::move(r), "tensor entry '" + it.key() + "' lacks dtype/shape/data_offsets");
        }
        SafetensorsTensorInfo info;
        info.name = it.key();
        info.dtype = dt->get<std::string>();
        const std::uint64_t elem = dtypeSize(info.dtype);
        if (elem == 0) {
            return fail(std::move(r), "tensor '" + info.name + "' has disallowed dtype '" + info.dtype +
                                          "' (allowed: F16, BF16, F32)");
        }
        std::uint64_t count = 1;
        for (const auto& d : *sh) {
            if (!d.is_number_unsigned()) {
                return fail(std::move(r), "tensor '" + info.name + "' has a non-integer or negative dimension");
            }
            const std::uint64_t dim = d.get<std::uint64_t>();
            if (dim != 0 && count > std::numeric_limits<std::uint64_t>::max() / dim) {
                return fail(std::move(r), "tensor '" + info.name + "' shape overflows");
            }
            count *= dim;
            info.shape.push_back(dim);
        }
        if (count > std::numeric_limits<std::uint64_t>::max() / elem) {
            return fail(std::move(r), "tensor '" + info.name + "' byte size overflows");
        }
        if (!(*off)[0].is_number_unsigned() || !(*off)[1].is_number_unsigned()) {
            return fail(std::move(r), "tensor '" + info.name + "' has invalid data_offsets");
        }
        info.begin = (*off)[0].get<std::uint64_t>();
        info.end = (*off)[1].get<std::uint64_t>();
        if (info.begin > info.end || info.end > r.data_length) {
            return fail(std::move(r), "tensor '" + info.name + "' data_offsets out of bounds");
        }
        if (info.end - info.begin != count * elem) {
            return fail(std::move(r), "tensor '" + info.name + "' byte size does not match dtype/shape");
        }
        r.tensors.push_back(std::move(info));
    }
    if (r.tensors.empty()) {
        return fail(std::move(r), "safetensors file contains no tensors");
    }
    std::sort(r.tensors.begin(), r.tensors.end(),
              [](const SafetensorsTensorInfo& a, const SafetensorsTensorInfo& b) {
                  return a.begin != b.begin ? a.begin < b.begin : a.end < b.end;
              });
    std::uint64_t cursor = 0;
    for (const auto& t : r.tensors) {
        if (t.begin != cursor) {
            return fail(std::move(r), "tensor data is not contiguous or overlaps at '" + t.name + "'");
        }
        cursor = t.end;
    }
    if (cursor != r.data_length) {
        return fail(std::move(r), "tensor data does not cover the data region (trailing bytes)");
    }
    r.valid = true;
    return r;
}

std::string SafetensorsPackageValidator::sha256OfTensorData(const fs::path& path,
                                                            const SafetensorsInspection& inspection) {
    if (!inspection.valid) {
        return {};
    }
    return sha256Range(path, inspection.data_offset);
}

std::string SafetensorsPackageValidator::sha256OfFile(const fs::path& path) {
    return sha256Range(path, 0);
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
