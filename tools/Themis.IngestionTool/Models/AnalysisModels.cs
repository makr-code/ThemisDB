/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AnalysisModels.cs                                  ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:05:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bde12d3c6  2026-01-02  🔥 HOTFIX: Critical RocksDB Segmentation Fault Fix (v1.3.4) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.IngestionTool.Models
{
    public class FileAnalysisResult
    {
        public string FilePath { get; set; } = string.Empty;
        public string FileName { get; set; } = string.Empty;
        public long FileSize { get; set; }
        public string FileType { get; set; } = string.Empty;
        public string ContentHash { get; set; } = string.Empty;
        
        // Metriken
        public double RelevanceScore { get; set; }
        public double ImpactScore { get; set; }
        public double QualityScore { get; set; }
        public int GraphNodeCount { get; set; }
        public int RelationshipCount { get; set; }
        
        // NLP-Analyse
        public List<string> ExtractedEntities { get; set; } = new();
        public List<string> Keywords { get; set; } = new();
        public List<string> Topics { get; set; } = new();
        public string Summary { get; set; } = string.Empty;
        public string Language { get; set; } = string.Empty;
        
        // Metadaten
        public Dictionary<string, string> Metadata { get; set; } = new();
        public DateTime AnalysisTimestamp { get; set; }
        public TimeSpan ProcessingTime { get; set; }
        
        // Status
        public bool IsProcessed { get; set; }
        public bool IsDuplicate { get; set; }
        public string? ErrorMessage { get; set; }
    }

    public class IngestionPipelineResult
    {
        public int TotalFiles { get; set; }
        public int ProcessedFiles { get; set; }
        public int SkippedFiles { get; set; }
        public int ErrorFiles { get; set; }
        public int DuplicateFiles { get; set; }
        public List<FileAnalysisResult> Results { get; set; } = new();
        public TimeSpan TotalTime { get; set; }
        public bool IsDryRun { get; set; }
    }

    public class PipelineStage
    {
        public string Name { get; set; } = string.Empty;
        public string Description { get; set; } = string.Empty;
        public int ProcessedCount { get; set; }
        public int TotalCount { get; set; }
        public bool IsComplete { get; set; }
        public TimeSpan ElapsedTime { get; set; }
    }
}
