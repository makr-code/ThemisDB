/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ConnectionModels.cs                                ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:46:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b3d38c00  2025-11-17  Complete Phase 1: Enhanced UI with schema explorer and co... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AqlQueryBuilder.Models;

/// <summary>
/// Connection type to Themis database
/// </summary>
public enum ConnectionType
{
    HttpRest,       // HTTP/HTTPS REST API
    DirectCSharp,   // Direct C# API (in-process)
    DirectCpp,      // Direct C++ API (native interop)
    Socket,         // TCP Socket connection
    Udp             // UDP connection
}

/// <summary>
/// Configuration for connecting to Themis database
/// </summary>
public class ConnectionConfig
{
    public ConnectionType Type { get; set; } = ConnectionType.HttpRest;
    public string ServerUrl { get; set; } = "http://localhost:8080";
    public string? ApiKey { get; set; }
    public string? JwtToken { get; set; }
    
    // For socket/UDP
    public string? Host { get; set; } = "localhost";
    public int Port { get; set; } = 8080;
    
    // For direct C++ API
    public string? DatabasePath { get; set; }
    public bool UseNativeInterop { get; set; }
    
    // Connection timeout
    public int TimeoutSeconds { get; set; } = 30;
    
    // SSL/TLS settings
    public bool UseSsl { get; set; }
    public bool ValidateCertificate { get; set; } = true;
}

/// <summary>
/// Connection status
/// </summary>
public enum ConnectionStatus
{
    Disconnected,
    Connecting,
    Connected,
    Error
}
