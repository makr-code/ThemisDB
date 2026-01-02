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
