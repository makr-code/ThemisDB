/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainViewModel.cs                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:23:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     190                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using Themis.ImpactAnalysisViewer.Models;
using Themis.ImpactAnalysisViewer.Services;

namespace Themis.ImpactAnalysisViewer.ViewModels;

public partial class MainViewModel : ObservableObject
{
    private readonly ImpactAnalysisService _analysisService;

    [ObservableProperty]
    private string _serverUrl = "http://localhost:8529";

    [ObservableProperty]
    private bool _isConnected;

    [ObservableProperty]
    private string _statusMessage = "Not connected";

    [ObservableProperty]
    private DocumentChange _currentChange = new();

    [ObservableProperty]
    private ImpactAnalysisResult? _currentResult;

    [ObservableProperty]
    private bool _isAnalyzing;

    [ObservableProperty]
    private string _viewMode = "2D";  // 2D or 3D

    [ObservableProperty]
    private bool _enableHybridSearch;

    [ObservableProperty]
    private ObservableCollection<string> _selectedLayers = new()
    {
        "document", "process", "api", "database", "ui"
    };

    [ObservableProperty]
    private LayerConfiguration _layerConfig = new();

    public MainViewModel()
    {
        _analysisService = new ImpactAnalysisService(_serverUrl);
        
        // Initialize with sample data for testing
        CurrentChange = new DocumentChange
        {
            DocumentId = "api/v2/payment/process",
            ChangeType = "breaking_change",
            Magnitude = 0.95,
            SourceLayer = "api",
            Description = "Changed payment API signature"
        };
    }

    [RelayCommand]
    private async Task ConnectAsync()
    {
        try
        {
            IsConnected = await _analysisService.TestConnectionAsync();
            StatusMessage = IsConnected 
                ? "Connected to ThemisDB" 
                : "Connection failed";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error: {ex.Message}";
            IsConnected = false;
        }
    }

    [RelayCommand]
    private async Task AnalyzeImpactAsync()
    {
        if (!IsConnected)
        {
            StatusMessage = "Not connected to server";
            return;
        }

        try
        {
            IsAnalyzing = true;
            StatusMessage = "Analyzing impact...";

            var config = new Dictionary<string, object>
            {
                ["use_hybrid_search"] = EnableHybridSearch,
                ["max_depth"] = 5
            };

            if (SelectedLayers.Count > 1)
            {
                CurrentResult = await _analysisService.AnalyzeMultiLayerImpactAsync(
                    CurrentChange,
                    SelectedLayers.ToArray(),
                    config);
            }
            else
            {
                CurrentResult = await _analysisService.AnalyzeImpactAsync(
                    CurrentChange,
                    config);
            }

            StatusMessage = CurrentResult != null
                ? $"Analysis complete: {CurrentResult.TotalAffectedCount} nodes affected"
                : "Analysis failed";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Error: {ex.Message}";
        }
        finally
        {
            IsAnalyzing = false;
        }
    }

    [RelayCommand]
    private void SwitchView(string mode)
    {
        ViewMode = mode;
        StatusMessage = $"Switched to {mode} view";
    }

    [RelayCommand]
    private void ToggleLayer(string layer)
    {
        if (_layerConfig.LayerSettings.TryGetValue(layer, out var settings))
        {
            settings.Visible = !settings.Visible;
        }
    }

    [RelayCommand]
    private void ClearAnalysis()
    {
        CurrentResult = null;
        StatusMessage = "Analysis cleared";
    }

    [RelayCommand]
    private async Task ExportResultsAsync()
    {
        if (CurrentResult == null)
        {
            StatusMessage = "No results to export";
            return;
        }

        try
        {
            // Export implementation would go here
            StatusMessage = "Export functionality coming soon";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Export failed: {ex.Message}";
        }
    }
}
