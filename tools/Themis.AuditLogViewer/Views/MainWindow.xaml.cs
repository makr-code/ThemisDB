/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using Themis.AuditLogViewer.ViewModels;

namespace Themis.AuditLogViewer.Views;

public partial class MainWindow : Window
{
    private bool _sidebarVisible = true;

    public MainWindow(MainWindowViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
    }

    private void AboutButton_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new AboutWindow { Owner = this };
        dlg.ShowDialog();
    }

    private void HamburgerToggle_Click(object sender, RoutedEventArgs e)
    {
        _sidebarVisible = !_sidebarVisible;
        if (SidebarPanel != null) SidebarPanel.Visibility = _sidebarVisible ? Visibility.Visible : Visibility.Collapsed;
        if (SidebarSplitter != null) SidebarSplitter.Visibility = _sidebarVisible ? Visibility.Visible : Visibility.Collapsed;
    }
}
