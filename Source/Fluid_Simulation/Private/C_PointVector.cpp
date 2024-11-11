// Fill out your copyright notice in the Description page of Project Settings.


#include "C_PointVector.h"

// Sets default values
AC_PointVector::AC_PointVector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AC_PointVector::BeginPlay()
{
	Super::BeginPlay();

	float randomLength = FMath::RandRange(1.f, 3.f);
	m_Velocity = FMath::VRand() * randomLength;
}

// Called every frame
void AC_PointVector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//UE_LOG(LogTemp, Warning, TEXT("PointVector tick"));
}

void AC_PointVector::SwapVariables()
{
	SwapVelocities();
	SwapDensities();
}

void AC_PointVector::SwapVelocities()
{
	FVector tempVel = m_PrevVelocity;
	m_PrevVelocity = m_Velocity;
	m_Velocity = tempVel;
}

void AC_PointVector::SwapDensities()
{
	float tempDens = m_PrevDensity;
	m_PrevDensity = m_Density;
	m_Density = tempDens;
}

