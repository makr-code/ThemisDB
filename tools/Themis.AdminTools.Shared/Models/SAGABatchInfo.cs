/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SAGABatchInfo.cs                                   ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:46:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     71                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 20c4e9c84  2025-11-02  feat: Complete feature set - Auth, Governance, Compliance... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AdminTools.Shared.Models;

/// <summary>
/// Represents basic information about a SAGA batch.
/// </summary>
public class SAGABatchInfo
{
    /// <summary>
    /// Unique batch identifier (e.g., batch_1730499145582_abc123).
    /// </summary>
    public string BatchId { get; set; } = string.Empty;

    /// <summary>
    /// ISO 8601 timestamp when the batch was created and signed.
    /// </summary>
    public string Timestamp { get; set; } = string.Empty;

    /// <summary>
    /// Number of SAGA steps in this batch.
    /// </summary>
    public int EntryCount { get; set; }

    /// <summary>
    /// Base64-encoded PKI signature (SHA-256).
    /// </summary>
    public string Signature { get; set; } = string.Empty;

    /// <summary>
    /// SHA-256 hash of the batch content (hex encoded).
    /// </summary>
    public string? Hash { get; set; }

    /// <summary>
    /// Formatted timestamp for display.
    /// </summary>
    public string TimestampFormatted
    {
        get
        {
            if (DateTime.TryParse(Timestamp, out var dt))
            {
                return dt.ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss");
            }
            return Timestamp;
        }
    }

    /// <summary>
    /// Short batch ID for display (first 16 characters).
    /// </summary>
    public string BatchIdShort => BatchId.Length > 16 ? BatchId[..16] + "..." : BatchId;
}
