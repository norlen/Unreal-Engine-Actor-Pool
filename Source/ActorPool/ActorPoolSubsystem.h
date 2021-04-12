// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

class AActorPoolCharacter;

/**
 * 
 */
UCLASS()
class ACTORPOOL_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	// If this subsystem should be created. If we return false we must check when getting the subsystem that is not `nullptr`.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Initialize the pool with instances our desired class.
	UFUNCTION(BlueprintCallable)
	void InitializePool(TSubclassOf<AActorPoolCharacter> InClassToSpawn);

	// Returns a character from the pool with the actors location set to `Location`.
	UFUNCTION(BlueprintCallable)
	AActorPoolCharacter* Spawn(const FVector Location);

	// Returns the actor to the pool.
	UFUNCTION(BlueprintCallable)
	void Despawn(AActorPoolCharacter* Character);

	UFUNCTION(BlueprintCallable)
    void PrintStats() const;

protected:
	// Spawns the actual actor used by the pool.
	AActor* SpawnActor() const;
	
	// The actor class we want to spawn.
	UPROPERTY(Transient)
	TSubclassOf<AActor> ClassToSpawn;
	
	// The actual pool of actors, which are ready to be handed out.
	UPROPERTY(Transient)
	TArray<AActor*> PooledObjects;
	
	// How many actors we want to spawn into the pool.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool")
	int32 NumActorsToPool = 2000;

	// How many objects to spawn per frame.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	int32 SpawnPerFrame = 100;
	
	// Initialize the pool by spawning all the actors.
	void InitializePoolInternal(TSubclassOf<AActor> InClassToSpawn);

	// Initial spawning for the actor pool. If SpawnPerFrame is set to zero all actors will be spawned in a single
	// frame, otherwise it will spawn that amount per frame until it is done.
	UFUNCTION()
	void PopulatePool();
	
	// Tries to retrieve a single actor from the pool. If no more actors exist, it will return `nullptr`. 
	UFUNCTION(BlueprintCallable)
    AActor* GetActorFromPool();

	// Returns the actor to the pool. Note that this should be called instead of `Destroy`.
	UFUNCTION(BlueprintCallable)
    void ReturnActorToPool(AActor* Actor);

	// When the number of actors in the pool go below this level, start replenishing the pool. Set to zero to disable.
	int32 MinActorsInPool = 1500;

	// How many additional objects should be spawned per frame when the pool goes below `MinActorsInPool`.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	int32 SpawnAdditionalPerFrame = 1;

	// Spawn additional actors into the pool per frame until `MinActorsInPool` is reached.
	UFUNCTION()
	void ReplenishPool();

	// -----------------------------------------------------------------------------------------------------------------
	// Statistics
	// -----------------------------------------------------------------------------------------------------------------

	// How many actors that currently reside in the pool.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stats")
	int32 ActorsInPool = 0;
	
	// How many actors are currently handed out by the pool.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stats")
	int32 CurrentActorsSpawned = 0;

	// How many actors have been handed out at most at a certain time.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stats")
	int32 MaxActorsSpawned = 0;

	// How many actors have been handed out in total.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stats")
	int32 TotalActorsSpawned = 0;

	// How many actors have been returned to the pool.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stats")
	int32 TotalActorsDespawned = 0;

	// How many actors in total the pool has spawned.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stats")
	int32 TotalActorsSpawnedBySystem = 0;
};
