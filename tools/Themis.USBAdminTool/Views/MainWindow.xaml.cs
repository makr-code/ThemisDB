/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using Themis.USBAdminTool.ViewModels;

namespace Themis.USBAdminTool.Views;

public partial class MainWindow : Window
{
    public MainWindow(MainViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;

        // Trigger initial drive scan on startup.
        Loaded += async (_, _) => await viewModel.RefreshDrivesCommand.ExecuteAsync(null);
    }
}
