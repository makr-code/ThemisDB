/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLogViewerView.xaml.cs                         ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
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
