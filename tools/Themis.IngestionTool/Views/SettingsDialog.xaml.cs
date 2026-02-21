/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SettingsDialog.xaml.cs                             ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using Themis.IngestionTool.ViewModels;

namespace Themis.IngestionTool.Views
{
    public partial class SettingsDialog : Window
    {
        private readonly SettingsDialogViewModel _viewModel;

        public SettingsDialog(SettingsDialogViewModel viewModel)
        {
            InitializeComponent();
            _viewModel = viewModel;
            DataContext = _viewModel;
        }

        private void OnOk(object sender, RoutedEventArgs e)
        {
            _viewModel.SaveSettings();
            DialogResult = true;
            Close();
        }

        private void OnCancel(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }

        private void OnReset(object sender, RoutedEventArgs e)
        {
            var result = MessageBox.Show(
                "Möchten Sie wirklich alle Einstellungen auf die Standardwerte zurücksetzen?",
                "Zurücksetzen bestätigen",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);
            
            if (result == MessageBoxResult.Yes)
            {
                _viewModel.ResetToDefaults();
            }
        }

        private async void OnScanServers(object sender, RoutedEventArgs e)
        {
            await _viewModel.ScanForServersAsync();
        }

        private void OnApplyServer(object sender, RoutedEventArgs e)
        {
            _viewModel.ApplySelectedServer();
        }
    }
}
