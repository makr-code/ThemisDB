/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FileDetailsView.xaml.cs                            ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:59:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     37                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows.Controls;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Views
{
    public partial class FileDetailsView : UserControl
    {
        public FileDetailsView()
        {
            InitializeComponent();
        }

        public void ShowDetails(FileAnalysisResult result)
        {
            DataContext = result;
        }
    }
}
