using System.Windows;
using RailwayMonitor.WPF.ViewModels;
using MahApps.Metro.Controls;

namespace RailwayMonitor.WPF;

/// <summary>
/// Main Window - Railway Monitoring System
/// </summary>
public partial class MainWindow : MetroWindow
{
    public MainWindow(MainViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
        
        Loaded += async (s, e) => await viewModel.InitializeAsync();
    }
}
