/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:48:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 20c4e9c84  2025-11-02  feat: Complete feature set - Auth, Governance, Compliance... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using Themis.PIIManager.ViewModels;

namespace Themis.PIIManager.Views
{
    public partial class MainWindow : Window
    {
        private bool _sidebarVisible = true;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainViewModel();
        }

        private void AboutButton_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new AboutWindow { Owner = this };
            dlg.ShowDialog();
        }

        private void HamburgerToggle_Click(object sender, RoutedEventArgs e)
        {
            _sidebarVisible = !_sidebarVisible;
            SidebarPanel.Visibility = _sidebarVisible ? Visibility.Visible : Visibility.Collapsed;
            SidebarSplitter.Visibility = _sidebarVisible ? Visibility.Visible : Visibility.Collapsed;
        }
    }
}