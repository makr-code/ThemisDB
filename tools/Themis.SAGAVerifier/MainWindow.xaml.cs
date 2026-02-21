/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:15:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿using System;
using System.Windows;
using Themis.SAGAVerifier.ViewModels;

namespace Themis.SAGAVerifier
{
    public partial class MainWindow : Window
    {
        private bool _sidebarVisible = true;

        public MainWindow(MainViewModel viewModel)
        {
            InitializeComponent();
            DataContext = viewModel;
            Loaded += async (s, e) =>
            {
                try
                {
                    await viewModel.LoadBatchesCommand.ExecuteAsync(null);
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Error loading batches: {ex.Message}\n\n{ex.StackTrace}", 
                        "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            };
        }

        private void AboutButton_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new AboutWindow { Owner = this };
            dlg.ShowDialog();
        }

        private void HamburgerToggle_Click(object sender, RoutedEventArgs e)
        {
            // Toggle sidebar visibility
            _sidebarVisible = !_sidebarVisible;
            if (SidebarPanel != null) SidebarPanel.Visibility = _sidebarVisible ? Visibility.Visible : Visibility.Collapsed;
            if (SidebarSplitter != null) SidebarSplitter.Visibility = _sidebarVisible ? Visibility.Visible : Visibility.Collapsed;
        }
    }
}
