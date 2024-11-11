// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_PointVector.generated.h"

//Class setup more like a struct

UCLASS()
class FLUID_SIMULATION_API AC_PointVector final : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_PointVector();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector m_Velocity{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float m_Density{};

protected:
	// Called when the game starts or when spawned
	UFUNCTION(BlueprintCallable)
	virtual void BeginPlay() override;

public:	
	// Called every frame
	UFUNCTION(BlueprintCallable)
	virtual void Tick(float DeltaTime) override;

	FVector m_PrevVelocity{};
	float m_PrevDensity{};

	void SwapVariables();
	void SwapVelocities();
	void SwapDensities();

};
