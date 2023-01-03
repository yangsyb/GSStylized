// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <GeometryScript/GeometryScriptTypes.h>
#include <GeometryFramework/Public/UDynamicMesh.h>

/**
 * 
 */
 class STYLIZEDGENERATOR_API BranchPoint
 {
 public:
	 BranchPoint();
	 ~BranchPoint();

	 FVector Location;
	 float Scale;
 };


class STYLIZEDGENERATOR_API TreeBranch
{
public:
	TreeBranch();
	~TreeBranch();

	TArray<FVector> GetBranchGrowthPointsLocation();
	void GenerateBoundingBox();
	void GenerateChildBranches();

	int BranchSeed;
	double BranchLength;
	float GrowthStartPercent;
	int ChildBranchNum;
	FVector BoundingBoxCenter; 
	FVector BoundingBoxExtent;

	TObjectPtr<UDynamicMesh> Mesh;
	TArray<BranchPoint> BranchGrowthPoints;
	TArray<FVector2D> BranchShapePoints;
	TArray<TObjectPtr<TreeBranch>> ChildBranches;
	FGeometryScriptDynamicMeshBVH MeshBVH;
};
