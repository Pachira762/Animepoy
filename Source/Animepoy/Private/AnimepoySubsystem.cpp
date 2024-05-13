// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimepoySubsystem.h"
#include "EngineUtils.h"
#include "Animepoy.h"
#include "AnimepoySceneViewExtension.h"

void UAnimepoySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AnimepoySceneViewExtension = FSceneViewExtensions::NewExtension<FAnimepoySceneViewExtension>(this);

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		GEngine->OnLevelActorAdded().AddUObject(this, &UAnimepoySubsystem::OnActorSpawned);
		GEngine->OnLevelActorDeleted().AddUObject(this, &UAnimepoySubsystem::OnActorDeleted);
		GEngine->OnLevelActorListChanged().AddUObject(this, &UAnimepoySubsystem::OnActorListChanged);
	}
#endif
}

void UAnimepoySubsystem::Deinitialize()
{
	Super::Deinitialize();

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		GEngine->OnLevelActorAdded().RemoveAll(this);
		GEngine->OnLevelActorDeleted().RemoveAll(this);
		GEngine->OnLevelActorListChanged().RemoveAll(this);
	}
#endif
}

void UAnimepoySubsystem::OnActorSpawned(AActor* Actor)
{
	AAnimepoy* AsAnimepoy = Cast<AAnimepoy>(Actor);
	if (AsAnimepoy)
	{
		Animepoy = AsAnimepoy;
	}
}

void UAnimepoySubsystem::OnActorDeleted(AActor* Actor)
{
	if ((AActor*)Animepoy == Actor)
	{
		Animepoy = nullptr;
	}
}

void UAnimepoySubsystem::OnActorListChanged()
{
	for (TActorIterator<AAnimepoy> It(GetWorld()); It; ++It)
	{
		Animepoy = *It;
		break;
	}
}
