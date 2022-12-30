// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeometryActors/GeneratedDynamicMeshActor.h"
#include "StylizedMesh.generated.h"

/**
 * 
 */
UCLASS()
class STYLIZEDGENERATOR_API AStylizedMesh : public AGeneratedDynamicMeshActor
{
	GENERATED_BODY()

protected:
	void Rebuild();

	UFUNCTION(BlueprintCallable, CallInEditor)
	virtual void RebuildStylizedMesh(UDynamicMesh* TargetMesh);
};
