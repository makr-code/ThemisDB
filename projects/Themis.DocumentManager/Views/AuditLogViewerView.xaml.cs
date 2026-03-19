/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLogViewerView.xaml.cs                         ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     65                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
