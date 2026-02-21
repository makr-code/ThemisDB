/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            App.xaml.cs                                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🔴 ALPHA                                        ║
    • Quality Score:   34.0/100                                       ║
    • Total Lines:     478                                            ║
    • Open Issues:     TODOs: 0, Stubs: 12                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 Early Development                                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;
using MediatR;
using Microsoft.Extensions.DependencyInjection;
using System;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Dashboard.ViewModels;
using Themis.DocumentManager.Features.Dashboard.Views;
using Themis.DocumentManager.Features.DocumentBrowser.Views;
using Themis.DocumentManager.Infrastructure.Persistence;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Features.Dashboard.Services;
using Themis.DocumentManager.Features.Geo.Services;
using Themis.DocumentManager.Features.Geo.ViewModels;
using Themis.DocumentManager.Features.Graph.Services;
using Themis.DocumentManager.Features.Graph.ViewModels;
using Themis.DocumentManager.Features.Gantt.Services;
using Themis.DocumentManager.Features.Gantt.ViewModels;
using Themis.DocumentManager.Features.Favorites.ViewModels;
using Themis.DocumentManager.Features.DocumentBrowser.ViewModels;
using Themis.DocumentManager.Features.MetadataForm.Services;
using Themis.DocumentManager.Features.MetadataForm.ViewModels;
using Themis.DocumentManager.Features.MetadataForm.Views;
using Themis.DocumentManager.Features.TaskBasket.ViewModels;
using Themis.DocumentManager.Features.TaskBasket.Views;
using Themis.DocumentManager.Features.AIChat.Services;
using Themis.DocumentManager.Features.AIChat.ViewModels;
using Themis.DocumentManager.Features.AIChat.Views;
using Themis.DocumentManager.Features.ERDQueryEditor.Services;
using Themis.DocumentManager.Features.ERDQueryEditor.ViewModels;
using Themis.DocumentManager.Features.ERDQueryEditor.Views;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.ViewModels.Navigation;
using Themis.DocumentManager.Views;
using Themis.DocumentManager.Features.Timeline.Views;
using Themis.DocumentManager.Features.Gantt.Views;

namespace Themis.DocumentManager;

/// <summary>
/// Interaction logic for App.xaml
/// </summary>
public partial class App : System.Windows.Application
{
    private ServiceProvider? _serviceProvider;
    private SplashScreen? _splashScreen;
    private DynamicAssemblyLoader? _assemblyLoader;

    /// <summary>
    /// Public access to ServiceProvider for DI lookups
    /// </summary>
    public IServiceProvider? ServiceProvider => _serviceProvider;

    /// <summary>
    /// Get service from DI container
    /// </summary>
    public static T? GetService<T>() where T : class
    {
        var app = Current as App;
        return app?.ServiceProvider?.GetService(typeof(T)) as T;
    }

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // Fehlerbehandlung für globale Exceptions
        AppDomain.CurrentDomain.UnhandledException += (sender, args) =>
        {
            var exception = args.ExceptionObject as Exception;
            var msg = $"UNHANDLED EXCEPTION:\n{exception}";
            File.WriteAllText("crash.log", msg);
            System.Diagnostics.Debug.WriteLine(msg);
            MessageBox.Show($"Unerwarteter Fehler: {exception?.Message}\n\n{exception?.StackTrace}", 
                          "Kritischer Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
            Shutdown(1);
        };

        // SplashScreen anzeigen
        _splashScreen = new SplashScreen();
        _splashScreen.Show();

        // Asynchrone Initialisierung mit besserer Fehlerbehandlung
        Task.Run(async () =>
        {
            try
            {
                await InitializeApplicationAsync();
                
                // Zum UI-Thread zurückkehren
                Dispatcher.Invoke(() =>
                {
                    try
                    {
                        if (_serviceProvider == null)
                        {
                            throw new InvalidOperationException("ServiceProvider wurde nicht initialisiert");
                        }

                        var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
                        mainWindow.Show();
                        
                        _splashScreen?.Close();
                    }
                    catch (Exception ex)
                    {
                        var msg = $"MAINWINDOW SHOW ERROR:\n{ex}";
                        File.WriteAllText("mainwindow-error.log", msg);
                        System.Diagnostics.Debug.WriteLine(msg);
                        MessageBox.Show($"Fehler beim Anzeigen des Hauptfensters: {ex.Message}\n\n{ex.StackTrace}", 
                                      "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
                        Shutdown(1);
                    }
                });
            }
            catch (Exception ex)
            {
                var errorMsg = $"Fehler beim Starten der Anwendung:\n{ex.Message}";
                if (ex.InnerException != null)
                    errorMsg += $"\n\nInner Exception: {ex.InnerException.Message}";
                File.WriteAllText("startup-error.log", ex.ToString());
                System.Diagnostics.Debug.WriteLine($"STARTUP ERROR:\n{ex}");
                
                Dispatcher.Invoke(() =>
                {
                    try
                    {
                        _splashScreen?.Close();
                    }
                    catch { }
                    
                    MessageBox.Show(errorMsg, "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
                    Shutdown(1);
                });
            }
        });
    }

    private async Task InitializeApplicationAsync()
    {
        try
        {
            // Schritt 1: Assembly Loader initialisieren
            _splashScreen?.UpdateStatus("Lade Module...", 10);
            await Task.Delay(200);
            _assemblyLoader = new DynamicAssemblyLoader();
            
            // Schritt 2: Dynamisches Laden von Assemblies
            _splashScreen?.UpdateStatus("Lade Themis-Module...", 20);
            await Task.Delay(200);
            _assemblyLoader.LoadAssembliesByPrefix("Themis");
            
            _splashScreen?.UpdateStatus("Lade Microsoft-Module...", 30);
            await Task.Delay(200);
            _assemblyLoader.LoadAssembliesByPrefix("Microsoft");
            
            _splashScreen?.UpdateStatus("Lade Zusatz-Module...", 40);
            await Task.Delay(200);
            _assemblyLoader.LoadAssembliesFromDirectory();
            
            // Schritt 3: Services konfigurieren
            _splashScreen?.UpdateStatus("Konfiguriere Services...", 60);
            await Task.Delay(200);
            var serviceCollection = new ServiceCollection();
            ConfigureServices(serviceCollection);
            
            // Schritt 4: ServiceProvider erstellen
            _splashScreen?.UpdateStatus("Erstelle Service Provider...", 80);
            await Task.Delay(200);
            _serviceProvider = serviceCollection.BuildServiceProvider();

            // DSM: initialen Pull aus ThemisDB durchführen (falls verfügbar), damit Graph/Geo/Timeline Daten haben.
            try
            {
                var dsmStore = _serviceProvider.GetService(typeof(DsmLocalDataStore)) as DsmLocalDataStore;
                if (dsmStore != null)
                {
                    _splashScreen?.UpdateStatus("Lade DSM-Daten...", 85);
                    await dsmStore.SyncFromRemoteAsync();
                }
            }
            catch (Exception syncEx)
            {
                System.Diagnostics.Debug.WriteLine($"DSM Sync übersprungen: {syncEx.Message}");
            }
            
            // Schritt 5: Finale Initialisierung
            _splashScreen?.UpdateStatus("Finalisiere...", 95);
            await Task.Delay(300);
            
            // Schritt 6: Background Services starten
            StartBackgroundServices();
            
            _splashScreen?.UpdateStatus("Bereit!", 100);
            await Task.Delay(200);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Fehler bei InitializeApplicationAsync: {ex.Message}");
            System.Diagnostics.Debug.WriteLine($"Stack Trace: {ex.StackTrace}");
            throw;
        }
    }

    private void StartBackgroundServices()
    {
        try
        {
            // Document Lock Cleanup Service starten
            var cleanupService = _serviceProvider?.GetService(typeof(Infrastructure.BackgroundJobs.DocumentLockCleanupService)) 
                as Infrastructure.BackgroundJobs.DocumentLockCleanupService;
            
            cleanupService?.Start();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Fehler beim Starten der Background Services: {ex.Message}");
            // Nicht-kritischer Fehler, App kann trotzdem starten
        }
    }

    private void ConfigureServices(IServiceCollection services)
    {
        try
        {
            // Clean Architecture Layers
            // MediatR for CQRS
            services.AddMediatR(cfg => cfg.RegisterServicesFromAssembly(Assembly.GetExecutingAssembly()));
            
            // FluentValidation
            services.AddValidatorsFromAssembly(Assembly.GetExecutingAssembly());
            
            // Infrastructure Layer - Repository Pattern
            services.AddSingleton<IThemisRepository, ThemisRepository>();
            
            // Core Services - nur die notwendigsten
            services.AddSingleton<IThemisApiClient, ThemisApiClient>();
            services.AddSingleton<IThemisDBService, ThemisDBService>();
            services.AddSingleton<DsmLocalDataStore>(sp => new DsmLocalDataStore(sp.GetService<IThemisDBService>()));
            services.AddSingleton<IDsmMetadataStore>(sp => sp.GetRequiredService<DsmLocalDataStore>());
            services.AddSingleton<IDsmGraphStore>(sp => sp.GetRequiredService<DsmLocalDataStore>());
            services.AddSingleton<IDsmVectorStore>(sp => sp.GetRequiredService<DsmLocalDataStore>());
            services.AddSingleton<IDsmGeoStore>(sp => sp.GetRequiredService<DsmLocalDataStore>());
            services.AddSingleton<IDsmTimelineStore>(sp => sp.GetRequiredService<DsmLocalDataStore>());
            services.AddSingleton<IDsmProcessLinkStore>(sp => sp.GetRequiredService<DsmLocalDataStore>());
            services.AddSingleton<IAuthenticationService, AuthenticationService>();
            services.AddSingleton<IOllamaService, OllamaService>();
            services.AddSingleton<IDocumentService, DocumentService>();
            services.AddSingleton<ISearchService, SearchService>();
            services.AddSingleton<IMetadataService, MetadataService>();
            services.AddSingleton<IGeoService, GeoService>();
            services.AddSingleton<IMapConfigurationService, MapConfigurationService>();
            services.AddSingleton<IGeoLayerService, GeoLayerService>();
            services.AddSingleton<IGeoFeatureService, GeoFeatureService>();
            services.AddSingleton<IGeocodingService, GeocodingService>();
            services.AddSingleton<IGeoDocumentService, GeoDocumentService>();
            services.AddSingleton<ITimelineService, TimelineService>();
            services.AddSingleton<IVectorService, VectorService>();
            services.AddSingleton<IGraphService, GraphService>();
            services.AddSingleton<IDashboardService, DashboardService>();
            
            // Schema and Query Services (ERM/ERD & Query Editor)
            services.AddSingleton<ISchemaService, SchemaService>();
            services.AddSingleton<IQueryService, QueryService>();

            // DSM-backed Timeline Aggregation + Notifications (No-Op)
            services.AddSingleton<INotificationService, NoOpNotificationService>();
            services.AddSingleton<ITimelineAggregationService>(sp =>
            {
                var dsm = sp.GetRequiredService<DsmLocalDataStore>();
                return new DsmTimelineAggregationService(dsm);
            });
            services.AddSingleton<IRevisionService, RevisionService>();
            services.AddSingleton<IOfficeIntegrationService, OfficeIntegrationService>();
            
            // Status Monitoring Service (ThemisDB + Ollama)
            services.AddSingleton<StatusMonitorService>();
            
            // Administrative Structure Services
            // DISABLED: Uses AQL which ThemisDB 0.1.0 doesn't support
            // services.AddSingleton<IProcessTimelineService, ProcessTimelineService>();
            // services.AddSingleton<IAdministrativeStructureService, AdministrativeStructureService>();
            
            // Phase 1 VIS Features Services
            // DISABLED: Stubs only, depends on complex entity model
            // services.AddSingleton<INotificationService, NotificationService>();
            // services.AddSingleton<IInboxService, InboxService>();
            // services.AddSingleton<IReminderService, ReminderService>();
            // services.AddSingleton<ICosigningService, CosigningService>();
            // services.AddSingleton<IProcessLogService, ProcessLogService>();
            // services.AddSingleton<IFilingPlanService, FilingPlanService>();
            
            // Geo Services (OSM Layer Support)
            // DISABLED: Stubs only, not essential for basic DSM
            // services.AddSingleton<IMapConfigurationService, MapConfigurationService>();
            // services.AddSingleton<IGeoLayerService, GeoLayerService>();
            // services.AddSingleton<IGeoFeatureService, GeoFeatureService>();
            // services.AddSingleton<IGeocodingService, GeocodingService>();
            // services.AddSingleton<IGeoDocumentService, GeoDocumentService>();
            
            // Metadata Badge Services (Smart Input with Semantic Similarity)
            // DISABLED: Stubs only
            // services.AddSingleton<IBadgePatternService, BadgePatternService>();
            // services.AddSingleton<IAbbreviationService, AbbreviationService>();
            // services.AddSingleton<IMetadataBadgeService, MetadataBadgeService>();
            // services.AddSingleton<ISmartSuggestionService, SmartSuggestionService>();
            // services.AddSingleton<ISmartInputValidatorService, SmartInputValidatorService>();
            
            // Process Watch & Timeline Services
            // DISABLED: Stubs only
            // services.AddSingleton<IProcessWatchService, ProcessWatchService>();
            // services.AddSingleton<ITimelineAggregationService, TimelineAggregationService>();
            services.AddSingleton<IGanttService, GanttService>();
            
            // Outbox Service
            // DISABLED: Stubs only
            // services.AddSingleton<IOutboxService, OutboxService>();
            
            // Phase 2 Services (Email, Scan, OCR, Search, Notifications, Forms)
            // DISABLED: Stubs only, wait for full implementation
            // services.AddSingleton<IEmailIntegrationService, EmailIntegrationService>();
            // services.AddSingleton<IScanService, ScanService>();
            // services.AddSingleton<IOCRService, OCRService>();
            // services.AddSingleton<IFullTextSearchService, FullTextSearchService>();
            // services.AddSingleton<IEnhancedNotificationService, EnhancedNotificationService>();
            // services.AddSingleton<IFormManagementService, EnhancedFormManagementService>();
            
            // Form Template System Services (NEW - Dynamic Metadata Masks with YAML/JSON Config)
            services.AddSingleton<IFormTemplateService, FormTemplateService>();
            // services.AddSingleton<IFormConfigurationLoader, FormConfigurationLoader>();
            services.AddSingleton<IFormDatabaseMappingService, FormDatabaseMappingService>();
            // services.AddSingleton<IFormTestDataService, FormTestDataService>();
            // services.AddSingleton<IFormAuditService, FormAuditService>();
            // services.AddSingleton<IFormSubmissionHistoryService, FormSubmissionHistoryService>();
            // services.AddSingleton<IFormAnalyticsService, FormAnalyticsService>();
            // services.AddSingleton<ISmartFormService, SmartFormService>();
            // services.AddSingleton<IFormContextService, FormContextService>();
            
            // SmartForm Configuration & Customization Services (Phase 23 - Dynamic Configuration & LLM Labels)
            // DISABLED: Stubs only
            // services.AddSingleton<ISmartFormConfigurationService, SmartFormConfigurationService>();
            // services.AddSingleton<IFormFieldLabelingService, FormFieldLabelingService>();
            // services.AddSingleton<IFormUICustomizationService, FormUICustomizationService>();
            
            // Graph & Geo Visualization Services (Phase 24 - 3D Graph & OSM Map Integration)
            services.AddSingleton<IGraphVisualizationService, GraphVisualizationService>();
            services.AddSingleton<IOsmMapRenderer, OsmMapRenderer>();
            
            // DirectX 3D Rendering Services (Phase 27 - Native 3D Graph Rendering)
            services.AddDirectX3DServices();
            
            // Phase 27 UI Styling & Theme System
            services.AddSingleton<ISettingsService, SettingsService>();
            services.AddSingleton<IAnimationService, AnimationService>();
            services.AddSingleton<IThemeService>(sp => 
            {
                var settingsService = sp.GetRequiredService<ISettingsService>();
                var themeService = new ThemeService(settingsService);
                themeService.Initialize();
                return themeService;
            });
            
            // Phase 30 Keyboard Navigation & Accessibility
            services.AddSingleton<IKeyboardNavigationService, KeyboardNavigationService>();
            
            // Phase 2 Collaboration Services (Sprint 5-6 - Check-in/Check-out, SignalR, Comments)
            // DISABLED: Stubs only
            // services.AddSingleton<ICommentService, CommentService>();
            // services.AddSingleton<Infrastructure.SignalR.ISignalRService, Infrastructure.SignalR.SignalRService>();
            
            // Phase 2 Background Jobs (Sprint 5-6 - Lock Cleanup)
            // DISABLED: Not needed for minimal DSM
            // services.AddSingleton<Infrastructure.BackgroundJobs.DocumentLockCleanupService>();
            
            // Phase 2 AI/ML Services (Sprint 7-8 - Classification & Metadata Extraction)
            // DISABLED: Stubs only
            // services.AddSingleton<Infrastructure.MachineLearning.DocumentClassifier>();
            // services.AddSingleton<Infrastructure.MachineLearning.MetadataExtractor>();
            
            // Email Threading Services
            // DISABLED: Stubs only
            // services.AddSingleton<IEmailHeaderService, EmailHeaderService>();
            
            // Seamless Integration Services (Messenger, Calendar, Tasks)
            // DISABLED: Stubs only
            // services.AddSingleton<IMessengerIntegrationService, MessengerIntegrationService>();
            // services.AddSingleton<ICalendarIntegrationService, CalendarIntegrationService>();
            // services.AddSingleton<IOutlookTaskService, OutlookTaskService>();
            // services.AddSingleton<ISeamlessIntegrationOrchestrator, SeamlessIntegrationOrchestrator>();
            
            // Phase 3 Compliance & Integration Services
            // DISABLED: Stubs only or depend on AQL
            // services.AddSingleton<IFourEyesPrincipleService, FourEyesPrincipleService>();
            // services.AddSingleton<IFileAccessLogService, FileAccessLogService>();
            // services.AddSingleton<ISubstitutionService, SubstitutionService>();
            // services.AddSingleton<IEGovService, EGovService>();
            // services.AddSingleton<ITransferNoteService, TransferNoteService>();
            
            // NEW: Rollenbasierte Berechtigungen, Kontextmenüs, Prozess-Verknüpfungen und Audit-Logging
            services.AddSingleton<IRoleBasedPermissionService, RoleBasedPermissionService>();
            services.AddSingleton<IContextMenuService, ContextMenuService>();
            services.AddSingleton<IProcessLinkingService, ProcessLinkingService>();
            services.AddSingleton<IAuditLoggingService, AuditLoggingService>();
            services.AddSingleton<IMetadataFormGeneratorService, MetadataFormGeneratorService>();
            services.AddSingleton<IMetadataLayoutService, MetadataLayoutService>();
            services.AddSingleton<IMetadataBindingService, MetadataBindingService>();
            services.AddSingleton<ISmartMetadataLayoutEngine, SmartMetadataLayoutEngine>();
            services.AddSingleton<IDialogService, DialogService>();
            
            // AI Assistant Services (VSCode-Style with SSE & MCP)
            services.AddSingleton<IAIChatService, AIChatService>();
            services.AddSingleton<IMCPToolService, MCPToolService>();
            services.AddSingleton<ILLMProviderService, LLMProviderService>();
            
            // Help System Services
            services.AddSingleton<IHelpService, HelpService>();

            // ViewModels
            services.AddTransient<MainViewModel>();
            services.AddTransient<BreadcrumbViewModel>();
            services.AddTransient<UserSwitcherViewModel>();
            services.AddTransient<DocumentBrowserViewModel>();
            services.AddTransient<DocumentDetailViewModel>();
            services.AddTransient<SearchViewModel>();
            services.AddTransient<GeoViewModel>();
            services.AddTransient<TimelineViewModel>();
            services.AddTransient<GraphViewModel>();
            services.AddTransient<DocumentCollaborationViewModel>();
            services.AddTransient<MetadataFormViewModel>();
            services.AddTransient<ProcessLinkingDialogViewModel>();
            services.AddTransient<TaskBasketViewModel>();
            services.AddTransient<TasksRightSidebarViewModel>();
            services.AddTransient<TestDataGeneratorViewModel>();
            services.AddTransient<AIChatViewModel>(sp =>
            {
                var auth = sp.GetRequiredService<IAuthenticationService>();
                var userId = auth.CurrentUserId ?? "urn:themis:user:local-admin";
                return new AIChatViewModel(
                    sp.GetRequiredService<IAIChatService>(),
                    sp.GetRequiredService<IMCPToolService>(),
                    userId);
            });
            services.AddTransient<DashboardViewModel>();
            services.AddTransient<InboxViewModel>();
            services.AddTransient<GanttViewModel>();
            services.AddTransient<DocumentPreviewViewModel>();
            services.AddTransient<FavoritesViewModel>();
            services.AddTransient<Themis.DocumentManager.ViewModels.Navigation.IntelligentBreadcrumbViewModel>();
            services.AddTransient<AuditLogViewerViewModel>();
            services.AddTransient<ERDViewModel>();
            services.AddTransient<QueryEditorViewModel>();

            // Views - WICHTIG: MainWindow am Ende registrieren
            services.AddTransient<DashboardPreviewView>();
            services.AddTransient<DocumentBrowserSimpleView>();
            services.AddTransient<TimelineSimpleView>();
            services.AddTransient<TimelineViewImproved>();
            services.AddTransient<GanttView>();
            services.AddTransient<FullDashboardSimpleView>();
            services.AddSingleton<MainWindow>();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Fehler in ConfigureServices: {ex.Message}");
            System.Diagnostics.Debug.WriteLine($"Stack Trace: {ex.StackTrace}");
            throw;
        }
    }

    protected override void OnExit(ExitEventArgs e)
    {
        _serviceProvider?.Dispose();
        base.OnExit(e);
    }
}
