// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <GeometryFramework/Public/UDynamicMesh.h>
#include <GeometryScript/MeshPrimitiveFunctions.h>
#include <GeometryScript/GeometryScriptTypes.h>
#include "TreeBranch.h"
#include <Generators/SweepGenerator.h>

#include "GeometryScriptFunctionsExtension.generated.h"

/**
 * 
 */

UCLASS()
class STYLIZEDGENERATOR_API UGeometryScriptFunctionsExtension : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
 	static UDynamicMesh* AppendSweptBranch(
 		UDynamicMesh* TargetMesh,
 		FGeometryScriptPrimitiveOptions PrimitiveOptions,
 		FTransform Transform,
 		const TArray<FVector2D>& PolygonVertices,
 		const TArray<BranchPoint>& SweepPath,
 		bool bLoop = false,
 		bool bCapped = true,
 		UGeometryScriptDebug* Debug = nullptr);
	
};

