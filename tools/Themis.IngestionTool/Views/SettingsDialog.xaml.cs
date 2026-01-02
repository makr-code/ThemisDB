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
    }
}
