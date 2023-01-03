// Fill out your copyright notice in the Description page of Project Settings.


#include "GeometryScriptFunctionsExtension.h"
#include <Generators/SweepGenerator.h>
#include <GeometryScript/MeshPrimitiveFunctions.h>
#include <DynamicMesh/MeshTransforms.h>
#include <DynamicMesh/Public/DynamicMeshEditor.h>


using namespace UE::Geometry;

class FGeneralizedBranchGenerator : public FSweepGeneratorBase
{
public:
	FPolygon2d CrossSection;
	TArray<FVector3d> Path;
	TArray<float> PathScale;
	FFrame3d InitialFrame;
	TArray<FFrame3d> PathFrames;
	TArray<FVector2d> PathScales;

	bool bCapped = false;
	bool bLoop = false;
	ECapType CapType = ECapType::FlatTriangulation;

	bool bUVScaleRelativeWorld = false;
	float UnitUVInWorldCoordinates = 100;

	TArray<float> CrossSectionTexCoord;
	TArray<float> PathTexCoord;

public:
	virtual FMeshShapeGenerator& Generate() override
	{
		const TArray<FVector2d>& XVerts = CrossSection.GetVertices();
		ECapType Caps[2] = { ECapType::None, ECapType::None };

		if (bCapped && !bLoop)
		{
			Caps[0] = CapType;
			Caps[1] = CapType;
		}
		int PathNum = Path.Num();

		bool bHavePathScaling = (PathScales.Num() == PathNum);
		bool bApplyScaling = (bHavePathScaling) && (bLoop == false);
		bool bNeedArcLength = (bApplyScaling || bUVScaleRelativeWorld);
		double TotalPathArcLength = (bNeedArcLength) ? UE::Geometry::CurveUtil::ArcLength<double, FVector3d>(Path, bLoop) : 1.0;

		FAxisAlignedBox2f Bounds = (FAxisAlignedBox2f)CrossSection.Bounds();
		double BoundsMaxDimInv = 1.0 / FMathd::Max(Bounds.MaxDim(), .001);
		FVector2f SectionScale(1.f, 1.f), CapScale((float)BoundsMaxDimInv, (float)BoundsMaxDimInv);
		if (bUVScaleRelativeWorld)
		{
			double Perimeter = CrossSection.Perimeter();
			SectionScale.X = float(Perimeter / UnitUVInWorldCoordinates);
			SectionScale.Y = float(TotalPathArcLength / UnitUVInWorldCoordinates);
			CapScale.X = CapScale.Y = 1.0f / UnitUVInWorldCoordinates;
		}
		ConstructMeshTopology(CrossSection, {}, {}, {}, true, Path, PathNum + (bLoop ? 1 : 0), bLoop, Caps, SectionScale, CapScale, Bounds.Center(), CrossSectionTexCoord, PathTexCoord);

		int XNum = CrossSection.VertexCount();
		TArray<FVector2d> XNormals; XNormals.SetNum(XNum);
		for (int Idx = 0; Idx < XNum; Idx++)
		{
			XNormals[Idx] = CrossSection.GetNormal_FaceAvg(Idx);
		}

		double AccumArcLength = 0;
		FFrame3d CrossSectionFrame = InitialFrame;
		bool bHaveExplicitFrames = (PathFrames.Num() == Path.Num());
		for (int PathIdx = 0; PathIdx < PathNum; ++PathIdx)
		{
			FVector3d C = Path[PathIdx];
			FVector3d X, Y;
			if (bHaveExplicitFrames == false)
			{
				FVector3d Tangent = UE::Geometry::CurveUtil::Tangent<double, FVector3d>(Path, PathIdx, bLoop);
				CrossSectionFrame.AlignAxis(2, Tangent);
				X = CrossSectionFrame.X();
				Y = CrossSectionFrame.Y();
			}
			else
			{
				C = PathFrames[PathIdx].Origin;
				X = PathFrames[PathIdx].X();
				Y = PathFrames[PathIdx].Y();
			}

			double T = FMathd::Clamp((AccumArcLength / TotalPathArcLength), 0.0, 1.0);
			//double UniformScale = (bApplyScaling) ? FMathd::Lerp(StartScale, EndScale, T) : 1.0;
			double UniformScale = PathScale[PathIdx];
			FVector2d PathScaling = (bHavePathScaling) ? PathScales[PathIdx] : FVector2d::One();

			for (int SubIdx = 0; SubIdx < XNum; SubIdx++)
			{
				FVector2d XP = UniformScale * PathScaling * CrossSection[SubIdx];
				FVector2d XN = XNormals[SubIdx];
				Vertices[SubIdx + PathIdx * XNum] = C + X * XP.X + Y * XP.Y;
				Normals[SubIdx + PathIdx * XNum] = (FVector3f)(X * XN.X + Y * XN.Y);
			}

			if (PathIdx < PathNum - 1)
			{
				AccumArcLength += Distance(C, Path[PathIdx + 1]);
			}
		}
		if (bCapped && !bLoop)
		{
			for (int CapIdx = 0; CapIdx < 2; CapIdx++)
			{
				if (Caps[CapIdx] == ECapType::FlatMidpointFan)
				{
					Vertices[CapVertStart[CapIdx]] = Path[CapIdx * (Path.Num() - 1)];
				}
			}

			for (int CapIdx = 0; CapIdx < 2; CapIdx++)
			{
				FVector3d Normal = CurveUtil::Tangent<double, FVector3d>(Path, CapIdx * (PathNum - 1), bLoop) * (double)(CapIdx * 2 - 1);
				for (int SubIdx = 0; SubIdx < XNum; SubIdx++)
				{
					Normals[CapNormalStart[CapIdx] + SubIdx] = (FVector3f)Normal;
				}
			}
		}

		for (int k = 0; k < Normals.Num(); ++k)
		{
			Normalize(Normals[k]);
		}

		return *this;
	}
};


static void AppendPrimitive(
	UDynamicMesh* TargetMesh,
	FMeshShapeGenerator* Generator,
	FTransform Transform,
	FGeometryScriptPrimitiveOptions PrimitiveOptions,
	FVector3d PreTranslate = FVector3d::Zero())
{
	auto ApplyOptionsToMesh = [&Transform, &PrimitiveOptions, PreTranslate](FDynamicMesh3& Mesh)
	{
		if (PreTranslate.SquaredLength() > 0)
		{
			MeshTransforms::Translate(Mesh, PreTranslate);
		}

		MeshTransforms::ApplyTransform(Mesh, (FTransformSRT3d)Transform, true);
		if (PrimitiveOptions.PolygroupMode == EGeometryScriptPrimitivePolygroupMode::SingleGroup)
		{
			for (int32 tid : Mesh.TriangleIndicesItr())
			{
				Mesh.SetTriangleGroup(tid, 0);
			}
		}
		if (PrimitiveOptions.bFlipOrientation)
		{
			Mesh.ReverseOrientation(true);
			if (Mesh.HasAttributes())
			{
				FDynamicMeshNormalOverlay* Normals = Mesh.Attributes()->PrimaryNormals();
				for (int elemid : Normals->ElementIndicesItr())
				{
					Normals->SetElement(elemid, -Normals->GetElement(elemid));
				}
			}
		}
	};

	if (TargetMesh->IsEmpty())
	{
		TargetMesh->EditMesh([&](FDynamicMesh3& EditMesh)
			{
				EditMesh.Copy(Generator);
				ApplyOptionsToMesh(EditMesh);
			}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);
	}
	else
	{
		FDynamicMesh3 TempMesh(Generator);
		ApplyOptionsToMesh(TempMesh);
		TargetMesh->EditMesh([&](FDynamicMesh3& EditMesh)
			{
				FMeshIndexMappings TmpMappings;
				FDynamicMeshEditor Editor(&EditMesh);
				Editor.AppendMesh(&TempMesh, TmpMappings);

			}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);
	}
}


UDynamicMesh* UGeometryScriptFunctionsExtension::AppendSweptBranch(UDynamicMesh* TargetMesh, FGeometryScriptPrimitiveOptions PrimitiveOptions, FTransform Transform, const TArray<FVector2D>& PolygonVertices, const TArray<BranchPoint>& SweepPath, bool bLoop /*= false*/, bool bCapped /*= true*/, UGeometryScriptDebug* Debug /*= nullptr*/)
{
	if (TargetMesh == nullptr)
	{
		return TargetMesh;
	}
	if (PolygonVertices.Num() < 3)
	{
		return TargetMesh;
	}
	if (SweepPath.Num() < 2)
	{
		return TargetMesh;
	}

	FGeneralizedBranchGenerator SweepGen;
	for (FVector2D Point : PolygonVertices)
	{
		SweepGen.CrossSection.AppendVertex(FVector2d(Point.X, Point.Y));
	}
 	for (BranchPoint SweepPathPos : SweepPath)
 	{
 		SweepGen.Path.Add(SweepPathPos.Location);
		SweepGen.PathScale.Add(SweepPathPos.Scale);
 	}

	SweepGen.bLoop = bLoop;
	SweepGen.bCapped = bCapped;
	SweepGen.bPolygroupPerQuad = (PrimitiveOptions.PolygroupMode == EGeometryScriptPrimitivePolygroupMode::PerQuad);
	SweepGen.InitialFrame = FFrame3d(SweepGen.Path[0]);

	SweepGen.Generate();

	AppendPrimitive(TargetMesh, &SweepGen, Transform, PrimitiveOptions);
	return TargetMesh;
}
