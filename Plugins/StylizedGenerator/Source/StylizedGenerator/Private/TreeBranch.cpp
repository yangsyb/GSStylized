// Fill out your copyright notice in the Description page of Project Settings.


#include "TreeBranch.h"
#include <GeometryScript/MeshQueryFunctions.h>
#include <Kismet/KismetMathLibrary.h>
#include <GeometryScript/MeshSpatialFunctions.h>
#include <GeometryScript/MeshPrimitiveFunctions.h>
#include "PCGFunctions.h"

BranchPoint::BranchPoint()
{

}

BranchPoint::~BranchPoint()
{

}

TreeBranch::TreeBranch()
{
	Mesh = NewObject<UDynamicMesh>();
}

TreeBranch::~TreeBranch()
{
	
}

TArray<FVector> TreeBranch::GetBranchGrowthPointsLocation()
{
	TArray<FVector> Ret;
	for(auto BranchPoint : BranchGrowthPoints)
	{
		Ret.Add(BranchPoint.Location);
	}
	return Ret;
}

void TreeBranch::GenerateBoundingBox()
{
	if(!Mesh) return;
	FBox BoundingBox = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(Mesh);
	BoundingBoxCenter = (BoundingBox.Min + BoundingBox.Max) * 0.5;
	BoundingBoxExtent = (BoundingBox.Max - BoundingBox.Min) * 0.5;
}

void TreeBranch::GenerateChildBranches()
{
	if (!Mesh) return;
	FRandomStream RandomStream = UKismetMathLibrary::MakeRandomStream(BranchSeed);
	for (int Index = 0; Index < ChildBranchNum; Index++)
	{
		TObjectPtr<TreeBranch> CurrentChildBranch = NewObject<TreeBranch>();
		FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBoxFromStream(BoundingBoxCenter, BoundingBoxExtent, RandomStream);
		FGeometryScriptTrianglePoint NearestResult;
		TEnumAsByte<EGeometryScriptSearchOutcomePins> Outcome;
		UGeometryScriptLibrary_MeshSpatial::FindNearestPointOnMesh(Mesh, MeshBVH, RandomPoint, FGeometryScriptSpatialQueryOptions(), NearestResult, Outcome);
		if (Outcome == EGeometryScriptSearchOutcomePins::Found)
		{
			FTransform BranchTransform;
			BranchTransform.SetLocation(NearestResult.Position);
			bool bIsValidTriangle;
			FVector BranchNormal = UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleFaceNormal(Mesh, NearestResult.TriangleID, bIsValidTriangle);

			int HitTessellation = UPCGFunctions::FindNearest(NearestResult.Position, GetBranchGrowthPointsLocation());
			//float HitScale = FMath::Lerp(1.0f, TreeTopScale, float(HitTessellation) / float(TreeTessellation));
			float HitScale = 1;


			BranchTransform.SetRotation(UKismetMathLibrary::MakeRotFromZ(BranchNormal).Quaternion());

			TArray<FVector> BranchPointsLocation = UPCGFunctions::GenerateBranchSplinePoints(NearestResult.Position, BranchNormal, FVector(0, 0, 1), 400, 20, 0.7);
			BranchPointsLocation.Insert(CachedTreePointsLocation[HitTessellation], 0);
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleSweptPolygon(CurrentChildBranch->Mesh, FGeometryScriptPrimitiveOptions(), FTransform(), CachedTrunkPointsLocation, BranchPointsLocation, false, true, HitScale, 0.1);
		}
		ChildBranches.Add(CurrentChildBranch);
	}
}
