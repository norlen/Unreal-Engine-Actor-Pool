// Fill out your copyright notice in the Description page of Project Settings.

#include "ActorPoolSubsystem.h"
#include "ActorPool.h"
#include "ActorPoolCharacter.h"
#include <algorithm>

void UActorPoolSubsystem::PrintStats() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Red, FString::Printf(TEXT("Actors in pool: %d"), ActorsInPool));
		GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, FString::Printf(TEXT("Actors currently spawned: %d"), CurrentActorsSpawned));
		GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Red, FString::Printf(TEXT("Max actors spawned: %d"), MaxActorsSpawned));
		GEngine->AddOnScreenDebugMessage(3, 5.f, FColor::Red, FString::Printf(TEXT("Total actors spawned: %d"), TotalActorsSpawned) );
		GEngine->AddOnScreenDebugMessage(4, 5.f, FColor::Red, FString::Printf(TEXT("Actors returned to pool: %d"), TotalActorsDespawned));
		GEngine->AddOnScreenDebugMessage(5, 5.f, FColor::Red, FString::Printf(TEXT("Total actors spawned: %d"), TotalActorsSpawnedBySystem));
	}
}

void UActorPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// We initialize the pool in a separate call, so nothing is really needed here.
	UE_LOG(LogActorPool, Log, TEXT("Initializing actor pool subsystem"));
}

void UActorPoolSubsystem::Deinitialize()
{
	UE_LOG(LogActorPool, Log, TEXT("Deinitializing actor pool subsystem"));

	for (AActor* Actor : PooledObjects)
	{
		if (Actor && !Actor->IsPendingKill())
		{
			Actor->Destroy();
		}
	}
}

bool UActorPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UActorPoolSubsystem::InitializePool(TSubclassOf<AActorPoolCharacter> InClassToSpawn)
{
	// Just pass it along to the internal implementation.
	InitializePoolInternal(TSubclassOf<AActor>(InClassToSpawn));
}

AActorPoolCharacter* UActorPoolSubsystem::Spawn(const FVector Location)
{
	AActorPoolCharacter* Character = Cast<AActorPoolCharacter>(GetActorFromPool());
	if (Character)
	{
		Character->SetActorLocation(Location);
		Character->Activate();
	}
	return Character;
}

void UActorPoolSubsystem::Despawn(AActorPoolCharacter* Character)
{
	Character->Deactivate();
	ReturnActorToPool(Character);
}

AActor* UActorPoolSubsystem::SpawnActor() const
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Actor = GetWorld()->SpawnActor(ClassToSpawn, &FVector::ZeroVector, &FRotator::ZeroRotator, SpawnParams);
	AActorPoolCharacter* Character = Cast<AActorPoolCharacter>(Actor);
	if (Character)
	{
		Character->Deactivate();
	}
	return Actor;
}

void UActorPoolSubsystem::InitializePoolInternal(TSubclassOf<AActor> InClassToSpawn)
{
	if (InClassToSpawn == nullptr)
	{
		UE_LOG(LogActorPool, Warning, TEXT("Invalid or no Class passed to initialize pool"));
	}
	ClassToSpawn = InClassToSpawn;

	PopulatePool();
}

void UActorPoolSubsystem::PopulatePool()
{
	// How many actors we have spawned in total.
	const int32 LeftToSpawn = NumActorsToPool - TotalActorsSpawnedBySystem;

	// How many we should spawn this frame. If SpawnPerFrame is set to zero we spawn all actors in one frame.
	int32 SpawnThisFrame = NumActorsToPool;
	if (SpawnPerFrame != 0)
	{
		SpawnThisFrame = std::min(SpawnPerFrame, LeftToSpawn);
	}
	
	for (int i = 0; i < SpawnThisFrame; i++)
	{
		AActor* Actor = SpawnActor();
		if (Actor)
		{
			ActorsInPool += 1;
			TotalActorsSpawnedBySystem += 1;
			PooledObjects.Add(Actor);
		}
	}

	// Check if we have spawned all the actors we need.
	if (TotalActorsSpawnedBySystem < NumActorsToPool)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorPoolSubsystem::PopulatePool);
	}
}

AActor* UActorPoolSubsystem::GetActorFromPool()
{
	AActor* Actor = nullptr;
	if (PooledObjects.Num() > 0)
	{
		Actor = PooledObjects.Pop();

		ActorsInPool -= 1;
		CurrentActorsSpawned += 1;
		TotalActorsSpawned += 1;
		if (CurrentActorsSpawned > MaxActorsSpawned)
		{
			MaxActorsSpawned = CurrentActorsSpawned;
		}
	}

	// Number of actors cannot go below 0, but make it clear that this only
	// runs if `MinActorsInPool` is set to a non-zero value.
	if (PooledObjects.Num() < MinActorsInPool)
	{
		ReplenishPool();
	}
	return Actor;
}

void UActorPoolSubsystem::ReturnActorToPool(AActor* Actor)
{
	// Check that is is valid, and that no one has called `Destroy` on it.
	if (Actor && !Actor->IsPendingKill())
	{
		ActorsInPool += 1;
		TotalActorsDespawned += 1;
		CurrentActorsSpawned -= 1;
		
		PooledObjects.Push(Actor);
	}
}

void UActorPoolSubsystem::ReplenishPool()
{
	const int32 LeftToSpawn = MinActorsInPool - PooledObjects.Num();
	const int32 SpawnThisFrame = std::min(SpawnAdditionalPerFrame, LeftToSpawn);
	
	for (int i = 0; i < SpawnThisFrame; i++)
	{
		AActor* Actor = SpawnActor();
		if (Actor)
		{
			ActorsInPool += 1;
			TotalActorsSpawnedBySystem += 1;
			PooledObjects.Add(Actor);
		}
	}

	// Check if we have spawned all the actors we need.
	if (TotalActorsSpawnedBySystem < NumActorsToPool)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorPoolSubsystem::ReplenishPool);
	}
}
