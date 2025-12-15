// Copyright ThemisDB Team. Licensed under MIT License.

#include "ThemisDBClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

UThemisDBClient::UThemisDBClient()
{
	HttpModule = &FHttpModule::Get();
}

void UThemisDBClient::Initialize(const FString& ServerURL, int32 Port)
{
	BaseURL = FString::Printf(TEXT("%s:%d"), *ServerURL, Port);
	UE_LOG(LogTemp, Log, TEXT("ThemisDBClient initialized with URL: %s"), *BaseURL);
}

bool UThemisDBClient::TestConnection()
{
	// TODO: Implement synchronous health check
	return false;
}

void UThemisDBClient::QueryBuildingsAsync(
	FGeoLocation SouthWest,
	FGeoLocation NorthEast,
	FOnBuildingsReceived OnComplete)
{
	// Create AQL query
	FString AQLQuery = FString::Printf(TEXT(R"(
		FOR building IN osm_buildings
			FILTER building.location.latitude >= %f
				AND building.location.latitude <= %f
				AND building.location.longitude >= %f
				AND building.location.longitude <= %f
			LIMIT 1000
			RETURN {
				id: building._key,
				footprint: building.footprint,
				height: building.height,
				type: building.tags.building
			}
	)"),
	SouthWest.Latitude,
	NorthEast.Latitude,
	SouthWest.Longitude,
	NorthEast.Longitude);

	// Create HTTP request
	TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
	Request->SetURL(BaseURL + TEXT("/query/aql"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// Create JSON body
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("query"), AQLQuery);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);

	// Bind callback
	Request->OnProcessRequestComplete().BindUObject(this, &UThemisDBClient::HandleBuildingsResponse, OnComplete);

	// Send request
	Request->ProcessRequest();

	UE_LOG(LogTemp, Log, TEXT("ThemisDBClient: Querying buildings in bounding box"));
}

void UThemisDBClient::QueryTerrainAsync(
	FGeoLocation Center,
	float RadiusKm,
	FOnTerrainReceived OnComplete)
{
	// TODO: Implement terrain query
	UE_LOG(LogTemp, Warning, TEXT("ThemisDBClient: QueryTerrainAsync not yet implemented"));
}

void UThemisDBClient::ExecuteAQLAsync(
	const FString& AQLQuery,
	FOnQueryComplete OnComplete)
{
	TSharedRef<IHttpRequest> Request = HttpModule->CreateRequest();
	Request->SetURL(BaseURL + TEXT("/query/aql"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("query"), AQLQuery);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonString);
	Request->OnProcessRequestComplete().BindUObject(this, &UThemisDBClient::HandleAQLResponse, OnComplete);
	Request->ProcessRequest();

	UE_LOG(LogTemp, Log, TEXT("ThemisDBClient: Executing AQL query"));
}

void UThemisDBClient::HandleBuildingsResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful,
	FOnBuildingsReceived Callback)
{
	TArray<FOSMBuilding> Buildings;

	if (bWasSuccessful && Response.IsValid())
	{
		FString ResponseString = Response->GetContentAsString();
		UE_LOG(LogTemp, Log, TEXT("ThemisDBClient: Received response: %s"), *ResponseString);

		// Parse JSON response
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* ResultsArray;
			if (JsonObject->TryGetArrayField(TEXT("results"), ResultsArray))
			{
				for (const TSharedPtr<FJsonValue>& Value : *ResultsArray)
				{
					const TSharedPtr<FJsonObject>& BuildingObj = Value->AsObject();
					FOSMBuilding Building;

					Building.ID = BuildingObj->GetStringField(TEXT("id"));
					Building.Height = BuildingObj->GetNumberField(TEXT("height"));
					Building.BuildingType = BuildingObj->GetStringField(TEXT("type"));

					// Parse footprint array
					const TArray<TSharedPtr<FJsonValue>>* FootprintArray;
					if (BuildingObj->TryGetArrayField(TEXT("footprint"), FootprintArray))
					{
						for (const TSharedPtr<FJsonValue>& FootprintValue : *FootprintArray)
						{
							const TSharedPtr<FJsonObject>& PointObj = FootprintValue->AsObject();
							FGeoLocation Point;
							Point.Latitude = PointObj->GetNumberField(TEXT("latitude"));
							Point.Longitude = PointObj->GetNumberField(TEXT("longitude"));
							Building.Footprint.Add(Point);
						}
					}

					Buildings.Add(Building);
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("ThemisDBClient: Parsed %d buildings"), Buildings.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ThemisDBClient: Request failed"));
	}

	// Execute callback
	Callback.ExecuteIfBound(Buildings);
}

void UThemisDBClient::HandleTerrainResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful,
	FOnTerrainReceived Callback)
{
	FTerrainData Terrain;

	if (bWasSuccessful && Response.IsValid())
	{
		// TODO: Parse terrain data
		UE_LOG(LogTemp, Log, TEXT("ThemisDBClient: Terrain data received"));
	}

	Callback.ExecuteIfBound(Terrain);
}

void UThemisDBClient::HandleAQLResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful,
	FOnQueryComplete Callback)
{
	if (bWasSuccessful && Response.IsValid())
	{
		FString ResponseString = Response->GetContentAsString();
		Callback.ExecuteIfBound(true, ResponseString);
	}
	else
	{
		Callback.ExecuteIfBound(false, TEXT("Request failed"));
	}
}

// Blueprint Library Implementation
FVector UThemisDBBlueprintLibrary::GeoToWorld(FGeoLocation GeoLocation, FGeoLocation WorldOrigin)
{
	// Simple Mercator projection (for small areas)
	// For production, use proper coordinate transformation
	const double EarthRadius = 6371000.0; // meters
	
	double LatDiff = (GeoLocation.Latitude - WorldOrigin.Latitude) * PI / 180.0;
	double LonDiff = (GeoLocation.Longitude - WorldOrigin.Longitude) * PI / 180.0;
	
	double X = LonDiff * EarthRadius * FMath::Cos(WorldOrigin.Latitude * PI / 180.0);
	double Y = LatDiff * EarthRadius;
	double Z = GeoLocation.Altitude - WorldOrigin.Altitude;
	
	// Convert to Unreal coordinates (100 units = 1 meter)
	return FVector(X * 100.0, Y * 100.0, Z * 100.0);
}

FGeoLocation UThemisDBBlueprintLibrary::WorldToGeo(FVector WorldLocation, FGeoLocation WorldOrigin)
{
	const double EarthRadius = 6371000.0;
	
	// Convert from Unreal units to meters
	double X = WorldLocation.X / 100.0;
	double Y = WorldLocation.Y / 100.0;
	double Z = WorldLocation.Z / 100.0;
	
	double LatDiff = Y / EarthRadius;
	double LonDiff = X / (EarthRadius * FMath::Cos(WorldOrigin.Latitude * PI / 180.0));
	
	FGeoLocation Result;
	Result.Latitude = WorldOrigin.Latitude + (LatDiff * 180.0 / PI);
	Result.Longitude = WorldOrigin.Longitude + (LonDiff * 180.0 / PI);
	Result.Altitude = WorldOrigin.Altitude + Z;
	
	return Result;
}

float UThemisDBBlueprintLibrary::GeoDistance(FGeoLocation A, FGeoLocation B)
{
	// Haversine formula
	const double EarthRadius = 6371000.0;
	
	double Lat1 = A.Latitude * PI / 180.0;
	double Lat2 = B.Latitude * PI / 180.0;
	double DLat = (B.Latitude - A.Latitude) * PI / 180.0;
	double DLon = (B.Longitude - A.Longitude) * PI / 180.0;
	
	double a = FMath::Sin(DLat / 2.0) * FMath::Sin(DLat / 2.0) +
	           FMath::Cos(Lat1) * FMath::Cos(Lat2) *
	           FMath::Sin(DLon / 2.0) * FMath::Sin(DLon / 2.0);
	
	double c = 2.0 * FMath::Atan2(FMath::Sqrt(a), FMath::Sqrt(1.0 - a));
	
	return static_cast<float>(EarthRadius * c);
}
