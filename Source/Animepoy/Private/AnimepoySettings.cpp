// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimepoySettings.h"
#include "AnimepoyWorldSubsystem.h"

// Sets default values
AAnimepoySettings::AAnimepoySettings()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AAnimepoySettings::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GetWorld();
	if (World)
	{
		UAnimepoyWorldSubsystem* WorldSubsystem = World->GetSubsystem<UAnimepoyWorldSubsystem>();

		if (WorldSubsystem)
		{
			WorldSubsystem->OnActorSpawned(this);
		}
	}
}

void AAnimepoySettings::BeginDestroy()
{
	const UWorld* World = GetWorld();
	if (World)
	{
		UAnimepoyWorldSubsystem* WorldSubsystem = World->GetSubsystem<UAnimepoyWorldSubsystem>();

		if (WorldSubsystem)
		{
			WorldSubsystem->OnActorDeleted(this);
		}
	}

	Super::BeginDestroy();
}
