/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLogViewerView.xaml.cs                         ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
