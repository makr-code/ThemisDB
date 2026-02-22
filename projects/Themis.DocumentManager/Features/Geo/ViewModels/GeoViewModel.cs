/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GeoViewModel.cs                                    ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     645                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Features.Geo.Services;

namespace Themis.DocumentManager.Features.Geo.ViewModels
{
    /// <summary>
    /// ViewModel for GeoView - Manages OSM map state and layer interactions
    /// Follows MVVM pattern for clean separation of concerns
    /// </summary>
    public partial class GeoViewModel : INotifyPropertyChanged
    {
        private readonly IOsmMapRenderer _mapRenderer;
        private readonly IGeoLayerService _layerService;
        private readonly IGeoFeatureService _featureService;

        private MapConfiguration? _currentMapConfig;
        private ObservableCollection<GeoLayer> _layers;
        private ObservableCollection<GeoFeature> _selectedFeatures;
        private GeoFeature? _selectedFeature;
        private string _mapHtml = string.Empty;
        private bool _isLoading;
        private string _statusMessage = "Bereit";

        public GeoViewModel(
            IOsmMapRenderer mapRenderer,
            IGeoLayerService layerService,
            IGeoFeatureService featureService)
        {
            _mapRenderer = mapRenderer ?? throw new ArgumentNullException(nameof(mapRenderer));
            _layerService = layerService ?? throw new ArgumentNullException(nameof(layerService));
            _featureService = featureService ?? throw new ArgumentNullException(nameof(featureService));

            _layers = new ObservableCollection<GeoLayer>();
            _selectedFeatures = new ObservableCollection<GeoFeature>();

            // Initialize commands
            LoadMapCommand = new RelayCommand(async () => await LoadMapAsync());
            ToggleLayerCommand = new RelayCommand<GeoLayer>(async layer => await ToggleLayerVisibilityAsync(layer));
            AddFeatureCommand = new RelayCommand<GeoFeature>(async feature => await AddFeatureAsync(feature));
            RemoveFeatureCommand = new RelayCommand<GeoFeature>(async feature => await RemoveFeatureAsync(feature));
            ZoomToFeatureCommand = new RelayCommand<GeoFeature>(async feature => await ZoomToFeatureAsync(feature));
            RefreshLayersCommand = new RelayCommand(async () => await RefreshLayersAsync());
        }

        #region Properties

        public MapConfiguration? CurrentMapConfig
        {
            get => _currentMapConfig;
            set
            {
                if (_currentMapConfig != value)
                {
                    _currentMapConfig = value;
                    OnPropertyChanged();
                }
            }
        }

        public ObservableCollection<GeoLayer> Layers
        {
            get => _layers;
            set
            {
                if (_layers != value)
                {
                    _layers = value;
                    OnPropertyChanged();
                }
            }
        }

        public ObservableCollection<GeoFeature> SelectedFeatures
        {
            get => _selectedFeatures;
            set
            {
                if (_selectedFeatures != value)
                {
                    _selectedFeatures = value;
                    OnPropertyChanged();
                }
            }
        }

        public GeoFeature? SelectedFeature
        {
            get => _selectedFeature;
            set
            {
                if (_selectedFeature != value)
                {
                    _selectedFeature = value;
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(HasSelectedFeature));
                }
            }
        }

        public string MapHtml
        {
            get => _mapHtml;
            set
            {
                if (_mapHtml != value)
                {
                    _mapHtml = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool IsLoading
        {
            get => _isLoading;
            set
            {
                if (_isLoading != value)
                {
                    _isLoading = value;
                    OnPropertyChanged();
                }
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set
            {
                if (_statusMessage != value)
                {
                    _statusMessage = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool HasSelectedFeature => _selectedFeature != null;

        #endregion

        #region Commands

        public ICommand LoadMapCommand { get; }
        public ICommand ToggleLayerCommand { get; }
        public ICommand AddFeatureCommand { get; }
        public ICommand RemoveFeatureCommand { get; }
        public ICommand ZoomToFeatureCommand { get; }
        public ICommand RefreshLayersCommand { get; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Initialize ViewModel with default map configuration
        /// </summary>
        public async Task InitializeAsync()
        {
            await LoadDefaultConfigurationAsync();
            await LoadLayersAsync();
            await GenerateMapHtmlAsync();
        }

        /// <summary>
        /// Load map with specific configuration
        /// </summary>
        public async Task LoadMapAsync(MapConfiguration config)
        {
            CurrentMapConfig = config;
            await LoadLayersAsync();
            await GenerateMapHtmlAsync();
        }

        /// <summary>
        /// Add layer to map
        /// </summary>
        public async Task AddLayerAsync(GeoLayer layer)
        {
            if (layer == null)
                throw new ArgumentNullException(nameof(layer));

            try
            {
                IsLoading = true;
                StatusMessage = $"Füge Layer '{layer.Name}' hinzu...";

                // Save to database
                await _layerService.CreateLayerAsync(layer);

                // Add to collection
                Layers.Add(layer);

                // Regenerate map HTML
                await GenerateMapHtmlAsync();

                StatusMessage = $"Layer '{layer.Name}' erfolgreich hinzugefügt";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Hinzufügen des Layers: {ex.Message}";
                throw;
            }
            finally
            {
                IsLoading = false;
            }
        }

        /// <summary>
        /// Remove layer from map
        /// </summary>
        public async Task RemoveLayerAsync(GeoLayer layer)
        {
            if (layer == null)
                throw new ArgumentNullException(nameof(layer));

            try
            {
                IsLoading = true;
                StatusMessage = $"Entferne Layer '{layer.Name}'...";

                // Delete from database
                await _layerService.DeleteLayerAsync(layer.Id);

                // Remove from collection
                Layers.Remove(layer);

                // Regenerate map HTML
                await GenerateMapHtmlAsync();

                StatusMessage = $"Layer '{layer.Name}' erfolgreich entfernt";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Entfernen des Layers: {ex.Message}";
                throw;
            }
            finally
            {
                IsLoading = false;
            }
        }

        /// <summary>
        /// Toggle layer visibility
        /// </summary>
        public async Task ToggleLayerVisibilityAsync(GeoLayer? layer)
        {
            if (layer == null)
                return;

            layer.IsVisible = !layer.IsVisible;

            // Update in database
            await _layerService.UpdateLayerAsync(layer);

            OnPropertyChanged(nameof(Layers));
        }

        /// <summary>
        /// Add feature to specific layer
        /// </summary>
        public async Task AddFeatureToLayerAsync(string layerId, GeoFeature feature)
        {
            if (string.IsNullOrEmpty(layerId))
                throw new ArgumentNullException(nameof(layerId));
            if (feature == null)
                throw new ArgumentNullException(nameof(feature));

            try
            {
                IsLoading = true;
                StatusMessage = $"Füge Feature '{feature.Name}' hinzu...";

                // Set layer ID
                feature.LayerId = layerId;

                // Save to database
                await _featureService.CreateFeatureAsync(feature);

                StatusMessage = $"Feature '{feature.Name}' erfolgreich hinzugefügt";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Hinzufügen des Features: {ex.Message}";
                throw;
            }
            finally
            {
                IsLoading = false;
            }
        }

        /// <summary>
        /// Load features for specific layer
        /// </summary>
        public async Task<List<GeoFeature>> LoadFeaturesForLayerAsync(string layerId)
        {
            if (string.IsNullOrEmpty(layerId))
                throw new ArgumentNullException(nameof(layerId));

            try
            {
                var query = $@"
                    FOR feature IN GeoFeatures
                        FILTER feature.layerId == '{layerId}'
                        RETURN feature
                ";

                return (await _featureService.GetFeaturesAsync(query)).ToList();
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Laden der Features: {ex.Message}";
                return new List<GeoFeature>();
            }
        }

        #endregion

        #region Private Methods

        private async Task LoadMapAsync()
        {
            await LoadDefaultConfigurationAsync();
            await LoadLayersAsync();
            await GenerateMapHtmlAsync();
        }

        private async Task LoadDefaultConfigurationAsync()
        {
            CurrentMapConfig = new MapConfiguration
            {
                Id = "default-map",
                Name = "Standard Karte",
                DefaultCenter = new MapCenter
                {
                    Latitude = 51.1657,  // Deutschland Zentrum
                    Longitude = 10.4515
                },
                DefaultZoom = 6,
                MinZoom = 3,
                MaxZoom = 18,
                BaseLayerType = "OpenStreetMap",
                EnableClustering = true,
                EnableHeatmap = false,
                ShowLegend = true,
                ShowLayerControl = true
            };

            await Task.CompletedTask;
        }

        private async Task LoadLayersAsync()
        {
            try
            {
                IsLoading = true;
                StatusMessage = "Lade Layer...";

                var query = @"
                    FOR layer IN GeoLayers
                        FILTER layer.isActive == true
                        SORT layer.displayOrder ASC
                        RETURN layer
                ";

                var layers = await _layerService.GetLayersAsync(query);

                Layers.Clear();
                foreach (var layer in layers)
                {
                    Layers.Add(layer);
                }

                // If no layers found, create default examples
                if (Layers.Count == 0)
                {
                    await CreateDefaultLayersAsync();
                }

                StatusMessage = $"{Layers.Count} Layer geladen";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Laden der Layer: {ex.Message}";
                
                // Fallback to default layers
                await CreateDefaultLayersAsync();
            }
            finally
            {
                IsLoading = false;
            }
        }

        private async Task CreateDefaultLayersAsync()
        {
            var defaultLayers = new List<GeoLayer>
            {
                new GeoLayer
                {
                    Id = "markers-layer",
                    Name = "Standorte",
                    Type = LayerType.Markers,
                    IsVisible = true,
                    DisplayOrder = 1,
                    Style = new LayerStyle
                    {
                        Color = "#2196F3",
                        FillColor = "#2196F3",
                        Opacity = 0.8,
                        IconUrl = "https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon.png",
                        IconSize = new int[] { 25, 41 }
                    }
                },
                new GeoLayer
                {
                    Id = "regions-layer",
                    Name = "Regionen",
                    Type = LayerType.Polygons,
                    IsVisible = true,
                    DisplayOrder = 2,
                    Style = new LayerStyle
                    {
                        Color = "#4CAF50",
                        FillColor = "#4CAF50",
                        Opacity = 0.3,
                        Weight = 2
                    }
                },
                new GeoLayer
                {
                    Id = "routes-layer",
                    Name = "Routen",
                    Type = LayerType.Lines,
                    IsVisible = false,
                    DisplayOrder = 3,
                    Style = new LayerStyle
                    {
                        Color = "#FF5722",
                        Opacity = 0.7,
                        Weight = 3
                    }
                }
            };

            Layers.Clear();
            foreach (var layer in defaultLayers)
            {
                Layers.Add(layer);
            }

            await Task.CompletedTask;
        }

        private async Task GenerateMapHtmlAsync()
        {
            if (CurrentMapConfig == null)
                return;

            try
            {
                IsLoading = true;
                StatusMessage = "Generiere Karte...";

                var html = _mapRenderer.GenerateMapHtml(CurrentMapConfig, Layers.ToList());
                MapHtml = html;

                StatusMessage = "Karte erfolgreich generiert";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Generieren der Karte: {ex.Message}";
                throw;
            }
            finally
            {
                IsLoading = false;
            }
        }

        private async Task RefreshLayersAsync()
        {
            await LoadLayersAsync();
            await GenerateMapHtmlAsync();
        }

        private async Task AddFeatureAsync(GeoFeature? feature)
        {
            if (feature == null)
                return;

            // Implementation depends on which layer to add to
            // For now, just add to selected features
            SelectedFeatures.Add(feature);
            await Task.CompletedTask;
        }

        private async Task RemoveFeatureAsync(GeoFeature? feature)
        {
            if (feature == null)
                return;

            try
            {
                await _featureService.DeleteFeatureAsync(feature.Id);
                SelectedFeatures.Remove(feature);
                
                if (SelectedFeature == feature)
                {
                    SelectedFeature = null;
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"Fehler beim Entfernen des Features: {ex.Message}";
            }
        }

        private async Task ZoomToFeatureAsync(GeoFeature? feature)
        {
            if (feature == null)
                return;

            // This would trigger an event that the View can listen to
            // to execute JavaScript zoom command
            SelectedFeature = feature;

            await Task.CompletedTask;
        }

        #endregion

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion
    }

    /// <summary>
    /// Simple RelayCommand implementation for MVVM
    /// </summary>
    public class RelayCommand : ICommand
    {
        private readonly Func<Task> _execute;
        private readonly Func<bool>? _canExecute;

        public RelayCommand(Func<Task> execute, Func<bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public event EventHandler? CanExecuteChanged
        {
            add => CommandManager.RequerySuggested += value;
            remove => CommandManager.RequerySuggested -= value;
        }

        public bool CanExecute(object? parameter)
        {
            return _canExecute?.Invoke() ?? true;
        }

        public async void Execute(object? parameter)
        {
            await _execute();
        }
    }

    /// <summary>
    /// Generic RelayCommand with parameter
    /// </summary>
    public class RelayCommand<T> : ICommand
    {
        private readonly Func<T?, Task> _execute;
        private readonly Func<T?, bool>? _canExecute;

        public RelayCommand(Func<T?, Task> execute, Func<T?, bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public event EventHandler? CanExecuteChanged
        {
            add => CommandManager.RequerySuggested += value;
            remove => CommandManager.RequerySuggested -= value;
        }

        public bool CanExecute(object? parameter)
        {
            return _canExecute?.Invoke((T?)parameter) ?? true;
        }

        public async void Execute(object? parameter)
        {
            await _execute((T?)parameter);
        }
    }
}
