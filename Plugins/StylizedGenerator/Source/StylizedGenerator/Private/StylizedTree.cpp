// Fill out your copyright notice in the Description page of Project Settings.


#include "StylizedTree.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include <GeometryScript/MeshPrimitiveFunctions.h>
#include <GeometryScript/MeshSpatialFunctions.h>
#include <GeometryScript/MeshQueryFunctions.h>
#include <GeometryScript/MeshBasicEditFunctions.h>
#include <GeometryScript/MeshSubdivideFunctions.h>
#include <GeometryScript/MeshNormalsFunctions.h>
#include "PCGFunctions.h"

AStylizedTree::AStylizedTree(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TreeSpline = CreateDefaultSubobject<USplineComponent>("TreeSpline");
	TrunkSpline = CreateDefaultSubobject<USplineComponent>("TrunkSpline");
	TreeSpline->SetupAttachment(this->RootComponent);
	TrunkSpline->SetupAttachment(this->RootComponent);

	TArray<FVector> DefaultTreeSplinePoints;
	DefaultTreeSplinePoints.Add(FVector(0, 0, 0));
	DefaultTreeSplinePoints.Add(FVector(0, 0, 800));
	TreeSpline->SetSplinePoints(DefaultTreeSplinePoints, ESplineCoordinateSpace::Type::Local, true);

	TArray<FVector> DefaultTrunkSplinePoints;
	DefaultTrunkSplinePoints.Add(FVector(-50, -50, 0));
	DefaultTrunkSplinePoints.Add(FVector(-50, 50, 0));
	DefaultTrunkSplinePoints.Add(FVector(50, 50, 0));
	DefaultTrunkSplinePoints.Add(FVector(50, -50, 0));
	TrunkSpline->SetClosedLoop(true);
	TrunkSpline->SetSplinePoints(DefaultTrunkSplinePoints, ESplineCoordinateSpace::Type::Local, true);
}

void AStylizedTree::PostEditMove(bool bFinished)
{
	if (bFinished)
	{
		CachedTreePointsLocation.Empty();
		CachedTrunkPointsLocation.Empty();
		for (int Index = 0; Index <= TreeTessellation; Index++)
		{
			FVector CurrentSplineLocation = TreeSpline->GetLocationAtTime(double(Index) / double(TreeTessellation), ESplineCoordinateSpace::Local);
			CachedTreePointsLocation.Add(CurrentSplineLocation);
		}

		for (int Index = 0; Index <= TrunkTessellation; Index++)
		{
			FVector CurrentSplineLocation = TrunkSpline->GetLocationAtTime(double(Index) / double(TrunkTessellation), ESplineCoordinateSpace::Local);
			FVector2D CurrentSplineLocation2D = FVector2D(CurrentSplineLocation.X, CurrentSplineLocation.Y);
			CachedTrunkPointsLocation.Insert(CurrentSplineLocation2D, 0);
		}

		this->Rebuild();
	}
}

void AStylizedTree::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//TODO Better Cache
	CachedTreePointsLocation.Empty();
	CachedTrunkPointsLocation.Empty();
	for (int Index = 0; Index <= TreeTessellation; Index++)
	{
		FVector CurrentSplineLocation = TreeSpline->GetLocationAtTime(double(Index) / double(TreeTessellation), ESplineCoordinateSpace::Local);
		CachedTreePointsLocation.Add(CurrentSplineLocation);
	}

	for (int Index = 0; Index <= TrunkTessellation; Index++)
	{
		FVector CurrentSplineLocation = TrunkSpline->GetLocationAtTime(double(Index) / double(TrunkTessellation), ESplineCoordinateSpace::Local);
		FVector2D CurrentSplineLocation2D = FVector2D(CurrentSplineLocation.X, CurrentSplineLocation.Y);
		CachedTrunkPointsLocation.Insert(CurrentSplineLocation2D, 0);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AStylizedTree::RebuildStylizedMesh(UDynamicMesh* TargetMesh)
{
	//ConstructTreeTrunk
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleSweptPolygon(TargetMesh, FGeometryScriptPrimitiveOptions(), FTransform(), CachedTrunkPointsLocation, CachedTreePointsLocation, false, true, 1.0, TreeTopScale);
	//Tessellation to get more triangles, for better find nearest points in spatial
	//UGeometryScriptLibrary_MeshSubdivideFunctions::ApplyUniformTessellation(TargetMesh, 2);

	//BranchScatter
	FRandomStream RandomStream = UKismetMathLibrary::MakeRandomStream(BranchSeed);
	//TODO Cache BVH
	UGeometryScriptLibrary_MeshSpatial::BuildBVHForMesh(TargetMesh, DynamicMeshBVH);
	FBox MainBoundingBox = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(TargetMesh);
	FVector MainBoundingBoxCenter, MainBoundingBoxExtent;
	MainBoundingBoxCenter = (MainBoundingBox.Min + MainBoundingBox.Max) * 0.5;
	MainBoundingBoxExtent = (MainBoundingBox.Max - MainBoundingBox.Min) * 0.5;
	UDynamicMesh* SecondLevelBranches = AllocateComputeMesh();

	// Second Level Branches
	for (int Index = 0; Index < SecondBranchNum; Index++)
	{
		FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBoxFromStream(MainBoundingBoxCenter, MainBoundingBoxExtent, RandomStream);
		FGeometryScriptTrianglePoint NearestResult;
		TEnumAsByte<EGeometryScriptSearchOutcomePins> Outcome;
		UGeometryScriptLibrary_MeshSpatial::FindNearestPointOnMesh(TargetMesh, DynamicMeshBVH, RandomPoint, FGeometryScriptSpatialQueryOptions(), NearestResult, Outcome);
		if (Outcome == EGeometryScriptSearchOutcomePins::Found)
		{
			FTransform BranchTransform;
			BranchTransform.SetLocation(NearestResult.Position);
			bool bIsValidTriangle;
			FVector BranchNormal = UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleFaceNormal(TargetMesh, NearestResult.TriangleID, bIsValidTriangle);

			int HitTessellation = UPCGFunctions::FindNearest(NearestResult.Position, CachedTreePointsLocation);
			float HitScale = FMath::Lerp(1.0f, TreeTopScale, float(HitTessellation) / float(TreeTessellation));


			BranchTransform.SetRotation(UKismetMathLibrary::MakeRotFromZ(BranchNormal).Quaternion());

			TArray<FVector> BranchPointsLocation = UPCGFunctions::GenerateBranchSplinePoints(NearestResult.Position, BranchNormal, FVector(0, 0, 1), 400, 20, 0.7);
			BranchPointsLocation.Insert(CachedTreePointsLocation[HitTessellation], 0);
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleSweptPolygon(SecondLevelBranches, FGeometryScriptPrimitiveOptions(), FTransform(), CachedTrunkPointsLocation, BranchPointsLocation, false, true, HitScale, 0.1);
		}
	}
	//UGeometryScriptLibrary_MeshNormalsFunctions::RecomputeNormals(SecondLevelBranches, FGeometryScriptCalculateNormalsOptions());

	// Third Level Branches
	FGeometryScriptDynamicMeshBVH SecondLevelBVH;
	UGeometryScriptLibrary_MeshSpatial::BuildBVHForMesh(SecondLevelBranches, SecondLevelBVH);
	FBox SecondBoundingBox = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(SecondLevelBranches);
	FVector SecondBoundingBoxCenter, SecondBoundingBoxExtent;
	SecondBoundingBoxCenter = (SecondBoundingBox.Min + SecondBoundingBox.Max) * 0.5;
	SecondBoundingBoxExtent = (SecondBoundingBox.Max - SecondBoundingBox.Min) * 0.5;
	UDynamicMesh* ThirdLevelBranches = AllocateComputeMesh();

	for (int Index = 0; Index < SecondBranchNum * ThirdBranchNum; Index++)
	{
		FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBoxFromStream(SecondBoundingBoxCenter, SecondBoundingBoxExtent, RandomStream);
		FGeometryScriptTrianglePoint NearestResult;
		TEnumAsByte<EGeometryScriptSearchOutcomePins> Outcome;
		UGeometryScriptLibrary_MeshSpatial::FindNearestPointOnMesh(SecondLevelBranches, SecondLevelBVH, RandomPoint, FGeometryScriptSpatialQueryOptions(), NearestResult, Outcome);
		if (Outcome == EGeometryScriptSearchOutcomePins::Found)
		{
			FTransform BranchTransform;
			BranchTransform.SetLocation(NearestResult.Position);
			bool bIsValidTriangle;
			FVector BranchNormal = UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleFaceNormal(SecondLevelBranches, NearestResult.TriangleID, bIsValidTriangle);
			BranchTransform.SetRotation(UKismetMathLibrary::MakeRotFromZ(BranchNormal).Quaternion());

			TArray<FVector> BranchPointsLocation = UPCGFunctions::GenerateBranchSplinePoints(NearestResult.Position, BranchNormal, FVector(0, 0, 1), 100, 20, 0.8);
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleSweptPolygon(ThirdLevelBranches, FGeometryScriptPrimitiveOptions(), FTransform(), CachedTrunkPointsLocation, BranchPointsLocation, false, true, 0.2, 0.02);
			//UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(ScatteredBranch, FGeometryScriptPrimitiveOptions(), BranchTransform, 30, 30, 30);
		}
	}

	UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(TargetMesh, SecondLevelBranches, FTransform());
	UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(TargetMesh, ThirdLevelBranches, FTransform());
	ReleaseComputeMesh(SecondLevelBranches);
	ReleaseComputeMesh(ThirdLevelBranches);
}
