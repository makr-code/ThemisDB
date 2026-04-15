/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AppSettings.cs                                     ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     89                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.IngestionTool.Models
{
    public class AppSettings
    {
        // ThemisDB Connection
        public string ThemisHost { get; set; } = "localhost";
        public int ThemisPort { get; set; } = 8765;
        public string ThemisApiUrl { get; set; } = "http://localhost:8765/api";  // REST API endpoint for graph/vector queries

        // gRPC Configuration
        public bool UseGrpc { get; set; } = true;  // Neue Einstellung: gRPC vs HTTP
        public int ThemisGrpcPort { get; set; } = 50051;  // Standard gRPC Port

        // Database
        public string DatabasePath { get; set; } = "ingestion_tracker.db";
        public int MaxFileSize { get; set; } = 100;

        // Metadata Features
        public bool EnableVectorMetadata { get; set; } = true;
        public bool EnableGraphMetadata { get; set; } = true;
        public bool EnableRelationalMetadata { get; set; } = true;

        // Last Used Values
        public string LastSourceFolder { get; set; } = string.Empty;
        public string LastOutputFile { get; set; } = "ingestion_output.json";

        // LLM Configuration
        public string LlamaEndpoint { get; set; } = "http://localhost:11434/api/generate";
        public string LlamaModel { get; set; } = "llama2";
        public int LlamaMaxTokens { get; set; } = 200;
        public double LlamaTemperature { get; set; } = 0.7;

        // Pipeline Configuration
        public int MaxParallelFiles { get; set; } = 4;
        public bool EnableBatching { get; set; } = true;
        public int BatchSize { get; set; } = 10;
        public bool EnableCaching { get; set; } = true;

        // ThemisDB API Features
        public bool UseTransactions { get; set; } = true;
        public bool UseBatchOperations { get; set; } = true;
        public bool StoreVectors { get; set; } = true;
        public bool TrackTimeSeries { get; set; } = true;

        // Embedding Service Configuration
        public string EmbeddingProvider { get; set; } = "ollama"; // "ollama" oder "huggingface"
        public string EmbeddingModel { get; set; } = "nomic-embed-text"; // Ollama: "nomic-embed-text", "all-minilm"
        public string OllamaHost { get; set; } = "localhost";
        public int OllamaPort { get; set; } = 11434;
        public string HuggingFaceApiKey { get; set; } = string.Empty;

        // Caching Configuration
        public bool EnableCacheService { get; set; } = true;
        public int CacheMaxSize { get; set; } = 1000;
        public int CacheTTLMinutes { get; set; } = 60;

        // Resilience Configuration (Polly)
        public int MaxRetries { get; set; } = 3;
        public int CircuitBreakerThreshold { get; set; } = 5;
        public int CircuitBreakerDurationSeconds { get; set; } = 30;

        // LLM Status Monitoring
        public bool EnableLlmStatusMonitoring { get; set; } = true;
        public int LlmStatusCheckIntervalSeconds { get; set; } = 10;
        public bool ShowLlmStatusInStatusBar { get; set; } = true;
    }
}
