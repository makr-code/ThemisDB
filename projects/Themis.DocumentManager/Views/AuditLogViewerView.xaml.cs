using System.Threading.Tasks;
using System.Windows.Controls;
using Microsoft.Win32;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Views
{
    public partial class AuditLogViewerView : UserControl
    {
        public AuditLogViewerView()
        {
            InitializeComponent();
        }

        private async void ExportCsv_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            if (DataContext is not AuditLogViewerViewModel vm) return;
            var dlg = new SaveFileDialog
            {
                Filter = "CSV-Datei (*.csv)|*.csv|Alle Dateien (*.*)|*.*",
                FileName = $"audit-logs-{System.DateTime.Now:yyyyMMdd-HHmmss}.csv"
            };
            if (dlg.ShowDialog() == true)
            {
                await vm.ExportCsvAsync(dlg.FileName);
            }
        }

        private async void ExportJson_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            if (DataContext is not AuditLogViewerViewModel vm) return;
            var dlg = new SaveFileDialog
            {
                Filter = "JSON-Datei (*.json)|*.json|Alle Dateien (*.*)|*.*",
                FileName = $"audit-logs-{System.DateTime.Now:yyyyMMdd-HHmmss}.json"
            };
            if (dlg.ShowDialog() == true)
            {
                await vm.ExportJsonAsync(dlg.FileName);
            }
        }
    }
}
