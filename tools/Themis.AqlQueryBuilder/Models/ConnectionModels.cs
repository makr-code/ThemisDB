/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ConnectionModels.cs                                ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
