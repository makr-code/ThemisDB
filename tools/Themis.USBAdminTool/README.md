> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Themis USB Admin Tool

Windows desktop application (WPF / .NET 8) for managing the secure ThemisDB admin USB stick lifecycle.

## Features

| Tab | Description |
|-----|-------------|
| **Laufwerke** | Detect all connected removable drives, show volume label, serial, free space, and license status |
| **Provisionieren** | Generate a new `themis_admin.lic`, sign it with an RSA private key, and write it to the USB together with the `.themis_hardening.json` side-channel record |
| **Verifizieren** | Check all three hardening layers: license JSON validity, SHA-256 volume hash (Layer 1), and USB serial binding (Layer 3) |
| **Übertragen** | Copy a license from one USB stick to another, re-signing for the target's volume serial |
| **Reparieren** | Re-provision a damaged or expired USB while preserving the original organization metadata |

## Requirements

- Windows 10 / 11 (x64)
- .NET 8.0 Desktop Runtime or SDK
- Administrator privileges (required for WMI disk serial queries)

## Building

```shell
dotnet build tools/Themis.USBAdminTool /p:EnableWindowsTargeting=true
```

Publish a self-contained executable:

```shell
dotnet publish tools/Themis.USBAdminTool -c Release -r win-x64 --self-contained true /p:EnableWindowsTargeting=true
```

## Configuration (`appsettings.json`)

| Key | Default | Description |
|-----|---------|-------------|
| `UsbAdminTool.DefaultLicenseFileName` | `themis_admin.lic` | File written to the USB root — must match `USBAdminConfig::license_file` |
| `UsbAdminTool.DefaultPrivateKeyPath` | *(empty)* | Path to the RSA private key PEM used for signing. Leave empty to use the development placeholder (not suitable for production) |
| `UsbAdminTool.WriteHardeningRecord` | `true` | Write `.themis_hardening.json` to the USB after provisioning |
| `UsbAdminTool.DefaultAdminScopes` | see file | Space/scope list embedded in the license |

## USB Hardening Layers

The tool mirrors the three layers enforced by `USBVolumeHardening` (C++) on the server side:

1. **SHA-256 volume hash** (`expected_volume_hash`) — the tool computes `SHA256(themis_admin.lic)` after writing and records it in `.themis_hardening.json`. Any FAT-level modification to the file will change this hash and be detected.
2. **Read-only mount** — enforced on the Linux server via `/proc/mounts`; not applicable to the Windows tool.
3. **USB serial binding** (`expected_usb_serial`) — the tool reads the hardware serial number via WMI (`Win32_DiskDrive`) and records it in the hardening record. The server rejects any other physical device.

After provisioning, copy the values printed in the **Provisionieren** result box into the ThemisDB server configuration:

```yaml
usb_admin:
  expected_volume_hash: "<64-char hex>"
  expected_usb_serial:  "<serial>"
```

## RSA Signing

The canonical data string signed and verified by both this tool and the C++ authenticator is:

```
license_key|organization|hardware_id|issued_epoch|expiry_epoch|scope1|scope2|…
```

Supply a PKCS#8 or traditional RSA private key in PEM format via `DefaultPrivateKeyPath` (or the UI browse button). The corresponding public key must be configured in `USBAdminConfig::public_key_path` on the server.

## Related

- C++ hardening implementation: `src/security/usb_volume_hardening.cpp`
- C++ authenticator: `src/security/usb_admin_authenticator.cpp`
- Admin tools shared library: `tools/Themis.AdminTools.Shared/`
- [Admin Tools User Guide](../../docs/admin_tools_user_guide.md)
