/**
 * @file usb_volume_hardening.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>

namespace themis {
namespace security {

/**
 * @brief USB Volume Hardening — defence-in-depth against FAT manipulation.
 *
 * An attacker who has physical access to the USB stick can trivially edit the
 * FAT filesystem (e.g. with a hex editor or `dd`) to replace or modify the
 * license file.  USBVolumeHardening adds three independent layers of protection
 * on top of the existing RSA license signature:
 *
 * 1. **Volume integrity hash** — the server pins the expected SHA-256 hash of
 *    the license file content at provisioning time.  Any FAT-level modification
 *    of the file (even a single byte) will produce a different hash and be
 *    rejected before the license is parsed.
 *
 * 2. **Read-only mount enforcement** — the server can require that the USB
 *    filesystem is mounted read-only (`mount -o ro`).  A read-only mount
 *    prevents any process (including a compromised one running on the host)
 *    from writing to the stick while it is in use.
 *
 * 3. **USB device serial binding** — the server can pin the USB device serial
 *    number reported by the hardware.  This prevents a simple `dd` clone of
 *    the stick from being accepted: the cloned device will have a different
 *    serial number (vendor serial stored in SCSI VPD page 0x80 / USB string
 *    descriptor 0x03).
 *
 * All methods are static and stateless; they can be called without constructing
 * an object.  Error conditions are indicated by returning an empty string or
 * `false` — never by throwing.
 */
class USBVolumeHardening {
public:
    // ── Volume Integrity ──────────────────────────────────────────────────────

    /**
     * @brief Compute the SHA-256 hash of the license file on the USB volume.
     *
     * Reads the raw bytes of `<mount_path>/<license_file>` and returns the
     * lowercase hex-encoded SHA-256 digest.  Any I/O or OpenSSL error returns
     * an empty string.
     *
     * @param mount_path   Path where the USB volume is mounted.
     * @param license_file Filename of the license file relative to mount_path.
     * @return 64-character lowercase hex string, or "" on error.
     */
    static std::string computeVolumeHash(const std::string& mount_path,
                                         const std::string& license_file);

    /**
     * @brief Verify that the license file hash matches a pre-provisioned value.
     *
     * Computes the current hash via `computeVolumeHash()` and performs a
     * constant-time comparison against `expected_hash` to prevent timing
     * side-channels.  Returns false if `expected_hash` is empty (caller should
     * treat an unconfigured hash as "no check performed", not as "pass").
     *
     * @param mount_path     Path where the USB volume is mounted.
     * @param license_file   License filename relative to mount_path.
     * @param expected_hash  64-character lowercase hex SHA-256 to match.
     * @return true iff the current hash matches expected_hash exactly.
     */
    static bool verifyVolumeHash(const std::string& mount_path,
                                 const std::string& license_file,
                                 const std::string& expected_hash);

    // ── Mount Flags ───────────────────────────────────────────────────────────

    /**
     * @brief Check whether a filesystem is mounted read-only.
     *
     * On Linux, parses `/proc/mounts` to find the entry whose mount point
     * equals `mount_path` and checks for the `ro` option.
     * On Windows, calls `GetVolumeInformation()` and checks
     * `FILE_READ_ONLY_VOLUME`.
     * On unsupported platforms, always returns false.
     *
     * @param mount_path  Exact mount-point path to look up.
     * @return true iff the filesystem is currently mounted read-only.
     */
    static bool isMountedReadOnly(const std::string& mount_path);

    // ── Device Serial Binding ─────────────────────────────────────────────────

    /**
     * @brief Retrieve the USB device serial number for a mounted volume.
     *
     * On Linux, resolves the block device for `mount_path` via `/proc/mounts`,
     * strips the partition suffix, and reads the serial from
     * `/sys/class/block/<dev>/device/../../serial`.
     * On Windows, returns the hex-encoded volume serial number from
     * `GetVolumeInformation()`.
     * On unsupported platforms or on error, returns an empty string.
     *
     * @param mount_path  Path where the USB volume is mounted.
     * @return Serial string (trimmed), or "" if not available.
     */
    static std::string getUSBDeviceSerial(const std::string& mount_path);

    /**
     * @brief Verify the USB device serial matches a pre-provisioned value.
     *
     * Retrieves the current serial via `getUSBDeviceSerial()` and compares it
     * against `expected_serial` using a constant-time comparison.  Returns
     * false if `expected_serial` is empty.
     *
     * @param mount_path       Path where the USB volume is mounted.
     * @param expected_serial  Serial string recorded at provisioning time.
     * @return true iff the device serial matches expected_serial.
     */
    static bool verifyUSBSerial(const std::string& mount_path,
                                const std::string& expected_serial);
};

} // namespace security
} // namespace themis
