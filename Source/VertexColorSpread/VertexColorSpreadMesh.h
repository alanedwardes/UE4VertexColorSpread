#pragma once

#include "GameFramework/Actor.h"
#include "VertexColorSpreadMesh.generated.h"

/**
Channel used for spreading vertex colors.
*/
UENUM()
namespace AVertexColorSpreadChannel
{
	enum Channel
	{
		Red,
		Green,
		Blue,
		Alpha,
	};
}

/**
A class capable of spreading intense vertex colors, designed for use with shaders that can leverage the data.
*/
UCLASS(Experimental)
class AVertexColorSpreadMesh : public AActor
{
	GENERATED_UCLASS_BODY()

	/**
	Actor tick.
	*/
	virtual void Tick(float DeltaTime) override;

	/**
	Spread the vertex colors.
	*/
	void Spread();

	/**
	Create LOD info entires, and ensure the override vertex color buffer exists.
	*/
	void InitialiseLODInfoAndBuffers();

	/**
	Create a new override vertex color buffer.
	*/
	void InitialiseInstancedOverrideVertexColorBuffer(FStaticMeshComponentLODInfo* InstanceMeshLODInfo, FStaticMeshLODResources& LODModel);

	/**
	Spread intense vertex colors out.
	*/
	void SpreadIntenseColors(FStaticMeshComponentLODInfo* InstanceMeshLODInfo, FStaticMeshLODResources& LODModel);

	/**
	Spread color from the specified vert.
	*/
	void SpreadColorFromVert(int32 StartVertIndex, FStaticMeshComponentLODInfo* InstanceMeshLODInfo, FStaticMeshLODResources& LODModel);

	/**
	Get the intensity of the selected channel (see Channel)
	*/
	int32 GetNearestVertIndex(FVector Position, FStaticMeshLODResources& LODModel);

	/**
	Get the intensity of the selected channel (see Channel)
	*/
	uint8 GetIntensity(FColor Color);

	/**
	Set the intensity of the selected channel to the given value (see Channel)
	*/
	void SetIntensity(FColor *Color, uint8 Intensity);

	/**
	Build a list of adjacent vertices (using the adjacency cache)
	*/
	TArray<int32> GetAdjacentVertIndexes(int32 StartVertIndex);

	/**
	Build an adjacent vertex cache to speed up spreading calculations.
	*/
	void BuildAdjacencyCache(FStaticMeshLODResources& LODModel);

	/**
	The adjacency cache store.
	*/
	TMultiMap<int32, int32> AdjacencyCache;

	/**
	The mesh with spreadable vertex colors.
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ColorSpread)
	UStaticMeshComponent* ColorSpreadComponent;

	/**
	The particle to spawn when vertices reach a certain intensity
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ParticleSystem)
	class UParticleSystem *IntenseParticleSystem;

	/**
	A list of spawned particles associated with verts
	*/
	TMap<int32, class UParticleSystemComponent*> Particles;

	/**
	Whether the spreading is enabled or disabled.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ColorSpread)
	bool bActive;

	/**
	Interval between spreading in seconds. Directly affects the interval at which the code is run.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ColorSpread)
	float Interval;

	/**
	Intensity needed to trigger the color to bleed to adjacent vertices (0..1).
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ColorSpread)
	float TriggerIntensity;

	/**
	Distance to spread the color when the threshold is met. This will depend on how closely packed the vertices are.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ColorSpread)
	float Distance;

	/**
	Which channel to spread
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ColorSpread)
	TEnumAsByte<AVertexColorSpreadChannel::Channel> Channel;

	/**
	When point damage is taken.
	*/
	UFUNCTION()
	void TakePointDamageDelegate(float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);

	/**
	Internal setting, whether the timer is current active.
	*/
	bool bTimerActive;
};
