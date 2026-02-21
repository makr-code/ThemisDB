/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AnalysisServiceInterfaces.cs                       ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:46:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bde12d3c6  2026-01-02  🔥 HOTFIX: Critical RocksDB Segmentation Fault Fix (v1.3.4) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    public interface ILlamaService
    {
        Task<bool> IsAvailableAsync();
        Task<string> GenerateSummaryAsync(string content);
        Task<List<string>> ExtractKeywordsAsync(string content);
        Task<List<string>> ExtractEntitiesAsync(string content);
        Task<double> CalculateRelevanceScoreAsync(string content);
    }

    public interface INlpAnalysisService
    {
        Task<List<string>> ExtractTopicsAsync(string content);
        Task<string> DetectLanguageAsync(string content);
        Task<double> CalculateQualityScoreAsync(string content);
        Dictionary<string, string> ExtractMetadata(string filePath);
    }

    public interface IGraphAnalysisService
    {
        Task<int> EstimateGraphNodesAsync(string content);
        Task<int> EstimateRelationshipsAsync(string content);
        Task<double> CalculateImpactScoreAsync(string content);
    }

    public interface IIngestionPipelineService
    {
        Task<IngestionPipelineResult> ExecutePipelineAsync(
            string sourceFolder,
            bool isDryRun,
            IProgress<PipelineStage>? progress = null,
            IProgress<FileAnalysisResult>? fileProgress = null);
        void CancelPipeline();
    }
}
