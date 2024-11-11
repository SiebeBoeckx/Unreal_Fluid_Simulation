// Fill out your copyright notice in the Description page of Project Settings.


#include "C_GridManager.h"
#include "C_PointVector.h"

// Sets default values
AC_GridManager::AC_GridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AC_GridManager::BeginPlay()
{
	Super::BeginPlay();
	
	Populate();
}

void AC_GridManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	m_pPointVectors.Empty();
}

void AC_GridManager::Populate()
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("Incapable of getting World, GridManager/Populate"));
		return;
	}

	m_RealGridSize = m_GridSize + 2; //2 Extra in all directions for boundaries
	const float worldOffset = (m_RealGridSize * m_GapSize) / 2 - m_GapSize / 2; //Distance to offset around center around 0,0,0

	for (int i{}; i < m_RealGridSize; ++i) //X-loop
	{
		for (int j{}; j < m_RealGridSize; ++j) //Y-loop
		{
			for (int k{}; k < m_RealGridSize; ++k) //Z-loop
			{
				const float x = i * m_GapSize - worldOffset;
				const float y = j * m_GapSize - worldOffset;
				const float z = k * m_GapSize - worldOffset;
				const FVector pos{ x,y,z };

				auto pPointVector = GetWorld()->SpawnActor<AActor>(ActorToSpawn, pos, FRotator::ZeroRotator);
				m_pPointVectors.Add(pPointVector);
			}
		}
	}
}

#pragma region Density

void AC_GridManager::HandleDensities(float dt)
{
	SwapDensities();

	const float a = dt * m_DiffuseAmount * m_GridSize * m_GridSize;
	LinearSolveDensities(a);

	SwapDensities();
	
	AdVectDensities(dt);
}

void AC_GridManager::LinearSolveDensities(const float a)
{
	for (int iter{}; iter < m_Iterations; ++iter)
	{
		for (int x{ 1 }; x <= m_GridSize; ++x)
		{
			for (int y{ 1 }; y <= m_GridSize; ++y)
			{
				for (int z{ 1 }; z <= m_GridSize; ++z)
				{
					const int idx{ GetIdx(x,y,z) };
					AC_PointVector* pPointVector = Cast<AC_PointVector>(m_pPointVectors[idx]);

					if (!pPointVector)
					{
						continue;
					}

					const float prevDensity = pPointVector->m_PrevDensity;
					const float totalNeigborDensities = GetNeighborDensities(x, y, z);
					
					pPointVector->m_Density = (prevDensity + totalNeigborDensities * a) / (1 + 6 * a);
					
					//if (pPointVector->m_Density > 10.f)
					//{
					//	UE_LOG(LogTemp, Warning, TEXT("Density: %s"), *FString::Printf(TEXT("%f"), pPointVector->m_Density));
					//}
				}
			}
		}
		SetBoundsDiffuse();
	}
}

void AC_GridManager::AdVectDensities(float dt)
{
	const float dt0 = dt * m_GridSize;
	int i{}, j{}, k{}, i1{}, j1{}, k1{};
	float x{}, y{}, z{}, s{}, t{}, u{}, s1{}, t1{}, u1{};

	for (int idxX{ 1 }; idxX <= m_GridSize; idxX++)
	{
		for (int idxY{ 1 }; idxY <= m_GridSize; idxY++)
		{
			for (int idxZ{ 1 }; idxZ <= m_GridSize; idxZ++)
			{
				AC_PointVector* pPointVector = Cast<AC_PointVector>(m_pPointVectors[GetIdx(idxX, idxY, idxZ)]);

				x = AdVectIfChecks(idxX - pPointVector->m_Velocity.X * dt0);
				y = AdVectIfChecks(idxY - pPointVector->m_Velocity.Y * dt0);
				z = AdVectIfChecks(idxZ - pPointVector->m_Velocity.Z * dt0);

				i = static_cast<int>(x);
				i1 = i + 1;

				j = static_cast<int>(y);
				j1 = j + 1;

				k = static_cast<int>(z);
				k1 = k + 1;

				s1 = x - i;
				s = 1.f - s1;

				t1 = y - j;
				t = 1.f - t1;

				u1 = z - k;
				u = 1.f - u1;

				//if (pPointVector->m_Density >= 1.f)
				//{
				//	float test = s + s1;
				//}

				pPointVector->m_Density = AdvectPrevDensityCalculations(i, j, k, i1, j1, k1, s, t, u, s1, t1, u1);
			}
		}
	}

	SetBoundsDiffuse();
}

void AC_GridManager::SwapDensities()
{
	for (AActor* pActor : m_pPointVectors)
	{
		AC_PointVector* pPointVector = Cast<AC_PointVector>(pActor);

		if (pPointVector)
		{
			pPointVector->SwapDensities();
		}
	}
}

float AC_GridManager::GetNeighborDensities(int x, int y, int z)
{
	float totalNeighborDensities{};

	//if (x == m_GridSize)
	//{
	//	std::cout << "Hello\n";
	//}

	//x-1
	int neighborIdx{ GetIdx(x - 1, y ,z) };
	AC_PointVector* pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborDensities += pNeighbor->m_Density;
	}

	//x+1
	neighborIdx = GetIdx(x + 1, y ,z);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborDensities += pNeighbor->m_Density;
	}

	//y-1
	neighborIdx = GetIdx(x, y - 1, z);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborDensities += pNeighbor->m_Density;
	}

	//y+1
	neighborIdx = GetIdx(x, y + 1, z);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborDensities += pNeighbor->m_Density;
	}

	//z-1
	neighborIdx = GetIdx(x, y, z - 1);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborDensities += pNeighbor->m_Density;
	}

	//z+1
	neighborIdx = GetIdx(x, y, z + 1);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborDensities += pNeighbor->m_Density;
	}

	return totalNeighborDensities;
}

void AC_GridManager::SetBoundsDiffuse()
{
	//Z-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int y{ 1 }; y <= m_GridSize; ++y)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x,y,0)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x,y,1)]);

			pPointVectorEdge->m_Density = pPointVectorNeighbor->m_Density;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize + 1)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize)]);

			pPointVectorEdge->m_Density = pPointVectorNeighbor->m_Density;
		}
	}

	//Y-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 0, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 1, z)]);

			pPointVectorEdge->m_Density = pPointVectorNeighbor->m_Density;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize + 1, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize, z)]);

			pPointVectorEdge->m_Density = pPointVectorNeighbor->m_Density;
		}
	}

	//X-edge
	for (int y{ 1 }; y <= m_GridSize; ++y)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, y, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, y, z)]);

			pPointVectorEdge->m_Density = pPointVectorNeighbor->m_Density;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize + 1, y, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, y, z)]);

			pPointVectorEdge->m_Density = pPointVectorNeighbor->m_Density;
		}
	}

	//====================================================================================================
	//CORNERS
	//====================================================================================================

	//(0,0,0)
	AC_PointVector* pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 0)]);

	AC_PointVector* pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, 0)]);
	AC_PointVector* pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, 0)]);
	AC_PointVector* pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 1)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(N-1,0,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, 0	, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize			, 0	, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, 1	, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, 0	, 1)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(0,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, m_RealGridSize - 1	, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1	, m_RealGridSize - 1	, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, m_GridSize	, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, m_RealGridSize - 1	, 1)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(0,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, 0	, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1	, 0	, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, 1	, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, 0	, m_GridSize)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(N-1,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, m_RealGridSize - 1	, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize			, m_RealGridSize - 1	, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, m_GridSize	, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, m_RealGridSize - 1	, 1)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(N-1,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, 0,	m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize			, 0,	m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, 1,	m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, 0,	m_GridSize)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(0,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, m_RealGridSize - 1	, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1	, m_RealGridSize - 1	, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, m_GridSize			, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0	, m_RealGridSize - 1	, m_GridSize)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;

	//(N-1,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, m_RealGridSize - 1	, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize			, m_RealGridSize - 1	, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, m_GridSize			, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1	, m_RealGridSize - 1	, m_GridSize)]);

	pPointVectorCorner->m_Density = (pCornerNeighborX->m_Density + pCornerNeighborY->m_Density + pCornerNeighborZ->m_Density) / 3.f;
}

float AC_GridManager::AdvectPrevDensityCalculations(int i, int j, int k, int i1, int j1, int k1, float s, float t, float u, float s1, float t1, float u1)
{
	const float prevDensity1 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j, k)])->m_PrevDensity;
	const float prevDensity2 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j, k1)])->m_PrevDensity;
	const float prevDensity3 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j1, k)])->m_PrevDensity;
	const float prevDensity4 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j1, k1)])->m_PrevDensity;

	const float prevDensity5 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j, k)])->m_PrevDensity;
	const float prevDensity6 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j, k1)])->m_PrevDensity;
	const float prevDensity7 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j1, k)])->m_PrevDensity;
	const float prevDensity8 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j1, k1)])->m_PrevDensity;

	const float calc1 = s *	(t * (u * prevDensity1
								+ u1 * prevDensity2)
							+ t1 * (u * prevDensity3
								+ u1 * prevDensity4));
	const float calc2 = s1 * (t * (u * prevDensity5
								+ u1 * prevDensity6)
							+ t1 * (u * prevDensity7
								+ u1 * prevDensity8));

	return calc1 + calc2;
}
#pragma endregion

#pragma region Velocity

void AC_GridManager::HandleVelocities(float dt)
{
	SwapVelocities();

	const float a = dt * m_Viscosity * m_GridSize * m_GridSize;
	LinearSolveVelocities(a);

	Project();
	
	SwapVelocities();
	
	AdVectVelocities(dt);
	
	Project();
}

void AC_GridManager::LinearSolveVelocities(float a)
{
	for (int iter{}; iter < m_Iterations; ++iter)
	{
		for (int x{ 1 }; x <= m_GridSize; ++x)
		{
			for (int y{ 1 }; y <= m_GridSize; ++y)
			{
				for (int z{ 1 }; z <= m_GridSize; ++z)
				{
					const int idx{ GetIdx(x,y,z) };
					AC_PointVector* pPointVector = Cast<AC_PointVector>(m_pPointVectors[idx]);

					if (!pPointVector)
					{
						continue;
					}

					const FVector prevVelocity = pPointVector->m_PrevVelocity;
					const FVector totalNeigborVelocities = GetNeighborVelocities(x, y, z);

					pPointVector->m_Velocity = (prevVelocity + totalNeigborVelocities * a) / (1 + 6 * a);
				}
			}
		}
		SetBoundsVelocity();
	}
}

void AC_GridManager::AdVectVelocities(float dt)
{
	const float dt0 = dt * m_GridSize;
	int i{}, j{}, k{}, i1{}, j1{}, k1{};
	float x{}, y{}, z{}, s{}, t{}, u{}, s1{}, t1{}, u1{};

	for (int idxX{ 1 }; idxX <= m_GridSize; idxX++)
	{
		for (int idxY{ 1 }; idxY <= m_GridSize; idxY++)
		{
			for (int idxZ{ 1 }; idxZ <= m_GridSize; idxZ++)
			{
				AC_PointVector* pPointVector = Cast<AC_PointVector>(m_pPointVectors[GetIdx(idxX, idxY, idxZ)]);

				x = AdVectIfChecks(idxX - pPointVector->m_Velocity.X * dt0);
				y = AdVectIfChecks(idxY - pPointVector->m_Velocity.Y * dt0);
				z = AdVectIfChecks(idxZ - pPointVector->m_Velocity.Z * dt0);

				i = static_cast<int>(x);
				i1 = i + 1;

				j = static_cast<int>(y);
				j1 = j + 1;

				k = static_cast<int>(z);
				k1 = k + 1;

				s1 = x - i;
				s = 1.f - s1;

				t1 = y - j;
				t = 1.f - t1;

				u1 = z - k;
				u = 1.f - u1;

				pPointVector->m_Velocity = AdvectPrevVelocityCalculations(i, j, k, i1, j1, k1, s, t, u, s1, t1, u1);
			}
		}
	}

	SetBoundsDiffuse();
}

void AC_GridManager::Project()
{
	const float h = m_GapSize / m_GridSize;

	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int y{ 1 }; y <= m_GridSize; ++y)
		{
			for (int z{ 1 }; z <= m_GridSize; ++z)
			{
				SetDivergence(x, y, z, h);
			}
		}
	}
	SetBoundsDivergence();
	SetBoundsPressure();

	//m_pPointVectors;

	LinearSolvePressure();

	SetProjectedVelocities(h);

	for (AActor* pActor : m_pPointVectors)
	{
		AC_PointVector* pPointVector = Cast<AC_PointVector>(pActor);

		pPointVector->m_PrevVelocity = pPointVector->m_Velocity;
	}
}

void AC_GridManager::SwapVelocities()
{
	for (AActor* pActor : m_pPointVectors)
	{
		AC_PointVector* pPointVector = Cast<AC_PointVector>(pActor);

		if (pPointVector)
		{
			pPointVector->SwapVelocities();
		}
	}
}

FVector AC_GridManager::GetNeighborVelocities(int x, int y, int z)
{
	FVector totalNeighborVelocities{0.f,0.f,0.f};

	//x-1
	int neighborIdx{ GetIdx(x - 1, y ,z) };
	AC_PointVector* pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborVelocities += pNeighbor->m_Velocity;
	}

	//x+1
	neighborIdx = GetIdx(x + 1, y, z);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborVelocities += pNeighbor->m_Velocity;
	}

	//y-1
	neighborIdx = GetIdx(x, y - 1, z);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborVelocities += pNeighbor->m_Velocity;
	}

	//y+1
	neighborIdx = GetIdx(x, y + 1, z);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborVelocities += pNeighbor->m_Velocity;
	}

	//z-1
	neighborIdx = GetIdx(x, y, z - 1);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborVelocities += pNeighbor->m_Velocity;
	}

	//z+1
	neighborIdx = GetIdx(x, y, z + 1);
	pNeighbor = Cast<AC_PointVector>(m_pPointVectors[neighborIdx]);
	if (pNeighbor)
	{
		totalNeighborVelocities += pNeighbor->m_Velocity;
	}

	return totalNeighborVelocities;
}

void AC_GridManager::SetBoundsVelocity()
{
	//Z-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int y{ 1 }; y <= m_GridSize; ++y)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, 0)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, 1)]);

			pPointVectorEdge->m_Velocity.X = pPointVectorNeighbor->m_Velocity.X;
			pPointVectorEdge->m_Velocity.Y = pPointVectorNeighbor->m_Velocity.Y;
			pPointVectorEdge->m_Velocity.Z = -pPointVectorNeighbor->m_Velocity.Z;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize + 1)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize)]);

			pPointVectorEdge->m_Velocity.X = pPointVectorNeighbor->m_Velocity.X;
			pPointVectorEdge->m_Velocity.Y = pPointVectorNeighbor->m_Velocity.Y;
			pPointVectorEdge->m_Velocity.Z = -pPointVectorNeighbor->m_Velocity.Z;
		}
	}

	//Y-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 0, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 1, z)]);

			pPointVectorEdge->m_Velocity.X = pPointVectorNeighbor->m_Velocity.X;
			pPointVectorEdge->m_Velocity.Y = -pPointVectorNeighbor->m_Velocity.Y;
			pPointVectorEdge->m_Velocity.Z = pPointVectorNeighbor->m_Velocity.Z;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize + 1, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize, z)]);

			pPointVectorEdge->m_Velocity.X = pPointVectorNeighbor->m_Velocity.X;
			pPointVectorEdge->m_Velocity.Y = -pPointVectorNeighbor->m_Velocity.Y;
			pPointVectorEdge->m_Velocity.Z = pPointVectorNeighbor->m_Velocity.Z;
		}
	}

	//X-edge
	for (int y{ 1 }; y <= m_GridSize; ++y)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, y, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, y, z)]);

			pPointVectorEdge->m_Velocity.X = -pPointVectorNeighbor->m_Velocity.X;
			pPointVectorEdge->m_Velocity.Y = pPointVectorNeighbor->m_Velocity.Y;
			pPointVectorEdge->m_Velocity.Z = pPointVectorNeighbor->m_Velocity.Z;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize + 1, y, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, y, z)]);

			pPointVectorEdge->m_Velocity.X = -pPointVectorNeighbor->m_Velocity.X;
			pPointVectorEdge->m_Velocity.Y = pPointVectorNeighbor->m_Velocity.Y;
			pPointVectorEdge->m_Velocity.Z = pPointVectorNeighbor->m_Velocity.Z;
		}
	}

	//====================================================================================================
	//CORNERS
	//====================================================================================================

	//(0,0,0)
	AC_PointVector* pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 0)]);

	AC_PointVector* pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, 0)]);
	AC_PointVector* pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, 0)]);
	AC_PointVector* pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 1)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(N-1,0,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, 0, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 1, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, 1)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(0,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, m_RealGridSize - 1, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_GridSize, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, 1)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(0,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, m_GridSize)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(N-1,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, m_RealGridSize - 1, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_GridSize, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, 1)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(N-1,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, 0, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 1, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, m_GridSize)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(0,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, m_RealGridSize - 1, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_GridSize, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, m_GridSize)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;

	//(N-1,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, m_RealGridSize - 1, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_GridSize, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, m_GridSize)]);

	pPointVectorCorner->m_Velocity = (pCornerNeighborX->m_Velocity + pCornerNeighborY->m_Velocity + pCornerNeighborZ->m_Velocity) / 3.f;
}

FVector AC_GridManager::AdvectPrevVelocityCalculations(int i, int j, int k, int i1, int j1, int k1, float s, float t, float u, float s1, float t1, float u1)
{
	const FVector prevDensity1 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j, k)])->m_PrevVelocity;
	const FVector prevDensity2 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j, k1)])->m_PrevVelocity;
	const FVector prevDensity3 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j1, k)])->m_PrevVelocity;
	const FVector prevDensity4 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i, j1, k1)])->m_PrevVelocity;

	const FVector prevDensity5 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j, k)])->m_PrevVelocity;
	const FVector prevDensity6 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j, k1)])->m_PrevVelocity;
	const FVector prevDensity7 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j1, k)])->m_PrevVelocity;
	const FVector prevDensity8 = Cast<AC_PointVector>(m_pPointVectors[GetIdx(i1, j1, k1)])->m_PrevVelocity;

	const FVector calc1 = s * (t * (u * prevDensity1
									+ u1 * prevDensity2)
							+ t1 * (u * prevDensity3
									+ u1 * prevDensity4));
	const FVector calc2 = s1 * (t * (u * prevDensity5
									+ u1 * prevDensity6)
								+ t1 * (u * prevDensity7
									+ u1 * prevDensity8));

	return calc1 + calc2;
}

void AC_GridManager::SetDivergence(int x, int y, int z, float h)
{
	//Set divergence in the y value of the previous velocity, keep x empty for pressure later
	//X
	float equationVelX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x + 1, y, z)])->m_Velocity.X;
	equationVelX -= Cast<AC_PointVector>(m_pPointVectors[GetIdx(x - 1, y, z)])->m_Velocity.X;

	//Y
	float equationVelY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y + 1, z)])->m_Velocity.Y;
	equationVelY -= Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y - 1, z)])->m_Velocity.Y;

	//Z
	float equationVelZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z + 1)])->m_Velocity.Z;
	equationVelZ -= Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z - 1)])->m_Velocity.Z;

	const float divergence = (equationVelX + equationVelY + equationVelZ) * -0.5f * h;

	const FVector prevVel{ Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z)])->m_PrevVelocity };

	FVector newPrevVel{ 0, divergence, prevVel.Z };
	Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z)])->m_PrevVelocity = newPrevVel;
}

void AC_GridManager::SetBoundsDivergence()
{
	//Z-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int y{ 1 }; y <= m_GridSize; ++y)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, 0)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, 1)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize + 1)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;
		}
	}

	//Y-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 0, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 1, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize + 1, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;
		}
	}

	//X-edge
	for (int y{ 1 }; y <= m_GridSize; ++y)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, y, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, y, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize + 1, y, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, y, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;
		}
	}

	//====================================================================================================
	//CORNERS
	//====================================================================================================

	//(0,0,0)
	AC_PointVector* pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 0)]);

	AC_PointVector* pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, 0)]);
	AC_PointVector* pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, 0)]);
	AC_PointVector* pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{	pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,0,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, 0, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 1, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(0,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, m_RealGridSize - 1, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_GridSize, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(0,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, m_RealGridSize - 1, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_GridSize, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, 0, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 1, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(0,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, m_RealGridSize - 1, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_GridSize, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, m_RealGridSize - 1, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_GridSize, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ pPointVectorCorner->m_PrevVelocity.X,
													(pCornerNeighborX->m_PrevVelocity.Y + pCornerNeighborY->m_PrevVelocity.Y + pCornerNeighborZ->m_PrevVelocity.Y) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Z };
}

void AC_GridManager::SetBoundsPressure()
{
	//Z-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int y{ 1 }; y <= m_GridSize; ++y)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, 0)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, 1)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize + 1)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, m_GridSize)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;
		}
	}

	//Y-edge
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 0, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, 1, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize + 1, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, m_GridSize, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;
		}
	}

	//X-edge
	for (int y{ 1 }; y <= m_GridSize; ++y)
	{
		for (int z{ 1 }; z <= m_GridSize; ++z)
		{
			AC_PointVector* pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, y, z)]);
			AC_PointVector* pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, y, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;

			pPointVectorEdge = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize + 1, y, z)]);
			pPointVectorNeighbor = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, y, z)]);

			pPointVectorEdge->m_PrevVelocity = pPointVectorNeighbor->m_PrevVelocity;
		}
	}

	//====================================================================================================
	//CORNERS
	//====================================================================================================

	//(0,0,0)
	AC_PointVector* pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 0)]);

	AC_PointVector* pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, 0)]);
	AC_PointVector* pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, 0)]);
	AC_PointVector* pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,0,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, 0, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 1, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(0,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, m_RealGridSize - 1, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_GridSize, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(0,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, 0, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 1, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, 0, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,N-1,0)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, 0)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, m_RealGridSize - 1, 0)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_GridSize, 0)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, 1)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,0,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, 0, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 1, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, 0, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(0,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(1, m_RealGridSize - 1, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_GridSize, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(0, m_RealGridSize - 1, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };

	//(N-1,N-1,N-1)
	pPointVectorCorner = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, m_RealGridSize - 1)]);

	pCornerNeighborX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_GridSize, m_RealGridSize - 1, m_RealGridSize - 1)]);
	pCornerNeighborY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_GridSize, m_RealGridSize - 1)]);
	pCornerNeighborZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(m_RealGridSize - 1, m_RealGridSize - 1, m_GridSize)]);

	pPointVectorCorner->m_PrevVelocity = FVector{ (pCornerNeighborX->m_PrevVelocity.X + pCornerNeighborY->m_PrevVelocity.X + pCornerNeighborZ->m_PrevVelocity.X) / 3.f,
													pPointVectorCorner->m_PrevVelocity.Y,
													pPointVectorCorner->m_PrevVelocity.Z };
}

void AC_GridManager::LinearSolvePressure()
{
	for (int iter{}; iter < m_Iterations; ++iter)
	{
		for (int x{ 1 }; x <= m_GridSize; ++x)
		{
			for (int y{ 1 }; y <= m_GridSize; ++y)
			{
				for (int z{ 1 }; z <= m_GridSize; ++z)
				{
					const int idx{ GetIdx(x,y,z) };
					AC_PointVector* pPointVector = Cast<AC_PointVector>(m_pPointVectors[idx]);

					//if (idx == 176)
					//{
					//	std::cout << "Test\n";
					//}

					if (!pPointVector)
					{
						continue;
					}

					float totalPrevVelFloat = pPointVector->m_PrevVelocity.Y; //Get div
					//Add neighbor pressures
					//X
					totalPrevVelFloat += Cast<AC_PointVector>(m_pPointVectors[GetIdx(x + 1, y, z)])->m_PrevVelocity.X;
					totalPrevVelFloat += Cast<AC_PointVector>(m_pPointVectors[GetIdx(x - 1, y, z)])->m_PrevVelocity.X;

					//Y
					totalPrevVelFloat += Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y + 1, z)])->m_PrevVelocity.X;
					totalPrevVelFloat += Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y - 1, z)])->m_PrevVelocity.X;

					//Z
					totalPrevVelFloat += Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z + 1)])->m_PrevVelocity.X;
					totalPrevVelFloat += Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z - 1)])->m_PrevVelocity.X;

					pPointVector->m_PrevVelocity.X = totalPrevVelFloat / 6.f;												
				}
			}
		}
		SetBoundsPressure();
	}
}

void AC_GridManager::SetProjectedVelocities(float h)
{
	for (int x{ 1 }; x <= m_GridSize; ++x)
	{
		for (int y{ 1 }; y <= m_GridSize; ++y)
		{
			for (int z{ 1 }; z <= m_GridSize; ++z)
			{
				const int idx{ GetIdx(x,y,z) };

				AC_PointVector* pPointVector = Cast<AC_PointVector>(m_pPointVectors[idx]);

				//X
				float equationVelX = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x + 1, y, z)])->m_PrevVelocity.X;
				equationVelX -= Cast<AC_PointVector>(m_pPointVectors[GetIdx(x - 1, y, z)])->m_PrevVelocity.X;
				equationVelX *= -0.5f;
				equationVelX /= h;

				//Y
				float equationVelY = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y + 1, z)])->m_PrevVelocity.X;
				equationVelY -= Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y - 1, z)])->m_PrevVelocity.X;
				equationVelY *= -0.5f;
				equationVelY /= h;

				//Z
				float equationVelZ = Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z + 1)])->m_PrevVelocity.X;
				equationVelZ -= Cast<AC_PointVector>(m_pPointVectors[GetIdx(x, y, z - 1)])->m_PrevVelocity.X;
				equationVelZ *= -0.5f;
				equationVelZ /= h;

				pPointVector->m_Velocity.X -= equationVelX;
				pPointVector->m_Velocity.Y -= equationVelY;
				pPointVector->m_Velocity.Z -= equationVelZ;
			}
		}
	}
	SetBoundsVelocity();
}

#pragma endregion

#pragma region Helpers

int AC_GridManager::GetIdx(int x, int y, int z)
{
	int xIdx = x * m_RealGridSize * m_RealGridSize;
	int yIdx = y * m_RealGridSize;
	int zIdx = z;

	return xIdx + yIdx + zIdx;
}

float AC_GridManager::AdVectIfChecks(float value)
{
	if (value < 0.5f) value = 0.5f;
	if (value > m_GridSize + 0.5f) value = m_GridSize + 0.5f;

	return value;
}
#pragma endregion

// Called every frame
void AC_GridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleVelocities(DeltaTime);
	HandleDensities(DeltaTime);
}

