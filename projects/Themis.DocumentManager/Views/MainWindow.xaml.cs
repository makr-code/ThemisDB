using System.Windows;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Views;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;
    private readonly IOfficeIntegrationService _officeService;
    private bool _isFullscreen = false;
    private WindowState _previousWindowState;
    private WindowStyle _previousWindowStyle;

    public MainWindow(MainViewModel viewModel, IOfficeIntegrationService officeService)
    {
        InitializeComponent();
        _viewModel = viewModel;
        _officeService = officeService;
        DataContext = _viewModel;

        _viewModel.PropertyChanged += ViewModel_PropertyChanged;
        
        Loaded += (s, e) => UpdateMenuItems();
    }

    private void ViewModel_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(MainViewModel.CurrentView))
        {
            LoadView(_viewModel.CurrentView);
        }
    }

    private void LoadView(string viewName)
    {
    }

    private void RibbonTab_Click(object sender, RoutedEventArgs e)
    {
        // Hide all ribbon content panels
        RibbonStartContent.Visibility = Visibility.Collapsed;
        RibbonInsertContent.Visibility = Visibility.Collapsed;
        RibbonViewContent.Visibility = Visibility.Collapsed;
        RibbonModulesContent.Visibility = Visibility.Collapsed;

        // Show the selected ribbon content
        if (sender == TabStart)
            RibbonStartContent.Visibility = Visibility.Visible;
        else if (sender == TabInsert)
            RibbonInsertContent.Visibility = Visibility.Visible;
        else if (sender == TabView)
            RibbonViewContent.Visibility = Visibility.Visible;
        else if (sender == TabModules)
            RibbonModulesContent.Visibility = Visibility.Visible;
    }

    private void RightSidebarTab_Click(object sender, RoutedEventArgs e)
    {
        // Hide all right sidebar content panels
        RightSidebarGraphContent.Visibility = Visibility.Collapsed;
        RightSidebarChatContent.Visibility = Visibility.Collapsed;

        // Show the selected tab content
        if (sender == RightTabGraph)
            RightSidebarGraphContent.Visibility = Visibility.Visible;
        else if (sender == RightTabChat)
            RightSidebarChatContent.Visibility = Visibility.Visible;
    }

    private void ToggleFullscreen_Click(object sender, RoutedEventArgs e)
    {
        _isFullscreen = !_isFullscreen;
        
        if (_isFullscreen)
        {
            _previousWindowState = WindowState;
            _previousWindowStyle = WindowStyle;
            WindowState = WindowState.Normal;
            WindowStyle = WindowStyle.None;
            WindowState = WindowState.Maximized;
        }
        else
        {
            WindowStyle = _previousWindowStyle;
            WindowState = _previousWindowState;
        }
    }

    private void ToggleWindowMode_Click(object sender, RoutedEventArgs e)
    {
        WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
    }

    private void ToggleSidebar_Click(object sender, RoutedEventArgs e)
    {
        if (sender is MenuItem item)
        {
            if (item.Name == "MenuLeftSidebar")
            {
                LeftSidebarColumn.Width = item.IsChecked ? new GridLength(250) : new GridLength(0);
            }
            else if (item.Name == "MenuRightSidebar")
            {
                RightSidebarColumn.Width = item.IsChecked ? new GridLength(300) : new GridLength(0);
            }
        }
    }

    private void UpdateMenuItems()
    {
    }

    protected override void OnKeyDown(System.Windows.Input.KeyEventArgs e)
    {
        if (e.Key == System.Windows.Input.Key.F11)
        {
            ToggleFullscreen_Click(null!, null!);
            e.Handled = true;
        }
        
        base.OnKeyDown(e);
    }
}
