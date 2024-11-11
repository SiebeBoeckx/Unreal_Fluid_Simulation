// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_GridManager.generated.h"

class AC_PointVector;

UCLASS()
class FLUID_SIMULATION_API AC_GridManager final : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_GridManager();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor>ActorToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int m_GridSize{10};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float m_GapSize{100.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float m_DiffuseAmount{ 0.01f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float m_Viscosity{ 0.01f };

private:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TArray<AActor*> m_pPointVectors{};
	int m_RealGridSize{};
	const int m_Iterations{ 4 };

	void Populate();

	void HandleDensities(float dt);
	void LinearSolveDensities(float a);
	void AdVectDensities(float dt);

	void SwapDensities();
	float GetNeighborDensities(int x, int y, int z);
	void SetBoundsDiffuse();
	float AdvectPrevDensityCalculations(int i, int j, int k, int i1, int j1, int k1, float s, float t, float u, float s1, float t1, float u1);

	void HandleVelocities(float dt);
	void LinearSolveVelocities(float a);
	void AdVectVelocities(float dt);
	void Project();

	void SwapVelocities();
	FVector GetNeighborVelocities(int x, int y, int z);
	void SetBoundsVelocity();
	FVector AdvectPrevVelocityCalculations(int i, int j, int k, int i1, int j1, int k1, float s, float t, float u, float s1, float t1, float u1);
	void SetDivergence(int x, int y, int z, float h);
	void SetBoundsDivergence();
	void SetBoundsPressure();
	void LinearSolvePressure(); //A little different from the other linear solvers
	void SetProjectedVelocities(float h);

	int GetIdx(int x, int y, int z);
	float AdVectIfChecks(float value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
