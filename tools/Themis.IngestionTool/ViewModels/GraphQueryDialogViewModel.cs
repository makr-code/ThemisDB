/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphQueryDialogViewModel.cs                       ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     374                                            ║
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
    /// ViewModel für Graph Query Interface - Traversal, Path Finding, Communities
    /// </summary>
    public class GraphQueryDialogViewModel : ViewModelBase
    {
        private readonly IGraphQueryService _graphQueryService;
        private readonly ILoggerService _loggerService;

        // Traversal Properties
        private string _traversalSourceEntityId = string.Empty;
        private int _traversalMaxDepth = 3;
        private string _traversalRelationshipType = string.Empty;
        private bool _traversalIsLoading = false;
        private string _traversalStatus = "Bereit";

        // Path Finding Properties
        private string _pathSourceEntityId = string.Empty;
        private string _pathTargetEntityId = string.Empty;
        private int _pathMaxDepth = 5;
        private bool _pathIsLoading = false;
        private string _pathStatus = "Bereit";

        // Neighborhood Properties
        private string _neighborhoodEntityId = string.Empty;
        private int _neighborhoodDistance = 2;
        private bool _neighborhoodIsLoading = false;
        private string _neighborhoodStatus = "Bereit";

        // Community Detection Properties
        private int _communityMinSize = 3;
        private bool _communityIsLoading = false;
        private string _communityStatus = "Bereit";

        // Results
        public ObservableCollection<EntityNode> TraversalEntities { get; } = new();
        public ObservableCollection<RelationshipEdge> TraversalRelationships { get; } = new();
        public ObservableCollection<string> PathResult { get; } = new();
        public ObservableCollection<EntityNode> NeighborhoodEntities { get; } = new();
        public ObservableCollection<RelationshipEdge> NeighborhoodRelationships { get; } = new();
        public ObservableCollection<Community> CommunityResults { get; } = new();

        // Commands
        public ICommand TraverseCommand { get; }
        public ICommand FindPathCommand { get; }
        public ICommand GetNeighborhoodCommand { get; }
        public ICommand DetectCommunitiesCommand { get; }
        public ICommand ClearResultsCommand { get; }

        public GraphQueryDialogViewModel(
            IGraphQueryService graphQueryService,
            ILoggerService loggerService)
        {
            _graphQueryService = graphQueryService;
            _loggerService = loggerService;

            TraverseCommand = new RelayCommand(async () => await ExecuteTraversal());
            FindPathCommand = new RelayCommand(async () => await ExecutePathFinding());
            GetNeighborhoodCommand = new RelayCommand(async () => await ExecuteNeighborhood());
            DetectCommunitiesCommand = new RelayCommand(async () => await ExecuteCommunityDetection());
            ClearResultsCommand = new RelayCommand(() => ClearAllResults());
        }

        #region Traversal

        public string TraversalSourceEntityId
        {
            get => _traversalSourceEntityId;
            set { SetProperty(ref _traversalSourceEntityId, value); }
        }

        public int TraversalMaxDepth
        {
            get => _traversalMaxDepth;
            set { SetProperty(ref _traversalMaxDepth, value); }
        }

        public string TraversalRelationshipType
        {
            get => _traversalRelationshipType;
            set { SetProperty(ref _traversalRelationshipType, value); }
        }

        public bool TraversalIsLoading
        {
            get => _traversalIsLoading;
            set { SetProperty(ref _traversalIsLoading, value); }
        }

        public string TraversalStatus
        {
            get => _traversalStatus;
            set { SetProperty(ref _traversalStatus, value); }
        }

        private async Task ExecuteTraversal()
        {
            if (string.IsNullOrWhiteSpace(TraversalSourceEntityId))
            {
                TraversalStatus = "❌ Source Entity ID erforderlich";
                return;
            }

            TraversalIsLoading = true;
            TraversalStatus = "⏳ Traversiere Graph...";

            try
            {
                var result = await _graphQueryService.TraverseRelationshipsAsync(
                    TraversalSourceEntityId,
                    TraversalMaxDepth,
                    TraversalRelationshipType ?? null);

                TraversalEntities.Clear();
                foreach (var entity in result.Entities)
                    TraversalEntities.Add(entity);

                TraversalRelationships.Clear();
                foreach (var rel in result.Relationships)
                    TraversalRelationships.Add(rel);

                TraversalStatus = $"✅ {result.Entities.Count} Entities, {result.Relationships.Count} Beziehungen ({result.ExecutionTimeMs}ms)";
                _loggerService.LogInfo($"Traversal erfolgreich: {result.Entities.Count} Entities");
            }
            catch (Exception ex)
            {
                TraversalStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Traversal Fehler: {ex.Message}");
            }
            finally
            {
                TraversalIsLoading = false;
            }
        }

        #endregion

        #region Path Finding

        public string PathSourceEntityId
        {
            get => _pathSourceEntityId;
            set { SetProperty(ref _pathSourceEntityId, value); }
        }

        public string PathTargetEntityId
        {
            get => _pathTargetEntityId;
            set { SetProperty(ref _pathTargetEntityId, value); }
        }

        public int PathMaxDepth
        {
            get => _pathMaxDepth;
            set { SetProperty(ref _pathMaxDepth, value); }
        }

        public bool PathIsLoading
        {
            get => _pathIsLoading;
            set { SetProperty(ref _pathIsLoading, value); }
        }

        public string PathStatus
        {
            get => _pathStatus;
            set { SetProperty(ref _pathStatus, value); }
        }

        private async Task ExecutePathFinding()
        {
            if (string.IsNullOrWhiteSpace(PathSourceEntityId) || string.IsNullOrWhiteSpace(PathTargetEntityId))
            {
                PathStatus = "❌ Source und Target Entity ID erforderlich";
                return;
            }

            PathIsLoading = true;
            PathStatus = "⏳ Finde Pfad...";

            try
            {
                var result = await _graphQueryService.FindPathAsync(
                    PathSourceEntityId,
                    PathTargetEntityId,
                    PathMaxDepth);

                PathResult.Clear();
                if (result.PathFound)
                {
                    foreach (var entityId in result.Path)
                        PathResult.Add(entityId);
                    PathStatus = $"✅ Pfad gefunden: {result.PathLength} Hops ({result.ExecutionTimeMs}ms)";
                    _loggerService.LogInfo($"Pfad gefunden mit {result.PathLength} Hops");
                }
                else
                {
                    PathStatus = "⚠️ Kein Pfad gefunden";
                    _loggerService.LogInfo("Kein Pfad zwischen Entities gefunden");
                }
            }
            catch (Exception ex)
            {
                PathStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Path Finding Fehler: {ex.Message}");
            }
            finally
            {
                PathIsLoading = false;
            }
        }

        #endregion

        #region Neighborhood

        public string NeighborhoodEntityId
        {
            get => _neighborhoodEntityId;
            set { SetProperty(ref _neighborhoodEntityId, value); }
        }

        public int NeighborhoodDistance
        {
            get => _neighborhoodDistance;
            set { SetProperty(ref _neighborhoodDistance, value); }
        }

        public bool NeighborhoodIsLoading
        {
            get => _neighborhoodIsLoading;
            set { SetProperty(ref _neighborhoodIsLoading, value); }
        }

        public string NeighborhoodStatus
        {
            get => _neighborhoodStatus;
            set { SetProperty(ref _neighborhoodStatus, value); }
        }

        private async Task ExecuteNeighborhood()
        {
            if (string.IsNullOrWhiteSpace(NeighborhoodEntityId))
            {
                NeighborhoodStatus = "❌ Entity ID erforderlich";
                return;
            }

            NeighborhoodIsLoading = true;
            NeighborhoodStatus = "⏳ Lade Nachbarschaft...";

            try
            {
                var result = await _graphQueryService.GetNeighborhoodAsync(
                    NeighborhoodEntityId,
                    NeighborhoodDistance);

                NeighborhoodEntities.Clear();
                foreach (var entity in result.Entities)
                    NeighborhoodEntities.Add(entity);

                NeighborhoodRelationships.Clear();
                foreach (var rel in result.Relationships)
                    NeighborhoodRelationships.Add(rel);

                NeighborhoodStatus = $"✅ {result.Entities.Count} Nachbarn ({result.ExecutionTimeMs}ms)";
                _loggerService.LogInfo($"Nachbarschaft geladen: {result.Entities.Count} Entities");
            }
            catch (Exception ex)
            {
                NeighborhoodStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Nachbarschafts-Fehler: {ex.Message}");
            }
            finally
            {
                NeighborhoodIsLoading = false;
            }
        }

        #endregion

        #region Community Detection

        public int CommunityMinSize
        {
            get => _communityMinSize;
            set { SetProperty(ref _communityMinSize, value); }
        }

        public bool CommunityIsLoading
        {
            get => _communityIsLoading;
            set { SetProperty(ref _communityIsLoading, value); }
        }

        public string CommunityStatus
        {
            get => _communityStatus;
            set { SetProperty(ref _communityStatus, value); }
        }

        private async Task ExecuteCommunityDetection()
        {
            CommunityIsLoading = true;
            CommunityStatus = "⏳ Erkenne Communities...";

            try
            {
                var result = await _graphQueryService.DetectCommunitiesAsync(CommunityMinSize);

                CommunityResults.Clear();
                foreach (var community in result.Communities)
                    CommunityResults.Add(community);

                CommunityStatus = $"✅ {result.Communities.Count} Communities (Modularität: {result.Modularity:F3}) ({result.ExecutionTimeMs}ms)";
                _loggerService.LogInfo($"Community Detection: {result.Communities.Count} Communities gefunden");
            }
            catch (Exception ex)
            {
                CommunityStatus = $"❌ Fehler: {ex.Message}";
                _loggerService.LogError($"Community Detection Fehler: {ex.Message}");
            }
            finally
            {
                CommunityIsLoading = false;
            }
        }

        #endregion

        private void ClearAllResults()
        {
            TraversalEntities.Clear();
            TraversalRelationships.Clear();
            PathResult.Clear();
            NeighborhoodEntities.Clear();
            NeighborhoodRelationships.Clear();
            CommunityResults.Clear();

            TraversalStatus = "Bereit";
            PathStatus = "Bereit";
            NeighborhoodStatus = "Bereit";
            CommunityStatus = "Bereit";
        }
    }
}
