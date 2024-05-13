// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Animepoy.h"
#include "AnimepoySubsystem.generated.h"

UCLASS()
class ANIMEPOY_API UAnimepoySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection)override;

	virtual void Deinitialize()override;

public:
	void OnActorSpawned(AActor* Actor);

	void OnActorDeleted(AActor* Actor);

	void OnActorListChanged();

	void SetAnimepoyRenderProxy(const FAnimepoyRenderProxy& NewAnimepoyRenderProxy)
	{
		FScopeLock Lock(&Mutex);
		AnimepoyRenderProxy = NewAnimepoyRenderProxy;
	}

	FAnimepoyRenderProxy GetAnimepoyRenderProxy() const
	{
		FScopeLock Lock(&Mutex);
		return AnimepoyRenderProxy;
	}

private:
	TSharedPtr<class FAnimepoySceneViewExtension, ESPMode::ThreadSafe> AnimepoySceneViewExtension;

	AAnimepoy* Animepoy;

	mutable FCriticalSection Mutex;

	FAnimepoyRenderProxy AnimepoyRenderProxy = {};
};
