/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProductionReleaseConfig.cs                         ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// Production-facing toggles for the DirectX 11 rendering stack.
/// Keeps defaults safe for release while allowing feature overrides.
/// </summary>
public class ProductionReleaseConfig
{
    public bool EnableAdvancedOptimization { get; set; } = true;
    public bool EnableAdvancedEffects { get; set; } = true;
    public bool EnableShadows { get; set; } = true;
    public bool EnableSSAO { get; set; } = true;
    public bool EnableParallax { get; set; } = true;
    public bool EnableNormalMapping { get; set; } = true;
    public bool EnablePerformanceTelemetry { get; set; } = true;
    public bool EnableVerboseLogging { get; set; } = false;

    // Guardrails
    public float TargetFps { get; set; } = 60f;
    public int MaxInstanceBatchSize { get; set; } = 2048;
    public int MaxShadowMapSize { get; set; } = 2048;
    public int MaxSSAO_Samples { get; set; } = 16;

    /// <summary>
    /// Enforce safe release defaults when user input is out of range.
    /// </summary>
    public void Clamp()
    {
        TargetFps = Math.Clamp(TargetFps, 30f, 240f);
        MaxInstanceBatchSize = Math.Clamp(MaxInstanceBatchSize, 256, 8192);
        MaxShadowMapSize = ClampPowerOfTwo(MaxShadowMapSize, 256, 4096);
        MaxSSAO_Samples = Math.Clamp(MaxSSAO_Samples, 4, 32);
    }

    private static int ClampPowerOfTwo(int value, int min, int max)
    {
        int v = value;
        // Snap to nearest power-of-two between min and max
        int p = 1;
        while (p < v) p <<= 1;
        // pick closer of p or p>>1
        int lower = p >> 1;
        int upper = p;
        int snapped = (Math.Abs(upper - v) < Math.Abs(v - lower)) ? upper : lower;
        snapped = Math.Clamp(snapped, min, max);
        return snapped;
    }
}
