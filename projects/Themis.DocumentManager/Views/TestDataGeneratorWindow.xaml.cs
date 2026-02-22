/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TestDataGeneratorWindow.xaml.cs                    ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     35                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using Themis.DocumentManager.ViewModels;

namespace Themis.DocumentManager.Views;

/// <summary>
/// Interaction logic for TestDataGeneratorWindow.xaml
/// </summary>
public partial class TestDataGeneratorWindow : Window
{
    public TestDataGeneratorWindow(TestDataGeneratorViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
    }
}
