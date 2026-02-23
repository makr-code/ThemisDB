/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphLegend.cs                                     ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     312                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace Themis.DocumentManager.Controls;

/// <summary>
/// Custom Legend Control für GraphView mit Node/Edge-Typen und Statistiken.
/// Phase 30 - Advanced Custom Controls.
/// </summary>
public class GraphLegend : Control
{
    #region Dependency Properties

    public static readonly DependencyProperty NodeCountProperty =
        DependencyProperty.Register(
            nameof(NodeCount),
            typeof(int),
            typeof(GraphLegend),
            new PropertyMetadata(0));

    public static readonly DependencyProperty EdgeCountProperty =
        DependencyProperty.Register(
            nameof(EdgeCount),
            typeof(int),
            typeof(GraphLegend),
            new PropertyMetadata(0));

    public static readonly DependencyProperty SelectedNodeTypeProperty =
        DependencyProperty.Register(
            nameof(SelectedNodeType),
            typeof(string),
            typeof(GraphLegend),
            new PropertyMetadata(null));

    public static readonly DependencyProperty NodeTypesProperty =
        DependencyProperty.Register(
            nameof(NodeTypes),
            typeof(ObservableCollection<NodeTypeInfo>),
            typeof(GraphLegend),
            new PropertyMetadata(null));

    public static readonly DependencyProperty IsCollapsedProperty =
        DependencyProperty.Register(
            nameof(IsCollapsed),
            typeof(bool),
            typeof(GraphLegend),
            new PropertyMetadata(false));

    #endregion

    #region Properties

    public int NodeCount
    {
        get => (int)GetValue(NodeCountProperty);
        set => SetValue(NodeCountProperty, value);
    }

    public int EdgeCount
    {
        get => (int)GetValue(EdgeCountProperty);
        set => SetValue(EdgeCountProperty, value);
    }

    public string? SelectedNodeType
    {
        get => (string?)GetValue(SelectedNodeTypeProperty);
        set => SetValue(SelectedNodeTypeProperty, value);
    }

    public ObservableCollection<NodeTypeInfo> NodeTypes
    {
        get => (ObservableCollection<NodeTypeInfo>)GetValue(NodeTypesProperty);
        set => SetValue(NodeTypesProperty, value);
    }

    public bool IsCollapsed
    {
        get => (bool)GetValue(IsCollapsedProperty);
        set => SetValue(IsCollapsedProperty, value);
    }

    #endregion

    #region Commands

    public ICommand FilterByTypeCommand { get; }
    public ICommand ToggleCollapseCommand { get; }
    public ICommand ClearFilterCommand { get; }

    #endregion

    #region Events

    public event EventHandler<string>? NodeTypeFilterChanged;
    public event EventHandler? FilterCleared;

    #endregion

    static GraphLegend()
    {
        DefaultStyleKeyProperty.OverrideMetadata(
            typeof(GraphLegend),
            new FrameworkPropertyMetadata(typeof(GraphLegend)));
    }

    public GraphLegend()
    {
        NodeTypes = new ObservableCollection<NodeTypeInfo>();
        
        FilterByTypeCommand = new RelayCommand<string>(FilterByType);
        ToggleCollapseCommand = new RelayCommand(ToggleCollapse);
        ClearFilterCommand = new RelayCommand(ClearFilter);

        InitializeDefaultNodeTypes();
    }

    private void InitializeDefaultNodeTypes()
    {
        NodeTypes.Add(new NodeTypeInfo("Document", Colors.DeepSkyBlue, 0));
        NodeTypes.Add(new NodeTypeInfo("Person", Colors.LimeGreen, 0));
        NodeTypes.Add(new NodeTypeInfo("Organization", Colors.Orange, 0));
        NodeTypes.Add(new NodeTypeInfo("Location", Colors.Crimson, 0));
        NodeTypes.Add(new NodeTypeInfo("Concept", Colors.Purple, 0));
    }

    public override void OnApplyTemplate()
    {
        base.OnApplyTemplate();

        // Hook up template parts
        if (GetTemplateChild("PART_NodeTypesList") is ListBox listBox)
        {
            listBox.SelectionChanged += NodeTypesList_SelectionChanged;
        }

        if (GetTemplateChild("PART_ToggleButton") is Button toggleButton)
        {
            toggleButton.Click += (s, e) => ToggleCollapse();
        }

        if (GetTemplateChild("PART_ClearFilterButton") is Button clearButton)
        {
            clearButton.Click += (s, e) => ClearFilter();
        }
    }

    #region Command Handlers

    private void FilterByType(string? nodeType)
    {
        if (string.IsNullOrEmpty(nodeType)) return;

        SelectedNodeType = nodeType;
        NodeTypeFilterChanged?.Invoke(this, nodeType);
    }

    private void ToggleCollapse()
    {
        IsCollapsed = !IsCollapsed;
    }

    private void ClearFilter()
    {
        SelectedNodeType = null;
        FilterCleared?.Invoke(this, EventArgs.Empty);
    }

    #endregion

    #region Event Handlers

    private void NodeTypesList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (e.AddedItems.Count > 0 && e.AddedItems[0] is NodeTypeInfo nodeTypeInfo)
        {
            FilterByType(nodeTypeInfo.TypeName);
        }
    }

    #endregion

    #region Helper Methods

    public void UpdateNodeTypeCount(string typeName, int count)
    {
        var nodeType = NodeTypes.FirstOrDefault(nt => nt.TypeName == typeName);
        if (nodeType != null)
        {
            nodeType.Count = count;
        }
    }

    public void ResetCounts()
    {
        foreach (var nodeType in NodeTypes)
        {
            nodeType.Count = 0;
        }
        NodeCount = 0;
        EdgeCount = 0;
    }

    #endregion

    #region RelayCommand Implementation

    private class RelayCommand : ICommand
    {
        private readonly Action _execute;
        private readonly Func<bool>? _canExecute;

        public event EventHandler? CanExecuteChanged
        {
            add => CommandManager.RequerySuggested += value;
            remove => CommandManager.RequerySuggested -= value;
        }

        public RelayCommand(Action execute, Func<bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

        public void Execute(object? parameter) => _execute();
    }

    private class RelayCommand<T> : ICommand
    {
        private readonly Action<T?> _execute;
        private readonly Func<T?, bool>? _canExecute;

        public event EventHandler? CanExecuteChanged
        {
            add => CommandManager.RequerySuggested += value;
            remove => CommandManager.RequerySuggested -= value;
        }

        public RelayCommand(Action<T?> execute, Func<T?, bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => 
            _canExecute?.Invoke((T?)parameter) ?? true;

        public void Execute(object? parameter) => _execute((T?)parameter);
    }

    #endregion
}

/// <summary>
/// Info-Klasse für Node-Typen
/// </summary>
public class NodeTypeInfo : System.ComponentModel.INotifyPropertyChanged
{
    private int _count;

    public string TypeName { get; set; }
    public Color Color { get; set; }
    
    public int Count
    {
        get => _count;
        set
        {
            if (_count != value)
            {
                _count = value;
                PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(nameof(Count)));
            }
        }
    }

    public Brush ColorBrush => new SolidColorBrush(Color);

    public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;

    public NodeTypeInfo(string typeName, Color color, int count = 0)
    {
        TypeName = typeName;
        Color = color;
        Count = count;
    }
}
