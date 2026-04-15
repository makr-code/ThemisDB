/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AnalysisServiceInterfaces.cs                       ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
