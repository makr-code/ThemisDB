/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GeoView.xaml.cs                                    ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     454                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.Wpf;
using Microsoft.Extensions.DependencyInjection;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.Geo.Views
{
    /// <summary>
    /// Interaction logic for GeoView.xaml
    /// Displays OpenStreetMap with dynamic layers using Leaflet.js
    /// </summary>
    public partial class GeoView : UserControl
    {
        private readonly IOsmMapRenderer _mapRenderer;
        
        private WebView2? _webView;
        private MapConfiguration? _currentMapConfig;
        private List<GeoLayer> _layers = new();
        private bool _isMapInitialized = false;

        public GeoView()
        {
            InitializeComponent();
            
            // Services werden via DI injiziert
            _mapRenderer = App.GetService<IOsmMapRenderer>() 
                ?? throw new InvalidOperationException("IOsmMapRenderer not registered");

            Loaded += OnLoaded;
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            await InitializeMapAsync();
        }

        /// <summary>
        /// Initialize WebView2 and load Leaflet map
        /// </summary>
        private async Task InitializeMapAsync()
        {
            try
            {
                // Show loading indicator
                LoadingPanel.Visibility = Visibility.Visible;

                // Create WebView2 instance
                _webView = new WebView2();
                _webView.CoreWebView2InitializationCompleted += OnWebViewInitialized;

                // Add to Frame
                MapFrame.Content = _webView;

                // Initialize WebView2 environment
                await _webView.EnsureCoreWebView2Async(null);

                // Load default map configuration
                await LoadDefaultMapAsync();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler beim Initialisieren der Karte: {ex.Message}", 
                    "Kartenfehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            finally
            {
                LoadingPanel.Visibility = Visibility.Collapsed;
            }
        }

        private void OnWebViewInitialized(object? sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            if (e.IsSuccess)
            {
                _isMapInitialized = true;
                
                // Enable developer tools in debug mode
#if DEBUG
                if (_webView?.CoreWebView2 != null)
                {
                    _webView.CoreWebView2.Settings.AreDevToolsEnabled = true;
                }
#endif
            }
            else
            {
                MessageBox.Show($"WebView2 Initialisierung fehlgeschlagen: {e.InitializationException?.Message}",
                    "WebView2 Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        /// <summary>
        /// Load default map centered on Germany
        /// </summary>
        private async Task LoadDefaultMapAsync()
        {
            // Default: Deutschland Zentrum
            _currentMapConfig = new MapConfiguration
            {
                Id = "default-map",
                Name = "Standard Karte",
                DefaultCenter = new MapCenter 
                { 
                    Latitude = 51.1657, 
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

            // Load layers from service
            await LoadLayersAsync();

            // Generate and display map HTML
            await RenderMapAsync();
        }

        /// <summary>
        /// Load geo layers from service
        /// </summary>
        private async Task LoadLayersAsync()
        {
            try
            {
                // For now, create default example layers
                // In production, would query AQL via ThemisDB
                _layers = CreateDefaultLayers();
            }
            catch
            {
                // Fallback to default layers
                _layers = CreateDefaultLayers();
            }
            
            // Populate layer list in UI
            PopulateLayerList();
        }

        /// <summary>
        /// Create default example layers
        /// </summary>
        private List<GeoLayer> CreateDefaultLayers()
        {
            return new List<GeoLayer>
            {
                new GeoLayer
                {
                    Id = "markers-layer",
                    Name = "Standorte",
                    Type = LayerType.Markers,
                    IsVisible = true,
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
                    Style = new LayerStyle
                    {
                        Color = "#FF5722",
                        Opacity = 0.7,
                        Weight = 3
                    }
                }
            };
        }

        /// <summary>
        /// Populate layer control sidebar with layer items
        /// </summary>
        private void PopulateLayerList()
        {
            LayerListPanel.Children.Clear();

            foreach (var layer in _layers)
            {
                var layerItem = CreateLayerControlItem(layer);
                LayerListPanel.Children.Add(layerItem);
            }
        }

        /// <summary>
        /// Create UI control for single layer
        /// </summary>
        private Border CreateLayerControlItem(GeoLayer layer)
        {
            var border = new Border
            {
                Margin = new Thickness(0, 0, 0, 8),
                Padding = new Thickness(8),
                Background = System.Windows.Media.Brushes.White,
                BorderBrush = System.Windows.Media.Brushes.LightGray,
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(4)
            };

            var grid = new Grid();
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

            // Layer name
            var nameText = new TextBlock
            {
                Text = layer.Name,
                FontWeight = FontWeights.Medium,
                VerticalAlignment = VerticalAlignment.Center
            };
            Grid.SetColumn(nameText, 0);
            grid.Children.Add(nameText);

            // Visibility toggle
            var visibilityCheck = new CheckBox
            {
                IsChecked = layer.IsVisible,
                VerticalAlignment = VerticalAlignment.Center,
                Tag = layer.Id
            };
            visibilityCheck.Checked += OnLayerVisibilityChanged;
            visibilityCheck.Unchecked += OnLayerVisibilityChanged;
            Grid.SetColumn(visibilityCheck, 1);
            grid.Children.Add(visibilityCheck);

            border.Child = grid;
            return border;
        }

        /// <summary>
        /// Handle layer visibility toggle
        /// </summary>
        private async void OnLayerVisibilityChanged(object sender, RoutedEventArgs e)
        {
            if (sender is CheckBox checkbox && checkbox.Tag is string layerId)
            {
                var layer = _layers.FirstOrDefault(l => l.Id == layerId);
                if (layer != null)
                {
                    layer.IsVisible = checkbox.IsChecked ?? false;
                    await UpdateLayerVisibilityAsync(layerId, layer.IsVisible);
                }
            }
        }

        /// <summary>
        /// Update layer visibility in map via JavaScript
        /// </summary>
        private async Task UpdateLayerVisibilityAsync(string layerId, bool isVisible)
        {
            if (_webView?.CoreWebView2 == null || !_isMapInitialized)
                return;

            try
            {
                var script = isVisible
                    ? $"if (window.layers && window.layers['{layerId}']) {{ window.layers['{layerId}'].addTo(map); }}"
                    : $"if (window.layers && window.layers['{layerId}']) {{ map.removeLayer(window.layers['{layerId}']); }}";

                await _webView.CoreWebView2.ExecuteScriptAsync(script);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error updating layer visibility: {ex.Message}");
            }
        }

        /// <summary>
        /// Render complete map with all layers
        /// </summary>
        private async Task RenderMapAsync()
        {
            if (_webView?.CoreWebView2 == null || _currentMapConfig == null)
                return;

            try
            {
                LoadingPanel.Visibility = Visibility.Visible;

                // Generate HTML via OsmMapRenderer service
                var html = _mapRenderer.GenerateMapHtml(_currentMapConfig, _layers);

                // Navigate to HTML
                _webView.CoreWebView2.NavigateToString(html);

                // Update info panel
                InfoText.Text = $"Karte geladen: {_layers.Count} Layer verfügbar";
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler beim Rendern der Karte: {ex.Message}",
                    "Render-Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            finally
            {
                LoadingPanel.Visibility = Visibility.Collapsed;
            }
        }

        /// <summary>
        /// Public method to load specific map configuration
        /// </summary>
        public async Task LoadMapConfigurationAsync(MapConfiguration config)
        {
            _currentMapConfig = config;
            await LoadLayersAsync();
            await RenderMapAsync();
        }

        /// <summary>
        /// Public method to add feature to specific layer
        /// </summary>
        public async Task AddFeatureToLayerAsync(string layerId, GeoFeature feature)
        {
            if (_webView?.CoreWebView2 == null || !_isMapInitialized)
                return;

            try
            {
                var layer = _layers.FirstOrDefault(l => l.Id == layerId);
                if (layer == null)
                    return;

                // Generate feature JavaScript
                var featureJs = GenerateFeatureJavaScript(feature);
                
                var script = $@"
                    if (window.layers && window.layers['{layerId}']) {{
                        var feature = {featureJs};
                        L.geoJSON(feature, {{
                            pointToLayer: function(geoJsonPoint, latlng) {{
                                return L.marker(latlng);
                            }}
                        }}).addTo(window.layers['{layerId}']);
                    }}
                ";

                await _webView.CoreWebView2.ExecuteScriptAsync(script);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error adding feature: {ex.Message}");
            }
        }

        /// <summary>
        /// Generate GeoJSON JavaScript for feature
        /// </summary>
        private string GenerateFeatureJavaScript(GeoFeature feature)
        {
            // Simple point feature example
            if (feature.Geometry.Type == "Point" && feature.Geometry.Coordinates is double[] coords && coords.Length >= 2)
            {
                return $@"{{
                    ""type"": ""Feature"",
                    ""geometry"": {{
                        ""type"": ""Point"",
                        ""coordinates"": [{coords[0]}, {coords[1]}]
                    }},
                    ""properties"": {{
                        ""name"": ""{feature.Name}"",
                        ""description"": ""{feature.Description}""
                    }}
                }}";
            }

            return "{}";
        }

        /// <summary>
        /// Zoom map to specific bounds
        /// </summary>
        public async Task ZoomToBoundsAsync(double minLat, double minLon, double maxLat, double maxLon)
        {
            if (_webView?.CoreWebView2 == null || !_isMapInitialized)
                return;

            try
            {
                var script = $@"
                    map.fitBounds([
                        [{minLat}, {minLon}],
                        [{maxLat}, {maxLon}]
                    ]);
                ";

                await _webView.CoreWebView2.ExecuteScriptAsync(script);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error zooming to bounds: {ex.Message}");
            }
        }
    }
}
