/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SAGAVerificationResult.cs                          ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     90                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text.Json.Serialization;

namespace Themis.AdminTools.Shared.Models;

/// <summary>
/// Result of SAGA batch signature verification.
/// </summary>
public class SAGAVerificationResult
{
    /// <summary>
    /// Batch ID that was verified.
    /// </summary>
    [JsonPropertyName("batch_id")]
    public string BatchId { get; set; } = string.Empty;

    /// <summary>
    /// Overall verification result (true if both signature and hash are valid).
    /// </summary>
    [JsonPropertyName("verified")]
    public bool Verified { get; set; }

    /// <summary>
    /// Whether the PKI signature is valid.
    /// </summary>
    [JsonPropertyName("signature_valid")]
    public bool SignatureValid { get; set; }

    /// <summary>
    /// Whether the hash matches the batch content.
    /// </summary>
    [JsonPropertyName("hash_match")]
    public bool HashMatch { get; set; }

    /// <summary>
    /// Human-readable message about the verification result.
    /// </summary>
    [JsonPropertyName("message")]
    public string Message { get; set; } = string.Empty;

    /// <summary>
    /// Display-friendly verification status.
    /// </summary>
    public string StatusDisplay => Verified ? "✓ Verified" : "✗ Failed";

    /// <summary>
    /// Detailed status message.
    /// </summary>
    public string DetailedStatus
    {
        get
        {
            if (Verified)
                return "Batch signature and hash verified successfully.";
            
            var issues = new List<string>();
            if (!SignatureValid)
                issues.Add("Invalid signature");
            if (!HashMatch)
                issues.Add("Hash mismatch");
            
            return $"Verification failed: {string.Join(", ", issues)}";
        }
    }
}
