/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ConnectionModels.cs                                ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
