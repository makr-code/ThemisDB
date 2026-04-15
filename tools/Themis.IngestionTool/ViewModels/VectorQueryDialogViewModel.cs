/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            VectorQueryDialogViewModel.cs                      ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:57:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     294                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.ObjectModel;
using System.Windows.Input;
using Themis.IngestionTool.Services;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.ViewModels
{
    /// <summary>
    /// ViewModel für Vector Query Interface - Semantic Search, Similarity
    /// </summary>
    public class VectorQueryDialogViewModel : ViewModelBase
    {
        private readonly IVectorQueryService _vectorQueryService;
        private readonly ILoggerService _loggerService;

        // Search Properties
        private string _searchQuery = string.Empty;
        private int _searchTopK = 10;
        private double _searchThreshold = 0.7;
        private bool _searchIsLoading = false;
        private string _searchStatus = "Bereit";

        // Similarity Properties
        private string _similarityEntityId1 = string.Empty;
        private string _similarityEntityId2 = string.Empty;
        private double _similarityScore = 0;
        private bool _similarityIsLoading = false;
        private string _similarityStatus = "Bereit";

        // Statistics Properties
        private bool _statsIsLoading = false;
        private string _statsStatus = "Bereit";
        private long _totalVectors = 0;
        private int _vectorDimension = 0;
        private double _avgQueryTimeMs = 0;

        // Results
        public ObservableCollection<VectorSearchMatch> SearchResults { get; } = new();
        public VectorStatistics VectorStats { get; private set; } = new();

        // Commands
        public ICommand SearchCommand { get; }
        public ICommand ComputeSimilarityCommand { get; }
        public ICommand GetStatsCommand { get; }
        public ICommand ClearResultsCommand { get; }

        public VectorQueryDialogViewModel(
            IVectorQueryService vectorQueryService,
            ILoggerService loggerService)
        {
            _vectorQueryService = vectorQueryService;
            _loggerService = loggerService;

            SearchCommand = new RelayCommand(async () => await ExecuteSearch());
            ComputeSimilarityCommand = new RelayCommand(async () => await ExecuteSimilarityComputation());
            GetStatsCommand = new RelayCommand(async () => await ExecuteGetStats());
            ClearResultsCommand = new RelayCommand(() => ClearAllResults());
        }

        #region Search

        public string SearchQuery
        {
            get => _searchQuery;
            set { SetProperty(ref _searchQuery, value); }
        }

        public int SearchTopK
        {
            get => _searchTopK;
            set { SetProperty(ref _searchTopK, value); }
        }

        public double SearchThreshold
        {
            get => _searchThreshold;
            set { SetProperty(ref _searchThreshold, value); }
        }

        public bool SearchIsLoading
        {
            get => _searchIsLoading;
            set { SetProperty(ref _searchIsLoading, value); }
        }

        public string SearchStatus
        {
            get => _searchStatus;
            set { SetProperty(ref _searchStatus, value); }
        }

        private async Task ExecuteSearch()
        {
            if (string.IsNullOrWhiteSpace(SearchQuery))
            {
                SearchStatus = "❌ Suchtext erforderlich";
                return;
            }

            SearchIsLoading = true;
            SearchStatus = "⏳ Durchsuche Vektoren...";

            try
            {
                var result = await _vectorQueryService.SearchSimilarAsync(
                    SearchQuery,
                    SearchTopK,
                    SearchThreshold);

                SearchResults.Clear();
                foreach (var match in result.Results)
                    SearchResults.Add(match);

                var filteredCount = result.Results.Count(r => r.SimilarityScore >= SearchThreshold);
                SearchStatus = $"✅ {filteredCount} Treffer gefunden ({result.ExecutionTimeMs}ms)";
                _loggerService.LogInfo($"Vector Search: {filteredCount} Treffer für '{SearchQuery}'");
            }
            catch (Exception ex)
            {
                SearchStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Vector Search Fehler: {ex.Message}");
            }
            finally
            {
                SearchIsLoading = false;
            }
        }

        #endregion

        #region Similarity

        public string SimilarityEntityId1
        {
            get => _similarityEntityId1;
            set { SetProperty(ref _similarityEntityId1, value); }
        }

        public string SimilarityEntityId2
        {
            get => _similarityEntityId2;
            set { SetProperty(ref _similarityEntityId2, value); }
        }

        public double SimilarityScore
        {
            get => _similarityScore;
            set { SetProperty(ref _similarityScore, value); }
        }

        public bool SimilarityIsLoading
        {
            get => _similarityIsLoading;
            set { SetProperty(ref _similarityIsLoading, value); }
        }

        public string SimilarityStatus
        {
            get => _similarityStatus;
            set { SetProperty(ref _similarityStatus, value); }
        }

        private async Task ExecuteSimilarityComputation()
        {
            if (string.IsNullOrWhiteSpace(SimilarityEntityId1) || string.IsNullOrWhiteSpace(SimilarityEntityId2))
            {
                SimilarityStatus = "❌ Beide Entity IDs erforderlich";
                return;
            }

            SimilarityIsLoading = true;
            SimilarityStatus = "⏳ Berechne Ähnlichkeit...";

            try
            {
                var result = await _vectorQueryService.ComputeSimilarityAsync(
                    SimilarityEntityId1,
                    SimilarityEntityId2);

                SimilarityScore = result.SimilarityScore;
                SimilarityStatus = $"✅ Ähnlichkeit: {result.SimilarityScore:P2} ({result.ExecutionTimeMs}ms)";
                _loggerService.LogInfo($"Similarity Score: {result.SimilarityScore:F4}");
            }
            catch (Exception ex)
            {
                SimilarityStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Similarity Computation Fehler: {ex.Message}");
            }
            finally
            {
                SimilarityIsLoading = false;
            }
        }

        #endregion

        #region Statistics

        public long TotalVectors
        {
            get => _totalVectors;
            set { SetProperty(ref _totalVectors, value); }
        }

        public int VectorDimension
        {
            get => _vectorDimension;
            set { SetProperty(ref _vectorDimension, value); }
        }

        public double AvgQueryTimeMs
        {
            get => _avgQueryTimeMs;
            set { SetProperty(ref _avgQueryTimeMs, value); }
        }

        public bool StatsIsLoading
        {
            get => _statsIsLoading;
            set { SetProperty(ref _statsIsLoading, value); }
        }

        public string StatsStatus
        {
            get => _statsStatus;
            set { SetProperty(ref _statsStatus, value); }
        }

        private async Task ExecuteGetStats()
        {
            StatsIsLoading = true;
            StatsStatus = "⏳ Lade Statistiken...";

            try
            {
                var stats = await _vectorQueryService.GetVectorStatsAsync();

                TotalVectors = stats.TotalVectors;
                VectorDimension = stats.VectorDimension;
                AvgQueryTimeMs = stats.AvgQueryTimeMs;
                VectorStats = stats;

                StatsStatus = $"✅ {stats.TotalVectors} Vektoren ({stats.VectorDimension}D) - Avg Query: {stats.AvgQueryTimeMs:F2}ms";
                _loggerService.LogInfo($"Vector Stats: {stats.TotalVectors} vectors, {stats.VectorDimension} dimensions");
            }
            catch (Exception ex)
            {
                StatsStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Stats Fehler: {ex.Message}");
            }
            finally
            {
                StatsIsLoading = false;
            }
        }

        #endregion

        private void ClearAllResults()
        {
            SearchResults.Clear();
            SearchQuery = string.Empty;
            SimilarityScore = 0;
            SimilarityEntityId1 = string.Empty;
            SimilarityEntityId2 = string.Empty;

            SearchStatus = "Bereit";
            SimilarityStatus = "Bereit";
            StatsStatus = "Bereit";
        }
    }
}
