// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGFunctions.h"

TArray<FVector> UPCGFunctions::GenerateBranchSplinePoints(FVector StartLocation, FVector StartNormal, FVector UpVector, double BranchLength, int TessellationNum, double NormalWeightMin)
{
	TArray<FVector> RetBranchPoints;
	FVector CurrentPointLocation = StartLocation;
	RetBranchPoints.Add(CurrentPointLocation);

	double SingleTessBrachLength = BranchLength / TessellationNum;
	for (int Index = 0; Index < TessellationNum; Index++)
	{
		double NormalWeight = FMath::RandRange(NormalWeightMin, 1.0);
		double UpWeight = 1.0 - NormalWeight;
		FVector GrowthDirction = NormalWeight * StartNormal + UpWeight * UpVector;

		double SquaredNormalizedGrowth = FMath::Square(SingleTessBrachLength) / (FMath::Square(GrowthDirction.X) + FMath::Square(GrowthDirction.Y) + FMath::Square(GrowthDirction.Z));
		double NormalizedGrowth = FMath::Sqrt(SquaredNormalizedGrowth);
		FVector Growth = FVector(NormalizedGrowth * GrowthDirction.X, NormalizedGrowth * GrowthDirction.Y, NormalizedGrowth * GrowthDirction.Z);
		CurrentPointLocation += Growth;

		RetBranchPoints.Add(CurrentPointLocation);
	}

	return RetBranchPoints;
}