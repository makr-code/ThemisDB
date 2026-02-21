/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AnalysisServiceInterfaces.cs                       ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
