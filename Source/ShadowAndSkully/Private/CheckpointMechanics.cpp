// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckpointMechanics.h"

// Sets default values
ACheckpointMechanics::ACheckpointMechanics()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	auto BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("Collider"));
	BoxCollider->RegisterComponentWithWorld(GetWorld());
	BoxCollider->SetBoxExtent(CollisionScale);
	RootComponent = BoxCollider;

}

// Called when the game starts or when spawned
void ACheckpointMechanics::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACheckpointMechanics::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

