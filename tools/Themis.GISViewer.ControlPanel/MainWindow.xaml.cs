using System.Windows;
using Themis.GISViewer.ControlPanel.ViewModels;

namespace Themis.GISViewer.ControlPanel;

public partial class MainWindow : Window
{
    public MainWindow(MainViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
    }
}
