// Copyright ThemisDB Team. Licensed under MIT License.

#include "ThemisGISViewer.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FThemisGISViewerModule, ThemisGISViewer, "ThemisGISViewer");

void FThemisGISViewerModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("ThemisGISViewer module starting up"));
}

void FThemisGISViewerModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("ThemisGISViewer module shutting down"));
}
