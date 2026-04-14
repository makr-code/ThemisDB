/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:23:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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