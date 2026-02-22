/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IsolationLevel.cs                                  ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     36                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace ThemisDB.Client;

/// <summary>
/// Isolation level for database transactions
/// </summary>
public enum IsolationLevel
{
    /// <summary>
    /// Read committed isolation level - default
    /// </summary>
    ReadCommitted,
    
    /// <summary>
    /// Snapshot isolation level - provides repeatable reads
    /// </summary>
    Snapshot
}
