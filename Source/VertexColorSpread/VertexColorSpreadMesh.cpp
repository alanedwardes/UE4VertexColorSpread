
#include "VertexColorSpread.h"
#include "StaticMeshResources.h"
#include "VertexColorSpreadMesh.h"

AVertexColorSpreadMesh::AVertexColorSpreadMesh(const class FObjectInitializer& ObjectInitialiser)
	: Super(ObjectInitialiser)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	ColorSpreadComponent = ObjectInitialiser.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("ColorSpreadMesh"));
	RootComponent = ColorSpreadComponent;

	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, TEXT("TakePointDamageDelegate"));
	OnTakePointDamage.AddUnique(Delegate);

	Distance = 50.f;
	TriggerIntensity = .2f;
	Interval = .1f;
	bActive = true;
	bTimerActive = false;
}

void AVertexColorSpreadMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bActive && !bTimerActive)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AVertexColorSpreadMesh::Spread, Interval, false);
		bTimerActive = true;
	}
}

void AVertexColorSpreadMesh::Spread()
{
	if (!bActive)
	{
		bTimerActive = false;
		return;
	}

	// Init the buffers and LOD data
	InitialiseLODInfoAndBuffers();

	FStaticMeshLODResources& LODModel = ColorSpreadComponent->StaticMesh->RenderData->LODResources[0];
	FStaticMeshComponentLODInfo* InstanceMeshLODInfo = &ColorSpreadComponent->LODData[0];

	BuildAdjacencyCache(LODModel);

	// Start our spread code
	SpreadIntenseColors(InstanceMeshLODInfo, LODModel);

	// Notify the render thread about the buffer change
	BeginUpdateResourceRHI(InstanceMeshLODInfo->OverrideVertexColors);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVertexColorSpreadMesh::Spread, Interval, false);
}

void AVertexColorSpreadMesh::InitialiseInstancedOverrideVertexColorBuffer(FStaticMeshComponentLODInfo* InstanceMeshLODInfo, FStaticMeshLODResources& LODModel)
{
	// Check that we don't already have an overridden vertex color buffer
	check(InstanceMeshLODInfo->OverrideVertexColors == nullptr);

	// Create a new buffer
	InstanceMeshLODInfo->OverrideVertexColors = new FColorVertexBuffer;

	if ((int32)LODModel.ColorVertexBuffer.GetNumVertices() >= LODModel.GetNumVertices())
	{
		// If the mesh already has vertex colours, initialise OverrideVertexColors from them
		InstanceMeshLODInfo->OverrideVertexColors->InitFromColorArray(&LODModel.ColorVertexBuffer.VertexColor(0), LODModel.GetNumVertices());
	}
	else
	{
		// If it doesn't, set all overridden vert colours to black
		InstanceMeshLODInfo->OverrideVertexColors->InitFromSingleColor(FColor::Black, LODModel.GetNumVertices());
	}

	BeginInitResource(InstanceMeshLODInfo->OverrideVertexColors);
	ColorSpreadComponent->MarkRenderStateDirty();
}

void AVertexColorSpreadMesh::SpreadIntenseColors(FStaticMeshComponentLODInfo* InstanceMeshLODInfo, FStaticMeshLODResources& LODModel)
{
	for (int32 i = 0; i < LODModel.GetNumVertices(); i++)
	{
		auto Intensity = GetIntensity(InstanceMeshLODInfo->OverrideVertexColors->VertexColor(i));

		// If the value is greater than the threshold, spread the colour
		if (Intensity >= TriggerIntensity * 255)
		{
			SpreadColorFromVert(i, InstanceMeshLODInfo, LODModel);
		}
	}
}

void AVertexColorSpreadMesh::SpreadColorFromVert(int32 StartVertIndex, FStaticMeshComponentLODInfo* InstanceMeshLODInfo, FStaticMeshLODResources& LODModel)
{
	auto LocalToWorld = ColorSpreadComponent->ComponentToWorld.ToMatrixWithScale();;

	auto StartPosition = LODModel.PositionVertexBuffer.VertexPosition(StartVertIndex);
	if (!Particles.Contains(StartVertIndex))
	{
		auto WorldVertexPosition = LocalToWorld.TransformPosition(StartPosition);
		auto Particle = UGameplayStatics::SpawnEmitterAttached(IntenseParticleSystem, ColorSpreadComponent, NAME_None, WorldVertexPosition, FRotator(.0f, .0f, .0f), EAttachLocation::KeepWorldPosition);
		Particles.Add(StartVertIndex, Particle);
	}

	auto AdjacentVerts = GetAdjacentVertIndexes(StartVertIndex);
	for (auto Vert : AdjacentVerts)
	{
		auto Intensity = GetIntensity(InstanceMeshLODInfo->OverrideVertexColors->VertexColor(Vert));
		if (Intensity < 255)
		{
			SetIntensity(&InstanceMeshLODInfo->OverrideVertexColors->VertexColor(Vert), Intensity + 1);
		}
	}
}

uint8 AVertexColorSpreadMesh::GetIntensity(FColor Color)
{
	switch (Channel)
	{
	case AVertexColorSpreadChannel::Red:
		return Color.R;
	case AVertexColorSpreadChannel::Green:
		return Color.G;
	case AVertexColorSpreadChannel::Blue:
		return Color.B;
	case AVertexColorSpreadChannel::Alpha:
		return Color.A;
	default:
		return 0;
	}
}

int32 AVertexColorSpreadMesh::GetNearestVertIndex(FVector Position, FStaticMeshLODResources& LODModel)
{
	auto ShortestDistance = -1;
	auto NearestVertexIndex = -1;
	auto LocalToWorld = ColorSpreadComponent->ComponentToWorld.ToMatrixWithScale();

	for (auto i = 0; i < LODModel.GetNumVertices(); i++)
	{
		auto LocalVertexPosition = LODModel.PositionVertexBuffer.VertexPosition(i);
		auto WorldVertexPosition = LocalToWorld.TransformPosition(LocalVertexPosition);
		auto Distance = FVector::DistSquared(WorldVertexPosition, Position);

		if (Distance < ShortestDistance || ShortestDistance < 0)
		{
			ShortestDistance = Distance;
			NearestVertexIndex = i;
		}
	}

	return NearestVertexIndex;
}

void AVertexColorSpreadMesh::TakePointDamageDelegate(float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	// Init the buffers and LOD data
	InitialiseLODInfoAndBuffers();

	FStaticMeshComponentLODInfo* InstanceMeshLODInfo = &ColorSpreadComponent->LODData[0];
	FStaticMeshLODResources& LODModel = ColorSpreadComponent->StaticMesh->RenderData->LODResources[0];

	auto VertIndex = GetNearestVertIndex(HitLocation, LODModel);

	if (VertIndex < 0)
		return;

	if (VertIndex >= (int)InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices())
		return;

	auto Intensity = GetIntensity(InstanceMeshLODInfo->OverrideVertexColors->VertexColor(VertIndex));

	SetIntensity(&InstanceMeshLODInfo->OverrideVertexColors->VertexColor(VertIndex), FMath::Min(Intensity + Damage, 255.0f));

	// Notify the render thread about the buffer change
	BeginUpdateResourceRHI(InstanceMeshLODInfo->OverrideVertexColors);
}

void AVertexColorSpreadMesh::InitialiseLODInfoAndBuffers()
{
	if (ColorSpreadComponent->LODData.Num() == 0)
	{
		ColorSpreadComponent->SetLODDataCount(1, ColorSpreadComponent->LODData.Num());
	}

	FStaticMeshLODResources& LODModel = ColorSpreadComponent->StaticMesh->RenderData->LODResources[0];
	FStaticMeshComponentLODInfo* InstanceMeshLODInfo = &ColorSpreadComponent->LODData[0];

	if (InstanceMeshLODInfo->OverrideVertexColors == nullptr)
	{
		InitialiseInstancedOverrideVertexColorBuffer(InstanceMeshLODInfo, LODModel);
	}
}

void AVertexColorSpreadMesh::SetIntensity(FColor *Color, uint8 Intensity)
{
	switch (Channel)
	{
	case AVertexColorSpreadChannel::Red:
		Color->R = Intensity;
		break;
	case AVertexColorSpreadChannel::Green:
		Color->G = Intensity;
		break;
	case AVertexColorSpreadChannel::Blue:
		Color->B = Intensity;
		break;
	case AVertexColorSpreadChannel::Alpha:
		Color->A = Intensity;
		break;
	}
}

TArray<int32> AVertexColorSpreadMesh::GetAdjacentVertIndexes(int32 StartVertIndex)
{
	TArray<int32> AdjacentArray;
	AdjacencyCache.MultiFind(StartVertIndex, AdjacentArray);
	return AdjacentArray;
}

void AVertexColorSpreadMesh::BuildAdjacencyCache(FStaticMeshLODResources& LODModel)
{
	if (AdjacencyCache.Num() != 0)
		return;

	for (auto StartVertex = 0; StartVertex < LODModel.GetNumVertices(); StartVertex++)
	{
		auto StartPosition = LODModel.PositionVertexBuffer.VertexPosition(StartVertex);
		for (auto EndVertex = 0; EndVertex < LODModel.GetNumVertices(); EndVertex++)
		{
			if (StartVertex == EndVertex)
				continue;
			
			auto EndPosition = LODModel.PositionVertexBuffer.VertexPosition(EndVertex);
			if (FVector::Dist(StartPosition, EndPosition) <= Distance)
			{
				AdjacencyCache.Add(StartVertex, EndVertex);
			}
		}
	}
}