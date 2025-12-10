# Phase 1 - Sprint 1: Clean Architecture Foundation

**Start Date:** 2025-12-10  
**Duration:** 2 Wochen  
**Ziel:** Clean Architecture Layers etablieren + MediatR Integration

---

## Sprint 1 Tasks

### 1. NuGet Packages installieren ✅
- [x] MediatR (v12.2.0)
- [x] FluentValidation (v11.9.0)  
- [x] FluentValidation.DependencyInjectionExtensions (v11.9.0)

### 2. Domain Layer erstellen ✅
- [x] Domain/Events/ - DocumentCreatedEvent
- [x] Domain/Exceptions/ - DocumentNotFoundException

### 3. Application Layer erstellen ✅
- [x] Application/Common/Interfaces/ - IThemisRepository
- [x] Application/Documents/Commands/CreateDocument/ - Command, Handler, Validator
- [x] Application/Documents/Queries/GetDocument/ - Query, Handler
- [x] Application/Documents/Queries/GetDocuments/ - Query, Handler

### 4. Infrastructure Layer erstellen ✅
- [x] Infrastructure/Persistence/ThemisRepository - Repository Implementation

### 5. Erste Commands & Queries ✅
- [x] CreateDocumentCommand + Handler + Validator
- [x] GetDocumentQuery + Handler  
- [x] GetDocumentsQuery + Handler

### 6. DI Container konfigurieren ✅
- [x] MediatR registrieren in App.xaml.cs
- [x] FluentValidation registrieren in App.xaml.cs
- [x] IThemisRepository → ThemisRepository registrieren

---

## Success Metrics Sprint 1

- ✅ Clean Architecture Struktur etabliert (Domain, Application, Infrastructure)
- ✅ MediatR konfiguriert (CQRS Pattern)
- ✅ 3 Commands/Queries implementiert
- ✅ FluentValidation aktiv
- ✅ Repository Pattern für ThemisDB implementiert

---

## Dateien erstellt

### Domain Layer (2 Dateien)
- Domain/Events/DocumentCreatedEvent.cs
- Domain/Exceptions/DocumentNotFoundException.cs

### Application Layer (7 Dateien)
- Application/Common/Interfaces/IThemisRepository.cs
- Application/Documents/Commands/CreateDocument/CreateDocumentCommand.cs
- Application/Documents/Commands/CreateDocument/CreateDocumentCommandHandler.cs
- Application/Documents/Commands/CreateDocument/CreateDocumentCommandValidator.cs
- Application/Documents/Queries/GetDocument/GetDocumentQuery.cs
- Application/Documents/Queries/GetDocument/GetDocumentQueryHandler.cs
- Application/Documents/Queries/GetDocuments/GetDocumentsQuery.cs
- Application/Documents/Queries/GetDocuments/GetDocumentsQueryHandler.cs

### Infrastructure Layer (1 Datei)
- Infrastructure/Persistence/ThemisRepository.cs

### Configuration (1 Datei geändert)
- App.xaml.cs - MediatR + FluentValidation DI Setup
- Themis.DocumentManager.csproj - NuGet Packages

---

## Next: Sprint 2

- Migration bestehender Services zu Application Layer
- UpdateDocumentCommand + Handler + Validator
- DeleteDocumentCommand + Handler  
- Validation Behavior für MediatR Pipeline
- Unit Tests erstellen (Ziel: 30% Coverage)
- Logging mit Serilog integrieren
