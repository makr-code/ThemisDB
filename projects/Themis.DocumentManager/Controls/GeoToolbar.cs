/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GeoToolbar.cs                                      ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     284                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Themis.DocumentManager.Controls;

/// <summary>
/// Custom Toolbar für GeoView mit Map-Operationen.
/// Phase 29 - Custom Controls.
/// </summary>
public class GeoToolbar : ToolBar
{
    #region Dependency Properties

    public static readonly DependencyProperty ZoomLevelProperty =
        DependencyProperty.Register(
            nameof(ZoomLevel),
            typeof(int),
            typeof(GeoToolbar),
            new PropertyMetadata(10, OnZoomLevelChanged));

    public static readonly DependencyProperty CurrentLayerProperty =
        DependencyProperty.Register(
            nameof(CurrentLayer),
            typeof(string),
            typeof(GeoToolbar),
            new PropertyMetadata("OpenStreetMap"));

    public static readonly DependencyProperty IsHeatmapEnabledProperty =
        DependencyProperty.Register(
            nameof(IsHeatmapEnabled),
            typeof(bool),
            typeof(GeoToolbar),
            new PropertyMetadata(false));

    #endregion

    #region Properties

    public int ZoomLevel
    {
        get => (int)GetValue(ZoomLevelProperty);
        set => SetValue(ZoomLevelProperty, value);
    }

    public string CurrentLayer
    {
        get => (string)GetValue(CurrentLayerProperty);
        set => SetValue(CurrentLayerProperty, value);
    }

    public bool IsHeatmapEnabled
    {
        get => (bool)GetValue(IsHeatmapEnabledProperty);
        set => SetValue(IsHeatmapEnabledProperty, value);
    }

    #endregion

    #region Commands

    public ICommand ZoomInCommand { get; }
    public ICommand ZoomOutCommand { get; }
    public ICommand ResetViewCommand { get; }
    public ICommand ToggleLayersCommand { get; }
    public ICommand ToggleHeatmapCommand { get; }
    public ICommand ExportMapCommand { get; }

    #endregion

    #region Events

    public event EventHandler<int>? ZoomChanged;
    public event EventHandler<string>? LayerChanged;
    public event EventHandler<bool>? HeatmapToggled;
    public event EventHandler? ExportRequested;

    #endregion

    static GeoToolbar()
    {
        DefaultStyleKeyProperty.OverrideMetadata(
            typeof(GeoToolbar),
            new FrameworkPropertyMetadata(typeof(GeoToolbar)));
    }

    public GeoToolbar()
    {
        ZoomInCommand = new RelayCommand(ZoomIn);
        ZoomOutCommand = new RelayCommand(ZoomOut);
        ResetViewCommand = new RelayCommand(ResetView);
        ToggleLayersCommand = new RelayCommand(ToggleLayers);
        ToggleHeatmapCommand = new RelayCommand(ToggleHeatmap);
        ExportMapCommand = new RelayCommand(ExportMap);

        BuildToolbarItems();
    }

    private void BuildToolbarItems()
    {
        Items.Clear();

        // Zoom Buttons
        Items.Add(CreateButton("🔍+", ZoomInCommand, "Vergrößern (Strg++)"));
        Items.Add(CreateButton("🔍-", ZoomOutCommand, "Verkleinern (Strg+-)"));
        Items.Add(new Separator());

        // Layer Button
        Items.Add(CreateButton("🗺️", ToggleLayersCommand, "Layer auswählen"));
        Items.Add(new Separator());

        // Heatmap Toggle
        var heatmapButton = CreateToggleButton("🔥", "Heatmap aktivieren");
        heatmapButton.Click += (s, e) => ToggleHeatmap();
        Items.Add(heatmapButton);
        Items.Add(new Separator());

        // Reset View
        Items.Add(CreateButton("🏠", ResetViewCommand, "Ansicht zurücksetzen"));
        Items.Add(new Separator());

        // Export
        Items.Add(CreateButton("💾", ExportMapCommand, "Karte exportieren"));
    }

    private Button CreateButton(string content, ICommand command, string tooltip)
    {
        var button = new Button
        {
            Content = content,
            Command = command,
            ToolTip = tooltip,
            Width = 40,
            Height = 40,
            Margin = new Thickness(2)
        };

        try
        {
            button.Style = System.Windows.Application.Current?.FindResource("MapToolbarButtonStyle") as Style;
        }
        catch
        {
            // Fallback if style not found
        }

        return button;
    }

    private System.Windows.Controls.Primitives.ToggleButton CreateToggleButton(string content, string tooltip)
    {
        return new System.Windows.Controls.Primitives.ToggleButton
        {
            Content = content,
            ToolTip = tooltip,
            Width = 40,
            Height = 40,
            Margin = new Thickness(2)
        };
    }

    #region Command Handlers

    private void ZoomIn()
    {
        if (ZoomLevel < 18)
        {
            ZoomLevel++;
            ZoomChanged?.Invoke(this, ZoomLevel);
        }
    }

    private void ZoomOut()
    {
        if (ZoomLevel > 1)
        {
            ZoomLevel--;
            ZoomChanged?.Invoke(this, ZoomLevel);
        }
    }

    private void ResetView()
    {
        ZoomLevel = 10;
        ZoomChanged?.Invoke(this, ZoomLevel);
    }

    private void ToggleLayers()
    {
        // Show layer selection popup
        var popup = new System.Windows.Controls.Primitives.Popup
        {
            PlacementTarget = this,
            Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom,
            StaysOpen = false
        };

        var listBox = new ListBox
        {
            Width = 200,
            Items = { "OpenStreetMap", "Satellite", "Terrain", "Dark Mode" }
        };

        listBox.SelectionChanged += (s, e) =>
        {
            if (listBox.SelectedItem != null)
            {
                CurrentLayer = listBox.SelectedItem.ToString() ?? "OpenStreetMap";
                LayerChanged?.Invoke(this, CurrentLayer);
                popup.IsOpen = false;
            }
        };

        popup.Child = listBox;
        popup.IsOpen = true;
    }

    private void ToggleHeatmap()
    {
        IsHeatmapEnabled = !IsHeatmapEnabled;
        HeatmapToggled?.Invoke(this, IsHeatmapEnabled);
    }

    private void ExportMap()
    {
        ExportRequested?.Invoke(this, EventArgs.Empty);
    }

    #endregion

    private static void OnZoomLevelChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is GeoToolbar toolbar)
        {
            toolbar.ZoomChanged?.Invoke(toolbar, (int)e.NewValue);
        }
    }

    #region RelayCommand Implementation

    private class RelayCommand : ICommand
    {
        private readonly Action _execute;
        private readonly Func<bool>? _canExecute;

        public event EventHandler? CanExecuteChanged
        {
            add => CommandManager.RequerySuggested += value;
            remove => CommandManager.RequerySuggested -= value;
        }

        public RelayCommand(Action execute, Func<bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

        public void Execute(object? parameter) => _execute();
    }

    #endregion
}
