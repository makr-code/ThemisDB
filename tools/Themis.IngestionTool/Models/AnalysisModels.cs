/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AnalysisModels.cs                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:33:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     79                                             ║
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
