// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StylizedMesh.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include <GeometryScript/GeometryScriptTypes.h>
#include "TreeBranch.h"
#include "StylizedTree.generated.h"

/**
 * 
 */
UCLASS()
class STYLIZEDGENERATOR_API AStylizedTree : public AStylizedMesh
{
	GENERATED_UCLASS_BODY()
protected:
	UPROPERTY(VisibleAnywhere, BluePrintReadWrite)
	USplineComponent* TreeSpline;

	UPROPERTY(VisibleAnywhere, BluePrintReadWrite)
	USplineComponent* TrunkSpline;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int TreeTessellation = 20;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int TrunkTessellation = 20;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	float TreeTopScale = 0.3;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int BranchSeed = 558;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int SecondBranchNum = 4;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int ThirdBranchNum = 5;

	TArray<FVector> CachedTreePointsLocation;
	TArray<FVector2D> CachedTrunkPointsLocation;

	TObjectPtr<TreeBranch> MainBranch;
	FGeometryScriptDynamicMeshBVH DynamicMeshBVH;


	void PostEditMove(bool bFinished) override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
private:
	virtual void RebuildStylizedMesh(UDynamicMesh* TargetMesh) override;
};
