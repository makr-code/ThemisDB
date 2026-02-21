/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ServiceConstants.cs                                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     90                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service-wide constants and configuration values
/// </summary>
public static class ServiceConstants
{
    // Pagination
    public const int DefaultPageSize = 50;
    public const int MaxPageSize = 1000;
    public const int MinPageSize = 1;
    
    // Timeouts
    public const int DefaultTimeoutSeconds = 30;
    public const int LongRunningTimeoutSeconds = 300;
    
    // Retry Policy
    public const int MaxRetries = 3;
    public const int RetryDelayMilliseconds = 1000;
    
    // Caching
    public const int CacheDurationMinutes = 15;
    public const int ShortCacheDurationMinutes = 5;
    public const int LongCacheDurationMinutes = 60;
    
    // Limits
    public const int MaxBatchSize = 100;
    public const int MaxConcurrentOperations = 10;
    public const int MaxFileSizeMB = 100;
    
    // URN Prefixes
    public const string UrnThemis = "urn:themis";
    public const string UrnAuthority = "urn:themis:authority";
    public const string UrnInbox = "urn:themis:inbox";
    public const string UrnReminder = "urn:themis:reminder";
    public const string UrnCosigning = "urn:themis:cosigning";
    public const string UrnNotification = "urn:themis:notification";
    public const string UrnGeo = "urn:themis:geo";
    public const string UrnLLM = "urn:themis:llm";
    
    // Collection Names
    public const string CollectionInbox = "inbox_items";
    public const string CollectionReminders = "reminders";
    public const string CollectionCosigning = "cosigning";
    public const string CollectionNotifications = "notifications";
    public const string CollectionFilingPlans = "filing_plans";
    public const string CollectionGeoFeatures = "geo_features";
    public const string CollectionMapConfigs = "map_configurations";
    
    // LLM
    public const int DefaultMaxTokens = 4096;
    public const double DefaultTemperature = 0.7;
    public const int MaxEmbeddingBatchSize = 100;
    
    // Security
    public const int MinPasswordLength = 12;
    public const int MaxLoginAttempts = 5;
    public const int LockoutDurationMinutes = 30;
    
    // Validation Patterns
    public const string EmailPattern = @"^[^@\s]+@[^@\s]+\.[^@\s]+$";
    public const string FileNumberPattern = @"^[A-Z0-9\s\-/]+$";
    public const string AqlFieldNamePattern = @"^[a-zA-Z0-9_]+$";
}
