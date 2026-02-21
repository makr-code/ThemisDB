/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     143                                            ║
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

using System.Windows;
using System.Windows.Input;
using RailwayMonitor.WPF.ViewModels;
using RailwayMonitor.WPF.Services;

namespace RailwayMonitor.WPF;

/// <summary>
/// MainWindow für Railway Monitoring System
/// Folgt DMS Best Practices: Minimales Code-Behind, 4-Row Layout, Service Injection
/// </summary>
public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;
    
    // Static Commands für XAML Binding
    public static RoutedCommand FocusSearchCommand = new RoutedCommand();
    public static RoutedCommand ToggleLeftSidebarCommand = new RoutedCommand();
    public static RoutedCommand ToggleRightSidebarCommand = new RoutedCommand();

    public MainWindow(MainViewModel viewModel)
    {
        try
        {
            InitializeComponent();
            
            _viewModel = viewModel;
            DataContext = _viewModel;
            
            // Register static command handlers
            CommandBindings.Add(new CommandBinding(FocusSearchCommand, FocusSearch_Executed));
            CommandBindings.Add(new CommandBinding(ToggleLeftSidebarCommand, ToggleLeftSidebar_Executed));
            CommandBindings.Add(new CommandBinding(ToggleRightSidebarCommand, ToggleRightSidebar_Executed));
            
            // Initialize ViewModel
            Loaded += async (s, e) => await _viewModel.InitializeAsync();
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Fehler beim Initialisieren: {ex.Message}", "Fehler", 
                MessageBoxButton.OK, MessageBoxImage.Error);
            throw;
        }
    }

    // Event Handlers für Menu-Navigation (DMS Pattern)
    
    private void MenuFullscreen_Click(object sender, RoutedEventArgs e)
    {
        WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
    }

    private void MenuDocumentation_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = "https://github.com/makr-code/ThemisDB/tree/main/docs/projects",
                UseShellExecute = true
            });
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Dokumentation konnte nicht geöffnet werden: {ex.Message}", "Fehler",
                MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    private void MenuAbout_Click(object sender, RoutedEventArgs e)
    {
        MessageBox.Show(
            "Railway Monitoring System for Deutsche Bahn\n\n" +
            "Version: 1.0.0\n" +
            "Built with .NET 8.0 & WPF\n\n" +
            "Features:\n" +
            "- Real-time train tracking\n" +
            "- Energy management\n" +
            "- AI-powered analysis (Ollama)\n" +
            "- ThemisDB backend",
            "Über Railway Monitor",
            MessageBoxButton.OK,
            MessageBoxImage.Information);
    }

    // Command Handlers
    
    private void FocusSearch_Executed(object sender, ExecutedRoutedEventArgs e)
    {
        SearchBox?.Focus();
    }

    private void ToggleLeftSidebar_Executed(object sender, ExecutedRoutedEventArgs e)
    {
        if (ColLeft.Width.Value > 0)
        {
            ColLeft.Width = new GridLength(0);
        }
        else
        {
            ColLeft.Width = new GridLength(280);
        }
    }

    private void ToggleRightSidebar_Executed(object sender, ExecutedRoutedEventArgs e)
    {
        // Right sidebar is currently unused, but can be toggled
        if (ColRight.Width.Value > 0)
        {
            ColRight.Width = new GridLength(0);
        }
        else
        {
            ColRight.Width = new GridLength(320);
        }
    }
}
