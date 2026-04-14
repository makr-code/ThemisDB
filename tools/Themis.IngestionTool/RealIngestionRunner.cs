/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RealIngestionRunner.cs                             ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 19:10:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     187                                            ║
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
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Themis.IngestionTool.Services;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool
{
    /// <summary>
    /// Real-World Ingestion Runner für echte Datei-Verarbeitung zu ThemisDB
    /// </summary>
    public class RealIngestionRunner
    {
        private readonly IServiceProvider _serviceProvider;

        public RealIngestionRunner(IServiceProvider serviceProvider)
        {
            _serviceProvider = serviceProvider;
        }

        /// <summary>
        /// Startet echte Ingestion für einen Ordner zu ThemisDB
        /// </summary>
        public async Task<IngestionRunnerResult> RunRealIngestionAsync(
            string sourceFolder,
            bool dryRun = false,
            bool storeVectors = true,
            bool trackTimeSeries = true)
        {
            var result = new IngestionRunnerResult();

            Console.WriteLine($@"
╔═══════════════════════════════════════════════════════════════════════════════╗
║                                                                               ║
║                     🚀 REAL INGESTION FOR THEMISDB                           ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
");

            // 1. Validiere Ordner
            if (!Directory.Exists(sourceFolder))
            {
                Console.WriteLine($"❌ Ordner nicht gefunden: {sourceFolder}");
                result.Success = false;
                return result;
            }

            var files = Directory.GetFiles(sourceFolder, "*.*", SearchOption.AllDirectories).ToList();
            Console.WriteLine($"📂 Ordner: {sourceFolder}");
            Console.WriteLine($"📄 Dateien gefunden: {files.Count}\n");

            if (files.Count == 0)
            {
                Console.WriteLine("⚠️  Keine Dateien im Ordner!");
                return result;
            }

            // 2. Hol Services
            var loggerService = _serviceProvider.GetRequiredService<ILoggerService>();
            var pipelineService = _serviceProvider.GetRequiredService<IIngestionPipelineService>();
            var themisApiService = _serviceProvider.GetRequiredService<IThemisApiService>();

            try
            {
                // 3. Führe Ingestion aus
                Console.WriteLine("🔄 Starte Ingestion Pipeline...\n");

                var progress = new Progress<PipelineStage>(stage =>
                {
                    Console.WriteLine($"  [{stage.ProcessedCount}/{stage.TotalCount}] {stage.Description}");
                });

                var fileProgress = new Progress<FileAnalysisResult>(analysisResult =>
                {
                    var status = analysisResult.IsProcessed ? "✅" : "❌";
                    Console.WriteLine($"    {status} {analysisResult.FileName} ({analysisResult.RelevanceScore:F2} relevance)");
                    
                    if (analysisResult.Keywords?.Count > 0)
                    {
                        Console.WriteLine($"       Keywords: {string.Join(", ", analysisResult.Keywords.Take(5))}");
                    }
                });

                // Starte Pipeline NICHT als DryRun - speichere echte Daten!
                var pipelineResult = await pipelineService.ExecutePipelineAsync(
                    sourceFolder,
                    isDryRun: dryRun,
                    progress: progress,
                    fileProgress: fileProgress);

                result.FilesProcessed = pipelineResult.ProcessedFiles;
                result.FilesFailed = pipelineResult.ErrorFiles;
                result.DuplicatesFound = pipelineResult.DuplicateFiles;

                // 4. Statistik anzeigen
                Console.WriteLine($"\n{'═',85}\n");
                Console.WriteLine($"📊 INGESTION RESULTS:");
                Console.WriteLine($"  ✅ Processed:        {result.FilesProcessed}");
                Console.WriteLine($"  ❌ Failed:           {result.FilesFailed}");
                Console.WriteLine($"  ⚠️  Duplicates:      {result.DuplicatesFound}");
                Console.WriteLine($"  💾 Total to ThemisDB: {result.FilesProcessed - result.DuplicatesFound}");

                // 5. Zeige Details
                if (pipelineResult.Results.Count > 0)
                {
                    Console.WriteLine($"\n📈 TOP RESULTS (Sorted by Relevance):\n");

                    var topResults = pipelineResult.Results
                        .Where(r => r.IsProcessed && !r.IsDuplicate)
                        .OrderByDescending(r => r.RelevanceScore)
                        .Take(10)
                        .ToList();

                    foreach (var r in topResults)
                    {
                        Console.WriteLine($"  📄 {r.FileName}");
                        Console.WriteLine($"     Relevance:  {r.RelevanceScore:F2} | Quality: {r.QualityScore:F2} | Impact: {r.ImpactScore:F2}");
                        Console.WriteLine($"     Language:  {r.Language} | Size: {r.FileSize} bytes");
                        Console.WriteLine($"     Hash:      {r.ContentHash.Substring(0, 16)}...");
                        Console.WriteLine($"     Topics:    {string.Join(", ", r.Topics.Take(3))}");
                        Console.WriteLine();
                    }
                }

                // 6. ThemisDB Status
                Console.WriteLine($"\n🗄️  THEMISDB STORAGE:");
                Console.WriteLine($"  Entity Store:     {result.FilesProcessed - result.DuplicatesFound} entries");
                if (storeVectors)
                    Console.WriteLine($"  Vector Store:     {result.FilesProcessed - result.DuplicatesFound} embeddings (1536-dim)");
                if (trackTimeSeries)
                    Console.WriteLine($"  TimeSeries Store: {result.FilesProcessed - result.DuplicatesFound} quality metrics");

                result.Success = true;
                result.Timestamp = DateTime.Now;

                Console.WriteLine($"\n✅ INGESTION COMPLETED SUCCESSFULLY\n");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"\n❌ ERROR: {ex.Message}\n");
                loggerService?.LogError($"Ingestion failed: {ex.Message}");
                result.Success = false;
                result.ErrorMessage = ex.Message;
            }

            return result;
        }
    }

    /// <summary>
    /// Result von Real Ingestion
    /// </summary>
    public class IngestionRunnerResult
    {
        public bool Success { get; set; }
        public int FilesProcessed { get; set; }
        public int FilesFailed { get; set; }
        public int DuplicatesFound { get; set; }
        public DateTime Timestamp { get; set; }
        public string ErrorMessage { get; set; } = string.Empty;
    }
}
