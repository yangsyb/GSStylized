// Fill out your copyright notice in the Description page of Project Settings.


#include "StylizedMesh.h"

void AStylizedMesh::Rebuild()
{
	UDynamicMesh* Self = this->GetDynamicMeshComponent()->GetDynamicMesh();
	Self->Reset();
	this->OnRebuildGeneratedMesh(Self);
}

void AStylizedMesh::RebuildStylizedMesh(UDynamicMesh* TargetMesh)
{

}
