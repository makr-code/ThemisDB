/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            HashService.cs                                     ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 20:54:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 684f1ae3bf  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace Themis.USBAdminTool.Services;

/// <summary>
/// Provides SHA-256 hashing utilities that mirror the C++ EVP_Digest
/// implementation in <c>src/security/usb_volume_hardening.cpp</c>.
/// Both implementations produce identical 64-character lowercase hex digests.
/// </summary>
public sealed class HashService
{
    /// <summary>
    /// Compute the SHA-256 hash of the file at <paramref name="filePath"/>.
    /// </summary>
    /// <returns>64-character lowercase hex string, or empty string on I/O error.</returns>
    public async Task<string> ComputeFileHashAsync(string filePath)
    {
        if (!File.Exists(filePath))
            return string.Empty;

        try
        {
            await using var stream = new FileStream(
                filePath, FileMode.Open, FileAccess.Read, FileShare.Read,
                bufferSize: 65536, useAsync: true);

            var digest = await SHA256.HashDataAsync(stream);
            return ToHexString(digest);
        }
        catch (IOException)
        {
            return string.Empty;
        }
    }

    /// <summary>
    /// Compute the SHA-256 hash of a UTF-8 string (used for testing and canonical
    /// data construction).
    /// </summary>
    public static string ComputeStringHash(string text)
    {
        var bytes = Encoding.UTF8.GetBytes(text);
        var digest = SHA256.HashData(bytes);
        return ToHexString(digest);
    }

    /// <summary>
    /// Convert raw bytes to a lowercase hex string — same format used by the
    /// C++ implementation and by <c>sha256sum</c>.
    /// </summary>
    public static string ToHexString(byte[] bytes)
    {
        var sb = new StringBuilder(bytes.Length * 2);
        foreach (var b in bytes)
            sb.Append(b.ToString("x2"));
        return sb.ToString();
    }
}
