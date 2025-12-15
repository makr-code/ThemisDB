// Copyright ThemisDB Team. Licensed under MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ThemisDBClient.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBuildingsReceived, const TArray<FOSMBuilding>&, Buildings);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTerrainReceived, const FTerrainData&, Terrain);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnQueryComplete, bool, bSuccess, const FString&, ResultJSON);

/**
 * Geographic location structure
 */
USTRUCT(BlueprintType)
struct THEMISDBPLUGIN_API FGeoLocation
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	double Latitude = 0.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	double Longitude = 0.0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	double Altitude = 0.0;

	FGeoLocation() = default;
	FGeoLocation(double InLat, double InLon, double InAlt = 0.0)
		: Latitude(InLat), Longitude(InLon), Altitude(InAlt) {}
};

/**
 * OSM Building structure
 */
USTRUCT(BlueprintType)
struct THEMISDBPLUGIN_API FOSMBuilding
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	FString ID;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	TArray<FGeoLocation> Footprint;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	float Height = 10.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	TMap<FString, FString> Tags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	FString BuildingType;
};

/**
 * Terrain data structure
 */
USTRUCT(BlueprintType)
struct THEMISDBPLUGIN_API FTerrainData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	TArray<float> HeightMap;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	int32 Width = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	int32 Height = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	float MinElevation = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GIS")
	float MaxElevation = 0.0f;
};

/**
 * ThemisDB Client for querying geospatial data
 */
UCLASS(BlueprintType)
class THEMISDBPLUGIN_API UThemisDBClient : public UObject
{
	GENERATED_BODY()
	
public:
	UThemisDBClient();

	/**
	 * Initialize connection to ThemisDB server
	 * @param ServerURL The base URL of the ThemisDB server (e.g., "http://localhost:8765")
	 * @param Port The port number
	 */
	UFUNCTION(BlueprintCallable, Category = "ThemisDB")
	void Initialize(const FString& ServerURL, int32 Port = 8765);
	
	/**
	 * Query buildings in a bounding box (async)
	 * @param SouthWest Southwest corner of bounding box
	 * @param NorthEast Northeast corner of bounding box
	 * @param OnComplete Callback when query completes
	 */
	UFUNCTION(BlueprintCallable, Category = "ThemisDB|Geo")
	void QueryBuildingsAsync(
		FGeoLocation SouthWest,
		FGeoLocation NorthEast,
		FOnBuildingsReceived OnComplete
	);
	
	/**
	 * Query terrain data in a circular area (async)
	 * @param Center Center point
	 * @param RadiusKm Radius in kilometers
	 * @param OnComplete Callback when query completes
	 */
	UFUNCTION(BlueprintCallable, Category = "ThemisDB|Geo")
	void QueryTerrainAsync(
		FGeoLocation Center,
		float RadiusKm,
		FOnTerrainReceived OnComplete
	);
	
	/**
	 * Execute an AQL query (async)
	 * @param AQLQuery The AQL query string
	 * @param OnComplete Callback when query completes
	 */
	UFUNCTION(BlueprintCallable, Category = "ThemisDB|Query")
	void ExecuteAQLAsync(
		const FString& AQLQuery,
		FOnQueryComplete OnComplete
	);

	/**
	 * Test connection to ThemisDB
	 */
	UFUNCTION(BlueprintCallable, Category = "ThemisDB")
	bool TestConnection();

private:
	FHttpModule* HttpModule;
	FString BaseURL;
	
	void HandleBuildingsResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bWasSuccessful,
		FOnBuildingsReceived Callback
	);

	void HandleTerrainResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bWasSuccessful,
		FOnTerrainReceived Callback
	);

	void HandleAQLResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bWasSuccessful,
		FOnQueryComplete Callback
	);
};

/**
 * Blueprint function library for ThemisDB utilities
 */
UCLASS()
class THEMISDBPLUGIN_API UThemisDBBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Convert geographic coordinates to Unreal world coordinates
	 */
	UFUNCTION(BlueprintPure, Category = "ThemisDB|Utilities")
	static FVector GeoToWorld(FGeoLocation GeoLocation, FGeoLocation WorldOrigin);

	/**
	 * Convert Unreal world coordinates to geographic coordinates
	 */
	UFUNCTION(BlueprintPure, Category = "ThemisDB|Utilities")
	static FGeoLocation WorldToGeo(FVector WorldLocation, FGeoLocation WorldOrigin);

	/**
	 * Calculate distance between two geographic points (in meters)
	 */
	UFUNCTION(BlueprintPure, Category = "ThemisDB|Utilities")
	static float GeoDistance(FGeoLocation A, FGeoLocation B);
};
