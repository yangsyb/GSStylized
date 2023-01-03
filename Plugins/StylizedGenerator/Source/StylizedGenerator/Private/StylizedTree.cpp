// Fill out your copyright notice in the Description page of Project Settings.


#include "StylizedTree.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GeometryScriptFunctionsExtension.h"
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
// 		CachedTreePointsLocation.Empty();
// 		CachedTrunkPointsLocation.Empty();
// 		for (int Index = 0; Index <= TreeTessellation; Index++)
// 		{
// 			FVector CurrentSplineLocation = TreeSpline->GetLocationAtTime(double(Index) / double(TreeTessellation), ESplineCoordinateSpace::Local);
// 			CachedTreePointsLocation.Add(CurrentSplineLocation);
// 		}
// 
// 		for (int Index = 0; Index <= TrunkTessellation; Index++)
// 		{
// 			FVector CurrentSplineLocation = TrunkSpline->GetLocationAtTime(double(Index) / double(TrunkTessellation), ESplineCoordinateSpace::Local);
// 			FVector2D CurrentSplineLocation2D = FVector2D(CurrentSplineLocation.X, CurrentSplineLocation.Y);
// 			CachedTrunkPointsLocation.Insert(CurrentSplineLocation2D, 0);
// 		}
// 
// 		this->Rebuild();


		MainBranch->BranchGrowthPoints.Empty();
		MainBranch->BranchShapePoints.Empty();
		for(int Index = 0; Index <= TreeTessellation; Index++)
		{
			BranchPoint CurrentBranchPoint;
			CurrentBranchPoint.Location = TreeSpline->GetLocationAtTime(double(Index) / double(TreeTessellation), ESplineCoordinateSpace::Local);
			CurrentBranchPoint.Scale = 1;
			MainBranch->BranchGrowthPoints.Add(CurrentBranchPoint);
		}
		for (int Index = 0; Index <= TrunkTessellation; Index++)
		{
			FVector CurrentSplineLocation = TrunkSpline->GetLocationAtTime(double(Index) / double(TrunkTessellation), ESplineCoordinateSpace::Local);
			FVector2D CurrentSplineLocation2D = FVector2D(CurrentSplineLocation.X, CurrentSplineLocation.Y);
			MainBranch->BranchShapePoints.Insert(CurrentSplineLocation2D, 0);
		}
		this->Rebuild();
	}
}

void AStylizedTree::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//TODO Better Cache
// 	CachedTreePointsLocation.Empty();
// 	CachedTrunkPointsLocation.Empty();
// 	for (int Index = 0; Index <= TreeTessellation; Index++)
// 	{
// 		FVector CurrentSplineLocation = TreeSpline->GetLocationAtTime(double(Index) / double(TreeTessellation), ESplineCoordinateSpace::Local);
// 		CachedTreePointsLocation.Add(CurrentSplineLocation);
// 	}
// 
// 	for (int Index = 0; Index <= TrunkTessellation; Index++)
// 	{
// 		FVector CurrentSplineLocation = TrunkSpline->GetLocationAtTime(double(Index) / double(TrunkTessellation), ESplineCoordinateSpace::Local);
// 		FVector2D CurrentSplineLocation2D = FVector2D(CurrentSplineLocation.X, CurrentSplineLocation.Y);
// 		CachedTrunkPointsLocation.Insert(CurrentSplineLocation2D, 0);
// 	}

	MainBranch->BranchGrowthPoints.Empty();
	MainBranch->BranchShapePoints.Empty();
	for (int Index = 0; Index <= TreeTessellation; Index++)
	{
		BranchPoint CurrentBranchPoint;
		CurrentBranchPoint.Location = TreeSpline->GetLocationAtTime(double(Index) / double(TreeTessellation), ESplineCoordinateSpace::Local);
		CurrentBranchPoint.Scale = 1;
		MainBranch->BranchGrowthPoints.Add(CurrentBranchPoint);
	}
	for (int Index = 0; Index <= TrunkTessellation; Index++)
	{
		FVector CurrentSplineLocation = TrunkSpline->GetLocationAtTime(double(Index) / double(TrunkTessellation), ESplineCoordinateSpace::Local);
		FVector2D CurrentSplineLocation2D = FVector2D(CurrentSplineLocation.X, CurrentSplineLocation.Y);
		MainBranch->BranchShapePoints.Insert(CurrentSplineLocation2D, 0);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AStylizedTree::RebuildStylizedMesh(UDynamicMesh* TargetMesh)
{
	//ConstructTreeTrunk
	UGeometryScriptFunctionsExtension::AppendSweptBranch(MainBranch->Mesh, FGeometryScriptPrimitiveOptions(), FTransform(), MainBranch->BranchShapePoints, MainBranch->BranchGrowthPoints, false, true);

	//BranchScatter
	FRandomStream RandomStream = UKismetMathLibrary::MakeRandomStream(BranchSeed);
	//TODO Cache BVH
	UGeometryScriptLibrary_MeshSpatial::BuildBVHForMesh(TargetMesh, MainBranch->MeshBVH);
	MainBranch->GenerateBoundingBox();
	MainBranch->GenerateChildBranches();

	// Second Level Branches
// 	for (int Index = 0; Index < SecondBranchNum; Index++)
// 	{
// 		FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBoxFromStream(MainBranch->BoundingBoxCenter, MainBranch->BoundingBoxExtent, RandomStream);
// 		FGeometryScriptTrianglePoint NearestResult;
// 		TEnumAsByte<EGeometryScriptSearchOutcomePins> Outcome;
// 		UGeometryScriptLibrary_MeshSpatial::FindNearestPointOnMesh(TargetMesh, MainBranch->MeshBVH, RandomPoint, FGeometryScriptSpatialQueryOptions(), NearestResult, Outcome);
// 		if (Outcome == EGeometryScriptSearchOutcomePins::Found)
// 		{
// 			FTransform BranchTransform;
// 			BranchTransform.SetLocation(NearestResult.Position);
// 			bool bIsValidTriangle;
// 			FVector BranchNormal = UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleFaceNormal(TargetMesh, NearestResult.TriangleID, bIsValidTriangle);
// 
// 			int HitTessellation = UPCGFunctions::FindNearest(NearestResult.Position, MainBranch->GetBranchGrowthPointsLocation());
// 			//float HitScale = FMath::Lerp(1.0f, TreeTopScale, float(HitTessellation) / float(TreeTessellation));
// 			float HitScale = 1;
// 
// 
// 			BranchTransform.SetRotation(UKismetMathLibrary::MakeRotFromZ(BranchNormal).Quaternion());
// 
// 			TArray<FVector> BranchPointsLocation = UPCGFunctions::GenerateBranchSplinePoints(NearestResult.Position, BranchNormal, FVector(0, 0, 1), 400, 20, 0.7);
// 			BranchPointsLocation.Insert(CachedTreePointsLocation[HitTessellation], 0);
// 			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleSweptPolygon(SecondLevelBranches, FGeometryScriptPrimitiveOptions(), FTransform(), CachedTrunkPointsLocation, BranchPointsLocation, false, true, HitScale, 0.1);
// 		}
// 	}
	//UGeometryScriptLibrary_MeshNormalsFunctions::RecomputeNormals(SecondLevelBranches, FGeometryScriptCalculateNormalsOptions());

	// Third Level Branches
// 	FGeometryScriptDynamicMeshBVH SecondLevelBVH;
// 	UGeometryScriptLibrary_MeshSpatial::BuildBVHForMesh(SecondLevelBranches, SecondLevelBVH);
// 	FBox SecondBoundingBox = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(SecondLevelBranches);
// 	FVector SecondBoundingBoxCenter, SecondBoundingBoxExtent;
// 	SecondBoundingBoxCenter = (SecondBoundingBox.Min + SecondBoundingBox.Max) * 0.5;
// 	SecondBoundingBoxExtent = (SecondBoundingBox.Max - SecondBoundingBox.Min) * 0.5;
// 	UDynamicMesh* ThirdLevelBranches = AllocateComputeMesh();
// 
// 	for (int Index = 0; Index < SecondBranchNum * ThirdBranchNum; Index++)
// 	{
// 		FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBoxFromStream(SecondBoundingBoxCenter, SecondBoundingBoxExtent, RandomStream);
// 		FGeometryScriptTrianglePoint NearestResult;
// 		TEnumAsByte<EGeometryScriptSearchOutcomePins> Outcome;
// 		UGeometryScriptLibrary_MeshSpatial::FindNearestPointOnMesh(SecondLevelBranches, SecondLevelBVH, RandomPoint, FGeometryScriptSpatialQueryOptions(), NearestResult, Outcome);
// 		if (Outcome == EGeometryScriptSearchOutcomePins::Found)
// 		{
// 			FTransform BranchTransform;
// 			BranchTransform.SetLocation(NearestResult.Position);
// 			bool bIsValidTriangle;
// 			FVector BranchNormal = UGeometryScriptLibrary_MeshQueryFunctions::GetTriangleFaceNormal(SecondLevelBranches, NearestResult.TriangleID, bIsValidTriangle);
// 			BranchTransform.SetRotation(UKismetMathLibrary::MakeRotFromZ(BranchNormal).Quaternion());
// 
// 			TArray<FVector> BranchPointsLocation = UPCGFunctions::GenerateBranchSplinePoints(NearestResult.Position, BranchNormal, FVector(0, 0, 1), 100, 20, 0.8);
// 			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleSweptPolygon(ThirdLevelBranches, FGeometryScriptPrimitiveOptions(), FTransform(), CachedTrunkPointsLocation, BranchPointsLocation, false, true, 0.2, 0.02);
// 			//UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(ScatteredBranch, FGeometryScriptPrimitiveOptions(), BranchTransform, 30, 30, 30);
// 		}
// 	}
// 
// 	UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(TargetMesh, SecondLevelBranches, FTransform());
// 	UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(TargetMesh, ThirdLevelBranches, FTransform());
// 	ReleaseComputeMesh(SecondLevelBranches);
// 	ReleaseComputeMesh(ThirdLevelBranches);
}
