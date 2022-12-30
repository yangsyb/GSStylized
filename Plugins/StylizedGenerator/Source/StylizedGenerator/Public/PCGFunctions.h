// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PCGFunctions.generated.h"

/**
 * 
 */
UCLASS()
class STYLIZEDGENERATOR_API UPCGFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = Generation)
	static TArray<FVector> GenerateBranchSplinePoints(FVector StartLocation, FVector StartNormal, FVector UpVector, double BranchLength, int TessellationNum, double NormalWeightMin);

	UFUNCTION(BlueprintCallable, Category = Generation)
	static int FindNearest(FVector Location, TArray<FVector> LocationList);
};
