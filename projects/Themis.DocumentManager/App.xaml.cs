using FluentValidation;
using MediatR;
using Microsoft.Extensions.DependencyInjection;
using System;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Infrastructure.Persistence;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Views;

namespace Themis.DocumentManager;

/// <summary>
/// Interaction logic for App.xaml
/// </summary>
public partial class App : Application
{
    private ServiceProvider? _serviceProvider;
    private SplashScreen? _splashScreen;
    private DynamicAssemblyLoader? _assemblyLoader;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // Fehlerbehandlung für globale Exceptions
        AppDomain.CurrentDomain.UnhandledException += (sender, args) =>
        {
            var exception = args.ExceptionObject as Exception;
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
            services.AddSingleton<IDocumentService, DocumentService>();
            services.AddSingleton<ISearchService, SearchService>();
            services.AddSingleton<IMetadataService, MetadataService>();
            services.AddSingleton<IGeoService, GeoService>();
            services.AddSingleton<ITimelineService, TimelineService>();
            services.AddSingleton<IVectorService, VectorService>();
            services.AddSingleton<IGraphService, GraphService>();
            services.AddSingleton<IRevisionService, RevisionService>();
            services.AddSingleton<IOfficeIntegrationService, OfficeIntegrationService>();
            
            // Administrative Structure Services
            services.AddSingleton<IProcessTimelineService, ProcessTimelineService>();
            services.AddSingleton<IAdministrativeStructureService, AdministrativeStructureService>();
            
            // Phase 1 VIS Features Services
            services.AddSingleton<INotificationService, NotificationService>();
            services.AddSingleton<IInboxService, InboxService>();
            services.AddSingleton<IReminderService, ReminderService>();
            services.AddSingleton<ICosigningService, CosigningService>();
            services.AddSingleton<IProcessLogService, ProcessLogService>();
            services.AddSingleton<IFilingPlanService, FilingPlanService>();
            
            // Geo Services (OSM Layer Support)
            services.AddSingleton<IMapConfigurationService, MapConfigurationService>();
            services.AddSingleton<IGeoLayerService, GeoLayerService>();
            services.AddSingleton<IGeoFeatureService, GeoFeatureService>();
            services.AddSingleton<IGeocodingService, GeocodingService>();
            services.AddSingleton<IGeoDocumentService, GeoDocumentService>();
            
            // Metadata Badge Services (Smart Input with Semantic Similarity)
            services.AddSingleton<IBadgePatternService, BadgePatternService>();
            services.AddSingleton<IAbbreviationService, AbbreviationService>();
            services.AddSingleton<IMetadataBadgeService, MetadataBadgeService>();
            services.AddSingleton<ISmartSuggestionService, SmartSuggestionService>();
            services.AddSingleton<ISmartInputValidatorService, SmartInputValidatorService>();
            
            // Process Watch & Timeline Services
            services.AddSingleton<IProcessWatchService, ProcessWatchService>();
            services.AddSingleton<ITimelineAggregationService, TimelineAggregationService>();
            services.AddSingleton<IGanttService, GanttService>();
            
            // Outbox Service
            services.AddSingleton<IOutboxService, OutboxService>();
            
            // Phase 2 Services (Email, Scan, OCR, Search, Notifications, Forms)
            services.AddSingleton<IEmailIntegrationService, EmailIntegrationService>();
            services.AddSingleton<IScanService, ScanService>();
            services.AddSingleton<IOCRService, OCRService>();
            services.AddSingleton<IFullTextSearchService, FullTextSearchService>();
            services.AddSingleton<IEnhancedNotificationService, EnhancedNotificationService>();
            services.AddSingleton<IFormManagementService, EnhancedFormManagementService>();
            
            // Form Template System Services (NEW - Dynamic Metadata Masks with YAML/JSON Config)
            services.AddSingleton<IFormTemplateService, FormTemplateService>();
            services.AddSingleton<IFormConfigurationLoader, FormConfigurationLoader>();
            services.AddSingleton<IFormDatabaseMappingService, FormDatabaseMappingService>();
            services.AddSingleton<IFormTestDataService, FormTestDataService>();
            services.AddSingleton<IFormAuditService, FormAuditService>();
            services.AddSingleton<IFormSubmissionHistoryService, FormSubmissionHistoryService>();
            services.AddSingleton<IFormAnalyticsService, FormAnalyticsService>();
            services.AddSingleton<ISmartFormService, SmartFormService>();
            services.AddSingleton<IFormContextService, FormContextService>();
            
            // SmartForm Configuration & Customization Services (Phase 23 - Dynamic Configuration & LLM Labels)
            services.AddSingleton<ISmartFormConfigurationService, SmartFormConfigurationService>();
            services.AddSingleton<IFormFieldLabelingService, FormFieldLabelingService>();
            services.AddSingleton<IFormUICustomizationService, FormUICustomizationService>();
            
            // Graph & Geo Visualization Services (Phase 24 - 3D Graph & OSM Map Integration)
            services.AddSingleton<IGraphVisualizationService, GraphVisualizationService>();
            services.AddSingleton<IOsmMapRenderer, OsmMapRenderer>();
            
            // Phase 2 Collaboration Services (Sprint 5-6 - Check-in/Check-out, SignalR, Comments)
            services.AddSingleton<IDocumentLockingService, DocumentLockingService>();
            services.AddSingleton<ICommentService, CommentService>();
            services.AddSingleton<Infrastructure.SignalR.ISignalRService, Infrastructure.SignalR.SignalRService>();
            
            // Phase 2 Background Jobs (Sprint 5-6 - Lock Cleanup)
            services.AddSingleton<DocumentLockCleanupConfiguration>();
            services.AddSingleton<Infrastructure.BackgroundJobs.DocumentLockCleanupService>();
            
            // Phase 2 AI/ML Services (Sprint 7-8 - Classification & Metadata Extraction)
            services.AddSingleton<Infrastructure.MachineLearning.DocumentClassifier>();
            services.AddSingleton<Infrastructure.MachineLearning.MetadataExtractor>();
            services.AddSingleton<Services.Classification.IClassificationService, Services.Classification.ClassificationService>();
            
            // Email Threading Services
            services.AddSingleton<IEmailHeaderService, EmailHeaderService>();
            
            // Seamless Integration Services (Messenger, Calendar, Tasks)
            services.AddSingleton<IMessengerIntegrationService, MessengerIntegrationService>();
            services.AddSingleton<ICalendarIntegrationService, CalendarIntegrationService>();
            services.AddSingleton<IOutlookTaskService, OutlookTaskService>();
            services.AddSingleton<ISeamlessIntegrationOrchestrator, SeamlessIntegrationOrchestrator>();
            
            // Phase 3 Compliance & Integration Services
            services.AddSingleton<IFourEyesPrincipleService, FourEyesPrincipleService>();
            services.AddSingleton<IFileAccessLogService, FileAccessLogService>();
            services.AddSingleton<ISubstitutionService, SubstitutionService>();
            services.AddSingleton<IEGovService, EGovService>();
            services.AddSingleton<ITransferNoteService, TransferNoteService>();
            
            // AI Assistant Services (VSCode-Style with SSE & MCP)
            services.AddSingleton<IAIChatService, AIChatService>();
            services.AddSingleton<IMCPToolService, MCPToolService>();
            services.AddSingleton<ILLMProviderService, LLMProviderService>();
            
            // Help System Services
            services.AddSingleton<IHelpService, HelpService>();

            // ViewModels
            services.AddTransient<MainViewModel>();
            services.AddTransient<DocumentBrowserViewModel>();
            services.AddTransient<DocumentDetailViewModel>();
            services.AddTransient<SearchViewModel>();
            services.AddTransient<GeoViewModel>();
            services.AddTransient<TimelineViewModel>();
            services.AddTransient<GraphViewModel>();
            services.AddTransient<DocumentCollaborationViewModel>();

            // Views - WICHTIG: MainWindow am Ende registrieren
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
