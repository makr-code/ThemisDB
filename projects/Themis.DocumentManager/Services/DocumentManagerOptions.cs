/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentManagerOptions.cs                          ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • fb654c1f1  2025-12-07  Add best practices improvements and intelligent metadata ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Configuration options for Document Manager
/// </summary>
public class DocumentManagerOptions
{
    /// <summary>
    /// Configuration section name in appsettings.json
    /// </summary>
    public const string SectionName = "DocumentManager";
    
    // ThemisDB Connection
    
    /// <summary>
    /// ThemisDB base URL
    /// </summary>
    public string ThemisDbUrl { get; set; } = "http://localhost:8529";
    
    /// <summary>
    /// ThemisDB database name
    /// </summary>
    public string DatabaseName { get; set; } = "themis";
    
    /// <summary>
    /// Connection timeout in seconds
    /// </summary>
    public int ConnectionTimeoutSeconds { get; set; } = 30;
    
    /// <summary>
    /// Maximum concurrent requests
    /// </summary>
    public int MaxConcurrentRequests { get; set; } = 10;
    
    // Caching
    
    /// <summary>
    /// Enable in-memory caching
    /// </summary>
    public bool EnableCaching { get; set; } = true;
    
    /// <summary>
    /// Cache expiration in minutes
    /// </summary>
    public int CacheExpirationMinutes { get; set; } = 15;
    
    // Logging
    
    /// <summary>
    /// Enable detailed logging
    /// </summary>
    public bool EnableLogging { get; set; } = true;
    
    /// <summary>
    /// Minimum log level (Debug, Information, Warning, Error, Critical)
    /// </summary>
    public string LogLevel { get; set; } = "Information";
    
    /// <summary>
    /// Enable performance metrics
    /// </summary>
    public bool EnableMetrics { get; set; } = true;
    
    // LLM Configuration
    
    /// <summary>
    /// Default LLM provider (OpenAI, AzureOpenAI, Anthropic, Ollama, HuggingFace)
    /// </summary>
    public string LLMProvider { get; set; } = "OpenAI";
    
    /// <summary>
    /// LLM API endpoint
    /// </summary>
    public string LLMApiEndpoint { get; set; } = "https://api.openai.com/v1";
    
    /// <summary>
    /// LLM API key (should be stored securely, e.g., Azure Key Vault)
    /// </summary>
    public string LLMApiKey { get; set; } = string.Empty;
    
    /// <summary>
    /// Default LLM model name
    /// </summary>
    public string LLMModelName { get; set; } = "gpt-4";
    
    /// <summary>
    /// Enable LLM features
    /// </summary>
    public bool EnableLLM { get; set; } = true;
    
    // Office Integration
    
    /// <summary>
    /// Enable Office COM integration
    /// </summary>
    public bool EnableOfficeIntegration { get; set; } = true;
    
    /// <summary>
    /// Automatically enable Track Changes
    /// </summary>
    public bool AutoEnableTrackChanges { get; set; } = true;
    
    /// <summary>
    /// Revision backup directory
    /// </summary>
    public string RevisionBackupPath { get; set; } = "./revisions";
    
    // Geo Configuration
    
    /// <summary>
    /// Enable geo features
    /// </summary>
    public bool EnableGeoFeatures { get; set; } = true;
    
    /// <summary>
    /// Default map center latitude
    /// </summary>
    public double DefaultMapCenterLat { get; set; } = 51.1657; // Germany center
    
    /// <summary>
    /// Default map center longitude
    /// </summary>
    public double DefaultMapCenterLon { get; set; } = 10.4515; // Germany center
    
    /// <summary>
    /// Default map zoom level
    /// </summary>
    public int DefaultMapZoom { get; set; } = 6;
    
    /// <summary>
    /// Nominatim geocoding service URL
    /// </summary>
    public string GeocodingServiceUrl { get; set; } = "https://nominatim.openstreetmap.org";
    
    // Security
    
    /// <summary>
    /// Enable authentication
    /// </summary>
    public bool EnableAuthentication { get; set; } = true;
    
    /// <summary>
    /// Enable authorization
    /// </summary>
    public bool EnableAuthorization { get; set; } = true;
    
    /// <summary>
    /// Session timeout in minutes
    /// </summary>
    public int SessionTimeoutMinutes { get; set; } = 30;
    
    /// <summary>
    /// Enable audit logging
    /// </summary>
    public bool EnableAuditLogging { get; set; } = true;
    
    // Features
    
    /// <summary>
    /// Enable inbox module
    /// </summary>
    public bool EnableInbox { get; set; } = true;
    
    /// <summary>
    /// Enable reminders and deadlines
    /// </summary>
    public bool EnableReminders { get; set; } = true;
    
    /// <summary>
    /// Enable co-signing workflows
    /// </summary>
    public bool EnableCosigning { get; set; } = true;
    
    /// <summary>
    /// Enable notifications
    /// </summary>
    public bool EnableNotifications { get; set; } = true;
    
    /// <summary>
    /// Enable filing plan management
    /// </summary>
    public bool EnableFilingPlan { get; set; } = true;
    
    // Performance
    
    /// <summary>
    /// Enable query result caching
    /// </summary>
    public bool EnableQueryCache { get; set; } = true;
    
    /// <summary>
    /// Maximum query results to cache
    /// </summary>
    public int MaxCachedQueries { get; set; } = 100;
    
    /// <summary>
    /// Enable background job processing
    /// </summary>
    public bool EnableBackgroundJobs { get; set; } = true;
    
    /// <summary>
    /// Background job interval in seconds
    /// </summary>
    public int BackgroundJobIntervalSeconds { get; set; } = 60;
    
    // Validation
    
    /// <summary>
    /// Validates the configuration options
    /// </summary>
    /// <returns>True if configuration is valid</returns>
    /// <exception cref="InvalidOperationException">Thrown when configuration is invalid</exception>
    public bool Validate()
    {
        if (string.IsNullOrWhiteSpace(ThemisDbUrl))
            throw new InvalidOperationException("ThemisDbUrl is required");
            
        if (string.IsNullOrWhiteSpace(DatabaseName))
            throw new InvalidOperationException("DatabaseName is required");
            
        if (ConnectionTimeoutSeconds <= 0)
            throw new InvalidOperationException("ConnectionTimeoutSeconds must be positive");
            
        if (MaxConcurrentRequests <= 0)
            throw new InvalidOperationException("MaxConcurrentRequests must be positive");
            
        if (EnableLLM && string.IsNullOrWhiteSpace(LLMApiKey))
            throw new InvalidOperationException("LLMApiKey is required when LLM features are enabled");
            
        return true;
    }
}
