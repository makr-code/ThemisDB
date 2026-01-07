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
