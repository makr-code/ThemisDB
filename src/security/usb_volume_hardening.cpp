/**
 * @file usb_volume_hardening.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=2, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/usb_volume_hardening.h"
#include "utils/logger.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cctype>
#include <algorithm>
#include <memory>

#include <openssl/evp.h>
#include <openssl/crypto.h>

#if defined(__linux__)
#   include <sys/statvfs.h>
#elif defined(_WIN32)
#   include <Windows.h>
#endif

namespace themis {
namespace security {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct USBVolume_EVP_MD_CTX_Deleter {
    void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
};

using USBVolume_EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, USBVolume_EVP_MD_CTX_Deleter>;

} // anonymous namespace

// ── Internal helpers ──────────────────────────────────────────────────────────

namespace {

/// Trim leading/trailing ASCII whitespace (space, tab, CR, LF) from a string.
[[maybe_unused]] static std::string trimWhitespace(std::string s) {
    auto notSpace = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

/// Build the platform-correct path separator for a file on the volume.
static std::string joinPath(const std::string& dir, const std::string& file) {
#if defined(_WIN32)
    return dir + "\\" + file;
#else
    return dir + "/" + file;
#endif
}

} // anonymous namespace

// ── USBVolumeHardening::computeVolumeHash ─────────────────────────────────────

std::string USBVolumeHardening::computeVolumeHash(const std::string& mount_path,
                                                   const std::string& license_file) {
    const std::string file_path = joinPath(mount_path, license_file);

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        THEMIS_WARN("USBVolumeHardening: cannot open '{}' for hashing", file_path);
        return "";
    }

    // Use the EVP digest API (OpenSSL 3.x preferred; backward-compatible with 1.x).
    USBVolume_EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new());
    if (!ctx) {
        THEMIS_ERROR("USBVolumeHardening: EVP_MD_CTX_new failed");
        return "";
    }

    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        THEMIS_ERROR("USBVolumeHardening: EVP_DigestInit_ex failed");
        return "";
    }

    std::vector<char> buf(65536);
    while (file.read(buf.data(), static_cast<std::streamsize>(buf.size())) || file.gcount() > 0) {
        auto n = static_cast<size_t>(file.gcount());
        if (EVP_DigestUpdate(ctx.get(), buf.data(), n) != 1) {
            THEMIS_ERROR("USBVolumeHardening: EVP_DigestUpdate failed");
            return "";
        }
    }

    if (file.bad()) {
        THEMIS_ERROR("USBVolumeHardening: I/O error reading '{}'", file_path);
        return "";
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest, &digest_len) != 1) {
        THEMIS_ERROR("USBVolumeHardening: EVP_DigestFinal_ex failed");
        return "";
    }

    std::ostringstream oss = {};
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

// ── USBVolumeHardening::verifyVolumeHash ──────────────────────────────────────

bool USBVolumeHardening::verifyVolumeHash(const std::string& mount_path,
                                          const std::string& license_file,
                                          const std::string& expected_hash) {
    if (expected_hash.empty()) {
        THEMIS_WARN("USBVolumeHardening: verifyVolumeHash called with empty expected_hash — check skipped");
        return false;
    }

    const std::string actual = computeVolumeHash(mount_path, license_file);
    if (actual.empty()) {
        THEMIS_WARN("USBVolumeHardening: volume hash computation failed");
        return false;
    }

    if (static_cast<int>(actual.size()) != expected_hash.size()) {
        THEMIS_WARN("USBVolumeHardening: volume hash length mismatch (actual={} expected={})",
                    actual.size(),static_cast<int>(expected_hash.size()));
        return false;
    }

    // Constant-time comparison to prevent timing attacks.
    bool match = (CRYPTO_memcmp(actual.data(), expected_hash.data(),static_cast<int>(actual.size())) == 0);
    if (!match) {
        THEMIS_WARN("USBVolumeHardening: volume hash mismatch — possible FAT manipulation");
    }
    return match;
}

// ── USBVolumeHardening::isMountedReadOnly ─────────────────────────────────────

bool USBVolumeHardening::isMountedReadOnly(const std::string& mount_path) {
#if defined(__linux__)
    // Parse /proc/mounts:  <device> <mountpoint> <fstype> <options> <dump> <pass>
    std::ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) {
        THEMIS_WARN("USBVolumeHardening: cannot open /proc/mounts");
        return false;
    }

    std::string line = {};
    while (std::getline(mounts, line)) {
        std::istringstream iss(line);
        std::string dev, mp, fstype, options;
        if (!(iss >> dev >> mp >> fstype >> options)) {
          continue;
        }
        if (mp != mount_path) {
          continue;
        }

        // Options are comma-separated; "ro" means read-only, "rw" means read-write.
        std::istringstream opts(options);
        std::string opt = {};
        while (std::getline(opts, opt, ',')) {
            if (opt == "ro") {
              return true;
            }
            if (opt == "rw") {
              return false;
            }
        }
        // If neither "ro" nor "rw" found, treat as read-write (safe default).
        return false;
    }

    THEMIS_WARN("USBVolumeHardening: mount point '{}' not found in /proc/mounts", mount_path);
    return false;

#elif defined(_WIN32)
    char volume_path[MAX_PATH];
    if (!GetVolumePathNameA(mount_path.c_str(), volume_path, MAX_PATH)) {
        THEMIS_WARN("USBVolumeHardening: GetVolumePathName failed for '{}'", mount_path);
        return false;
    }

    DWORD fs_flags = 0;
    if (!GetVolumeInformationA(volume_path, nullptr, 0, nullptr, nullptr, &fs_flags, nullptr, 0)) {
        THEMIS_WARN("USBVolumeHardening: GetVolumeInformation failed for '{}'", mount_path);
        return false;
    }

    return (fs_flags & FILE_READ_ONLY_VOLUME) != 0;

#else
    THEMIS_WARN("USBVolumeHardening: isMountedReadOnly not implemented on this platform");
    return false;
#endif
}

// ── USBVolumeHardening::getUSBDeviceSerial ───────────────────────────────────

std::string USBVolumeHardening::getUSBDeviceSerial(const std::string& mount_path) {
#if defined(__linux__)
    // Step 1: Find the block device for this mount point via /proc/mounts.
    std::ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) {
        THEMIS_WARN("USBVolumeHardening: cannot open /proc/mounts");
        return "";
    }

    std::string device = {};
    std::string line = {};
    while (std::getline(mounts, line)) {
        std::istringstream iss(line);
        std::string dev, mp;
        if (!(iss >> dev >> mp)) {
          continue;
        }
        if (mp == mount_path) {
            device = dev;
            break;
        }
    }

    if (device.empty()) {
        THEMIS_WARN("USBVolumeHardening: mount point '{}' not found in /proc/mounts", mount_path);
        return "";
    }

    // Step 2: Get the base device name (strip /dev/ prefix and partition suffix).
    // e.g. /dev/sdb1 → sdb, /dev/mmcblk0p1 → mmcblk0
    std::string dev_name = device;
    if (static_cast<int>(dev_name.size()) > 5 && dev_name.substr(0, 5) == "/dev/") {
        dev_name = dev_name.substr(5);
    }

    // For sdX devices: strip trailing digits (sdb1 → sdb).
    // For mmcblk devices: strip 'p' + digits (mmcblk0p1 → mmcblk0).
    if (dev_name.find("mmcblk") == 0 || dev_name.find("nvme") == 0) {
        // Strip 'p' followed by digits at end
        auto p = dev_name.rfind('p');
        if (p != std::string::npos) {
            bool all_digits = true;
            for (size_t i = p + 1; i <static_cast<int>(dev_name.size()); ++i) {
                if (!std::isdigit(static_cast<unsigned char>(dev_name[i]))) {
                    all_digits = false;
                    break;
                }
            }
            if (all_digits && p + 1 <static_cast<int>(dev_name.size())) {
                dev_name = dev_name.substr(0, p);
            }
        }
    } else {
        // Strip trailing digits (e.g. sdb1 → sdb)
        while (!dev_name.empty() && std::isdigit(static_cast<unsigned char>(dev_name.back()))) {
            dev_name.pop_back();
        }
    }

    // Step 3: Try to read serial from sysfs.
    // For USB mass storage attached as scsi_disk the sysfs path is:
    //   /sys/class/block/<dev>/device/../../serial   (USB string descriptor)
    const std::vector<std::string> sysfs_candidates = {
        "/sys/class/block/" + dev_name + "/device/../../serial",
        "/sys/block/"       + dev_name + "/device/../../serial",
        "/sys/class/block/" + dev_name + "/device/../serial",
        "/sys/block/"       + dev_name + "/device/../serial",
    };

    for (const auto& path : sysfs_candidates) {
        std::ifstream sf(path);
        if (sf.is_open()) {
            std::string serial = {};
            std::getline(sf, serial);
            serial = trimWhitespace(serial);
            if (!serial.empty()) {
                return serial;
            }
        }
    }

    THEMIS_WARN("USBVolumeHardening: USB serial not found in sysfs for device '{}'", dev_name);
    return "";

#elif defined(_WIN32)
    // On Windows, return the hex-encoded volume serial number (4-byte DWORD).
    // This is not the hardware USB serial but is stable for a given USB stick.
    DWORD attrs = GetFileAttributesA(mount_path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        THEMIS_WARN("USBVolumeHardening: mount path does not exist or is not a directory: '{}'", mount_path);
        return "";
    }

    char volume_path[MAX_PATH];
    if (!GetVolumePathNameA(mount_path.c_str(), volume_path, MAX_PATH)) {
        THEMIS_WARN("USBVolumeHardening: GetVolumePathName failed for '{}'", mount_path);
        return "";
    }

    DWORD volume_serial = 0;
    if (!GetVolumeInformationA(volume_path, nullptr, 0, &volume_serial, nullptr, nullptr, nullptr, 0)) {
        THEMIS_WARN("USBVolumeHardening: GetVolumeInformation failed for '{}'", mount_path);
        return "";
    }

    std::ostringstream oss = {};
    oss << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << volume_serial;
    return oss.str();

#else
    THEMIS_WARN("USBVolumeHardening: getUSBDeviceSerial not implemented on this platform");
    return "";
#endif
}

// ── USBVolumeHardening::verifyUSBSerial ───────────────────────────────────────

bool USBVolumeHardening::verifyUSBSerial(const std::string& mount_path,
                                         const std::string& expected_serial) {
    if (expected_serial.empty()) {
        THEMIS_WARN("USBVolumeHardening: verifyUSBSerial called with empty expected_serial — check skipped");
        return false;
    }

    const std::string actual = getUSBDeviceSerial(mount_path);
    if (actual.empty()) {
        THEMIS_WARN("USBVolumeHardening: USB serial read failed — anti-cloning check cannot be performed");
        return false;
    }

    if (static_cast<int>(actual.size()) != expected_serial.size()) {
        THEMIS_WARN("USBVolumeHardening: USB serial length mismatch — possible cloned device");
        return false;
    }

    // Constant-time comparison.
    bool match = (CRYPTO_memcmp(actual.data(), expected_serial.data(),static_cast<int>(actual.size())) == 0);
    if (!match) {
        THEMIS_WARN("USBVolumeHardening: USB serial mismatch — possible cloned USB device");
    }
    return match;
}

} // namespace security
} // namespace themis

